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
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_INFO
#include "drv_keypad.h"
#include "osi_api.h"
#include "osi_log.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "diag_auto_test.h"
#include "poc.h"

#define POWER_KEY_PRESSED_TIMEOUT (3000) // ms

typedef struct app_key
{
    bool ready_shutdown;
    osiTimer_t *pwrkey_timer;
} appKey_t;

static void _appPwrKeyTimer(void *p)
{
    appKey_t *key = (appKey_t *)p;
    key->ready_shutdown = true;
}

static void _appKeyCB(keyMap_t id, keyState_t evt, void *p)
{
    appKey_t *key = (appKey_t *)p;
    uint8_t status;

    if (evt & KEY_STATE_PRESS)
        status = 1;
    if (evt & KEY_STATE_RELEASE)
        status = 0;

    if (osiGetBootMode() == OSI_BOOTMODE_BBAT)
    {
        drvSetPBkeyStatus((uint32_t)evt);
        return;
    }
    if (id == KEY_MAP_POWER)
    {
        if (evt == KEY_STATE_PRESS)
        {
            osiTimerStart(key->pwrkey_timer, POWER_KEY_PRESSED_TIMEOUT);
        }
        else
        {
            if (key->ready_shutdown)
            {

                osiDelayUS(1000 * 100);
                osiShutdown(OSI_SHUTDOWN_POWER_OFF);
            }
            else
                osiTimerStop(key->pwrkey_timer);
        }
    }
    else
#ifdef CONFIG_POC_KEYPAD_SUPPORT
    if(!pocKeypadHandle(id, evt, p))
#endif
    {
        OSI_LOGI(0, "key id %d,status %d", id, status);
    }
}

void appKeypadInit()
{
    appKey_t *key = (appKey_t *)calloc(1, sizeof(appKey_t));
    if (key == NULL)
        return;
    key->pwrkey_timer = osiTimerCreate(NULL, _appPwrKeyTimer, key);
    if (key->pwrkey_timer == NULL)
    {
        free(key);
        return;
    }

    key->ready_shutdown = false;

    drvKeypadInit();
    // power key
    uint32_t mask = KEY_STATE_PRESS | KEY_STATE_RELEASE;
    drvKeypadSetCB(_appKeyCB, mask, key);
}
