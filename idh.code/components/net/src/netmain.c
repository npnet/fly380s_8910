/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
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
#include "osi_api.h"
#include "osi_log.h"
#include "cfw_dispatch.h"
#include "cfw.h"
#include "cfw_event.h"
#include "tcpip.h"
#include "lwipopts.h"
#include "netmain.h"
#include "ppp_interface.h"
#include "netdev_interface.h"

osiThread_t *netThreadID = NULL;

extern void tcpip_thread(void *arg);

#ifdef CONFIG_SOC_8910
extern int vnet4gSelfRegister(uint8_t nCid, uint8_t nSimId);
#endif

static void net_thread(void *arg)
{

    for (;;)
    {
        osiEvent_t event = {};
        osiEventWait(osiThreadCurrent(), &event);
        if (event.id == 0)
            continue;
        OSI_LOGI(0, "Netthread get a event: 0x%08x/0x%08x/0x%08x/0x%08x", (unsigned int)event.id, (unsigned int)event.param1, (unsigned int)event.param2, (unsigned int)event.param3);

        OSI_LOGI(0, "Netthread switch");
        if ((!cfwIsCfwIndicate(event.id)) && (cfwInvokeUtiCallback(&event)))
        {
            ; // handled by UTI
        }
        else
        {
            CFW_EVENT *cfw_event = (CFW_EVENT *)&event;
            switch (event.id)
            {
            case EV_INTER_APS_TCPIP_REQ:
            {
                struct tcpip_msg *msg;
                msg = (struct tcpip_msg *)event.param1;
                tcpip_thread(msg);
                break;
            }
            case EV_TCPIP_CFW_GPRS_ACT:
            {
                uint8_t nCid, nSimId;
                nCid = event.param1;
                nSimId = cfw_event->nFlag;
                TCPIP_netif_create(nCid, nSimId);

#ifdef CONFIG_SOC_8910
                vnet4gSelfRegister(nCid, nSimId); //dianxin4G×Ô×¢²á
                if (netdevIsConnected())
                {
                    netdevNetUp();
                }
#endif
                break;
            }
            case EV_TCPIP_CFW_GPRS_DEACT:
            {
                uint8_t nCid, nSimId;
                nCid = event.param1;
                nSimId = cfw_event->nFlag;
#ifdef CONFIG_SOC_8910
                if (netdevIsConnected())
                {
                    netdevNetDown(nSimId, nCid);
                }
#endif
                TCPIP_netif_destory(nCid, nSimId);
                break;
            }
#ifdef CONFIG_SOC_8910
            case EV_TCPIP_ETHCARD_CONNECT:
            {
                netdevConnect();
                break;
            }
            case EV_TCPIP_ETHCARD_DISCONNECT:
            {
                netdevDisconnect();
                break;
            }
#endif
            default:
                break;
            }
        }
    }
}

osiThread_t *netGetTaskID()
{
    return netThreadID;
}

void net_init()
{

    netThreadID = osiThreadCreate("net", net_thread, NULL, OSI_PRIORITY_NORMAL, 8192 * 4, 64);

    tcpip_init(NULL, NULL);
}
