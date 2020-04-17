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
#include "8955/hal_psram_cfg.h"
#include "hwregs.h"

#define CMDWORD_ADDR(n) (((n)&0x1FF) << 22)
#define CMDWORD_DATA(n) (((n)&0x3FFFF) << 4)
#define CMDWORD_READ (1 << 31)
#define CMDWORD_WRITE_FMT(addr, data) (CMDWORD_ADDR(addr) | CMDWORD_DATA(data))
#define CMDWORD_READ_FMT(addr) (CMDWORD_ADDR(addr) | CMDWORD_READ)

uint32_t gSysClkFreq OSI_SECTION_SRAM_UNINIT;

OSI_SECTION_BOOT_TEXT static void _xcvOpen(void)
{
    REG_RF_SPI_CTRL_T ctrl = {
        .b.enable = 1,
        .b.cs_polarity = 0,
        .b.digrf_read = 1,
        .b.clocked_back2back = 0,
        .b.input_mode = 0,
        .b.clock_polarity = 0,
        .b.clock_delay = 1,
        .b.do_delay = 2,
        .b.di_delay = 3,
        .b.cs_delay = 2,
        .b.cs_end_hold = 0,
        .b.cs_end_pulse = 3,
        .b.frame_size = 27,
        .b.input_frame_size = 17,
        .b.turnaround_time = 2,
    };
    hwp_rfSpi->ctrl = ctrl.v;
    hwp_rfSpi->command = 0;

    REG_RF_SPI_DIVIDER_T divider = {
        .b.divider = (gSysClkFreq + HAL_FREQ_13M) / (HAL_FREQ_13M * 2) - 1,
        .b.clock_limiter = 0,
    };
    hwp_rfSpi->divider = divider.v;
}

OSI_SECTION_BOOT_TEXT static void _xcvClose(void)
{
    REG_RF_SPI_COMMAND_T command;

    // Drive all the ouputs of the SPI interface to a logical '0'
    REG_FIELD_WRITE1(hwp_rfSpi->command, command, drive_zero, 1);

    // Disable the RF-SPI and clear the configuration
    hwp_rfSpi->ctrl = 0;
}

OSI_UNUSED OSI_SECTION_BOOT_TEXT static uint32_t _xcvRead(uint16_t address)
{
    REG_RF_SPI_COMMAND_T command;
    REG_RF_SPI_CMD_SIZE_T cmd_size;

    uint32_t cmd_word = CMDWORD_READ_FMT(address);

    uint32_t saved_ctrl = hwp_rfSpi->ctrl; // save the original configuration

    // There are hard-coded size: (frameSize=27, inputFrameSize=17)
    // * CmdSize = 10bits -> 2byte
    // * RdSize = 18bits -> 3bytes

    // Change the SPI mode to input and change the frame size
    REG_RF_SPI_CTRL_T ctrl;
    REG_FIELD_CHANGE2(hwp_rfSpi->ctrl, ctrl, frame_size, 9, input_mode, 1);

    // Fill the Tx fifo
    REG_FIELD_WRITE2(hwp_rfSpi->command, command, flush_cmd_fifo, 1, flush_rx_fifo, 1);
    hwp_rfSpi->cmd_data = (cmd_word >> 24) & 0xFF;
    hwp_rfSpi->cmd_data = (cmd_word >> 16) & 0xFF;

    // Set the readout size
    REG_FIELD_WRITE1(hwp_rfSpi->cmd_size, cmd_size, cmd_size, 2);

    // And send the command
    REG_FIELD_WRITE1(hwp_rfSpi->command, command, send_cmd, 1);

    // Wait for the SPI to finish
    REG_RF_SPI_STATUS_T status;
    REG_WAIT_FIELD_EQZ(status, hwp_rfSpi->status, active_status);

    // Wait until bytes ready, otherwise Rx FIFO will be underflow
    REG_WAIT_FIELD_GE(status, hwp_rfSpi->status, rx_level, 3);

    uint8_t data2 = (uint8_t)hwp_rfSpi->rx_data;
    uint8_t data1 = (uint8_t)hwp_rfSpi->rx_data;
    uint8_t data0 = (uint8_t)(hwp_rfSpi->rx_data << 6);

    // Change the SPI mode back to output
    hwp_rfSpi->ctrl = saved_ctrl;
    return ((data2 << 16) | (data1 << 8) | data0) >> 6;
}

OSI_SECTION_BOOT_TEXT static void _xcvWrite(uint16_t address, uint32_t data)
{
    REG_RF_SPI_COMMAND_T command;
    REG_RF_SPI_CMD_SIZE_T cmd_size;

    uint32_t cmd_word = CMDWORD_WRITE_FMT(address, data);

    // Flush the Tx fifo
    REG_FIELD_WRITE2(hwp_rfSpi->command, command, flush_cmd_fifo, 1, flush_rx_fifo, 1);
    hwp_rfSpi->cmd_data = (cmd_word >> 24) & 0xFF;
    hwp_rfSpi->cmd_data = (cmd_word >> 16) & 0xFF;
    hwp_rfSpi->cmd_data = (cmd_word >> 8) & 0xFF;
    hwp_rfSpi->cmd_data = (cmd_word >> 0) & 0xFF;

    // Set the cmd size
    REG_FIELD_WRITE1(hwp_rfSpi->cmd_size, cmd_size, cmd_size, 4);

    // And send the command
    REG_FIELD_WRITE1(hwp_rfSpi->command, command, send_cmd, 1);

    // Wait for the SPI to start - at least one byte has been sent
    REG_RF_SPI_STATUS_T status;
    REG_WAIT_FIELD_LT(status, hwp_rfSpi->status, cmd_data_level, 4);

    // Wait for the SPI to finish
    REG_WAIT_FIELD_EQZ(status, hwp_rfSpi->status, active_status);
}

OSI_SECTION_BOOT_TEXT static void _xcvConfigure(void)
{
    _xcvOpen();

    _xcvWrite(0x30, 0x00a00); // Soft Resetn down
    osiDelayUS(10000);        // Delay 10ms

#ifdef CHANNEL_FREQUENCY_MODE
    _xcvWrite(0x30, 0x00555); // Soft Resetn up
#else
    _xcvWrite(0x30, 0x00515); // Soft Resetn up
#endif

#ifdef USE_EXT_XTAL_32K       // EXT 32K enable
    _xcvWrite(0xc4, 0x163f0); // Pu_xtal_pmu=1
    osiDelayUS(100);          // Delay 100us
    _xcvWrite(0xc4, 0x063f0); // clk32K_lpo_en_pmu=0
    osiDelayUS(100);          // Delay 100us
    _xcvWrite(0xc4, 0x0e3f0); // clk32K_xtal_en_pmu=0
    osiDelayUS(100);          // Delay 100us
    _xcvWrite(0xc4, 0x0a3f0); // lpo_32K_pu=0
#else                         // 26M Div 32k enable
    // Direct-reg pu_xtal=1 before enabling 6p5m to avoid 26M clock burr
    _xcvWrite(0x20, 0x310a1);
    _xcvWrite(0xc1, 0x3e410); // Enable_clk_6p5m=1
    // Restore pu_xtal setting after enabling 6p5m
    _xcvWrite(0x20, 0x110a1);
    _xcvWrite(0xc4, 0x043f0); // clk32k_lpo_en_pmu=0
    _xcvWrite(0xc0, 0x8004);  // set osc low power v_bit to 4
    osiDelayUS(100);
    _xcvWrite(0xc4, 0x203f1); // clk32k_26mXtal_en_pmu=1 Xen_bt_enable=1
#endif

    _xcvWrite(0x101, 0x04926); // pll_cpaux_bit=7  0x01f26
    _xcvWrite(0x10d, 0x0a021);
    _xcvWrite(0x10e, 0x0a021);
    _xcvWrite(0x05, 0xa950); //0x2a950

    _xcvClose();
}

static inline void _pmuInit(void)
{
    _xcvConfigure();
    halIspiInit();

    // Reset the registers except some special regs
    REG_PMU_PMU_8809_30H_T r30h = {};
    r30h.b.soft_resetn = 1;
    halPmuWrite(&hwp_pmu->pmu_8809_30h, r30h.v);

    osiDelayUS(1000); // 1ms
    r30h.b.register_resetn = 1;
    halPmuWrite(&hwp_pmu->pmu_8809_30h, r30h.v);

    // Disable SDMMC LDO
    REG_PMU_LDO_SETTINGS_02H_T r02h = {halPmuRead(&hwp_pmu->ldo_settings_02h)};
    r02h.b.vmc_enable = 0;
    halPmuWrite(&hwp_pmu->ldo_settings_02h, r02h.v);

    // Set LDO LP and NLP value
    REG_PMU_PMU_8809_4BH_T r4bh = {
        .b.vbuck1_ldo_bit_nlp = CONFIG_VCORE_LDO_MEDIUM,
        .b.vbuck1_ldo_bit_lp = CONFIG_VCORE_LDO_LP,
    };
    halPmuWrite(&hwp_pmu->pmu_8809_4bh, r4bh.v);

    // Set DCDC LP and NLP value
    REG_PMU_PMU_8809_2FH_T r2fh = {
        .b.vbuck1_bit_lp = CONFIG_VCORE_DCDC_LP,
        .b.vbuck1_bit_nlp = CONFIG_VCORE_DCDC_MEDIUM,
    };
    halPmuWrite(&hwp_pmu->pmu_8809_2fh, r2fh.v);

    // Init DCDC frequency
    REG_PMU_PMU_8809_2DH_T r2dh = {
        .b.antiring_disable_buck1 = 0,
        .b.heavy_load_buck1 = 1,
        .b.discharge_en_buck1 = 1,
        .b.low_sense_buck1 = 1,
        .b.osc_freq_buck1 = 2,
        .b.pfm_buck1_threshold = 2,
        .b.counter_disable_buck1 = 0,
        .b.pfm_clk_disable_buck1_reg = 0,
        .b.pfm_clk_disable_buck1_dr = 0,
        .b.pfm_mode_clk_prd_buck1 = 2,
        .b.buck1_ldo_bit = 4, // ??
    };
    REG_PMU_PMU_8809_2EH_T r2eh = {
        .b.ir_disable_en_buck2 = 0,
        .b.heavy_load_buck2 = 1,
        .b.discharge_en_buck2 = 1,
        .b.low_sense_buck2 = 1,
        .b.osc_freq_buck2 = 2,
        .b.pfm_buck2_threshold = 2,
        .b.counter_disable_buck2 = 0,
        .b.pfm_clk_disable_buck2_reg = 0,
        .b.pfm_clk_disable_buck2_dr = 0,
        .b.pfm_mode_clk_prd_buck2 = 2,
        .b.buck2_ldo_bit = 0, // ??
    };
    halPmuWrite(&hwp_pmu->pmu_8809_2dh, r2dh.v);
    halPmuWrite(&hwp_pmu->pmu_8809_2eh, r2eh.v);

    REG_PMU_LDO_ACTIVE_SETTING_1_03H_T r03h = {
        .b.lp_mode_b_act = 1,
        .b.pd_spimem_act = 0,
        .b.pd_bl_led_act = 0,
        .b.pd_vmic_act = 1,
        .b.pd_vusb_act = 0, // vmem is derived from vusb
        .b.pd_vibr_act = 1,
        .b.pd_vmc_act = 1,
        .b.pd_vlcd_act = 1,
        .b.pd_vcam_act = 1,
        .b.pd_vasw_act = 1,
        .b.pd_va_act = 1,
        .b.pd_vio_act = 0,
        .b.pd_vm_act = 0,
#ifdef CONFIG_DEFAULT_VCORE_DCDC_MODE
        .b.pd_buck1_ldo_act = 0,
        .b.pfm_mode_sel_buck1_act = 0,
        .b.pu_buck1_act = 1,
#else
        .b.pd_buck1_ldo_act = 0,
        .b.pfm_mode_sel_buck1_act = 0,
        .b.pu_buck1_act = 0,
#endif
    };
    REG_PMU_LDO_LP_SETTING_1_08H_T r08h = {
        .b.lp_mode_b_lp = 0,
        .b.pd_spimem_lp = 0,
        .b.pd_bl_led_lp = 1,
        .b.pd_vmic_lp = 1,
        .b.pd_vusb_lp = 0, // vmem is derived from vusb
        .b.pd_vibr_lp = 1,
        .b.pd_vmc_lp = 1,
        .b.pd_vlcd_lp = 1,
        .b.pd_vcam_lp = 1,
        .b.pd_vasw_lp = 1,
        .b.pd_va_lp = 1,
        .b.pd_vio_lp = 0,
        .b.pd_vm_lp = 0,
#ifdef CONFIG_DEFAULT_VCORE_DCDC_MODE
        .b.pd_buck1_ldo_lp = 0,
        .b.pfm_mode_sel_buck1_lp = 1,
        .b.pu_buck1_lp = 1,
#else
        .b.pd_buck1_ldo_lp = 0,
        .b.pfm_mode_sel_buck1_lp = 0,
        .b.pu_buck1_lp = 0,
#endif
    };
    halPmuWrite(&hwp_pmu->ldo_active_setting_1_03h, r03h.v);
    halPmuWrite(&hwp_pmu->ldo_lp_setting_1_08h, r08h.v);

    // Set Vmem and VSPIMEM voltage in active & lp mode
    REG_PMU_LDO_ACTIVE_SETTING_3_05H_T r05h = {
        .b.vrtc_dcdc2_en_act = 0,
        .b.vrtc_dcdc1_en_act = 0,
        .b.vio_ibit_act = 4,
        .b.vmem_ibit_act = 7,
        .b.vspimem_ibit_act = 4,
        .b.vbackup_vbit_act = 4,
    };
    REG_PMU_LDO_LP_SETTING_3_0AH_T r0ah = {r05h.v};
    halPmuWrite(&hwp_pmu->ldo_active_setting_3_05h, r05h.v);
    halPmuWrite(&hwp_pmu->ldo_lp_setting_3_0ah, r0ah.v);

    REG_PMU_CALIBRATION_SETTING_2_17H_T r17h = {
        .b.pd_charge_ldo = 0,
        .b.ts_i_ctrl_battery = 8,
        .b.pu_ts_battery = 0,
    };
    halPmuWrite(&hwp_pmu->calibration_setting_2_17h, r17h.v);

    // Set PMU to active stat
    REG_PMU_REVISIONID_01H_T r01h = {.b.pd_mode_sel = 1};
    halPmuWrite(&hwp_pmu->revisionid_01h, r01h.v);

    // Maybe wait a while (2ms) is helpfull
    osiDelayUS(2000); // 2ms
}

OSI_SECTION_BOOT_TEXT const halPsramCfg_t *halGetPsramCfg(uint32_t vcore_mode, uint32_t vcore_level)
{
    return &gPsramCfg[vcore_mode][vcore_level];
}

static inline void _ddrInit(void)
{
#ifdef CONFIG_DEFAULT_VCORE_DCDC_MODE
    const halPsramCfg_t *cfg = halGetPsramCfg(HAL_VCORE_MODE_DCDC, HAL_VCORE_LEVEL_MEDIUM);
#else
    const halPsramCfg_t *cfg = halGetPsramCfg(HAL_VCORE_MODE_LDO, HAL_VCORE_LEVEL_MEDIUM);
#endif

    REG_PSRAM8_CTRL_POWER_UP_T power_up;
    REG_FIELD_WRITE1(hwp_psram8Ctrl->power_up, power_up, hw_power_pulse, 1);
    REG_WAIT_FIELD_NEZ(power_up, hwp_psram8Ctrl->power_up, init_done);

    hwp_psram8Ctrl->timeout_val = 0x7ffff;
    hwp_psram8Ctrl->clk_ctrl = cfg->ddr_clk_ctrl;
    hwp_psram8Ctrl->dqs_ctrl = cfg->ddr_dqs_ctrl;

    osiDelayUS(10);
    hwp_psram8Ctrl->cre = 1;
    *((volatile uint32_t *)0xa2000000) = cfg->mr0;
    *((volatile uint32_t *)0xa2000000) = cfg->mr0;
    hwp_psram8Ctrl->cre = 0;

    osiDelayUS(10);
    hwp_psram8Ctrl->cre = 1;
    *((volatile uint32_t *)0xa2000004) = cfg->mr4;
    *((volatile uint32_t *)0xa2000004) = cfg->mr4;
    hwp_psram8Ctrl->cre = 0;

    osiDelayUS(10);
    hwp_psram8Ctrl->cre = 1;
    (void)*((volatile uint32_t *)0xa2000000);
    (void)*((volatile uint32_t *)0xa2000004);
    hwp_psram8Ctrl->cre = 0;
}

static inline void _pllInit(void)
{
    REG_SYS_CTRL_CFG_XTAL_DIV_T cfg_xtal_div = {
        .b.cfg_xtal_div = 1, // 52M
        .b.cfg_xtal_div_update = 1,
    };
    REG_SYS_CTRL_SEL_CLOCK_T sel_clock = {
        .b.slow_sel_rf = 0,     // not use oscillator
        .b.sys_sel_fast = 1,    // slow
        .b.tcu_13m_sel = 0,     // 13M
        .b.pll_disable_lps = 0, // enable PLL on LPS
        .b.rf_detect_bypass = 0,
        .b.rf_detect_reset = 0,
        .b.spiflash_sel_fast = 1,
        .b.mem_bridge_sel_fast = 1,
        .b.bb_sel_fast = 1,
        .b.pll_bypass_lock = 0,
        .b.voc_sel_fast = 1,
    };
    REG_SPI_FLASH_SPI_CONFIG_T spi_config = {
        .b.quad_mode = 1,
        .b.spi_wprotect_pin = 0,
        .b.spi_hold_pin = 0,
        .b.sample_delay = 1,
        .b.bypass_start_cmd = 1,
        .b.clk_divider = 2,
        .b.cmd_quad = 0,
        .b.tx_rx_size = 0,
    };

    gSysClkFreq = HAL_FREQ_52M;
    halApplyRegisters((uint32_t)&hwp_sysCtrl->cfg_xtal_div, cfg_xtal_div.v,
                      (uint32_t)&hwp_sysCtrl->sel_clock, sel_clock.v,
                      (uint32_t)&hwp_spiFlash->spi_config, spi_config.v,
                      REG_APPLY_END);

    sel_clock.v = hwp_sysCtrl->sel_clock;
    REG_SYS_CTRL_PLL_CTRL_T pll_ctrl;
    if (sel_clock.b.pll_locked == 0)
    {
        // Turning off the Pll and reset of the Lock detector
        REG_FIELD_WRITE3(hwp_sysCtrl->pll_ctrl, pll_ctrl,
                         pll_enable, 0,
                         pll_lock_reset, 0, // 0 for reset
                         pll_clk_fast_enable, 0);

        osiDelayUS(10);

        // Turning on the Pll
        REG_FIELD_WRITE3(hwp_sysCtrl->pll_ctrl, pll_ctrl,
                         pll_enable, 1,
                         pll_lock_reset, 1, // 1 for not rest
                         pll_clk_fast_enable, 0);

        // Wait for lock.
        REG_WAIT_FIELD_NEZ(sel_clock, hwp_sysCtrl->sel_clock, pll_locked);

        osiDelayUS(10);

        // Enable PLL clock for fast clock
        REG_FIELD_CHANGE1(hwp_sysCtrl->pll_ctrl, pll_ctrl, pll_clk_fast_enable, 1);
    }

    hwp_sysCtrl->cfg_clk_sys = halFreqToPllDivider(CONFIG_DEFAULT_SYS_CLK_FREQ);
    hwp_sysCtrl->cfg_clk_mem_bridge = halFreqToPllDivider(CONFIG_MEM_CLK_HIGH_FREQ);
    hwp_sysCtrl->cfg_clk_spiflash = halFreqToPllDivider(CONFIG_DEFAULT_SPIFLASH_INTF_FREQ * 2);
    hwp_sysCtrl->cfg_clk_bb = halFreqToPllDivider(CONFIG_DEFAULT_BB_CLK_FREQ);
    hwp_sysCtrl->cfg_clk_voc = halFreqToPllDivider(HAL_FREQ_26M);

    sel_clock.b.sys_sel_fast = 0; // fast
    sel_clock.b.spiflash_sel_fast = 0;
    sel_clock.b.mem_bridge_sel_fast = 0;
    sel_clock.b.bb_sel_fast = 0;
    sel_clock.b.voc_sel_fast = 0;

    gSysClkFreq = CONFIG_DEFAULT_SYS_CLK_FREQ;
    halApplyRegisters((uint32_t)&hwp_sysCtrl->sel_clock, sel_clock.v,
                      REG_APPLY_END);

    REG_SYS_CTRL_CLK_SYS_MODE_T clk_sys_mode = {
        .b.mode_sys_mailbox = 1, //manual
        .b.mode_sys_gouda = 1,
        .b.mode_sysd_pwm = 1,
        .b.mode_sys_a2a = 1,
    };
    REG_SYS_CTRL_CLK_BB_MODE_T clk_bb_mode = {
        .b.mode_bb_mailbox = 1,
        .b.mode_bb_rom_ahb = 1,
        .b.mode_bb_com_regs = 1,
        .b.mode_bb_a2a = 1,
    };
    REG_SYS_CTRL_CLK_OTHER_DISABLE_T clk_other_disable = {
        .b.disable_oc_usbphy = 1,
    };
    REG_SYS_CTRL_CLK_PER_DISABLE_T clk_per_disable = {
        .b.disable_per_sdmmc = 1,
        .b.disable_per_sdmmc2 = 1,
    };
    hwp_sysCtrl->clk_sys_mode = clk_sys_mode.v;
    hwp_sysCtrl->clk_bb_mode = clk_bb_mode.v;
    hwp_sysCtrl->clk_other_mode = 0;
    hwp_sysCtrl->clk_other_disable = clk_other_disable.v;
    hwp_sysCtrl->clk_per_disable = clk_per_disable.v;
}

OSI_SECTION_BOOT_TEXT void halClockInit(void)
{
    _pllInit();
    _pmuInit();
    _ddrInit();
}

OSI_SECTION_BOOT_TEXT void halChipPreBoot(void)
{
    uint32_t chip_id = halGetChipId();
    uint32_t metal_id = halGetMetalId();
    osiDebugEvent(chip_id | metal_id);

    // This is only for 8955 U02 and later
    if (chip_id != HAL_CHIP_ID_8955 || HAL_METAL_BEFORE(metal_id, HAL_METAL_ID_U02))
        OSI_DEAD_LOOP;

    // unlock sys_ctrl, and won't lock it again
    hwp_sysCtrl->reg_dbg = SYS_CTRL_PROTECT_UNLOCK;

    // MAGIC: enable "r 0" in coolwatcher
    hwp_sysCtrl->cfg_rsd |= (1 << 3);

    // Set SDMMC pads to GPIO
    hwp_iomux->pad_sdmmc_clk_cfg = IOMUX_PAD_SDMMC_CLK_SEL_V_FUN_GPIO_8_SEL;
    hwp_iomux->pad_sdmmc_cmd_cfg = IOMUX_PAD_SDMMC_CMD_SEL_V_FUN_GPIO_9_SEL;
    hwp_iomux->pad_sdmmc_data_0_cfg = IOMUX_PAD_SDMMC_DATA_0_SEL_V_FUN_GPIO_10_SEL;
    hwp_iomux->pad_sdmmc_data_1_cfg = IOMUX_PAD_SDMMC_DATA_1_SEL_V_FUN_GPIO_11_SEL;
    hwp_iomux->pad_sdmmc_data_2_cfg = IOMUX_PAD_SDMMC_DATA_2_SEL_V_FUN_GPIO_12_SEL;
    hwp_iomux->pad_sdmmc_data_3_cfg = IOMUX_PAD_SDMMC_DATA_3_SEL_V_FUN_GPIO_13_SEL;
}
