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

#include "osi_api.h"
#include "osi_profile.h"
#include "hal_config.h"
#include "cmsis_core.h"
#include <stddef.h>

#define DCACHE_LINE_SIZE (32)
#define GICD_ENABLE_REG_COUNT OSI_DIV_ROUND_UP(IRQ_GIC_LINE_COUNT, 32)
#define GICD_PRIORITY_REG_COUNT OSI_DIV_ROUND_UP(IRQ_GIC_LINE_COUNT, 4)
#define GICD_TARGET_REG_COUNT OSI_DIV_ROUND_UP(IRQ_GIC_LINE_COUNT, 4)
#define GICD_CFG_REG_COUNT OSI_DIV_ROUND_UP(IRQ_GIC_LINE_COUNT, 16)

struct osiIrqHandlerBody
{
    osiIrqHandler_t handler;
    void *ctx;
};

typedef struct
{
    uint32_t gicd_enable[GICD_ENABLE_REG_COUNT];
    uint32_t gicd_priority[GICD_PRIORITY_REG_COUNT];
    uint32_t gicd_target[GICD_TARGET_REG_COUNT];
    uint32_t gicd_cfg[GICD_CFG_REG_COUNT];
    uint32_t gicc_pmr;
    uint32_t gicc_bpr;
} osiIrqContext_t;

static osiIrqContext_t gOsiIrqCtx;

static struct osiIrqHandlerBody gOsiIrqBody[IRQ_GIC_LINE_COUNT] = {};

void osiIrqSuspend(osiSuspendMode_t mode)
{
    osiIrqContext_t *p = &gOsiIrqCtx;

    for (int n = 0; n < GICD_PRIORITY_REG_COUNT; n++)
        p->gicd_priority[n] = GICDistributor->IPRIORITYR[n];
    for (int n = 0; n < GICD_TARGET_REG_COUNT; n++)
        p->gicd_target[n] = GICDistributor->ITARGETSR[n];
    for (int n = 0; n < GICD_CFG_REG_COUNT; n++)
        p->gicd_cfg[n] = GICDistributor->ICFGR[n];

    p->gicc_pmr = GICInterface->PMR;
    p->gicc_bpr = GICInterface->BPR;
}

void osiIrqResume(osiSuspendMode_t mode, bool aborted)
{
    if (aborted)
        return;

    osiIrqContext_t *p = &gOsiIrqCtx;

    GIC_DisableDistributor();
    for (int n = 0; n < GICD_ENABLE_REG_COUNT; n++)
        GICDistributor->ISENABLER[n] = p->gicd_enable[n];
    for (int n = 0; n < GICD_PRIORITY_REG_COUNT; n++)
        GICDistributor->IPRIORITYR[n] = p->gicd_priority[n];
    for (int n = 0; n < GICD_TARGET_REG_COUNT; n++)
        GICDistributor->ITARGETSR[n] = p->gicd_target[n];
    for (int n = 0; n < GICD_CFG_REG_COUNT; n++)
        GICDistributor->ICFGR[n] = p->gicd_cfg[n];
    GIC_EnableDistributor();

    GIC_EnableInterface();
    GICInterface->PMR = p->gicc_pmr;
    GICInterface->BPR = p->gicc_bpr;
}

void osiIrqInit(void)
{
    for (unsigned n = 0; n < IRQ_GIC_LINE_COUNT; n++)
        gOsiIrqBody[n].handler = NULL;
    GIC_Enable();
}

bool osiIrqSetHandler(uint32_t irqn, osiIrqHandler_t handler, void *ctx)
{
    if (irqn >= IRQ_GIC_LINE_COUNT)
        return false;

    gOsiIrqBody[irqn].handler = handler;
    gOsiIrqBody[irqn].ctx = ctx;
    return true;
}

bool osiIrqEnable(uint32_t irqn)
{
    osiIrqContext_t *p = &gOsiIrqCtx;
    if (irqn >= IRQ_GIC_LINE_COUNT)
        return false;

    p->gicd_enable[irqn / 32] |= 1 << (irqn % 32);
    GIC_EnableIRQ(irqn);
    return true;
}

bool osiIrqDisable(uint32_t irqn)
{
    osiIrqContext_t *p = &gOsiIrqCtx;
    if (irqn >= IRQ_GIC_LINE_COUNT)
        return false;

    p->gicd_enable[irqn / 32] &= ~(1 << (irqn % 32));
    GIC_DisableIRQ(irqn);
    return true;
}

bool osiIrqEnabled(uint32_t irqn)
{
    return (irqn < IRQ_GIC_LINE_COUNT && GIC_GetEnableIRQ(irqn));
}

bool osiIrqSetPriority(uint32_t irqn, uint32_t priority)
{
    if (irqn >= IRQ_GIC_LINE_COUNT)
        return false;

    GIC_SetPriority(irqn, priority);
    return true;
}

uint32_t osiIrqGetPriority(uint32_t irqn)
{
    if (irqn >= IRQ_GIC_LINE_COUNT)
        return 0x80000000U;

    return GIC_GetPriority(irqn);
}

bool OSI_SECTION_SRAM_TEXT osiIrqPending(void)
{
    uint32_t isr = __get_ISR();
    return (isr & (CPSR_I_Msk | CPSR_F_Msk | CPSR_A_Msk)) != 0;
}

void vApplicationIRQHandler(uint32_t ulICCIAR)
{
    uint32_t irqn = ulICCIAR & 0x3FFUL;
    if (irqn >= IRQ_GIC_LINE_COUNT)
    {
        osiProfileIrqEnter(IRQ_GIC_LINE_COUNT);
        osiProfileIrqExit(IRQ_GIC_LINE_COUNT);
        return;
    }

    osiProfileIrqEnter(irqn);
    __enable_irq();

    if (gOsiIrqBody[irqn].handler != NULL)
        gOsiIrqBody[irqn].handler(gOsiIrqBody[irqn].ctx);

    osiProfileIrqExit(irqn);
}
