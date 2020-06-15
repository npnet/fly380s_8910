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

#include "hal_spi_flash.h"
#include "hal_chip.h"
#include "hwregs.h"
#include "osi_api.h"
#include "osi_profile.h"
#include "osi_byte_buf.h"
#include "hal_spi_flash_defs.h"

#define FLASHRAM_CODE OSI_SECTION_LINE(.ramtext.flashhal)
#define FLASHRAM_API FLASHRAM_CODE OSI_NO_INLINE
#include "hal_spi_flash_internal.h"

#define SECTOR_COUNT_4K (SIZE_4K / SIZE_4K)
#define SECTOR_COUNT_8K (SIZE_8K / SIZE_4K)
#define SECTOR_COUNT_16K (SIZE_16K / SIZE_4K)
#define SECTOR_COUNT_32K (SIZE_32K / SIZE_4K)
#define SECTOR_COUNT_1M ((1 << 20) / SIZE_4K)
#define SECTOR_COUNT_2M ((2 << 20) / SIZE_4K)
#define SECTOR_COUNT_4M ((4 << 20) / SIZE_4K)
#define SECTOR_COUNT_8M ((8 << 20) / SIZE_4K)
#define SECTOR_COUNT_16M ((16 << 20) / SIZE_4K)

typedef struct
{
    uint16_t offset;
    uint16_t wp;
} halSpiFlashWpMap_t;

/**
 * Table for GD 1MB
 */
FLASHRAM_CODE static const halSpiFlashWpMap_t gGD8MWpMap[] = {
    {SECTOR_COUNT_1M, WP_GD8M_ALL},
    {SECTOR_COUNT_1M - SECTOR_COUNT_1M / 16, WP_GD8M_15_16},
    {SECTOR_COUNT_1M - SECTOR_COUNT_1M / 8, WP_GD8M_7_8},
    {SECTOR_COUNT_1M - SECTOR_COUNT_1M / 4, WP_GD8M_3_4},
    {SECTOR_COUNT_1M / 2, WP_GD8M_1_2},
    {SECTOR_COUNT_1M / 4, WP_GD8M_1_4},
    {SECTOR_COUNT_1M / 8, WP_GD8M_1_8},
    {SECTOR_COUNT_1M / 16, WP_GD8M_1_16},
    {SECTOR_COUNT_32K, WP_GD8M_32K},
    {SECTOR_COUNT_16K, WP_GD8M_16K},
    {SECTOR_COUNT_8K, WP_GD8M_8K},
    {SECTOR_COUNT_4K, WP_GD8M_4K},
    {0, WP_GD8M_NONE},
};

/**
 * Table for GD 2MB
 */
FLASHRAM_CODE static const halSpiFlashWpMap_t gGD16MWpMap[] = {
    {SECTOR_COUNT_2M, WP_GD16M_ALL},
    {SECTOR_COUNT_2M - SECTOR_COUNT_2M / 32, WP_GD16M_31_32},
    {SECTOR_COUNT_2M - SECTOR_COUNT_2M / 16, WP_GD16M_15_16},
    {SECTOR_COUNT_2M - SECTOR_COUNT_2M / 8, WP_GD16M_7_8},
    {SECTOR_COUNT_2M - SECTOR_COUNT_2M / 4, WP_GD16M_3_4},
    {SECTOR_COUNT_2M / 2, WP_GD16M_1_2},
    {SECTOR_COUNT_2M / 4, WP_GD16M_1_4},
    {SECTOR_COUNT_2M / 8, WP_GD16M_1_8},
    {SECTOR_COUNT_2M / 16, WP_GD16M_1_16},
    {SECTOR_COUNT_2M / 32, WP_GD16M_1_32},
    {SECTOR_COUNT_32K, WP_GD16M_32K},
    {SECTOR_COUNT_16K, WP_GD16M_16K},
    {SECTOR_COUNT_8K, WP_GD16M_8K},
    {SECTOR_COUNT_4K, WP_GD16M_4K},
    {0, WP_GD16M_NONE},
};

/**
 * Table for GD 4MB
 */
FLASHRAM_CODE static const halSpiFlashWpMap_t gGD32MWpMap[] = {
    {SECTOR_COUNT_4M, WP_GD32M_ALL},
    {SECTOR_COUNT_4M - SECTOR_COUNT_4M / 64, WP_GD32M_63_64},
    {SECTOR_COUNT_4M - SECTOR_COUNT_4M / 32, WP_GD32M_31_32},
    {SECTOR_COUNT_4M - SECTOR_COUNT_4M / 16, WP_GD32M_15_16},
    {SECTOR_COUNT_4M - SECTOR_COUNT_4M / 8, WP_GD32M_7_8},
    {SECTOR_COUNT_4M - SECTOR_COUNT_4M / 4, WP_GD32M_3_4},
    {SECTOR_COUNT_4M / 2, WP_GD32M_1_2},
    {SECTOR_COUNT_4M / 4, WP_GD32M_1_4},
    {SECTOR_COUNT_4M / 8, WP_GD32M_1_8},
    {SECTOR_COUNT_4M / 16, WP_GD32M_1_16},
    {SECTOR_COUNT_4M / 32, WP_GD32M_1_32},
    {SECTOR_COUNT_4M / 64, WP_GD32M_1_64},
    {SECTOR_COUNT_32K, WP_GD32M_32K},
    {SECTOR_COUNT_16K, WP_GD32M_16K},
    {SECTOR_COUNT_8K, WP_GD32M_8K},
    {SECTOR_COUNT_4K, WP_GD32M_4K},
    {0, WP_GD32M_NONE},
};

/**
 * Table for GD 8MB
 */
FLASHRAM_CODE static const halSpiFlashWpMap_t gGD64MWpMap[] = {
    {SECTOR_COUNT_8M, WP_GD32M_ALL},
    {SECTOR_COUNT_8M - SECTOR_COUNT_8M / 64, WP_GD32M_63_64},
    {SECTOR_COUNT_8M - SECTOR_COUNT_8M / 32, WP_GD32M_31_32},
    {SECTOR_COUNT_8M - SECTOR_COUNT_8M / 16, WP_GD32M_15_16},
    {SECTOR_COUNT_8M - SECTOR_COUNT_8M / 8, WP_GD32M_7_8},
    {SECTOR_COUNT_8M - SECTOR_COUNT_8M / 4, WP_GD32M_3_4},
    {SECTOR_COUNT_8M / 2, WP_GD32M_1_2},
    {SECTOR_COUNT_8M / 4, WP_GD32M_1_4},
    {SECTOR_COUNT_8M / 8, WP_GD32M_1_8},
    {SECTOR_COUNT_8M / 16, WP_GD32M_1_16},
    {SECTOR_COUNT_8M / 32, WP_GD32M_1_32},
    {SECTOR_COUNT_8M / 64, WP_GD32M_1_64},
    {SECTOR_COUNT_32K, WP_GD32M_32K},
    {SECTOR_COUNT_16K, WP_GD32M_16K},
    {SECTOR_COUNT_8K, WP_GD32M_8K},
    {SECTOR_COUNT_4K, WP_GD32M_4K},
    {0, WP_GD32M_NONE},
};

/**
 * Table for GD 16MB
 */
FLASHRAM_CODE static const halSpiFlashWpMap_t gGD128MWpMap[] = {
    {SECTOR_COUNT_16M, WP_GD32M_ALL},
    {SECTOR_COUNT_16M - SECTOR_COUNT_16M / 64, WP_GD32M_63_64},
    {SECTOR_COUNT_16M - SECTOR_COUNT_16M / 32, WP_GD32M_31_32},
    {SECTOR_COUNT_16M - SECTOR_COUNT_16M / 16, WP_GD32M_15_16},
    {SECTOR_COUNT_16M - SECTOR_COUNT_16M / 8, WP_GD32M_7_8},
    {SECTOR_COUNT_16M - SECTOR_COUNT_16M / 4, WP_GD32M_3_4},
    {SECTOR_COUNT_16M / 2, WP_GD32M_1_2},
    {SECTOR_COUNT_16M / 4, WP_GD32M_1_4},
    {SECTOR_COUNT_16M / 8, WP_GD32M_1_8},
    {SECTOR_COUNT_16M / 16, WP_GD32M_1_16},
    {SECTOR_COUNT_16M / 32, WP_GD32M_1_32},
    {SECTOR_COUNT_16M / 64, WP_GD32M_1_64},
    {SECTOR_COUNT_32K, WP_GD32M_32K},
    {SECTOR_COUNT_16K, WP_GD32M_16K},
    {SECTOR_COUNT_8K, WP_GD32M_8K},
    {SECTOR_COUNT_4K, WP_GD32M_4K},
    {0, WP_GD32M_NONE},
};

/**
 * Table for XMCA
 */
FLASHRAM_CODE static const halSpiFlashWpMap_t gXmcaWpMap[] = {
    {128, WP_XMCA_ALL},
    {127, WP_XMCA_127_128},
    {126, WP_XMCA_126_128},
    {124, WP_XMCA_124_128},
    {120, WP_XMCA_120_128},
    {112, WP_XMCA_112_128},
    {96, WP_XMCA_96_128},
    {64, WP_XMCA_64_128},
    {32, WP_XMCA_32_128},
    {16, WP_XMCA_16_128},
    {8, WP_XMCA_8_128},
    {4, WP_XMCA_4_128},
    {2, WP_XMCA_2_128},
    {1, WP_XMCA_1_128},
    {8, WP_XMCA_NONE},
};

/**
 * Find WP bits from offset
 */
FLASHRAM_CODE static uint16_t prvFindFromWpMap(const halSpiFlashWpMap_t *wpmap, uint32_t offset)
{
    for (;;)
    {
        if (offset >= wpmap->offset)
            return wpmap->wp;
        ++wpmap;
    }
}

/**
 * Find real offset from offset
 */
FLASHRAM_CODE static uint16_t prvFindFromWpOffset(const halSpiFlashWpMap_t *wpmap, uint32_t offset)
{
    for (;;)
    {
        if (offset >= wpmap->offset)
            return wpmap->offset;
        ++wpmap;
    }
}

/**
 * SR with proper WP bits
 */
FLASHRAM_CODE static uint16_t prvStatusWpLower(const halSpiFlashProp_t *prop, uint16_t status, uint32_t offset)
{
    uint32_t capacity = prop->capacity;
    if (prop->wp_gd_en)
    {
        unsigned scount = (offset / SIZE_4K);
        if (capacity == (1 << 20))
            return (status & ~WP_GD8M_MASK) | prvFindFromWpMap(gGD8MWpMap, scount);
        else if (capacity == (2 << 20))
            return (status & ~WP_GD16M_MASK) | prvFindFromWpMap(gGD16MWpMap, scount);
        else if (capacity == (4 << 20))
            return (status & ~WP_GD32M_MASK) | prvFindFromWpMap(gGD32MWpMap, scount);
        else if (capacity == (8 << 20))
            return (status & ~WP_GD32M_MASK) | prvFindFromWpMap(gGD64MWpMap, scount);
        else if (capacity == (16 << 20))
            return (status & ~WP_GD32M_MASK) | prvFindFromWpMap(gGD128MWpMap, scount);
        else
            return status;
    }
    else if (prop->wp_xmca_en)
    {
        unsigned num = offset >> (CAPACITY_BITS(prop->mid) - 7);
        return (status & ~WP_XMCA_MASK) | prvFindFromWpMap(gXmcaWpMap, num);
    }
    else
    {
        return status;
    }
}

/**
 * SR with proper WP bits
 */
FLASHRAM_API uint16_t halSpiFlashStatusWpRange(const halSpiFlashProp_t *prop, uint16_t status, uint32_t offset, uint32_t size)
{
    return prvStatusWpLower(prop, status, offset);
}

/**
 * WP range from offset
 */
FLASHRAM_API osiUintRange_t halSpiFlashWpRange(const halSpiFlashProp_t *prop, uint32_t offset, uint32_t size)
{
    osiUintRange_t range = {0, 0};
    uint32_t capacity = prop->capacity;
    if (prop->wp_gd_en)
    {
        unsigned scount = (offset / SIZE_4K);
        if (capacity == (1 << 20))
            range.maxval = prvFindFromWpOffset(gGD8MWpMap, scount) * SIZE_4K;
        else if (capacity == (2 << 20))
            range.maxval = prvFindFromWpOffset(gGD16MWpMap, scount) * SIZE_4K;
        else if (capacity == (4 << 20))
            range.maxval = prvFindFromWpOffset(gGD32MWpMap, scount) * SIZE_4K;
        else if (capacity == (8 << 20))
            range.maxval = prvFindFromWpOffset(gGD64MWpMap, scount) * SIZE_4K;
        else if (capacity == (16 << 20))
            range.maxval = prvFindFromWpOffset(gGD128MWpMap, scount) * SIZE_4K;
    }
    else if (prop->wp_xmca_en)
    {
        unsigned num = offset >> (CAPACITY_BITS(prop->mid) - 7);
        range.maxval = prvFindFromWpOffset(gXmcaWpMap, num) << (CAPACITY_BITS(prop->mid) - 7);
    }
    return range;
}

/**
 * Write volatile SR 1/2, with check
 */
FLASHRAM_CODE static void prvWriteVolatileSR12(uintptr_t hwp, const halSpiFlashProp_t *prop, uint16_t sr)
{
    for (;;)
    {
        if (prop->write_sr12)
        {
            halSpiFlashWriteVolatileSREnable(hwp);
            halSpiFlashWriteSR12(hwp, sr);
        }
        else
        {
            halSpiFlashWriteVolatileSREnable(hwp);
            halSpiFlashWriteSR1(hwp, sr & 0xff);
            halSpiFlashWriteVolatileSREnable(hwp);
            halSpiFlashWriteSR2(hwp, (sr >> 8) & 0xff);
        }

        // check whether volatile SR are written correctly
        uint16_t new_sr = halSpiFlashReadSR12(hwp);
        if (new_sr == sr)
            break;
    }
}

/**
 * Write volatile SR 1, with check
 */
FLASHRAM_CODE static void prvWriteVolatileSR1(uintptr_t hwp, const halSpiFlashProp_t *prop, uint8_t sr)
{
    for (;;)
    {
        halSpiFlashWriteVolatileSREnable(hwp);
        halSpiFlashWriteSR1(hwp, sr);

        // check whether volatile SR are written correctly
        uint8_t new_sr = halSpiFlashReadSR1(hwp);
        if (new_sr == sr)
            break;
    }
}

/**
 * Prepare for erase/program
 */
FLASHRAM_API void halSpiFlashPrepareEraseProgram(uintptr_t hwp, const halSpiFlashProp_t *prop, uint32_t offset, uint32_t size)
{
    if (prop->volatile_sr_en && prop->wp_gd_en)
    {
        uint16_t sr = halSpiFlashReadSR12(hwp);
        uint16_t sr_open = halSpiFlashStatusWpRange(prop, sr, offset, size);
        if (sr != sr_open)
            prvWriteVolatileSR12(hwp, prop, sr_open);
    }
    else if (prop->volatile_sr_en && prop->wp_xmca_en)
    {
        uint8_t sr = halSpiFlashReadSR1(hwp);
        uint8_t sr_open = halSpiFlashStatusWpRange(prop, sr, offset, size);
        if (sr != sr_open)
            prvWriteVolatileSR1(hwp, prop, sr_open);
    }
    halSpiFlashWriteEnable(hwp);
}

/**
 * Finish for erase/program
 */
FLASHRAM_API void halSpiFlashFinishEraseProgram(uintptr_t hwp, const halSpiFlashProp_t *prop)
{
    if (prop->volatile_sr_en && prop->wp_gd_en)
    {
        uint16_t sr = halSpiFlashReadSR12(hwp);
        uint16_t sr_close = halSpiFlashStatusWpAll(prop, sr);
        if (sr != sr_close)
            prvWriteVolatileSR12(hwp, prop, sr_close);
    }
    else if (prop->volatile_sr_en && prop->wp_xmca_en)
    {
        uint8_t sr = halSpiFlashReadSR1(hwp);
        uint8_t sr_close = halSpiFlashStatusWpAll(prop, sr);
        if (sr != sr_close)
            prvWriteVolatileSR1(hwp, prop, sr_close);
    }
}

/**
 * OPCODE_WRITE_VOLATILE_STATUS_ENABLE
 */
FLASHRAM_API void halSpiFlashWriteVolatileSREnable(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_WRITE_VOLATILE_STATUS_ENABLE));
}

/**
 * OPCODE_PAGE_PROGRAM
 */
FLASHRAM_API void halSpiFlashPageProgram(uintptr_t hwp, uint32_t offset, const void *data, uint32_t size)
{
#ifdef CONFIG_SOC_8811
    hwp_med->med_clr = 0xffffffff;
#endif
    prvCmdNoRx(hwp, CMD_ADDRESS(OPCODE_PAGE_PROGRAM, offset), data, size);
}

/**
 * OPCODE_SECTOR_ERASE
 */
FLASHRAM_API void halSpiFlashErase4K(uintptr_t hwp, uint32_t offset)
{
#ifdef CONFIG_SOC_8811
    hwp_med->med_clr = 0xffffffff;
#endif
    prvCmdOnlyNoRx(hwp, CMD_ADDRESS(OPCODE_SECTOR_ERASE, offset));
}

/**
 * OPCODE_BLOCK32K_ERASE
 */
FLASHRAM_API void halSpiFlashErase32K(uintptr_t hwp, uint32_t offset)
{
#ifdef CONFIG_SOC_8811
    hwp_med->med_clr = 0xffffffff;
#endif
    prvCmdOnlyNoRx(hwp, CMD_ADDRESS(OPCODE_BLOCK32K_ERASE, offset));
}

/**
 * OPCODE_BLOCK_ERASE
 */
FLASHRAM_API void halSpiFlashErase64K(uintptr_t hwp, uint32_t offset)
{
#ifdef CONFIG_SOC_8811
    hwp_med->med_clr = 0xffffffff;
#endif
    prvCmdOnlyNoRx(hwp, CMD_ADDRESS(OPCODE_BLOCK_ERASE, offset));
}

/**
 * OPCODE_PROGRAM_SUSPEND
 */
FLASHRAM_API void halSpiFlashProgramSuspend(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_PROGRAM_SUSPEND));
}

/**
 * OPCODE_ERASE_SUSPEND
 */
FLASHRAM_API void halSpiFlashEraseSuspend(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_ERASE_SUSPEND));
}

/**
 * OPCODE_PROGRAM_RESUME
 */
FLASHRAM_API void halSpiFlashProgramResume(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_PROGRAM_RESUME));
}

/**
 * OPCODE_ERASE_RESUME
 */
FLASHRAM_API void halSpiFlashEraseResume(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_ERASE_RESUME));
}

/**
 * OPCODE_CHIP_ERASE
 */
FLASHRAM_API void halSpiFlashChipErase(uintptr_t hwp)
{
#ifdef CONFIG_SOC_8811
    hwp_med->med_clr = 0xffffffff;
#endif
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_CHIP_ERASE));
}

/**
 * OPCODE_POWER_DOWN
 */
FLASHRAM_API void halSpiFlashDeepPowerDown(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_POWER_DOWN));
}

/**
 * OPCODE_RELEASE_POWER_DOWN
 */
FLASHRAM_API void halSpiFlashReleaseDeepPowerDown(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_RELEASE_POWER_DOWN));
    osiDelayUS(DELAY_AFTER_RELEASE_DEEP_POWER_DOWN);
}

/**
 * OPCODE_SECURITY_PROGRAM
 */
FLASHRAM_API void halSpiFlashProgramSecurityRegister(uintptr_t hwp, uint32_t address, const void *data, uint32_t size)
{
    prvCmdNoRx(hwp, CMD_ADDRESS(OPCODE_SECURITY_PROGRAM, address), data, size);
}

/**
 * OPCODE_SECURITY_ERASE
 */
FLASHRAM_API void halSpiFlashEraseSecurityRegister(uintptr_t hwp, uint32_t address)
{
    prvCmdOnlyNoRx(hwp, CMD_ADDRESS(OPCODE_SECURITY_ERASE, address));
}

/**
 * Erase (64K/32K/4K)
 */
FLASHRAM_API void halSpiFlashErase(uintptr_t hwp, uint32_t offset, uint32_t size)
{
    if (size == SIZE_64K)
        halSpiFlashErase64K(hwp, offset);
    else if (size == SIZE_32K)
        halSpiFlashErase32K(hwp, offset);
    else
        halSpiFlashErase4K(hwp, offset);
}

/**
 * OPCODE_READ_UNIQUE_ID
 */
FLASHRAM_API void halSpiFlashReadUniqueId(uintptr_t hwp, uint8_t uid[8])
{
    uint8_t tx_data[4] = {};
    prvCmdRxFifo(hwp, EXTCMD_SRX(OPCODE_READ_UNIQUE_ID), tx_data, 4, uid, 8);
}

/**
 * OPCODE_READ_UNIQUE_ID
 */
FLASHRAM_API uint16_t halSpiFlashReadCpId(uintptr_t hwp)
{
    uint8_t uid[18];
    uint8_t tx_data[4] = {};
    prvCmdRxFifo(hwp, EXTCMD_SRX(OPCODE_READ_UNIQUE_ID), tx_data, 4, uid, 18);
    return osiBytesGetLe16(&uid[16]);
}

/**
 * OPCODE_SECURITY_READ
 */
FLASHRAM_API void halSpiFlashReadSecurityRegister(uintptr_t hwp, uint32_t address, void *data, uint32_t size)
{
    uint8_t tx_data[4] = {(address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff, 0};
    uint32_t val = prvCmdRxReadback(hwp, EXTCMD_SRX(OPCODE_SECURITY_READ), size, tx_data, 4);

    uint8_t *pin = (uint8_t *)&val;
    uint8_t *pout = (uint8_t *)data;
    for (unsigned n = 0; n < size; n++)
        *pout++ = *pin++;
}

/**
 * OPCODE_READ_SFDP
 */
FLASHRAM_API void halSpiFlashReadSFDP(uintptr_t hwp, uint32_t address, void *data, uint32_t size)
{
    uint8_t tx_data[4] = {(address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff, 0};
    prvCmdRxFifo(hwp, EXTCMD_SRX(OPCODE_READ_SFDP), tx_data, 4, data, size);
}
