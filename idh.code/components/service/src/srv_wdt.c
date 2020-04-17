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

#include "drv_wdt.h"
#include "osi_api.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t max_interval;
    uint32_t feed_interval;
    osiTimer_t *feed_timer;
} srvWdt_t;

static srvWdt_t gSrvWdt;

static void _feedDog(void *wd_)
{
    srvWdt_t *wd = (srvWdt_t *)wd_;
    drvWatchdogReload(wd->max_interval);
    osiTimerStartRelaxed(wd->feed_timer, wd->feed_interval, OSI_WAIT_FOREVER);
}

void srvWdtEnterDeepSleep(int64_t reset_ms)
{
    srvWdt_t *wd = &gSrvWdt;
    drvWatchdogReload((uint32_t)reset_ms);
    osiTimerStop(wd->feed_timer);
}

void srvWdtExitDeepSleep(void)
{
    srvWdt_t *wd = &gSrvWdt;
    drvWatchdogReload(wd->max_interval);
    osiTimerStartRelaxed(wd->feed_timer, wd->feed_interval, OSI_WAIT_FOREVER);
}

bool srvWdtInit(uint32_t max_interval_ms, uint32_t feed_interval_ms)
{
    srvWdt_t *wd = &gSrvWdt;
    if (max_interval_ms < feed_interval_ms)
        return false;

    osiTimer_t *timer = osiTimerCreate(NULL, _feedDog, (void *)wd);
    if (timer == NULL)
        return false;

    wd->max_interval = max_interval_ms;
    wd->feed_interval = feed_interval_ms;
    wd->feed_timer = timer;

    bool r = drvWatchdogStart(max_interval_ms);
    if (!r)
    {
        osiTimerDelete(timer);
        return false;
    }

    osiTimerStartRelaxed(wd->feed_timer, wd->feed_interval, OSI_WAIT_FOREVER);
    return true;
}
