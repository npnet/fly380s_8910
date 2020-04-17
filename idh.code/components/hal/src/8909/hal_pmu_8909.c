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
#include "hal_iomux.h"
#include "osi_api.h"
#include "osi_clock.h"
#include "osi_log.h"

// VRF28
// -----
// vbat 4.2         3.4
// 000  2.487903    2.484896
// 001  2.553272    2.549976
// 010  2.627699    2.624197
// 011  2.708735    2.704917
// 100  2.802003    2.797995
// 101  2.904397    2.900255
// 110  3.024144    3.019778
// 111  3.157968    3.153172

#define VSEL_PAD 1  // 0: 3.3v, 1: 2.8v, 3: 1.8v
#define VSEL_VIB 1  // 0: 3.2v, 1: 2.8v
#define VSEL_SD 1   // 0: 3.3v, 1: 2.8v, 3: 1.8v
#define VSEL_LCD 2  // 0: 1.8v, 1: 2.0v, 2: 2.8v, 3: 3.0v
#define VSEL_CAM0 2 // 0: 1.8v, 1: 2.0v, 2: 2.8v, 3: 3.0v
#define VSEL_CAM1 2 // 0: 1.8v, 1: 2.0v, 2: 2.8v, 3: 3.0v
#define VSEL_RF28 0 // 2.5v

#define RFPOWER_GPIO 9
#define POWERON_IND_GPIO 8

#define PMUC_BOOT_MODE_REG (hwp_pmuc->pmu2_rsv_cfg)

bool halGpioSetOutput(uint32_t id, bool level)
{
    if (id >= NB_GPIO)
        return false;

    if (id < 32)
    {
        uint32_t mask = (1 << id);
        if (level)
            hwp_gpio->gpio_set_l = mask;
        else
            hwp_gpio->gpio_clr_l = mask;
        hwp_gpio->gpio_oen_set_out_l = mask;
    }
    else
    {
        uint32_t mask = (1 << (id - 32));
        if (level)
            hwp_gpio->gpio_set_h = mask;
        else
            hwp_gpio->gpio_clr_h = mask;
        hwp_gpio->gpio_oen_set_out_h = mask;
    }
    return true;
}

static void _setRfPower(bool enabled)
{
    if (RFPOWER_GPIO >= 0)
    {
        halIomuxRequest(PINFUNC_GPIO_0 + RFPOWER_GPIO);
        halGpioSetOutput(RFPOWER_GPIO, enabled);
    }
}

static void _setPoweronInd(bool on)
{
    if (POWERON_IND_GPIO >= 0)
    {
        halIomuxRequest(PINFUNC_GPIO_0 + POWERON_IND_GPIO);
        halGpioSetOutput(POWERON_IND_GPIO, on);
    }
}

void halBootCauseMode(void)
{
    uint16_t magic = PMUC_BOOT_MODE_REG & 0xffff;
    PMUC_BOOT_MODE_REG = 0;

    OSI_LOGI(0, "boot mode %04x", magic);
    if (magic != 0)
        osiSetBootMode(magic);
}

void halPmuInit(void)
{
    uint32_t metal = halGetMetalId();

    // Set Vpad(Vio) voltage
    REG_PMUC_LDO_VIO_CFG_1_T vio_cfg_1 = {hwp_pmuc->ldo_vio_cfg_1};
    vio_cfg_1.b.vio_vsel_pm0 = VSEL_PAD;
    vio_cfg_1.b.vio_vsel_pm1 = VSEL_PAD; // 1.8v
    hwp_pmuc->ldo_vio_cfg_1 = vio_cfg_1.v;

    // decrease power on time
    if (HAL_METAL_FROM(metal, HAL_METAL_ID_U03))
    {
        REG_PMUC_DELAY_TIME_CTRL_1_T delay_time_ctrl_1 = {hwp_pmuc->delay_time_ctrl_1};
        delay_time_ctrl_1.b.dbb_rst_rls_time_sel = 1;
        hwp_pmuc->delay_time_ctrl_1 = delay_time_ctrl_1.v;
    }

    REG_PMUC_LDO_VIB_CFG_1_T vib_cfg_1 = {hwp_pmuc->ldo_vib_cfg_1};
    vib_cfg_1.b.vib_vsel_pm0 = VSEL_VIB;
    vib_cfg_1.b.vib_vsel_pm1 = VSEL_VIB;
    hwp_pmuc->ldo_vib_cfg_1 = vib_cfg_1.v;

    REG_PMUC_LDO_VSDMC_CFG_1_T vsdmc_cfg_1 = {hwp_pmuc->ldo_vsdmc_cfg_1};
    vsdmc_cfg_1.b.vsdmc_vsel_pm0 = VSEL_SD;
    vsdmc_cfg_1.b.vsdmc_vsel_pm1 = VSEL_SD;
    hwp_pmuc->ldo_vsdmc_cfg_1 = vsdmc_cfg_1.v;

    REG_PMUC_LDO_VLCD_CFG_1_T vlcd_cfg_1 = {hwp_pmuc->ldo_vlcd_cfg_1};
    vlcd_cfg_1.b.vlcd_vsel_pm0 = VSEL_LCD;
    vlcd_cfg_1.b.vlcd_vsel_pm1 = VSEL_LCD;
    hwp_pmuc->ldo_vlcd_cfg_1 = vlcd_cfg_1.v;

    REG_PMUC_LDO_VCAM0_CFG_1_T vcam0_cfg_1 = {hwp_pmuc->ldo_vcam0_cfg_1};
    vcam0_cfg_1.b.vcam0_vsel_pm0 = VSEL_CAM0;
    vcam0_cfg_1.b.vcam0_vsel_pm1 = VSEL_CAM0;
    hwp_pmuc->ldo_vcam0_cfg_1 = vcam0_cfg_1.v;

    REG_PMUC_LDO_VCAM1_CFG_1_T vcam1_cfg_1 = {hwp_pmuc->ldo_vcam1_cfg_1};
    vcam1_cfg_1.b.vcam1_vsel_pm0 = VSEL_CAM1;
    vcam1_cfg_1.b.vcam1_vsel_pm1 = VSEL_CAM1;
    hwp_pmuc->ldo_vcam1_cfg_1 = vcam1_cfg_1.v;

    REG_PMUC_LDO_VRF28_CFG_1_T vrf28_cfg_1 = {hwp_pmuc->ldo_vrf28_cfg_1};
    vrf28_cfg_1.b.vrf28_vbit_pm0 = VSEL_RF28;
    vrf28_cfg_1.b.vrf28_vbit_pm1 = VSEL_RF28;
    hwp_pmuc->ldo_vrf28_cfg_1 = vrf28_cfg_1.v;

    // check and set vrtc voltage
    REG_PMUC_VRTC_ULP_CFG_T vrtc_ulp_cfg = {
        .b.ldo_ulp_vrtc_vbit_pm3 = 8,
        .b.ldo_ulp_vrtc_vbit_pm0 = 8,
        .b.ldo_ulp_vrtc_vbit_pm1 = 5,
    };
    if (hwp_pmuc->vrtc_ulp_cfg != vrtc_ulp_cfg.v)
        hwp_pmuc->vrtc_ulp_cfg = vrtc_ulp_cfg.v;

    REG_PMUC_XTAL_CTRL_6_T xtal_ctrl_6 = {hwp_pmuc->xtal_ctrl_6};
    xtal_ctrl_6.b.pu_xdrv = 0;
    hwp_pmuc->xtal_ctrl_6 = xtal_ctrl_6.v;

    REG_PMUC_LDO_VSIM0_CFG_1_T ldo_vsim0_cfg_1 = {hwp_pmuc->ldo_vsim0_cfg_1};
    ldo_vsim0_cfg_1.b.pu_vsim0_pm0 = 0;
    ldo_vsim0_cfg_1.b.pu_vsim0_pm1 = 0;
    hwp_pmuc->ldo_vsim0_cfg_1 = ldo_vsim0_cfg_1.v;

    REG_PMUC_LDO_VSIM1_CFG_1_T ldo_vsim1_cfg_1 = {hwp_pmuc->ldo_vsim1_cfg_1};
    ldo_vsim1_cfg_1.b.pu_vsim1_pm0 = 0;
    ldo_vsim1_cfg_1.b.pu_vsim1_pm1 = 0;
    hwp_pmuc->ldo_vsim1_cfg_1 = ldo_vsim1_cfg_1.v;

#ifdef CONFIG_CRYSTAL_WITHOUT_OSCILLATOR
    REG_PMUC_LDO_TCXO_CFG_1_T ldo_tcxo_cfg_1 = {hwp_pmuc->ldo_tcxo_cfg_1};
    ldo_tcxo_cfg_1.b.pu_vtcxo_pm0 = 0;
    ldo_tcxo_cfg_1.b.pu_vtcxo_pm1 = 0;
    ldo_tcxo_cfg_1.b.pu_vtcxo_pm3 = 0;
    hwp_pmuc->ldo_tcxo_cfg_1 = ldo_tcxo_cfg_1.v;
#endif

    _setRfPower(true);
    _setPoweronInd(true);

    // check boot cause and mode
    halBootCauseMode();
}

bool halPmuSetSimVolt(int idx, halSimVoltClass_t volt)
{
    if (idx >= CONFIG_HAL_SIM_COUNT || volt == HAL_SIM_VOLT_CLASS_A)
        return false;

    // Fields of REG_PMUC_LDO_VSIM0_CFG_1_T and REG_PMUC_LDO_VSIM1_CFG_1_T are the same
    volatile uint32_t *vsim_reg = (idx == 0) ? &hwp_pmuc->ldo_vsim0_cfg_1
                                             : &hwp_pmuc->ldo_vsim1_cfg_1;
    REG_PMUC_LDO_VSIM0_CFG_1_T vsim;
    if (volt == HAL_SIM_VOLT_OFF)
    {
        REG_FIELD_CHANGE2(*vsim_reg, vsim,
                          pu_vsim0_pm0, 0,
                          pu_vsim0_pm1, 0);
    }
    else
    {
        uint8_t val;
        // 0: 1.8v, 1: 2.0v, 2: 2.8v, 3: 3.3v
        if (volt == HAL_SIM_VOLT_CLASS_C)
            val = 0;
        else // HAL_SIM_VOLT_CLASS_B
            val = 2;

        REG_FIELD_CHANGE4(*vsim_reg, vsim,
                          vsim0_vsel_pm0, val,
                          vsim0_vsel_pm1, val,
                          pu_vsim0_pm0, 1,
                          pu_vsim0_pm1, 1);
    }

    return true;
}

bool halPmuSelectSim(int idx)
{
    if (idx >= CONFIG_HAL_SIM_COUNT)
        return false;

    // Three are independent SCI, nothing is needed.
    return true;
}

OSI_NO_RETURN void halShutdown(int mode, int64_t wake_uptime)
{
    if (mode == OSI_SHUTDOWN_POWER_OFF)
    {
    }
    else
    {
        PMUC_BOOT_MODE_REG = (mode & 0xffff);

        REG_SYS_CTRL_SYS_RST_SET0_T sys_rst_set0 = {.b.set_rst_global = 1};
        hwp_sysCtrl->sys_rst_set0 = sys_rst_set0.v;
    }
    OSI_DEAD_LOOP;
}
