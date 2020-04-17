/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
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

#include "boot_debuguart.h"
#include "osi_compiler.h"
#include <stdint.h>
#include <stddef.h>

#define BOOT_TRACE_SIZE (256)

static uint32_t gBootTraceBuf[BOOT_TRACE_SIZE / 4];
static uint32_t gBootTraceRequested;

uint32_t *bootTraceBufRequest(uint32_t size)
{
    if (size > BOOT_TRACE_SIZE)
        return NULL;
    gBootTraceRequested = size;
    return gBootTraceBuf;
}

void bootTraceBufFilled(void)
{
    bootDebuguartWriteAll(gBootTraceBuf, gBootTraceRequested);
    gBootTraceRequested = 0;
}

OSI_STRONG_ALIAS(osiTraceBufRequestLocked, bootTraceBufRequest);
OSI_STRONG_ALIAS(osiTraceBufRequest, bootTraceBufRequest);
OSI_STRONG_ALIAS(osiTraceBufFilled, bootTraceBufFilled);
