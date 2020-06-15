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

#include "drv_spi_flash.h"
#include "hal_spi_flash.h"
#include "hal_spi_flash_defs.h"
#include "hal_chip.h"
#include "hwregs.h"
#include "osi_api.h"
#include "osi_profile.h"
#include "osi_byte_buf.h"
#ifdef CONFIG_CPU_ARM
#include "cmsis_core.h"
#endif
#include <string.h>

#define FLASHRAM_CODE OSI_SECTION_LINE(.ramtext.flashdrv)
#define FLASHRAM_API FLASHRAM_CODE OSI_NO_INLINE

// From experiments, 2us is enough. However, it is better to follow datasheet
#define DELAY_AFTER_SUSPEND (20) // us

// From experiments, if the time from resume to suspend is too short,
// the erase will take huge time to finish. Though in most cases there
// doesn't exist soo many interrupt, it is needed to avoid worst case.
#define MIN_ERASE_PROGRAM_TIME (100) // us

enum
{
    FLASH_NO_ERASE_PROGRAM,
    FLASH_ERASE,
    FLASH_PROGRAM,
    FLASH_ERASE_SUSPEND,
    FLASH_PROGRAM_SUSPEND
};

struct drvSpiFlash
{
    bool opened;
    bool xip_protect;
    unsigned name;
    uintptr_t hwp;
    halSpiFlashProp_t prop;
    uintptr_t base_address;
    osiMutex_t *lock;           // suspend will cause thread switch
    uint32_t block_prohibit[8]; // 16M/64K/32bit_per_word
};

#if defined(CONFIG_SOC_8910)
static drvSpiFlash_t gDrvSpiFlashCtx[] = {
    {
        .opened = false,
        .xip_protect = true,
        .name = DRV_NAME_SPI_FLASH,
        .hwp = (uintptr_t)hwp_spiFlash,
        .base_address = CONFIG_NOR_PHY_ADDRESS,
    },
    {
        .opened = false,
        .xip_protect = true,
        .name = DRV_NAME_SPI_FLASH_EXT,
        .hwp = (uintptr_t)hwp_spiFlash1,
        .base_address = CONFIG_NOR_EXT_PHY_ADDRESS,
    },
};
#endif

#ifdef CONFIG_SOC_8955
static drvSpiFlash_t gDrvSpiFlashCtx[] = {
    {
        .opened = false,
        .xip_protect = true,
        .name = DRV_NAME_SPI_FLASH,
        .hwp = (uintptr_t)hwp_spiFlash,
        .base_address = CONFIG_NOR_PHY_ADDRESS,
    },
};
#endif

#ifdef CONFIG_SOC_8909
static drvSpiFlash_t gDrvSpiFlashCtx[] = {
    {
        .opened = false,
        .xip_protect = true,
        .name = DRV_NAME_SPI_FLASH,
        .hwp = (uintptr_t)hwp_spiFlash,
        .base_address = CONFIG_NOR_PHY_ADDRESS,
    },
    {
        .opened = false,
        .xip_protect = true,
        .name = DRV_NAME_SPI_FLASH_EXT,
        .hwp = (uintptr_t)hwp_spiFlashExt,
        .base_address = CONFIG_NOR_EXT_PHY_ADDRESS,
    },
};
#endif

#ifdef CONFIG_SOC_8811
static drvSpiFlash_t gDrvSpiFlashCtx[] = {
    {
        .opened = false,
        .xip_protect = true,
        .name = DRV_NAME_SPI_FLASH,
        .hwp = (uintptr_t)hwp_spiFlash,
        .base_address = CONFIG_NOR_PHY_ADDRESS,
    },
    {
        .opened = false,
        .xip_protect = true,
        .name = DRV_NAME_SPI_FLASH_EXT,
        .hwp = (uintptr_t)hwp_spiflashExt,
        .base_address = CONFIG_NOR_EXT_PHY_ADDRESS,
    },
};
#endif

/**
 * Find flash instance by name
 */
OSI_FORCE_INLINE static inline drvSpiFlash_t *prvFlashGetByName(uint32_t name)
{
    for (unsigned n = 0; n < OSI_ARRAY_SIZE(gDrvSpiFlashCtx); n++)
    {
        if (gDrvSpiFlashCtx[n].name == name)
            return &gDrvSpiFlashCtx[n];
    }
    return NULL;
}

/**
 * Flash API protection acquire, it is different for XIP/NON-XIP
 */
OSI_FORCE_INLINE static uint32_t prvProtectAquire(drvSpiFlash_t *d)
{
    osiMutexLock(d->lock);
    return (d->xip_protect) ? osiEnterCritical() : 0;
}

/**
 * Flash API protection release, it is different for XIP/NON-XIP
 */
OSI_FORCE_INLINE static void prvProtectRelease(drvSpiFlash_t *d, uint32_t critical)
{
    if (d->xip_protect)
        osiExitCritical(critical);
    osiMutexUnlock(d->lock);
}

/**
 * Whether address is soft write protected
 */
OSI_FORCE_INLINE static bool prvIsWriteProhibit(drvSpiFlash_t *d, uint32_t address)
{
    unsigned block = address / SIZE_64K; // 64KB block
    return osiBitmapIsSet(d->block_prohibit, block);
}

/**
 * Whether range is soft write protected
 */
FLASHRAM_CODE static bool prvIsRangeWriteProhibit(drvSpiFlash_t *d, uint32_t address, unsigned size)
{
    uint32_t critical = osiEnterCritical();
    for (unsigned n = 0; n < size; n += SIZE_64K)
    {
        if (prvIsWriteProhibit(d, address + n))
        {
            osiExitCritical(critical);
            return true;
        }
    }
    osiExitCritical(critical);
    return false;
}

/**
 * Disable AHB read of flash controller
 */
OSI_FORCE_INLINE static void prvDisableAhbRead(drvSpiFlash_t *d)
{
#ifdef CONFIG_SOC_8910
    // not use AHB_READ_DISABLE feature
#elif defined(CONFIG_SOC_8811)
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d->hwp;
    REG_SPI_FLASH_AHB_FLASH_CTRL_T ahb_flash_ctrl;
    REG_FIELD_CHANGE1(hwp->ahb_flash_ctrl, ahb_flash_ctrl,
                      ahb_read, SPI_FLASH_AHB_READ_V_DISABLE);
#else
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d->hwp;
    REG_SPI_FLASH_SPI_CS_SIZE_T spi_cs_size;
    REG_FIELD_CHANGE1(hwp->spi_cs_size, spi_cs_size,
                      ahb_read_disable, 1);
#endif
}

/**
 * Enable AHB read of flash controller
 */
OSI_FORCE_INLINE static void prvEnableAhbRead(drvSpiFlash_t *d)
{
#ifdef CONFIG_SOC_8910
    // It is not clear whether there are "hidden" instruction prefetch
    // during flash erase/program. If it happens, the prefetch instructions
    // is not reliable, and invalidate I-Cache will be useful. And the
    // performance impact is acceptable.
    L1C_InvalidateICacheAll();
#elif defined(CONFIG_SOC_8811)
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d->hwp;
    REG_SPI_FLASH_AHB_FLASH_CTRL_T ahb_flash_ctrl;
    REG_FIELD_CHANGE1(hwp->ahb_flash_ctrl, ahb_flash_ctrl,
                      ahb_read, SPI_FLASH_AHB_READ_V_ENABLE);
#else
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d->hwp;
    REG_SPI_FLASH_SPI_CS_SIZE_T spi_cs_size;
    REG_FIELD_CHANGE1(hwp->spi_cs_size, spi_cs_size,
                      ahb_read_disable, 0);
#endif
}

/**
 * Page program with XIP
 */
FLASHRAM_CODE static void prvPageProgram(
    drvSpiFlash_t *d, uint32_t offset,
    const uint8_t *data, size_t size)
{
    uint32_t critical = osiEnterCritical();

    osiProfileEnter(PROFCODE_FLASH_PROGRAM);
    prvDisableAhbRead(d);

    halSpiFlashPrepareEraseProgram(d->hwp, &d->prop, offset, size);
    halSpiFlashPageProgram(d->hwp, offset, data, size);
    osiDelayUS(MIN_ERASE_PROGRAM_TIME); // Wait a while

    for (;;)
    {
        if (halSpiFlashIsWipFinished(d->hwp))
        {
            halSpiFlashFinishEraseProgram(d->hwp, &d->prop);
            prvEnableAhbRead(d);

            osiProfileExit(PROFCODE_FLASH_PROGRAM);
            osiExitCritical(critical);
            return;
        }

        if (d->prop.suspend_en && osiIrqPending())
        {
            halSpiFlashProgramSuspend(d->hwp);

            osiDelayUS(DELAY_AFTER_SUSPEND); // Wait a while
            halSpiFlashWaitWipFinish(d->hwp);
            prvEnableAhbRead(d);
            osiExitCritical(critical);

            osiDelayUS(5); // avoid CPU can't take interrupt

            critical = osiEnterCritical();

            prvDisableAhbRead(d);
            halSpiFlashProgramResume(d->hwp);
            osiDelayUS(MIN_ERASE_PROGRAM_TIME); // Wait a while
        }
    }
}

/**
 * Erase with XIP
 */
FLASHRAM_CODE static void prvErase(drvSpiFlash_t *d, uint32_t offset, size_t size)
{
    uint32_t critical = osiEnterCritical();

    osiProfileEnter(PROFCODE_FLASH_ERASE);
    prvDisableAhbRead(d);

    halSpiFlashPrepareEraseProgram(d->hwp, &d->prop, offset, size);
    halSpiFlashErase(d->hwp, offset, size);
    osiDelayUS(MIN_ERASE_PROGRAM_TIME); // Wait a while

    for (;;)
    {
        if (halSpiFlashIsWipFinished(d->hwp))
        {
            halSpiFlashFinishEraseProgram(d->hwp, &d->prop);
            prvEnableAhbRead(d);

            osiProfileExit(PROFCODE_FLASH_ERASE);
            osiExitCritical(critical);
            return;
        }

        if (d->prop.suspend_en && osiIrqPending())
        {
            halSpiFlashEraseSuspend(d->hwp);

            osiDelayUS(DELAY_AFTER_SUSPEND); // Wait a while
            halSpiFlashWaitWipFinish(d->hwp);
            prvEnableAhbRead(d);
            osiExitCritical(critical);

            osiDelayUS(5); // avoid CPU can't take interrupt

            critical = osiEnterCritical();

            prvDisableAhbRead(d);
            halSpiFlashEraseResume(d->hwp);
            osiDelayUS(MIN_ERASE_PROGRAM_TIME); // Wait a while
        }
    }
}

/**
 * Page program without XIP
 */
FLASHRAM_CODE static void prvEraseNoXipLocked(drvSpiFlash_t *d, uint32_t offset, size_t size)
{
    prvDisableAhbRead(d);
    halSpiFlashPrepareEraseProgram(d->hwp, &d->prop, offset, size);
    halSpiFlashErase(d->hwp, offset, size);
    halSpiFlashWaitWipFinish(d->hwp);
    halSpiFlashFinishEraseProgram(d->hwp, &d->prop);
    prvEnableAhbRead(d);
}

/**
 * Erase without XIP
 */
FLASHRAM_CODE static void prvPageProgramNoXipLocked(
    drvSpiFlash_t *d, uint32_t offset,
    const uint8_t *data, size_t size)
{
    prvDisableAhbRead(d);
    halSpiFlashPrepareEraseProgram(d->hwp, &d->prop, offset, size);
    halSpiFlashPageProgram(d->hwp, offset, data, size);
    halSpiFlashWaitWipFinish(d->hwp);
    halSpiFlashFinishEraseProgram(d->hwp, &d->prop);
    prvEnableAhbRead(d);
}

/**
 * Open flash driver
 */
FLASHRAM_API drvSpiFlash_t *drvSpiFlashOpen(uint32_t name)
{
    drvSpiFlash_t *d = prvFlashGetByName(name);
    if (d == NULL)
        return NULL;

    uint32_t critical = osiEnterCritical();

    if (!d->opened)
    {
        halSpiFlashStatusCheck(d->hwp);
        d->prop.mid = halSpiFlashReadId(d->hwp);
        halSpiFlashPropsByMid(d->prop.mid, &d->prop);

        // !!! THESE FLASH ARE UNDER TEST, THOUGH IT SEEMS TO WORK.
        if (d->prop.type == HAL_SPI_FLASH_TYPE_XTX ||
            d->prop.type == HAL_SPI_FLASH_TYPE_XMCA)
        {
            osiExitCritical(critical);
            return NULL;
        }

        d->opened = true;
        d->lock = osiMutexCreate();
    }

    osiExitCritical(critical);
    return d;
}

/**
 * Set whether XIP protection is needed
 */
bool drvSpiFlashSetXipEnabled(drvSpiFlash_t *d, bool enable)
{
    if (d == NULL || d->name == DRV_NAME_SPI_FLASH)
        return false;

    d->xip_protect = enable;
    return true;
}

/**
 * Set range of soft write protection
 */
void drvSpiFlashSetRangeWriteProhibit(drvSpiFlash_t *d, uint32_t start, uint32_t end)
{
    if (start > d->prop.capacity || end > d->prop.capacity)
        return;

    unsigned block_start = OSI_ALIGN_UP(start, SIZE_64K) / SIZE_64K;
    unsigned block_end = OSI_ALIGN_DOWN(end, SIZE_64K) / SIZE_64K;
    for (unsigned block = block_start; block < block_end; block++)
        osiBitmapSet(d->block_prohibit, block);
}

/**
 * Clear range of soft write protection
 */
void drvSpiFlashClearRangeWriteProhibit(drvSpiFlash_t *d, uint32_t start, uint32_t end)
{
    if (start > d->prop.capacity || end > d->prop.capacity)
        return;

    unsigned block_start = OSI_ALIGN_UP(start, SIZE_64K) / SIZE_64K;
    unsigned block_end = OSI_ALIGN_DOWN(end, SIZE_64K) / SIZE_64K;
    for (unsigned block = block_start; block < block_end; block++)
        osiBitmapClear(d->block_prohibit, block);
}

/**
 * Get flash manufacture id
 */
uint32_t drvSpiFlashGetID(drvSpiFlash_t *d)
{
    return d->prop.mid;
}

/**
 * Get flash capacity in bytes
 */
uint32_t drvSpiFlashCapacity(drvSpiFlash_t *d)
{
    return d->prop.capacity;
}

/**
 * Map flash address to CPU accessible address
 */
const void *drvSpiFlashMapAddress(drvSpiFlash_t *d, uint32_t offset)
{
    if (offset >= d->prop.capacity)
        return NULL;
    return (const void *)REG_ACCESS_ADDRESS((uintptr_t)d->base_address + offset);
}

/**
 * Read data from flash
 */
bool drvSpiFlashRead(drvSpiFlash_t *d, uint32_t offset, void *data, uint32_t size)
{
    if (size == 0)
        return true;
    if (d == NULL || data == NULL || offset + size > d->prop.capacity)
        return false;

    const void *fl = (const void *)REG_ACCESS_ADDRESS((uintptr_t)d->base_address + offset);
    memcpy(data, fl, size);
    return true;
}

/**
 * Read data from flash, and check with provided data
 */
bool drvSpiFlashReadCheck(drvSpiFlash_t *d, uint32_t offset, const void *data, uint32_t size)
{
    if (size == 0)
        return true;
    if (d == NULL || data == NULL || offset + size > d->prop.capacity)
        return false;

    const void *fl = (const void *)REG_ACCESS_ADDRESS((uintptr_t)d->base_address + offset);
    return memcmp(data, fl, size) == 0;
}

/**
 * Write to flash
 */
FLASHRAM_API bool drvSpiFlashWrite(drvSpiFlash_t *d, uint32_t offset, const void *data, size_t size)
{
    uint32_t o_offset = offset;
    size_t o_size = size;

    if (data == NULL || offset + size > d->prop.capacity)
        return false;

    if (prvIsRangeWriteProhibit(d, offset, size))
        return false;

    osiMutexLock(d->lock); // exclusive write
    if (d->xip_protect)
    {
        while (size > 0)
        {
            uint32_t next_page = OSI_ALIGN_DOWN(offset + PAGE_SIZE, PAGE_SIZE);
            uint32_t bsize = OSI_MIN(unsigned, next_page - offset, size);

            prvPageProgram(d, offset, data, bsize);
            data = (const char *)data + bsize;
            offset += bsize;
            size -= bsize;
        }
    }
    else
    {
        while (size > 0)
        {
            uint32_t next_page = OSI_ALIGN_DOWN(offset + PAGE_SIZE, PAGE_SIZE);
            uint32_t bsize = OSI_MIN(unsigned, next_page - offset, size);

            prvPageProgramNoXipLocked(d, offset, data, bsize);
            data = (const char *)data + bsize;
            offset += bsize;
            size -= bsize;
        }
    }

    osiMutexUnlock(d->lock);
    osiDCacheInvalidate(drvSpiFlashMapAddress(d, o_offset), o_size);
    return true;
}

/**
 * Erase flash range
 */
FLASHRAM_API bool drvSpiFlashErase(drvSpiFlash_t *d, uint32_t offset, size_t size)
{
    uint32_t o_offset = offset;
    size_t o_size = size;

    if (offset + size > d->prop.capacity ||
        !OSI_IS_ALIGNED(offset, SIZE_4K) ||
        !OSI_IS_ALIGNED(size, SIZE_4K))
        return false;

    if (prvIsRangeWriteProhibit(d, offset, size))
        return false;

    osiMutexLock(d->lock);
    if (d->xip_protect)
    {
        while (size > 0)
        {
            if (OSI_IS_ALIGNED(offset, SIZE_64K) && size >= SIZE_64K)
            {
                prvErase(d, offset, SIZE_64K);
                offset += SIZE_64K;
                size -= SIZE_64K;
            }
            else if (OSI_IS_ALIGNED(offset, SIZE_32K) && size >= SIZE_32K)
            {
                prvErase(d, offset, SIZE_32K);
                offset += SIZE_32K;
                size -= SIZE_32K;
            }
            else
            {
                prvErase(d, offset, SIZE_4K);
                offset += SIZE_4K;
                size -= SIZE_4K;
            }
        }
    }
    else
    {
        while (size > 0)
        {
            if (OSI_IS_ALIGNED(offset, SIZE_64K) && size >= SIZE_64K)
            {
                prvEraseNoXipLocked(d, offset, SIZE_64K);
                offset += SIZE_64K;
                size -= SIZE_64K;
            }
            else if (OSI_IS_ALIGNED(offset, SIZE_32K) && size >= SIZE_32K)
            {
                prvEraseNoXipLocked(d, offset, SIZE_32K);
                offset += SIZE_32K;
                size -= SIZE_32K;
            }
            else
            {
                prvEraseNoXipLocked(d, offset, SIZE_4K);
                offset += SIZE_4K;
                size -= SIZE_4K;
            }
        }
    }

    osiMutexUnlock(d->lock);
    osiDCacheInvalidate(drvSpiFlashMapAddress(d, o_offset), o_size);
    return true;
}

/**
 * Chip erase
 */
FLASHRAM_API bool drvSpiFlashChipErase(drvSpiFlash_t *d)
{
    if (d == NULL || prvIsRangeWriteProhibit(d, 0, d->prop.capacity))
        return false;

    uint32_t critical = prvProtectAquire(d);

    halSpiFlashPrepareEraseProgram(d->hwp, &d->prop, 0, d->prop.capacity);
    halSpiFlashChipErase(d->hwp);
    halSpiFlashWaitWipFinish(d->hwp);
    halSpiFlashFinishEraseProgram(d->hwp, &d->prop);

    prvProtectRelease(d, critical);
    return true;
}

/**
 * Read flash status register
 */
FLASHRAM_API uint16_t drvSpiFlashReadSR(drvSpiFlash_t *d)
{
    if (d == NULL)
        return 0;

    uint32_t critical = prvProtectAquire(d);
    uint16_t sr = d->prop.has_sr2 ? halSpiFlashReadSR12(d->hwp) : halSpiFlashReadSR1(d->hwp);
    prvProtectRelease(d, critical);
    return sr;
}

/**
 * write flash status register
 */
FLASHRAM_API void drvSpiFlashWriteSR(drvSpiFlash_t *d, uint16_t sr)
{
    if (d == NULL)
        return;

    uint32_t critical = prvProtectAquire(d);
    halSpiFlashWriteSRByProp(d->hwp, &d->prop, sr);
    prvProtectRelease(d, critical);
}

/**
 * Enter deep power down mode
 */
FLASHRAM_API void drvSpiFlashDeepPowerDown(drvSpiFlash_t *d)
{
    uint32_t critical = prvProtectAquire(d);
    halSpiFlashDeepPowerDown(d->hwp);
    prvProtectRelease(d, critical);
}

/**
 * Release deep power down mode
 */
FLASHRAM_API void drvSpiFlashReleaseDeepPowerDown(drvSpiFlash_t *d)
{
    uint32_t critical = prvProtectAquire(d);
    halSpiFlashReleaseDeepPowerDown(d->hwp);
    prvProtectRelease(d, critical);
}

/**
 * Read unique ID
 */
FLASHRAM_API int drvSpiFlashReadUniqueId(drvSpiFlash_t *d, uint8_t *id)
{
    if (d == NULL || id == NULL)
        return -1;

    int size = -1;
    uint32_t critical = prvProtectAquire(d);
    if (d->prop.uid_gd_en)
    {
        size = 8;
        halSpiFlashReadUniqueId(d->hwp, id);
    }
    else if (d->prop.type == HAL_SPI_FLASH_TYPE_XMCA)
    {
        size = 12;
        halSpiFlashReadSFDP(d->hwp, 0x80, id, 12);
    }
    else if (d->prop.type == HAL_SPI_FLASH_TYPE_XTX)
    {
        size = 16;
        halSpiFlashReadSFDP(d->hwp, 0x194, id, 16);
    }
    prvProtectRelease(d, critical);
    return size;
}

/**
 * Read CPID
 */
FLASHRAM_API uint16_t drvSpiFlashReadCpId(drvSpiFlash_t *d)
{
    if (d == NULL || !d->prop.cpid_en)
        return 0;

    uint32_t critical = prvProtectAquire(d);
    uint16_t id = halSpiFlashReadCpId(d->hwp);
    prvProtectRelease(d, critical);
    return id;
}

/**
 * Read security registers
 */
FLASHRAM_API bool drvSpiFlashReadSecurityRegister(
    drvSpiFlash_t *d, uint8_t num, uint16_t address,
    void *data, uint32_t size)
{
    if (size == 0)
        return true;
    if (d == NULL || data == NULL)
        return false;
    if (!d->prop.sreg_3_en && !d->prop.sreg_1_en)
        return false;
    if (address + size > d->prop.sreg_block_size)
        return false;
    if (d->prop.sreg_3_en && (num < 1 || num > 3))
        return false;
    if (d->prop.sreg_1_en && num != 0)
        return false;

    uint32_t saddr = d->prop.sreg_3_en ? ((num << 12) | address) : address;
    while (size > 0)
    {
        uint32_t read_size = OSI_MIN(uint32_t, size, 4);

        uint32_t critical = prvProtectAquire(d);
        halSpiFlashReadSecurityRegister(d->hwp, saddr, data, read_size);
        prvProtectRelease(d, critical);

        saddr += read_size;
        size -= read_size;
        data = (char *)data + read_size;
    }
    return true;
}

/**
 * Read security registers
 */
FLASHRAM_API bool drvSpiFlashProgramSecurityRegister(
    drvSpiFlash_t *d, uint8_t num, uint16_t address,
    const void *data, uint32_t size)
{
    if (size == 0)
        return true;
    if (d == NULL || data == NULL)
        return false;
    if (!d->prop.sreg_3_en && !d->prop.sreg_1_en)
        return false;
    if (address + size > d->prop.sreg_block_size)
        return false;
    if (d->prop.sreg_3_en && (num < 1 || num > 3))
        return false;
    if (d->prop.sreg_1_en && num != 0)
        return false;

    uint32_t saddr = d->prop.sreg_3_en ? ((num << 12) | address) : address;
    while (size > 0)
    {
        unsigned block_size = OSI_MIN(unsigned, size, PAGE_SIZE);

        uint32_t critical = prvProtectAquire(d); // avoid read, and there are no suspend
        halSpiFlashWriteEnable(d->hwp);
        halSpiFlashProgramSecurityRegister(d->hwp, saddr, data, block_size);
        halSpiFlashWaitWipFinish(d->hwp);
        prvProtectRelease(d, critical);

        saddr += block_size;
        data = (char *)data + block_size;
        size -= block_size;
    }

    return true;
}

/**
 * Erase security register
 */
FLASHRAM_API bool drvSpiFlashEraseSecurityRegister(drvSpiFlash_t *d, uint8_t num)
{
    if (d == NULL)
        return false;
    if (!d->prop.sreg_3_en && !d->prop.sreg_1_en)
        return false;
    if (d->prop.sreg_3_en && (num < 1 || num > 3))
        return false;
    if (d->prop.sreg_1_en && num != 0)
        return false;

    uint32_t saddr = d->prop.sreg_3_en ? (num << 12) : 0;
    uint32_t critical = prvProtectAquire(d); // avoid read, and there are no suspend
    halSpiFlashWriteEnable(d->hwp);
    halSpiFlashEraseSecurityRegister(d->hwp, saddr);
    halSpiFlashWaitWipFinish(d->hwp);
    prvProtectRelease(d, critical);
    return true;
}

/**
 * Lock security register
 */
FLASHRAM_API bool drvSpiFlashLockSecurityRegister(drvSpiFlash_t *d, uint8_t num)
{
    if (d == NULL)
        return false;
    if (!d->prop.sreg_3_en && !d->prop.sreg_1_en)
        return false;
    if (d->prop.sreg_3_en && (num < 1 || num > 3))
        return false;
    if (d->prop.sreg_1_en && num != 0)
        return false;

    uint16_t mask = d->prop.sreg_1_en ? STREG_LB : (num == 1) ? STREG_LB1 : (num == 2) ? STREG_LB2 : STREG_LB3;

    uint32_t critical = prvProtectAquire(d);
    uint32_t sr = halSpiFlashReadSR12(d->hwp);
    halSpiFlashWriteSRByProp(d->hwp, &d->prop, sr | mask);
    prvProtectRelease(d, critical);
    return true;
}

/**
 * Read SFDP
 */
FLASHRAM_API bool drvSpiFlashReadSFDP(drvSpiFlash_t *d, unsigned address, void *data, unsigned size)
{
    if (size == 0)
        return true;
    if (d == NULL || data == NULL)
        return false;
    if (!d->prop.sfdp_en)
        return false;

    uint32_t critical = prvProtectAquire(d);
    halSpiFlashReadSFDP(d->hwp, address, data, size);
    prvProtectRelease(d, critical);
    return true;
}

/**
 * Get flash properties
 */
halSpiFlashProp_t *drvSpiFlashGetProp(drvSpiFlash_t *d)
{
    return &d->prop;
}
