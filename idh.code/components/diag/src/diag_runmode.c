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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('D', 'G', 'R', 'M')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_INFO

#include "diag_runmode.h"
#include "diag.h"
#include "osi_api.h"
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
#include "cmddef.h"
#include "drv_names.h"
#include "drv_charger.h"
#include "drv_serial.h"
#include "diag_config.h"
#include "drv_usb.h"
#include "osi_log.h"
#include "osi_sysnv.h"

typedef struct
{
    diagRm_t mode;
    bool diagDeviceUSB;
    osiSemaphore_t *sema;
} rmCtx_t;

static bool _runModeHandle(const diagMsgHead_t *cmd, void *ctx)
{
    uint32_t sc = osiEnterCritical();
    uint8_t mode = DIAG_RM_NORMAL;
    rmCtx_t *rc = (rmCtx_t *)ctx;

    diagUnregisterCmdHandle(MSG_RUNMODE);

    if ((cmd->subtype & 0xF0) == 0x80)
        mode = cmd->subtype & 0x0F;

    // CALIB MODE
    if (cmd->subtype == 0x90)
        mode = DIAG_RM_CALIB;
    // BBAT MODE
    if (cmd->subtype == 0x95)
        mode = DIAG_RM_BBAT;

    rc->mode = mode;

    if (rc->diagDeviceUSB)
        diagOutputPacket(cmd, 8);

    osiSemaphoreRelease(rc->sema);
    osiExitCritical(sc);

    return true;
}

static bool _checkDiagDevice(rmCtx_t *rc, uint32_t timeout)
{
    if (diagDeviceType() == DIAG_DEVICE_UART)
    {
        rc->diagDeviceUSB = false;
        return true;
    }
#ifdef CONFIG_DIAG_DEFAULT_USERIAL
    else // usb serial
    {
        rc->diagDeviceUSB = true;
        if (gSysnvUsbDetMode == USB_DETMODE_CHARGER &&
            drvChargerGetType() == DRV_CHARGER_TYPE_NONE)
            return false;

        if (!drvUsbSetWorkMode(DRV_USB_NPI_SERIAL))
            return false;

        drvUsbEnable(0);
        int64_t start = osiUpTime();
        while (osiUpTime() - start < timeout)
        {
            if (drvSerialCheckReadyByName(CONFIG_DIAG_DEFAULT_USERIAL))
            {
                OSI_LOGI(0, "diag usb device connected in %u milliseconds", (unsigned)(osiUpTime() - start));
                return true;
            }
            osiThreadSleep(2);
        }
    }
#endif
    return false;
}

diagRm_t diagRmCheck(uint32_t ms)
{
    rmCtx_t rc = {.mode = DIAG_RM_NORMAL};
    rc.sema = osiSemaphoreCreate(1, 0);
    if (rc.sema == NULL)
        return DIAG_RM_NORMAL;

    diagRegisterCmdHandle(MSG_RUNMODE, _runModeHandle, &rc);
    if (!_checkDiagDevice(&rc, ms))
    {
        OSI_LOGI(0, "diag device open fail");
        goto done;
    }

    if (!rc.diagDeviceUSB)
    {
        // send ping message if use uart diag channel
        diagMsgHead_t h;
        h.seq_num = 0;
        h.len = 8;
        h.type = MSG_RUNMODE;
        h.subtype = 0x01;
        diagOutputPacket(&h, 8);
    }

    osiSemaphoreTryAcquire(rc.sema, ms);

done:
    diagUnregisterCmdHandle(MSG_RUNMODE);
    osiSemaphoreDelete(rc.sema);

    OSI_LOGI(0, "diag device %d run mode: %d", rc.diagDeviceUSB, rc.mode);
#ifdef CONFIG_DIAG_DEFAULT_USERIAL
    if (diagDeviceType() == DIAG_DEVICE_USERIAL && rc.mode != DIAG_RM_NORMAL)
    {
        diagWaitWriteFinish(20);
        drvUsbDisable();
        osiThreadSleep(500);
        drvUsbEnable(0);
    }
    else
    {
        drvUsbDisable();
    }
#endif

    return rc.mode;
}
