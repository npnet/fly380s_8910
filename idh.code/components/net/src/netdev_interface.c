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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('N', 'D', 'E', 'V')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_INFO

#include "cfw.h"
#include "osi_api.h"
#include "osi_log.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "lwip/tcpip.h"

#include "drv_ps_path.h"
#include "netdev_interface.h"
#include "drv_ether.h"
#include "hal_chip.h"

#ifndef GPRS_MTU
#define GPRS_MTU 1500
#endif

typedef struct
{
    drvPsIntfDataArriveCB_t old_psintf_cb;
    struct netif *dev_netif;
    int is_created;
} netSession_t;

typedef struct
{
    drvEther_t *ether;
    netSession_t *session;
    osiTimer_t *connect_timer;
    bool netdevIsConnected;
    osiClockConstrainRegistry_t clk_registry;
} netdevIntf_t;

extern void lwip_pspathDataInput(void *ctx, drvPsIntf_t *p);
extern osiThread_t *netGetTaskID();
extern bool isDhcpPackage(drvEthPacket_t *pkt, uint32_t size);
extern void dhcps_reply(drvEther_t *ether, drvEthPacket_t *pkt, uint32_t size, struct netif *netif);
extern bool isARPPackage(drvEthPacket_t *pkt, uint32_t size);
extern void ARP_reply(drvEther_t *ether, drvEthPacket_t *pkt, uint32_t size, struct netif *netif);
extern bool isRouterSolicitationPackage(drvEthPacket_t *pkt, uint32_t size);
extern void routerAdvertisement_reply(drvEther_t *ether, drvEthPacket_t *pkt, uint32_t size, struct netif *netif);
extern bool isRouterAdvertisementPackage(drvEthPacket_t *pkt, uint32_t size);
extern bool isNeighborSolicitationPackage(drvEthPacket_t *pkt, uint32_t size);
extern void neighborAdvertisement_reply(drvEther_t *ether, drvEthPacket_t *pkt, uint32_t size, struct netif *netif);

static bool prvNdevDataToPs(netSession_t *session, const void *data, size_t size);
static netSession_t *prvNdevSessionCreate();
static bool prvNdevSessionDelete(netSession_t *session);

static netdevIntf_t gNetIntf = {.clk_registry = {.tag = HAL_NAME_NETCARD}};

static void prvEthEventCB(uint32_t evt_id, void *ctx)
{
    netdevIntf_t *nc = (netdevIntf_t *)ctx;
    OSI_LOGI(0, "Ethernet event cb %d (e: %p, s: %p)", evt_id, nc->ether, nc->session);
    if (DRV_ETHER_EVENT_CONNECT == evt_id)
    {
        osiEvent_t tcpip_event = {0};
        tcpip_event.id = EV_TCPIP_ETHCARD_CONNECT;
        osiEventSend(netGetTaskID(), (const osiEvent_t *)&tcpip_event);
        nc->netdevIsConnected = true;
        osiRequestSysClkActive(&nc->clk_registry);
        OSI_LOGI(0, "net card EV_TCPIP_ETHCARD_CONNECT");
    }

    if (DRV_ETHER_EVENT_DISCONNECT == evt_id)
    {
        osiEvent_t tcpip_event = {0};
        tcpip_event.id = EV_TCPIP_ETHCARD_DISCONNECT;
        osiEventSend(netGetTaskID(), (const osiEvent_t *)&tcpip_event);
        nc->netdevIsConnected = false;
        osiReleaseClk(&nc->clk_registry);
        OSI_LOGI(0, "net card EV_TCPIP_ETHCARD_DISCONNECT");
    }
}

static void prvEthUploadDataCB(drvEthPacket_t *pkt, uint32_t size, void *ctx)
{
    netdevIntf_t *nc = (netdevIntf_t *)ctx;
    netSession_t *session = nc->session;

    if (session == NULL)
    {
        OSI_LOGE(0, "upload data no session");
        return;
    }

    if (size > ETH_HLEN)
    {
        OSI_LOGE(0, "prvEthUploadDataCB data size: %d", size - ETH_HLEN);
        uint8_t *ipData = pkt->data;
        sys_arch_dump(ipData, size - ETH_HLEN);
    }

    if (session->dev_netif != NULL && isARPPackage(pkt, size)) //drop ARP package
    {
        ARP_reply(nc->ether, pkt, size, session->dev_netif);
        return;
    }

    if (session->dev_netif != NULL && isNeighborSolicitationPackage(pkt, size))
    {
        neighborAdvertisement_reply(nc->ether, pkt, size, session->dev_netif);
        return;
    }

    bool isDHCPData = isDhcpPackage(pkt, size);
    if (session->dev_netif != NULL && isDHCPData)
    {
        dhcps_reply(nc->ether, pkt, size, session->dev_netif);
    }
    else
    {
        prvNdevDataToPs(session, pkt->data, size - ETH_HLEN);
    }
}

static void prvNdevProcessPsData(void *ctx)
{
    netdevIntf_t *nc = &gNetIntf;
    struct netif *nif = (struct netif *)ctx;
    if (nif == NULL)
    {
        OSI_LOGE(0, "NC PsIntf get netif failed");
        osiPanic();
    }

    int rsize = 0;
    do
    {
        drvEthReq_t *tx_req;
        while ((tx_req = drvEtherTxReqAlloc(nc->ether)) == NULL)
        {
            osiThreadSleep(1);
            if (nc->ether == NULL)
                return;
        }

        uint8_t *rdata = tx_req->payload->data;
        rsize = drvPsIntfRead(nif->pspathIntf, rdata, 1600);
        if (rsize <= 0)
        {
            drvEtherTxReqFree(nc->ether, tx_req);
        }
        if (rsize > 0)
        {
            nif->u32RndisDLSize += rsize;
            OSI_LOGE(0, "PsIntfRead %d", rsize);
            if (!drvEtherTxReqSubmit(nc->ether, tx_req, rsize))
            {
                drvEtherTxReqFree(nc->ether, tx_req);
                OSI_LOGW(0, "PS to NC error %d", rsize);
                return;
            }
        }
    } while (rsize > 0);
}

static void prvPsDataToNdev(void *ctx, drvPsIntf_t *p)
{
    osiThreadCallback(netGetTaskID(), prvNdevProcessPsData, (void *)ctx);
}

static bool prvNdevDataToPs(netSession_t *session, const void *data, size_t size)
{
    struct netif *inp_netif = session->dev_netif;
    if (inp_netif == NULL)
        return false;

    if (size <= 0)
        return false;

    int written = 0x00;

    extern bool ATGprsGetDPSDFlag(CFW_SIM_ID nSim);
#define GET_SIM(sim_cid) (((sim_cid) >> 4) & 0xf)

    if (!ATGprsGetDPSDFlag(GET_SIM(inp_netif->sim_cid)))
        written = drvPsIntfWrite(inp_netif->pspathIntf, data, size);
    else
        written = size;

    if (written < 0)
    {
        OSI_LOGI(0, "written: %d will osiPanic", written);
        return false;
    }
    else
    {
        if (written == size)
        {
            OSI_LOGE(0, "PsIntfWrite %d", size);
            inp_netif->u32RndisULSize += written;
        }
        else
            OSI_LOGW(0, "PsIntfWrite error %d %d", size, written);
    }
    return true;
}

static void prvProcessNdevConnect(void *par)
{
    netdevIntf_t *nc = (netdevIntf_t *)par;
    OSI_LOGI(0, "netdevConnect (ether %p, session %p)", nc->ether, nc->session);
    if (nc->ether == NULL)
        return;

    if (nc->session == NULL)
        nc->session = prvNdevSessionCreate();

    if (nc->session == NULL)
        return;

    drvEtherSetULDataCB(nc->ether, prvEthUploadDataCB, nc);
    if (!drvEtherOpen(nc->ether))
    {
        OSI_LOGI(0, "NC connect ether open fail");
        prvNdevSessionDelete(nc->session);
        nc->session = NULL;
    }
}

void netdevConnect()
{
    OSI_LOGI(0, "netdevConnect timer start.");
    netdevIntf_t *nc = &gNetIntf;
    if (nc->connect_timer == NULL)
        nc->connect_timer = osiTimerCreate(netGetTaskID(), prvProcessNdevConnect, nc);
    osiTimerStart(nc->connect_timer, 3000);
}

void netdevDisconnect()
{
    netdevIntf_t *nc = &gNetIntf;
    drvEtherClose(nc->ether);
    prvNdevSessionDelete(nc->session);
    nc->session = NULL;
    if (nc->connect_timer != NULL)
    {
        osiTimerDelete(nc->connect_timer);
        nc->connect_timer = NULL;
    }
}

void netdevNetUp()
{
    OSI_LOGI(0, "netdevNetUp");
    netdevIntf_t *nc = &gNetIntf;
    if (nc->ether == NULL)
        return;

    if (nc->session == NULL)
        nc->session = prvNdevSessionCreate();

    if (nc->session == NULL)
        return;

    drvEtherSetULDataCB(nc->ether, prvEthUploadDataCB, nc);
    if (!drvEtherOpen(nc->ether))
    {
        OSI_LOGI(0, "NC net up ether open fail.");
        prvNdevSessionDelete(nc->session);
        nc->session = NULL;
        return;
    }
}

void netdevNetDown(uint8_t nSimID, uint8_t nCID)
{
    netdevIntf_t *nc = &gNetIntf;
    OSI_LOGI(0, "pre netdevNetDown");
    if ((nc->netdevIsConnected == true) && (nc->session != NULL) && (nc->session->is_created == true))
    {
        struct netif *inp_netif = nc->session->dev_netif;
        if ((inp_netif != NULL) && ((nSimID << 4 | nCID) == inp_netif->sim_cid))
        {
            OSI_LOGI(0, "netdevNetDown");
            drvEtherClose(nc->ether);
            prvNdevSessionDelete(nc->session);
            nc->session = NULL;
        }
    }
}

bool netdevIsConnected()
{
    return gNetIntf.netdevIsConnected;
}

void netdevExit()
{
    OSI_LOGI(0, "NC exit");
    uint32_t critical = osiEnterCritical();
    netdevIntf_t *nc = &gNetIntf;
    drvEtherSetEventCB(nc->ether, NULL, NULL);
    nc->ether = NULL;
    nc->netdevIsConnected = false;
    osiExitCritical(critical);
}

void netdevInit(drvEther_t *ether)
{
    OSI_LOGI(0, "netdevInit");
    uint32_t critical = osiEnterCritical();
    gNetIntf.ether = ether;
    drvEtherSetEventCB(ether, prvEthEventCB, &gNetIntf);
    osiExitCritical(critical);
}

netSession_t *prvNdevSessionCreate()
{
    static netSession_t s_net_session;
    OSI_LOGI(0, "Net session create");
    uint32_t critical = osiEnterCritical();
    if (netif_default == NULL || netif_default->pspathIntf == NULL || netif_default->link_mode != NETIF_LINK_MODE_LWIP)
    {
        osiExitCritical(critical);
        OSI_LOGE(0, "Net session create no netif_default %p", netif_default);
        return NULL;
    }

    netSession_t *session = &s_net_session;
    if (session->is_created == true)
    {
        osiExitCritical(critical);
        OSI_LOGI(0, "Net session create already created");
        return session;
    }
    memset(session, 0, sizeof(netSession_t));
    session->dev_netif = netif_default;
    session->old_psintf_cb = drvPsIntfSetDataArriveCB(session->dev_netif->pspathIntf, prvPsDataToNdev);
    OSI_LOGI(0, "Net end session %x\n", session);
    session->dev_netif->link_mode = NETIF_LINK_MODE_NETCARD;
    netif_default = NULL;
#if !LWIP_SINGLE_NETIF
    if (netif_list != NULL)
    {
        struct netif *next_netif = netif_list;
        while (next_netif != NULL)
        {
            if ((next_netif->name[0] != 'l') && (next_netif->name[1] != 'o') && (next_netif->link_mode == NETIF_LINK_MODE_LWIP))
            {
                netif_set_default(next_netif);
                break;
            }
            next_netif = next_netif->next;
        }
    }
#endif
    session->is_created = true;
    osiExitCritical(critical);
    return session;
}

bool prvNdevSessionDelete(netSession_t *session)
{
    uint32_t critical = osiEnterCritical();
    if (session == NULL)
    {
        osiExitCritical(critical);
        return false;
    }

    if (session->dev_netif != NULL)
    {
        if (session->dev_netif->pspathIntf != NULL)
            drvPsIntfSetDataArriveCB(session->dev_netif->pspathIntf, lwip_pspathDataInput);
        session->dev_netif->link_mode = NETIF_LINK_MODE_LWIP;
    }
#if !LWIP_SINGLE_NETIF
    if (netif_default == NULL && netif_list != NULL)
    {
        struct netif *next_netif = netif_list;
        netif_default = NULL;
        while (next_netif != NULL)
        {
            if ((next_netif->name[0] != 'l') && (next_netif->name[1] != 'o') && (next_netif->link_mode == NETIF_LINK_MODE_LWIP))
            {
                netif_set_default(next_netif);
                break;
            }
            next_netif = next_netif->next;
        }
    }
#endif
    session->is_created = false;
    osiExitCritical(critical);
    return true;
}
