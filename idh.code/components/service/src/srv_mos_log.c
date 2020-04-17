/* Copyright (C) 2019 RDA Technologies Limited and/or its affiliates("RDA").
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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('M', 'S', 'L', 'G')
// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "osi_log.h"
#include "osi_api.h"
#include "hal_shmem_region.h"
#include "drv_md_ipc.h"
#include "drv_serial.h"
#include "drv_names.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define MOS_THREAD_PRIORITY (OSI_PRIORITY_NORMAL)
#define MOS_THREAD_STACK_SIZE (1024)

#define MOS_LOG_RETRY_INTERVAL (10)
#define MOS_USB_SERIAL_NAME (DRV_NAME_USRL_COM2)
#define MOS_USB_TX_MAX_SIZE (4096)
#define MOS_USB_RX_BUF_SIZE (512 * 2)
#define MOS_USB_RX_DMA_SIZE (512)
#define MOS_USB_WRITE_TIMEOUT (1000)

#define MOS_LOG_BUFFER_OK (0x4F4B)
#define MAX_BUF_LEN (512 * 1024) // for sanity check
#define MIN_BUF_LEN (1024)       // for sanity check

enum
{
    MOS_PORT_CP_UART = 0,
    MOS_PORT_CLOSE = 2,
    MOS_PORT_AP_UART = 4,
    MOS_PORT_AP_USB = 5,
    MOS_PORT_AP_SD = 6,
    MOS_PORT_INVALID = 0xfe,
};

typedef volatile struct
{
    uint32_t head;            // log buffer base address
    uint32_t length;          // log buffer total length (2^n BYTES)
    uint32_t rd_index;        // log buffer read offset
    uint32_t wr_index;        // log buffer write offset
    uint32_t cur_wr_index;    // log buffer current write offset
    uint16_t port;            // 0 -> mos uart; 2 -> close(do not send); 4 -> IPC+UART; 5 -> IPC+USB; 6 -> IPC+FLASH
    uint16_t status;          // buffer available: 0K(0x4F4B)
    uint32_t overflow_index;  // write index of protect area
    uint32_t remain_send_len; // remain the send length
    uint32_t cur_send_len;    // current the send length
    uint32_t protect_len;     // log buffer protect length
} logBuf_t;

typedef struct
{
    logBuf_t *log_buf;
    osiWorkQueue_t *work_queue;
    osiWork_t *write_work;
    osiWork_t *open_work;
    osiWork_t *close_work;
    drvSerial_t *serial;
    osiTimer_t *retry_timer;
} mosLog_t;

static void prvSerialCb(drvSerialEvent_t event, unsigned long param)
{
    mosLog_t *m = (mosLog_t *)(param);

    if (event & DRV_SERIAL_EVENT_BROKEN)
        osiWorkEnqueueLast(m->close_work, m->work_queue);
    if (event & DRV_SERIAL_EVENT_READY)
        osiWorkEnqueueLast(m->open_work, m->work_queue);
}

static void prvSerialOpen(void *param)
{
    mosLog_t *m = (mosLog_t *)param;
    drvSerialOpen(m->serial);
    osiWorkEnqueue(m->write_work, m->work_queue);
}

static void prvSerialClose(void *param)
{
    mosLog_t *m = (mosLog_t *)param;
    drvSerialClose(m->serial);
}

static void prvSendBuffer(mosLog_t *m, unsigned head, unsigned size)
{
    // split the whole buffer into pieces, to avoid occupy USB for too long
    while (size > 0)
    {
        unsigned wsize = OSI_MIN(unsigned, size, MOS_USB_TX_MAX_SIZE);
        drvSerialWriteDirect(m->serial, (void *)head, wsize, MOS_USB_WRITE_TIMEOUT,
                             DRV_SERIAL_FLAG_TXBUF_UNCACHED);

        head += wsize;
        size -= wsize;
    }
}

static int prvSendLogChannel(mosLog_t *m, logBuf_t *buf)
{
    if (buf->port != MOS_PORT_AP_USB)
        return -1;

    if (buf->status != MOS_LOG_BUFFER_OK)
        return 0;

    unsigned head = buf->head;
    unsigned buf_len = buf->length;
    unsigned rd_index = buf->rd_index;
    unsigned wr_index = buf->wr_index;
    unsigned overflow_index = buf->overflow_index;

    OSI_LOGD(0, "mos send %x/%u/%u/%u/%u", head, buf_len, rd_index, wr_index, overflow_index);

    // sanity check
    if (!OSI_IS_POW2(buf_len) || buf_len > MAX_BUF_LEN || buf_len < MIN_BUF_LEN)
        return 0;

    // 4 bytes alignment is required for USB transfer
    rd_index = OSI_ALIGN_DOWN(rd_index, 4);
    wr_index = OSI_ALIGN_DOWN(wr_index, 4);
    overflow_index = OSI_ALIGN_DOWN(overflow_index, 4);

    unsigned wsize1 = (wr_index < rd_index) ? buf->overflow_index - rd_index : 0;
    if (wsize1 > 0)
    {
        prvSendBuffer(m, head + rd_index, wsize1);
        rd_index = (rd_index + wsize1) & (buf_len - 1);
    }

    unsigned wsize2 = (wr_index > rd_index) ? wr_index - rd_index : 0;
    if (wsize2 > 0)
    {
        prvSendBuffer(m, head + rd_index, wsize2);
        rd_index = (rd_index + wsize2) & (buf_len - 1);
    }

    // update read pointer for peer CPU
    buf->rd_index = rd_index;
    if (wsize1 + wsize2 > 0)
    {
        OSI_LOGD(0, "mos log write/%d/%d rd/%d wr/%d ",
                 wsize1, wsize2, rd_index, wr_index);
    }
    return wsize1 + wsize2;
}

static void prvSerialWrite(void *param)
{
    mosLog_t *m = (mosLog_t *)param;
    if (!drvSerialCheckReady(m->serial))
        return;

    logBuf_t *mos_buf = m->log_buf;
    prvSendLogChannel(m, mos_buf);

    if (mos_buf->port == MOS_PORT_AP_USB)
        osiTimerStartRelaxed(m->retry_timer, MOS_LOG_RETRY_INTERVAL, OSI_WAIT_FOREVER);
}

static void prvIpcNotify(void *param)
{
    mosLog_t *m = (mosLog_t *)param;
    OSI_LOGD(0, "mos notify");
    if (drvSerialCheckReady(m->serial))
    {
        osiTimerStop(m->retry_timer);
        osiWorkEnqueue(m->write_work, m->work_queue);
    }
}

static bool prvLogBufInit(mosLog_t *m)
{
    const halShmemRegion_t *region = halShmemGetRegion(MEM_MOS_DEBUG_NAME);
    if (!region)
        return false;
    m->log_buf = (logBuf_t *)region->address;
    return true;
}

bool srvMosLogInit(void)
{
    mosLog_t *m = (mosLog_t *)calloc(1, sizeof(mosLog_t));
    if (m == NULL)
        return false;

    // Though RX data are not handled, it may make PC happy to receive
    // and drop RX data.
    drvSerialCfg_t scfg = {};
    scfg.rx_buf_size = MOS_USB_RX_BUF_SIZE;
    scfg.rx_dma_size = MOS_USB_RX_DMA_SIZE;
    scfg.event_mask = DRV_SERIAL_EVENT_BROKEN | DRV_SERIAL_EVENT_READY;
    scfg.event_cb = prvSerialCb;
    scfg.param = (unsigned long)m;

    m->serial = drvSerialAcquire(MOS_USB_SERIAL_NAME, &scfg);
    if (m->serial == NULL)
        goto fail;

    m->work_queue = osiWorkQueueCreate("mos-log", 1, MOS_THREAD_PRIORITY,
                                       MOS_THREAD_STACK_SIZE);
    if (m->work_queue == NULL)
        goto fail;

    m->write_work = osiWorkCreate(prvSerialWrite, NULL, m);
    if (m->write_work == NULL)
        goto fail;

    m->open_work = osiWorkCreate(prvSerialOpen, NULL, m);
    if (m->open_work == NULL)
        goto fail;

    m->close_work = osiWorkCreate(prvSerialClose, NULL, m);
    if (m->close_work == NULL)
        goto fail;

    m->retry_timer = osiTimerCreateWork(m->write_work, m->work_queue);
    if (m->retry_timer == NULL)
        goto fail;

    if (!prvLogBufInit(m))
        goto fail;

    ipc_register_mos_notify(prvIpcNotify, m);
    OSI_LOGI(0, "mos log service init %p/%p", m, m->log_buf);
    return true;

fail:
    osiTimerDelete(m->retry_timer);
    osiWorkDelete(m->close_work);
    osiWorkDelete(m->open_work);
    osiWorkDelete(m->write_work);
    osiWorkQueueDelete(m->work_queue);
    drvSerialRelease(m->serial);
    free(m);
    return false;
}
