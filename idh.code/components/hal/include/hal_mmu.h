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

#ifndef _HAL_MMU_H_
#define _HAL_MMU_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// MMU table uses a simple model
// * Use L1 if possible
// * L2 is used for SRAM, pages used by CP can be set to RO
// * L2 page is used for ONE L1 entry. Shared memory pages can be set to RW
//      and others can be set to RO
// * AP memory should be started at L1 (1MB) boundary.

enum
{
    HAL_MMU_ACCESS_CACHE_RWX,
    HAL_MMU_ACCESS_CACHE_RX,
    HAL_MMU_ACCESS_CACHE_R,
    HAL_MMU_ACCESS_UNCACHE_RWX,
    HAL_MMU_ACCESS_UNCACHE_RX,
    HAL_MMU_ACCESS_UNCACHE_R,

    HAL_MMU_ACCESS_DEVICE_RW,
    HAL_MMU_ACCESS_DEVICE_R
};

void halMmuCreateTable(void);
void halMmuSetAccess(uintptr_t start, size_t size, uint32_t access);

#ifdef __cplusplus
}
#endif
#endif
