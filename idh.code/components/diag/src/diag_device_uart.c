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
#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('D', 'I', 'U', 'T')

#include "diag_device.h"
#include "diag_config.h"
#include "drv_names.h"
#include "drv_uart.h"
#include <stdlib.h>
#include "osi_log.h"

typedef struct
{
    diagDevice_t device;
    drvUart_t *uart;
} diagUart_t;

static int _diagUartSend(diagDevice_t *d_, const void *data, unsigned size, unsigned timeout)
{
    diagUart_t *d = (diagUart_t *)d_;
    return drvUartSendAll(d->uart, data, size, timeout);
}

static int _diagUartRecv(diagDevice_t *d_, void *buffer, unsigned size)
{
    diagUart_t *d = (diagUart_t *)d_;
    return drvUartReceive(d->uart, buffer, size);
}

static bool _diagUartWaitTxDone(diagDevice_t *d_, unsigned timeout)
{
    diagUart_t *d = (diagUart_t *)d_;
    return drvUartWaitTxFinish(d->uart, timeout);
}

static void _diagUartDestroy(diagDevice_t *d_)
{
    diagUart_t *d = (diagUart_t *)d_;
    drvUartClose(d->uart);
    drvUartDestroy(d->uart);
    free(d);
}

static void _diagUartCB(void *param, uint32_t event)
{
    diagUart_t *d = (diagUart_t *)param;
    OSI_LOGI(0, "Diag uart callback, event 0x%x", event);
    if (d->device.notify_cb)
        d->device.notify_cb(d->device.param);
}

diagDevice_t *diagDeviceUartCreate(const diagDevCfg_t *cfg)
{
    diagUart_t *d = (diagUart_t *)calloc(1, sizeof(diagUart_t));
    if (d == NULL)
        return NULL;

    drvUartCfg_t uart_cfg = {
        .baud = CONFIG_DIAG_DEFAULT_UART_BAUD,
        .data_bits = DRV_UART_DATA_BITS_8,
        .stop_bits = DRV_UART_STOP_BITS_1,
        .parity = DRV_UART_NO_PARITY,
        .rx_buf_size = cfg->rx_buf_size,
        .tx_buf_size = cfg->tx_buf_size,
        .event_mask = DRV_UART_EVENT_RX_ARRIVED | DRV_UART_EVENT_RX_OVERFLOW,
        .event_cb = _diagUartCB,
        .event_cb_ctx = d,
    };
    d->uart = drvUartCreate(CONFIG_DIAG_DEFAULT_UART, &uart_cfg);
    if (d->uart == NULL)
    {
        free(d);
        return NULL;
    }

    d->device.ops.send = _diagUartSend;
    d->device.ops.recv = _diagUartRecv;
    d->device.ops.destroy = _diagUartDestroy;
    d->device.ops.wait_tx_finish = _diagUartWaitTxDone;
    d->device.notify_cb = cfg->cb;
    d->device.param = cfg->param;

    drvUartOpen(d->uart);
    drvUartSetAutoSleep(d->uart, 500);
    return &d->device;
}
