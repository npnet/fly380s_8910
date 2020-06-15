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
#include "hwregs.h"
#include "hal_chip.h"
#include "osi_tick_unit.h"

#if defined(CONFIG_SOC_8955) || defined(CONFIG_SOC_8909)
#define HWTICK16K (hwp_timer->hwtimer_curval)
#define HWTICK (hwp_timer->hwtimer_curval)
#define US2HWTICK(us) OSI_US_TO_TICK16K(us)
#define HWTICK2US(tick) OSI_TICK16K_TO_US(tick)
#define HWTICK2MS(tick) OSI_TICK16K_TO_MS(tick)
#endif

#if defined(CONFIG_SOC_8910)
#define HWTICK16K (hwp_timer2->hwtimer_curval)
#define HWTICK (hwp_timer4->hwtimer_curval_l)
#define US2HWTICK(us) OSI_US_TO_TICK2M(us)
#define HWTICK2US(tick) OSI_TICK2M_TO_US(tick)
#define HWTICK2MS(tick) OSI_TICK2M_TO_MS(tick)
#endif

#if defined(CONFIG_SOC_8811)
#define HWTICK16K (hwp_timer1->hwtimer_curval_l)
#define HWTICK (hwp_timer2->hwtimer_curval_l)
#define US2HWTICK(us) OSI_US_TO_TICK1M(us)
#define HWTICK2US(tick) OSI_TICK1M_TO_US(tick)
#define HWTICK2MS(tick) OSI_TICK1M_TO_MS(tick)
#endif

#define HWTICK_FROM(start) ((unsigned)(HWTICK - (start)))
#define DEBUG_EVENT_TIMEOUT_US (300)

void OSI_SECTION_SRAM_BOOT_TEXT osiDelayLoops(uint32_t count)
{
    while (count-- > 0)
        OSI_NOP;
}

void OSI_NO_MIPS16 OSI_SECTION_SRAM_BOOT_TEXT osiDelayUS(uint32_t us)
{
#if defined(CONFIG_SOC_8955) || defined(CONFIG_SOC_8909)
    // 4 cycles: 1 (addiu) + 3 (bnez)
    extern uint32_t gSysClkFreq;
    uint64_t mul = (gSysClkFreq * ((1ULL << 52) / (1000000 * 4))) >> 32;
    unsigned counter = (us * mul) >> 20;

    counter++;
    asm volatile("$L_check:\n"
                 "addiu %0, -1\n"
                 "bnez  %0, $L_check\n" ::"r"(counter));
#else
    uint32_t start = HWTICK;
    uint32_t ticks = US2HWTICK(us);
    OSI_POLL_WAIT(HWTICK_FROM(start) > ticks);
#endif
}

#if !defined(CONFIG_SOC_8955) && !defined(CONFIG_SOC_8909)
bool OSI_SECTION_SRAM_BOOT_TEXT osiDebugEvent(uint32_t event)
{
    // Ideally, critical section is needed to ensure debug event
    // can be sent. However, debug event is not for "serious"
    // debug, and the it takes so long (~100us) to send an event.

    uint32_t start = HWTICK;
    uint32_t ticks = US2HWTICK(DEBUG_EVENT_TIMEOUT_US);

    if (!OSI_LOOP_WAIT_IF(hwp_debugHost->event != 0,
                          HWTICK_FROM(start) <= ticks))
        return false;

    hwp_debugHost->event = event;
    return true;
}
#endif

#ifdef HWTICK16K
uint32_t OSI_SECTION_SRAM_BOOT_TEXT osiHWTick16K(void)
{
    return HWTICK16K;
}
#endif

uint32_t OSI_SECTION_SRAM_BOOT_TEXT osiHWTick32(void)
{
    return HWTICK;
}
