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

#include "hal_chip.h"
#include "osi_api.h"
#include "osi_log.h"
#include "hwregs.h"
#include <stdarg.h>

unsigned halCalcDivider24(unsigned input, unsigned output)
{
    if (input == 0 || output == 0 || output > input)
        return 0;

    unsigned delta = -1U;
    unsigned rnum = 0, rdenom = 0;
    for (unsigned num = 1; num < 0x400; num++)
    {
        uint64_t up = (uint64_t)input * num;
        unsigned denom = (up + output / 2) / output;
        if (denom == 0 || denom >= 0x4000)
            continue;

        unsigned out = up / denom;
        unsigned diff = (out > output) ? out - output : output - out;
        if (diff < delta)
        {
            delta = diff;
            rnum = num;
            rdenom = denom;
        }
        if (diff == 0)
            break;
    }

    if (delta == -1U)
        return 0;
    return (rdenom << 10) | rnum;
}

unsigned halCalcDivider20(unsigned input, unsigned output)
{
    if (input == 0 || output == 0 || output > (input / 6))
        return 0;

    unsigned delta = -1U;
    unsigned rset = 1;
    unsigned rdiv = 1;
    for (unsigned nset = 16; nset >= 6; nset--)
    {
        unsigned ndiv = (input + (nset * output / 2)) / (nset * output);
        if (ndiv <= 1 || ndiv >= (1 << 16))
            continue;

        unsigned out = input / (nset * ndiv);
        unsigned diff = (out > output) ? out - output : output - out;
        if (diff < delta)
        {
            delta = diff;
            rset = nset;
            rdiv = ndiv;
        }
    }

    if (delta == -1U)
        return 0;
    return ((rset - 1) << 16) | (rdiv - 1);
}
