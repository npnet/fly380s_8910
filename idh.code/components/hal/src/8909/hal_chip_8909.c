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

OSI_SECTION_BOOT_TEXT uint32_t halGetChipId(void)
{
    REG_SYS_CTRL_CHIP_ID_T chip_id = {hwp_sysCtrl->chip_id};
    if (chip_id.b.major_id == 8909)
        return HAL_CHIP_ID_8909;

    osiPanic();
}

OSI_SECTION_BOOT_TEXT uint32_t halGetMetalId(void)
{
    REG_SYS_CTRL_CHIP_ID_T chip_id = {hwp_sysCtrl->chip_id};

    switch (chip_id.b.metal_id)
    {
    case 1:
        return HAL_METAL_ID_U01;
    case 3:
        return HAL_METAL_ID_U02;
    case 7:
        return HAL_METAL_ID_U03;
    case 0xf:
    default:
        return HAL_METAL_ID_U04;
    }
}

OSI_SECTION_BOOT_TEXT uint32_t halFreqToPllDivider(uint32_t freq)
{
#define DIVIDER_CHECK(divx2, n, v) \
    if ((divx2) <= (n))            \
        return (v);

    unsigned divx2 = (CONFIG_DEFAULT_MCUPLL_FREQ * 2 + freq / 2) / freq;
    DIVIDER_CHECK(divx2, HAL_CLK_DIV1P0, 15); // 312M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV1P5, 14); // 312M->208M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV2P0, 13); // 312M->156M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV2P5, 12); // 312M->125M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV3P0, 11); // 312M->104M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV3P5, 10); // 312M->89M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV4P0, 9);  // 312M->78M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV4P5, 8);  // 312M->69M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV5P0, 7);  // 312M->62M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV5P5, 6);  // 312M->57M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV6P0, 5);  // 312M->52M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV7P0, 4);  // 312M->45M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV8P0, 3);  // 312M->39M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV12P0, 2); // 312M->26M
    DIVIDER_CHECK(divx2, HAL_CLK_DIV16P0, 1); // 312M->20M
    return 0;                                 // 312M->13M; HAL_CLK_DIV24P0
}

OSI_SECTION_BOOT_TEXT uint32_t halFreqFromPllDivider(uint32_t divider)
{
    static const uint32_t freqs[] = {
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV24P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV16P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV12P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV8P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV7P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV6P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV5P5,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV5P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV4P5,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV4P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV3P5,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV3P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV2P5,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV2P0,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV1P5,
        (CONFIG_DEFAULT_MCUPLL_FREQ * 2) / HAL_CLK_DIV1P0,
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

    REG_SYS_CTRL_CLK_BB_ENABLE_T clk_bb_enable = {.b = {.enable_bcpu = 1}};
    REG_SYS_CTRL_BB_RST_CLR_T bb_rst_clr = {.b = {.clr_rst_bcpu = 1}};
    hwp_sysCtrl->clk_bb_enable = clk_bb_enable.v;
    hwp_sysCtrl->bb_rst_clr = bb_rst_clr.v;
    osiExitCritical(critical);
}

void halChangeSysClk(uint32_t freq, bool extram_access)
{
    uint32_t divider = halFreqToPllDivider(freq);
    uint32_t new_freq = halFreqFromPllDivider(divider);
    uint32_t old_freq = gSysClkFreq;

    if (new_freq > old_freq)
        osiInvokeSysClkChangeCallbacks(new_freq);

    hwp_sysCtrl->cfg_clk_sys = divider;
    gSysClkFreq = new_freq;

    if (new_freq < old_freq)
        osiInvokeSysClkChangeCallbacks(new_freq);
}

OSI_NO_MIPS16 OSI_SECTION_SRAM_BOOT_TEXT void osiDelayUS(uint32_t us)
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
    {
        uint32_t start = (uint32_t)address;
        boot_DcacheFlushAddrRange(start, start + size, false);
    }
}

void osiDCacheInvalidate(const void *address, size_t size)
{
    if (OSI_IS_KSEG0(address))
    {
        uint32_t start = (uint32_t)address;
        boot_DcacheFlushAddrRange(start, start + size, true);
    }
}

void osiDCacheCleanInvalidate(const void *address, size_t size)
{
    if (OSI_IS_KSEG0(address))
    {
        uint32_t start = (uint32_t)address;
        boot_DcacheFlushAddrRange(start, start + size, true);
    }
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
        uint32_t start = (uint32_t)address;
        boot_DcacheFlushAddrRange(start, start + size, false);
        boot_InvalidICache();
    }
}

OSI_SECTION_BOOT_TEXT void osiICacheSyncAll(void)
{
    boot_FlushDCache(true);
    boot_InvalidICache();
}

OSI_SECTION_SRAM_TEXT uint32_t osiEnterCritical(void)
{
    return hwp_sysIrq->sc;
}

OSI_SECTION_SRAM_TEXT void osiExitCritical(uint32_t critical)
{
    hwp_sysIrq->sc = critical;
    (void)hwp_sysIrq->nonmaskable;
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
