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
#include "osi_log.h"
#include <stdarg.h>

static osiMutex_t *gEfuseAccessLock = NULL;

static void prvEfuseEnable()
{
    uint32_t critical = osiEnterCritical();
    if (gEfuseAccessLock == NULL)
    {
        gEfuseAccessLock = osiMutexCreate();
        if (gEfuseAccessLock == NULL)
            osiPanic();
    }
    osiExitCritical(critical);

    osiMutexLock(gEfuseAccessLock);
    halEfuseOpen();
}

static void prvEfuseDisable()
{
    halEfuseClose();
    osiMutexUnlock(gEfuseAccessLock);
}

bool drvEfuseBatchRead(bool double_block, uint32_t index, uint32_t *value, ...)
{
    bool (*read_func)(uint32_t index, uint32_t * value) = NULL;
    if (double_block)
    {
        read_func = halEfuseDoubleRead;
    }
    else
    {
        read_func = halEfuseRead;
    }

    va_list ap;
    va_start(ap, value);

    bool result = true;
    bool first = true;
    prvEfuseEnable();
    for (;;)
    {
        if (!first)
        {
            index = va_arg(ap, uint32_t);
            if (DRV_EFUSE_ACCESS_END == index)
                break;
            value = va_arg(ap, uint32_t *);
        }

        OSI_LOGV(0, "EFUSE read %u/%d", index, double_block);
        result = read_func(index, value);
        if (result == false)
            break;
        first = false;
    }
    prvEfuseDisable();
    va_end(ap);

    return result;
}

bool drvEfuseBatchWrite(bool double_block, uint32_t index, uint32_t value, ...)
{
    bool (*write_func)(uint32_t index, uint32_t value) = NULL;
    if (double_block)
    {
        write_func = halEfuseDoubleWrite;
    }
    else
    {
        write_func = halEfuseWrite;
    }

    va_list ap;
    va_start(ap, value);

    bool result = true;
    bool first = true;
    prvEfuseEnable();
    for (;;)
    {
        if (!first)
        {
            index = va_arg(ap, uint32_t);
            if (DRV_EFUSE_ACCESS_END == index)
                break;
            value = va_arg(ap, uint32_t);
        }

        OSI_LOGV(0, "EFUSE write %u/%u/%d", index, value, double_block);
        result = write_func(index, value);
        if (result == false)
            break;
        first = false;
    }
    prvEfuseDisable();
    va_end(ap);

    return result;
}

bool drvEfuseRead(bool double_block, uint32_t index, uint32_t *value)
{
    return drvEfuseBatchRead(double_block, index, value, DRV_EFUSE_ACCESS_END);
}

bool drvEfuseWrite(bool double_block, uint32_t index, uint32_t value)
{
    return drvEfuseBatchWrite(double_block, index, value, DRV_EFUSE_ACCESS_END);
}
