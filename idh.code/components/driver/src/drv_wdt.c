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

#include "osi_log.h"
#include "drv_wdt.h"
#include "hal_adi_bus.h"
#include "hwregs.h"

#define AON_WDT_HZ (32768)
#define WDT_RESTET_MIN (10)

bool drvWatchdogStart(uint32_t reset_ms)
{
    if (reset_ms < WDT_RESTET_MIN)
        return false;

    uint32_t reload = (reset_ms * (uint64_t)AON_WDT_HZ) / 1000;

    REG_RDA2720M_GLOBAL_MODULE_EN0_T module_en0;
    REG_RDA2720M_GLOBAL_RTC_CLK_EN0_T rtc_clk_en0;
    REG_RDA2720M_WDG_WDG_CTRL_T wdg_ctrl;
    halAdiBusBatchChange(
        &hwp_rda2720mWdg->wdg_ctrl,
        HAL_ADI_BUS_OVERWITE(0), // wdg_run
        &hwp_rda2720mGlobal->module_en0,
        REG_FIELD_MASKVAL1(module_en0, wdg_en, 1),
        &hwp_rda2720mGlobal->rtc_clk_en0,
        REG_FIELD_MASKVAL1(rtc_clk_en0, rtc_wdg_en, 1),
        &hwp_rda2720mWdg->wdg_load_high,
        HAL_ADI_BUS_OVERWITE(reload >> 16),
        &hwp_rda2720mWdg->wdg_load_low,
        HAL_ADI_BUS_OVERWITE(reload & 0xffff),
        &hwp_rda2720mWdg->wdg_ctrl,
        REG_FIELD_MASKVAL2(wdg_ctrl, wdg_run, 1, wdg_rst_en, 1),
        HAL_ADI_BUS_CHANGE_END);

    return true;
}

bool drvWatchdogReload(uint32_t reset_ms)
{
    if (reset_ms < WDT_RESTET_MIN)
        return false;

    uint32_t reload = (reset_ms * (uint64_t)AON_WDT_HZ) / 1000;
    halAdiBusBatchChange(
        &hwp_rda2720mWdg->wdg_load_high,
        HAL_ADI_BUS_OVERWITE(reload >> 16),
        &hwp_rda2720mWdg->wdg_load_low,
        HAL_ADI_BUS_OVERWITE(reload & 0xffff),
        HAL_ADI_BUS_CHANGE_END);

    return true;
}

void drvWatchdogStop(void)
{
    REG_RDA2720M_GLOBAL_MODULE_EN0_T module_en0;
    REG_RDA2720M_GLOBAL_RTC_CLK_EN0_T rtc_clk_en0;
    halAdiBusBatchChange(
        &hwp_rda2720mWdg->wdg_ctrl,
        HAL_ADI_BUS_OVERWITE(0), // wdg_run
        &hwp_rda2720mGlobal->module_en0,
        REG_FIELD_MASKVAL1(module_en0, wdg_en, 0),
        &hwp_rda2720mGlobal->rtc_clk_en0,
        REG_FIELD_MASKVAL1(rtc_clk_en0, rtc_wdg_en, 0),
        HAL_ADI_BUS_CHANGE_END);
}
