/* Copyright (C) 2017 RDA Technologies Limited and/or its affiliates("RDA").
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

#include "boot_spi_flash.h"
#include "hal_spi_flash.h"
#include "boot_platform.h"
#include "hwregs.h"
#include "osi_api.h"
#include "osi_byte_buf.h"

#define DELAYUS(us) bootDelayUS(us)

#define SIZE_4K (4 * 1024)
#define SIZE_32K (32 * 1024)
#define SIZE_64K (64 * 1024)
#define PAGE_SIZE (256)

struct bootSpiFlash
{
    bool opened;
    uintptr_t hwp;
    uint32_t id;
    uint32_t capacity;
    uintptr_t base_address;
    uint32_t block_prohibit[8]; // 16M/64K/32bit_per_word
};

static bootSpiFlash_t gDrvSpiFlashCtx = {
    .opened = false,
    .hwp = (uintptr_t)hwp_spiFlash,
    .base_address = 0x60000000,
};

static bootSpiFlash_t gDrvSpiFlashExCtx = {
    .opened = false,
    .hwp = (uintptr_t)hwp_spiFlash1,
    .base_address = 0x70000000,
};

static inline bool prvIsWriteProhibit(bootSpiFlash_t *d, uint32_t address)
{
    unsigned block = address / SIZE_64K; // 64KB block
    return osiBitmapIsSet(d->block_prohibit, block);
}

static void prvPageProgram(bootSpiFlash_t *d, uint32_t offset, const uint8_t *data, size_t size)
{
    halSpiFlashPrepareEraseProgram(d->hwp, d->id, offset, size);
    halSpiFlashPageProgram(d->hwp, offset, data, size);
}

static void prvErase(bootSpiFlash_t *d, uint32_t offset, size_t size)
{
    halSpiFlashPrepareEraseProgram(d->hwp, d->id, offset, size);
    halSpiFlashErase(d->hwp, offset, size);
}

static void prvFlashOpen(bootSpiFlash_t *d)
{
    if (d->opened)
        return;

    halSpiFlashStatusCheck(d->hwp);
    d->id = halSpiFlashReadId(d->hwp);
    d->capacity = (1 << ((d->id >> 16) & 0xff));
    d->opened = true;
}

bootSpiFlash_t *bootSpiFlashOpen(void)
{
    bootSpiFlash_t *d = &gDrvSpiFlashCtx;
    prvFlashOpen(d);
    return d;
}

bootSpiFlash_t *bootSpiFlashExtOpen(void)
{
    bootSpiFlash_t *d = &gDrvSpiFlashExCtx;
    prvFlashOpen(d);
    return d;
}

void bootSpiFlashSetRangeWriteProhibit(bootSpiFlash_t *d, uint32_t start, uint32_t end)
{
    if (start > d->capacity || end > d->capacity)
        return;

    unsigned block_start = OSI_ALIGN_UP(start, SIZE_64K) / SIZE_64K;
    unsigned block_end = OSI_ALIGN_DOWN(end, SIZE_64K) / SIZE_64K;
    for (unsigned block = block_start; block < block_end; block++)
        osiBitmapSet(d->block_prohibit, block);
}

void bootSpiFlashClearRangeWriteProhibit(bootSpiFlash_t *d, uint32_t start, uint32_t end)
{
    if (start > d->capacity || end > d->capacity)
        return;

    unsigned block_start = OSI_ALIGN_UP(start, SIZE_64K) / SIZE_64K;
    unsigned block_end = OSI_ALIGN_DOWN(end, SIZE_64K) / SIZE_64K;
    for (unsigned block = block_start; block < block_end; block++)
        osiBitmapClear(d->block_prohibit, block);
}

uint32_t bootSpiFlashID(bootSpiFlash_t *d)
{
    return (d == NULL) ? 0 : d->id;
}

uint32_t bootSpiFlashCapacity(bootSpiFlash_t *d)
{
    return (d == NULL) ? 0 : d->capacity;
}

const void *bootSpiFlashMapAddress(bootSpiFlash_t *d, uint32_t offset)
{
    return (const void *)REG_ACCESS_ADDRESS((uintptr_t)d->base_address + offset);
}

bool bootSpiFlashOffsetValid(bootSpiFlash_t *d, uint32_t offset)
{
    return offset < d->capacity;
}

bool bootSpiFlashMapAddressValid(bootSpiFlash_t *d, const void *address)
{
    uintptr_t ptr = (uintptr_t)address;
    return (ptr >= d->base_address) && (ptr < d->base_address + d->capacity);
}

void bootSpiFlashWaitDone(bootSpiFlash_t *d)
{
    halSpiFlashWaitWipFinish(d->hwp);
    halSpiFlashFinishEraseProgram(d->hwp, d->id);
}

int bootSpiFlashWriteNoWait(bootSpiFlash_t *d, uint32_t offset, const void *data, size_t size)
{
    if (data == NULL || offset + size > d->capacity)
        return -1;

    if (prvIsWriteProhibit(d, offset))
        return -1;

    uint32_t next_page = OSI_ALIGN_DOWN(offset + PAGE_SIZE, PAGE_SIZE);
    uint32_t bsize = next_page - offset;
    if (bsize > size)
        bsize = size;

    halSpiFlashWaitWipFinish(d->hwp);
    prvPageProgram(d, offset, data, bsize);
    osiDCacheInvalidate(bootSpiFlashMapAddress(d, offset), bsize);
    return bsize;
}

int bootSpiFlashEraseNoWait(bootSpiFlash_t *d, uint32_t offset, size_t size)
{
    if (offset + size > d->capacity)
        return -1;

    if (prvIsWriteProhibit(d, offset))
        return -1;

    halSpiFlashWaitWipFinish(d->hwp);
    if (OSI_IS_ALIGNED(offset, SIZE_64K) && size >= SIZE_64K)
    {
        prvErase(d, offset, SIZE_64K);
        osiDCacheInvalidate(bootSpiFlashMapAddress(d, offset), SIZE_64K);
        return SIZE_64K;
    }
    else if (OSI_IS_ALIGNED(offset, SIZE_32K) && size >= SIZE_32K)
    {
        prvErase(d, offset, SIZE_32K);
        osiDCacheInvalidate(bootSpiFlashMapAddress(d, offset), SIZE_32K);
        return SIZE_32K;
    }
    else if (OSI_IS_ALIGNED(offset, SIZE_4K) && size >= SIZE_4K)
    {
        prvErase(d, offset, SIZE_4K);
        osiDCacheInvalidate(bootSpiFlashMapAddress(d, offset), SIZE_4K);
        return SIZE_4K;
    }
    else
    {
        return -1;
    }
}

void bootSpiFlashChipErase(bootSpiFlash_t *d)
{
    halSpiFlashWaitWipFinish(d->hwp);
    halSpiFlashPrepareEraseProgram(d->hwp, d->id, 0, d->capacity);
    halSpiFlashChipErase(d->hwp);
    halSpiFlashWaitWipFinish(d->hwp);
    halSpiFlashFinishEraseProgram(d->hwp, d->id);
    osiDCacheInvalidate(bootSpiFlashMapAddress(d, 0), d->capacity);
}

bool bootSpiFlashErase(bootSpiFlash_t *d, uint32_t offset, size_t size)
{
    while (size > 0)
    {
        int esize = bootSpiFlashEraseNoWait(d, offset, size);
        if (esize < 0)
        {
            halSpiFlashFinishEraseProgram(d->hwp, d->id);
            return false;
        }

        size -= esize;
        offset += esize;
        halSpiFlashWaitWipFinish(d->hwp);
    }
    halSpiFlashFinishEraseProgram(d->hwp, d->id);
    return true;
}

bool bootSpiFlashWrite(bootSpiFlash_t *d, uint32_t offset, const void *data, size_t size)
{
    while (size > 0)
    {
        int wsize = bootSpiFlashWriteNoWait(d, offset, data, size);
        if (wsize < 0)
        {
            halSpiFlashFinishEraseProgram(d->hwp, d->id);
            return false;
        }

        size -= wsize;
        offset += wsize;
        data = (const char *)data + wsize;
        halSpiFlashWaitWipFinish(d->hwp);
    }
    halSpiFlashFinishEraseProgram(d->hwp, d->id);
    return true;
}

void bootSpiFlashReadUniqueId(bootSpiFlash_t *d, uint8_t id[8])
{
    halSpiFlashReadUniqueId(d->hwp, id);
}
