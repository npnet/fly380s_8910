/* Copyright (C) 2020 RDA Technologies Limited and/or its affiliates("RDA").
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
#include <stdlib.h>
#include <sys/queue.h>
#include "hwregs.h"
#include "osi_log.h"
#include "osi_api.h"
#include "osi_compiler.h"
#include "osi_log.h"
#include "hal_efuse.h"
#include "hal_hwspinlock.h"

#define readl(addr) (*(volatile unsigned int *)(addr))
#define writel(val, addr) (*(volatile unsigned int *)(addr) = (val))

#define EFUSE_AP_USE_BIT (1 << 1)
#define EFUSE_USE_MAP hwp_mailbox->sysmail100

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
} halEfuseContext_t;

static halEfuseContext_t gHalEfuseCtx;

static inline bool prvBlockIsValid(int32_t block_index)
{
    return (block_index > RDA_EFUSE_BLOCK_LOCK_7_INDEX && block_index <= RDA_EFUSE_BLOCK_MAX_INDEX);
}

static inline void prvEfuseSetDouble(bool enable)
{
    REG_EFUSE_CTRL_EFUSE_SEC_EN_T sec_en;
    REG_FIELD_CHANGE1(hwp_efuseCtrl->efuse_sec_en, sec_en, double_bit_en_sec, enable ? 1 : 0);
}

static bool prvEfuseRead(uint32_t block_index, uint32_t *val)
{
    *val = readl((uint32_t)&hwp_efuseCtrl->efuse_mem + block_index * 4);
    return (hwp_efuseCtrl->efuse_sec_err_flag == 0);
}

static bool prvEfuseWrite(uint32_t block_index, uint32_t val)
{
    if (!prvBlockIsValid(block_index))
        return false;

    REG_EFUSE_CTRL_EFUSE_PW_SWT_T pw_swt = {};
    pw_swt.b.ns_s_pg_en = 1;
    hwp_efuseCtrl->efuse_pw_swt = pw_swt.v;
    osiDelayUS(1000);

    pw_swt.b.efs_enk2_on = 0;
    hwp_efuseCtrl->efuse_pw_swt = pw_swt.v;
    osiDelayUS(1000);

    pw_swt.b.efs_enk1_on = 1;
    hwp_efuseCtrl->efuse_pw_swt = pw_swt.v;
    osiDelayUS(1000);

    writel(val, (uint32_t)&hwp_efuseCtrl->efuse_mem + block_index * 4);

    pw_swt.b.efs_enk1_on = 0;
    hwp_efuseCtrl->efuse_pw_swt = pw_swt.v;
    osiDelayUS(1000);

    pw_swt.b.efs_enk2_on = 1;
    hwp_efuseCtrl->efuse_pw_swt = pw_swt.v;
    osiDelayUS(1000);

    pw_swt.b.ns_s_pg_en = 0;
    hwp_efuseCtrl->efuse_pw_swt = pw_swt.v;
    osiDelayUS(1000);

    return (hwp_efuseCtrl->efuse_sec_err_flag == 0);
}

static void prvEfuseInit(bool sw_cache_enable)
{
    halEfuseContext_t *d = &gHalEfuseCtx;

    if (d->init)
        return;

    SLIST_INIT(&d->single_bit_list);
    SLIST_INIT(&d->double_bit_list);
    d->sw_cache_enable = sw_cache_enable;
    d->init = true;
}

void halEfuseOpen(void)
{
    prvEfuseInit(true);
    uint32_t lock = halHwspinlockAcquire(HAL_HWSPINLOCK_ID_EFUSE);
    if (EFUSE_USE_MAP == 0)
    {
        REG_EFUSE_CTRL_EFUSE_SEC_MAGIC_NUMBER_T magic = {};
        magic.b.sec_efuse_magic_number = 0x8910;
        hwp_efuseCtrl->efuse_sec_magic_number = magic.v;

        REG_EFUSE_CTRL_EFUSE_SEC_EN_T sec_en = {hwp_efuseCtrl->efuse_sec_en};
        sec_en.b.sec_vdd_en = 1;
        hwp_efuseCtrl->efuse_sec_en = sec_en.v;
    }
    EFUSE_USE_MAP |= EFUSE_AP_USE_BIT;
    halHwspinlockRelease(lock, HAL_HWSPINLOCK_ID_EFUSE);
}

void halEfuseClose(void)
{
    uint32_t lock = halHwspinlockAcquire(HAL_HWSPINLOCK_ID_EFUSE);
    EFUSE_USE_MAP &= ~EFUSE_AP_USE_BIT;
    if (EFUSE_USE_MAP == 0)
    {
        REG_EFUSE_CTRL_EFUSE_SEC_EN_T sec_en = {hwp_efuseCtrl->efuse_sec_en};
        sec_en.b.sec_vdd_en = 0;
        hwp_efuseCtrl->efuse_sec_en = sec_en.v;

        hwp_efuseCtrl->efuse_sec_magic_number = 0;
    }
    halHwspinlockRelease(lock, HAL_HWSPINLOCK_ID_EFUSE);
}

bool halEfuseWrite(uint32_t block_index, uint32_t val)
{
    bool ret;

    halEfuseContext_t *d = &gHalEfuseCtx;

    if (!prvBlockIsValid(block_index))
        return false;

    prvEfuseSetDouble(false);
    ret = prvEfuseWrite(block_index, val);

    if (ret && d->sw_cache_enable)
    {
        efuseBlock_t *block = (efuseBlock_t *)malloc(sizeof(efuseBlock_t));
        if (block == NULL)
            return false;

        block->index = block_index;
        block->data = val;

        SLIST_INSERT_HEAD(&d->single_bit_list, block, iter);
    }

    return ret;
}

bool halEfuseDoubleWrite(uint32_t block_index, uint32_t val)
{
    bool ret;
    halEfuseContext_t *d = &gHalEfuseCtx;

    if (!prvBlockIsValid(block_index * 2))
        return false;

    prvEfuseSetDouble(true);
    ret = prvEfuseWrite(block_index, val);

    if (ret && d->sw_cache_enable)
    {
        efuseBlock_t *block = (efuseBlock_t *)malloc(sizeof(efuseBlock_t));
        if (block == NULL)
            return false;

        block->index = block_index;
        block->data = val;

        SLIST_INSERT_HEAD(&d->double_bit_list, block, iter);
    }

    return ret;
}

bool halEfuseRead(uint32_t block_index, uint32_t *val)
{
    bool ret;
    halEfuseContext_t *d = &gHalEfuseCtx;

    if (!prvBlockIsValid(block_index))
        return false;

    if (val == NULL)
        return false;

    efuseBlock_t *block;
    if (d->sw_cache_enable)
    {
        SLIST_FOREACH(block, &d->single_bit_list, iter)
        {
            if (block->index == block_index)
            {
                *val = block->data;
                OSI_LOGD(0, "cache block %d = 0x%08x", block->index, block->data);
                return true;
            }
        }
    }

    prvEfuseSetDouble(false);
    ret = prvEfuseRead(block_index, val);

    if (ret && d->sw_cache_enable)
    {
        block = (efuseBlock_t *)malloc(sizeof(efuseBlock_t));
        if (block == NULL)
            return false;

        block->index = block_index;
        block->data = *val;

        OSI_LOGD(0, "uncache block %d = 0x%08x", block->index, block->data);

        SLIST_INSERT_HEAD(&d->single_bit_list, block, iter);
    }

    return true;
}

bool halEfuseDoubleRead(uint32_t block_index, uint32_t *val)
{
    bool ret;
    halEfuseContext_t *d = &gHalEfuseCtx;

    if (!prvBlockIsValid(block_index * 2))
        return false;

    if (val == NULL)
        return false;

    efuseBlock_t *block;
    if (d->sw_cache_enable)
    {
        SLIST_FOREACH(block, &d->double_bit_list, iter)
        {
            if (block->index == block_index)
            {
                *val = block->data;
                OSI_LOGD(0, "cache block %d = 0x%08x", block->index, block->data);
                return true;
            }
        }
    }

    prvEfuseSetDouble(true);
    ret = prvEfuseRead(block_index, val);

    if (ret && d->sw_cache_enable)
    {
        block = (efuseBlock_t *)malloc(sizeof(efuseBlock_t));
        if (block == NULL)
            return false;

        block->index = block_index;
        block->data = *val;

        OSI_LOGD(0, "uncache block %d = 0x%08x", block->index, block->data);
    }

    SLIST_INSERT_HEAD(&d->double_bit_list, block, iter);

    return true;
}
