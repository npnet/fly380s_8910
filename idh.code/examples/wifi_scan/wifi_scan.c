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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('W', 'I', 'F', 'I')

#include "osi_api.h"
#include "osi_log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "drv_wifi.h"

static int prvWifiApInfoCompare(const void *ctx1, const void *ctx2)
{
    const wifiApInfo_t *w1 = (const wifiApInfo_t *)ctx1;
    const wifiApInfo_t *w2 = (const wifiApInfo_t *)ctx2;
    return (w1->rssival - w2->rssival);
}

static void prvWifiScanAllChannel(drvWifi_t *w, uint32_t scan_round)
{
    OSI_LOGI(0, "wifi scan all channel %u rounds", scan_round);
    wifiApInfo_t *aps = (wifiApInfo_t *)malloc(25 * sizeof(wifiApInfo_t));
    wifiScanRequest_t req = {};
    req.aps = aps;
    req.max = 25;
    req.found = 0;
    req.maxtimeout = 300;

    bool r = drvWifiScanAllChannel(w, &req, scan_round);
    if (!r)
    {
        OSI_LOGE(0, "wifi scan all channel fail");
    }
    else if (req.found != 0)
    {
        qsort(&req.aps[0], req.found, sizeof(wifiApInfo_t), prvWifiApInfoCompare);
        for (uint32_t i = 0; i < req.found; i++)
        {
            wifiApInfo_t *w = &req.aps[i];
            OSI_LOGI(0, "found ap - {mac address: %x%lx, rssival: %i dBm, channel: %u}",
                     w->bssid_high, w->bssid_low, w->rssival, w->channel);
        }
    }
    free(aps);
}

static void prvWifiScanChannel(drvWifi_t *w, uint8_t channel, uint32_t timeout)
{
    OSI_LOGI(0, "wifi scan channel %u/%u", channel, timeout);
    wifiApInfo_t *aps = (wifiApInfo_t *)malloc(25 * sizeof(wifiApInfo_t));
    wifiScanRequest_t req = {};
    req.aps = aps;
    req.max = 25;
    req.found = 0;
    req.maxtimeout = timeout;

    bool r = drvWifiScanChannel(w, &req, channel);
    if (!r)
    {
        OSI_LOGE(0, "wifi scan channel %u fail", channel);
    }
    else if (req.found != 0)
    {
        qsort(&req.aps[0], req.found, sizeof(wifiApInfo_t), prvWifiApInfoCompare);
        for (uint32_t i = 0; i < req.found; i++)
        {
            wifiApInfo_t *w = &req.aps[i];
            OSI_LOGI(0, "found ap - {mac address : %x%lx, rssival: %i dBm, channel: %u}",
                     w->bssid_high, w->bssid_low, w->rssival, w->channel);
        }
    }
    free(aps);
}

static void prvWifiScanThread(void *param)
{
    drvWifi_t *w = drvWifiOpen();
    if (w == NULL)
    {
        OSI_LOGE(0, "wifi scan example create wifi context fail");
        osiThreadExit();
        return;
    }

    for (;;)
    {
        osiThreadSleep(2000);
        for (unsigned i = 1; i <= 13; i++)
        {
            prvWifiScanChannel(w, i, 500);
            osiThreadSleep(2000);
        }

        prvWifiScanAllChannel(w, 1);
        osiThreadSleep(2000);

        prvWifiScanAllChannel(w, 2);
    }

    drvWifiClose(w);
    osiThreadExit();
}

int appimg_enter(void *param)
{
    OSI_LOGI(0, "wifi scan example enter");
    osiThreadCreate("wifi", prvWifiScanThread, NULL, OSI_PRIORITY_NORMAL, 1024, 1);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "wifi scan example exit");
}