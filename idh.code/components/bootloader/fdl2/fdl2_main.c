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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('F', 'D', 'L', '2')
// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "boot_fdl.h"
#include "boot_entry.h"
#include "boot_platform.h"
#include "boot_debuguart.h"
#include "boot_spi_flash.h"
#include "boot_mem.h"
#include "boot_mmu.h"
#include "boot_adi_bus.h"
#include "hal_chip.h"
#include "boot_secure.h"
#include "boot_irq.h"
#include "boot_timer.h"
#include "hal_config.h"
#include "hal_spi_flash.h"
#include "cmsis_core.h"
#include "hwregs.h"
#include "osi_log.h"
#include "osi_api.h"
#include "fs_mount.h"
#include "vfs.h"
#include "nvm.h"
#include "calclib/crc32.h"
#include "cpio_parser.h"
#include <sys/reent.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// Work memory size in context
#define WORKMEM_SIZE 0x20000
#define PACKAGE_FILE_MAX_SIZE 0x10000

// When bootloader is written, secure boot encrypt should be considered
#define BOOTLOADER_ADDRESS (CONFIG_NOR_PHY_ADDRESS + CONFIG_BOOT_FLASH_OFFSET)

// When modem image is written, fixed nv bin should be check for clear running
#define MODEM_IMAGE_ADDRESS (CONFIG_NOR_PHY_ADDRESS + CONFIG_FS_MODEM_FLASH_OFFSET)

// Logic addresses, which should match the value used in pac file
#define ERASE_NV_LOGIC_ADDRESS 0xFE000001
#define PHASECHECK_LOGIC_ADDRESS 0xFE000002
#define NV_LOGIC_ADDRESS 0xFE000003
#define PRE_PACK_FILE_LOGIC_ADDRESS 0xFE000004
#define DEL_APPIMG_LOGIC_ADDRESS 0xFE000005

#define FLASH_ERASE_ALL 0x00000000
#define FLASH_ERASE_ALL_SIZE 0xFFFFFFFF

#define FLASH_SECTOR_SIZE (0x1000)

#define FDL_SIZE_4K (0x1000)
#define SPI_FLASH_OFFSET(address) ((uint32_t)(address)&0x00ffffff)

#define BOOT_SIGN_SIZE (0xbd40)
#define ENCRYPT_OFF 0x1000
#define ENCRYPT_LEN 0x400
#define CHECK_CONNECT_TIMER_PERIOD (500)

static_assert(WORKMEM_SIZE >= CONFIG_NVBIN_FIXED_SIZE, "WORKMEM_SIZE is too small");
static_assert(WORKMEM_SIZE >= sizeof(phaseCheckHead_t), "WORKMEM_SIZE is too small");

typedef struct
{
    bootSpiFlash_t *flash;
    bootSpiFlash_t *flash_ext;
    cpioStream_t *cpio;
    fdlDnld_t dnld;
    phaseCheckHead_t phasecheck_data;
    bool phasecheck_pending;
    bool fs_mounted;
    bool running_cleared;
    uint32_t init_nvbin_crc;
    fdlChannel_t *channel;
    fdlDeviceType_t device_type;
    char work_mem[WORKMEM_SIZE];
} fdl2Context_t;

// Calculate nvbin CRC, return 0 on read fail
static uint32_t prvGetNvBinCrc(fdl2Context_t *ctx)
{
    int nvbin_size = nvmReadFixedBin(NULL, 0);
    if (nvbin_size <= 0 || nvbin_size > CONFIG_NVBIN_FIXED_SIZE)
        return 0;

    char *nvbin = &ctx->work_mem[0];
    memset(nvbin, 0xff, CONFIG_NVBIN_FIXED_SIZE);
    nvmReadFixedBin(nvbin, CONFIG_NVBIN_FIXED_SIZE);
    return crc32Calc(nvbin, CONFIG_NVBIN_FIXED_SIZE);
}

// Check whether running should be cleared
static void prvCheckClearRunning(fdl2Context_t *ctx)
{
    if (ctx->running_cleared)
        return;

    int nvbin_size = nvmReadFixedBin(NULL, 0);
    if (nvbin_size > 0 && nvbin_size <= CONFIG_NVBIN_FIXED_SIZE)
    {
        char *nvbin = &ctx->work_mem[0];
        memset(nvbin, 0xff, CONFIG_NVBIN_FIXED_SIZE);
        nvmReadFixedBin(nvbin, CONFIG_NVBIN_FIXED_SIZE);

        // do nothing if the nvbin CRC matches
        if (crc32Calc(nvbin, CONFIG_NVBIN_FIXED_SIZE) == ctx->init_nvbin_crc)
            return;
    }

    nvmClearRunning();
    ctx->running_cleared = true;
}

// Mount file system and initialize nvm
static bool prvFsNvmInit(fdl2Context_t *ctx, bool phasecheck)
{
    if (ctx->fs_mounted)
        return true;

    // NOTE: If the first time to mount fs is triggered by phasecheck, all
    // file system will be formatted.
    //
    // The assumption is: only FactoryDownload will send phasecheck before
    // other command using file system.
    if (phasecheck)
        ctx->fs_mounted = fsMountFormatAll(&gPartInfo);
    else
        ctx->fs_mounted = fsMountAllFdl(&gPartInfo);

    if (!ctx->fs_mounted)
    {
        OSI_LOGE(0, "FDL2: mount file system failed phasecheck/%d", phasecheck);
        return false;
    }

    // Initialize nvm before nv access.
    nvmInit();

    // HACK: ResearchDownload needs that nvid 2 must exist, and
    // previously this nv isn't added in 8910. Due to this nv isn't actually
    // used. So, at the beginning of FDL2, this will be written to make
    // ResearchDownload happy.
    if (nvmReadItem(NVID_CALIB_PARAM, NULL, 0) <= 0)
    {
        // Refer to CMakeLists.txt about these 2 external symbols.
        extern unsigned gDefCalibParam;
        extern unsigned gDefCalibParamSize;
        nvmWriteItem(NVID_CALIB_PARAM, &gDefCalibParam, gDefCalibParamSize);
    }

    // HACK: GSM calibration data size is changed from 7706 to 8192. The
    // size of existed GSM calibration data may be 7706. And it is needed
    // to pad the existed GSM calibration data to 8192, to make NVEditor
    // work, and make nv backup work.
    int calib_gsm_size = nvmReadFixedItem(0x26d, NULL, 0);
    if (calib_gsm_size > 0 && calib_gsm_size < 8192)
    {
        char *calib_gsm_bin = &ctx->work_mem[0];
        memset(calib_gsm_bin, 0, 8192);

        nvmReadFixedItem(0x26d, calib_gsm_bin, 8192);
        nvmWriteFixedItem(0x26d, calib_gsm_bin, 8192);
    }

    ctx->init_nvbin_crc = prvGetNvBinCrc(ctx);
    ctx->running_cleared = false;
    return true;
}

// Process connect packet
static void prvConnect(fdlEngine_t *fdl, fdl2Context_t *ctx)
{
    ctx->dnld.detect_time = bootHWTick16K();
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

// Process data start packet
static void prvDataStart(fdlEngine_t *fdl, fdlPacket_t *pkt, fdl2Context_t *ctx)
{
    uint32_t *ptr = (uint32_t *)pkt->content;
    uint32_t start_addr = OSI_FROM_BE32(*ptr++);
    uint32_t file_size = OSI_FROM_BE32(*ptr++);

    OSI_LOGD(0, "FDL2: data start, start_addr/0x%x, size/0x%x", start_addr, file_size);

    // The followings may access file system
    if (start_addr == ERASE_NV_LOGIC_ADDRESS ||
        start_addr == NV_LOGIC_ADDRESS ||
        start_addr == PRE_PACK_FILE_LOGIC_ADDRESS)
    {
        if (!prvFsNvmInit(ctx, false))
        {
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }

    if (start_addr == ERASE_NV_LOGIC_ADDRESS)
    {
        ; // erase in DATA_END
    }
    else if (start_addr == PHASECHECK_LOGIC_ADDRESS)
    {
        if (file_size != sizeof(phaseCheckHead_t))
        {
            OSI_LOGE(0, "FDL2: invalid phasecheck data size %d", file_size);
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }
    else if (start_addr == NV_LOGIC_ADDRESS)
    {
        if (file_size > CONFIG_NVBIN_FIXED_SIZE)
        {
            OSI_LOGE(0, "FDL2: invalid fixnv data size %d", file_size);
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }
    else if (start_addr == PRE_PACK_FILE_LOGIC_ADDRESS)
    {
        cpioStreamCfg_t cfg = {.file_size_max = PACKAGE_FILE_MAX_SIZE, .file_path_max = VFS_PATH_MAX};
        ctx->cpio = cpioStreamCreate(&cfg);
        if (ctx->cpio == NULL)
        {
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }
    else if (bootSpiFlashMapAddressValid(ctx->flash, (void *)start_addr))
    {
        // It is acceptable that the size parameter is not sector aligned.
        // So, align it before calling flash API.

        unsigned flash_offset = SPI_FLASH_OFFSET(start_addr);
        if (!bootSpiFlashErase(ctx->flash, flash_offset, OSI_ALIGN_UP(file_size, FLASH_SECTOR_SIZE)))
        {
            OSI_LOGE(0, "FDL2: erase flash failed 0x%x/0x%x", flash_offset, file_size);
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }
    else if (ctx->flash_ext != NULL && bootSpiFlashMapAddressValid(ctx->flash_ext, (void *)start_addr))
    {
        // It is acceptable that the size parameter is not sector aligned.
        // So, align it before calling flash API.

        unsigned flash_offset = SPI_FLASH_OFFSET(start_addr);
        if (!bootSpiFlashErase(ctx->flash_ext, flash_offset, OSI_ALIGN_UP(file_size, FLASH_SECTOR_SIZE)))
        {
            OSI_LOGE(0, "FDL2: erase ext flash failed 0x%x/0x%x", flash_offset, file_size);
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }
    else
    {
        OSI_LOGE(0, "FDL2: invalid data start address 0x%x", start_addr);
        fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
        return;
    }

    ctx->dnld.start_address = start_addr;
    ctx->dnld.total_size = file_size;
    ctx->dnld.received_size = 0;
    ctx->dnld.stage = SYS_STAGE_START;
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

static void prvMkdirRecursive(char *name)
{
    for (unsigned i = 1; i < strlen(name); ++i)
    {
        if (name[i] == '/')
        {
            name[i] = '\0';
            vfs_mkdir(name, 0);
            name[i] = '/';
        }
    }

    vfs_mkdir(name, 0);
}

static void prvPackageFile(cpioFile_t *f)
{
    char name[VFS_PATH_MAX + 1] = "/";
    bool isdir = f->mode & S_IFDIR;
    memcpy(name + 1, f->name, strlen(f->name) + 1);
    OSI_LOGXI(OSI_LOGPAR_SII, 0, "package file %s (%d/%u)", name, isdir, f->data_size);
    if (isdir)
    {
        prvMkdirRecursive(name);
    }
    else
    {
        for (unsigned i = strlen(name); i > 0; --i)
        {
            if (name[i] == '/')
            {
                name[i] = 0;
                prvMkdirRecursive(name);
                name[i] = '/';
                break;
            }
        }
        vfs_file_write(name, f->data, f->data_size);
    }
}

static void prvDataMidst(fdlEngine_t *fdl, fdlPacket_t *pkt, fdl2Context_t *ctx)
{
    uint16_t data_len = pkt->size;
    uint32_t start_addr = ctx->dnld.start_address;

    OSI_LOGD(0, "FDL2: data midst, start_addr/0x%x, size/0x%x, received/0x%x, len/0x%x, stage/0x%x",
             ctx->dnld.start_address, ctx->dnld.total_size, ctx->dnld.received_size,
             data_len, ctx->dnld.stage);

    if ((ctx->dnld.stage != SYS_STAGE_START) && (ctx->dnld.stage != SYS_STAGE_GATHER))
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_NOT_START);
        return;
    }

    if (ctx->dnld.received_size + data_len > ctx->dnld.total_size)
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_SIZE_ERROR);
        return;
    }

    if (start_addr == PRE_PACK_FILE_LOGIC_ADDRESS)
    {
        cpioStreamPushData(ctx->cpio, pkt->content, data_len);
        cpioFile_t *f;
        while ((f = cpioStreamPopFile(ctx->cpio)))
        {
            prvPackageFile(f);
            cpioStreamDestroyFile(ctx->cpio, f);
        }
    }
    else if (bootSpiFlashMapAddressValid(ctx->flash, (void *)start_addr))
    {
        unsigned flash_offset = SPI_FLASH_OFFSET(start_addr + ctx->dnld.received_size);
        if (!bootSpiFlashWrite(ctx->flash, flash_offset, pkt->content, data_len))
        {
            OSI_LOGE(0, "FDL2: write flash failed 0x%x/0x%x", flash_offset, data_len);
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }
    else if (ctx->flash_ext != NULL && bootSpiFlashMapAddressValid(ctx->flash_ext, (void *)start_addr))
    {
        unsigned flash_offset = SPI_FLASH_OFFSET(start_addr + ctx->dnld.received_size);
        if (!bootSpiFlashWrite(ctx->flash_ext, flash_offset, pkt->content, data_len))
        {
            OSI_LOGE(0, "FDL2: write ext flash failed 0x%x/0x%x", flash_offset, data_len);
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }
    else if (ctx->dnld.received_size + data_len > WORKMEM_SIZE)
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
        return;
    }
    else
    {
        memcpy(&ctx->work_mem[0] + ctx->dnld.received_size, pkt->content, data_len);
    }

    ctx->dnld.received_size += data_len;
    ctx->dnld.stage = SYS_STAGE_GATHER;
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

static void prvDataEnd(fdlEngine_t *fdl, fdl2Context_t *ctx)
{
    uint32_t start_addr = ctx->dnld.start_address;

    OSI_LOGD(0, "FDL2: data end, start_addr/0x%x, size/0x%x, received/0x%x, stage/0x%x",
             ctx->dnld.start_address, ctx->dnld.total_size, ctx->dnld.received_size,
             ctx->dnld.stage);

    if ((ctx->dnld.stage != SYS_STAGE_START) && (ctx->dnld.stage != SYS_STAGE_GATHER))
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_NOT_START);
        return;
    }

    if (ctx->dnld.received_size != ctx->dnld.total_size)
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_SIZE_ERROR);
        return;
    }

    if (start_addr == ERASE_NV_LOGIC_ADDRESS)
    {
        nvmClearRunning();
        ctx->running_cleared = true;
    }
    else if (start_addr == PHASECHECK_LOGIC_ADDRESS)
    {
        memcpy(&ctx->phasecheck_data, &ctx->work_mem[0], sizeof(phaseCheckHead_t));
        if (!prvFsNvmInit(ctx, true))
        {
            ctx->phasecheck_pending = false;
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }

        // runningnv is cleared during format
        ctx->running_cleared = true;

        // phasecheck data will be written after NV
        ctx->phasecheck_pending = true;
    }
    else if (start_addr == NV_LOGIC_ADDRESS)
    {
        char *nvbin = &ctx->work_mem[0];
        if (nvmWriteFixedBin(nvbin, ctx->dnld.received_size) < 0)
        {
            OSI_LOGE(0, "FDL2: write nv bin failed");
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }

        if (ctx->phasecheck_pending)
        {
            // write phaseCheck file
            ctx->phasecheck_pending = false;
            phaseCheckHead_t *phase_check = (phaseCheckHead_t *)&ctx->phasecheck_data;
            if (!nvmWritePhaseCheck(phase_check))
            {
                OSI_LOGE(0, "FDL2: write phase check failed");
                fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
                return;
            }
        }

        // check clear running after nv is written
        prvCheckClearRunning(ctx);
    }
    else if (start_addr == PRE_PACK_FILE_LOGIC_ADDRESS)
    {
        cpioStreamDestroy(ctx->cpio);
        ctx->cpio = NULL;
    }
    else if (start_addr == MODEM_IMAGE_ADDRESS)
    {
        // Modem file system is written by image. So, the block device and
        // file system is changed. And it is needed to umount/mount the
        // block device and file system. The easiest method is to
        // umount/mount all. It will waste some time, and the dynamic memory
        // may be considered in memory tight system.

        fsUmountAllFdl();
        ctx->fs_mounted = false;

        if (!prvFsNvmInit(ctx, false))
        {
            OSI_LOGE(0, "FDL2: failed to remount after write modem image");
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }

        // check clear running after modem image is written
        prvCheckClearRunning(ctx);
    }
    else if (start_addr == BOOTLOADER_ADDRESS)
    {
        if (checkSecureBootState())
        {
            if (checkDataSign((void *)start_addr, BOOT_SIGN_SIZE) != 0)
            {
                OSI_LOGE(0, "FDL2: bootloader secure boot verify fail");
                fdlEngineSendRespNoData(fdl, BSL_REP_SECURITY_VERIFICATION_FAIL);
                return;
            }

            // reuse the working memory
            char *pbuf = &ctx->work_mem[0];
            memcpy(pbuf, (void *)(start_addr + ENCRYPT_OFF), FDL_SIZE_4K);
            antiCloneEncryption(pbuf, ENCRYPT_LEN);

            unsigned flash_offset = SPI_FLASH_OFFSET(start_addr);
            if (!bootSpiFlashErase(ctx->flash, flash_offset + ENCRYPT_OFF, FDL_SIZE_4K))
            {
                fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
                return;
            }

            if (!bootSpiFlashWrite(ctx->flash, flash_offset + ENCRYPT_OFF, pbuf, FDL_SIZE_4K))
            {
                fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
                return;
            }
        }
    }

    ctx->dnld.stage = SYS_STAGE_END;
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

static void prvDelayActionTimer(void *param)
{
    fdl2Context_t *ctx = (fdl2Context_t *)param;

    bool exe = false;
    if (ctx->device_type == FDL_DEVICE_UART)
    {
        // last recv connect cmd time
        uint32_t ticks = bootHWTick16K() - ctx->dnld.detect_time;
        uint32_t mseconds = ticks * 1000 / 16384;
        if (mseconds > CHECK_CONNECT_TIMER_PERIOD * 5)
        {
            OSI_LOGI(0, "FDL2: execute %d ... %d ms", ctx->dnld.action, mseconds);
            exe = true;
        }
    }
    else if (!ctx->channel->connected(ctx->channel))
    {
        OSI_LOGI(0, "FDL2: device not connected, execute %d", ctx->dnld.action);
        exe = true;
    }

    if (exe)
    {
        if (DNLD_ACTION_RESET == ctx->dnld.action)
            bootReset(BOOT_RESET_NORMAL);
        else if (DNLD_ACTION_POWEROFF == ctx->dnld.action)
            bootPowerOff();
    }
}

static void prvResetNormal(fdlEngine_t *fdl, fdl2Context_t *ctx)
{
    OSI_LOGI(0, "FDL2: reset to normal");
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
    bootDelayUS(2000);

    bootReset(BOOT_RESET_NORMAL);
}

static void prvPowerOff(fdlEngine_t *fdl, fdl2Context_t *ctx)
{
    ctx->dnld.action = DNLD_ACTION_POWEROFF;
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
    bootDelayUS(2000);

    bootEnableInterrupt();
    bootEnableTimer(prvDelayActionTimer, ctx, CHECK_CONNECT_TIMER_PERIOD);
    OSI_LOGI(0, "FDL2: PowerOff");
}

static void prvReadFlash(fdlEngine_t *fdl, fdlPacket_t *pkt, fdl2Context_t *ctx)
{
    uint32_t *ptr = (uint32_t *)pkt->content;
    uint32_t addr = OSI_FROM_BE32(*ptr++);
    uint32_t size = OSI_FROM_BE32(*ptr++);
    uint32_t offset = OSI_FROM_BE32(*ptr++); // not all address has this parameter

    OSI_LOGD(0, "FDL2: read flash, addr/0x%x, size/0x%x, offset/0x%x", addr, size, offset);

    // The followings may access file system
    if (addr == PHASECHECK_LOGIC_ADDRESS ||
        addr == NV_LOGIC_ADDRESS)
    {
        if (!prvFsNvmInit(ctx, false))
        {
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }

    if (addr == PHASECHECK_LOGIC_ADDRESS)
    {
        // reuse the working memory
        phaseCheckHead_t *phase_check = (phaseCheckHead_t *)&ctx->work_mem[0];
        memset(phase_check, 0, sizeof(phaseCheckHead_t));
        if (!nvmReadPhasecheck(phase_check))
        {
            OSI_LOGE(0, "FDL2: read phasecheck failed");
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }

        fdlEngineSendRespData(fdl, BSL_REP_READ_FLASH, phase_check, sizeof(phaseCheckHead_t));
    }
    else if (addr == NV_LOGIC_ADDRESS)
    {
        // Assuming the read will be started with offset 0, and continuous
        char *nvbin = &ctx->work_mem[0];
        if (offset == 0)
        {
            memset(nvbin, 0xff, CONFIG_NVBIN_FIXED_SIZE);
            if (nvmReadFixedBin(nvbin, CONFIG_NVBIN_FIXED_SIZE) < 0)
            {
                fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
                return;
            }
        }

        if (offset >= CONFIG_NVBIN_FIXED_SIZE)
        {
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }

        unsigned send_size = OSI_MIN(unsigned, CONFIG_NVBIN_FIXED_SIZE - offset, size);
        fdlEngineSendRespData(fdl, BSL_REP_READ_FLASH, nvbin + offset, send_size);
    }
    else if (addr == ERASE_NV_LOGIC_ADDRESS)
    {
        fdlEngineSendRespData(fdl, BSL_REP_READ_FLASH, NULL, 0);
    }
    else if (bootSpiFlashMapAddressValid(ctx->flash, (void *)addr))
    {
        // valid flash offset not includes the end address
        if (!bootSpiFlashMapAddressValid(ctx->flash, (void *)(addr + size - 1)))
        {
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }

        uint32_t flash_offset = SPI_FLASH_OFFSET(addr);
        const void *fdata = bootSpiFlashMapAddress(ctx->flash, flash_offset);
        fdlEngineSendRespData(fdl, BSL_REP_READ_FLASH, fdata, size);
    }
    else if (ctx->flash_ext != NULL && bootSpiFlashMapAddressValid(ctx->flash_ext, (void *)addr))
    {
        // valid flash offset not includes the end address
        if (!bootSpiFlashMapAddressValid(ctx->flash_ext, (void *)(addr + size - 1)))
        {
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }

        uint32_t flash_offset = SPI_FLASH_OFFSET(addr);
        const void *fdata = bootSpiFlashMapAddress(ctx->flash_ext, flash_offset);
        fdlEngineSendRespData(fdl, BSL_REP_READ_FLASH, fdata, size);
    }
    else
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
        return;
    }
}

static void prvEraseFlash(fdlEngine_t *fdl, fdlPacket_t *pkt, fdl2Context_t *ctx)
{
    uint32_t *ptr = (uint32_t *)pkt->content;
    uint32_t addr = OSI_FROM_BE32(*ptr++);
    uint32_t size = OSI_FROM_BE32(*ptr++);

    OSI_LOGD(0, "FDL2: erase flash, addr/0x%x, size/0x%x", addr, size);

    // The followings may access file system
    if (addr == PHASECHECK_LOGIC_ADDRESS ||
        addr == NV_LOGIC_ADDRESS ||
        addr == ERASE_NV_LOGIC_ADDRESS ||
        addr == DEL_APPIMG_LOGIC_ADDRESS)
    {
        if (!prvFsNvmInit(ctx, false))
        {
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }

    if (addr == ERASE_NV_LOGIC_ADDRESS || addr == NV_LOGIC_ADDRESS)
    {
        nvmClearRunning();
        ctx->running_cleared = true;
    }
    else if (addr == PHASECHECK_LOGIC_ADDRESS)
    {
        // remove phasecheck file in /factory
        nvmClearPhaseCheck();
    }
    else if (addr == DEL_APPIMG_LOGIC_ADDRESS)
    {
#if defined(CONFIG_APPIMG_LOAD_FLASH) && defined(CONFIG_APPIMG_LOAD_FILE_NAME)
        vfs_unlink(CONFIG_APPIMG_LOAD_FILE_NAME);
#endif
    }
    else if (FLASH_ERASE_ALL == addr && FLASH_ERASE_ALL_SIZE == size)
    {
        // try to save phasecheck when erase all
        bool phasecheck_exit = nvmReadPhasecheck(&ctx->phasecheck_data);

        if (ctx->fs_mounted)
        {
            fsUmountAllFdl();
            ctx->fs_mounted = false;
        }

        bootSpiFlashChipErase(ctx->flash);

        if (ctx->flash_ext != NULL)
            bootSpiFlashChipErase(ctx->flash_ext);

        prvFsNvmInit(ctx, true);

        if (phasecheck_exit)
            nvmWritePhaseCheck(&ctx->phasecheck_data);
    }
    else if (bootSpiFlashMapAddressValid(ctx->flash, (void *)addr))
    {
        uint32_t flash_offset = SPI_FLASH_OFFSET(addr);
        if (!bootSpiFlashErase(ctx->flash, flash_offset, size))
        {
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }
    else if (ctx->flash_ext != NULL && bootSpiFlashMapAddressValid(ctx->flash_ext, (void *)addr))
    {
        uint32_t flash_offset = SPI_FLASH_OFFSET(addr);
        if (!bootSpiFlashErase(ctx->flash_ext, flash_offset, size))
        {
            fdlEngineSendRespNoData(fdl, BSL_REP_VERIFY_ERROR);
            return;
        }
    }
    else
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_DEST_ERROR);
        return;
    }

    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

static void prvProcessPkt(fdlEngine_t *fdl, fdlPacket_t *pkt, void *ctx_)
{
    if (fdl == NULL || pkt == NULL)
        bootPanic();

    OSI_LOGV(0, "FDL2: pkt type/0x%x, size/0x%x", pkt->type, pkt->size);

    fdl2Context_t *ctx = (fdl2Context_t *)ctx_;
    switch (pkt->type)
    {
    case BSL_CMD_CONNECT:
        prvConnect(fdl, ctx);
        break;

    case BSL_CMD_START_DATA:
        prvDataStart(fdl, pkt, ctx);
        break;

    case BSL_CMD_MIDST_DATA:
        prvDataMidst(fdl, pkt, ctx);
        break;

    case BSL_CMD_END_DATA:
        prvDataEnd(fdl, ctx);
        break;

    case BSL_CMD_NORMAL_RESET:
        prvResetNormal(fdl, ctx);
        break;

    case BSL_CMD_READ_FLASH:
        prvReadFlash(fdl, pkt, ctx);
        break;

    case BSL_REPARTITION:
        fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
        break;

    case BSL_ERASE_FLASH:
        prvEraseFlash(fdl, pkt, ctx);
        break;

    case BSL_CMD_POWER_OFF:
        prvPowerOff(fdl, ctx);
        break;

    default:
        OSI_LOGE(0, "FDL2, cmd not support yet %x", pkt->type);
        fdlEngineSendRespNoData(fdl, BSL_REP_INVALID_CMD);
        bootPanic();
        break;
    }
}

static bool prvInit(fdl2Context_t *ctx)
{
    ctx->flash = bootSpiFlashOpen();
    if (ctx->flash == NULL)
        return false;

    ctx->flash_ext = NULL;
#if defined(CONFIG_SUPPORT_EXT_FLASH)
    ctx->flash_ext = bootSpiFlashExtOpen();
    if (ctx->flash_ext == NULL)
        return false;
#endif

    ctx->fs_mounted = false;
    ctx->phasecheck_pending = false;
    return true;
}

void bootStart(uint32_t param)
{
    OSI_CLEAR_SECTION(bss);

    halSpiFlashStatusCheck(0);
    halClockInit();
    halRamInit();

    bootMmuEnable((uint32_t *)CONFIG_BOOT_TTB_START, (uint32_t *)(CONFIG_BOOT_TTB_START + 0x4000));

    bootInitRunTime();
    __FPU_Enable();
    _impure_ptr = _GLOBAL_REENT;

    bootDebuguartInit();
    bootAdiBusInit();
    bootHeapInit((uint32_t *)CONFIG_BOOT_SRAM_HEAP_START, CONFIG_BOOT_SRAM_HEAP_SIZE,
                 (uint32_t *)CONFIG_RAM_PHY_ADDRESS, CONFIG_RAM_SIZE);
    bootHeapDefaultExtRam();

    fdlDeviceInfo_t device = (fdlDeviceInfo_t)param;
    OSI_LOGI(0, "FDL2RUN device %d baud %d ......", device.b.type, device.b.baud);

    fdl2Context_t *fdl2_ctx = (fdl2Context_t *)calloc(1, sizeof(fdl2Context_t));
    if (!prvInit(fdl2_ctx))
        bootPanic();

    fdl2_ctx->device_type = device.b.type;
    fdlChannel_t *ch = (device.b.type == FDL_DEVICE_UART) ? fdlOpenUart(device.b.baud) : fdlOpenUsbSerial();
    if (ch == NULL)
    {
        OSI_LOGE(0, "FDL2 fail, can't open %d pdl channel", device.b.type);
        bootPanic();
    }

    fdlEngine_t *fdl = fdlEngineCreate(ch);
    if (fdl == NULL)
    {
        OSI_LOGE(0, "FDL2 fail, can not create fdl engine");
        bootPanic();
    }

    fdl2_ctx->channel = ch;
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);

    fdlEngineProcess(fdl, prvProcessPkt, fdl2_ctx);

    // never return here
    bootPanic();
}
