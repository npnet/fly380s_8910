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

#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_INFO
#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('D', 'I', 'S', 'L')

#include "diag_device.h"
#include "diag_config.h"
#include "drv_serial.h"
#include "drv_names.h"
#include "osi_log.h"
#include "osi_api.h"

#include <stdlib.h>
#include <string.h>

#define DIAG_USRL_RX_DMA_SIZE (512)
#define DIAG_USRL_TX_DMA_SIZE (512)

typedef struct
{
    diagDevice_t device;
    drvSerial_t *serial;
} diagSerial_t;

static int _diagSerialSend(diagDevice_t *d_, const void *data, unsigned size, unsigned timeout)
{
    diagSerial_t *d = (diagSerial_t *)d_;
    int send = 0;
    for (;;)
    {
        int trans = drvSerialWrite(d->serial, data, size);
        if (trans < 0)
        {
            OSI_LOGE(0, "Diag usb output error %d", trans);
            return trans;
        }

        send += trans;
        size -= trans;
        data = (const char *)data + trans;
        if (size == 0)
            break;

        if (!drvSerialWaitWriteAvail(d->serial, timeout))
        {
            OSI_LOGE(0, "Diag usb output not avail %u/%u", send, size);
            break;
        }
    }
    return send;
}

static int _diagSerialRecv(diagDevice_t *d_, void *buffer, unsigned size)
{
    diagSerial_t *d = (diagSerial_t *)d_;
    return drvSerialRead(d->serial, buffer, size);
}

static bool _diagSerialWaitTxDone(diagDevice_t *d_, unsigned timeout)
{
    diagSerial_t *d = (diagSerial_t *)d_;
    return drvSerialWaitWriteFinish(d->serial, timeout);
}

static void _diagSerialDestroy(diagDevice_t *d_)
{
    diagSerial_t *d = (diagSerial_t *)d_;
    drvSerialClose(d->serial);
    drvSerialRelease(d->serial);
    free(d);
}

static void _diagSerialCB(drvSerialEvent_t event, unsigned long param)
{
    diagSerial_t *d = (diagSerial_t *)param;
    if (event & (DRV_SERIAL_EVENT_BROKEN | DRV_SERIAL_EVENT_READY))
        OSI_LOGW(0, "Diag usb connect/disconnect %x", event);

    if (d->device.notify_cb)
        d->device.notify_cb(d->device.param);
}

diagDevice_t *diagDeviceUserialCreate(const diagDevCfg_t *cfg)
{
    diagSerial_t *d = (diagSerial_t *)calloc(1, sizeof(diagSerial_t));
    if (d == NULL)
        return NULL;

    uint32_t event = DRV_SERIAL_EVENT_RX_ARRIVED |
                     DRV_SERIAL_EVENT_RX_OVERFLOW |
                     DRV_SERIAL_EVENT_BROKEN |
                     DRV_SERIAL_EVENT_READY;
    drvSerialCfg_t serl_cfg = {};
    serl_cfg.tx_buf_size = cfg->tx_buf_size;
    serl_cfg.rx_buf_size = cfg->rx_buf_size;
    serl_cfg.tx_dma_size = DIAG_USRL_TX_DMA_SIZE;
    serl_cfg.rx_dma_size = DIAG_USRL_RX_DMA_SIZE;
    serl_cfg.event_mask = event;
    serl_cfg.param = (unsigned long)d;
    serl_cfg.event_cb = _diagSerialCB;

    d->serial = drvSerialAcquire(CONFIG_DIAG_DEFAULT_USERIAL, &serl_cfg);
    if (d->serial == NULL)
    {
        free(d);
        return NULL;
    }

    d->device.ops.send = _diagSerialSend;
    d->device.ops.recv = _diagSerialRecv;
    d->device.ops.destroy = _diagSerialDestroy;
    d->device.ops.wait_tx_finish = _diagSerialWaitTxDone;
    d->device.notify_cb = cfg->cb;
    d->device.param = cfg->param;

    drvSerialOpen(d->serial);
    return &d->device;
}
