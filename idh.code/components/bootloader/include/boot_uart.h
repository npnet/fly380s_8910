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

#ifndef _BOOT_UART_H_
#define _BOOT_UART_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BOOT_UART1,
    BOOT_UART2,
    BOOT_UART3
} bootUartID_t;

typedef struct bootUart bootUart_t;

bootUart_t *bootUartOpen(bootUartID_t id, uint32_t baud);
bool bootUartSetBaud(bootUart_t *uart, uint32_t baud);
int bootUartAvail(bootUart_t *uart);
int bootUartRead(bootUart_t *uart, void *data, size_t size);
int bootUartWrite(bootUart_t *uart, const void *data, size_t size);
void bootUartFlush(bootUart_t *uart);

#ifdef __cplusplus
}
#endif
#endif
