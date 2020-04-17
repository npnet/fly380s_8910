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

#include "osi_log.h"
#include "osi_trace.h"
#include "app_config.h"
#include "at_engine.h"
#include "fs_mount.h"
#include "ml.h"
#include "hal_cp_ctrl.h"
#include "net_config.h"
#include "netmain.h"
#include "drv_charger.h"
#include "drv_rtc.h"
#include "drv_gpio.h"
#include "drv_usb.h"
#include "drv_axidma.h"
#include "drv_md_nvm.h"
#include "drv_debughost.h"
#include "drv_spi_flash.h"
#include "diag.h"
#include "diag_runmode.h"
#include "app_internal.h"
#include "modem_itf.h"
#include "hal_chip.h"
#include "factory_sector.h"
#include "mal_api.h"

static const char gBuildRevision[] OSI_SECTION_RW_KEEP = BUILD_IDENTIFY;

extern void CFW_Start(void);
extern void aworker_start(void);

static void prvSetFlashWriteProhibit(void)
{
    // ATTENTION: This will set write prohibit for bootloader an AP codes
    //
    // If there are requiement (though not reasonable) to change bootloader
    // or AP codes, the followings should be changed. And when there are
    // more regions are known never changed, more regions can be added.

    drvSpiFlash_t *flash = drvSpiFlashOpen(DRV_NAME_SPI_FLASH);
    drvSpiFlashSetRangeWriteProhibit(flash, CONFIG_BOOT_FLASH_OFFSET,
                                     CONFIG_BOOT_FLASH_OFFSET + CONFIG_BOOT_FLASH_SIZE);
    drvSpiFlashSetRangeWriteProhibit(flash, CONFIG_APP_FLASH_OFFSET,
                                     CONFIG_APP_FLASH_OFFSET + CONFIG_APP_FLASH_SIZE);
}

void osiAppStart(void)
{
    OSI_LOGI(0, "application start");

    osiInvokeGlobalCtors();
    prvSetFlashWriteProhibit();

    halPmuInit();

    mlInit();
    if (!fsMountAllApp(&gPartInfo))
        osiPanic();

    drvDhostInit();
    osiTraceSenderInit(drvDhostSendAll, drvDhostBlueScreenSend);

    factorySectorInit();
    modemInit();
    drvRtcInit();
    CFW_Start();
    malInit();
    aworker_start();
    atEngineStart();
    net_init();

    // wait a while for PM source created
    osiThreadSleep(10);
    osiClockManStart();
    osiPmStart();
}
