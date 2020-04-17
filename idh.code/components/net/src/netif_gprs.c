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
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "lwip/tcpip.h"

#include "drv_ps_path.h"
#include "netif_ppp.h"
#include "at_cfw.h"

#define SIM_CID(sim, cid) ((((sim)&0xf) << 4) | ((cid)&0xf))

extern osiThread_t *netGetTaskID();
extern struct netif *getGprsNetIf(uint8_t nSim, uint8_t nCid);
extern uint32_t *getDNSServer(uint8_t nCid, uint8_t nSimID);
extern uint32_t do_send_stored_packet(uint8_t nCID, CFW_SIM_ID nSimID);
extern void dns_clean_entries(void);

#define MAX_SIM_ID 2
#define MAX_CID 7

static struct netif gprs_netif[MAX_SIM_ID][MAX_CID] = {0};
struct netif *getGprsGobleNetIf(uint8_t nSim, uint8_t nCid)
{
    sys_arch_printf("gprs_netif[%d][%d] = %p", nSim, nCid, &gprs_netif[nSim][nCid]);
    return &gprs_netif[nSim][nCid];
}
static void gprs_data_ipc_to_lwip(void *ctx)
{
    struct netif *inp_netif = (struct netif *)ctx;
    struct pbuf *p, *q;
    sys_arch_printf("gprs_data_ipc_to_lwip");
    uint8_t pData[1600];
    int len = 0;
    int readLen = 0;
    int offset = 0;
    do
    {
        sys_arch_printf("drvPsIntfRead in");
        readLen = drvPsIntfRead(inp_netif->pspathIntf, pData, 1600);
        len = readLen;
        sys_arch_printf("drvPsIntfRead out %d", len);
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
                inp_netif->input(p, inp_netif);
                inp_netif->u32LwipDLSize += readLen;
            }
        }
    } while (readLen > 0);
}

void lwip_pspathDataInput(void *ctx, drvPsIntf_t *p)
{
    sys_arch_printf("lwip_pspathDataInput osiThreadCallback in ");
    osiThreadCallback(netGetTaskID(), gprs_data_ipc_to_lwip, (void *)ctx);
    sys_arch_printf("lwip_pspathDataInput osiThreadCallback out");
}

static err_t data_output(struct netif *netif, struct pbuf *p,
                         ip_addr_t *ipaddr)
{
    uint16_t offset = 0;
    struct pbuf *q = NULL;

#if 1
    sys_arch_printf("data_output ---------tot_len=%d, flags=0x%x---------", p->tot_len, p->flags);

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

    sys_arch_printf("drvPsIntfWrite--in---");

    extern bool ATGprsGetDPSDFlag(CFW_SIM_ID nSim);
#define GET_SIM(sim_cid) (((sim_cid) >> 4) & 0xf)

    if (!ATGprsGetDPSDFlag(GET_SIM(netif->sim_cid)))
        drvPsIntfWrite((drvPsIntf_t *)netif->pspathIntf, pData, p->tot_len);

    sys_arch_printf("drvPsIntfWrite--out---");
    netif->u32LwipULSize += p->tot_len;
    free(pData);
#endif

#if 0
    CFW_GPRS_DATA *pGprsData = NULL;

    sys_arch_printf("data_output ---------tot_len=%d, flags=0x%x---------", p->tot_len, p->flags);

    pGprsData = malloc(sizeof(CFW_GPRS_DATA) + p->tot_len);
    if (pGprsData == NULL)
    {
        return !ERR_OK;
    }

    pGprsData->nDataLength = p->tot_len;

    for (q = p; q != NULL; q = q->next)
    {
        memcpy(&pGprsData->pData[offset], q->payload, q->len);
        offset += q->len;
    }

    sys_arch_dump(pGprsData->pData, pGprsData->nDataLength);

    CFW_GprsSendData(netif->sim_cid & 0x0f, pGprsData, 0, (netif->sim_cid & 0xf0) >> 4);

    free(pGprsData);
#endif

    sys_arch_printf("data_output--return in netif_gprs---");

    return ERR_OK;
}

static err_t netif_gprs_init(struct netif *netif)
{
    /* initialize the snmp variables and counters inside the struct netif
   * ifSpeed: no assumption can be made!
   */
    netif->name[0] = 'G';
    netif->name[1] = 'P';

#if LWIP_IPV4
    netif->output = (netif_output_fn)data_output;
#endif
#if LWIP_IPV6
    netif->output_ip6 = (netif_output_ip6_fn)data_output;
#endif
    netif->mtu = GPRS_MTU;
    if (NULL == netif_default)
    {
        int nCid = netif->sim_cid & 0x0f;
        int nSimId = (netif->sim_cid & 0xf0) >> 4;
        netif_set_default(netif);
        CFW_GPRS_PDPCONT_INFO_V2 pdp_context;
        ip_addr_t ip4_dns1 = {0};
        ip_addr_t ip4_dns2 = {0};
#if LWIP_IPV6
        ip_addr_t ip6_dns1 = {0};
        ip_addr_t ip6_dns2 = {0};
#endif
        CFW_GprsGetPdpCxtV2(nCid, &pdp_context, nSimId);

        if ((pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IP) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IPV4V6) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_X25) ||
            (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_PPP)) //add evade for nPdpType == 0, need delete later
        {
            if (pdp_context.nPdpDnsSize != 0)
            {
                IP_ADDR4(&ip4_dns1, pdp_context.pPdpDns[0], pdp_context.pPdpDns[1], pdp_context.pPdpDns[2], pdp_context.pPdpDns[3]);
                IP_ADDR4(&ip4_dns2, pdp_context.pPdpDns[21], pdp_context.pPdpDns[22], pdp_context.pPdpDns[23], pdp_context.pPdpDns[24]);
                OSI_LOGI(0, "DNS size:%d  DNS1:%d.%d.%d.%d", pdp_context.nPdpDnsSize,
                         pdp_context.pPdpDns[0], pdp_context.pPdpDns[1], pdp_context.pPdpDns[2], pdp_context.pPdpDns[3]);
                OSI_LOGI(0, "DNS2:%d.%d.%d.%d",
                         pdp_context.pPdpDns[21], pdp_context.pPdpDns[22], pdp_context.pPdpDns[23], pdp_context.pPdpDns[24]);
            }
        }
        dns_setserver(0, &ip4_dns1);
        dns_setserver(1, &ip4_dns2);
#if LWIP_IPV6
        if (((pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IPV6) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IPV4V6)))
        {
            if (pdp_context.nPdpDnsSize != 0)
            {
                uint32_t addr0 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpDns[4], pdp_context.pPdpDns[5], pdp_context.pPdpDns[6], pdp_context.pPdpDns[7]));
                uint32_t addr1 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpDns[8], pdp_context.pPdpDns[9], pdp_context.pPdpDns[10], pdp_context.pPdpDns[11]));
                uint32_t addr2 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpDns[12], pdp_context.pPdpDns[13], pdp_context.pPdpDns[14], pdp_context.pPdpDns[15]));
                uint32_t addr3 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpDns[16], pdp_context.pPdpDns[17], pdp_context.pPdpDns[18], pdp_context.pPdpDns[19]));
                uint32_t addr4 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpDns[25], pdp_context.pPdpDns[26], pdp_context.pPdpDns[27], pdp_context.pPdpDns[28]));
                uint32_t addr5 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpDns[29], pdp_context.pPdpDns[30], pdp_context.pPdpDns[31], pdp_context.pPdpDns[32]));
                uint32_t addr6 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpDns[33], pdp_context.pPdpDns[34], pdp_context.pPdpDns[35], pdp_context.pPdpDns[36]));
                uint32_t addr7 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpDns[37], pdp_context.pPdpDns[38], pdp_context.pPdpDns[39], pdp_context.pPdpDns[40]));
                OSI_LOGI(0, "DNS size:%d  ipv6 DNS 12-15:%d.%d.%d.%d", pdp_context.nPdpDnsSize,
                         pdp_context.pPdpDns[12], pdp_context.pPdpDns[13], pdp_context.pPdpDns[14], pdp_context.pPdpDns[15]);
                OSI_LOGI(0, " ipv6 DNS 33-36:%d.%d.%d.%d",
                         pdp_context.pPdpDns[33], pdp_context.pPdpDns[34], pdp_context.pPdpDns[35], pdp_context.pPdpDns[36]);
                IP_ADDR6(&ip6_dns1, addr0, addr1, addr2, addr3);
                IP_ADDR6(&ip6_dns2, addr4, addr5, addr6, addr7);
            }
        }
#endif

#if LWIP_IPV6
        if (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IPV6)
        {
            dns_setserver(0, &ip6_dns1);
            dns_setserver(1, &ip6_dns2);
        }
        if (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IPV4V6)
        {
            if (IP_IS_V6(&ip6_dns1))
                dns_setserver(1, &ip6_dns1);
        }
#endif
    }

    return ERR_OK;
}

extern uint8_t CFW_GprsGetPdpIpv6Addr(uint8_t nCid, uint8_t *nLength, uint8_t *ipv6Addr, CFW_SIM_ID nSimID);
extern uint8_t CFW_GprsGetPdpIpv4Addr(uint8_t nCid, uint8_t *nLength, uint8_t *ipv4Addr, CFW_SIM_ID nSimID);

void TCPIP_netif_create(uint8_t nCid, uint8_t nSimId)
{
    struct netif *netif = NULL;
    uint8_t T_cid = nSimId << 4 | nCid;
    netif = getGprsNetIf(nSimId, nCid);
    if (netif != NULL)
    {
        sys_arch_printf("TCPIP_netif_create netif already created cid %d simid %d\n", nCid, nSimId);
        return;
    }

    ip4_addr_t ip4 = {0};
#if LWIP_IPV6
    ip6_addr_t ip6 = {0};
#endif
    //uint8_t lenth;

    sys_arch_printf("TCPIP_netif_create cid %d simid %d\n", nCid, nSimId);

    AT_Gprs_CidInfo *pCidInfo = &gAtCfwCtx.sim[nSimId].cid_info[nCid];

    if (pCidInfo->uCid != nCid)
    {
        pCidInfo->uCid = nCid;
    }

    CFW_GPRS_PDPCONT_INFO_V2 pdp_context;
    CFW_GprsGetPdpCxtV2(nCid, &pdp_context, nSimId);
    sys_arch_printf("TCPIP_netif_create pdp_context.PdnType %d\n", pdp_context.PdnType);
    if ((pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IP) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IPV4V6) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_X25) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_PPP)) //add evade for nPdpType == 0, need delete later
    {
        //uint8_t ipv4[4] = {0};
        //CFW_GprsGetPdpIpv4Addr(nCid, &lenth, ipv4, nSimId);
        //IP4_ADDR(&ip4, ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
        IP4_ADDR(&ip4, pdp_context.pPdpAddr[0], pdp_context.pPdpAddr[1], pdp_context.pPdpAddr[2], pdp_context.pPdpAddr[3]);
        sys_arch_printf("TCPIP_netif_create IP4: %s", ip4addr_ntoa(&ip4));
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
    netif->link_mode = NETIF_LINK_MODE_LWIP;
    netif->pdnType = pdp_context.PdnType;
    netif->is_used = 1;

    drvPsIntf_t *dataPsPath = drvPsIntfOpen(nSimId, nCid, lwip_pspathDataInput, (void *)netif);
    if (dataPsPath != NULL)
    {
        netif->pspathIntf = dataPsPath;
        /**if dataPsPath is NULL, it has been write in netif->pspathIntf. 
		   netif->pspathIntf need not update any more.**/
    }
#if LWIP_TCPIP_CORE_LOCKING
    LOCK_TCPIP_CORE();
#endif
    netif_add(netif, &ip4, NULL, NULL, NULL, netif_gprs_init, tcpip_input);
#if LWIP_IPV6
    if (((pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IPV6) || (pdp_context.PdnType == CFW_GPRS_PDP_TYPE_IPV4V6)))
    {
        //uint8_t pdpAddr[22];
        //CFW_GprsGetPdpIpv6Addr(nCid, &lenth, pdpAddr, nSimId);
        //uint32_t addr0 = PP_HTONL(LWIP_MAKEU32(pdpAddr[0], pdpAddr[1], pdpAddr[2], pdpAddr[3]));
        //uint32_t addr1 = PP_HTONL(LWIP_MAKEU32(pdpAddr[4], pdpAddr[5], pdpAddr[6], pdpAddr[7]));
        //uint32_t addr2 = PP_HTONL(LWIP_MAKEU32(pdpAddr[8], pdpAddr[9], pdpAddr[10], pdpAddr[11]));
        //uint32_t addr3 = PP_HTONL(LWIP_MAKEU32(pdpAddr[12], pdpAddr[13], pdpAddr[14], pdpAddr[15]));

        uint32_t addr0 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpAddr[4], pdp_context.pPdpAddr[5], pdp_context.pPdpAddr[6], pdp_context.pPdpAddr[7]));
        uint32_t addr1 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpAddr[8], pdp_context.pPdpAddr[9], pdp_context.pPdpAddr[10], pdp_context.pPdpAddr[11]));
        uint32_t addr2 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpAddr[12], pdp_context.pPdpAddr[13], pdp_context.pPdpAddr[14], pdp_context.pPdpAddr[15]));
        uint32_t addr3 = PP_HTONL(LWIP_MAKEU32(pdp_context.pPdpAddr[16], pdp_context.pPdpAddr[17], pdp_context.pPdpAddr[18], pdp_context.pPdpAddr[19]));
        IP6_ADDR(&ip6, addr0, addr1, addr2, addr3);
        netif_ip6_addr_set(netif, 0, &ip6);
        netif_ip6_addr_set_state(netif, 0, IP6_ADDR_VALID);
        sys_arch_printf("TCPIP_netif_create IP6: %s", ip6addr_ntoa(&ip6));
    }
#endif
    netif_set_up(netif);
    netif_set_link_up(netif);
#if LWIP_TCPIP_CORE_LOCKING
    UNLOCK_TCPIP_CORE();
#endif
    sys_arch_printf("TCPIP_netif_create, netif->num: 0x%x\n", netif->num);
}

void TCPIP_netif_destory(uint8_t nCid, uint8_t nSimId)
{
    struct netif *netif = NULL;

    netif = getGprsNetIf(nSimId, nCid);
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
        sys_arch_printf("TCPIP_netif_destory netif");
        tcp_debug_print_pcbs();
#if LWIP_TCPIP_CORE_LOCKING
        UNLOCK_TCPIP_CORE();
#endif
    }
    else
    {
        sys_arch_printf("Error ########### CFW_GPRS_DEACTIVED can't find netif for CID=0x%x\n", SIM_CID(nSimId, nCid));
    }
}

void BAL_ApsTaskTcpipProc(osiEvent_t *pEvent)
{
    sys_arch_printf("Enter BAL_ApsTaskTcpipProc");
    sys_arch_printf("pEvent->id:%ld", pEvent->id);
    sys_arch_printf("pEvent->param1:%lx", pEvent->param1);
    sys_arch_printf("pEvent->param2:%lx", pEvent->param2);
    sys_arch_printf("pEvent->param3:%lx", pEvent->param3);

    CFW_EVENT *cfw_event = (CFW_EVENT *)pEvent;
    if (pEvent->id == EV_CFW_GPRS_DATA_IND)
    {
#if 0
        struct pbuf *p, *q;
        uint16_t len;
        uint16_t offset;
        CFW_GPRS_DATA *Msg;
        uint8_t nCid, nSimId, T_cid, nDlc;

        struct netif *inp_netif;
        Msg = (CFW_GPRS_DATA *)pEvent->param2;
        nCid = pEvent->param1;
        nSimId = LOUINT8(pEvent->param3);
        nDlc = HIUINT16(pEvent->param3);
        T_cid = nSimId << 4 | nCid;
        UNUSED_P(T_cid);
        UNUSED_P(nDlc);
        inp_netif = getGprsNetIf(nSimId, nCid);
        len = Msg->nDataLength;
        sys_arch_dump(Msg->pData, len);
        if (inp_netif != NULL)
        {
            p = (struct pbuf *)pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
            offset = 0;
            if (p != NULL)
            {
                if (len > p->len)
                {
                    for (q = p; len > q->len; q = q->next)
                    {
                        memcpy(q->payload, &(Msg->pData[offset]), q->len);
                        len -= q->len;
                        offset += q->len;
                    }
                    if (len != 0)
                    {
                        memcpy(q->payload, &(Msg->pData[offset]), len);
                    }
                }
                else
                {
                    memcpy(p->payload, &(Msg->pData[offset]), len);
                }

                inp_netif->input(p, inp_netif);
            }
        }
        free(Msg);
#endif
    }
    else if (pEvent->id == EV_CFW_GPRS_CTRL_RELEASE_IND)
    {
        sys_arch_printf("EV_CFW_GPRS_CTRL_RELEASE_IND");
        //do_send_stored_packet(pEvent->param1, pEvent->param2);  howie porting
    }
    else if (pEvent->id == EV_CFW_GPRS_ACT_RSP)
    {
#if 0
        sys_arch_printf("EV_CFW_GPRS_ACT_RSP \n");
        struct netif *netif;
        uint8_t nCid, nSimId, T_cid, channelId;
        nCid = pEvent->param1;
        nSimId = LOUINT8(pEvent->param3);
        T_cid = nSimId << 4 | nCid;
        channelId = HIUINT16(pEvent->param3);
        UNUSED_P(channelId);
        if (HIUINT8(pEvent->param3) == CFW_GPRS_ACTIVED)
        {
            uint8_t addr[THE_PDP_ADDR_MAX_LEN];
            uint32_t Ip_Addr;
            ip4_addr_t ip = {0};
            uint8_t lenth;
            UNUSED_P(lenth);
            CFW_GprsGetPdpAddr(nCid, &lenth, addr, nSimId);
            Ip_Addr = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];

            sys_arch_printf("APS : active success cid = 0x%x\n", T_cid);
            sys_arch_printf("IP: %d.%d.%d.%d",
                            (uint16_t)(ntohl(Ip_Addr) >> 24) & 0xff,
                            (uint16_t)(ntohl(Ip_Addr) >> 16) & 0xff,
                            (uint16_t)(ntohl(Ip_Addr) >> 8) & 0xff,
                            (uint16_t)ntohl(Ip_Addr) & 0xff);

            netif = malloc(sizeof(struct netif));
            memset(netif, 0, sizeof(struct netif));
            netif->sim_cid = T_cid;
            ip.addr = Ip_Addr;
            drvPsIntf_t *dataPsPath = drvPsIntfOpen(nSimId, nCid, lwip_pspathDataInput, (void *)netif);
            netif->pspathIntf = dataPsPath;
            netif_add(netif, &ip, NULL, NULL, NULL, netif_gprs_init, tcpip_input);
            netif_set_up(netif);
            netif_set_link_up(netif);
            sys_arch_printf("APS, netif->num: 0x%x\n", netif->num);
        }
        else if (HIUINT8(pEvent->param3) == CFW_GPRS_DEACTIVED)
        {
            netif = getGprsNetIf(nSimId, nCid);
            if (netif != NULL)
            {
                netif_set_link_down(netif);
                netif_remove(netif);
                free(netif);
                tcp_debug_print_pcbs();
            }
            else
            {
                sys_arch_printf("Error ########### CFW_GPRS_DEACTIVED can't find netif for CID=0x%x\n", T_cid);
            }
        }
#endif
    }
    else if (pEvent->id == EV_CFW_GPRS_ATT_RSP)
    {
        sys_arch_printf("EV_CFW_GPRS_ATT_RSP \n");
        if (cfw_event->nType == CFW_GPRS_DEACTIVED)
        {
            tcp_debug_print_pcbs();
        }
    }
    else if (pEvent->id == EV_CFW_GPRS_CXT_DEACTIVE_IND)
    {
        struct netif *netif;
        uint8_t nCid, nSimId;
        nCid = pEvent->param1;
        nSimId = cfw_event->nFlag;
        sys_arch_printf("EV_CFW_GPRS_CXT_DEACTIVE_IND cid = 0x%x ,cause: 0x%x\n",
                        SIM_CID(nSimId, nCid), (unsigned int)pEvent->param2);
        netif = getGprsNetIf(nSimId, nCid);
        if (netif != NULL)
        {
            drvPsIntf_t *dataPsPath = netif->pspathIntf;
            drvPsIntfClose(dataPsPath);
            netif_set_link_down(netif);
            netif_remove(netif);
            free(netif);
            tcp_debug_print_pcbs();
        }
        else
        {
            sys_arch_printf("EV_CFW_GPRS_CXT_DEACTIVE_IND can't find netif for CID=0x%x\n", SIM_CID(nSimId, nCid));
        }
    }
}

void wifi_dns_setserver(u8_t numdns, const ip_addr_t *dnsserver) {}
