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

#include "cfw.h"
#include "osi_api.h"
#include "osi_log.h"
#include "sockets.h"
#include "lwip/prot/ip.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "lwip/tcpip.h"
#include "lwip/ip4_nat.h"

#include "drv_ps_path.h"
#include "netif_ppp.h"
#include "netutils.h"
#include "at_cfw.h"
#if IP_NAT
#define SIM_CID(sim, cid) ((((sim)&0xf) << 4) | ((cid)&0xf))

extern osiThread_t *netGetTaskID();
extern uint32_t *getDNSServer(uint8_t nCid, uint8_t nSimID);
extern uint32_t do_send_stored_packet(uint8_t nCID, CFW_SIM_ID nSimID);
extern void dns_clean_entries(void);

#define MAX_SIM_ID 2
#define MAX_CID 7
static struct netif gprs_netif[MAX_SIM_ID][MAX_CID] = {0};
static void gprs_data_ipc_to_lwip_nat_wan(void *ctx)
{
    struct netif *inp_netif = (struct netif *)ctx;
    if (inp_netif == NULL)
        return;
    struct pbuf *p, *q;
    OSI_LOGI(0x10007538, "gprs_data_ipc_to_lwip");
    uint8_t *pData = malloc(1600);
    if (pData == NULL)
        return;
    int len = 0;
    int readLen = 0;
    int offset = 0;
    do
    {
        OSI_LOGI(0x10007539, "drvPsIntfRead in");
        readLen = drvPsIntfRead(inp_netif->pspathIntf, pData, 1600);
#ifdef CONFIG_NET_TRACE_IP_PACKET
        uint8_t *ipdata = pData;
        uint16_t identify = (ipdata[4] << 8) + ipdata[5];
        OSI_LOGI(0x0, "Wan DL read from IPC thread identify %04x", identify);
#endif
        len = readLen;
        OSI_LOGI(0x1000753a, "drvPsIntfRead out %d", len);
        if (len > 0)
        {
            sys_arch_dump(pData, len);
        }
        if (inp_netif != NULL && len > 0)
        {
            p = (struct pbuf *)pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
            offset = 0;
            if (p != NULL)
            {
                if (len > p->len)
                {
                    for (q = p; len > q->len; q = q->next)
                    {
                        memcpy(q->payload, &(pData[offset]), q->len);
                        len -= q->len;
                        offset += q->len;
                    }
                    if (len != 0)
                    {
                        memcpy(q->payload, &(pData[offset]), len);
                    }
                }
                else
                {
                    memcpy(p->payload, &(pData[offset]), len);
                }
#if LWIP_IPV6
                if (IP_HDR_GET_VERSION(p->payload) == 6)
                { //find lan netif with same SimCid and same IPV6 addr to send
                    struct netif *netif;
                    u8_t sim_cid = inp_netif->sim_cid;
                    u8_t taken = 0;
                    NETIF_FOREACH(netif)
                    {
                        if (sim_cid == netif->sim_cid && (NETIF_LINK_MODE_NAT_LWIP_LAN == netif->link_mode || NETIF_LINK_MODE_NAT_PPP_LAN == netif->link_mode || NETIF_LINK_MODE_NAT_NETDEV_LAN == netif->link_mode))
                        {
                            struct ip6_hdr *ip6hdr = p->payload;
                            ip6_addr_t current_iphdr_dest;
                            ip6_addr_t *current_netif_addr;
                            ip6_addr_copy_from_packed(current_iphdr_dest, ip6hdr->dest);
                            current_netif_addr = (ip6_addr_t *)netif_ip6_addr(netif, 0);
                            if (current_netif_addr->addr[2] == current_iphdr_dest.addr[2] && current_netif_addr->addr[3] == current_iphdr_dest.addr[3])
                            {
                                OSI_LOGI(0x0, "gprs_data_ipc_to_lwip_nat_wan IPV6 to Lan netif");
                                netif->input(p, netif);
                                taken = 1;
                                break;
                            }
                        }
                    }
                    if (taken == 0)
                    {
                        NETIF_FOREACH(netif)
                        {
                            if (sim_cid == netif->sim_cid && (NETIF_LINK_MODE_NAT_LWIP_LAN == netif->link_mode || NETIF_LINK_MODE_NAT_PPP_LAN == netif->link_mode || NETIF_LINK_MODE_NAT_NETDEV_LAN == netif->link_mode))
                            {
                                OSI_LOGI(0x0, "gprs_data_ipc_to_lwip_nat_wan brodcast IPV6 to Lan netif");
                                pbuf_ref(p);
                                netif->input(p, netif);
                            }
                        }
                        inp_netif->input(p, inp_netif);
                    }
                }
                else
#endif
                {
                    u8_t taken = 0;
#if LWIP_TCPIP_CORE_LOCKING
                    LOCK_TCPIP_CORE();
#endif
                    taken = ip4_nat_input(p);
#if LWIP_TCPIP_CORE_LOCKING
                    UNLOCK_TCPIP_CORE();
#endif
                    if (taken == 0)
                        inp_netif->input(p, inp_netif);
                }
                inp_netif->u32LwipDLSize += readLen;
            }
        }
    } while (readLen > 0);
    free(pData);
}

void lwip_nat_wan_pspathDataInput(void *ctx, drvPsIntf_t *p)
{
    OSI_LOGI(0x0, "lwip_nat_wan_pspathDataInput osiThreadCallback in ");
    osiThreadCallback(netGetTaskID(), gprs_data_ipc_to_lwip_nat_wan, (void *)ctx);
    OSI_LOGI(0x0, "lwip_nat_wan_pspathDataInput osiThreadCallback out");
}

static err_t nat_wan_data_output(struct netif *netif, struct pbuf *p,
                                 ip_addr_t *ipaddr)
{
    uint16_t offset = 0;
    struct pbuf *q = NULL;

#if 1
    OSI_LOGI(0x1000753d, "data_output ---------tot_len=%d, flags=0x%x---------", p->tot_len, p->flags);

    uint8_t *pData = malloc(p->tot_len);
    if (pData == NULL)
    {
        return !ERR_OK;
    }

    for (q = p; q != NULL; q = q->next)
    {
        memcpy(&pData[offset], q->payload, q->len);
        offset += q->len;
    }

    sys_arch_dump(pData, p->tot_len);

    OSI_LOGI(0x1000753e, "drvPsIntfWrite--in---");
    drvPsIntfWrite((drvPsIntf_t *)netif->pspathIntf, pData, p->tot_len);
    OSI_LOGI(0x1000753f, "drvPsIntfWrite--out---");
    netif->u32LwipULSize += p->tot_len;
    free(pData);
#endif

    OSI_LOGI(0x10007540, "data_output--return in netif_gprs---");

    return ERR_OK;
}

static err_t netif_gprs_nat_wan_init(struct netif *netif)
{
    /* initialize the snmp variables and counters inside the struct netif
   * ifSpeed: no assumption can be made!
   */
    netif->name[0] = 'G';
    netif->name[1] = 'P';

#if LWIP_IPV4
    netif->output = (netif_output_fn)nat_wan_data_output;
#endif
#if LWIP_IPV6
    netif->output_ip6 = (netif_output_ip6_fn)nat_wan_data_output;
#endif
    netif->mtu = GPRS_MTU;

    return ERR_OK;
}

extern uint8_t CFW_GprsGetPdpIpv6Addr(uint8_t nCid, uint8_t *nLength, uint8_t *ipv6Addr, CFW_SIM_ID nSimID);
extern uint8_t CFW_GprsGetPdpIpv4Addr(uint8_t nCid, uint8_t *nLength, uint8_t *ipv4Addr, CFW_SIM_ID nSimID);

struct netif *TCPIP_nat_wan_netif_create(uint8_t nCid, uint8_t nSimId)
{
    struct netif *netif = NULL;
    uint8_t T_cid = nSimId << 4 | nCid;
    netif = getGprsNetIf(nSimId, nCid);
    if (netif != NULL)
    {
        OSI_LOGI(0x0, "TCPIP_nat_wan_netif_create netif already created cid %d simid %d\n", nCid, nSimId);
        return netif;
    }

    ip4_addr_t ip4 = {0};
    //uint8_t lenth;

    OSI_LOGI(0x0, "TCPIP_nat_wan_netif_create cid %d simid %d\n", nCid, nSimId);

    AT_Gprs_CidInfo *pCidInfo = &gAtCfwCtx.sim[nSimId].cid_info[nCid];

    if (pCidInfo->uCid != nCid)
    {
        pCidInfo->uCid = nCid;
    }

    CFW_GPRS_PDPCONT_INFO_V2 pdp_context;
    CFW_GprsGetPdpCxtV2(nCid, &pdp_context, nSimId);
    OSI_LOGI(0x0, "TCPIP_nat_wan_netif_create pdp_context.PdnType %d\n", pdp_context.PdnType);
    if ((pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IP) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IPV4V6) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_X25) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_PPP)) //add evade for nPdpType == 0, need delete later
    {
        //uint8_t ipv4[4] = {0};
        //CFW_GprsGetPdpIpv4Addr(nCid, &lenth, ipv4, nSimId);
        //IP4_ADDR(&ip4, ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
        IP4_ADDR(&ip4, pdp_context.pPdpAddr[0], pdp_context.pPdpAddr[1], pdp_context.pPdpAddr[2], pdp_context.pPdpAddr[3]);
        OSI_LOGXI(OSI_LOGPAR_S, 0x0, "TCPIP_nat_wan_netif_create IP4: %s", ip4addr_ntoa(&ip4));
    }
    if (nSimId < MAX_SIM_ID && nCid < MAX_CID)
    {
        netif = &gprs_netif[nSimId][nCid];
    }
    else
    {
        netif = malloc(sizeof(struct netif));
    }
    memset(netif, 0, sizeof(struct netif));
    netif->sim_cid = T_cid;
    netif->is_ppp_mode = 0;
    netif->link_mode = NETIF_LINK_MODE_NAT_WAN;
    netif->pdnType = pdp_context.PdnType;
    netif->is_used = 1;

    drvPsIntf_t *dataPsPath = drvPsIntfOpen(nSimId, nCid, lwip_nat_wan_pspathDataInput, (void *)netif);
    if (dataPsPath != NULL)
    {
        netif->pspathIntf = dataPsPath;
        /**if dataPsPath is NULL, it has been write in netif->pspathIntf. 
		   netif->pspathIntf need not update any more.**/
    }
#if LWIP_TCPIP_CORE_LOCKING
    LOCK_TCPIP_CORE();
#endif
    netif_add(netif, &ip4, NULL, NULL, NULL, netif_gprs_nat_wan_init, tcpip_input);
#if LWIP_TCPIP_CORE_LOCKING
    UNLOCK_TCPIP_CORE();
#endif
    OSI_LOGI(0x0, "TCPIP_nat_wan_netif_create, netif->num: 0x%x\n", netif->num);
    return netif;
}

void TCPIP_nat_wan_netif_destory(uint8_t nCid, uint8_t nSimId)
{
    struct netif *netif = NULL;

    netif = getGprsWanNetIf(nSimId, nCid);
    if (netif != NULL)
    {
#if LWIP_TCPIP_CORE_LOCKING
        LOCK_TCPIP_CORE();
#endif
        drvPsIntf_t *dataPsPath = netif->pspathIntf;
        drvPsIntfClose(dataPsPath);
        netif_set_link_down(netif);
        netif_remove(netif);
        dns_clean_entries();
        if (nSimId < MAX_SIM_ID && nCid < MAX_CID)
        {
            netif->is_used = 0;
        }
        else
        {
            free(netif);
        }
        OSI_LOGI(0x0, "TCPIP_nat_wan_netif_destory netif");
        tcp_debug_print_pcbs();
#if LWIP_TCPIP_CORE_LOCKING
        UNLOCK_TCPIP_CORE();
#endif
    }
    else
    {
        OSI_LOGI(0x1000754c, "Error ########### CFW_GPRS_DEACTIVED can't find netif for CID=0x%x\n", SIM_CID(nSimId, nCid));
    }
}

u8_t ip4_wan_forward(struct pbuf *p, struct netif *inp)
{
    u8_t taken = 0;
    if (IP_HDR_GET_VERSION(p->payload) == 4)
    { //find lan netif with same CID IPV4 addr to send
        struct netif *netif;
        u8_t sim_cid = inp->sim_cid;
        NETIF_FOREACH(netif)
        {
            if (sim_cid == netif->sim_cid && (NETIF_LINK_MODE_NAT_LWIP_LAN == netif->link_mode || NETIF_LINK_MODE_NAT_PPP_LAN == netif->link_mode || NETIF_LINK_MODE_NAT_NETDEV_LAN == netif->link_mode))
            {
                struct ip_hdr *iphdr = p->payload;
                ip4_addr_t current_iphdr_dest;
                ip4_addr_t *current_netif_addr;
                ip4_addr_copy(current_iphdr_dest, iphdr->dest);
                current_netif_addr = (ip4_addr_t *)netif_ip4_addr(netif);
                if (current_iphdr_dest.addr == current_netif_addr->addr)
                {
                    OSI_LOGI(0x0, "ip4_wan_forward IPV4 to Lan netif");
                    pbuf_ref(p);
                    netif->input(p, netif);
                    taken = 1;
                    break;
                }
            }
        }
    }
    return taken;
}
#endif
