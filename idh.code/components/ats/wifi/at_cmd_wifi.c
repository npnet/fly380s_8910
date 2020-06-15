/* Copyright (C) 2020 RDA Technologies Limited and/or its affiliates("RDA").
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
#include "osi_api.h"
#include "at_command.h"
#include "at_engine.h"
#include "at_response.h"
#include "drv_wifi.h"

#include <stdlib.h>
#include <stdio.h>

#define WIFI_SCAN_MAX_TIME (5000)
#define WIFI_SCAN_DEFAULT_TIME (600)
#define WIFI_SCAN_MAX_AP_CNT (300)
#define WIFI_SCAN_DEFAULT_AP_CNT (100)
#define WIFI_SCAN_DEFAULT_ROUND (1)

static drvWifi_t *gDrvWifi = NULL;

void atCmdHandleWifiOpen(atCommand_t *cmd)
{
    if (cmd->type == AT_CMD_EXE)
    {
        if (gDrvWifi != NULL)
        {
            OSI_LOGI(0, "wifi already open");
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
            return;
        }

        gDrvWifi = drvWifiOpen();
        if (gDrvWifi == NULL)
        {
            OSI_LOGE(0, "wifi open create wifi context fail");
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }

        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

void atCmdHandleWifiClose(atCommand_t *cmd)
{
    if (cmd->type == AT_CMD_EXE)
    {
        if (gDrvWifi == NULL)
        {
            OSI_LOGI(0, "wifi already close");
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
            return;
        }

        drvWifiClose(gDrvWifi);
        gDrvWifi = NULL;
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

static int prvWifiApInfoCompare(const void *ctx1, const void *ctx2)
{
    const wifiApInfo_t *w1 = (const wifiApInfo_t *)ctx1;
    const wifiApInfo_t *w2 = (const wifiApInfo_t *)ctx2;
    return (w1->rssival - w2->rssival);
}

void atCmdHandleWifiScan(atCommand_t *cmd)
{
    if (cmd->type == AT_CMD_READ)
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        char resp[40];
        snprintf(resp, 40, "+WIFISCAN: <0-13>, <timeout>, <max_ap>");
        atCmdRespInfoText(cmd->engine, resp);
        RETURN_OK(cmd->engine);
    }

    drvWifi_t *d = gDrvWifi;
    if (d == NULL)
    {
        OSI_LOGE(0, "wifi scan open device first");
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
    }

    uint8_t ch = 0;
    uint32_t timeout = WIFI_SCAN_DEFAULT_TIME;
    uint32_t maxap = WIFI_SCAN_DEFAULT_AP_CNT;
    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        ch = atParamDefUintInRange(cmd->params[0], 0, 0, 13, &paramok);
        if (!paramok)
            goto param_end;

        timeout = atParamDefUintInRange(cmd->params[1], WIFI_SCAN_DEFAULT_TIME, 120, WIFI_SCAN_MAX_TIME, &paramok);
        if (!paramok)
            goto param_end;

        maxap = atParamDefUintInRange(cmd->params[2], WIFI_SCAN_DEFAULT_AP_CNT, 1, WIFI_SCAN_MAX_AP_CNT, &paramok);
    param_end:
        if (!paramok)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
    }

    OSI_LOGI(0, "wifi scan channel %u/%u/%u", ch, timeout, maxap);
    wifiApInfo_t *aps = (wifiApInfo_t *)calloc(maxap, sizeof(wifiApInfo_t));
    if (aps == NULL)
    {
        OSI_LOGE(0, "wifi sacn fail allocate memory %u", maxap * sizeof(wifiApInfo_t));
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);
    }

    wifiScanRequest_t req = {
        .aps = aps,
        .max = maxap,
        .found = 0,
        .maxtimeout = timeout,
    };

    bool r;
    if (ch != 0)
        r = drvWifiScanChannel(d, &req, ch);
    else
        r = drvWifiScanAllChannel(d, &req, WIFI_SCAN_DEFAULT_ROUND);

    if (r)
    {
        qsort(&req.aps[0], req.found, sizeof(wifiApInfo_t), prvWifiApInfoCompare);
        char resp[64];
        for (uint32_t i = 0; i < req.found; ++i)
        {
            wifiApInfo_t *w = &req.aps[i];
            snprintf(resp, 64, "%x%lx, %i, %u", w->bssid_high, w->bssid_low, w->rssival, w->channel);
            atCmdRespInfoText(cmd->engine, resp);
        }
    }

    free(aps);
    if (!r)
    {
        OSI_LOGE(0, "WIFI Scan start fail");
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
    }
    else
    {
        RETURN_OK(cmd->engine);
    }
}
