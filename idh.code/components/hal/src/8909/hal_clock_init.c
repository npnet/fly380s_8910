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
#include "osi_api.h"
#include "hwregs.h"

uint32_t gSysClkFreq OSI_SECTION_SRAM_UNINIT;

static inline uint32_t _pllDivider(uint32_t freq)
{
    return freq * (1ULL << 27) / 52000000ULL;
}

static inline void _pmuInit(void)
{
    REG_PMUC_LDO_BUCK1_CFG_5_T ldo_buck1 = {hwp_pmuc->ldo_buck1_cfg_5};
    REG_PMUC_LDO_BUCK1_CFG_6_T dcdc_buck1 = {hwp_pmuc->ldo_buck1_cfg_6};
    REG_PMUC_LDO_BUCK1_CFG_4_T buck1_cfg_4 = {hwp_pmuc->ldo_buck1_cfg_4};

    // Set LDO LP and NLP value
    ldo_buck1.b.vbuck1_ldo_bit_pm0 = CONFIG_VCORE_LDO_MEDIUM;
    ldo_buck1.b.vbuck1_ldo_bit_pm1 = CONFIG_VCORE_LDO_LP;
    hwp_pmuc->ldo_buck1_cfg_5 = ldo_buck1.v;

    // Set DCDC LP and NLP value
    dcdc_buck1.b.vbuck1_bit_pm0 = CONFIG_VCORE_DCDC_MEDIUM;
    dcdc_buck1.b.vbuck1_bit_pm1 = CONFIG_VCORE_DCDC_LP;
    hwp_pmuc->ldo_buck1_cfg_6 = dcdc_buck1.v;

    // set soft_en to 0
    REG_PMUC_LDO_BUCK1_CFG_3_T buck1_cfg_3 = {hwp_pmuc->ldo_buck1_cfg_3};
    buck1_cfg_3.b.soft_en_buck1 = 0;
    hwp_pmuc->ldo_buck1_cfg_3 = buck1_cfg_3.v;

    REG_PMUC_LDO_BUCK2_CFG_3_T buck2_cfg_3 = {hwp_pmuc->ldo_buck2_cfg_3};
    buck2_cfg_3.b.soft_en_buck2 = 0;
    hwp_pmuc->ldo_buck2_cfg_3 = buck2_cfg_3.v;

    osiDelayUS(100);

#ifdef CONFIG_DEFAULT_VCORE_DCDC_MODE
    buck1_cfg_4.b.buck1_dcdc_mode_en = 1;
#else
    buck1_cfg_4.b.buck1_dcdc_mode_en = 0;
#endif
    hwp_pmuc->ldo_buck1_cfg_4 = buck1_cfg_4.v;

    // Set buck2 to LDO or DCDC mode
    REG_PMUC_LDO_BUCK2_CFG_4_T buck2_cfg_4 = {hwp_pmuc->ldo_buck2_cfg_4};
#ifdef CONFIG_RF_LDO_MODE
    buck2_cfg_4.b.buck2_dcdc_mode_en = 0;
#else
    buck2_cfg_4.b.buck2_dcdc_mode_en = 1;
#endif
    hwp_pmuc->ldo_buck2_cfg_4 = buck2_cfg_4.v;

    // Set Vmem/Vspimem voltage in active & lp mode
    REG_PMUC_LDO_VMEM_CFG_1_T vmem_cfg_1 = {hwp_pmuc->ldo_vmem_cfg_1};
    REG_PMUC_LDO_VMEM_CFG_2_T vmem_cfg_2 = {hwp_pmuc->ldo_vmem_cfg_2};
    REG_PMUC_LDO_VSPIMEM_CFG_1_T vspimem_cfg_1 = {hwp_pmuc->ldo_vspimem_cfg_1};
    vmem_cfg_1.b.pu_vmem_pm0 = 1;
    vmem_cfg_1.b.pu_vmem_pm1 = 1;
    vmem_cfg_2.b.vmem_vbit_pm0 = 7; // 1.90V
    vmem_cfg_2.b.vmem_vbit_pm1 = 7;
    vmem_cfg_2.b.vspimem_vbit_pm1 = 8;
    vspimem_cfg_1.b.vspimem_vbit_pm0 = 8;
    hwp_pmuc->ldo_vmem_cfg_1 = vmem_cfg_1.v;
    hwp_pmuc->ldo_vmem_cfg_2 = vmem_cfg_2.v;
    hwp_pmuc->ldo_vspimem_cfg_1 = vspimem_cfg_1.v;

    // Set Vusb voltage in active
    REG_PMUC_LDO_VUSB_CFG_2_T vusb_cfg_2 = {hwp_pmuc->ldo_vusb_cfg_2};
    vusb_cfg_2.b.vusb_vbit_pm0 = 3;
    hwp_pmuc->ldo_vusb_cfg_2 = vusb_cfg_2.v;

    // Maybe wait a while (2ms) is helpfull
    osiDelayUS(2000);
}

static inline void _ddrInit(void)
{
    hwp_psram8Ctrl->cre = 1;

    REG_PSRAM8_CTRL_DELAY_TRAIN_T delay_train = {
        .b.training_en = 0,
        .b.delay_step = 1,
        .b.init_delay = 0,
        .b.init_trim = 2,
        .b.wait_dll_value = 0xf,
        .b.delay_threshold = 1,
        .b.delay_final_add = 0,
        .b.auto_cfg = 0,
    };
    hwp_psram8Ctrl->delay_train = delay_train.v;
    osiDelayUS(10);

    delay_train.b.training_en = 1;
    hwp_psram8Ctrl->delay_train = delay_train.v;
    osiDelayUS(10);

    REG_PSRAM8_CTRL_DLL_STATE_T dll_state;
    REG_WAIT_FIELD_NEZ(dll_state, hwp_psram8Ctrl->dll_state, dll_locked);
    osiDelayUS(10);

    REG_PSRAM8_CTRL_CTRL_TIME_T ctrl_time = {
        .b.r_tcph = 5,
        .b.w_tcph = 7,
        .b.wl = 0,
        .b.rl = 3,
        .b.rl_type = 0,
    };
    REG_PSRAM8_CTRL_DQS_CTRL_T dqs_ctrl = {
        .b.i_dqs_l_delay = dll_state.b.train_delay,
        .b.o_dqs_l_delay = dll_state.b.train_delay,
    };
    REG_PSRAM8_CTRL_CLK_CTRL_T clk_ctrl = {.b.o_clk_delay = dll_state.b.train_delay};

    delay_train.b.training_en = 0;
    hwp_psram8Ctrl->delay_train = delay_train.v;
    hwp_psram8Ctrl->ctrl_time = ctrl_time.v;
    hwp_psram8Ctrl->dqs_ctrl = dqs_ctrl.v;
    hwp_psram8Ctrl->clk_ctrl = clk_ctrl.v;
    osiDelayUS(10);

    REG_PSRAM8_CTRL_POWER_UP_T power_up = {.b = {.hw_power_pulse = 1}};
    hwp_psram8Ctrl->power_up = power_up.v;

    REG_WAIT_FIELD_NEZ(power_up, hwp_psram8Ctrl->power_up, init_done_state);
    osiDelayUS(10);

    hwp_psram8Ctrl->mr0 = 0xf;
    hwp_psram8Ctrl->mr4 = 0;
    osiDelayUS(10);

    hwp_psram8Ctrl->cre = 0;
}

static inline void _pllInit(void)
{
    // turn off MCU PLL
    REG_SYS_CTRL_PLL_CTRL_T pll_ctrl;
    REG_FIELD_CHANGE1(hwp_sysCtrl->pll_ctrl, pll_ctrl, mcu_pll_pd, 1);

    // set PLL divider
    unsigned pll_div = _pllDivider(CONFIG_DEFAULT_MCUPLL_FREQ);
    hwp_abb->mpll_sdm_freq_1 = pll_div >> 16;
    hwp_abb->mpll_sdm_freq_2 = pll_div & 0xffff;

    REG_ABB_MPLL_SDM_SETTING_T mpll_sdm_setting;
    REG_FIELD_CHANGE1(hwp_abb->mpll_sdm_setting, mpll_sdm_setting,
                      mpll_sdm_int_dec_sel, 3);

    osiDelayUS(10);

    // Enable pll
    pll_ctrl.v = 0;
    pll_ctrl.b.mcu_pll_pu = 1;
#ifdef CONFIG_GSM_SUPPORT
    pll_ctrl.b.gsm_pll_pu = 1;
    pll_ctrl.b.au_pll_pu = 1;
#endif
#ifdef CONFIG_LTE_NBIOT_SUPPORT
    pll_ctrl.b.nb_pll_pu = 1;
#endif
    hwp_sysCtrl->pll_ctrl = pll_ctrl.v;

    // Wait pll lock
    REG_WAIT_FIELD_NEZ(pll_ctrl, hwp_sysCtrl->pll_ctrl, mcu_pll_locked);
    osiDelayUS(10);

    // set divider and switch to fast clock
    hwp_sysCtrl->cfg_clk_sys = halFreqToPllDivider(CONFIG_DEFAULT_SYS_CLK_FREQ);
    hwp_sysCtrl->cfg_clk_bb = halFreqToPllDivider(CONFIG_DEFAULT_BB_CLK_FREQ);
    hwp_sysCtrl->cfg_clk_voc = halFreqToPllDivider(HAL_FREQ_52M);
    hwp_sysCtrl->cfg_clk_psram = halFreqToPllDivider(CONFIG_DEFAULT_MEM_CLK_FREQ);
    hwp_sysCtrl->cfg_clk_spiflash = halFreqToPllDivider(CONFIG_DEFAULT_SPIFLASH_INTF_FREQ * 2);

    // it is a simple divider, not mn divider
    hwp_sysCtrl->cfg_clk_rfdig = OSI_DIV_ROUND(CONFIG_DEFAULT_MCUPLL_FREQ, CONFIG_DEFAULT_RFDIG_CLK_FREQ);

    REG_SYS_CTRL_SEL_CLOCK_T sel_clock = {hwp_sysCtrl->sel_clock};
    sel_clock.b.sel_clk_sys = 0;
    sel_clock.b.sel_clk_bb = 0;
    sel_clock.b.sel_clk_psram = 0;
    sel_clock.b.sel_clk_gsm_tcu = 0;
    sel_clock.b.sel_clk_nb_tcu = 0;
    sel_clock.b.sel_clk_spiflash = 0;
    sel_clock.b.sel_clk_rf = 0;

    REG_SPI_FLASH_SPI_CONFIG_T spi_config = {
        .b.quad_mode = 1,
        .b.spi_wprotect_pin = 0,
        .b.spi_hold_pin = 0,
        .b.sample_delay = 2,
        .b.bypass_start_cmd = 1,
        .b.clk_divider = 2,
        .b.cmd_quad = 0,
        .b.tx_rx_size = 0,
    };

    gSysClkFreq = CONFIG_DEFAULT_SYS_CLK_FREQ;
    halApplyRegisters((uint32_t)&hwp_sysCtrl->sel_clock, sel_clock.v,
                      (uint32_t)&hwp_spiFlash->spi_config, spi_config.v,
                      REG_APPLY_END);

    REG_PMUC_XTAL_CTRL_1_T xtal_ctrl_1 = {hwp_pmuc->xtal_ctrl_1};
    xtal_ctrl_1.b.pu_32kxtal = 1;
    xtal_ctrl_1.b.pu_lpo32k = 1;    // NOTE: *do not* close PMUC_PU_LPO32K
    xtal_ctrl_1.b.xtal_reg_bit = 4; // normal xtal voltage level
    xtal_ctrl_1.b.pm1_en_xtal = 1;
    xtal_ctrl_1.b.pm3_en_xtal = 1;
    hwp_pmuc->xtal_ctrl_1 = xtal_ctrl_1.v;

    // Enable 6.5M PLL
    REG_PMUC_XTAL_CTRL_6_T xtal_ctrl_6 = {hwp_pmuc->xtal_ctrl_6};
    xtal_ctrl_6.b.enable_clk_6p5m = 1;
    hwp_pmuc->xtal_ctrl_6 = xtal_ctrl_6.v;
    osiDelayUS(50);

    // select 32K source to xtal26M
    REG_PMUC_XTAL_DIVIDER_CTRL_T xtal_divider_ctrl = {
        .b.step_offset_update = 0,
        .b.div_lp_mode_h_dr = 0,
        .b.div_lp_mode_h_reg = 0,
        .b.sel_32k_src = 2, // xtal26m 32k
    };
    hwp_pmuc->xtal_divider_ctrl = xtal_divider_ctrl.v;

    // Close rc 26m, it is not needed now
    REG_ABB_OSC_AND_DOUBLER_CFG_T osc_and_doubler_cfg = {hwp_abb->osc_and_doubler_cfg};
    osc_and_doubler_cfg.b.pu_osc26m = 0;
    hwp_abb->osc_and_doubler_cfg = osc_and_doubler_cfg.v;

    // Autodisable xcpu/bcpu cache rams when not access cache rams,
    // and disable when sleep
    REG_SYS_CTRL_MISC_CTRL_T misc_ctrl;
    REG_FIELD_CHANGE4(hwp_sysCtrl->misc_ctrl, misc_ctrl,
                      xcpu_cache_rams_disable, 1,
                      xcpu_autodisable_cache, 1,
                      bcpu_cache_rams_disable, 1,
                      bcpu_autodisable_cache, 1);

    // reg details
    // Bit3: tune phase
    // Bit5: enable switching in several frequency
    // Bit7: number of frequency (at most 4)
    // Bit8: set output clock polarity
    // Bit9: enable output clock
    // some test data:
    // 0x200 - 3.05M unchanged
    // 0x220 - 3.05M 2.89M switch time to time
    // 0x260 - 3.05M 2.88M 2.73M 2.6M switch time to time
    // 0x2A0 - 3.05M 3.25M switch time to time
    // 0x2E0 - 3.05M 3.25M 3.46M 3.71M switch time to time
    hwp_cauDig->cfg_sync_clk_buck1 = 0x200; // 3.05M clock
    hwp_cauDig->cfg_sync_clk_buck2 = 0x208; // 3.05M with 1/4 clock phase diff
    hwp_cauDig->cfg_chr_chopper_clk = 0;

#ifdef CONFIG_LTE_NBIOT_SUPPORT
    // it is the same as hal_NBTcuEarlyInit
    REG_TCU_CTRL_T nbtcu_ctrl = {
        .b.load_val = 0,
        .b.enable = 1,
        .b.load = 1,
        .b.nolatch = 0,
        .b.wakeup_en = 0,
    };
    REG_TCU_CFG_CLK_DIV_T nbtcu_cfg_clk_div = {
        .b.tcu_clk_same_sys = 0,
        .b.enable_dai_simple_208k = 0,
        .b.enable_qbit = 1,
    };
    hwp_nbTcu->ctrl = nbtcu_ctrl.v;
    hwp_nbTcu->wrap_val = 1919; //LPS_TCU_NB_DEFAULT_WRAP_COUNT;
    hwp_nbTcu->cfg_clk_div = nbtcu_cfg_clk_div.v;
#endif
}

OSI_SECTION_BOOT_TEXT void halClockInit(void)
{
    _pllInit();
    _pmuInit();
    _ddrInit();

    REG_SYS_CTRL_DBG_DISABLE_ACG6_T dbg_disable_acg6 = {
        .b.dbg_disable_pclk_mod_rf_if = 1,
    };
    REG_SYS_CTRL_DBG_DISABLE_ACG7_T dbg_disable_acg7 = {
        .b.dbg_disable_clk_rf_tx = 1,
        .b.dbg_disable_clk_rf_rx = 1,
    };

    hwp_sysCtrl->dbg_disable_acg6 = dbg_disable_acg6.v;
    hwp_sysCtrl->dbg_disable_acg7 = dbg_disable_acg7.v;
}

OSI_SECTION_BOOT_TEXT void halChipPreBoot(void)
{
    uint32_t chip_id = halGetChipId();
    uint32_t metal_id = halGetMetalId();
    osiDebugEvent(chip_id | metal_id);

    // This is only for 8909 U02 and later
    if (chip_id != HAL_CHIP_ID_8909 || HAL_METAL_BEFORE(metal_id, HAL_METAL_ID_U02))
        OSI_DEAD_LOOP;

    // unlock sys_ctrl, and won't lock it again
    hwp_sysCtrl->reg_dbg = SYS_CTRL_PROTECT_UNLOCK;

    gSysClkFreq = HAL_FREQ_52M;
    REG_SYS_CTRL_SEL_CLOCK_T sel_clock = {
        .b.sel_clk_slow = 1, // xtal52m
        .b.sel_clk_sys = 1,
        .b.sel_clk_bb = 1,
        .b.sel_clk_psram = 1,
        .b.sel_clk_spiflash = 1,
        .b.sel_clk_spiflash_ext = 1,
        .b.sel_clk_gsm_tcu = 1,
        .b.sel_clk_nb_tcu = 1,
        .b.sel_clk_voc = 1,
        .b.sel_clk_cam_out = 1,
        .b.sel_aif_stb = 1,
        .b.sel_clk_wcn = 1,
        .b.sel_clk_rf = 1,
        .b.sel_clk_uart1 = 1,
        .b.sel_clk_uart2 = 1,
        .b.sel_clk_uart3 = 1,
        .b.sel_clk_uart4 = 1,
        .b.sel_clk_dbg = 1,
        .b.sel_clk_usbc_backup = 0,
        .b.sel_clk_usbc = 0};
    REG_SPI_FLASH_SPI_CONFIG_T spi_config = {
        .b.quad_mode = 0, // can't enable quad before check QE
        .b.spi_wprotect_pin = 0,
        .b.spi_hold_pin = 0,
        .b.sample_delay = 2, // slow freq can be set to 1
        .b.bypass_start_cmd = 1,
        .b.clk_divider = 2,
        .b.cmd_quad = 0,
        .b.tx_rx_size = 0};

    halApplyRegisters((uint32_t)&hwp_sysCtrl->sel_clock, sel_clock.v,
                      (uint32_t)&hwp_spiFlash->spi_config, spi_config.v,
                      REG_APPLY_END);

    REG_PMUC_ABNORMAL_PWROFF_SRC_T abnormal_pwroff_src;
    REG_PMUC_ABNORMAL_PWROFF_STATUS_CLR_T abnormal_pwroff_status_clr;
    if (REG_FIELD_GET(hwp_pmuc->abnormal_pwroff_src, abnormal_pwroff_src, uvlo_occur))
    {
        // output event to indicate BOR happened
        osiDebugEvent(0xb0800001);
        REG_FIELD_WRITE1(hwp_pmuc->abnormal_pwroff_status_clr, abnormal_pwroff_status_clr,
                         uvlo_occur_clr, 1);
    }

    REG_PMUC_NAND_AND_UVLO_T nand_and_uvlo;
    REG_FIELD_CHANGE3(hwp_pmuc->nand_and_uvlo, nand_and_uvlo,
                      en_uvlo, 1,
                      uvlo_vth_bit, 0, // threshold 2.2V
                      uvlo_detect_bypass, 0);

    // make sure RC32K is power-up
    REG_PMUC_XTAL_CTRL_1_T xtal_ctrl_1 = {hwp_pmuc->xtal_ctrl_1};
    if (xtal_ctrl_1.b.pu_lpo32k != 1)
    {
        xtal_ctrl_1.b.pu_lpo32k = 1;
        hwp_pmuc->xtal_ctrl_1 = xtal_ctrl_1.v;
        osiDelayUS(100);
    }

    // select RC32K
    REG_PMUC_XTAL_DIVIDER_CTRL_T xtal_divider_ctrl = {hwp_pmuc->xtal_divider_ctrl};
    if (xtal_divider_ctrl.b.sel_32k_src != 0)
    {
        xtal_divider_ctrl.b.sel_32k_src = 0;
        hwp_pmuc->xtal_divider_ctrl = xtal_divider_ctrl.v;
    }

    REG_PMUC_PU_STATUS_T pu_status = {hwp_pmuc->pu_status};
    if (!pu_status.b.pu_done)
    {
        REG_PMUC_PMUC_PU_CTRL_T pu_ctrl = {hwp_pmuc->pmuc_pu_ctrl};
        pu_ctrl.b.reg_pu_done = 1;
        hwp_pmuc->pmuc_pu_ctrl = pu_ctrl.v;

        REG_PMUC_PMUC_INT_CLR_T int_clr = {hwp_pmuc->pmuc_int_clr};
        int_clr.b.pu_ready_clr = 1;
        int_clr.b.pu_done_clr = 1;
        hwp_pmuc->pmuc_int_clr = int_clr.v;

        REG_PMUC_PMUC_INT_MASK_T int_mask = {hwp_pmuc->pmuc_int_mask};
        int_mask.b.pu_ready_int_mask = 1;
        hwp_pmuc->pmuc_int_mask = int_mask.v;
        hwp_pmuc->wake_up_status_clr = hwp_pmuc->wake_up_status;
    }

    hwp_pmuc->power_mode_ctrl = 0;
}
