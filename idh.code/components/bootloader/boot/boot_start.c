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

#include <stdint.h>
#include <machine/endian.h>
#include "hal_config.h"
#include "boot_config.h"
#include "boot_platform.h"
#include "boot_fdl.h"
#include <sys/reent.h>
#include "hal_chip.h"
#include "boot_debuguart.h"
#include "boot_adi_bus.h"
#include "boot_mem.h"
#include "boot_entry.h"
#include "cmsis_core.h"
#include "boot_secure.h"
#include "boot_spi_flash.h"
#include "boot_vfs.h"
#include "boot_mmu.h"
#include "boot_timer.h"
#include "boot_irq.h"
#include "hal_spi_flash.h"
#include "flash_block_device.h"
#include "fupdate.h"
#include "fs_mount.h"
#include "osi_api.h"
#include "osi_log.h"
#include "image_header.h"

typedef void (*bootJumpFunc_t)(uint32_t param);

static void prvFirmwareUpdateProgress(int block_count, int block)
{
    OSI_LOGI(0, "FUPDATE block: %d/%d", block, block_count);
}

static void prvFirmwareUpdate(void)
{
    if (!fsMountAllBoot(&gPartInfo))
        return;

    fupdateResult_t result = fupdateRun(prvFirmwareUpdateProgress);

    OSI_LOGI(0, "FUPDATE: %d", result);
    if (result == FUPDATE_RESULT_FAILED)
        bootReset(BOOT_RESET_NORMAL);
}

#ifdef CONFIG_BOOT_TIMER_IRQ_ENABLE
static void prvTimerCallback(void)
{
    // usually, just feed external watchdog here
}
#endif

static const image_header_t *prvAppUimageHeader(void)
{
    return (const image_header_t *)(CONFIG_NOR_PHY_ADDRESS + CONFIG_APP_FLASH_OFFSET);
}

static void prvSetFlashWriteProhibit(void)
{
    // ATTENTION: This will set write prohibit for bootloader
    //
    // If there are requiement (though not reasonable) to change bootloader
    // the followings should be changed. And when there are more regions are
    // known never changed, more regions can be added.

    bootSpiFlash_t *flash = bootSpiFlashOpen();
    bootSpiFlashSetRangeWriteProhibit(flash, CONFIG_BOOT_FLASH_OFFSET,
                                      CONFIG_BOOT_FLASH_OFFSET + CONFIG_BOOT_FLASH_SIZE);
}

static bool prvCheckNormalPowerUp()
{
#ifndef CONFIG_CHARGER_POWERUP
    return (bootPowerOnCause() != OSI_BOOTCAUSE_CHARGE);
#endif
    return true;
}

void bootStart(uint32_t param)
{
    halSpiFlashStatusCheck(0);
    halClockInit();
    halRamInit();

    bootMmuEnable((uint32_t *)CONFIG_BOOT_TTB_START, (uint32_t *)(CONFIG_BOOT_TTB_START + 0x4000));

    bootInitRunTime();
    __FPU_Enable();
    _impure_ptr = _GLOBAL_REENT;

    bootAdiBusInit();

    const image_header_t *header = prvAppUimageHeader();
    if (bootIsFromPsmSleep())
    {
        bootJumpFunc_t entry = (bootJumpFunc_t)__ntohl(header->ih_ep);
        entry(param);
    }

    if (!prvCheckNormalPowerUp())
    {
        osiDelayUS(1000 * 10);
        bootPowerOff();
    }

    bootDebuguartInit();

    bootHeapInit((uint32_t *)CONFIG_BOOT_SRAM_HEAP_START, CONFIG_BOOT_SRAM_HEAP_SIZE,
                 (uint32_t *)CONFIG_RAM_PHY_ADDRESS, CONFIG_RAM_SIZE);
    bootHeapDefaultExtRam();

#ifdef CONFIG_BOOT_TIMER_IRQ_ENABLE
    bootEnableInterrupt();
    bootEnableTimer(prvTimerCallback, CONFIG_BOOT_TIMER_PERIOD);
#endif

    prvSetFlashWriteProhibit();
    prvFirmwareUpdate();

    uint32_t sync_cnt = 0;
    uint32_t timeout = (50 * 16); // 50ms
    uint32_t start_tick = bootHWTick16K();

    fdlChannel_t *ch = fdlOpenUart(CONFIG_FDL_UART_BAUD);
    if (ch == NULL)
    {
        OSI_LOGE(0, "BOOT fail, can't open uart pdl channel");
        bootPanic();
    }

    fdlEngine_t *fdl = fdlEngineCreate(ch);
    if (fdl == NULL)
    {
        OSI_LOGE(0, "BOOT fail, can not create fdl engine");
        bootPanic();
    }
    for (;;)
    {
        uint8_t sync;
        if (fdlEngineReadRaw(fdl, &sync, 1))
        {
            if (sync == HDLC_FLAG)
            {
                sync_cnt++;
                timeout = (150 * 16);
            }
            OSI_LOGI(0, "BOOT recv 0x%x , recv 0x7e cnt %d", sync, sync_cnt);
        }
        if (sync_cnt >= 3)
        {
            fdlEngineSendVersion(fdl);

            OSI_LOGI(0, "ENTER DOWNLOAD MODE ");
            bootDelayUS(50 * 1000);
            bootReset(BOOT_RESET_FORCE_DOWNLOAD);
            //or download fdl1 and run fdl1 TODO
            break;
        }

        if ((bootHWTick16K() - start_tick) > timeout)
            break;
    }

    if (__ntohl(header->ih_magic) == IH_MAGIC && !checkUimageSign((void *)header))
    {
        bootMmuDisable();
#ifdef CONFIG_BOOT_TIMER_IRQ_ENABLE
        bootDisableTimer();
        bootDisableInterrupt();
#endif
        bootJumpFunc_t entry = (bootJumpFunc_t)__ntohl(header->ih_ep);
        entry(param);
    }

    bootDebugEvent(0xdead0000);
    OSI_DEAD_LOOP;
}
