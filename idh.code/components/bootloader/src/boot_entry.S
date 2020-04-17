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

#include "boot_config.h"

#define CPSR_M_USR 0x10U
#define CPSR_M_FIQ 0x11U
#define CPSR_M_IRQ 0x12U
#define CPSR_M_SVC 0x13U
#define CPSR_M_MON 0x16U
#define CPSR_M_ABT 0x17U
#define CPSR_M_HYP 0x1AU
#define CPSR_M_UND 0x1BU
#define CPSR_M_SYS 0x1FU

    .text
    .arm

    .align 5
    .section BOOT_ENTRY, #alloc, #execinstr

    .global bootEntry
    .type   Vectors, %function
bootEntry:
    LDR     R4, =Vectors
    MCR     p15, 0, R4, c12, c0, 0

    CPS     CPSR_M_IRQ
    LDR     SP, =CONFIG_BOOT_IRQ_STACK_TOP
    CPS     CPSR_M_SVC
    LDR     SP, =CONFIG_BOOT_SVC_STACK_TOP

    BLX     bootStart

    .align 5
    .global Vectors
    .type   Vectors, %function
Vectors:
    B       .   // Reset
    B       .   // Undef
    B       .   // SVC
    B       .   // PAbt
    B       .   // DAbt
    B       .
    LDR     PC, =IRQ_Handler
    B       .   // FIQ

    .global IRQ_Handler
    .type   IRQ_Handler, %function
IRQ_Handler:
    PUSH    {r0-r12, r14}

    BLX     bootIrqHandler

    POP     {r0-r12, r14}
    SUBS    pc, lr, #4
