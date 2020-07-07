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

#include "drv_efuse.h"
#include "hal_config.h"
#include "hal_efuse.h"
#include "osi_api.h"

#define EFUSE_MASTER_AP (1 << 0)
#define EFUSE_MASTER_CP (1 << 1)

static uint8_t gOpenMaster = 0;

static void _openByMaster(uint8_t master)
{
    uint32_t critical = osiEnterCritical();
    if (gOpenMaster == 0)
        halEfuseOpen();
    gOpenMaster |= master;
    osiExitCritical(critical);
}

static void _closeByMaster(uint32_t master)
{
    uint32_t critical = osiEnterCritical();
    gOpenMaster &= ~master;
    if (gOpenMaster == 0)
        halEfuseClose();
    osiExitCritical(critical);
}

void drvEfuseOpen()
{
    _openByMaster(EFUSE_MASTER_AP);
}

void drvEfuseClose()
{
    _closeByMaster(EFUSE_MASTER_AP);
}

#ifdef CONFIG_SOC_8910

void drvEfuseOpen_CP()
{
    _openByMaster(EFUSE_MASTER_CP);
}

void drvEfuseClose_CP()
{
    _closeByMaster(EFUSE_MASTER_CP);
}

#endif

bool drvEfuseRead(uint32_t block_index, uint32_t *value)
{
    return halEfuseRead(block_index, value);
}

bool drvEfuseWrite(uint32_t block_index, uint32_t value)
{
    return halEfuseWrite(block_index, value);
}

bool drvEfuseDoubleRead(uint32_t block_index, uint32_t *value)
{
    return false;
}

bool drvEfuseDoubleWrite(uint32_t block_index, uint32_t value)
{
    return false;
}
