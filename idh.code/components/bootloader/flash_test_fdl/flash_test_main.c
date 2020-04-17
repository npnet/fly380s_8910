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

// #define LOCAL_LOG_LEVEL LOG_LEVEL_DEBUG

#include "boot_fdl.h"
#include "boot_entry.h"
#include "boot_platform.h"
#include "boot_debuguart.h"
#include "boot_mem.h"
#include "boot_adi_bus.h"
#include "boot_spi_flash.h"
#include "hal_chip.h"
#include "hal_config.h"
#include "hal_spi_flash.h"
#include "cmsis_core.h"
#include "osi_log.h"
#include <sys/reent.h>
#include <stdint.h>
#include <string.h>
#include <machine/endian.h>

#define FDL_SIZE_4K (0x1000)
#define FDL_SIZE_32K (0x8000)
#define FDL_SIZE_64K (0x10000)

#define ALIGN_SIZE(v, a) (((v) & ~((a)-1)) == 0 ? 1 : 0)
#define FLASH_TEST_ACK 0xD0
#define FLASH_TEST_ERR 0xD1
#define PATTERN_LEN 4096

uint32_t test_cnt;
static uint8_t pattern[PATTERN_LEN];

struct flashtest_time
{
    uint32_t erase;
    uint32_t write;
};
typedef struct flashtest_time flashtest_time_t;

static void _arrayValueInit(void)
{
    uint16_t arr_len = PATTERN_LEN;
    uint16_t val_len = 64;

    uint8_t values[] = {0xa, 0x5, 0xa, 0x5, 0xa, 0x5, 0xa, 0x5, 0x5, 0xa, 0x5, 0xa, 0x5, 0xa, 0x5, 0xa,
                        0x0, 0x0, 0x0, 0x0, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0, 0x0,
                        0xf, 0xf, 0x0, 0x0, 0xf, 0xf, 0x0, 0x0, 0x0, 0x0, 0xf, 0xf, 0x0, 0x0, 0xf, 0xf,
                        0xf, 0x0, 0xf, 0x0, 0xf, 0x0, 0xf, 0x0, 0x0, 0xf, 0x0, 0xf, 0x0, 0xf, 0x0, 0xf};
    int i = 0;
    while (i < arr_len)
    {
        memcpy(&pattern[i], values, val_len);
        i += val_len;
    }
    OSI_LOGI(0, "FLASH: init pattern %x %x %x ... %x %x %x", pattern[0], pattern[1], pattern[2], pattern[4093], pattern[4094], pattern[4095]);
}

static bool _bootFlashChipErase(bootSpiFlash_t *flash)
{
    bootSpiFlashChipErase(flash);

    uint8_t *rd_data = (uint8_t *)bootSpiFlashMapAddress(flash, 0);
    uint32_t rd_size = bootSpiFlashCapacity(flash);
    while (rd_size > 0)
    {
        if (*(rd_data++) != 0xff)
        {
            OSI_LOGE(0, "FLASH: Erase whole flash error2  0x%x != 0xff...", rd_data);
            return false;
        }
        rd_size--;
    }
    return true;
}

static bool _bootFlashErase(bootSpiFlash_t *flash, uint32_t offset, size_t size, size_t sector)
{
    bool res = false;

    switch (sector)
    {
    case FDL_SIZE_4K:
    case FDL_SIZE_32K:
    case FDL_SIZE_64K:
        res = bootSpiFlashErase(flash, offset, size);
        break;

    default:
        OSI_LOGE(0, "FLASH: sector invalid  0x%x", sector);
    }
    return res;
}

static bool _bootFlashWrite(bootSpiFlash_t *flash, uint32_t offset, size_t size)
{
    uint32_t len = PATTERN_LEN;
    uint32_t wr_addr = offset;
    uint32_t wr_size = size;

    while (wr_size > 0)
    {
        if (!bootSpiFlashWrite(flash, wr_addr, &pattern[0], len))
        {
            OSI_LOGE(0, "FLASH: Write flash error offset %x", wr_addr);
            return false;
        }
        wr_addr += len;
        wr_size -= len;
    }

    // check write ok
    uint32_t rd_size = size;
    const void *fdata = bootSpiFlashMapAddress(flash, offset);
    uint8_t *rd_data = (uint8_t *)fdata;

    while (rd_size > 0)
    {
        for (int i = 0; i < len; i++)
        {
            if (*(rd_data + i) != pattern[i])
            {
                OSI_LOGE(0, "FLASH: Check write flash error %x shoule be %x", *(rd_data + i), pattern[i]);
                return false;
            }
        }
        rd_data += len;
        rd_size -= len;
    }
    return true;
}

static void flashEraseWriteStart(fdlEngine_t *fdl, fdlPacket_t *pkt, bootSpiFlash_t *flash)
{
    uint32_t *ptr = (uint32_t *)pkt->content;

    uint32_t offset = __ntohl(*ptr++);
    uint32_t test_size = __ntohl(*ptr++);
    uint32_t sector_size = __ntohl(*ptr++);
    flashtest_time_t flash_time = {};

    OSI_LOGI(0, "FLASH: offset 0x%x, size 0x%x  sector 0x%x...", offset, test_size, sector_size);

    if (!offset && !test_size)
    {
        uint32_t erase_time = bootHWTick16K();
        if (!_bootFlashChipErase(flash))
        {
            OSI_LOGE(0, "FLASH: erase whole flash fail");
            const char erase_string[] = "Erase whole flash fail";
            size_t er_len = strlen(erase_string);
            fdlEngineSendRespData(fdl, FLASH_TEST_ERR, erase_string, er_len);
            bootPanic();
        }
        flash_time.erase = (bootHWTick16K() - erase_time) / 16;
        flash_time.write = 0;
        OSI_LOGI(0, "FLASH: erase whole flash time %d, use %d ms ...", ++test_cnt, flash_time.erase);
    }
    else
    {
        if (!bootSpiFlashOffsetValid(flash, offset) ||
            !bootSpiFlashOffsetValid(flash, offset + test_size - 1))
        {
            OSI_LOGE(0, "FLASH: addr and size invalid");
            // send cmd err
            const char cmd_string[] = "Cmd addr or size invalid";
            size_t cmd_len = strlen(cmd_string);
            fdlEngineSendRespData(fdl, FLASH_TEST_ERR, cmd_string, cmd_len);
        }

        uint32_t start_tick = bootHWTick16K();
        OSI_LOGI(0, "start test flash...");

        // erase flash test
        if (!_bootFlashErase(flash, offset, test_size, sector_size))
        {
            OSI_LOGE(0, "FLASH: erase flash error");
            const char sector_string[] = "Erase sector flash fail";
            size_t sec_len = strlen(sector_string);
            fdlEngineSendRespData(fdl, FLASH_TEST_ERR, sector_string, sec_len);
            bootPanic();
        }
        flash_time.erase = (bootHWTick16K() - start_tick) / 16;

        // write flash test
        start_tick = bootHWTick16K();
        if (!_bootFlashWrite(flash, offset, test_size))
        {
            OSI_LOGE(0, "FLASH: write flash error");
            const char wr_string[] = "Write sector flash fail";
            size_t wr_len = strlen(wr_string);
            fdlEngineSendRespData(fdl, FLASH_TEST_ERR, wr_string, wr_len);
            bootPanic();
        }
        flash_time.write = (bootHWTick16K() - start_tick) / 16;

        OSI_LOGI(0, "FLASH:time %d erase/write ~~~ %d ms / %d ms...", ++test_cnt, flash_time.erase, flash_time.write);
    }

    fdlEngineSendRespData(fdl, FLASH_TEST_ACK, &flash_time, sizeof(flashtest_time_t));
}

static void _process_flash(fdlEngine_t *fdl, fdlPacket_t *pkt, void *flash_)
{
    if (fdl == NULL || pkt == NULL)
        bootPanic();

    bootSpiFlash_t *flash = (bootSpiFlash_t *)flash_;
    switch (pkt->type)
    {
    case BSL_CMD_CONNECT:
        fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
        break;

    case BSL_ERASE_FLASH:
        flashEraseWriteStart(fdl, pkt, flash);
        break;

    default:
        OSI_LOGE(0, "FDL1, cmd not support yet %x", pkt->type);
        bootPanic();
        break;
    }
}

void bootStart(uint32_t param)
{
    halSpiFlashStatusCheck(0);
    halClockInit();

    bootInitRunTime();
    __FPU_Enable();
    _impure_ptr = _GLOBAL_REENT;

    bootDebuguartInit();
    bootAdiBusInit();
    bootHeapInit((uint32_t *)CONFIG_BOOT_SRAM_HEAP_START, CONFIG_BOOT_SRAM_HEAP_SIZE, 0, 0);

    OSI_LOGI(0, "FLASH_TEST_RUN ......");

    fdlChannel_t *ch = fdlOpenUart(CONFIG_FDL_UART_BAUD);
    if (ch == NULL)
    {
        OSI_LOGE(0, "FDL1 fail, can't open uart pdl channel");
        bootPanic();
    }

    fdlEngine_t *fdl = fdlEngineCreate(ch);
    if (fdl == NULL)
    {
        OSI_LOGE(0, "FDL1 fail, can not create fdl engine");
        bootPanic();
    }
    for (;;)
    {
        uint8_t sync;
        if (fdlEngineReadRaw(fdl, &sync, 1))
        {
            if (sync == HDLC_FLAG)
            {
                if (!fdlEngineSendVersion(fdl))
                {
                    OSI_LOGE(0, "FDL1 fail, fail to send version string");
                    bootPanic();
                }
                break;
            }
        }
    }
    _arrayValueInit();

    bootSpiFlash_t *flash = bootSpiFlashOpen();
    fdlEngineProcess(fdl, _process_flash, flash);

    // never return here
    fdlEngineDestroy(fdl);
    bootPanic();
}