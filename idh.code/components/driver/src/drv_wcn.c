/* Copyright (C) 2019 RDA Technologies Limited and/or its affiliates("RDA").
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

#include "drv_wifi.h"
#include "hwregs.h"
#include "osi_api.h"
#include "osi_log.h"
#include "drv_names.h"
#include "hal_chip.h"
#include <stdlib.h>
#include "drv_wcn.h"

#define CP_PWRCTRL_BTFM_STABLE (1 << 1)

typedef struct
{
    osiPmSource_t *pm_source;
    uint32_t user_mask;
} wcnPriv_t;

struct drv_wcn
{
    wcnPriv_t *priv;
    drvWcnUser_t user;
};

static wcnPriv_t *gWcnPriv = NULL;

static inline void _wcnClkEnable()
{
    // enable 26M for wcn
    hwp_clkrst->clken_bb_sysctrl_set = 0x1;
    hwp_pwrctrl->btfm_pwr_ctrl = 0x2; // power on digital
    halClock26MRequest(CLK_26M_USER_WCN);

    // setting BT clk
    hwp_sysCtrl->cfg_pll_sys_ahb_btfm_div = 0x3; // 8 div 100MHZ
    REG_SYS_CTRL_CFG_PLL_SYS_AHB_BTFM_DIV_T ahb_btfm_div_cfg = {hwp_sysCtrl->cfg_pll_sys_ahb_btfm_div};
    ahb_btfm_div_cfg.b.cfg_pll_ahb_btfm_div_update = 1;
    hwp_sysCtrl->cfg_pll_sys_ahb_btfm_div = ahb_btfm_div_cfg.v;
}

static inline void _wcnClkDisable()
{
    halClock26MRelease(CLK_26M_USER_WCN);
}

static inline void _wcnPwrOn()
{
    while (!(hwp_pwrctrl->btfm_pwr_stat & CP_PWRCTRL_BTFM_STABLE))
        ;
    if (hwp_pwrctrl->btfm_pwr_stat != 0x3)
    {
        hwp_pwrctrl->btfm_pwr_ctrl = 0x2;
    }
    while (hwp_pwrctrl->btfm_pwr_stat != 0x3)
        ;

    // open BT PHY ldo
    halPmuSwitchPower(HAL_POWER_WCN, true, false);
    osiThreadSleep(30);
}

static inline void _wcnPwrOff()
{
    // power off digital
    hwp_pwrctrl->btfm_pwr_ctrl = 0x1;
    // close BT PHY ldo
    halPmuSwitchPower(HAL_POWER_WCN, false, false);
}

static inline void _wcnRfInit()
{
    hwp_wcnRfIf->sys_control = 0x261;
    hwp_wcnRfIf->bt_control = 0x27;
    hwp_wcnRfIf->inter_freq_setting = 0x8002BC;
    hwp_wcnRfIf->pga_setting0 = 0x1820;
    hwp_wcnRfIf->pga_setting1 = 0xEC222;
    hwp_wcnRfIf->mdll_setting = 0x61c8;

    hwp_wcnRfIf->power_dr = 0x40400;
    hwp_wcnRfIf->power_reg = 0x40400;

    hwp_wcnRfIf->lna_rmx_setting = 0x80C1D8;
    hwp_wcnRfIf->bt_dac_setting = 0x1208A;
    hwp_wcnRfIf->bt_txrf_setting = 0x1e000777;
    hwp_wcnRfIf->new_add = 0x23;
    hwp_wcnRfIf->vco_setting = 0x197881C;
    hwp_wcnRfIf->pll_setting0 = 0x22211000;
    hwp_wcnRfIf->pll_setting1 = 0x1555C81A;
    hwp_wcnRfIf->pll_setting2 = 0xD000028;
    hwp_wcnRfIf->pll_cal_setting0 = 0x5001cb;
    hwp_wcnRfIf->rxflt_cal_setting0 = 0x4B9027F;
    hwp_wcnRfIf->rxflt_cal_setting2 = 0x472b2b27;
    hwp_wcnRfIf->pll_sdm_setting0 = 0x144800;

    hwp_wcnModem->sw_swap_dccal = 0x5230;
    hwp_wcnRfIf->rxflt_setting = 0xb361112;
    hwp_wcnRfIf->pga_setting0 = 0x1820;
    hwp_wcnModem->sw_ctrl = 0x4170;
    hwp_wcnModem->tx_polar_mode_ctl = 0xb;
    hwp_wcnModem->tx_gain_ctrl = 0xA37E;
    hwp_wcnModem->dpsk_gfsk_tx_ctrl = 0xD048;

    hwp_wcnRfIf->bt_tx_gain_table_0 = 0x11C01195;
    hwp_wcnRfIf->bt_tx_gain_table_1 = 0x112E139D;
    hwp_wcnRfIf->bt_tx_gain_table_2 = 0x154C1700;
    hwp_wcnRfIf->bt_tx_gain_table_3 = 0x91E1D1E;

    hwp_wcnRfIf->ana_reserved = 0xF07900;
    hwp_wcnRfIf->dig_reserved = 0xD244;

    hwp_wcnRfIf->bt_gain_table_0 = 0x00000010;
    hwp_wcnRfIf->bt_gain_table_1 = 0x00000810;
    hwp_wcnRfIf->bt_gain_table_2 = 0x00001010;
    hwp_wcnRfIf->bt_gain_table_3 = 0x00001810;
    hwp_wcnRfIf->bt_gain_table_4 = 0x00001820;
    hwp_wcnRfIf->bt_gain_table_5 = 0x00001830;
    hwp_wcnRfIf->bt_gain_table_6 = 0x00001840;
    hwp_wcnRfIf->bt_gain_table_7 = 0x00001841;
    hwp_wcnRfIf->bt_gain_table_8 = 0x00005041;
    hwp_wcnRfIf->bt_gain_table_9 = 0x00005043;
    hwp_wcnRfIf->bt_gain_table_a = 0x0000D043;
    hwp_wcnRfIf->bt_gain_table_b = 0x0001D043;
    hwp_wcnRfIf->bt_gain_table_c = 0x0001D047;
    hwp_wcnRfIf->bt_gain_table_d = 0x0001F847;
    hwp_wcnRfIf->bt_gain_table_e = 0x0001F84B;
    hwp_wcnRfIf->bt_gain_table_f = 0x0001F84F;

    hwp_wcnBtReg->timgencntl = 0x81c200c8;
    hwp_wcnRfIf->fm_gain_table_7 = 0x2FFC2FFE;
    hwp_wcnRfIf->fm_gain_table_6 = 0x2FF42FF8;
    hwp_wcnRfIf->fm_gain_table_5 = 0x27E02FF0;
    hwp_wcnRfIf->fm_gain_table_4 = 0x27C027D0;
    hwp_wcnRfIf->fm_gain_table_3 = 0x15401DC0;
    hwp_wcnRfIf->fm_gain_table_2 = 0x12C01340;
    hwp_wcnRfIf->fm_gain_table_1 = 0x10401240;
    hwp_wcnRfIf->fm_gain_table_0 = 0x10201030;

    // wifi set AGC table
    hwp_wcnRfIf->wf_gain_table_f = 0x1F84F;
    hwp_wcnRfIf->wf_gain_table_e = 0x1F84D;
    hwp_wcnRfIf->wf_gain_table_d = 0x1F84C;
    hwp_wcnRfIf->wf_gain_table_c = 0x1F848;
    hwp_wcnRfIf->wf_gain_table_b = 0x1F844;
    hwp_wcnRfIf->wf_gain_table_a = 0x1F840;
    hwp_wcnRfIf->wf_gain_table_9 = 0x1F830;
    hwp_wcnRfIf->wf_gain_table_8 = 0x1F820;
    hwp_wcnRfIf->wf_gain_table_7 = 0x1F810;
    hwp_wcnRfIf->wf_gain_table_6 = 0x1F800;
    hwp_wcnRfIf->wf_gain_table_5 = 0x17000;
    hwp_wcnRfIf->wf_gain_table_4 = 0x14800;
    hwp_wcnRfIf->wf_gain_table_3 = 0x14000;
    hwp_wcnRfIf->wf_gain_table_2 = 0xA800;
    hwp_wcnRfIf->wf_gain_table_1 = 0xA000;
    hwp_wcnRfIf->wf_gain_table_0 = 0x8000;

    hwp_wcnRfIf->adc_setting0 = 0x620; // change adc_clk_edge to 0

    hwp_wcnWlan->phy_reg_offset_addr = 0xff;
    hwp_wcnWlan->phy_reg_write_data = 0x1;

    hwp_wcnWlan->phy_reg_offset_addr = 0x0;
    hwp_wcnWlan->phy_reg_write_data = 0xfb1f810;

    hwp_wcnWlan->phy_reg_offset_addr = 0x8;
    hwp_wcnWlan->phy_reg_write_data = 0x25211aa8;

    // Set the initial gain to F
    hwp_wcnWlan->phy_reg_offset_addr = 0x0C;
    hwp_wcnWlan->phy_reg_write_data = 0xF1585800;

    hwp_wcnRfIf->sys_control = 0x263;
    osiThreadSleep(5);
}

static void _wcnStart()
{
    _wcnClkEnable();
    _wcnPwrOn();
    _wcnRfInit();
}

static void _wcnStop()
{
    _wcnClkDisable();
    _wcnPwrOff();
}

static const osiPmSourceOps_t _wcnPmOps = {};

static wcnPriv_t *prvWcnInit()
{
    wcnPriv_t *p = (wcnPriv_t *)calloc(1, sizeof(wcnPriv_t));
    if (p == NULL)
        return NULL;
    p->pm_source = osiPmSourceCreate(DRV_NAME_WCN, &_wcnPmOps, p);
    if (p->pm_source == NULL)
    {
        free(p);
        return NULL;
    }
    _wcnStart();
    return p;
}

static void prvWcnExit(wcnPriv_t *p)
{
    _wcnStop();
    osiPmSourceDelete(p->pm_source);
    free(p);
}

drvWcn_t *drvWcnOpen(drvWcnUser_t user)
{
    if (gWcnPriv == NULL)
        gWcnPriv = prvWcnInit();

    if (gWcnPriv == NULL)
        return NULL;

    OSI_LOGD(0, "drv wcn gWcnPriv is %p", gWcnPriv);
    wcnPriv_t *p = gWcnPriv;
    if (p->user_mask & user)
    {
        OSI_LOGE(0, "wcn open %u already open", user);
        return NULL;
    }

    drvWcn_t *d = (drvWcn_t *)calloc(1, sizeof(drvWcn_t));
    if (d == NULL)
        return NULL;

    uint32_t critical = osiEnterCritical();
    p->user_mask |= user;
    d->priv = p;
    d->user = user;
    osiExitCritical(critical);
    return d;
}

void drvWcnClose(drvWcn_t *d)
{
    if (d == NULL || d->priv != gWcnPriv)
        return;

    wcnPriv_t *p = d->priv;
    if (!(p->user_mask & d->user))
        return;

    uint32_t critical = osiEnterCritical();
    p->user_mask &= ~(d->user);
    if (p->user_mask == 0)
    {
        gWcnPriv = NULL;
        osiExitCritical(critical);
        prvWcnExit(d->priv);
    }
    else
    {
        osiExitCritical(critical);
    }
    free(d);
}

bool drvWcnRequest(drvWcn_t *d)
{
    if (!d)
        return false;

    osiPmWakeLock(d->priv->pm_source);
    return true;
}

void drvWcnRelease(drvWcn_t *d)
{
    if (d)
        osiPmWakeUnlock(d->priv->pm_source);
}
