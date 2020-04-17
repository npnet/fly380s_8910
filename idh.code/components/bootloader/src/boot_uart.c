/* Copyright (C) 2017 RDA Technologies Limited and/or its affiliates("RDA").
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

#include "boot_uart.h"
#include "boot_platform.h"
#include "boot_fdl_channel.h"
#include "hwregs.h"
#include "osi_api.h"
#include <stdlib.h>

#define PDL_UART BOOT_UART2

#define UART_BAUD_BASE_CLK (26000000)
#define UART_RX_TIMEOUT_CNT (4 * 10)
#define UART_TX_DELAY (0)
#define UART_TXFIFO_SIZE (128)
#define UART_RXFIFO_SIZE (128)
#define UART_TX_TRIG_FIFO_MODE (0)
#define UART_RX_TRIG_FIFO_MODE (64)
#define UART_TX_TRIG_DMA_MODE (1)
#define UART_RX_TRIG_DMA_MODE (64)

struct bootUart
{
    HWP_ARM_UART_T *hwp;
    uint32_t baud;
};

typedef struct
{
    fdlChannel_t ops;
    bootUart_t *uart;
} fdlUartChannel_t;

static void _setBaud(bootUart_t *d)
{
    unsigned delta_min = -1U;
    unsigned nset_chosen = 1; // make compiler happy
    unsigned div_chosen = 1;  // make compiler happy

    for (int nset = 16; nset >= 6; nset--)
    {
        unsigned div = (UART_BAUD_BASE_CLK + nset * d->baud / 2) / (nset * d->baud);
        if (div <= 1 || div >= (1 << 16))
            continue;

        unsigned real_baud = UART_BAUD_BASE_CLK / (nset * div);
        unsigned delta = (real_baud > d->baud)
                             ? real_baud - d->baud
                             : d->baud - real_baud;
        if (delta < delta_min)
        {
            delta_min = delta;
            nset_chosen = nset;
            div_chosen = div;
        }
    }

    if (delta_min == -1U)
        bootPanic();

    REG_ARM_UART_UART_BAUD_T uart_baud = {};
    uart_baud.b.baud_div = div_chosen - 1;
    uart_baud.b.baud_const = nset_chosen - 1;
    d->hwp->uart_baud = uart_baud.v;
}

static void _startConfig(bootUart_t *d)
{
    REG_ARM_UART_UART_CONF_T uart_conf = {};
    uart_conf.b.check = 0;    // DRV_UART_NO_PARITY
    uart_conf.b.parity = 0;   // DRV_UART_EVEN_PARITY
    uart_conf.b.stop_bit = 0; // DRV_UART_STOP_BITS_1
    uart_conf.b.st_check = 1;
    uart_conf.b.rx_ie = 0;
    uart_conf.b.tx_ie = 0;
    uart_conf.b.tout_ie = 0;
    uart_conf.b.hwfc = 0;
    uart_conf.b.rx_trig_hwfc = 0;
    uart_conf.b.tout_hwfc = 0;
    uart_conf.b.hdlc = 0;
    uart_conf.b.frm_stp = 1; // ??
    uart_conf.b.trail = 0;
    uart_conf.b.txrst = 1;
    uart_conf.b.rxrst = 1;
    uart_conf.b.at_parity_en = 0;
    uart_conf.b.at_parity_sel = 0;
    uart_conf.b.at_verify_2byte = 1;
    uart_conf.b.at_div_mode = 0;
    uart_conf.b.at_enable = 0;
    d->hwp->uart_conf = uart_conf.v;

    REG_ARM_UART_UART_DELAY_T uart_delay = {};
    uart_delay.b.toutcnt = UART_RX_TIMEOUT_CNT;
    uart_delay.b.two_tx_delay = UART_TX_DELAY;
    d->hwp->uart_delay = uart_delay.v;

    REG_ARM_UART_UART_RXTRIG_T uart_rxtrig = {};
    uart_rxtrig.b.rx_trig = UART_RX_TRIG_FIFO_MODE;
    d->hwp->uart_rxtrig = uart_rxtrig.v;

    REG_ARM_UART_UART_TXTRIG_T uart_txtrig = {};
    uart_txtrig.b.tx_trig = UART_TX_TRIG_FIFO_MODE;
    d->hwp->uart_txtrig = uart_txtrig.v;

    for (;;)
    {
        uart_conf.v = d->hwp->uart_conf;
        if (uart_conf.b.txrst == 0 && uart_conf.b.rxrst == 0)
            break;
    }

    d->hwp->uart_status = d->hwp->uart_status;
}

bootUart_t *bootUartOpen(bootUartID_t uart, uint32_t baud)
{
    bootUart_t *d = (bootUart_t *)malloc(sizeof(bootUart_t));
    if (d == NULL)
        bootPanic();

    if (uart == BOOT_UART1)
        d->hwp = hwp_uart1;
    else if (uart == BOOT_UART2)
        d->hwp = hwp_uart2;
    else if (uart == BOOT_UART3)
        d->hwp = hwp_uart3;
    else
    {
        free(d); // not support
        return NULL;
    }

    d->baud = baud;
    _setBaud(d);
    _startConfig(d);
    return d;
}

bool bootUartSetBaud(bootUart_t *d, uint32_t baud)
{
    if (baud == d->baud)
        return true;

    d->baud = baud;
    _setBaud(d);
    return true;
}

int bootUartAvail(bootUart_t *d)
{
    REG_ARM_UART_UART_RXFIFO_STAT_T uart_rxfifo_stat = {d->hwp->uart_rxfifo_stat};
    return uart_rxfifo_stat.b.rx_fifo_cnt;
}

int bootUartRead(bootUart_t *d, void *data, size_t size)
{
    REG_ARM_UART_UART_RXFIFO_STAT_T uart_rxfifo_stat = {d->hwp->uart_rxfifo_stat};
    int bytes = uart_rxfifo_stat.b.rx_fifo_cnt;
    if (bytes > size)
        bytes = size;

    uint8_t *data8 = (uint8_t *)data;
    for (int n = 0; n < bytes; n++)
        *data8++ = d->hwp->uart_rx;
    return bytes;
}

int bootUartWrite(bootUart_t *d, const void *data, size_t size)
{
    REG_ARM_UART_UART_TXFIFO_STAT_T uart_txfifo_stat = {d->hwp->uart_txfifo_stat};
    uint32_t tx_fifo_cnt = uart_txfifo_stat.b.tx_fifo_cnt;
    int bytes = tx_fifo_cnt < UART_TXFIFO_SIZE
                    ? UART_TXFIFO_SIZE - tx_fifo_cnt
                    : 0;
    if (bytes > size)
        bytes = size;

    const uint8_t *data8 = (const uint8_t *)data;
    for (int n = 0; n < bytes; n++)
        d->hwp->uart_tx = *data8++;
    return bytes;
}

static bool prvUartSetBaud(fdlChannel_t *ch, uint32_t baud)
{
    fdlUartChannel_t *p = (fdlUartChannel_t *)ch;
    return bootUartSetBaud(p->uart, baud);
}

static int prvUartAvail(fdlChannel_t *ch)
{
    fdlUartChannel_t *p = (fdlUartChannel_t *)ch;
    return bootUartAvail(p->uart);
}

static int prvUartRead(fdlChannel_t *ch, void *data, size_t size)
{
    fdlUartChannel_t *p = (fdlUartChannel_t *)ch;
    return bootUartRead(p->uart, data, size);
}

static int prvUartWrite(fdlChannel_t *ch, const void *data, size_t size)
{
    fdlUartChannel_t *p = (fdlUartChannel_t *)ch;
    return bootUartWrite(p->uart, data, size);
}

static void prvUartFlushInput(fdlChannel_t *ch)
{
    fdlUartChannel_t *p = (fdlUartChannel_t *)ch;

    int val;
    while (bootUartRead(p->uart, &val, 4) > 0)
        ;
}

static void prvUartFlush(fdlChannel_t *ch)
{
}

static bool prvUartConnected(fdlChannel_t *ch)
{
    return true;
}

static void prvUartDestroy(fdlChannel_t *ch)
{
    fdlUartChannel_t *p = (fdlUartChannel_t *)ch;
    free(p);
}

fdlChannel_t *fdlOpenUart(uint32_t baud)
{
    fdlUartChannel_t *ch = (fdlUartChannel_t *)malloc(sizeof(fdlUartChannel_t));
    if (ch == NULL)
        bootPanic();

    ch->ops.set_baud = prvUartSetBaud;
    ch->ops.avail = prvUartAvail;
    ch->ops.read = prvUartRead;
    ch->ops.write = prvUartWrite;
    ch->ops.flush_input = prvUartFlushInput;
    ch->ops.flush = prvUartFlush;
    ch->ops.destroy = prvUartDestroy;
    ch->ops.connected = prvUartConnected;
    ch->uart = bootUartOpen(PDL_UART, baud);
    return (fdlChannel_t *)ch;
}
