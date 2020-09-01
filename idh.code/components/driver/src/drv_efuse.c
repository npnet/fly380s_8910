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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('E', 'F', 'U', 'S')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include <sys/queue.h>
#include <stdlib.h>
#include "drv_efuse.h"
#include "hal_config.h"
#include "hal_efuse.h"
#include "osi_api.h"
#include "osi_log.h"
#include <stdarg.h>

static osiMutex_t *gEfuseAccessLock = NULL;

typedef SLIST_ENTRY(efuseBlock) efuseCacheIter_t;
typedef SLIST_HEAD(efuseCacheHead, efuseBlock) efuseCacheHead_t;
typedef struct efuseBlock
{
    efuseCacheIter_t iter;
    uint32_t index;
    uint32_t data;
} efuseBlock_t;

typedef struct
{
    efuseCacheHead_t single_bit_list;
    efuseCacheHead_t double_bit_list;
    bool sw_cache_enable;
    bool init;
} drvEfuseContext_t;

static drvEfuseContext_t gDrvEfuseCtx;

static void prvEfuseInit(void)
{
    drvEfuseContext_t *d = &gDrvEfuseCtx;

    if (d->init)
        return;

    SLIST_INIT(&d->single_bit_list);
    SLIST_INIT(&d->double_bit_list);
    d->init = true;
    d->sw_cache_enable = true;
}

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
    prvEfuseInit();
    halEfuseOpen();
}

static void prvEfuseDisable()
{
    halEfuseClose();
    osiMutexUnlock(gEfuseAccessLock);
}

static void prvEfuseCacheClean(bool double_block, uint32_t block_index)
{
    drvEfuseContext_t *d = &gDrvEfuseCtx;
    efuseBlock_t *block;
    efuseCacheHead_t *cache_list;

    if (d->sw_cache_enable)
    {
        cache_list = double_block ? &d->double_bit_list : &d->single_bit_list;

        SLIST_FOREACH(block, cache_list, iter)
        {
            if (block->index == block_index)
            {
                SLIST_REMOVE(cache_list, block, efuseBlock, iter);
                OSI_LOGD(0, "cache clean block %d = 0x%08x", block->index, block->data);
            }
        }
    }
}

static bool prvEfuseCacheRead(bool double_block, uint32_t block_index, uint32_t *value)
{
    drvEfuseContext_t *d = &gDrvEfuseCtx;
    efuseBlock_t *block;
    efuseCacheHead_t *cache_list;

    if (d->sw_cache_enable)
    {
        cache_list = double_block ? &d->double_bit_list : &d->single_bit_list;

        SLIST_FOREACH(block, cache_list, iter)
        {
            if (block->index == block_index)
            {
                *value = block->data;
                OSI_LOGD(0, "cache block %d = 0x%08x", block->index, block->data);
                return true;
            }
        }
    }

    return false;
}

static bool prvEfuseCacheWrite(bool double_block, uint32_t block_index, uint32_t *value)
{
    drvEfuseContext_t *d = &gDrvEfuseCtx;
    efuseBlock_t *block;
    efuseCacheHead_t *cache_list;

    if (d->sw_cache_enable)
    {
        block = (efuseBlock_t *)malloc(sizeof(efuseBlock_t));
        if (block == NULL)
            return false;

        block->index = block_index;
        block->data = *value;

        OSI_LOGD(0, "uncache block %d = 0x%08x", block->index, block->data);

        cache_list = double_block ? &d->double_bit_list : &d->single_bit_list;
        SLIST_INSERT_HEAD(cache_list, block, iter);
    }

    return true;
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
    bool cache_read = true;
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
        cache_read = prvEfuseCacheRead(double_block, index, value);
        if (cache_read)
            result = true;
        else
            result = read_func(index, value);

        if (result == false)
            break;

        if (result && !cache_read)
            prvEfuseCacheWrite(double_block, index, value);

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
        else
            prvEfuseCacheClean(double_block, index);

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
