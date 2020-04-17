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
    uint32_t config;
    uint32_t *start_address;
    uint16_t pos;
    uint16_t size;
    uint32_t freq;
} osiProfileContext_t;

static uint32_t gProfileBuf[PROFILE_BUF_SIZE / 4];
static osiProfileContext_t gProfileCtx;

void osiProfileInit(void)
{
    osiProfileContext_t *p = &gProfileCtx;
    p->config = 0;
    p->start_address = gProfileBuf;
    p->pos = 0;
    p->size = PROFILE_BUF_SIZE / 4;
    p->freq = osiChipProfileTickFreq();
}

void osiProfileCode(unsigned code)
{
    // config: 0/running 1/pause
    // remainingSize: total size

    osiProfileContext_t *p = &gProfileCtx;
    if (p->start_address == 0)
        return;

    unsigned critical = osiEnterCritical();

    if (PROFILE_STOP_ON_FULL && p->pos + 1 >= p->size)
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
    osiProfileCode(code);
}

void osiProfileExit(unsigned code)
{
    osiProfileCode(code | CPEXITFLAG);
}

void osiProfileThreadEnter(unsigned id)
{
    unsigned code = id + CPTHREAD_START;
    if (code > CPTHREAD_END)
        code = CPTHREAD_END;
    osiProfileCode(code);
}

void osiProfileThreadExit(unsigned id)
{
    unsigned code = id + CPTHREAD_START;
    if (code > CPTHREAD_END)
        code = CPTHREAD_END;
    osiProfileCode(code | CPEXITFLAG);
}

void osiProfileIrqEnter(unsigned id)
{
    unsigned code = id + CPIRQ_START;
    if (code > CPIRQ_END)
        code = CPIRQ_END;
    osiProfileCode(code);
}

void osiProfileIrqExit(unsigned id)
{
    unsigned code = id + CPIRQ_START;
    if (code > CPIRQ_END)
        code = CPIRQ_END;
    osiProfileCode(code | CPEXITFLAG);
}
