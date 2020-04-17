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
#include <string.h>

#define PMU_REG_CHANGE_MAX (8)
#define VCORE_STABLE_TIME (160) // us

#define CYCLE_COUNT_PER_LOOP 5 // based on measurement, mips32 and mips16 are the same
#define LOOP_COUNT_PER_1KUS(hz) ((hz) / ((1000000 * CYCLE_COUNT_PER_LOOP) / 1024))

#define LOOP_DELAY(count)             \
    for (int n = (count); n > 0; --n) \
    OSI_NOP
#define DELAY_US(us) LOOP_DELAY(((us)*param->count_unit) / 1024)

typedef struct
{
    int pmu_reg_count;
    uint16_t pmu_reg_addr[PMU_REG_CHANGE_MAX];
    uint16_t pmu_reg_val[PMU_REG_CHANGE_MAX];
    bool pll_on;
    bool pll_off;
    bool ddr_cfg_changed;
    uint8_t ddr_divider;
    uint32_t ddr_clk_ctrl;
    uint32_t ddr_dqs_ctrl;
    uint32_t ifc_mask;
    uint32_t count_unit;
    uint32_t wait_us;
} halDDRChangeParam_t;

halDDRChangeParam_t gVcoreDDRParam OSI_SECTION_SRAM_UC_BSS;

static inline void _pmuWrite(uint16_t address, uint16_t data)
{
    unsigned cmd = (0 << 29) | (0 << 25) | ((address & 0x1ff) << 16) | data;

    // wait at least one tx space
    REG_SPI_STATUS_T status;
    REG_WAIT_FIELD_NEZ(status, hwp_spi3->status, tx_space);

    hwp_spi3->rxtx_buffer = cmd;

    REG_WAIT_FIELD_EQZ(status, hwp_spi3->status, active_status);
    REG_WAIT_FIELD_EQ(status, hwp_spi3->status, tx_space, SPI_TX_FIFO_SIZE);
}

// Due to PSRAM is inaccessible during this function, this function should
// only access uncachable SRAM and CPU general purpose registers. Even read
// cachable SRAM is prohibitted, due to it may cause unintentionally DCache
// write back. And DCache clean is expensive on 8955.
OSI_NO_INLINE OSI_SECTION_SRAM_TEXT static void _changeVcoreDDR(halDDRChangeParam_t *param)
{
    // wait a while to let PSRAM into idle (> 40 PSRAM clock cycles)
    DELAY_US(16);

    for (int n = 0; n < param->pmu_reg_count; n++)
        _pmuWrite(param->pmu_reg_addr[n], param->pmu_reg_val[n]);

    // if (param->pmu_reg_count > 0)
    //     DELAY_US(160);

    REG_SYS_CTRL_SEL_CLOCK_T sel_clock = {hwp_sysCtrl->sel_clock};
    REG_SYS_CTRL_PLL_CTRL_T pll_ctrl;
    if (param->pll_on)
    {
        if (sel_clock.b.pll_locked == 0)
        {
            // Turning off the Pll and reset of the Lock detector
            REG_FIELD_WRITE3(hwp_sysCtrl->pll_ctrl, pll_ctrl,
                             pll_enable, 0,
                             pll_lock_reset, 0, // 0 for reset
                             pll_clk_fast_enable, 0);

            DELAY_US(8);

            // Turning on the Pll
            REG_FIELD_WRITE3(hwp_sysCtrl->pll_ctrl, pll_ctrl,
                             pll_enable, 1,
                             pll_lock_reset, 1, // 1 for not rest
                             pll_clk_fast_enable, 0);

            // Wait for lock.
            REG_WAIT_FIELD_NEZ(sel_clock, hwp_sysCtrl->sel_clock, pll_locked);

            DELAY_US(8);

            // Enable PLL clock for fast clock
            REG_FIELD_CHANGE1(hwp_sysCtrl->pll_ctrl, pll_ctrl, pll_clk_fast_enable, 1);
        }

        sel_clock.b.sys_sel_fast = 0; // fast
        sel_clock.b.mem_bridge_sel_fast = 0;
        sel_clock.b.spiflash_sel_fast = 0;
        sel_clock.b.bb_sel_fast = 0;
        hwp_sysCtrl->sel_clock = sel_clock.v;
        hwp_sysCtrl->cfg_clk_mem_bridge = param->ddr_divider;
    }

    if (param->pll_off)
    {
        sel_clock.b.sys_sel_fast = 1; // fast
        sel_clock.b.mem_bridge_sel_fast = 1;
        sel_clock.b.spiflash_sel_fast = 1;
        sel_clock.b.bb_sel_fast = 1;
        hwp_sysCtrl->sel_clock = sel_clock.v;
        hwp_sysCtrl->cfg_clk_mem_bridge = HAL_CLK_DIV12P0; // 52MHz

        if (sel_clock.b.pll_locked != 0)
        {
            // Turning off the Pll and reset of the Lock detector
            REG_FIELD_WRITE3(hwp_sysCtrl->pll_ctrl, pll_ctrl,
                             pll_enable, 0,
                             pll_lock_reset, 0, // 0 for reset
                             pll_clk_fast_enable, 0);
        }
    }

    if (param->ddr_cfg_changed)
    {
        DELAY_US(8);
        hwp_psram8Ctrl->clk_ctrl = param->ddr_clk_ctrl;
        hwp_psram8Ctrl->dqs_ctrl = param->ddr_dqs_ctrl;
        DELAY_US(8);
    }

    DELAY_US(param->wait_us);
}

static uint32_t _flushRxIFC(void)
{
    uint32_t rxmask = 0;
    REG_SYS_IFC_STATUS_T status;
    REG_SYS_IFC_CONTROL_T control;

    for (int ch = 0; ch < SYS_IFC_STD_CHAN_NB; ch++)
    {
        status.v = hwp_sysIfc->std_ch[ch].status;
        if (status.b.enable && hwp_sysIfc->std_ch[ch].tc != 0)
        {
            control.v = hwp_sysIfc->std_ch[ch].control;
            if ((control.b.req_src & 1) == 1) // RX
            {
                rxmask |= (1 << ch);

                control.b.flush = 1;
                hwp_sysIfc->std_ch[ch].control = control.v;
                REG_WAIT_FIELD_NEZ(status, hwp_sysIfc->std_ch[ch].status, fifo_empty);
            }
        }
    }

    return rxmask;
}

static void _restoreRxIFC(uint32_t mask)
{
    REG_SYS_IFC_CONTROL_T control;

    for (int ch = 0; ch < SYS_IFC_STD_CHAN_NB; ch++)
    {
        if ((mask & (1 << ch)) != 0)
            REG_FIELD_CHANGE1(hwp_sysIfc->std_ch[ch].control, control, flush, 0);
    }
}

static bool _ddrChangePermitted(void)
{
    // RFSPI IFC will use SRAM, skip it

    // TODO: check VOC
    // TODO: check gouda
    // TODO: check DMA

    for (int ch = 0; ch < SYS_IFC_STD_CHAN_NB; ch++)
    {
        REG_SYS_IFC_STATUS_T status = {hwp_sysIfc->std_ch[ch].status};
        if (status.b.enable && hwp_sysIfc->std_ch[ch].tc != 0)
        {
            REG_SYS_IFC_CONTROL_T control = {hwp_sysIfc->std_ch[ch].control};
            if ((control.b.req_src & 1) == 0) // TX
            {
                uint32_t start_addr = hwp_sysIfc->std_ch[ch].start_addr;
                if ((start_addr & 0x0f000000) == 0x02000000) // PSRAM
                    return false;
            }
        }
    }

    return true;
}

bool halSysPllPsramReconfig(bool pll_needed)
{
    if (!_ddrChangePermitted())
        return false;

    halDDRChangeParam_t *param = &gVcoreDDRParam;
    if (pll_needed)
    {
        param->ddr_divider = halFreqToPllDivider(CONFIG_MEM_CLK_HIGH_FREQ);
        param->pmu_reg_count = halRequestVcoreRegs(HAL_VCORE_USER_SYS, HAL_VCORE_LEVEL_MEDIUM,
                                                   param->pmu_reg_addr, param->pmu_reg_val);
    }
    else
    {
        param->ddr_divider = halFreqToPllDivider(CONFIG_MEM_CLK_LOW_FREQ);
        param->pmu_reg_count = halRequestVcoreRegs(HAL_VCORE_USER_SYS, HAL_VCORE_LEVEL_LOW,
                                                   param->pmu_reg_addr, param->pmu_reg_val);

        if (halGetVcoreLevel() != HAL_VCORE_LEVEL_LOW)
            osiPanic();
    }

    // it should be called after vcore level is changed
    const halPsramCfg_t *cfg = halGetCurrentPsramCfg();
    param->ddr_dqs_ctrl = cfg->ddr_dqs_ctrl;
    param->ddr_clk_ctrl = cfg->ddr_clk_ctrl;
    param->pll_on = pll_needed;
    param->pll_off = !pll_needed;
    param->ddr_cfg_changed = true;
    param->count_unit = LOOP_COUNT_PER_1KUS(gSysClkFreq);
    param->wait_us = VCORE_STABLE_TIME;
    param->ifc_mask = _flushRxIFC();

    _changeVcoreDDR(&gVcoreDDRParam);
    _restoreRxIFC(param->ifc_mask);
    return true;
}

static bool _pmuWriteProtected(int count, const uint16_t *addr, const uint16_t *val, uint32_t wait_us, const halPsramCfg_t *cfg)
{
    if (count > PMU_REG_CHANGE_MAX)
        osiPanic();

    if (!_ddrChangePermitted())
        return false;

    halDDRChangeParam_t *param = &gVcoreDDRParam;
    param->pmu_reg_count = count;
    memcpy(param->pmu_reg_addr, addr, count * sizeof(uint16_t));
    memcpy(param->pmu_reg_val, val, count * sizeof(uint16_t));
    param->pll_on = false;
    param->pll_off = false;
    if (cfg != NULL)
    {
        param->ddr_cfg_changed = true;
        param->ddr_dqs_ctrl = cfg->ddr_dqs_ctrl;
        param->ddr_clk_ctrl = cfg->ddr_clk_ctrl;
    }
    else
    {
        param->ddr_cfg_changed = false;
    }

    param->count_unit = LOOP_COUNT_PER_1KUS(gSysClkFreq);
    param->ifc_mask = _flushRxIFC();
    param->wait_us = wait_us;

    _changeVcoreDDR(&gVcoreDDRParam);
    _restoreRxIFC(param->ifc_mask);
    return true;
}

bool halPmuWriteProtected(int count, const uint16_t *addr, const uint16_t *val, uint32_t wait_us)
{
    return _pmuWriteProtected(count, addr, val, wait_us, NULL);
}

bool halChangeVcoreRegs(int count, const uint16_t *addr, const uint16_t *val, const halPsramCfg_t *cfg)
{
    return _pmuWriteProtected(count, addr, val, VCORE_STABLE_TIME, cfg);
}
