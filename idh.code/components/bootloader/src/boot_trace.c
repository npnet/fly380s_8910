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

bool bootTraceBufPut(const void *data, unsigned size)
{
    bootDebuguartWriteAll(data, size);
    return true;
}

bool bootTraceBufPutMulti(const osiBuffer_t *bufs, unsigned count, int size)
{
    for (unsigned n = 0; n < count; n++)
        bootDebuguartWriteAll((const void *)bufs[n].ptr, bufs[n].size);
    return true;
}

OSI_DECL_STRONG_ALIAS(bootTraceBufPut, bool osiTraceBufPut(const void *data, unsigned size));
OSI_DECL_STRONG_ALIAS(bootTraceBufPutMulti, bool osiTraceBufPutMulti(const osiBuffer_t *bufs, unsigned count, int size));
