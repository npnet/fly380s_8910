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
#include "osi_sysnv.h"
#include "osi_trace.h"
#include "app_config.h"
#include "at_engine.h"
#include "fs_config.h"
#include "fs_mount.h"
#include "fs_mount_sdcard.h"
#include "ml.h"
#include "hal_shmem_region.h"
#include "hal_cp_ctrl.h"
#include "hal_iomux.h"
#include "net_config.h"
#include "hal_config.h"
#include "hwregs.h"
#include "netmain.h"
#include "drv_ps_path.h"
#include "drv_pmic_intr.h"
#include "drv_charger.h"
#include "drv_rtc.h"
#include "drv_adc.h"
#include "drv_gpio.h"
#include "drv_usb.h"
#include "drv_axidma.h"
#include "drv_md_nvm.h"
#include "drv_md_ipc.h"
#include "drv_debughost.h"
#include "drv_spi_flash.h"
#include "drv_config.h"
#include "nvm.h"
#include "vfs.h"
#include "diag.h"
#include "diag_runmode.h"
#include "diag_auto_test.h"
#include "app_internal.h"
#include "srv_usb_trace.h"
#include "srv_modem_log.h"
#include "srv_mos_log.h"
#include "srv_rf_param.h"
#include "fupdate.h"
#include "srv_wdt.h"
#include "app_loader.h"
#include "mal_api.h"
#include "audio_device.h"
#include <stdlib.h>
#include "tts_player.h"
#include "poc.h"

#define CHECK_RUN_MODE_TIMEOUT (500) // ms

static const char gBuildRevision[] OSI_SECTION_RW_KEEP = BUILD_IDENTIFY;

// TODO:
extern void ipcInit(void);
extern int32_t ipc_at_init(void);
extern void CFW_RpcInit(void);
extern bool aworker_start();

static void _checkGpioCalibMode(void)
{
#ifdef CONFIG_BOARD_ENTER_CALIB_BY_GPIO
    drvGpioConfig_t cfg = {
        .mode = DRV_GPIO_INPUT,
        .intr_enabled = false,
        .intr_level = false,
        .rising = true,
        .falling = false,
        .debounce = false,
    };

    struct drvGpio *gpio = drvGpioOpen(CONFIG_BOARD_ENTER_CALIB_GPIO, &cfg, NULL, NULL);
    if (drvGpioRead(gpio))
        osiSetBootMode(OSI_BOOTMODE_CALIB);
#endif
}

static void prvSetFlashWriteProhibit(void)
{
    // ATTENTION: This will set write prohibit for bootloader and AP codes
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

#ifdef CONFIG_USB_TRACE_ENABLE
static bool prvTraceSendDebughostUsb(const void *data, unsigned size)
{
    bool ok0 = drvDhostSendAll(data, size);
    bool ok1 = srvUsbTraceSendAll(data, size);
    return ok0 && ok1;
}
#endif

static void prvTraceInit(void)
{
    osiTraceDevice_t device = gSysnvTraceDevice;

    // always initialize debughost
    drvDhostInit();
    osiTraceSetEnable(gSysnvTraceEnabled);

#ifdef CONFIG_USB_TRACE_ENABLE
    if (device & OSI_TRACE_DEVICE_USBSERIAL)
        srvUsbTraceInit();
#endif

    // default sender is debughost
    osiTraceSender_t sender = drvDhostSendAll;
    osiTraceSender_t bs_sender = drvDhostBlueScreenSend;

#ifdef CONFIG_USB_TRACE_ENABLE
    if (device == OSI_TRACE_DEVICE_DEBUGHOST_USBSERIAL)
        sender = prvTraceSendDebughostUsb;
    if (device == OSI_TRACE_DEVICE_USBSERIAL)
        sender = srvUsbTraceSendAll;
#endif

    osiTraceSenderInit(sender, bs_sender);
}

void osiAppStart(void)
{
    OSI_LOGXI(OSI_LOGPAR_S, 0, "application start (%s)", gBuildRevision);

#ifdef CONFIG_WDT_ENABLE
    srvWdtInit(CONFIG_APP_WDT_MAX_INTERVAL, CONFIG_APP_WDT_FEED_INTERVAL);
#endif

    osiInvokeGlobalCtors();
    prvSetFlashWriteProhibit();

    mlInit();
    if (!fsMountAllApp(&gPartInfo))
        osiPanic();

#ifdef CONFIG_FS_MOUNT_SDCARD
    bool mount_sd = fsMountSdcard();
    OSI_LOGI(0, "application mount sd card %d", mount_sd);
#endif

    // besure ap nvm directory exist
    vfs_mkdir(CONFIG_FS_AP_NVM_DIR, 0);

    osiSysnvLoad();
    prvTraceInit();
    nvmInit();

    fupdateStatus_t fup_status = fupdateGetStatus();
    if (fup_status == FUPDATE_STATUS_FINISHED)
    {
        char *old_version = NULL;
        char *new_version = NULL;
        if (fupdateGetVersion(&old_version, &new_version))
        {
            OSI_LOGXI(OSI_LOGPAR_SS, 0, "FUPDATE: %s -> %s",
                      old_version, new_version);
            free(old_version);
            free(new_version);
        }
        //fupdateInvalidate(true);
    }

    if (!halShareMemRegionLoad())
        osiPanic();

    osiPsmRestore();

    drvAxidmaInit();
    drvGpioInit();
    drvPmicIntrInit();
    drvRtcInit();
    drvAdcInit();
    drvChargerInit();
    drvUsbInit();

    ipcInit();
    drvNvmIpcInit();

    svcRfParamInit();

    ipc_at_init();
    drvPsPathInit();

#ifdef CONFIG_DIAG_ENABLED
    diagInit();
#endif

    _checkGpioCalibMode();
    if (osiGetBootMode() == OSI_BOOTMODE_NORMAL)
    {
        diagRm_t run_mode = diagRmCheck(CHECK_RUN_MODE_TIMEOUT);
        if (run_mode == DIAG_RM_CALIB)
            osiSetBootMode(OSI_BOOTMODE_CALIB);
        else if (run_mode == DIAG_RM_CALIB_POST)
            osiSetBootMode(OSI_BOOTMODE_CALIB_POST);
        else if (run_mode == DIAG_RM_BBAT)
            osiSetBootMode(OSI_BOOTMODE_BBAT);
    }

    if (osiGetBootMode() == OSI_BOOTMODE_CALIB)
    {
        OSI_LOGI(0, "application enter calibration mode");
        fsRemountFactory(0); // read-write
    }
    else if (osiGetBootMode() == OSI_BOOTMODE_CALIB_POST)
    {
        OSI_LOGI(0, "application enter calibration post mode");
        fsRemountFactory(0); // read-write
        diagCalibPostInit();
    }
    else if (osiGetBootMode() == OSI_BOOTMODE_BBAT)
    {
        OSI_LOGI(0, "application enter BBAT mode");
        fsRemountFactory(0); // read-write
        diagAutoTestInit();
    }
    else
    {
        bool usbok = drvUsbSetWorkMode(gSysnvUsbWorkMode);
        OSI_LOGI(0, "application start usb mode %d/%d/%d", gSysnvUsbWorkMode, gSysnvUsbDetMode, usbok);
        if (usbok)
        {
            drvUsbEnable(CONFIG_USB_DETECT_DEBOUNCE_TIME);
        }
    }

    if (!srvModemLogInit())
    {
        OSI_LOGE(0, "init modem log fail.");
        osiPanic();
    }

    if (!srvMosLogInit())
    {
        OSI_LOGE(0, "init mos log fail.");
    }

    CFW_RpcInit();
    malInit();

#ifdef CONFIG_APP_START_ATR
    // asynchrous worker, start before at task
    aworker_start();
    atEngineStart();
#endif

    // zsp_uart & uart_3 are both for cp
    halIomuxRequestBatch(PINFUNC_ZSP_UART_TXD, PINFUNC_UART_3_TXD, HAL_IOMUX_REQUEST_END);
    if (!halCpLoad())
        osiPanic();

    audevInit();
    appKeypadInit();
#ifdef CONFIG_TTS_SUPPORT
    ttsPlayerInit();
#endif

#ifdef CONFIG_NET_TCPIP_SUPPORT
    net_init();
#endif

#if defined(CONFIG_APPIMG_LOAD_FLASH) && defined(CONFIG_APPIMG_FLASH_OFFSET)
    const void *flash_img_address = (const void *)(CONFIG_NOR_PHY_ADDRESS + CONFIG_APPIMG_FLASH_OFFSET);
    if (appImageFromMem(flash_img_address, &gAppImgFlash))
    {
        OSI_LOGI(0, "Find app image at 0x%x", flash_img_address);
        gAppImgFlash.enter(NULL);
    }
#endif

#if defined(CONFIG_APPIMG_LOAD_FILE) && defined(CONFIG_APPIMG_LOAD_FILE_NAME)
    if (appImageFromFile(CONFIG_APPIMG_LOAD_FILE_NAME, &gAppImgFile))
    {
        OSI_LOGI(0, "Find file image at " CONFIG_APPIMG_LOAD_FILE_NAME);
        gAppImgFile.enter(NULL);
    }
#endif

    // wait a while for PM source created
    osiThreadSleep(10);
    osiPmStart();

    // HACK: Now CP will change hwp_debugUart->irq_mask. After CP is changed,
    // the followings should be removed.
    osiThreadSleep(1000);
    drvDhostRestoreConfig();

#ifdef CONFIG_POC_SUPPORT
	osiThreadCreate("pocStart", pocStart, NULL, OSI_PRIORITY_NORMAL, 1024, 64);
#endif
}
