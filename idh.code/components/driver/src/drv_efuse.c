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

#include <stdlib.h>
#include "hwregs.h"
#include "osi_log.h"
#include "osi_api.h"
#include "osi_compiler.h"
#include "osi_log.h"
#include "drv_efuse.h"

#define RDA_EFUSE_PUBKEY_START (8)
#define RDA_PUBKEY0_EFUSE_BLOCK_INDEX (8)
#define RDA_PUBKEY1_EFUSE_BLOCK_INDEX (10)
#define RDA_PUBKEY2_EFUSE_BLOCK_INDEX (12)
#define RDA_PUBKEY3_EFUSE_BLOCK_INDEX (14)
#define RDA_PUBKEY4_EFUSE_BLOCK_INDEX (16)
#define RDA_PUBKEY5_EFUSE_BLOCK_INDEX (18)
#define RDA_PUBKEY6_EFUSE_BLOCK_INDEX (20)
#define RDA_PUBKEY7_EFUSE_BLOCK_INDEX (30)
#define RDA_EFUSE_SECURITY_CFG_INDEX (22)
#define RDA_EFUSE_UNIQUE_ID_LOW_INDEX (24)
#define RDA_EFUSE_UNIQUE_ID_HIGH_INDEX (26)
#define RDA_EFUSE_SERIALNUM_CFG_INDEX (28)

#define readl(addr) (*(volatile unsigned int *)(addr))
#define writel(val, addr) (*(volatile unsigned int *)(addr) = (val))

#define BLOCK_MIX (0)
#define BLOCK_MAX (81)

static bool blockIsValid(int32_t block_index)
{
    if (block_index < 0 || block_index > BLOCK_MAX)
        return false;
    else
        return true;
}

void drvEfuseOpen(void)
{
    REG_EFUSE_CTRL_EFUSE_SEC_MAGIC_NUMBER_T magic;
    REG_FIELD_WRITE1(hwp_efuseCtrl->efuse_sec_magic_number, magic, sec_efuse_magic_number, 0x8910);

    REG_EFUSE_CTRL_EFUSE_SEC_EN_T sec_en;
    REG_FIELD_WRITE1(hwp_efuseCtrl->efuse_sec_en, sec_en, sec_vdd_en, 1);

    REG_EFUSE_CTRL_EFUSE_PW_SWT_T pw_swt;
    REG_FIELD_WRITE2(hwp_efuseCtrl->efuse_pw_swt, pw_swt, efs_enk1_on, 1, ns_s_pg_en, 1);
}

void drvEfuseClose(void)
{
    REG_EFUSE_CTRL_EFUSE_SEC_MAGIC_NUMBER_T magic;
    REG_FIELD_WRITE1(hwp_efuseCtrl->efuse_sec_magic_number, magic, sec_efuse_magic_number, 0x8910);

    REG_EFUSE_CTRL_EFUSE_SEC_EN_T sec_en;
    REG_FIELD_WRITE1(hwp_efuseCtrl->efuse_sec_en, sec_en, sec_vdd_en, 0);

    REG_EFUSE_CTRL_EFUSE_PW_SWT_T pw_swt;
    REG_FIELD_WRITE3(hwp_efuseCtrl->efuse_pw_swt, pw_swt,
                     efs_enk1_on, 0, efs_enk2_on, 1, ns_s_pg_en, 0);
}

bool drvEfuseRead(int32_t block_index, uint32_t *val)
{
    if (!blockIsValid(block_index))
        return false;
    *val = readl((uint32_t)&hwp_efuseCtrl->efuse_mem + block_index * 4) |
           readl((uint32_t)&hwp_efuseCtrl->efuse_mem + block_index * 4 + 4);

    return true;
}

bool drvEfuseWrite(int32_t block_index, uint32_t val)
{
    if (!blockIsValid(block_index))
        return false;
    writel(val, (uint32_t)&hwp_efuseCtrl->efuse_mem + block_index * 4);
    writel(val, (uint32_t)&hwp_efuseCtrl->efuse_mem + block_index * 4 + 4);
    return true;
}
