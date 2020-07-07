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
#include "osi_chip.h"
#include "hwregs.h"

#define PROFILE_BUF_SIZE (48 * 1024)
#define PROFILE_STOP_ON_FULL (0)

typedef struct
{
    uint32_t mode;           // profile mode
    uint32_t *start_address; // profile buffer start address
    uint16_t pos;            // profile next write position, in word
    uint16_t size;           // profile buffer size, in word
    uint32_t freq;           // profile tick frequency
} osiProfileContext_t;

static uint32_t gProfileBuf[PROFILE_BUF_SIZE / 4];
static osiProfileContext_t gProfileCtx;

void osiProfileInit(void)
{
    osiProfileContext_t *p = &gProfileCtx;
    p->mode = OSI_PROFILE_MODE_NORMAL;
    p->start_address = gProfileBuf;
    p->pos = 0;
    p->size = PROFILE_BUF_SIZE / 4;
    p->freq = osiChipProfileTickFreq();
}

void osiProfileSetMode(osiProfileMode_t mode)
{
#if (CONFIG_KERNEL_PROFILE_BUF_SIZE > 0)
    unsigned critical = osiEnterCritical();
    osiProfileContext_t *p = &gProfileCtx;
    if (p->mode != mode)
    {
        p->mode = mode;
        p->pos = 0;
    }
    osiExitCritical(critical);
#endif
}

void osiProfileCode(unsigned code)
{
    osiProfileContext_t *p = &gProfileCtx;
    if (p->start_address == 0)
        return;

    unsigned critical = osiEnterCritical();

    if (p->mode == OSI_PROFILE_MODE_STOP_ON_FULL && p->pos + 1 >= p->size)
    {
        osiExitCritical(critical);
        return;
    }

    static uint32_t prev_tick = 0;
    uint32_t tick = osiChipProfileTick();
    uint32_t pcode = (tick << 16) | (code & 0xffff);
    uint32_t tick_delta_hi = (tick - prev_tick) >> 16;

    prev_tick = tick;
    if (tick_delta_hi > 1)
    {
        p->start_address[p->pos++] = (tick_delta_hi << 16) | 0;
        if (p->pos >= p->size)
            p->pos = 0;
    }

    p->start_address[p->pos++] = pcode;
    if (p->pos >= p->size)
        p->pos = 0;

    osiExitCritical(critical);
}

void osiProfileEnter(unsigned code)
{
#if (CONFIG_KERNEL_PROFILE_BUF_SIZE > 0)
    osiProfileCode(code & ~PROFCODE_EXIT_FLAG);
#endif
}

void osiProfileExit(unsigned code)
{
#if (CONFIG_KERNEL_PROFILE_BUF_SIZE > 0)
    osiProfileCode(code | PROFCODE_EXIT_FLAG);
#endif
}

void osiProfileThreadEnter(unsigned id)
{
#if (CONFIG_KERNEL_PROFILE_BUF_SIZE > 0)
    unsigned code = id + PROFCODE_THREAD_START;
    if (code > PROFCODE_THREAD_END)
        code = PROFCODE_THREAD_END;
    osiProfileCode(code);
#endif
}

void osiProfileThreadExit(unsigned id)
{
#if (CONFIG_KERNEL_PROFILE_BUF_SIZE > 0)
    unsigned code = id + PROFCODE_THREAD_START;
    if (code > PROFCODE_THREAD_END)
        code = PROFCODE_THREAD_END;
    osiProfileCode(code | PROFCODE_EXIT_FLAG);
#endif
}

void osiProfileIrqEnter(unsigned id)
{
#if (CONFIG_KERNEL_PROFILE_BUF_SIZE > 0)
    unsigned code = id + PROFCODE_IRQ_START;
    if (code > PROFCODE_IRQ_END)
        code = PROFCODE_IRQ_END;
    osiProfileCode(code);
#endif
}

void osiProfileIrqExit(unsigned id)
{
#if (CONFIG_KERNEL_PROFILE_BUF_SIZE > 0)
    unsigned code = id + PROFCODE_IRQ_START;
    if (code > PROFCODE_IRQ_END)
        code = PROFCODE_IRQ_END;
    osiProfileCode(code | PROFCODE_EXIT_FLAG);
#endif
}
