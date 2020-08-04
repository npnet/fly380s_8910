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

#include "drv_wifi.h"
#include "hwregs.h"
#include "osi_api.h"
#include <stdlib.h>

#include "drv_wcn.h"

#define WIFI_SCAN_MIN_INTERVAL (120)
#define WIFI_RSSI_CALIB_OFFSET (28)

typedef struct
{
    bool scan_all;
    wifiScanRequest_t *req;
    union {
        uint32_t channel;
        uint32_t round;
    };
} scanCtx_t;

struct drv_wifi
{
    osiWorkQueue_t *work_queue;
    osiWork_t *work_scan;
    osiMutex_t *lock;
    osiSemaphore_t *sema_sync;
    drvWcn_t *wcn;
    scanCtx_t context;
};

static void prvWifiOpen()
{
    hwp_wcnRfIf->sys_control = 0x267;
    hwp_wcnWlan->config_reg = 0x802;
}

static void prvWifiClose()
{
    hwp_wcnRfIf->wf_control = 0x2;
    hwp_wcnRfIf->sys_control = 0x63;
}

static void prvWifiSetChannel(uint8_t ch)
{
    REG_WCN_RF_IF_WF_CONTROL_T wf_control = {};
    wf_control.b.wf_tune = 0;
    wf_control.b.wf_chn = ch;
    hwp_wcnRfIf->wf_control = wf_control.v;
    osiThreadSleep(1);
    wf_control.b.wf_tune = 1;
    hwp_wcnRfIf->wf_control = wf_control.v;
}

static bool prvWifiDataReady()
{
    REG_WCN_WLAN_DATARDYINT_T dataready = {hwp_wcnWlan->datardyint};
    return dataready.b.datardyint == 1;
}

static void prvWifiClearReady()
{
    REG_WCN_WLAN_CONFIG_REG_T conf_reg = {hwp_wcnWlan->config_reg};
    conf_reg.b.apb_clear = 1;
    hwp_wcnWlan->config_reg = conf_reg.v;
    REG_WAIT_FIELD_NEZ(conf_reg, hwp_wcnWlan->config_reg, apb_clear);

    conf_reg.b.apb_clear = 0;
    hwp_wcnWlan->config_reg = conf_reg.v;
    REG_WAIT_FIELD_EQZ(conf_reg, hwp_wcnWlan->config_reg, apb_clear);
}

static inline void prvLoadApInfo(wifiApInfo_t *info)
{
    REG_WCN_WLAN_CONFIG_REG_T conf_reg = {hwp_wcnWlan->config_reg};
    conf_reg.b.apb_hold = 1;
    hwp_wcnWlan->config_reg = conf_reg.v;
    REG_WAIT_FIELD_NEZ(conf_reg, hwp_wcnWlan->config_reg, apb_hold);

    info->bssid_low = hwp_wcnWlan->bssidaddr_l;
    info->bssid_high = (hwp_wcnWlan->bssidaddr_h & 0xffff);
    info->rssival = (int8_t)hwp_wcnWlan->rssival + WIFI_RSSI_CALIB_OFFSET;

    conf_reg.b.apb_hold = 0;
    hwp_wcnWlan->config_reg = conf_reg.v;
    REG_WAIT_FIELD_EQZ(conf_reg, hwp_wcnWlan->config_reg, apb_hold);
}

static bool prvSaveApInfo(wifiApInfo_t *info, wifiScanRequest_t *req)
{
    if (req->found >= req->max)
        return false;

    bool new_ap = true;
    for (unsigned i = req->found; i > 0; --i)
    {
        wifiApInfo_t *s = &req->aps[i - 1];
        if (s->bssid_high == info->bssid_high && s->bssid_low == info->bssid_low)
        {
            if (info->rssival > s->rssival)
            {
                s->rssival = info->rssival;
                s->channel = info->channel;
            }
            new_ap = false;
            break;
        }
    }

    if (new_ap)
        req->aps[req->found++] = *info;

    return true;
}

static void prvWifiScanChannel_(uint8_t ch, uint32_t timeout, wifiScanRequest_t *req)
{
    osiElapsedTimer_t elapsed;
    osiElapsedTimerStart(&elapsed);
    while (osiElapsedTime(&elapsed) < timeout)
    {
        if (prvWifiDataReady())
        {
            wifiApInfo_t info = {.channel = ch};
            prvLoadApInfo(&info);
            prvWifiClearReady();
            if (!prvSaveApInfo(&info, req))
                return;
        }
    }
}

static void prvWifiScanChannel(uint8_t ch, uint32_t timeout, wifiScanRequest_t *req)
{
    uint32_t tout1 = WIFI_SCAN_MIN_INTERVAL;
    if (tout1 > timeout)
        tout1 = timeout;
    uint32_t tout2 = timeout - tout1;

    prvWifiSetChannel(ch);
    prvWifiScanChannel_(ch, tout1, req);

    // the broadcast interval from wifi ap is 100 ms, if got none
    // during 120 ms, we can presume there is no ap on this channel
    if (req->found != 0 && tout2 != 0 && req->found < req->max)
    {
        prvWifiScanChannel_(ch, tout2, req);
    }
}

static void prvWifiScan(void *p)
{
    drvWifi_t *d = (drvWifi_t *)p;
    prvWifiOpen();
    if (d->context.scan_all)
    {
        for (unsigned n = 0; n < d->context.round; ++n)
        {
            for (uint8_t c = 1; c <= WIFI_CHANNEL_MAX; ++c)
                prvWifiScanChannel(c, d->context.req->maxtimeout, d->context.req);
        }
    }
    else
    {
        prvWifiScanChannel(d->context.channel, d->context.req->maxtimeout, d->context.req);
    }

    prvWifiClose();
    osiSemaphoreRelease(d->sema_sync);
}

static void prvWifiDestroy(drvWifi_t *w)
{
    osiWorkQueueDelete(w->work_queue);
    osiWorkDelete(w->work_scan);
    osiMutexDelete(w->lock);
    osiSemaphoreDelete(w->sema_sync);
    free(w);
}

drvWifi_t *drvWifiOpen()
{
    drvWifi_t *d = (drvWifi_t *)calloc(1, sizeof(drvWifi_t));
    if (d == NULL)
        return NULL;

    d->work_queue = osiWorkQueueCreate("wcn-wifi", 1, OSI_PRIORITY_NORMAL, 1024);
    if (d->work_queue == NULL)
        goto fail;

    d->work_scan = osiWorkCreate(prvWifiScan, NULL, d);
    if (d->work_scan == NULL)
        goto fail;

    d->lock = osiMutexCreate();
    if (d->lock == NULL)
        goto fail;

    d->sema_sync = osiSemaphoreCreate(1, 0);
    if (d->sema_sync == NULL)
        goto fail;

    d->wcn = drvWcnOpen(WCN_USER_WIFI);

    return d;

fail:

    prvWifiDestroy(d);
    return NULL;
}

void drvWifiClose(drvWifi_t *d)
{
    if (d)
    {
        drvWcnClose(d->wcn);
        prvWifiDestroy(d);
    }
}

bool drvWifiScanAllChannel(drvWifi_t *d, wifiScanRequest_t *req, uint32_t round)
{
    osiMutexLock(d->lock);
    osiSemaphoreTryAcquire(d->sema_sync, 0);
    drvWcnRequest(d->wcn);

    d->context.scan_all = true;
    d->context.req = req;
    d->context.round = round;
    d->context.req->found = 0;
    bool result = osiWorkEnqueue(d->work_scan, d->work_queue);
    if (result)
    {
        osiSemaphoreAcquire(d->sema_sync);
    }
    drvWcnRelease(d->wcn);
    osiMutexUnlock(d->lock);
    return result;
}

bool drvWifiScanChannel(drvWifi_t *d, wifiScanRequest_t *req, uint32_t channel)
{
    if (d == NULL || req == NULL)
        return false;
    if (channel < 1 || channel > WIFI_CHANNEL_MAX)
        return false;

    osiMutexLock(d->lock);
    osiSemaphoreTryAcquire(d->sema_sync, 0);
    drvWcnRequest(d->wcn);

    d->context.scan_all = false;
    d->context.req = req;
    d->context.channel = channel;
    d->context.req->found = 0;
    bool result = osiWorkEnqueue(d->work_scan, d->work_queue);
    if (result)
    {
        osiSemaphoreAcquire(d->sema_sync);
    }
    drvWcnRelease(d->wcn);
    osiMutexUnlock(d->lock);
    return result;
}
