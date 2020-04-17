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

#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "hwregs.h"
#include "osi_api.h"
#include "osi_log.h"
#include "hal_adi_bus.h"
#include "hal_chip.h"
#include "hal_config.h"
#include <stdlib.h>
#include <string.h>

#define PWR_WR_PROT_MAGIC (0x6e7f)
#define PSM_WR_PROT_MAGIC (0x454e)

#define PSM_MIN_CNT (5)
#define PSM_MODE_REG (hwp_rda2720mPsm->reserved_2)
#define PMU_BOOT_MODE_REG (hwp_rda2720mGlobal->por_rst_monitor)

#define PSM_CNT_MS (80) // (clk_cal_64k_div_th + 1) * 10ms

uint32_t halPmuReadPsmInitCnt(void)
{
    uint32_t cnt_init = halAdiBusRead(&hwp_rda2720mPsm->psm_cnt_update_l_value);
    uint32_t cnt;
    REG_WAIT_COND((cnt = halAdiBusRead(&hwp_rda2720mPsm->psm_cnt_update_l_value)) != cnt_init);

    uint32_t cnt_h = halAdiBusRead(&hwp_rda2720mPsm->psm_cnt_update_h_value);
    uint32_t cnt_l = halAdiBusRead(&hwp_rda2720mPsm->psm_cnt_update_l_value);
    if (cnt_l != cnt)
        cnt_h = halAdiBusRead(&hwp_rda2720mPsm->psm_cnt_update_h_value);
    return (cnt_h << 16) | cnt_l;
}

void halPmuCheckPsm(void)
{
    REG_RDA2720M_GLOBAL_MODULE_EN0_T module_en0;

    // enable PSM module
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->module_en0, module_en0, psm_en, 1);

    uint32_t psm_magic = halAdiBusRead(&PSM_MODE_REG);
    OSI_LOGD(0, "pmu: psm magic 0x%x", psm_magic);

    if (psm_magic == OSI_SHUTDOWN_PSM_SLEEP || psm_magic == OSI_SHUTDOWN_POWER_OFF)
    {
        osiSetBootCause(OSI_BOOTCAUSE_PSM_WAKEUP);

        uint32_t psm_cnt = halPmuReadPsmInitCnt();
        osiSetUpTime(psm_cnt * PSM_CNT_MS);

        if (psm_magic == OSI_SHUTDOWN_PSM_SLEEP)
            osiSetBootMode(OSI_BOOTMODE_PSM_RESTORE);

        OSI_LOGD(0, "pmu: wakeup cnt/%u", psm_cnt);

        // clear it, and it will only be set before PSM power off
        halAdiBusWrite(&PSM_MODE_REG, 0);
    }

    // disable PSM module
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->module_en0, module_en0, psm_en, 0);
}

void halBootCauseMode(void)
{
    REG_RDA2720M_GLOBAL_POR_7S_CTRL_T por_7s_ctrl;
    REG_RDA2720M_GLOBAL_POR_SRC_FLAG_T por_src_flag;
    REG_RDA2720M_GLOBAL_MODULE_EN0_T module_en0;
    REG_RDA2720M_GLOBAL_RTC_CLK_EN0_T rtc_clk_en0;
    REG_RDA2720M_GLOBAL_SWRST_CTRL0_T swrst_ctrl0;

    swrst_ctrl0.v = halAdiBusRead(&hwp_rda2720mGlobal->swrst_ctrl0);
    por_src_flag.v = halAdiBusRead(&hwp_rda2720mGlobal->por_src_flag);
    OSI_LOGI(0, "pmu: boot por_src_flag 0x%08x, swrst_ctrl0 0x%x",
              por_src_flag.v, swrst_ctrl0.b.reg_rst_en);
    if (swrst_ctrl0.b.reg_rst_en == 0)
    {
        if (por_src_flag.v & (1 << 11))
            osiSetBootCause(OSI_BOOTCAUSE_PIN_RESET);

        if ((por_src_flag.v & (1 << 12)) ||
            (por_src_flag.v & (1 << 9)) ||
            (por_src_flag.v & (1 << 8)))
            osiSetBootCause(OSI_BOOTCAUSE_PWRKEY);

        if (por_src_flag.v & (1 << 6))
            osiSetBootCause(OSI_BOOTCAUSE_ALARM);

        if (por_src_flag.v & ((1 << 4) | (1 << 5) | (1 << 10)))
            osiSetBootCause(OSI_BOOTCAUSE_CHARGE);
    }
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->por_src_flag, por_src_flag,
                    reg_soft_rst_flg_clr, 1); // clear bit14
    REG_ADI_CHANGE4(hwp_rda2720mGlobal->por_7s_ctrl, por_7s_ctrl,
                    pbint_7s_flag_clr, 1, // clear bit12
                    chgr_int_flag_clr, 1, // clear bit10
                    pbint2_flag_clr, 1,   // clear bit9
                    pbint_flag_clr, 1     // clear bit8
    );

    // enable watchdog to check watchdog reset
    halAdiBusWrite(&hwp_rda2720mWdg->wdg_ctrl, 0); // clear wdg_run
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->module_en0, module_en0, wdg_en, 1);
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->rtc_clk_en0, rtc_clk_en0, rtc_wdg_en, 1);

    REG_RDA2720M_WDG_WDG_INT_RAW_T wdg_int_raw;
    wdg_int_raw.v = halAdiBusRead(&hwp_rda2720mWdg->wdg_int_raw);
    if (wdg_int_raw.b.wdg_int_raw)
    {
        osiSetBootCause(OSI_BOOTCAUSE_WDG);

        REG_RDA2720M_WDG_WDG_INT_CLR_T wdg_int_clr;
        REG_ADI_CHANGE1(hwp_rda2720mWdg->wdg_int_raw, wdg_int_clr, wdg_rst_clr, 1);
    }

    // disable watchdog and it should re-enable if used
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->module_en0, module_en0, wdg_en, 0);
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->rtc_clk_en0, rtc_clk_en0, rtc_wdg_en, 0);

    osiBootMode_t boot_mode = osiGetBootMode();
    uint32_t boot_mode_val = halAdiBusRead(&PMU_BOOT_MODE_REG);

    REG_SYS_CTRL_RESET_CAUSE_T reset_cause = {hwp_sysCtrl->reset_cause};
    OSI_LOGD(0, "pmu: boot mode magic 0x%x, por_src_flag 0x%x, wdg_int_raw 0x%x reset_cause 0x%x",
             boot_mode_val, por_src_flag.v, wdg_int_raw.v, reset_cause.v);

    if (boot_mode == OSI_BOOTMODE_NORMAL)
        osiSetBootMode(boot_mode_val);
    halAdiBusWrite(&PMU_BOOT_MODE_REG, 0);
}

void halPmuInit(void)
{
    halPmuUnlockPowerReg();

    halPmuSwitchPower(HAL_POWER_SPIMEM, true, true);
    halPmuSwitchPower(HAL_POWER_MEM, true, true);
    halPmuSwitchPower(HAL_POWER_VIO18, true, true);
    halPmuSwitchPower(HAL_POWER_VDD28, true, false);
    halPmuSwitchPower(HAL_POWER_DCXO, true, true);

#ifdef CONFIG_CAMA_POWER_ON_INIT
    halPmuSwitchPower(HAL_POWER_CAMA, true, false);
#else
    halPmuSwitchPower(HAL_POWER_CAMA, false, false);
#endif
    halPmuSwitchPower(HAL_POWER_CAMD, false, false);
    halPmuSwitchPower(HAL_POWER_LCD, false, false);
    halPmuSwitchPower(HAL_POWER_WCN, false, false);
    halPmuSwitchPower(HAL_POWER_USB, false, false);
    halPmuSwitchPower(HAL_POWER_VIBR, false, false);
    halPmuSwitchPower(HAL_POWER_SD, false, false);
    halPmuSwitchPower(HAL_POWER_BUCK_PA, false, false);
#ifdef CONFIG_BOARD_KPLED_USED_FOR_RF_SWITCH
    halPmuSwitchPower(HAL_POWER_KEYLED, true, false);
#else
    halPmuSwitchPower(HAL_POWER_KEYLED, false, false);
#endif

    // fgu pd mode
    REG_RDA2720M_GLOBAL_CHGR_DET_FGU_CTRL_T fgu_ctrl;
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->chgr_det_fgu_ctrl, fgu_ctrl,
                    ldo_fgu_pd, 1);

    // Not exist: slp_ldocamio_pd_en,
    REG_RDA2720M_GLOBAL_SLP_LDO_PD_CTRL0_T slp_ldo_pd_ctrl0;
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                    slp_ldorf15_pd_en, 1);

    REG_RDA2720M_GLOBAL_SLP_LDO_PD_CTRL1_T slp_ldo_pd_ctrl1;
    REG_ADI_CHANGE2(hwp_rda2720mGlobal->slp_ldo_pd_ctrl1, slp_ldo_pd_ctrl1,
                    slp_ldocp_pd_en, 1,
                    slp_ldoana_pd_en, 1);

    halAdiBusWrite(&hwp_rda2720mGlobal->slp_ldo_lp_ctrl0, 0x7fff); // all '1'
    halAdiBusWrite(&hwp_rda2720mGlobal->slp_ldo_lp_ctrl1, 0xf);    // all '1'
    halAdiBusWrite(&hwp_rda2720mGlobal->slp_dcdc_lp_ctrl, 0x13);   // all '1'

    REG_RDA2720M_GLOBAL_SLP_CTRL_T slp_ctrl = {
        .b.ldo_xtl_en = 0,
        .b.slp_io_en = 1,
        .b.slp_ldo_pd_en = 1};
    halAdiBusWrite(&hwp_rda2720mGlobal->slp_ctrl, slp_ctrl.v);

    REG_RDA2720M_GLOBAL_SLP_DCDC_PD_CTRL_T slp_dcdc_pd_ctrl;
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_dcdc_pd_ctrl, slp_dcdc_pd_ctrl,
                    slp_dcdccore_drop_en, 1);

    REG_RDA2720M_GLOBAL_DCDC_CORE_SLP_CTRL0_T dcdc_core_slp_ctrl0 = {
        .b.dcdc_core_slp_step_delay = 3,
        .b.dcdc_core_slp_step_num = 4,
        .b.dcdc_core_slp_step_vol = 0x10,
        .b.dcdc_core_slp_step_en = 1};
    halAdiBusWrite(&hwp_rda2720mGlobal->dcdc_core_slp_ctrl0, dcdc_core_slp_ctrl0.v);

    REG_RDA2720M_GLOBAL_DCDC_CORE_SLP_CTRL1_T dcdc_core_slp_ctrl1 = {
        .b.dcdc_core_vosel_ds_sw = 0xe0};
    halAdiBusWrite(&hwp_rda2720mGlobal->dcdc_core_slp_ctrl1, dcdc_core_slp_ctrl1.v);

    REG_RDA2720M_GLOBAL_DCDC_VLG_SEL_T dcdc_vlg_sel;
    REG_ADI_CHANGE2(hwp_rda2720mGlobal->dcdc_vlg_sel, dcdc_vlg_sel,
                    dcdc_core_nor_sw_sel, 1,
                    dcdc_core_slp_sw_sel, 1);

    // reset pin enable
    REG_RDA2720M_GLOBAL_POR_7S_CTRL_T por_7s_ctrl;
    REG_ADI_CHANGE3(hwp_rda2720mGlobal->por_7s_ctrl, por_7s_ctrl,
                    ext_rstn_mode, 1, key2_7s_rst_en, 1,
                    pbint_7s_rst_disable, 1);

    // check PSM module for boot cause/mode
    halPmuCheckPsm();

    // check boot cause and mode
    halBootCauseMode();
}

void halPmuEnterPm1(void)
{
}

void halPmuAbortPm1(void)
{
}

void halPmuExitPm1(void)
{
}

void halPmuEnterPm2(void)
{
}

void halPmuAbortPm2(void)
{
}

void halPmuExitPm2(void)
{
}

void halPmuUnlockPowerReg(void)
{
    halAdiBusWrite(&hwp_rda2720mGlobal->pwr_wr_prot_value, PWR_WR_PROT_MAGIC);
    REG_WAIT_COND(halAdiBusRead(&hwp_rda2720mGlobal->pwr_wr_prot_value) == 0x8000);
}

void halPmuUnlockPsmReg(void)
{
    halAdiBusWrite(&hwp_rda2720mPsm->psm_reg_wr_protect, PSM_WR_PROT_MAGIC);
    REG_WAIT_COND(halAdiBusRead(&hwp_rda2720mPsm->psm_reg_wr_protect) == 1);
}

bool halPmuSwitchPower(uint32_t id, bool enabled, bool lp_enabled)
{
    REG_RDA2720M_GLOBAL_LDO_CAMD_REG0_T ldo_camd_reg0;
    REG_RDA2720M_GLOBAL_LDO_CAMA_REG0_T ldo_cama_reg0;
    REG_RDA2720M_GLOBAL_LDO_LCD_REG0_T ldo_lcd_reg0;
    REG_RDA2720M_GLOBAL_DCDC_WPA_REG2_T dcdc_wpa_reg2;
    REG_RDA2720M_GLOBAL_LDO_SPIMEM_REG0_T ldo_spimem_reg0;
    REG_RDA2720M_GLOBAL_LDO_MMC_PD_REG_T ldo_mmc_pd_reg;
    REG_RDA2720M_GLOBAL_LDO_SD_PD_REG_T ldo_sd_pd_reg; // actually is VIO18
    REG_RDA2720M_GLOBAL_LDO_CON_REG0_T ldo_con_reg0;
    REG_RDA2720M_GLOBAL_LDO_USB_PD_REG_T ldo_usb_pd_reg;
    REG_RDA2720M_GLOBAL_VIBR_CTRL0_T vibr_ctrl0;
    REG_RDA2720M_GLOBAL_KPLED_CTRL0_T kpled_ctrl0;
    REG_RDA2720M_GLOBAL_SLP_LDO_PD_CTRL0_T slp_ldo_pd_ctrl0;
    REG_RDA2720M_GLOBAL_SLP_LDO_PD_CTRL1_T slp_ldo_pd_ctrl1;
    REG_RDA2720M_BLTC_BLTC_CTRL_T bltc_ctrl;
    REG_RDA2720M_BLTC_BLTC_PD_CTRL_T bltc_pd_ctrl;
    REG_RDA2720M_GLOBAL_MODULE_EN0_T module_en0;
    REG_RDA2720M_BLTC_RG_RGB_V0_T rg_rgb_v0;
    REG_RDA2720M_BLTC_RG_RGB_V1_T rg_rgb_v1;

    switch (id)
    {
    case HAL_POWER_MEM:
        // shouldn't power down mem, in POWER_PD_SW
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl1, slp_ldo_pd_ctrl1,
                        slp_ldomem_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_VIO18:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_sd_pd_reg, ldo_sd_pd_reg,
                        ldo_vio18_pd, enabled ? 0 : 1);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldovio18_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_VDD28:
        // shouldn't power down VDD28, in POWER_PD_SW
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldovdd28_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_DCXO:
        // shouldn't power down DCXO, in POWER_PD_SW
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldodcxo_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_CAMD:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_camd_reg0, ldo_camd_reg0,
                        ldo_camd_pd, enabled ? 0 : 1);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldocamd_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_CAMA:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_cama_reg0, ldo_cama_reg0,
                        ldo_cama_pd, enabled ? 0 : 1);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldocama_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_LCD:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_lcd_reg0, ldo_lcd_reg0,
                        ldo_lcd_pd, enabled ? 0 : 1);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldolcd_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_WCN:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_con_reg0, ldo_con_reg0,
                        ldo_con_pd, enabled ? 0 : 1);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldolcd_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_USB:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_usb_pd_reg, ldo_usb_pd_reg,
                        ldo_usb33_pd, enabled ? 0 : 1);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldousb33_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_SD:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_mmc_pd_reg, ldo_mmc_pd_reg,
                        ldo_mmc_pd, enabled ? 0 : 1);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldommc_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_VIBR:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->vibr_ctrl0, vibr_ctrl0,
                        ldo_vibr_pd, enabled ? 0 : 1);
        // no setting for lp
        break;

    case HAL_POWER_KEYLED:
        REG_ADI_CHANGE2(hwp_rda2720mBltc->bltc_ctrl, bltc_ctrl,
                        b_sel, 1, b_sw, 1);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->kpled_ctrl0, kpled_ctrl0,
                        kpled_v, 0xb);

        REG_ADI_CHANGE1(hwp_rda2720mGlobal->kpled_ctrl0, kpled_ctrl0,
                        slp_ldokpled_pd_en, lp_enabled ? 0 : 1);

        REG_RDA2720M_GLOBAL_KPLED_CTRL1_T kpled_ctrl1 = {
            .b.ldo_kpled_pd = 1,
            .b.ldo_kpled_reftrim = 16,
        };
        halAdiBusWrite(&hwp_rda2720mGlobal->kpled_ctrl1, kpled_ctrl1.v);

        REG_ADI_CHANGE1(hwp_rda2720mGlobal->kpled_ctrl1, kpled_ctrl1,
                        ldo_kpled_pd, enabled ? 0 : 1);
        break;

    case HAL_POWER_BACK_LIGHT:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->module_en0, module_en0, bltc_en, 1);
        REG_ADI_CHANGE4(hwp_rda2720mBltc->bltc_ctrl, bltc_ctrl,
                        g_sel, 1, g_sw, 1,
                        r_sel, 1, r_sw, 1);
        REG_ADI_CHANGE2(hwp_rda2720mBltc->bltc_pd_ctrl, bltc_pd_ctrl,
                        hw_pd, 0,
                        sw_pd, enabled ? 0 : 1);
        REG_ADI_CHANGE1(hwp_rda2720mBltc->rg_rgb_v0, rg_rgb_v0, rg_rgb_v0, 0x20);
        REG_ADI_CHANGE1(hwp_rda2720mBltc->rg_rgb_v1, rg_rgb_v1, rg_rgb_v1, 0x20);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->module_en0, module_en0, bltc_en, enabled ? 1 : 0);
        break;

    case HAL_POWER_BUCK_PA:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->dcdc_wpa_reg2, dcdc_wpa_reg2,
                        pd_buck_vpa, enabled ? 0 : 1);
        // no setting for lp
        break;

    case HAL_POWER_SPIMEM:
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_spimem_reg0, ldo_spimem_reg0,
                        ldo_spimem_pd, enabled ? 0 : 1);
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                        slp_ldospimem_pd_en, lp_enabled ? 0 : 1);
        break;

    case HAL_POWER_ANALOG:
        // no setting for normal
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl1, slp_ldo_pd_ctrl1,
                        slp_ldoana_pd_en, lp_enabled ? 0 : 1);
        break;

    default:
        // ignore silently
        break;
    }

    return true;
}

void halPmuSet7sReset(bool enable)
{
    REG_RDA2720M_GLOBAL_POR_7S_CTRL_T por_7s_ctrl;

    if (enable)
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->por_7s_ctrl, por_7s_ctrl,
                        pbint_7s_rst_disable, 0);
    else
        REG_ADI_CHANGE1(hwp_rda2720mGlobal->por_7s_ctrl, por_7s_ctrl,
                        pbint_7s_rst_disable, 1);
}

bool halPmuSetPowerLevel(uint32_t id, uint32_t mv)
{
    //for CAM;

    bool lp_enabled = false; // lowpower disable

    REG_RDA2720M_GLOBAL_LDO_CAMD_REG0_T ldo_camd_reg0;
    REG_RDA2720M_GLOBAL_LDO_CAMA_REG0_T ldo_cama_reg0;
    REG_RDA2720M_GLOBAL_LDO_CAMA_REG1_T ldo_cama_reg1;
    REG_RDA2720M_GLOBAL_SLP_LDO_PD_CTRL0_T slp_ldo_pd_ctrl0;

    if (id == HAL_POWER_CAMD)
    {
        switch (mv)
        {
        case SENSOR_VDD_1800MV:
            REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_camd_reg0, ldo_camd_reg0,
                            ldo_camd_pd, true ? 0 : 1);
            REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                            slp_ldocamd_pd_en, lp_enabled ? 0 : 1);
            break;

        case SENSOR_VDD_CLOSED:
            REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_camd_reg0, ldo_camd_reg0,
                            ldo_camd_pd, false ? 0 : 1);
            REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                            slp_ldocamd_pd_en, lp_enabled ? 0 : 1);
            break;

        default:
            break;
        }
    }

    if (id == HAL_POWER_CAMA)
    {
        switch (mv)
        {
        case SENSOR_VDD_1800MV:
            REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_cama_reg0, ldo_cama_reg0,
                            ldo_cama_pd, true ? 0 : 1);
            REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                            slp_ldocama_pd_en, lp_enabled ? 0 : 1);
            break;

        case SENSOR_VDD_2800MV:
            REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_cama_reg0, ldo_cama_reg0,
                            ldo_cama_pd, true ? 0 : 1);

            ldo_cama_reg1.v = hwp_rda2720mGlobal->ldo_cama_reg1;
            ldo_cama_reg1.b.ldo_cama_v = 0b1011111;
            hwp_rda2720mGlobal->ldo_cama_reg1 = ldo_cama_reg1.v;

            halAdiBusWrite(&hwp_rda2720mGlobal->ldo_cama_reg1, ldo_cama_reg1.v);
            REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                            slp_ldocama_pd_en, lp_enabled ? 0 : 1);
            break;

        case SENSOR_VDD_CLOSED:
            REG_ADI_CHANGE1(hwp_rda2720mGlobal->ldo_cama_reg0, ldo_cama_reg0,
                            ldo_cama_pd, 1);

            REG_ADI_CHANGE1(hwp_rda2720mGlobal->slp_ldo_pd_ctrl0, slp_ldo_pd_ctrl0,
                            slp_ldocama_pd_en, lp_enabled ? 0 : 1);
            break;

        default:
            // ignore silently
            break;
        }
    };
    return false;
}

void halPmuPsmPrepare(void)
{
    REG_RDA2720M_GLOBAL_SOFT_RST0_T soft_rst0;
    REG_RDA2720M_GLOBAL_MODULE_EN0_T module_en0;
    REG_RDA2720M_PSM_PSM_CTRL_T psm_ctrl;
    REG_RDA2720M_PSM_PSM_RC_CLK_DIV_T psm_rc_clk_div;
    REG_RDA2720M_PSM_PSM_STATUS_T psm_status;
    REG_RDA2720M_PSM_PSM_32K_CAL_TH_T psm_32k_cal_th;
    REG_RDA2720M_PSM_RTC_PWR_OFF_TH1_T rtc_pwr_off_th1;
    REG_RDA2720M_PSM_RTC_PWR_OFF_TH2_T rtc_pwr_off_th2;
    REG_RDA2720M_PSM_RTC_PWR_OFF_TH3_T rtc_pwr_off_th3;
    REG_RDA2720M_PSM_RTC_PWR_ON_TH1_T rtc_pwr_on_th1;
    REG_RDA2720M_PSM_RTC_PWR_ON_TH2_T rtc_pwr_on_th2;
    REG_RDA2720M_PSM_RTC_PWR_ON_TH3_T rtc_pwr_on_th3;

    // psm pclk enable
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->module_en0, module_en0, psm_en, 1);

    // psm apb soft_reset
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->soft_rst0, soft_rst0, psm_soft_rst, 1);
    osiDelayUS(10);
    REG_ADI_CHANGE1(hwp_rda2720mGlobal->soft_rst0, soft_rst0, psm_soft_rst, 0);

    // open PSM protect register
    halAdiBusWrite(&hwp_rda2720mPsm->psm_reg_wr_protect, PSM_WR_PROT_MAGIC);

    // psm module soft_reset, auto clear
    REG_ADI_WRITE1(hwp_rda2720mPsm->psm_ctrl, psm_ctrl, psm_software_reset, 1);
    osiDelayUS(10);

    // open PSM protect register
    halAdiBusWrite(&hwp_rda2720mPsm->psm_reg_wr_protect, PSM_WR_PROT_MAGIC);

    // clear psm status
    REG_ADI_WRITE1(hwp_rda2720mPsm->psm_ctrl, psm_ctrl, psm_status_clr, 1);

    // configure psm clk divider
    REG_ADI_WRITE3(hwp_rda2720mPsm->psm_rc_clk_div, psm_rc_clk_div,
                   rc_32k_cal_cnt_p, 4,
                   clk_cal_64k_div_th, 7,
                   wdg_rst_clk_sel_en, 0);

    // enable psm cnt
    REG_ADI_CHANGE1(hwp_rda2720mPsm->psm_ctrl, psm_ctrl, psm_cnt_en, 1);
    osiDelayUS(10);

    // update cnt value enable
    REG_ADI_CHANGE1(hwp_rda2720mPsm->psm_ctrl, psm_ctrl, psm_cnt_update, 1);

    // check psm update is ok
    REG_ADI_WAIT_FIELD_NEZ(psm_status, hwp_rda2720mPsm->psm_status, psm_cnt_update_vld);

    // update cnt value disable
    REG_ADI_CHANGE1(hwp_rda2720mPsm->psm_ctrl, psm_ctrl, psm_cnt_update, 0);

    // configure psm cal N and pre_cal time
    REG_ADI_WRITE2(hwp_rda2720mPsm->psm_32k_cal_th, psm_32k_cal_th,
                   rc_32k_cal_cnt_n, 7,
                   rc_32k_cal_pre_th, 2);

    // configure psm cal dn filter
    halAdiBusWrite(&hwp_rda2720mPsm->psm_26m_cal_dn_th, 0);

    // configure psm cal up filter
    halAdiBusWrite(&hwp_rda2720mPsm->psm_26m_cal_up_th, 0xffff);

    // configure rtc power off clk_en ,hold,
    REG_ADI_WRITE2(hwp_rda2720mPsm->rtc_pwr_off_th1, rtc_pwr_off_th1,
                   rtc_pwr_off_clk_en_th, 0x10,
                   rtc_pwr_off_hold_th, 0x18);

    // configure rtc power off reset, pd
    REG_ADI_WRITE2(hwp_rda2720mPsm->rtc_pwr_off_th2, rtc_pwr_off_th2,
                   rtc_pwr_off_rstn_th, 0x20,
                   rtc_pwr_off_pd_th, 0x28);

    // configure rtc power off pd done,
    REG_ADI_WRITE1(hwp_rda2720mPsm->rtc_pwr_off_th3, rtc_pwr_off_th3,
                   rtc_pwr_off_done_th, 0x40);

    // configure rtc power on pd , reset,
    REG_ADI_WRITE2(hwp_rda2720mPsm->rtc_pwr_on_th1, rtc_pwr_on_th1,
                   rtc_pwr_on_pd_th, 0x04,
                   rtc_pwr_on_rstn_th, 0x08);

    // configure rtc power on clk_en, ,hold,
    REG_ADI_WRITE2(hwp_rda2720mPsm->rtc_pwr_on_th2, rtc_pwr_on_th2,
                   rtc_pwr_on_hold_th, 0x10,
                   rtc_pwr_on_clk_en_th, 0x06);

    // configure rtc power on pd done,timeout
    REG_ADI_WRITE2(hwp_rda2720mPsm->rtc_pwr_on_th3, rtc_pwr_on_th3,
                   rtc_pwr_on_done_th, 0x40,
                   rtc_pwr_on_timeout_th, 0x48);

    // configure psm cal interval
    halAdiBusWrite(&hwp_rda2720mPsm->psm_cnt_interval_th, 6);

    // configure psm cal phase, <=psm_cnt_interval_th
    halAdiBusWrite(&hwp_rda2720mPsm->psm_cnt_interval_phase, 6);
}

OSI_NO_RETURN void halShutdown(int mode, int64_t wake_uptime)
{
    OSI_LOGD(0, "psm: shutdown mode/0x%x wake/%u", mode, (unsigned)wake_uptime);

    if (mode == OSI_SHUTDOWN_PSM_SLEEP || mode == OSI_SHUTDOWN_POWER_OFF)
    {
        halAdiBusWrite(&PSM_MODE_REG, mode);
        bool pwrkey_en = false;
        bool wakeup_en = false;

        if (mode == OSI_SHUTDOWN_POWER_OFF)
        {
#ifdef CONFIG_PWRKEY_POWERUP
            pwrkey_en = true;
#else
            pwrkey_en = false;
#endif

#ifdef CONFIG_WAKEUP_PIN_POWERUP
            wakeup_en = true;
#endif
        }
        else if (mode == OSI_SHUTDOWN_PSM_SLEEP)
        {
            wakeup_en = true;
#ifdef CONFIG_PWRKEY_WAKEUP_PSM
            pwrkey_en = true;
#endif
        }

        // configure psm each function enable signal
        REG_RDA2720M_PSM_PSM_CTRL_T psm_ctrl = {
            .b.psm_en = 1,
            .b.rtc_pwr_on_timeout_en = 0,
            .b.ext_int_pwr_en = wakeup_en ? 1 : 0,
            .b.pbint1_pwr_en = pwrkey_en ? 1 : 0,
            .b.pbint2_pwr_en = 0,
#ifdef CONFIG_CHARGER_POWERUP
            .b.charger_pwr_en = 1,
#else
            .b.charger_pwr_en = 0,
#endif
            .b.psm_cnt_alarm_en = 0,
            .b.psm_cnt_alm_en = 0,
            .b.psm_software_reset = 0,
            .b.psm_cnt_update = 1,
            .b.psm_cnt_en = 1,
            .b.psm_status_clr = 0,
            .b.psm_cal_en = 1,
            .b.rtc_32k_clk_sel = 0, // 32k less
        };

        if (wake_uptime != INT64_MAX)
        {
            int64_t delta = (wake_uptime - osiUpTime()) / PSM_CNT_MS;
            uint32_t cnt = (delta < PSM_MIN_CNT) ? PSM_MIN_CNT : (uint32_t)delta;

            OSI_LOGD(0, "psm: sleep cnt/%u", cnt);

            // configure psm normal wakeup time
            halAdiBusWrite(&hwp_rda2720mPsm->psm_cnt_l_th, cnt & 0xffff);
            halAdiBusWrite(&hwp_rda2720mPsm->psm_cnt_h_th, cnt >> 16);

            psm_ctrl.b.psm_cnt_alm_en = 1;
        }
        halAdiBusWrite(&hwp_rda2720mPsm->psm_ctrl, psm_ctrl.v);

        REG_RDA2720M_PSM_PSM_STATUS_T psm_status;
        REG_ADI_WAIT_FIELD_NEZ(psm_status, hwp_rda2720mPsm->psm_status, psm_cnt_update_vld);

        // power off rtc
        halAdiBusWrite(&hwp_rda2720mGlobal->power_pd_hw, 1);
    }
    else if (mode == OSI_SHUTDOWN_FORCE_DOWNLOAD)
    {
        // boot_mode will be checked in ROM
        REG_SYS_CTRL_RESET_CAUSE_T reset_cause = {hwp_sysCtrl->reset_cause};
        reset_cause.b.boot_mode &= ~3;
        reset_cause.b.boot_mode |= 1; // force download, clear product test
        reset_cause.b.sw_boot_mode = 0;
        hwp_sysCtrl->reset_cause = reset_cause.v;

        REG_SYS_CTRL_APCPU_RST_SET_T apcpu_rst = {};
        apcpu_rst.b.set_global_soft_rst = 1;
        hwp_sysCtrl->apcpu_rst_set = apcpu_rst.v;

        OSI_DEAD_LOOP;
    }
    else
    {
        halAdiBusWrite(&PMU_BOOT_MODE_REG, mode);

        REG_RDA2720M_GLOBAL_SWRST_CTRL0_T swrst_ctrl0;
        swrst_ctrl0.b.reg_rst_en = 1;
        halAdiBusWrite(&hwp_rda2720mGlobal->swrst_ctrl0, swrst_ctrl0.v);

        REG_RDA2720M_GLOBAL_SOFT_RST_HW_T soft_rst_hw;
        soft_rst_hw.b.reg_soft_rst = 1;
        halAdiBusWrite(&hwp_rda2720mGlobal->soft_rst_hw, soft_rst_hw.v);
    }

    OSI_DEAD_LOOP;
}
