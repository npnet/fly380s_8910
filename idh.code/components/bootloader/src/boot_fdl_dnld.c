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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('F', 'D', 'L', 'D')
// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "boot_fdl.h"
#include "boot_platform.h"
#include "boot_spi_flash.h"
#include "boot_adi_bus.h"
#include "hal_chip.h"
#include "boot_secure.h"
#include "boot_bsl_cmd.h"
#include "hal_config.h"
#include "osi_log.h"
#include "osi_api.h"
#include "drv_names.h"
#include "fs_mount.h"
#include "vfs.h"
#include "nvm.h"
#include "calclib/crc32.h"
#include "cpio_parser.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// Work memory size in context
#define WORKMEM_SIZE 0x20000
#define PACKAGE_FILE_MAX_SIZE 0x10000

// Logic addresses, which should match the value used in pac file
#define ERASE_RUNNING_NV_LOGIC_ADDRESS 0xFE000001
#define PHASECHECK_LOGIC_ADDRESS 0xFE000002
#define FIXNV_LOGIC_ADDRESS 0xFE000003
#define PRE_PACK_FILE_LOGIC_ADDRESS 0xFE000004
#define DEL_APPIMG_LOGIC_ADDRESS 0xFE000005
#define FMT_FLASH_LOGIC_ADDRESS 0xFE000006

#define FLASH_ERASE_ALL 0x00000000
#define FLASH_ERASE_ALL_SIZE 0xFFFFFFFF

#define FLASH_PAGE_SIZE (256)
#define FLASH_SECTOR_SIZE (0x1000)

#define FDL_SIZE_4K (0x1000)

// 8910 encrypt
#define BOOT_SIGN_SIZE (0xbd40)
#define ENCRYPT_OFF 0x1000
#define ENCRYPT_LEN 0x400

#define DELAY_POWEROFF_TIMEOUT (2000)

#define ERR_REP_RETURN(fdl, err) OSI_DO_WHILE0(fdlEngineSendRespNoData(fdl, err); return;)
#ifdef CONFIG_BOARD_WITH_EXT_FLASH
#define IS_FLASH_ADDRESS(address) (HAL_FLASH_DEVICE_NAME(address) == DRV_NAME_SPI_FLASH || HAL_FLASH_DEVICE_NAME(address) == DRV_NAME_SPI_FLASH_EXT)
#else
#define IS_FLASH_ADDRESS(address) (HAL_FLASH_DEVICE_NAME(address) == DRV_NAME_SPI_FLASH)
#endif

static_assert(WORKMEM_SIZE >= CONFIG_NVBIN_FIXED_SIZE, "WORKMEM_SIZE is too small");
static_assert(WORKMEM_SIZE >= sizeof(phaseCheckHead_t), "WORKMEM_SIZE is too small");

typedef struct
{
    cpioStream_t *cpio;
    fdlDataDnld_t dnld;
    bool fs_mounted;
    bool running_cleared;
    bool fixnv_read;
    uint32_t init_nvbin_crc;
    bool secure_boot_enable;
    bool delay_poweroff;
    unsigned device_type;
    fdlChannel_t *channel;
    unsigned max_packet_len;
    osiElapsedTimer_t connect_timer;
    char work_mem[WORKMEM_SIZE];
} fdlContext_t;

/**
 * Calculate nvbin CRC, return 0 on read fail. It will reuse \p work_mem.
 */
static uint32_t prvGetNvBinCrc(fdlContext_t *d)
{
    int nvbin_size = nvmReadFixedBin(NULL, 0);
    if (nvbin_size <= 0 || nvbin_size > CONFIG_NVBIN_FIXED_SIZE)
        return 0;

    char *nvbin = &d->work_mem[0];
    memset(nvbin, 0xff, CONFIG_NVBIN_FIXED_SIZE);
    nvmReadFixedBin(nvbin, CONFIG_NVBIN_FIXED_SIZE);
    return crc32Calc(nvbin, CONFIG_NVBIN_FIXED_SIZE);
}

#ifdef CONFIG_SOC_8910
/**
 * Check whether running should be cleared. It will reuse \p work_mem.
 * It is only called after modem image is changed.
 */
static void prvCheckClearRunning(fdlContext_t *d)
{
    if (d->running_cleared)
        return;

    int nvbin_size = nvmReadFixedBin(NULL, 0);
    if (nvbin_size > 0 && nvbin_size <= CONFIG_NVBIN_FIXED_SIZE)
    {
        char *nvbin = &d->work_mem[0];
        memset(nvbin, 0xff, CONFIG_NVBIN_FIXED_SIZE);
        nvmReadFixedBin(nvbin, CONFIG_NVBIN_FIXED_SIZE);

        // do nothing if the nvbin CRC matches
        if (crc32Calc(nvbin, CONFIG_NVBIN_FIXED_SIZE) == d->init_nvbin_crc)
            return;
    }

    nvmClearRunning();
    d->running_cleared = true;
}
#endif

/**
 * Mount file system and initialize nvm. Optionally, it will format file
 * system on mount fail.
 */
static bool prvFsNvmInit(fdlContext_t *d, bool format_on_fail)
{
    if (d->fs_mounted)
        return true;

    d->fs_mounted = format_on_fail ? fsMountWithFormatAll() : fsMountAll();
    if (!d->fs_mounted)
    {
        OSI_LOGE(0, "FDL: mount file system failed format_on_fail/%d", format_on_fail);
        return false;
    }

    // Initialize nvm before nv access.
    nvmInit();

#ifdef CONFIG_SOC_8910
    // HACK: ResearchDownload needs that nvid 2 must exist, and
    // previously this nv isn't added in 8910. Due to this nv isn't actually
    // used. So, at the beginning of FDL2, this will be written to make
    // ResearchDownload happy.
    if (nvmReadItem(NVID_CALIB_PARAM, NULL, 0) <= 0)
    {
        // Refer to CMakeLists.txt about these 2 external symbols.
        extern const unsigned gDefCalibParam[];
        extern const unsigned gDefCalibParamSize[];
        nvmWriteItem(NVID_CALIB_PARAM, gDefCalibParam, (unsigned)gDefCalibParamSize);
    }

    // HACK: GSM calibration data size is changed from 7706 to 8192. The
    // size of existed GSM calibration data may be 7706. And it is needed
    // to pad the existed GSM calibration data to 8192, to make NVEditor
    // work, and make nv backup work.
    int calib_gsm_size = nvmReadFixedItem(0x26d, NULL, 0);
    if (calib_gsm_size > 0 && calib_gsm_size < 8192)
    {
        char *calib_gsm_bin = &d->work_mem[0];
        memset(calib_gsm_bin, 0, 8192);

        nvmReadFixedItem(0x26d, calib_gsm_bin, 8192);
        nvmWriteFixedItem(0x26d, calib_gsm_bin, 8192);
    }
#endif

    d->init_nvbin_crc = prvGetNvBinCrc(d);
    d->running_cleared = false;
    return true;
}

/**
 * Initialize file system, **without** format on fail.
 */
static bool prvFsInit(fdlContext_t *d)
{
    return prvFsNvmInit(d, false);
}

/**
 * Initialize file system, **with** format on fail.
 */
static bool prvFsInitWithFormat(fdlContext_t *d)
{
    return prvFsNvmInit(d, true);
}

/**
 * BSL_CMD_CONNECT, the timestamp is recorded.
 */
static void prvConnect(fdlEngine_t *fdl, fdlContext_t *d)
{
    OSI_LOGI(0, "FDL: connect");

    osiElapsedTimerStart(&d->connect_timer);
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

/**
 * Flash download start. The common information start/total/received has
 * already initialized. Return response error code.
 */
static int prvFlashStart(fdlContext_t *d)
{
    fdlDataDnld_t *dn = &d->dnld;

    // Though it doesn't look reasonable, unaligned start address is supported.
    // In this case, the write address is not the same as erase address.
    dn->write_address = dn->start_address;
    dn->erase_address = OSI_ALIGN_DOWN(dn->start_address, FLASH_SECTOR_SIZE);

    unsigned faddress = HAL_FLASH_OFFSET(dn->erase_address);
    unsigned fend = HAL_FLASH_OFFSET(OSI_ALIGN_UP(dn->start_address + dn->total_size, FLASH_SECTOR_SIZE));
    int esize = bootSpiFlashEraseNoWait(dn->flash, faddress, fend - faddress);
    OSI_LOGV(0, "FDL2 flash erase no wait 0x%x/0x%x/%d", faddress, fend - faddress, esize);

    if (esize < 0)
    {
        dn->flash_stage = FLASH_STAGE_NONE;
        OSI_LOGE(0, "FDL: flash start failed 0x%x/0x%x", dn->start_address, dn->total_size);
        return -BSL_REP_DOWN_DEST_ERROR;
    }

    dn->erase_address += esize;
    dn->flash_stage = FLASH_STAGE_ERASE;
    dn->data_verify = crc32Init();
    return 0;
}

/**
 * Flash polling, called in FDL polling, midst and end. Don't access
 * \p dnld.received_size, rather the parameter should be used.
 */
static void prvFlashPolling(fdlContext_t *d, unsigned received)
{
    fdlDataDnld_t *dn = &d->dnld;

    if (dn->flash_stage == FLASH_STAGE_NONE ||
        dn->flash_stage == FLASH_STAGE_FINISH)
        return;

    if (dn->flash_stage == FLASH_STAGE_ERASE)
    {
        if (!bootSpiFlashIsDone(dn->flash))
            return;

        dn->flash_stage = FLASH_STAGE_WAIT_DATA;
    }
    else if (dn->flash_stage == FLASH_STAGE_WAIT_DATA)
    {
        int avail = received - (dn->write_address - dn->start_address);
        if (avail < FLASH_PAGE_SIZE || dn->write_address >= dn->erase_address)
            return;

        unsigned waddress = HAL_FLASH_OFFSET(dn->write_address);
        void *wdata = &d->work_mem[dn->write_address % WORKMEM_SIZE];
        int wsize = bootSpiFlashWriteNoWait(dn->flash, waddress, wdata, FLASH_PAGE_SIZE);
        OSI_LOGV(0, "FDL2 flash write no wait 0x%x/0x%x", waddress, wsize);

        dn->write_address += wsize;
        dn->flash_stage = FLASH_STAGE_WRITE;
    }
    else if (dn->flash_stage == FLASH_STAGE_WRITE)
    {
        if (!bootSpiFlashIsDone(dn->flash))
            return;

        if (dn->write_address < dn->erase_address)
        {
            dn->flash_stage = FLASH_STAGE_WAIT_DATA;
            return;
        }

        unsigned faddress = HAL_FLASH_OFFSET(dn->erase_address);
        unsigned fend = HAL_FLASH_OFFSET(OSI_ALIGN_UP(dn->start_address + dn->total_size, FLASH_SECTOR_SIZE));
        if (faddress >= fend)
        {
            dn->flash_stage = FLASH_STAGE_FINISH;
            return;
        }

        int esize = bootSpiFlashEraseNoWait(dn->flash, faddress, fend - faddress);
        OSI_LOGV(0, "FDL2 flash erase no wait 0x%x/0x%x/%d", faddress, fend - faddress, esize);
        dn->erase_address += esize;
        dn->flash_stage = FLASH_STAGE_ERASE;
    }
}

/**
 * Flash midst
 */
static void prvFlashMidst(fdlContext_t *d, const void *data, unsigned size)
{
    fdlDataDnld_t *dn = &d->dnld;

    dn->data_verify = crc32Update(dn->data_verify, data, size);

    // work_mem is reused as ring buffer. The index is address rather than
    // size. So, page write can always use linear buffer.
    unsigned mem_pos = (dn->start_address + dn->received_size) % WORKMEM_SIZE;
    unsigned tail = WORKMEM_SIZE - mem_pos;
    if (tail >= size)
    {
        memcpy(&d->work_mem[mem_pos], data, size);
    }
    else
    {
        memcpy(&d->work_mem[mem_pos], data, tail);
        memcpy(&d->work_mem[0], (const char *)data + tail, size - tail);
    }

    // Polling to make sure there are room for the next packet data.
    unsigned received = dn->received_size + size;
    unsigned received_address = dn->start_address + dn->received_size + size;
    while (received_address - dn->write_address > WORKMEM_SIZE - d->max_packet_len)
        prvFlashPolling(d, received);
}

/**
 * Flash end, write the remaining data and check crc.
 */
static int prvFlashEnd(fdlContext_t *d)
{
    fdlDataDnld_t *dn = &d->dnld;

    // size may be unaligned. So, polling to make sure the remaining data
    // less than a page.
    unsigned received_address = dn->start_address + dn->received_size;
    while (dn->write_address + FLASH_PAGE_SIZE <= received_address)
        prvFlashPolling(d, dn->received_size);

    // Write the remaining data, if exists.
    bootSpiFlashWaitDone(dn->flash);
    int remained = received_address - dn->write_address;
    if (remained > 0)
    {
        // It is possible that the remaining are located at unerased sector
        unsigned faddress = HAL_FLASH_OFFSET(dn->erase_address);
        unsigned fend = HAL_FLASH_OFFSET(OSI_ALIGN_UP(dn->start_address + dn->total_size, FLASH_SECTOR_SIZE));
        if (faddress < fend)
        {
            int esize = bootSpiFlashErase(dn->flash, faddress, fend - faddress);
            OSI_LOGV(0, "FDL2 flash erase 0x%x/0x%x/%d", faddress, fend - faddress, esize);
            dn->erase_address += esize;
        }

        unsigned waddress = HAL_FLASH_OFFSET(dn->write_address);
        void *wdata = &d->work_mem[dn->write_address % WORKMEM_SIZE];
        bootSpiFlashWrite(dn->flash, waddress, wdata, remained);
        OSI_LOGV(0, "FDL2 flash write 0x%x/0x%x", waddress, remained);
    }

    unsigned fstart = HAL_FLASH_OFFSET(dn->start_address);
    const void *fptr = bootSpiFlashMapAddress(dn->flash, fstart);
    unsigned fverify = crc32Calc(fptr, dn->total_size);

    dn->flash_stage = FLASH_STAGE_NONE;
    dn->flash = NULL;

    if (fverify != dn->data_verify)
    {
        OSI_LOGE(0, "FDL2 flash crc error, start/0x%x size/0x%x crc/0x%x calc/0x%x",
                 dn->start_address, dn->total_size, dn->data_verify, fverify);
        return -BSL_REP_VERIFY_ERROR;
    }
    return 0;
}

/**
 * Write one file from cpio.
 */
static void prvPackageFile(cpioFile_t *f)
{
    char name[VFS_PATH_MAX + 1] = "/";
    bool isdir = f->mode & S_IFDIR;
    memcpy(name + 1, f->name, strlen(f->name) + 1);

    OSI_LOGXD(OSI_LOGPAR_SII, 0, "package file %s (%d/%u)", name, isdir, f->data_size);
    if (isdir)
    {
        vfs_mkpath(name, 0);
    }
    else
    {
        vfs_mkfilepath(name, 0);
        vfs_file_write(name, f->data, f->data_size);
    }
}

/**
 * BSL_CMD_START_DATA
 * When file system access is needed, it should be chcked here.
 */
static void prvDataStart(fdlEngine_t *fdl, fdlPacket_t *pkt, fdlContext_t *d)
{
    int result = 0;
    uint32_t *ptr = (uint32_t *)pkt->content;
    uint32_t start_addr = OSI_FROM_BE32(*ptr++);
    uint32_t file_size = OSI_FROM_BE32(*ptr++);

    OSI_LOGI(0, "FDL: data start, start_addr/0x%x, size/0x%x", start_addr, file_size);

    d->dnld.start_address = start_addr;
    d->dnld.total_size = file_size;
    d->dnld.received_size = 0;

    if (start_addr == PHASECHECK_LOGIC_ADDRESS)
    {
        if (file_size != sizeof(phaseCheckHead_t))
        {
            OSI_LOGE(0, "FDL: invalid phasecheck data size %d", file_size);
            ERR_REP_RETURN(fdl, BSL_REP_DOWN_SIZE_ERROR);
        }

        if (!prvFsInitWithFormat(d)) // MAY FORMAT!!
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
    }
    else if (start_addr == FIXNV_LOGIC_ADDRESS)
    {
        if (file_size > CONFIG_NVBIN_FIXED_SIZE)
        {
            OSI_LOGE(0, "FDL: invalid fixnv data size %d", file_size);
            ERR_REP_RETURN(fdl, BSL_REP_DOWN_SIZE_ERROR);
        }

        if (!prvFsInitWithFormat(d)) // MAY FORMAT!!
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
    }
    else if (start_addr == PRE_PACK_FILE_LOGIC_ADDRESS)
    {
        if (!prvFsInit(d))
            ERR_REP_RETURN(fdl, BSL_REP_INCOMPATIBLE_PARTITION);

        cpioStreamCfg_t cfg = {.file_size_max = PACKAGE_FILE_MAX_SIZE, .file_path_max = VFS_PATH_MAX};
        d->cpio = cpioStreamCreate(&cfg);
        if (d->cpio == NULL)
            ERR_REP_RETURN(fdl, BSL_PHONE_NOT_ENOUGH_MEMORY);
    }
    else if (IS_FLASH_ADDRESS(start_addr))
    {
        bootSpiFlash_t *flash = bootSpiFlashOpen(HAL_FLASH_DEVICE_NAME(start_addr));
        if (flash == NULL)
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);

        fdlDataDnld_t *dn = &d->dnld;
        dn->flash = flash;
        if ((result = prvFlashStart(d)) < 0)
            ERR_REP_RETURN(fdl, -result);
    }
    else
    {
        OSI_LOGE(0, "FDL: invalid data start address 0x%x", start_addr);
        fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_DEST_ERROR);
        return;
    }

    d->dnld.stage = SYS_STAGE_START;
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

/**
 * BSL_CMD_MIDST_DATA
 */
static void prvDataMidst(fdlEngine_t *fdl, fdlPacket_t *pkt, fdlContext_t *d)
{
    uint16_t data_len = pkt->size;
    uint32_t start_addr = d->dnld.start_address;

    OSI_LOGD(0, "FDL: data midst, start_addr/0x%x, size/0x%x, received/0x%x, len/0x%x, stage/0x%x",
             d->dnld.start_address, d->dnld.total_size, d->dnld.received_size,
             data_len, d->dnld.stage);

    if ((d->dnld.stage != SYS_STAGE_START) && (d->dnld.stage != SYS_STAGE_GATHER))
    {
        d->dnld.flash_stage = FLASH_STAGE_NONE;
        d->dnld.flash = NULL;
        ERR_REP_RETURN(fdl, BSL_REP_DOWN_NOT_START);
    }

    if (d->dnld.received_size + data_len > d->dnld.total_size)
    {
        d->dnld.flash_stage = FLASH_STAGE_NONE;
        d->dnld.flash = NULL;
        ERR_REP_RETURN(fdl, BSL_REP_DOWN_SIZE_ERROR);
    }

    if (start_addr == PRE_PACK_FILE_LOGIC_ADDRESS)
    {
        cpioStreamPushData(d->cpio, pkt->content, data_len);
        cpioFile_t *f;
        while ((f = cpioStreamPopFile(d->cpio)))
        {
            prvPackageFile(f);
            cpioStreamDestroyFile(d->cpio, f);
        }
    }
    else if (d->dnld.flash_stage != FLASH_STAGE_NONE)
    {
        prvFlashMidst(d, pkt->content, data_len);
    }
    else if (d->dnld.received_size + data_len > WORKMEM_SIZE)
    {
        ERR_REP_RETURN(fdl, BSL_REP_DOWN_SIZE_ERROR);
    }
    else
    {
        memcpy(&d->work_mem[0] + d->dnld.received_size, pkt->content, data_len);
    }

    d->dnld.received_size += data_len;
    d->dnld.stage = SYS_STAGE_GATHER;
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

/**
 * BSL_CMD_END_DATA
 */
static void prvDataEnd(fdlEngine_t *fdl, fdlContext_t *d)
{
    int result = 0;
    uint32_t start_addr = d->dnld.start_address;

    OSI_LOGI(0, "FDL: data end, start_addr/0x%x, size/0x%x, received/0x%x, stage/0x%x",
             d->dnld.start_address, d->dnld.total_size, d->dnld.received_size,
             d->dnld.stage);

    if ((d->dnld.stage != SYS_STAGE_START) && (d->dnld.stage != SYS_STAGE_GATHER))
    {
        d->dnld.flash_stage = FLASH_STAGE_NONE;
        d->dnld.flash = NULL;
        ERR_REP_RETURN(fdl, BSL_REP_DOWN_NOT_START);
    }

    if (d->dnld.received_size != d->dnld.total_size)
    {
        d->dnld.flash_stage = FLASH_STAGE_NONE;
        d->dnld.flash = NULL;
        ERR_REP_RETURN(fdl, BSL_REP_DOWN_SIZE_ERROR);
    }

    if (start_addr == PHASECHECK_LOGIC_ADDRESS)
    {
        phaseCheckHead_t *phase_check = (phaseCheckHead_t *)&d->work_mem[0];
        if (!nvmWritePhaseCheck(phase_check))
        {
            OSI_LOGE(0, "FDL: write phase check failed");
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
        }
    }
    else if (start_addr == FIXNV_LOGIC_ADDRESS)
    {
        char *nvbin = &d->work_mem[0];
        if (nvmWriteFixedBin(nvbin, d->dnld.received_size) < 0)
        {
            OSI_LOGE(0, "FDL: write nv bin failed");
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
        }
    }
    else if (start_addr == PRE_PACK_FILE_LOGIC_ADDRESS)
    {
        cpioStreamDestroy(d->cpio);
        d->cpio = NULL;
    }
    else if (d->dnld.flash_stage != FLASH_STAGE_NONE)
    {
        if ((result = prvFlashEnd(d)) < 0)
            ERR_REP_RETURN(fdl, -result);

#ifdef CONFIG_SOC_8910
        if (start_addr == CONFIG_FS_MODEM_FLASH_ADDRESS)
        {
            // Modem file system is written by image. So, the block device and
            // file system is changed. And it is needed to umount/mount the
            // block device and file system. The easiest method is to
            // umount/mount all. It will waste some time, and the dynamic memory
            // may be considered in memory tight system.

            fsUmountAll();

            if (d->fs_mounted)
            {
                d->fs_mounted = false; // clear the flag
                if (!prvFsInit(d))
                {
                    OSI_LOGE(0, "FDL2: failed to remount after write modem image");
                    ERR_REP_RETURN(fdl, BSL_REP_VERIFY_ERROR);
                }

                // check clear running after modem image is written
                prvCheckClearRunning(d);
            }
        }
        else if (start_addr == CONFIG_BOOT_FLASH_ADDRESS)
        {
            if (checkSecureBootState())
            {
                if (checkDataSign((void *)start_addr, BOOT_SIGN_SIZE) != 0)
                {
                    OSI_LOGE(0, "FDL: bootloader secure boot verify fail");
                    ERR_REP_RETURN(fdl, BSL_REP_VERIFY_ERROR);
                }

                // reuse the working memory
                char *pbuf = &d->work_mem[0];
                memcpy(pbuf, (void *)(start_addr + ENCRYPT_OFF), FDL_SIZE_4K);
                antiCloneEncryption(pbuf, ENCRYPT_LEN);

                bootSpiFlash_t *flash = bootSpiFlashOpen(DRV_NAME_SPI_FLASH);
                unsigned flash_offset = HAL_FLASH_OFFSET(start_addr);
                if (!bootSpiFlashErase(flash, flash_offset + ENCRYPT_OFF, FDL_SIZE_4K))
                    ERR_REP_RETURN(fdl, BSL_REP_VERIFY_ERROR);

                if (!bootSpiFlashWrite(flash, flash_offset + ENCRYPT_OFF, pbuf, FDL_SIZE_4K))
                    ERR_REP_RETURN(fdl, BSL_REP_VERIFY_ERROR);
            }
        }
#endif
#ifdef CONFIG_SOC_8811
        else if (start_addr == CONFIG_BOOT1_FLASH_ADDRESS || start_addr == CONFIG_BOOT2_FLASH_ADDRESS)
        {
            if (d->secure_boot_enable)
            {
                const simageHeader_t *header = (const simageHeader_t *)start_addr;
                // TODO, security check first
                uint32_t version = bootSimageSecurityVersion(header);
                if (!bootUpdateVersion(version))
                {
                    OSI_LOGE(0, "FDL: update security version fail %d", version);
                    ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
                }
            }
        }
#endif
    }

    d->dnld.stage = SYS_STAGE_END;
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

/**
 * BSL_CMD_NORMAL_RESET
 */
static void prvResetNormal(fdlEngine_t *fdl, fdlContext_t *d)
{
    OSI_LOGI(0, "FDL: reset to normal");

    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
    osiDelayUS(2000);
    bootReset(BOOT_RESET_NORMAL);
}

/**
 * Polling for delayed poweroff
 */
static void prvPowerOffPolling(fdlContext_t *d)
{
    if (!d->delay_poweroff)
        return;

    if (BOOT_DNLD_FROM_UART(d->device_type))
    {
        unsigned ms = osiElapsedTime(&d->connect_timer);
        if (ms > DELAY_POWEROFF_TIMEOUT)
        {
            OSI_LOGI(0, "FDL: poweroff timeout", ms);
            bootPowerOff();

            // ideally, we shouldn't come here
            OSI_LOGE(0, "FDL: poweroff failed");
            d->delay_poweroff = false;
        }
    }

    if (BOOT_DNLD_FROM_USERIAL(d->device_type))
    {
        if (!d->channel->connected(d->channel))
        {
            OSI_LOGI(0, "FDL: device not connected, poweroff");
            bootPowerOff();

            // ideally, we shouldn't come here
            OSI_LOGE(0, "FDL: poweroff failed");
            d->delay_poweroff = false;
        }
    }
}

/**
 * BSL_CMD_POWER_OFF
 */
static void prvPowerOff(fdlEngine_t *fdl, fdlContext_t *d)
{
    OSI_LOGI(0, "FDL: poweroff");

    d->delay_poweroff = true;
    osiElapsedTimerStart(&d->connect_timer);
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

/**
 * BSL_CMD_READ_FLASH
 */
static void prvReadFlash(fdlEngine_t *fdl, fdlPacket_t *pkt, fdlContext_t *d)
{
    uint32_t *ptr = (uint32_t *)pkt->content;
    uint32_t addr = OSI_FROM_BE32(*ptr++);
    uint32_t size = OSI_FROM_BE32(*ptr++);
    uint32_t offset = OSI_FROM_BE32(*ptr++); // not all address has this parameter

    if (offset == 0)
        OSI_LOGI(0, "FDL: read flash, addr/0x%x, size/0x%x, offset/0x%x", addr, size, offset);
    else
        OSI_LOGD(0, "FDL: read flash, addr/0x%x, size/0x%x, offset/0x%x", addr, size, offset);

    if (addr == PHASECHECK_LOGIC_ADDRESS)
    {
        if (!prvFsInit(d))
            ERR_REP_RETURN(fdl, BSL_REP_INCOMPATIBLE_PARTITION);

        // reuse the working memory
        phaseCheckHead_t *phase_check = (phaseCheckHead_t *)&d->work_mem[0];
        memset(phase_check, 0, sizeof(phaseCheckHead_t));
        if (!nvmReadPhasecheck(phase_check))
        {
            OSI_LOGE(0, "FDL: read phasecheck failed");
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
        }

        fdlEngineSendRespData(fdl, BSL_REP_READ_FLASH, phase_check, sizeof(phaseCheckHead_t));
    }
    else if (addr == FIXNV_LOGIC_ADDRESS)
    {
        if (!prvFsInit(d))
            ERR_REP_RETURN(fdl, BSL_REP_INCOMPATIBLE_PARTITION);

        // Assuming the read will be started with offset 0, and continuous
        char *nvbin = &d->work_mem[0];
        if (offset == 0)
        {
            memset(nvbin, 0xff, CONFIG_NVBIN_FIXED_SIZE);
            if (nvmReadFixedBin(nvbin, CONFIG_NVBIN_FIXED_SIZE) < 0)
                ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
        }

        if (offset >= CONFIG_NVBIN_FIXED_SIZE)
            ERR_REP_RETURN(fdl, BSL_REP_INVALID_CMD);

        // Set flag to record that fixnv is read. This is regarded as nv backup.
        // In this case, "erase all" will restore phasecheck.
        d->fixnv_read = true;

        unsigned send_size = OSI_MIN(unsigned, CONFIG_NVBIN_FIXED_SIZE - offset, size);
        fdlEngineSendRespData(fdl, BSL_REP_READ_FLASH, nvbin + offset, send_size);
    }
    else if (IS_FLASH_ADDRESS(addr))
    {
        bootSpiFlash_t *flash = bootSpiFlashOpen(HAL_FLASH_DEVICE_NAME(addr));
        if (flash == NULL)
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);

        // valid flash offset not includes the end address
        if (!bootSpiFlashMapAddressValid(flash, (void *)(addr + size - 1)))
            ERR_REP_RETURN(fdl, BSL_REP_INVALID_CMD);

        uint32_t flash_offset = HAL_FLASH_OFFSET(addr);
        const void *fdata = bootSpiFlashMapAddress(flash, flash_offset);
        fdlEngineSendRespData(fdl, BSL_REP_READ_FLASH, fdata, size);
    }
    else
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_INVALID_CMD);
        return;
    }
}

/**
 * BSL_CMD_ERASE_FLASH
 */
static void prvEraseFlash(fdlEngine_t *fdl, fdlPacket_t *pkt, fdlContext_t *d)
{
    uint32_t *ptr = (uint32_t *)pkt->content;
    uint32_t addr = OSI_FROM_BE32(*ptr++);
    uint32_t size = OSI_FROM_BE32(*ptr++);

    OSI_LOGI(0, "FDL: erase flash, addr/0x%x, size/0x%x", addr, size);

    if (addr == ERASE_RUNNING_NV_LOGIC_ADDRESS)
    {
        if (prvFsInit(d))
            nvmClearRunning(); // ignore error
        d->running_cleared = true;
    }
    else if (addr == PHASECHECK_LOGIC_ADDRESS)
    {
        // remove phasecheck file in /factory
        if (prvFsInit(d))
            nvmClearPhaseCheck(); // ignore error
    }
    else if (addr == DEL_APPIMG_LOGIC_ADDRESS)
    {
#if defined(CONFIG_APPIMG_LOAD_FLASH) && defined(CONFIG_APPIMG_LOAD_FILE_NAME)
        if (prvFsInit(d))
            vfs_unlink(CONFIG_APPIMG_LOAD_FILE_NAME); // ignore error
#endif
    }
    else if (addr == FMT_FLASH_LOGIC_ADDRESS)
    {
        if (!fsMountFormatFlash(size)) // "size" is the flash block device name
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
    }
    else if (FLASH_ERASE_ALL == addr && FLASH_ERASE_ALL_SIZE == size)
    {
        // backup phasecheck in case of nv backup
        phaseCheckHead_t phasecheck_data;
        if (d->fixnv_read && !nvmReadPhasecheck(&phasecheck_data))
        {
            OSI_LOGE(0, "FDL: read phasecheck fail before erase all");
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
        }

        fsUmountAll();
        d->fs_mounted = false;

        bootSpiFlash_t *flash = bootSpiFlashOpen(DRV_NAME_SPI_FLASH);
        if (flash == NULL)
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);

        bootSpiFlashChipErase(flash);

#ifdef CONFIG_BOARD_WITH_EXT_FLASH
        bootSpiFlash_t *flash_ext = bootSpiFlashOpen(DRV_NAME_SPI_FLASH_EXT);
        if (flash_ext == NULL)
            ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);

        bootSpiFlashChipErase(flash_ext);
#endif

        if (d->fixnv_read)
        {
            if (!prvFsInitWithFormat(d))
            {
                OSI_LOGE(0, "FDL: format fail after erase all");
                ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
            }

            if (!nvmWritePhaseCheck(&phasecheck_data))
            {
                OSI_LOGE(0, "FDL: write phasecheck fail after erase all");
                ERR_REP_RETURN(fdl, BSL_REP_OPERATION_FAILED);
            }
        }
    }
    else if (IS_FLASH_ADDRESS(addr))
    {
        bootSpiFlash_t *flash = bootSpiFlashOpen(HAL_FLASH_DEVICE_NAME(addr));
        uint32_t flash_offset = HAL_FLASH_OFFSET(addr);
        if (!bootSpiFlashErase(flash, flash_offset, size))
            ERR_REP_RETURN(fdl, BSL_REP_INVALID_CMD);
    }
    else
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_INVALID_CMD);
        return;
    }

    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

/**
 * BSL_CMD_CHANGE_BAUD
 */
static void prvSetBaud(fdlEngine_t *fdl, fdlPacket_t *pkt, fdlContext_t *d)
{
    uint32_t *ptr = (uint32_t *)pkt->content;
    uint32_t baud = OSI_FROM_BE32(*ptr++);

    OSI_LOGD(0, "FDL: change baud %d", baud);

    // This is special, ACK must be sent in old baud rate.
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
    fdlEngineSetBaud(fdl, baud);
}

/**
 * Process packet, called by engine.
 */
static void prvProcessPkt(fdlEngine_t *fdl, fdlPacket_t *pkt, void *param)
{
    if (fdl == NULL || pkt == NULL)
        osiPanic();

    OSI_LOGV(0, "FDL: pkt type/0x%x, size/0x%x", pkt->type, pkt->size);

    fdlContext_t *d = (fdlContext_t *)param;
    switch (pkt->type)
    {
    case BSL_CMD_CONNECT:
        prvConnect(fdl, d);
        break;

    case BSL_CMD_START_DATA:
        prvDataStart(fdl, pkt, d);
        break;

    case BSL_CMD_MIDST_DATA:
        prvDataMidst(fdl, pkt, d);
        break;

    case BSL_CMD_END_DATA:
        prvDataEnd(fdl, d);
        break;

    case BSL_CMD_NORMAL_RESET:
        prvResetNormal(fdl, d);
        break;

    case BSL_CMD_READ_FLASH:
        prvReadFlash(fdl, pkt, d);
        break;

    case BSL_CMD_REPARTITION:
        OSI_LOGI(0, "FDL: repartition");
        fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
        break;

    case BSL_CMD_ERASE_FLASH:
        prvEraseFlash(fdl, pkt, d);
        break;

    case BSL_CMD_POWER_OFF:
        prvPowerOff(fdl, d);
        break;

    case BSL_CMD_CHANGE_BAUD:
        prvSetBaud(fdl, pkt, d);
        break;

    case BSL_CMD_ENABLE_DEBUG_MODE:
        fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
        break;

    default:
        OSI_LOGE(0, "FDL: cmd not support yet 0x%x", pkt->type);
        fdlEngineSendRespNoData(fdl, BSL_REP_INVALID_CMD);
        break;
    }
}

/**
 * Polling. Called by engine
 */
static void prvPolling(fdlEngine_t *fdl, void *param)
{
    fdlContext_t *d = (fdlContext_t *)param;

    prvFlashPolling(d, d->dnld.received_size);
    prvPowerOffPolling(d);
}

/**
 * Start download
 */
bool fdlDnldStart(fdlEngine_t *fdl, unsigned devtype)
{
    fdlContext_t *d = calloc(1, sizeof(fdlContext_t));
    if (d == NULL)
        return false;

    halPmuExtFlashPowerOn();

    d->device_type = devtype;
    d->channel = fdlEngineGetChannel(fdl);
    d->max_packet_len = fdlEngineGetMaxPacketLen(fdl);

#ifdef CONFIG_SOC_8811
    d->secure_boot_enable = bootSecureBootEnable();
#endif

    fsMountSetScenario(FS_SCENRARIO_FDL);
    d->fs_mounted = false;

    fdlEngineProcess(fdl, prvProcessPkt, prvPolling, d);
    return false;
}
