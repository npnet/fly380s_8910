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

#include "srv_config.h"

#ifdef CONFIG_USB_TRACE_ENABLE
#include "srv_usb_trace.h"
#include "drv_host_cmd.h"
#include "drv_serial.h"
#include "drv_names.h"
#include "osi_api.h"
#include "osi_log.h"
#include "osi_trace.h"
#include <stdlib.h>

#define USBTRACE_THREAD_PRIORITY (OSI_PRIORITY_LOW)
#define USBTRACE_THREAD_STACK_SIZE (1024)
#define USBTRACE_SERIAL_NAME (DRV_NAME_USRL_COM4)
#define USBTRACE_SEND_TIMEOUT (500)
#define USBTRACE_RX_BUF_SIZE (2048)
#define USBTRACE_RX_DMA_SIZE (512)

typedef struct
{
    drvSerial_t *serial;
    osiWork_t *rx_work;
    osiWork_t *open_work;
    osiWork_t *close_work;
    drvHostCmdEngine_t *cmd;
} srvUsbTraceContext_t;

static srvUsbTraceContext_t *gSrvUsbTraceCtx = NULL;

static void prvSerialCb(drvSerialEvent_t event, unsigned long param)
{
    srvUsbTraceContext_t *d = (srvUsbTraceContext_t *)param;

    OSI_LOGI(0, "USB trace serial event 0x%x", event);
    if (event & DRV_SERIAL_EVENT_RX_ARRIVED)
        osiWorkEnqueue(d->rx_work, osiTraceWorkQueue());

    if (event & DRV_SERIAL_EVENT_BROKEN)
        osiWorkEnqueueLast(d->close_work, osiTraceWorkQueue());

    if (event & DRV_SERIAL_EVENT_READY)
        osiWorkEnqueueLast(d->open_work, osiTraceWorkQueue());
}

static void prvSerialRx(void *param)
{
    srvUsbTraceContext_t *d = (srvUsbTraceContext_t *)param;

    uint8_t buf[128];
    for (;;)
    {
        int rsize = drvSerialRead(d->serial, buf, 128);
        if (rsize <= 0)
            break;

        drvHostCmdPushData(d->cmd, buf, rsize);
    }
}

static void prvSerialOpen(void *param)
{
    srvUsbTraceContext_t *d = (srvUsbTraceContext_t *)param;
    drvSerialOpen(d->serial);
}

static void prvSerialClose(void *param)
{
    srvUsbTraceContext_t *d = (srvUsbTraceContext_t *)param;
    drvSerialClose(d->serial);
}

static void prvHostCmdSend(void *ctx, uint8_t *packet, unsigned packet_len)
{
    srvUsbTraceSendAll(packet, packet_len);
}

bool srvUsbTraceSendAll(const void *data, unsigned size)
{
    srvUsbTraceContext_t *d = gSrvUsbTraceCtx;
    return drvSerialWriteDirect(d->serial, data, size, USBTRACE_SEND_TIMEOUT, 0);
}

bool srvUsbTraceBlueScreenSend(const void *data, unsigned size)
{
    return false;
}

bool srvUsbTraceInit(void)
{
    if (gSrvUsbTraceCtx != NULL)
        return false;

    srvUsbTraceContext_t *d = calloc(1, sizeof(srvUsbTraceContext_t));
    if (d == NULL)
        return false;

    drvSerialCfg_t scfg = {};
    scfg.rx_buf_size = USBTRACE_RX_BUF_SIZE;
    scfg.rx_dma_size = USBTRACE_RX_DMA_SIZE;
    scfg.event_mask = DRV_SERIAL_EVENT_RX_ARRIVED | DRV_SERIAL_EVENT_BROKEN | DRV_SERIAL_EVENT_READY;
    scfg.event_cb = prvSerialCb;
    scfg.param = (unsigned long)d;

    d->serial = drvSerialAcquire(USBTRACE_SERIAL_NAME, &scfg);
    if (d->serial == NULL)
        goto failed;

    d->rx_work = osiWorkCreate(prvSerialRx, NULL, d);
    if (d->rx_work == NULL)
        goto failed;

    d->open_work = osiWorkCreate(prvSerialOpen, NULL, d);
    if (d->open_work == NULL)
        goto failed;

    d->close_work = osiWorkCreate(prvSerialClose, NULL, d);
    if (d->close_work == NULL)
        goto failed;

    d->cmd = drvHostCmdUsbEngine();
    drvHostCmdEngineInit(d->cmd);
    drvHostCmdSetSender(d->cmd, prvHostCmdSend, d);
    drvHostCmdRegisterHander(d->cmd, HOST_FLOWID_SYSCMD, drvHostSyscmdHandler);
    drvHostCmdRegisterHander(d->cmd, HOST_FLOWID_READWRITE2, drvHostReadWriteHandler);
    drvHostCmdRegisterHander(d->cmd, HOST_FLOWID_READWRITE, drvHostReadWriteHandler);

    gSrvUsbTraceCtx = d;
    return true;

failed:
    osiWorkDelete(d->close_work);
    osiWorkDelete(d->open_work);
    osiWorkDelete(d->rx_work);
    drvSerialRelease(d->serial);
    free(d);
    return false;
}

void srcUsbTraceBlueScreenEnter(void)
{
}

void srvUsbTraceBlueScreenPoll(void)
{
}

#endif
