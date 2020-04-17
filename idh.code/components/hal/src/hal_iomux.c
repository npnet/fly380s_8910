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

#include "hal_iomux.h"
#include "drv_names.h"
#include "hwregs.h"
#include "osi_api.h"
#include <stdarg.h>
#include <assert.h>

typedef struct
{
    volatile uint32_t *reg;
    unsigned func;
} halIomuxConfig_t;

#if defined(CONFIG_SOC_8910) || defined(CONFIG_SOC_8909) || defined(CONFIG_SOC_8955)
#include "hal_iomux_pincfg.h"
#endif

#if defined(CONFIG_SOC_8910)
#define IOMUX_REG_COUNT (0x178 / 4)
#elif defined(CONFIG_SOC_8909)
#define IOMUX_REG_COUNT (284 / 4)
#elif defined(CONFIG_SOC_8955)
#define IOMUX_REG_COUNT (224 / 4)
#endif

typedef struct
{
    uint32_t suspend_regs[IOMUX_REG_COUNT];
} halIomuxContext_t;

static halIomuxContext_t gHalIomuxCtx;

static_assert(IOMUX_REG_COUNT * 4 == sizeof(*hwp_iomux), "IOMUX register count error");

static void _iomuxSuspend(void *ctx, osiSuspendMode_t mode)
{
    if (mode == OSI_SUSPEND_PM2)
    {
        halIomuxContext_t *d = &gHalIomuxCtx;
        volatile uint32_t *reg = (volatile uint32_t *)hwp_iomux;
        for (int n = 0; n < IOMUX_REG_COUNT; n++)
            d->suspend_regs[n] = *reg++;
    }
}

static void _iomuxResume(void *ctx, osiSuspendMode_t mode, uint32_t source)
{
    if (source & OSI_RESUME_ABORT)
        return;

    if (mode == OSI_SUSPEND_PM2)
    {
        halIomuxContext_t *d = &gHalIomuxCtx;
        volatile uint32_t *reg = (volatile uint32_t *)hwp_iomux;
        for (int n = 0; n < IOMUX_REG_COUNT; n++)
            *reg++ = d->suspend_regs[n];
    }
}

static const osiPmSourceOps_t _iomuxPmOps = {
    .suspend = _iomuxSuspend,
    .resume = _iomuxResume,
};

void halIomuxInit(void)
{
#ifdef CONFIG_SOC_8910
    REG_IOMUX_PAD_SIM_1_CLK_CFG_REG_T sim1 = {0};
    sim1.b.pad_sim_1_clk_pull_frc = 1;
    sim1.b.pad_sim_1_clk_pull_dn = 1;
    sim1.b.pad_sim_1_clk_pull_up = 1;
    uint32_t mask = sim1.v;

    sim1.b.pad_sim_1_clk_pull_dn = 0;
    sim1.b.pad_sim_1_clk_pull_up = 0;
    uint32_t value = sim1.v;

    volatile REG32 *change_list[] = {
        &hwp_iomux->pad_sim_1_clk_cfg_reg,
        &hwp_iomux->pad_sim_2_clk_cfg_reg,
        &hwp_iomux->pad_ap_jtag_tdo_cfg_reg,
        &hwp_iomux->pad_keyout_0_cfg_reg,
        &hwp_iomux->pad_keyout_1_cfg_reg,
        &hwp_iomux->pad_keyout_2_cfg_reg,
        &hwp_iomux->pad_keyout_3_cfg_reg,
        &hwp_iomux->pad_keyout_4_cfg_reg,
        &hwp_iomux->pad_keyout_5_cfg_reg,
    };

    for (uint8_t i = 0; i < OSI_ARRAY_SIZE(change_list); ++i)
    {
        uint32_t reg = *change_list[i];
        reg &= ~mask;
        reg |= value;
        *change_list[i] = reg;
    }

    REG_ANALOG_REG_PAD_SIM_1_CFG_T pad_sim1_cfg = {hwp_analogReg->pad_sim_1_cfg};
    pad_sim1_cfg.b.pad_sim_1_dio_spu = 1;
    hwp_analogReg->pad_sim_1_cfg = pad_sim1_cfg.v;

    REG_ANALOG_REG_PAD_SIM_2_CFG_T pad_sim2_cfg = {hwp_analogReg->pad_sim_2_cfg};
    pad_sim2_cfg.b.pad_sim_2_dio_spu = 1;
    hwp_analogReg->pad_sim_2_cfg = pad_sim2_cfg.v;

    hwp_iomux->pad_ap_jtag_tck_cfg_reg = 0x10200;
    hwp_iomux->pad_ap_jtag_trst_cfg_reg = 0x10200;
    hwp_iomux->pad_ap_jtag_tms_cfg_reg = 0x10200;
    hwp_iomux->pad_ap_jtag_tdi_cfg_reg = 0x10200;
    hwp_iomux->pad_sim_1_dio_cfg_reg = 0x10000;
    hwp_iomux->pad_sim_2_dio_cfg_reg = 0x10000;

#ifdef CONFIG_TST_H_GROUND
    REG_IOMUX_PAD_KEYIN_2_CFG_REG_T keyin2 = {};
    keyin2.b.pad_keyin_2_pull_up = 1;
    keyin2.b.pad_keyin_2_pull_frc = 1;
    hwp_iomux->pad_keyin_2_cfg_reg = keyin2.v;
#endif

#if defined(CONFIG_BOARD_SUPPORT_SIM1_DETECT) && (CONFIG_BOARD_SIM1_DETECT_GPIO == 4)
    REG_IOMUX_PAD_GPIO_4_CFG_REG_T gpio_4 = {};
    gpio_4.b.pad_gpio_4_pull_frc = 1;
#ifdef CONFIG_GPIO4_FORCE_PULL_UP
    gpio_4.b.pad_gpio_4_pull_up = 1;
#endif
    hwp_iomux->pad_gpio_4_cfg_reg = gpio_4.v;
#endif

#endif
    (void)osiPmSourceCreate(DRV_NAME_IOMUX, &_iomuxPmOps, NULL);
}

void halIomuxRequest(unsigned pinfunc)
{
    unsigned count = sizeof(gHalIomuxConfig) / sizeof(gHalIomuxConfig[0]);
    if (pinfunc >= count)
        return;

    const halIomuxConfig_t *cfg = &gHalIomuxConfig[pinfunc];
    if (cfg->reg != NULL) // NULL is for non-configurable
        *(cfg->reg) = (*(cfg->reg) & ~0xf) | cfg->func;
}

void halIomuxRequestBatch(unsigned pinfunc, ...)
{
    va_list ap;
    va_start(ap, pinfunc);

    while (pinfunc != HAL_IOMUX_REQUEST_END)
    {
        halIomuxRequest(pinfunc);
        pinfunc = va_arg(ap, unsigned);
    }
    va_end(ap);
}

void halIomuxRelease(unsigned pinfunc)
{
    unsigned count = sizeof(gHalIomuxConfig) / sizeof(gHalIomuxConfig[0]);
    if (pinfunc >= count)
        return;

    const halIomuxConfig_t *cfg = &gHalIomuxConfig[pinfunc];
    if (cfg->reg != NULL)
    {
        if ((*(cfg->reg) & 0xf) == cfg->func)
            *(cfg->reg) = (*(cfg->reg) & ~0xf);
    }
}

void halIomuxReleaseBatch(unsigned pinfunc, ...)
{
    va_list ap;
    va_start(ap, pinfunc);
    while (pinfunc != HAL_IOMUX_REQUEST_END)
    {
        halIomuxRelease(pinfunc);
        pinfunc = va_arg(ap, unsigned);
    }
    va_end(ap);
}
