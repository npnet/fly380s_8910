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

#define ROM_VERSION_2 0x51612152

#define ISPI_CS_PMU (0)
#define ISPI_CS_ABB (1)

extern uint32_t gSysClkFreq;

OSI_SECTION_BOOT_TEXT uint32_t halGetChipId(void)
{
    REG_CFG_REGS_CHIP_ID_T chip_id = {hwp_configRegs->chip_id};
    if (chip_id.b.prod_id == 0x809 && chip_id.b.sub_id == 25)
        return HAL_CHIP_ID_8955;

    osiPanic();
}

OSI_SECTION_BOOT_TEXT uint32_t halGetMetalId(void)
{
    REG_CFG_REGS_CHIP_ID_T chip_id = {hwp_configRegs->chip_id};

    switch (chip_id.b.metal_id)
    {
    case 0:
        return HAL_METAL_ID_U01;
    case 1:
        return HAL_METAL_ID_U02;
    case 2:
    default:
        return HAL_METAL_ID_U03;
    }
}

OSI_SECTION_BOOT_TEXT uint32_t halFreqToPllDivider(uint32_t freq)
{
#define DIVIDER_CHECK(divx2, n, v) \
    if ((divx2) <= (n))            \
        return (v);

    unsigned divx2 = (CONFIG_SYSPLL_FREQ * 2 + freq / 2) / freq;
    DIVIDER_CHECK(divx2, HAL_CLK_DIV2P0, 13); // 624M->312M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV2P5, 12); // 624M->250M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV3P0, 11); // 624M->208M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV3P5, 10); // 624M->178M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV4P0, 9);  // 624M->156M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV4P5, 8);  // 624M->139M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV5P0, 7);  // 624M->125M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV5P5, 6);  // 624M->113M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV6P0, 5);  // 624M->104M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV7P0, 4);  // 624M->89M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV8P0, 3);  // 624M->78M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV12P0, 2); // 624M->52M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV16P0, 1); // 624M->39M
    return 0;                                 // 624M->26M; HAL_CLK_DIV24P0
}

OSI_SECTION_BOOT_TEXT uint32_t halFreqFromPllDivider(uint32_t divider)
{
    static const uint32_t freqs[] = {
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV24P0,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV16P0,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV12P0,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV8P0,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV7P0,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV6P0,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV5P5,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV5P0,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV4P5,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV4P0,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV3P5,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV3P0,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV2P5,
        (CONFIG_SYSPLL_FREQ * 2) / HAL_CLK_DIV2P0,
    };

    return (divider < OSI_ARRAY_SIZE(freqs)) ? freqs[divider] : HAL_FREQ_26M;
}

void hal_SysSetBBClock(uint32_t freq)
{
    hwp_sysCtrl->cfg_clk_bb = halFreqToPllDivider(freq);
}

void hal_SysStartBcpu(void *bcpu_main, void *stack_start_addr)
{
    uint32_t critical = osiEnterCritical();

    // These 2 symbols are defined in BCPU ROM.
    extern uint32_t bcpu_stack_base;
    extern uint32_t bcpu_main_entry;
    *(uint32_t *)OSI_KSEG1(&bcpu_stack_base) = (uint32_t)stack_start_addr;
    *(uint32_t *)OSI_KSEG1(&bcpu_main_entry) = (uint32_t)bcpu_main;

    REG_SYS_CTRL_CLK_BB_ENABLE_T clk_bb_enable = {.b = {.enable_bb_bcpu = 1}};
    REG_SYS_CTRL_BB_RST_CLR_T bb_rst_clr = {.b = {.clr_rst_bcpu = 1}};
    hwp_sysCtrl->clk_bb_enable = clk_bb_enable.v;
    hwp_sysCtrl->bb_rst_clr = bb_rst_clr.v;
    osiExitCritical(critical);
}

void halChangeSysClk(uint32_t freq, bool extram_access)
{
    // The minimal clk_sys is 52M. It is needed to turn off PLL for power saving.
    if (freq < HAL_FREQ_52M)
        freq = HAL_FREQ_52M;

    REG_SYS_CTRL_SEL_CLOCK_T sel_clock = {hwp_sysCtrl->sel_clock};
    uint32_t divider = halFreqToPllDivider(freq);
    uint32_t new_freq = halFreqFromPllDivider(divider);
    uint32_t old_freq = gSysClkFreq;
    bool pll_needed = extram_access ? true : false;
    bool pll_on = sel_clock.b.pll_locked ? true : false;

    // When PLL is off, 52MHz is the only choice
    if (!pll_needed)
        new_freq = HAL_FREQ_52M;

    if (new_freq == old_freq && pll_on == pll_needed)
        return;

    if (pll_on == pll_needed)
    {
        // change divider only
        if (new_freq > old_freq)
            osiInvokeSysClkChangeCallbacks(new_freq);

        hwp_sysCtrl->cfg_clk_sys = divider;
        gSysClkFreq = new_freq;

        if (new_freq < old_freq)
            osiInvokeSysClkChangeCallbacks(new_freq);
    }
    else if (pll_needed)
    {
        // old_freq must be 52MHz, new_freq >= old_freq

        // change divider to 52MHz to make sure clk_sys not change during PLL on
        hwp_sysCtrl->cfg_clk_sys = HAL_CLK_DIV12P0; // 52MHz
        if (!halSysPllPsramReconfig(true))
            osiPanic();

        if (new_freq > old_freq)
            osiInvokeSysClkChangeCallbacks(new_freq);

        hwp_sysCtrl->cfg_clk_sys = divider;
        gSysClkFreq = new_freq;
    }
    else
    {
        // new_freq must be 52MHz, new_freq <= old_freq
        if (halSysPllPsramReconfig(false))
        {
            // change divider to 52MHz to make sure clk_sys not change
            // even PLL is turned later
            hwp_sysCtrl->cfg_clk_sys = HAL_CLK_DIV12P0; // 52MHz
            gSysClkFreq = new_freq;

            if (new_freq < old_freq)
                osiInvokeSysClkChangeCallbacks(new_freq);
        }
    }
}

OSI_SECTION_BOOT_TEXT void halIspiInit(void)
{
    REG_SPI_CFG_T cfg;
    REG_FIELD_CHANGE2(hwp_spi3->cfg, cfg,
                      clock_divider, 1, // sys_clk / 4
                      clock_limiter, 0);
}

OSI_SECTION_SRAM_BOOT_TEXT static uint16_t halIspiRead(uint8_t cs, uint16_t address)
{
    uint32_t critical = osiEnterCritical();
    unsigned cmd = (1 << 31) | (cs << 29) | (1 << 25) | ((address & 0x1ff) << 16);

    // wait at least one tx space
    REG_SPI_STATUS_T status;
    REG_WAIT_FIELD_NEZ(status, hwp_spi3->status, tx_space);

    hwp_spi3->rxtx_buffer = cmd;

    REG_WAIT_FIELD_EQZ(status, hwp_spi3->status, active_status);
    REG_WAIT_FIELD_GT(status, hwp_spi3->status, rx_level, 0);

    uint16_t data = hwp_spi3->rxtx_buffer & 0xffff;

    osiExitCritical(critical);
    return data;
}

OSI_SECTION_SRAM_BOOT_TEXT static void halIspiWrite(uint8_t cs, uint16_t address, uint16_t data)
{
    uint32_t critical = osiEnterCritical();
    unsigned cmd = (cs << 29) | (0 << 25) | ((address & 0x1ff) << 16) | data;

    // wait at least one tx space
    REG_SPI_STATUS_T status;
    REG_WAIT_FIELD_NEZ(status, hwp_spi3->status, tx_space);

    hwp_spi3->rxtx_buffer = cmd;

    REG_WAIT_FIELD_EQZ(status, hwp_spi3->status, active_status);
    REG_WAIT_FIELD_EQ(status, hwp_spi3->status, tx_space, SPI_TX_FIFO_SIZE);
    osiExitCritical(critical);
}

OSI_SECTION_SRAM_BOOT_TEXT uint16_t halPmuRead(volatile uint32_t *reg)
{
    uint16_t address = (((uintptr_t)reg - (uintptr_t)hwp_pmu) & 0x7ff) >> 2;
    return halIspiRead(ISPI_CS_PMU, address);
}

OSI_SECTION_SRAM_BOOT_TEXT void halPmuWrite(volatile uint32_t *reg, uint16_t data)
{
    uint16_t address = (((uintptr_t)reg - (uintptr_t)hwp_pmu) & 0x7ff) >> 2;
    halIspiWrite(ISPI_CS_PMU, address, data);
}

OSI_SECTION_SRAM_BOOT_TEXT uint16_t halAbbRead(volatile uint32_t *reg)
{
    uint16_t address = (((uintptr_t)reg - (uintptr_t)hwp_abb) & 0x7ff) >> 2;
    return halIspiRead(ISPI_CS_ABB, address);
}

OSI_SECTION_SRAM_BOOT_TEXT void halAbbWrite(volatile uint32_t *reg, uint16_t data)
{
    uint16_t address = (((uintptr_t)reg - (uintptr_t)hwp_abb) & 0x7ff) >> 2;
    halIspiWrite(ISPI_CS_ABB, address, data);
}

void OSI_NO_MIPS16 OSI_SECTION_SRAM_BOOT_TEXT osiDelayUS(uint32_t us)
{
    // 4 cycles: 1 (addiu) + 3 (bnez)
    extern uint32_t gSysClkFreq;
    uint64_t mul = (gSysClkFreq * ((1ULL << 52) / (1000000 * 4))) >> 32;
    unsigned counter = (us * mul) >> 20;

    counter++;
    asm volatile("$L_check:\n"
                 "addiu %0, -1\n"
                 "bnez  %0, $L_check\n" ::"r"(counter));
}

void osiDCacheClean(const void *address, size_t size)
{
    if (OSI_IS_KSEG0(address))
        boot_FlushDCache(true);
}

void osiDCacheInvalidate(const void *address, size_t size)
{
    if (OSI_IS_KSEG0(address))
        boot_FlushDCache(true);
}

void osiDCacheCleanInvalidate(const void *address, size_t size)
{
    if (OSI_IS_KSEG0(address))
        boot_FlushDCache(true);
}

void osiDCacheCleanAll(void)
{
    boot_FlushDCache(true);
}

void osiDCacheInvalidateAll(void)
{
    boot_InvalidDCache();
}

void osiDCacheCleanInvalidateAll(void)
{
    boot_FlushDCache(true);
}

void osiICacheInvalidate(const void *address, size_t size)
{
    if (OSI_IS_KSEG0(address))
        boot_InvalidICache();
}

void osiICacheInvalidateAll(void)
{
    boot_InvalidICache();
}

void osiICaheSync(const void *address, size_t size)
{
    if (OSI_IS_KSEG0(address))
    {
        boot_FlushDCache(true);
        boot_InvalidICache();
    }
}

OSI_SECTION_BOOT_TEXT void osiICacheSyncAll(void)
{
    boot_FlushDCache(true);
    boot_InvalidICache();
}

uint32_t OSI_SECTION_SRAM_BOOT_TEXT osiEnterCritical(void)
{
    uint32_t sc = hwp_sysIrq->sc;
    return sc;
}

void OSI_SECTION_SRAM_BOOT_TEXT osiExitCritical(uint32_t critical)
{
    hwp_sysIrq->sc = critical;
}

uint32_t osiHWTick16K(void)
{
    return hwp_timer->hwtimer_curval;
}

OSI_STRONG_ALIAS(osiIrqSave, osiEnterCritical);
OSI_STRONG_ALIAS(osiIrqRestore, osiExitCritical);
OSI_STRONG_ALIAS(uxPortEnterCritical, osiEnterCritical);
OSI_STRONG_ALIAS(vPortExitCritical, osiExitCritical);
OSI_STRONG_ALIAS(uxPortSetInterruptMaskFromISR, osiEnterCritical);
OSI_STRONG_ALIAS(vPortClearInterruptMaskFromISR, osiExitCritical);
OSI_STRONG_ALIAS(vPortDisableInterrupts, osiEnterCritical);
