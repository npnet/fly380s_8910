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
#include "hwregs.h"
#include "osi_api.h"
#include "osi_clock.h"

#define PMU_REG_INDEX(r) (((uint32_t)(r) >> 2) & 0x7f)

typedef struct
{
    uint8_t vcore_mode;
    uint8_t vcore_level;
    uint8_t vcore_level_req[HAL_VCORE_USER_COUNT];
} halPmuContext_t;

static halPmuContext_t gPmuCtx OSI_SECTION_SRAM_DATA;

static uint8_t gVcoreLDOLevelValue[HAL_VCORE_LEVEL_COUNT] = {
    CONFIG_VCORE_LDO_LOW,
    CONFIG_VCORE_LDO_MEDIUM,
    CONFIG_VCORE_LDO_HIGH,
};

static uint8_t gVcoreDCDCLevelValue[HAL_VCORE_LEVEL_COUNT] = {
    CONFIG_VCORE_DCDC_LOW,
    CONFIG_VCORE_DCDC_MEDIUM,
    CONFIG_VCORE_DCDC_HIGH,
};

void halPmuInit(void)
{
    halPmuContext_t *d = &gPmuCtx;

#ifdef CONFIG_DEFAULT_VCORE_DCDC_MODE
    d->vcore_mode = HAL_VCORE_MODE_DCDC;
#else
    d->vcore_mode = HAL_VCORE_MODE_LDO;
#endif
    d->vcore_level = HAL_VCORE_LEVEL_MEDIUM;

    for (int n = 0; n < HAL_VCORE_USER_COUNT; n++)
        d->vcore_level_req[n] = HAL_VCORE_LEVEL_LOW;
    d->vcore_level_req[HAL_VCORE_USER_SYS] = HAL_VCORE_LEVEL_MEDIUM;
}

const halPsramCfg_t *halGetCurrentPsramCfg(void)
{
    return halGetPsramCfg(gPmuCtx.vcore_mode, gPmuCtx.vcore_level);
}

uint32_t halGetVcoreMode(void)
{
    return gPmuCtx.vcore_mode;
}

uint32_t halGetVcoreLevel(void)
{
    return gPmuCtx.vcore_level;
}

static uint32_t _highestVcoreRequest(void)
{
    halPmuContext_t *p = &gPmuCtx;

    uint32_t level = p->vcore_level_req[0];
    for (int n = 1; n < HAL_VCORE_USER_COUNT; n++)
    {
        if (p->vcore_level_req[n] > level)
            level = p->vcore_level_req[n];
    }
    return level;
}

static int _vcoreChangeRegs(uint32_t mode, uint32_t level, uint16_t *addr, uint16_t *value)
{
    int count = 0;
    if (mode == HAL_VCORE_MODE_LDO)
    {
        REG_PMU_LDO_ACTIVE_SETTING_1_03H_T r03h = {halPmuRead(&hwp_pmu->ldo_active_setting_1_03h)};
        r03h.b.pd_buck1_ldo_act = 0;
        r03h.b.pfm_mode_sel_buck1_act = 0;
        r03h.b.pu_buck1_act = 0;
        *addr++ = PMU_REG_INDEX(&hwp_pmu->ldo_active_setting_1_03h);
        *value++ = r03h.v;
        count++;

        REG_PMU_LDO_LP_SETTING_1_08H_T r08h = {halPmuRead(&hwp_pmu->ldo_lp_setting_1_08h)};
        r08h.b.pd_buck1_ldo_lp = 0;
        r08h.b.pfm_mode_sel_buck1_lp = 0;
        r08h.b.pu_buck1_lp = 0;
        *addr++ = PMU_REG_INDEX(&hwp_pmu->ldo_lp_setting_1_08h);
        *value++ = r08h.v;
        count++;

        REG_PMU_PMU_8809_4BH_T r4bh = {halPmuRead(&hwp_pmu->pmu_8809_4bh)};
        r4bh.b.vbuck1_ldo_bit_nlp = gVcoreLDOLevelValue[level];
        *addr++ = PMU_REG_INDEX(&hwp_pmu->pmu_8809_4bh);
        *value++ = r4bh.v;
        count++;
    }
    else
    {
        REG_PMU_LDO_ACTIVE_SETTING_1_03H_T r03h = {halPmuRead(&hwp_pmu->ldo_active_setting_1_03h)};
        r03h.b.pd_buck1_ldo_act = 0;
        r03h.b.pfm_mode_sel_buck1_act = 0;
        r03h.b.pu_buck1_act = 1;
        *addr++ = PMU_REG_INDEX(&hwp_pmu->ldo_active_setting_1_03h);
        *value++ = r03h.v;
        count++;

        REG_PMU_LDO_LP_SETTING_1_08H_T r08h = {halPmuRead(&hwp_pmu->ldo_lp_setting_1_08h)};
        r08h.b.pd_buck1_ldo_lp = 0;
        r08h.b.pfm_mode_sel_buck1_lp = 1;
        r08h.b.pu_buck1_lp = 1;
        *addr++ = PMU_REG_INDEX(&hwp_pmu->ldo_lp_setting_1_08h);
        *value++ = r08h.v;
        count++;

        REG_PMU_PMU_8809_2FH_T r2fh = {halPmuRead(&hwp_pmu->pmu_8809_2fh)};
        r2fh.b.vbuck1_bit_nlp = gVcoreDCDCLevelValue[level];
        *addr++ = PMU_REG_INDEX(&hwp_pmu->pmu_8809_2fh);
        *value++ = r2fh.v;
        count++;
    }
    return count;
}

uint8_t halRequestVcoreRegs(uint32_t id, uint32_t level, uint16_t *addr, uint16_t *value)
{
    halPmuContext_t *p = &gPmuCtx;

    if (id >= HAL_VCORE_USER_COUNT || level >= HAL_VCORE_LEVEL_COUNT)
        osiPanic();

    p->vcore_level_req[id] = level;
    uint32_t new_level = _highestVcoreRequest();
    if (new_level == p->vcore_level)
        return 0;

    p->vcore_level = new_level;
    return _vcoreChangeRegs(p->vcore_mode, p->vcore_level, addr, value);
}

bool halRequestVcore(uint32_t id, uint32_t level)
{
    halPmuContext_t *p = &gPmuCtx;

    if (id >= HAL_VCORE_USER_COUNT || level >= HAL_VCORE_LEVEL_COUNT)
        return false;

    uint32_t critical = osiEnterCritical();
    p->vcore_level_req[id] = level;
    uint32_t new_level = _highestVcoreRequest();
    if (new_level == p->vcore_level)
    {
        osiExitCritical(critical);
        return true;
    }

    const halPsramCfg_t *cfg = halGetPsramCfg(p->vcore_mode, new_level);
    uint16_t reg_addr[4];
    uint16_t reg_val[4];
    int count = _vcoreChangeRegs(p->vcore_mode, new_level, reg_addr, reg_val);
    bool changed = halChangeVcoreRegs(count, reg_addr, reg_val, cfg);
    if (changed)
        p->vcore_level = new_level;

    osiExitCritical(critical);
    return changed;
}

bool halPmuSetSimVolt(int idx, halSimVoltClass_t volt)
{
    if (idx >= CONFIG_HAL_SIM_COUNT || volt == HAL_SIM_VOLT_CLASS_A)
        return false;

    bool sim_enabled = (volt != HAL_SIM_VOLT_OFF);
    bool sim_1v8 = (volt == HAL_SIM_VOLT_CLASS_C);

    REG_PMU_LDO_SETTINGS_02H_T r02h = {halPmuRead(&hwp_pmu->ldo_settings_02h)};
    REG_PMU_LDO_ACTIVE_SETTING_5_07H_T r07h = {halPmuRead(&hwp_pmu->ldo_active_setting_5_07h)};
    REG_PMU_LDO_LP_SETTING_5_0CH_T r0ch = {halPmuRead(&hwp_pmu->ldo_lp_setting_5_0ch)};

    if (idx == 0)
    {
        r02h.b.vsim1_enable = sim_enabled;
        r07h.b.vsim1_vsel_act = sim_1v8 ? 1 : 0;
        r0ch.b.vsim1_vsel_lp = sim_1v8 ? 1 : 0;
    }
    else
    {
        r02h.b.vsim2_enable = sim_enabled;
        r07h.b.vsim2_vsel_act = sim_1v8 ? 1 : 0;
        r0ch.b.vsim2_vsel_lp = sim_1v8 ? 1 : 0;
    }

    halPmuWrite(&hwp_pmu->ldo_active_setting_5_07h, r07h.v);
    halPmuWrite(&hwp_pmu->ldo_lp_setting_5_0ch, r0ch.v);
    halPmuWrite(&hwp_pmu->ldo_settings_02h, r02h.v);

    osiDelayUS(60); //delay 60us, waiting vsim stable
    return true;
}

bool halPmuSelectSim(int idx)
{
    if (idx >= CONFIG_HAL_SIM_COUNT)
        return false;

    REG_ABB_GPADC_SETTINGS_01H_T r01h = {};
    r01h.b.unsel_rst_val_1 = 1;
    r01h.b.unsel_rst_val_2 = 1;
    r01h.b.unsel_rst_val_3 = 1;
    r01h.b.unsel_rst_val_4 = 1;

    if (idx == 0)
        r01h.b.sim_select = 0;
    else if (idx == 1)
        r01h.b.sim_select = 1;
    else if (idx == 2)
        r01h.b.sim_select = 2;
    else if (idx == 3)
        r01h.b.sim_select = 3;
    else
        return false;

    halAbbWrite(&hwp_abb->gpadc_settings_01h, r01h.v);
    return true;
}

OSI_NO_RETURN void halShutdown(int mode, int64_t wake_uptime)
{
    OSI_DEAD_LOOP;
}