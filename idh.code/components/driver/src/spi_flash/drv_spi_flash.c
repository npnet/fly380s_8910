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
#include "hal_chip.h"
#include "hwregs.h"
#include "osi_api.h"
#include "osi_profile.h"
#include "osi_byte_buf.h"

// After suspend command, and WIP is changed from 1 to 0, a short delay
// is needed before read flash. This delay time isn't described in
// datasheet. With experiments, 2us is enough.
#define DELAY_AFTER_SUSPEND (5) // us

// From experiments, if the time from resume to suspend is too short,
// the erase will take huge time to finish. Though in most cases there
// doesn't exist soo many interrupt, it is needed to avoid worst case.
#define MIN_ERASE_PROGRAM_TIME (100) // us

#define SIZE_4K (4 * 1024)
#define SIZE_32K (32 * 1024)
#define SIZE_64K (64 * 1024)
#define PAGE_SIZE (256)

#define MID_WINBOND 0xEF
#define MID_GD 0xC8
#define MID(id) ((id)&0xff)

#define IS_GD(id) (MID(id) == MID_GD)
#define IS_WINBOND(id) (MID(id) == MID_WINBOND)

#define STREG_WIP (1 << 0)
#define STREG_WEL (1 << 1)
#define STREG_QE (1 << 9)
#define STREG_SUS2 (1 << 10)
#define STREG_SUS1 (1 << 15)
#define STREG_LB1 (1 << 11)
#define STREG_LB2 (1 << 12)
#define STREG_LB3 (1 << 13)

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
    uintptr_t hwp;
    uint32_t id;
    uint32_t capacity;
    uintptr_t base_address;
    osiMutex_t *lock;
    uint32_t block_prohibit[8]; // 16M/64K/32bit_per_word
};

#if defined(CONFIG_SOC_8910)

#define FLASH_EXT_SUPPORT
static drvSpiFlash_t gDrvSpiFlashCtx = {
    .opened = false,
    .hwp = (uintptr_t)hwp_spiFlash,
    .base_address = 0x60000000,
};
static drvSpiFlash_t gDrvSpiFlashExCtx = {
    .opened = false,
    .hwp = (uintptr_t)hwp_spiFlash1,
    .base_address = 0x70000000,
};

static inline drvSpiFlash_t *prvFlashGetByName(uint32_t name)
{
    if (name == DRV_NAME_SPI_FLASH)
        return &gDrvSpiFlashCtx;
    if (name == DRV_NAME_SPI_FLASH_EXT)
        return &gDrvSpiFlashExCtx;
    return NULL;
}

static inline bool prvIsDataInFlash(const void *data)
{
    if (drvSpiFlashOffset(&gDrvSpiFlashCtx, data) != -1U ||
        drvSpiFlashOffset(&gDrvSpiFlashExCtx, data) != -1U)
        return true;
    return false;
}

static bool prvMasterActive(void)
{
    return false;
}
#endif

#ifdef CONFIG_SOC_8955

#define PHYADDR_IN_FLASH(address) ((OSI_KSEG1(address) & ~0xffffff) == OSI_KSEG1(CONFIG_NOR_PHY_ADDRESS))
static drvSpiFlash_t gDrvSpiFlashCtx = {
    .opened = false,
    .hwp = (uintptr_t)hwp_spiFlash,
    .base_address = CONFIG_NOR_PHY_ADDRESS,
};

static inline drvSpiFlash_t *prvFlashGetByName(uint32_t name)
{
    if (name == DRV_NAME_SPI_FLASH)
        return &gDrvSpiFlashCtx;
    return NULL;
}

static inline bool prvIsDataInFlash(const void *data)
{
    if (drvSpiFlashOffset(&gDrvSpiFlashCtx, data) != -1U)
        return true;
    return false;
}

static bool prvMasterActive(void)
{
    REG_DMA_STATUS_T dma_status = {hwp_dma->status};
    if (dma_status.b.enable && !dma_status.b.int_done_status &&
        PHYADDR_IN_FLASH(hwp_dma->src_addr))
        return true;

    for (int i = 0; i < SYS_IFC_STD_CHAN_NB; i++)
    {
        REG_SYS_IFC_STATUS_T status;
        if (REG_FIELD_GET(hwp_sysIfc->std_ch[i].status, status, enable) &&
            hwp_sysIfc->std_ch[i].tc != 0 &&
            PHYADDR_IN_FLASH(hwp_sysIfc->std_ch[i].start_addr))
            return true;
    }

    REG_GOUDA_GD_STATUS_T gouda_status = {hwp_gouda->gd_status};
    if (gouda_status.b.ia_busy || gouda_status.b.lcd_busy)
    {
        REG_GOUDA_GD_OL_INPUT_FMT_T gd_ol_input_fmt;
        REG_GOUDA_GD_VL_INPUT_FMT_T gd_vl_input_fmt;
        for (int id = 0; id < OSI_ARRAY_SIZE(hwp_gouda->overlay_layer); id++)
        {
            if (PHYADDR_IN_FLASH(hwp_gouda->overlay_layer[id].gd_ol_rgb_src) &&
                REG_FIELD_GET(hwp_gouda->overlay_layer[id].gd_ol_input_fmt, gd_ol_input_fmt, active))
                return true;
        }

        if ((PHYADDR_IN_FLASH(hwp_gouda->gd_vl_y_src) ||
             PHYADDR_IN_FLASH(hwp_gouda->gd_vl_u_src) ||
             PHYADDR_IN_FLASH(hwp_gouda->gd_vl_v_src)) &&
            REG_FIELD_GET(hwp_gouda->gd_vl_input_fmt, gd_vl_input_fmt, active))
            return true;
    }

    return false;
}
#endif

#ifdef CONFIG_SOC_8909

#define PHYADDR_IN_FLASH(address) ((OSI_KSEG1(address) & ~0xffffff) == OSI_KSEG1(CONFIG_NOR_PHY_ADDRESS))
static drvSpiFlash_t gDrvSpiFlashCtx = {
    .opened = false,
    .hwp = (uintptr_t)hwp_spiFlash,
    .base_address = CONFIG_NOR_PHY_ADDRESS,
};

static inline drvSpiFlash_t *prvFlashGetByName(uint32_t name)
{
    if (name == DRV_NAME_SPI_FLASH)
        return &gDrvSpiFlashCtx;
    return NULL;
}

static inline bool prvIsDataInFlash(const void *data)
{
    if (drvSpiFlashOffset(&gDrvSpiFlashCtx, data) != -1U)
        return true;
    return false;
}

static bool prvMasterActive(void)
{
    REG_DMA_STATUS_T dma_status = {hwp_sysDma->status};
    if (dma_status.b.enable && !dma_status.b.int_done_status &&
        PHYADDR_IN_FLASH(hwp_sysDma->src_addr))
        return true;

    for (int i = 0; i < SYS_IFC_STD_CHAN_NB; i++)
    {
        REG_SYS_IFC_STATUS_T status;
        if (REG_FIELD_GET(hwp_sysIfc->std_ch[i].status, status, enable) &&
            hwp_sysIfc->std_ch[i].tc != 0 &&
            PHYADDR_IN_FLASH(hwp_sysIfc->std_ch[i].start_addr))
            return true;
    }

    REG_GOUDA_GD_STATUS_T gouda_status = {hwp_gouda->gd_status};
    if (gouda_status.b.ia_busy || gouda_status.b.lcd_busy)
    {
        REG_GOUDA_GD_OL_INPUT_FMT_T gd_ol_input_fmt;
        REG_GOUDA_GD_VL_INPUT_FMT_T gd_vl_input_fmt;
        for (int id = 0; id < OSI_ARRAY_SIZE(hwp_gouda->overlay_layer); id++)
        {
            if (PHYADDR_IN_FLASH(hwp_gouda->overlay_layer[id].gd_ol_rgb_src) &&
                REG_FIELD_GET(hwp_gouda->overlay_layer[id].gd_ol_input_fmt, gd_ol_input_fmt, active))
                return true;
        }

        if ((PHYADDR_IN_FLASH(hwp_gouda->gd_vl_y_src) ||
             PHYADDR_IN_FLASH(hwp_gouda->gd_vl_u_src) ||
             PHYADDR_IN_FLASH(hwp_gouda->gd_vl_v_src)) &&
            REG_FIELD_GET(hwp_gouda->gd_vl_input_fmt, gd_vl_input_fmt, active))
            return true;
    }

    return false;
}
#endif

static inline bool prvIsWriteProhibit(drvSpiFlash_t *d, uint32_t address)
{
    unsigned block = address / SIZE_64K; // 64KB block
    return osiBitmapIsSet(d->block_prohibit, block);
}

static uint32_t prvWaitMasterAndLock(void)
{
    uint32_t flag = osiEnterCritical();
    while (prvMasterActive())
    {
        osiExitCritical(flag);
        osiDelayUS(5);
        flag = osiEnterCritical();
    }
    return flag;
}

static inline void prvDisableAhbRead(drvSpiFlash_t *d)
{
    REG_SPI_FLASH_SPI_CS_SIZE_T spi_cs_size;
    REG_FIELD_CHANGE1(hwp_spiFlash->spi_cs_size, spi_cs_size,
                      ahb_read_disable, 1);
}

static inline void prvEnableAhbRead(drvSpiFlash_t *d)
{
    REG_SPI_FLASH_SPI_CS_SIZE_T spi_cs_size;
    REG_FIELD_CHANGE1(hwp_spiFlash->spi_cs_size, spi_cs_size,
                      ahb_read_disable, 0);
}

static void prvPageProgram(drvSpiFlash_t *d, uint32_t offset, const uint8_t *data, size_t size)
{
    uint32_t flag = prvWaitMasterAndLock();

    osiProfileEnter(PROFCODE_FLASH_PROGRAM);
    prvDisableAhbRead(d);

    halSpiFlashPrepareEraseProgram(d->hwp, d->id, offset, size);
    halSpiFlashPageProgram(d->hwp, offset, data, size);

    // Wait minimal erase/program time
    osiDelayUS(MIN_ERASE_PROGRAM_TIME);

    for (;;)
    {
        if (halSpiFlashIsWipFinished(d->hwp))
        {
            halSpiFlashFinishEraseProgram(d->hwp, d->id);
            prvEnableAhbRead(d);

            osiProfileExit(PROFCODE_FLASH_PROGRAM);
            osiExitCritical(flag);
            return;
        }

        if (osiIrqPending())
        {
            halSpiFlashProgramSuspend(d->hwp);

            // Wait a while after WIP is changed from 1 to 0.
            halSpiFlashWaitWipFinish(d->hwp);
            osiDelayUS(DELAY_AFTER_SUSPEND);
            prvEnableAhbRead(d);

            osiExitCritical(flag);

            osiDelayUS(5); // avoid CPU can't take interrupt
            flag = prvWaitMasterAndLock();

            prvDisableAhbRead(d);
            halSpiFlashProgramResume(d->hwp);

            // Wait minimal erase/program time
            osiDelayUS(MIN_ERASE_PROGRAM_TIME);
        }
    }
}

static void prvErase(drvSpiFlash_t *d, uint32_t offset, size_t size)
{
    uint32_t flag = prvWaitMasterAndLock();

    osiProfileEnter(PROFCODE_FLASH_ERASE);
    prvDisableAhbRead(d);

    halSpiFlashPrepareEraseProgram(d->hwp, d->id, offset, size);
    halSpiFlashErase(d->hwp, offset, size);

    // Wait minimal erase/program time
    osiDelayUS(MIN_ERASE_PROGRAM_TIME);

    for (;;)
    {
        if (halSpiFlashIsWipFinished(d->hwp))
        {
            halSpiFlashFinishEraseProgram(d->hwp, d->id);
            prvEnableAhbRead(d);

            osiProfileExit(PROFCODE_FLASH_ERASE);
            osiExitCritical(flag);
            return;
        }

        if (osiIrqPending())
        {
            halSpiFlashEraseSuspend(d->hwp);

            // Wait a while after WIP is changed from 1 to 0.
            halSpiFlashWaitWipFinish(d->hwp);
            osiDelayUS(DELAY_AFTER_SUSPEND);
            prvEnableAhbRead(d);

            osiExitCritical(flag);

            osiDelayUS(5); // avoid CPU can't take interrupt
            flag = prvWaitMasterAndLock();

            prvDisableAhbRead(d);
            halSpiFlashEraseResume(d->hwp);

            // Wait minimal erase/program time
            osiDelayUS(MIN_ERASE_PROGRAM_TIME);
        }
    }
}

drvSpiFlash_t *drvSpiFlashOpen(uint32_t name)
{
    drvSpiFlash_t *d = prvFlashGetByName(name);
    if (d == NULL)
        return NULL;

    uint32_t critical = osiEnterCritical();

    if (!d->opened)
    {
        halSpiFlashStatusCheck(d->hwp);
        d->id = halSpiFlashReadId(d->hwp);
        d->capacity = (1 << ((d->id >> 16) & 0xff));
        d->opened = true;
        d->lock = osiMutexCreate();
    }

    osiExitCritical(critical);
    return d;
}

void drvSpiFlashSetRangeWriteProhibit(drvSpiFlash_t *d, uint32_t start, uint32_t end)
{
    if (start > d->capacity || end > d->capacity)
        return;

    unsigned block_start = OSI_ALIGN_UP(start, SIZE_64K) / SIZE_64K;
    unsigned block_end = OSI_ALIGN_DOWN(end, SIZE_64K) / SIZE_64K;
    for (unsigned block = block_start; block < block_end; block++)
        osiBitmapSet(d->block_prohibit, block);
}

void drvSpiFlashClearRangeWriteProhibit(drvSpiFlash_t *d, uint32_t start, uint32_t end)
{
    if (start > d->capacity || end > d->capacity)
        return;

    unsigned block_start = OSI_ALIGN_UP(start, SIZE_64K) / SIZE_64K;
    unsigned block_end = OSI_ALIGN_DOWN(end, SIZE_64K) / SIZE_64K;
    for (unsigned block = block_start; block < block_end; block++)
        osiBitmapClear(d->block_prohibit, block);
}

uint32_t drvSpiFlashGetID(drvSpiFlash_t *d)
{
    return d->id;
}

uint32_t drvSpiFlashCapacity(drvSpiFlash_t *d)
{
    return d->capacity;
}

void drvSpiFlashWriteLock(drvSpiFlash_t *d)
{
    osiMutexLock(d->lock);
}

void drvSpiFlashUnlock(drvSpiFlash_t *d)
{
    osiMutexUnlock(d->lock);
}

const void *drvSpiFlashMapAddress(drvSpiFlash_t *d, uint32_t offset)
{
    if (offset >= d->capacity)
        return NULL;
    return (const void *)REG_ACCESS_ADDRESS((uintptr_t)d->base_address + offset);
}

uint32_t drvSpiFlashOffset(drvSpiFlash_t *d, const void *address)
{
    uintptr_t p = (uintptr_t)address;
    if (p >= d->base_address && p < d->base_address + d->capacity)
        return p - d->base_address;
    return -1U;
}

bool drvSpiFlashWrite(drvSpiFlash_t *d, uint32_t offset, const void *data, size_t size)
{
    uint32_t o_offset = offset;
    size_t o_size = size;

    if (data == NULL || offset + size > d->capacity)
        return false;

    // input data can't located in FLASH
    if (prvIsDataInFlash(data))
        return false;

    const uint8_t *p = (const uint8_t *)data;
    bool result = true;

    osiMutexLock(d->lock); // exclusive write
    while (size > 0)
    {
        uint32_t next_page = OSI_ALIGN_DOWN(offset + PAGE_SIZE, PAGE_SIZE);
        uint32_t bsize = next_page - offset;
        if (bsize > size)
            bsize = size;

        if (prvIsWriteProhibit(d, offset))
        {
            result = false;
            break;
        }

        prvPageProgram(d, offset, p, bsize);
        p += bsize;
        offset += bsize;
        size -= bsize;
    }
    osiMutexUnlock(d->lock);

    osiDCacheInvalidate(drvSpiFlashMapAddress(d, o_offset), o_size);
    return result;
}

bool drvSpiFlashErase(drvSpiFlash_t *d, uint32_t offset, size_t size)
{
    uint32_t o_offset = offset;
    size_t o_size = size;

    if (offset + size > d->capacity ||
        !OSI_IS_ALIGNED(offset, SIZE_4K) ||
        !OSI_IS_ALIGNED(size, SIZE_4K))
        return false;

    bool result = true;

    osiMutexLock(d->lock); // exclusive write
    while (size > 0)
    {
        if (prvIsWriteProhibit(d, offset))
        {
            result = false;
            break;
        }

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
    osiMutexUnlock(d->lock);

    osiDCacheInvalidate(drvSpiFlashMapAddress(d, o_offset), o_size);
    return result;
}

void drvSpiFlashChipErase(drvSpiFlash_t *d)
{
    osiMutexLock(d->lock);                  // exclusive write
    uint32_t critical = osiEnterCritical(); // avoid read, and there are no suspend

    halSpiFlashPrepareEraseProgram(d->hwp, d->id, 0, d->capacity);
    halSpiFlashChipErase(d->hwp);
    halSpiFlashWaitWipFinish(d->hwp);
    halSpiFlashFinishEraseProgram(d->hwp, d->id);

    osiExitCritical(critical);
    osiMutexUnlock(d->lock);
}

uint16_t drvSpiFlashReadStatus(drvSpiFlash_t *d)
{
    uint32_t critical = osiEnterCritical();
    uint16_t status = halSpiFlashReadStatus12(d->hwp);
    osiExitCritical(critical);
    return status;
}

void drvSpiFlashWriteStatus(drvSpiFlash_t *d, uint16_t status)
{
    osiMutexLock(d->lock);                  // exclusive write
    uint32_t critical = osiEnterCritical(); // avoid read, and there are no suspend

    halSpiFlashWriteEnable(d->hwp);
    halSpiFlashWriteStatus12(d->hwp, status);
    halSpiFlashWaitWipFinish(d->hwp);

    osiExitCritical(critical);
    osiMutexUnlock(d->lock);
}

void drvSpiFlashWriteVolatileStatus(drvSpiFlash_t *d, uint16_t status)
{
    osiMutexLock(d->lock);                  // exclusive write
    uint32_t critical = osiEnterCritical(); // avoid read

    halSpiFlashWriteVolatileStatusEnable(d->hwp);
    halSpiFlashWriteStatus12(d->hwp, status);

    osiExitCritical(critical);
    osiMutexUnlock(d->lock);
}

void drvSpiFlashDeepPowerDown(drvSpiFlash_t *d)
{
    // it should be called with interrupt disabled
    halSpiFlashDeepPowerDown(d->hwp);
}

void drvSpiFlashReleaseDeepPowerDown(drvSpiFlash_t *d)
{
    // it should be called with interrupt disabled
    halSpiFlashReleaseDeepPowerDown(d->hwp);
}

void drvSpiFlashReadUniqueId(drvSpiFlash_t *d, uint8_t id[8])
{
    uint32_t critical = osiEnterCritical();
    halSpiFlashReadUniqueId(d->hwp, id);
    osiExitCritical(critical);
}

uint16_t drvSpiFlashReadCpId(drvSpiFlash_t *d)
{
    if (!IS_GD(d->id))
        return 0;

    uint32_t critical = osiEnterCritical();
    uint16_t id = halSpiFlashReadCpId(d->hwp);
    osiExitCritical(critical);
    return id;
}

bool drvSpiFlashReadSecurityRegister(drvSpiFlash_t *d, uint8_t num, uint16_t address, void *data, uint32_t size)
{
    if (data == NULL || size == 0)
        return false;
    if (num < 1 || num > 3)
        return false;

    uint32_t saddr = (num << 12) | address;
    while (size > 0)
    {
        uint32_t read_size = OSI_MIN(uint32_t, size, 4);

        uint32_t critical = osiEnterCritical();
        halSpiFlashReadSecurityRegister(d->hwp, saddr, data, read_size);
        osiExitCritical(critical);

        saddr += read_size;
        size -= read_size;
        data = (char *)data + read_size;
    }
    return true;
}

bool drvSpiFlashProgramSecurityRegister(drvSpiFlash_t *d, uint8_t num, uint16_t address, const void *data, uint32_t size)
{
    if (data == NULL || size == 0)
        return false;
    if (num < 1 || num > 3)
        return false;
    if (prvIsDataInFlash(data))
        return false;

    osiMutexLock(d->lock);                  // exclusive write
    uint32_t critical = osiEnterCritical(); // avoid read, and there are no suspend

    uint32_t saddr = (num << 12) | address;
    while (size > 0)
    {
        unsigned block_size = OSI_MIN(unsigned, size, PAGE_SIZE);

        halSpiFlashWriteEnable(d->hwp);
        halSpiFlashProgramSecurityRegister(d->hwp, saddr, data, block_size);
        halSpiFlashWaitWipFinish(d->hwp);

        saddr += block_size;
        data = (char *)data + block_size;
        size -= block_size;
    }

    osiExitCritical(critical);
    osiMutexUnlock(d->lock);
    return true;
}

bool drvSpiFlashEraseSecurityRegister(drvSpiFlash_t *d, uint8_t num)
{
    if (num < 1 || num > 3)
        return false;

    osiMutexLock(d->lock);                  // exclusive write
    uint32_t critical = osiEnterCritical(); // avoid read, and there are no suspend

    halSpiFlashWriteEnable(d->hwp);
    halSpiFlashEraseSecurityRegister(d->hwp, num << 12);
    halSpiFlashWaitWipFinish(d->hwp);

    osiExitCritical(critical);
    osiMutexUnlock(d->lock);
    return true;
}

bool drvSpiFlashLockSecurityRegister(drvSpiFlash_t *d, uint8_t num)
{
    if (num < 1 || num > 3)
        return false;

    osiMutexLock(d->lock);                  // exclusive write
    uint32_t critical = osiEnterCritical(); // avoid read, and there are no suspend

    uint16_t status = halSpiFlashReadStatus12(d->hwp);
    if (num == 1)
        status |= STREG_LB1;
    else if (num == 2)
        status |= STREG_LB2;
    else
        status |= STREG_LB3;

    halSpiFlashWriteEnable(d->hwp);
    halSpiFlashWriteStatus12(d->hwp, status);
    halSpiFlashWaitWipFinish(d->hwp);

    osiExitCritical(critical);
    osiMutexUnlock(d->lock);
    return true;
}
