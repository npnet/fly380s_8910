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

#include "netdev_interface_imp.h"
#include "netdev_interface.h"

#if IP_NAT
#include "netdev_interface_nat_lan.h"
#include "lwip/ip4_nat.h"
#endif

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

netdevIntf_t gNetIntf = {.clk_registry = {.tag = HAL_NAME_NETCARD}};

static void prvEthEventCB(uint32_t evt_id, void *ctx)
{
    netdevIntf_t *nc = (netdevIntf_t *)ctx;
    OSI_LOGI(0x10007569, "Ethernet event cb %d (e: %p, s: %p)", evt_id, nc->ether, nc->session);
    if (DRV_ETHER_EVENT_CONNECT == evt_id)
    {
        osiEvent_t tcpip_event = {0};
        tcpip_event.id = EV_TCPIP_ETHCARD_CONNECT;
        osiEventSend(netGetTaskID(), (const osiEvent_t *)&tcpip_event);
        nc->netdevIsConnected = true;
        osiRequestSysClkActive(&nc->clk_registry);
        OSI_LOGI(0x1000756a, "net card EV_TCPIP_ETHCARD_CONNECT");
    }

    if (DRV_ETHER_EVENT_DISCONNECT == evt_id)
    {
        osiEvent_t tcpip_event = {0};
        tcpip_event.id = EV_TCPIP_ETHCARD_DISCONNECT;
        osiEventSend(netGetTaskID(), (const osiEvent_t *)&tcpip_event);
        nc->netdevIsConnected = false;
        osiReleaseClk(&nc->clk_registry);
        OSI_LOGI(0x1000756b, "net card EV_TCPIP_ETHCARD_DISCONNECT");
    }
}

static void prvEthUploadDataCB(drvEthPacket_t *pkt, uint32_t size, void *ctx)
{
    netdevIntf_t *nc = (netdevIntf_t *)ctx;
    netSession_t *session = nc->session;

    if (session == NULL)
    {
        OSI_LOGE(0x1000756c, "upload data no session");
        return;
    }

    if (size > ETH_HLEN)
    {
        OSI_LOGE(0x1000756d, "prvEthUploadDataCB data size: %d", size - ETH_HLEN);
        uint8_t *ipData = pkt->data;
#ifdef CONFIG_NET_TRACE_IP_PACKET
        uint8_t *ipdata = ipData;
        uint16_t identify = (ipdata[4] << 8) + ipdata[5];
        OSI_LOGI(0x0, "prvEthUploadDataCB identify %04x", identify);
#endif
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
        OSI_LOGE(0x1000756e, "NC PsIntf get netif failed");
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
#ifdef CONFIG_NET_TRACE_IP_PACKET
            uint8_t *ipdata = rdata;
            uint16_t identify = (ipdata[4] << 8) + ipdata[5];
            OSI_LOGI(0x0, "prvNdevProcessPsData identify %04x", identify);
#endif
            OSI_LOGE(0x1000756f, "PsIntfRead %d", rsize);
            if (!drvEtherTxReqSubmit(nc->ether, tx_req, rsize))
            {
                drvEtherTxReqFree(nc->ether, tx_req);
                OSI_LOGW(0x10007570, "PS to NC error %d", rsize);
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
        OSI_LOGI(0x10007571, "written: %d will osiPanic", written);
        //osiThreadSleep(2);
        //osiPanic();
        return false;
    }
    else
    {
        if (written == size)
        {
            OSI_LOGE(0x10007572, "PsIntfWrite %d", size);
            inp_netif->u32RndisULSize += written;
        }
        else
            OSI_LOGW(0x10007573, "PsIntfWrite error %d %d", size, written);
    }
    return true;
}

static void prvProcessNdevConnect(void *par)
{
    netdevIntf_t *nc = (netdevIntf_t *)par;
    OSI_LOGI(0x10007574, "netdevConnect (ether %p, session %p)", nc->ether, nc->session);
    if (nc->ether == NULL)
        return;

    if (nc->session == NULL)
        nc->session = prvNdevSessionCreate();

    if (nc->session == NULL)
        return;

    drvEtherSetULDataCB(nc->ether, prvEthUploadDataCB, nc);
    if (!drvEtherOpen(nc->ether))
    {
        OSI_LOGI(0x10007575, "NC connect ether open fail");
        prvNdevSessionDelete(nc->session);
        nc->session = NULL;
    }
}

void netdevConnect()
{
#if IP_NAT
    if (netif_default != NULL && get_nat_enabled((netif_default->sim_cid >> 4), (netif_default->sim_cid & 0x0f)))
    {
        netdevLanConnect();
    }
    else
    {
#endif
        OSI_LOGI(0x10007576, "netdevConnect timer start.");
        netdevIntf_t *nc = &gNetIntf;
        if (nc->connect_timer != NULL)
            osiTimerDelete(nc->connect_timer);
        nc->connect_timer = osiTimerCreate(netGetTaskID(), prvProcessNdevConnect, nc);
        osiTimerStart(nc->connect_timer, 500);
#if IP_NAT
    }
#endif
}

void netdevDisconnect()
{
#if IP_NAT
    netdevIntf_t *nc = &gNetIntf;
    struct netif *inp_netif = NULL;
    if ((nc->session != NULL) && (nc->session->is_created == true))
    {
        inp_netif = nc->session->dev_netif;
    }
    if (inp_netif != NULL && get_nat_enabled((inp_netif->sim_cid >> 4), (inp_netif->sim_cid & 0x0f)))
    {
        netdevLanDisconnect();
    }
    else
    {
#endif
        netdevIntf_t *nc = &gNetIntf;
        drvEtherClose(nc->ether);
        prvNdevSessionDelete(nc->session);
        nc->session = NULL;
        if (nc->connect_timer != NULL)
        {
            osiTimerDelete(nc->connect_timer);
            nc->connect_timer = NULL;
        }
#if IP_NAT
    }
#endif
}

void netdevNetUp()
{
#if IP_NAT
    if (netif_default != NULL && get_nat_enabled((netif_default->sim_cid >> 4), (netif_default->sim_cid & 0x0f)))
    {
        uint8_t nCid = netif_default->sim_cid & 0x0f;
        uint8_t nSimId = netif_default->sim_cid >> 4;
        netdevLanNetUp(nSimId, nCid);
    }
    else
    {
#endif
        OSI_LOGI(0x10007577, "netdevNetUp");
        netdevIntf_t *nc = &gNetIntf;
        if (nc->ether == NULL)
            return;

        if (nc->session != NULL)
        {
            OSI_LOGI(0, "netdevNetUp Net session create already created");
            return;
        }

        nc->session = prvNdevSessionCreate();

        if (nc->session == NULL)
            return;

        drvEtherSetULDataCB(nc->ether, prvEthUploadDataCB, nc);
        if (!drvEtherOpen(nc->ether))
        {
            OSI_LOGI(0x10007578, "NC net up ether open fail.");
            prvNdevSessionDelete(nc->session);
            nc->session = NULL;
            return;
        }
#if IP_NAT
    }
#endif
}

static void prvNdevRestart(void *ctx)
{
    if (netif_default != NULL)
    {
        netdevNetUp();
    }
}

void netdevNetDown(uint8_t nSimID, uint8_t nCID)
{
#if IP_NAT
    if (get_nat_enabled(nSimID, nCID))
    {
        netdevLanNetDown(nSimID, nCID);
    }
    else
    {
#endif
        netdevIntf_t *nc = &gNetIntf;
        OSI_LOGI(0x10007579, "pre netdevNetDown");
        if ((nc->netdevIsConnected == true) && (nc->session != NULL) && (nc->session->is_created == true))
        {
            struct netif *inp_netif = nc->session->dev_netif;
            if ((inp_netif != NULL) && ((nSimID << 4 | nCID) == inp_netif->sim_cid))
            {
                OSI_LOGI(0x1000757a, "netdevNetDown");
                drvEtherClose(nc->ether);
                prvNdevSessionDelete(nc->session);
                nc->session = NULL;
            }
        }
#if IP_NAT
    }
#endif
    if (netif_default != NULL) //muilty cid actived; restart Netdev on other cid
    {
        osiThreadCallback(netGetTaskID(), prvNdevRestart, NULL);
    }
}

bool netdevIsConnected()
{
    return gNetIntf.netdevIsConnected;
}

void netdevExit()
{
    OSI_LOGI(0x1000757b, "NC exit");
    uint32_t critical = osiEnterCritical();
    netdevIntf_t *nc = &gNetIntf;
    drvEtherSetEventCB(nc->ether, NULL, NULL);
    nc->ether = NULL;
    nc->netdevIsConnected = false;
    osiExitCritical(critical);
}

void netdevInit(drvEther_t *ether)
{
    OSI_LOGI(0x1000757c, "netdevInit");
    uint32_t critical = osiEnterCritical();
    gNetIntf.ether = ether;
    drvEtherSetEventCB(ether, prvEthEventCB, &gNetIntf);
    osiExitCritical(critical);
}

netSession_t *prvNdevSessionCreate()
{
    static netSession_t s_net_session;
    OSI_LOGI(0x1000757d, "Net session create");
    uint32_t critical = osiEnterCritical();
    if (netif_default == NULL || netif_default->pspathIntf == NULL || netif_default->link_mode != NETIF_LINK_MODE_LWIP)
    {
        osiExitCritical(critical);
        OSI_LOGE(0x1000757e, "Net session create no netif_default %p", netif_default);
        return NULL;
    }

    netSession_t *session = &s_net_session;
    if (session->is_created == true)
    {
        osiExitCritical(critical);
        OSI_LOGI(0x1000757f, "Net session create already created");
        return session;
    }
    memset(session, 0, sizeof(netSession_t));
    session->dev_netif = netif_default;
    session->old_psintf_cb = drvPsIntfSetDataArriveCB(session->dev_netif->pspathIntf, prvPsDataToNdev);
    OSI_LOGI(0x10007580, "Net end session %x\n", session);
    session->dev_netif->link_mode = NETIF_LINK_MODE_NETCARD;
    netif_default = NULL;
#if !LWIP_SINGLE_NETIF
    if (netif_list != NULL)
    {
        struct netif *next_netif = netif_list;
        while (next_netif != NULL)
        {
            OSI_LOGI(0, "reset default_netif next_netif id %d", next_netif->num);
            if ((next_netif->name[0] != 'l') && (next_netif->name[1] != 'o'))
            {
#if IP_NAT
                if ((next_netif->link_mode == NETIF_LINK_MODE_LWIP) || (next_netif->link_mode == NETIF_LINK_MODE_NAT_LWIP_LAN))
#else
                if (next_netif->link_mode == NETIF_LINK_MODE_LWIP)
#endif
                {
                    netif_set_default(next_netif);
                    break;
                }
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
            OSI_LOGI(0, "reset default_netif next_netif id %d", next_netif->num);
            if ((next_netif->name[0] != 'l') && (next_netif->name[1] != 'o'))
            {
#if IP_NAT
                if ((next_netif->link_mode == NETIF_LINK_MODE_LWIP) || (next_netif->link_mode == NETIF_LINK_MODE_NAT_LWIP_LAN))
#else
                if (next_netif->link_mode == NETIF_LINK_MODE_LWIP)
#endif
                {
                    netif_set_default(next_netif);
                    break;
                }
            }
            next_netif = next_netif->next;
        }
    }
#endif
    session->is_created = false;
    osiExitCritical(critical);
    return true;
}
