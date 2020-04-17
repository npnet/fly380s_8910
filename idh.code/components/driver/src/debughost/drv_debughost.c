/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "drv_config.h"
#include "drv_debughost.h"
#include "drv_host_cmd.h"
#include "drv_ifc.h"
#include "hwregs.h"
#include "hal_hw_map.h"
#include "osi_api.h"
#include "osi_log.h"
#include "osi_byte_buf.h"
#include "osi_fifo.h"
#include "osi_trace.h"
#include <string.h>
#include <assert.h>
#include <errno.h>

// Timeout to wait tx done. This timeout is to handle the case that
// debughost TX is blocked, and avoid infinite wait. It should be
// large enough for normal case.
#define TX_DONE_WAIT_TIMEOUT (500)

// Timeout for TX fifo write. This timeout is to handle the case that
// debughost TX is blocked, and avoid infinite wait. The timeout value
// is the time there are no TX room.
#define TX_FIFO_WRITE_TIMEOUT_US (200)

#ifdef CONFIG_SOC_8910
#define IRQN_DEBUG_UART HAL_SYSIRQ_NUM(SYS_IRQ_ID_DEBUG_UART)
#else
#define IRQN_DEBUG_UART SYS_IRQ_DEBUG_UART
#endif

#ifdef CONFIG_DEBUGHOST_RX_USE_IFC
#if defined(CONFIG_SOC_8955) || defined(CONFIG_SOC_8909)
static uint8_t gDhostRxDmaBuf[CONFIG_DEBUGHOST_RX_DMA_SIZE] OSI_SECTION_RAM_UC_BSS OSI_ALIGNED(4);
#else
static uint8_t gDhostRxDmaBuf[CONFIG_DEBUGHOST_RX_DMA_SIZE] OSI_CACHE_LINE_ALIGNED;
#endif
#endif

// MIPS should use OSI_KSEG1
#ifdef CONFIG_CPU_MIPS
#define DHOST_DCACHE_INV(address, size)
#else
#define DHOST_DCACHE_INV(address, size) osiDCacheInvalidate(address, size)
#endif

struct drvDhostRxContex
{
    uint8_t rx_buf[CONFIG_DEBUGHOST_RX_BUF_SIZE];
    osiFifo_t rx_fifo;
#ifdef CONFIG_DEBUGHOST_RX_USE_IFC
    drvIfcChannel_t rx_ifc;
#endif
    osiWork_t *rx_work;
    drvHostCmdEngine_t *cmd;
};

struct drvDhostTxContex
{
    drvIfcChannel_t tx_ifc;
    osiSemaphore_t *tx_sema;
    osiSemaphore_t *tx_done_sema;
};

struct drvDhostPmContext
{
    osiPmSource_t *pm_source;
};

typedef struct drvDhostContex
{
    struct drvDhostRxContex rx;
    struct drvDhostTxContex tx;
    struct drvDhostPmContext pm;
    unsigned ctrl_val;
    unsigned irq_mask_val;
    unsigned triggers_val;
} drvDhostContext_t;

static drvDhostContext_t gDhostCtx;

static int prvDhostFifoRead(void *data, unsigned size)
{
    REG_DEBUG_UART_STATUS_T status = {hwp_debugUart->status};
    int bytes = status.b.rx_fifo_level;
    if (bytes > size)
        bytes = size;

    uint8_t *data8 = (uint8_t *)data;
    for (int n = 0; n < bytes; n++)
        *data8++ = hwp_debugUart->rxtx_buffer;
    return bytes;
}

static int prvDhostFifoWrite(const void *data, unsigned size)
{
    REG_DEBUG_UART_STATUS_T status = {hwp_debugUart->status};
    uint32_t tx_fifo_level = status.b.tx_fifo_level;
    if (tx_fifo_level >= DEBUG_UART_TX_FIFO_SIZE)
        return 0;

    int bytes = DEBUG_UART_TX_FIFO_SIZE - tx_fifo_level;
    if (bytes > size)
        bytes = size;

    const uint8_t *data8 = (const uint8_t *)data;
    for (int n = 0; n < bytes; n++)
        hwp_debugUart->rxtx_buffer = *data8++;
    return bytes;
}

static void prvDhostFifoWriteAll(const void *data, unsigned size)
{
    osiElapsedTimer_t timer;
    osiElapsedTimerStart(&timer);

    while (size > 0)
    {
        if (osiElapsedTimeUS(&timer) > TX_FIFO_WRITE_TIMEOUT_US)
            return;

        int wsize = prvDhostFifoWrite(data, size);
        if (wsize > 0)
        {
            size -= wsize;
            data = (const char *)data + wsize;
            osiElapsedTimerStart(&timer);
        }
    }
}

static void prvDhostRxProc(void *param)
{
    drvDhostContext_t *d = (drvDhostContext_t *)param;
    char buf[64];

    for (;;)
    {
        int bytes = osiFifoGet(&d->rx.rx_fifo, buf, 64);
        if (bytes <= 0)
            break;

        drvHostCmdPushData(d->rx.cmd, buf, bytes);
    }
}

static void prvDhostRxISR(drvDhostContext_t *d)
{
    int bytes = 0;

#ifdef CONFIG_DEBUGHOST_RX_USE_IFC
    if (drvIfcReady(&d->rx.rx_ifc))
    {
        drvIfcFlush(&d->rx.rx_ifc);

        unsigned tc = drvIfcGetTC(&d->rx.rx_ifc);
        if (tc < CONFIG_DEBUGHOST_RX_DMA_SIZE)
        {
            bytes = CONFIG_DEBUGHOST_RX_DMA_SIZE - tc;
            DHOST_DCACHE_INV(gDhostRxDmaBuf, bytes);
            osiFifoPut(&d->rx.rx_fifo, gDhostRxDmaBuf, bytes);
        }

        drvIfcStart(&d->rx.rx_ifc, gDhostRxDmaBuf, CONFIG_DEBUGHOST_RX_DMA_SIZE);
    }
#else
    uint8_t buf[DEBUG_UART_RX_FIFO_SIZE];
    REG_DEBUG_UART_STATUS_T status = {hwp_debugUart->status};
    bytes = OSI_MIN(int, DEBUG_UART_RX_FIFO_SIZE, status.b.rx_fifo_level);

    if (bytes > 0)
    {
        for (int n = 0; n < bytes; n++)
            buf[n] = hwp_debugUart->rxtx_buffer;
        osiFifoPut(&d->rx.rx_fifo, buf, bytes);
    }
#endif

    if (bytes > 0)
    {
        OSI_LOGD(0, "debughost: get %d bytes", bytes);
        osiWorkEnqueue(d->rx.rx_work, osiSysWorkQueueLowPriority());
    }
}

static void prvDhostISR(void *param)
{
    drvDhostContext_t *d = (drvDhostContext_t *)param;
    REG_DEBUG_UART_IRQ_CAUSE_T cause = {hwp_debugUart->irq_cause};
    hwp_debugUart->irq_cause = cause.v; // clear tx_dma_done, rx_dma_done, rx_dma_timeout

    if (cause.b.tx_dma_done)
        osiSemaphoreRelease(d->tx.tx_done_sema);

    if (cause.b.rx_dma_done || cause.b.rx_dma_timeout ||
        cause.b.rx_data_available || cause.b.rx_timeout)
        prvDhostRxISR(d);
}

static void prvHostCmdSend(void *ctx, uint8_t *packet, unsigned packet_len)
{
    drvDhostSendAll(packet, packet_len);
}

bool drvDhostSendAll(const void *data, unsigned size)
{
    drvDhostContext_t *d = &gDhostCtx;

    REG_DEBUG_UART_STATUS_T status  = {hwp_debugUart->status};
    if (status.b.tx_fifo_level != 0 && !status.b.tx_active)
    {
        // debug host maybe blocked
        return false;
    }

    osiSemaphoreAcquire(d->tx.tx_sema);
    osiPmWakeLock(d->pm.pm_source);

    // in case the finish semaphore isn't taken by previous call
    osiSemaphoreTryAcquire(d->tx.tx_done_sema, 0);

    osiDCacheClean(data, size);
    drvIfcStart(&d->tx.tx_ifc, data, size);

    bool sent = osiSemaphoreTryAcquire(d->tx.tx_done_sema, TX_DONE_WAIT_TIMEOUT);
    if (!sent)
        drvIfcStop(&d->tx.tx_ifc);

    osiPmWakeUnlock(d->pm.pm_source);
    osiSemaphoreRelease(d->tx.tx_sema);
    return sent;
}

void drvDhostInit(void)
{
    hwp_debugUart->irq_mask = 0;

    REG_DEBUG_UART_TRIGGERS_T triggers = {};
    triggers.b.rx_trigger = 7;
    triggers.b.tx_trigger = 12;
    triggers.b.afc_level = 10;
    hwp_debugUart->triggers = triggers.v;

    REG_DEBUG_UART_CTRL_T ctrl = {};
    ctrl.b.enable = 1;
    ctrl.b.data_bits = DEBUG_UART_DATA_BITS_V_8_BITS;
    ctrl.b.parity_enable = DEBUG_UART_PARITY_ENABLE_V_NO;
    ctrl.b.tx_stop_bits = DEBUG_UART_TX_STOP_BITS_V_1_BIT;
    ctrl.b.tx_break_control = 0;
    ctrl.b.rx_fifo_reset = 1;
    ctrl.b.tx_fifo_reset = 1;
    ctrl.b.dma_mode = 1;
    ctrl.b.swrx_flow_ctrl = 1;
    ctrl.b.swtx_flow_ctrl = 1;
    ctrl.b.backslash_en = 1;
    ctrl.b.tx_finish_n_wait = 0;
    ctrl.b.divisor_mode = 0;
    ctrl.b.irda_enable = 0;
    ctrl.b.rx_rts = 1;
    ctrl.b.auto_flow_control = 1;
    ctrl.b.loop_back_mode = 0;
    ctrl.b.rx_lock_err = 0;
    ctrl.b.hst_txd_oen = DEBUG_UART_HST_TXD_OEN_V_ENABLE;
    ctrl.b.rx_break_length = 11;
    hwp_debugUart->ctrl = ctrl.v;

    drvDhostContext_t *d = &gDhostCtx;

    d->tx.tx_sema = osiSemaphoreCreate(1, 1);
    d->tx.tx_done_sema = osiSemaphoreCreate(1, 0);
    d->rx.rx_work = osiWorkCreate(prvDhostRxProc, NULL, d);
    d->pm.pm_source = osiPmSourceCreate(DRV_NAME_DEBUGUART, NULL, NULL);

    d->rx.cmd = drvHostCmdDebughostEngine();
    drvHostCmdEngineInit(d->rx.cmd);
    drvHostCmdSetSender(d->rx.cmd, prvHostCmdSend, d);
    drvHostCmdRegisterHander(d->rx.cmd, HOST_FLOWID_SYSCMD, drvHostSyscmdHandler);
    drvHostCmdRegisterHander(d->rx.cmd, HOST_FLOWID_READWRITE2, drvHostReadWriteHandler);
    drvHostCmdRegisterHander(d->rx.cmd, HOST_FLOWID_READWRITE, drvHostReadWriteHandler);

#ifdef CONFIG_DEBUGHOST_RX_USE_IFC
    drvIfcChannelInit(&d->rx.rx_ifc, DRV_NAME_DEBUGUART, DRV_IFC_RX);
    drvIfcRequestChannel(&d->rx.rx_ifc);
    drvIfcStart(&d->rx.rx_ifc, gDhostRxDmaBuf, CONFIG_DEBUGHOST_RX_DMA_SIZE);
#endif
    osiFifoInit(&d->rx.rx_fifo, d->rx.rx_buf, CONFIG_DEBUGHOST_RX_BUF_SIZE);

    drvIfcChannelInit(&d->tx.tx_ifc, DRV_NAME_DEBUGUART, DRV_IFC_TX);
    drvIfcRequestChannel(&d->tx.tx_ifc);

    REG_DEBUG_UART_IRQ_MASK_T irq_mask = {
        .b.tx_dma_done = 1,
#ifdef CONFIG_DEBUGHOST_RX_USE_IFC
        .b.rx_dma_done = 1,
        .b.rx_dma_timeout = 1,
#else
        .b.rx_data_available = 1,
        .b.rx_timeout = 1,
#endif
    };
    hwp_debugUart->irq_mask = irq_mask.v;

    d->ctrl_val = ctrl.v;
    d->irq_mask_val = irq_mask.v;
    d->triggers_val = triggers.v;

    osiIrqSetHandler(IRQN_DEBUG_UART, prvDhostISR, d);
    osiIrqSetPriority(IRQN_DEBUG_UART, SYS_IRQ_PRIO_DEBUG_UART);
    osiIrqEnable(IRQN_DEBUG_UART);
}

void drvDhostRestoreConfig(void)
{
    drvDhostContext_t *d = &gDhostCtx;
    hwp_debugUart->ctrl = d->ctrl_val;
    hwp_debugUart->irq_mask = d->irq_mask_val;
    hwp_debugUart->triggers = d->triggers_val;
}

static void prvHostCmdBlueScreenSend(void *ctx, uint8_t *packet, unsigned packet_len)
{
    prvDhostFifoWriteAll(packet, packet_len);
}

void drvDhostBlueScreenEnter(void)
{
    drvDhostContext_t *d = &gDhostCtx;

    OSI_LOOP_WAIT_TIMEOUT_US(!drvIfcIsRunning(&d->tx.tx_ifc), 500 * 1000);
    drvIfcStop(&d->tx.tx_ifc);

#ifdef CONFIG_DEBUGHOST_RX_USE_IFC
    drvIfcStop(&d->rx.rx_ifc);
#endif
    drvHostCmdSetSender(d->rx.cmd, prvHostCmdBlueScreenSend, d);
}

bool drvDhostBlueScreenSend(const void *data, unsigned size)
{
    prvDhostFifoWriteAll(data, size);
    return true;
}

void drvDhostBlueScreenPoll(void)
{
    drvDhostContext_t *d = &gDhostCtx;
    char read_buf[64];

    int rbytes = prvDhostFifoRead(read_buf, 64);
    if (rbytes > 0)
        drvHostCmdPushData(d->rx.cmd, read_buf, rbytes);
}
