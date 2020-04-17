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

// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG
#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('P', 'P', 'P', 'S')

#include "osi_log.h"
#include "osi_api.h"

#include "ppp_interface.h"
#include "lwipopts.h"
#ifdef CONFIG_NET_LWIP_PPP_ON
#include "sockets.h"
#include "netif/ppp/pppos.h"
#include "drv_ps_path.h"
#include "netif.h"
#include "lwip/prot/ip.h"

#include "at_cfw.h"
#include "netmain.h"

#define DUMP_MAX_SIZE (256)
#define DUMP_SIZE(size) ((size) > DUMP_MAX_SIZE ? DUMP_MAX_SIZE : (size))
extern bool AT_GetOperatorDefaultApn(uint8_t pOperatorId[6], const char **pOperatorDefaltApn);
extern void lwip_pspathDataInput(void *ctx, drvPsIntf_t *p);
extern struct netif *getGprsNetIf(uint8_t nSim, uint8_t cid);
extern err_t ppp_netif_output_ip4(struct netif *netif, struct pbuf *pb, const ip4_addr_t *ipaddr);
#if LWIP_IPV6
err_t ppp_netif_output_ip6(struct netif *netif, struct pbuf *pb, const ip6_addr_t *ipaddr);
#endif
static void _psIntfRead(void *ctx);

struct pppSession
{
    ppp_pcb *pcb;
    netif_output_fn old_output;
    drvPsIntfDataArriveCB_t old_psintf_cb;
    pppOutputCallback_t output_cb;
    pppEndCallback_t end_cb;
    pppFlowControlCallback_t flowctrl_cb;
    void *end_cb_ctx;
    void *output_cb_ctx;
    void *flowctrl_cb_ctx;
    struct netif *nif;
    u8_t cid;
    u8_t sim;
    osiNotify_t *dl_read_notify;
    osiThread_t *dl_thread;
    int is_deleted;
    int cgact_activated;
};

void ppp_ip_output(struct pbuf *p, pppSession_t *ctx)
{
    //OSI_LOGI(0x1000562a, "PPP ul ip output");

    struct netif *nif = ctx->nif;
    if (p != NULL && nif != NULL)
    {
#if LWIP_IPV6
        if (IP_HDR_GET_VERSION(p->payload) == 6)
            nif->output_ip6(nif, p, NULL);
        else
#endif
            nif->output(nif, p, NULL);
    }
    if (p)
        pbuf_free(p);
}

static void _psIntfRead(void *ctx)
{
    struct netif *nif = (struct netif *)ctx;
    if (nif == NULL)
    {
        OSI_LOGE(0x1000562b, "PPP PsIntf get netif failed");
        osiPanic();
    }

    if (nif->state == NULL)
        return;

    ppp_pcb *pcb = (ppp_pcb *)nif->state;
    if (pcb->ppp_session == NULL)
        return;

    pppSession_t *pppSession = (pppSession_t *)pcb->ppp_session;
    if (pppSession->flowctrl_cb != NULL)
    {
        int flowcntrol = pppSession->flowctrl_cb(pppSession->flowctrl_cb_ctx);
        if (flowcntrol > 1)
        {
            OSI_LOGI(0, "PPP dl read later by hard flow control");
            osiNotifyTrigger(pppSession->dl_read_notify);
            return;
        }
    }
    // Grab a buffer, and will be reused for all DL buffers
    // It is required that PBUF_POOL_BUFSIZE can hold the maximum DL buffer
    struct pbuf *p = pbuf_alloc(PBUF_RAW, 0, PBUF_POOL);
    if (p == NULL)
    {
        OSI_LOGE(0x1000562f, "PPP dl alloc pbuf failed, packet dropped");
        return;
    }

    for (;;)
    {
        int rsize = drvPsIntfRead(nif->pspathIntf, p->payload, PBUF_POOL_BUFSIZE);
        if (rsize <= 0)
            break;

        p->tot_len = p->len = rsize;

        nif->u32PPPDLSize += rsize;

#ifdef CONFIG_NET_TRACE_IP_PACKET
        uint8_t *ipdata = p->payload;
        uint16_t identify = (ipdata[4] << 8) + ipdata[5];
        OSI_LOGI(0x10005661, "PPP DL read from IPC thread identify %04x", identify);
#endif

#if LWIP_IPV6
        if (IP_HDR_GET_VERSION(p->payload) == 6)
            ppp_netif_output_ip6(nif, p, NULL);
        else
#endif
            ppp_netif_output_ip4(nif, p, NULL);

        if (pppSession->flowctrl_cb != NULL)
        {
            int flowcntrol = pppSession->flowctrl_cb(pppSession->flowctrl_cb_ctx);
            if (flowcntrol > 0)
            {
                OSI_LOGI(0x1000562c, "PPP dl read later by flow control");
                osiNotifyTrigger(pppSession->dl_read_notify);
                break;
            }
        }
    }

    pbuf_free(p);
}

static err_t _psIntfWrite(struct netif *netif, struct pbuf *p,
                          const ip4_addr_t *ipaddr)
{
    for (struct pbuf *q = p; q != NULL; q = q->next)
    {
        if (q->len == 0)
            continue;

        // When uplink buffer unavailable, it will be dropped inside
        int wsize = 0x00;

        extern bool ATGprsGetDPSDFlag(CFW_SIM_ID nSim);
#define GET_SIM(sim_cid) (((sim_cid) >> 4) & 0xf)

        if (!ATGprsGetDPSDFlag(GET_SIM(netif->sim_cid)))
            wsize = drvPsIntfWrite(netif->pspathIntf, q->payload, q->len);
        else
            wsize = q->len;

#ifdef CONFIG_NET_TRACE_IP_PACKET
        uint8_t *ipdata = q->payload;
        uint16_t identify = (ipdata[4] << 8) + ipdata[5];
        OSI_LOGI(0x10005662, "PPP UL write to IPC thread identify %04x", identify);
#endif

        if (wsize != q->len)
            OSI_LOGE(0x10005630, "PPP ul drop, size/%d written/%d", q->len, wsize);
        else
            netif->u32PPPULSize += wsize;
    }
    return ERR_OK;
}

static void _psIntfCB(void *ctx, drvPsIntf_t *p)
{
    struct netif *nif;
    ppp_pcb *pcb;
    pppSession_t *pppSession;
    if (ctx == NULL)
        return;

    nif = (struct netif *)ctx;
    if (nif->state == NULL)
        return;

    pcb = (ppp_pcb *)nif->state;
    if (pcb->ppp_session == NULL)
        return;

    pppSession = (pppSession_t *)pcb->ppp_session;
    osiNotifyTrigger(pppSession->dl_read_notify);
}

static void pppos_linkstate_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    pppSession_t *ppp = (pppSession_t *)ctx;

    OSI_LOGI(0x10005631, "PPP linkstate err_code = %d", err_code);
    if (err_code == PPPERR_CONNECT || err_code == PPPERR_USER || err_code == PPPERR_PEERDEAD || err_code == PPPERR_AUTHFAIL)
    {
        if (ppp->end_cb != NULL)
        {
            ppp->end_cb(ppp->output_cb_ctx, err_code);
            ppp->end_cb = NULL;
        }
    }
}

static u32_t pppos_output_cb(ppp_pcb *pcb, u8_t *data, u32_t len, void *ctx)
{
    pppSession_t *ppp = (pppSession_t *)ctx;

    //OSI_LOGI(0x10005632, "PPP dl output len/%d lcp_fsm/%d lpcp_fsm/%d",
    //         len, pcb->lcp_fsm.state, pcb->ipcp_fsm.state);

    if (ppp->output_cb != NULL)
        ppp->output_cb(ppp->output_cb_ctx, data, len);
    return len;
}

void ppp_StartGprs(u8_t *usrname, u8_t *usrpwd, void *ctx)
{
    OSI_LOGXI(OSI_LOGPAR_SS, 0x10005633, "PPP set usrname/'%s' usrpwd/'%s'", usrname, usrpwd);
}

static void _setPppAddress(pppSession_t *ppp)
{
    struct netif *nif = ppp->nif;
    if (nif == NULL)
    {
        OSI_LOGE(0x10005634, "PPP set address without netif");
        return;
    }

    ip4_addr_t addr;
    /* Set our address */
    IP4_ADDR(&addr, 192, 168, 0, 1);
    ppp_set_ipcp_ouraddr(ppp->pcb, &addr);
    ppp->pcb->ipcp_wantoptions.accept_local = 1;

    /* Set peer(his) address */
    ppp_set_ipcp_hisaddr(ppp->pcb, netif_ip4_addr(ppp->nif));

    /* Set primary DNS server */
    ip_addr_t dns_server0;
    CFW_GPRS_PDPCONT_INFO_V2 pdp_context;
    CFW_GprsGetPdpCxtV2(ppp->cid, &pdp_context, ppp->sim);
    IP_ADDR4(&dns_server0, pdp_context.pPdpDns[0], pdp_context.pPdpDns[1],
             pdp_context.pPdpDns[2], pdp_context.pPdpDns[3]);
    dns_setserver(0, &dns_server0);
    ppp_set_ipcp_dnsaddr(ppp->pcb, 0, ip_2_ip4(&dns_server0));

    /* Set secondary DNS server */

    OSI_LOGI(0x10005635, "PPP address local/%08x his/%08x dns/%08x",
             ppp->pcb->ipcp_wantoptions.ouraddr,
             ppp->pcb->ipcp_wantoptions.hisaddr,
             ppp->pcb->ipcp_allowoptions.dnsaddr[0]);
}
#if PPP_AUTHGPRS_SUPPORT
static void _pppAuthActDone(pppSession_t *ppp, int _iActiavedbyPPP)
{
    uint8_t simId = ppp->sim;
    uint8_t iCnt = 0;

    struct netif *netif = getGprsNetIf(simId, ppp->cid);
    while (netif == NULL || iCnt < 20)
    {
        osiThreadSleep(5);
        netif = getGprsNetIf(simId, ppp->cid);
        iCnt++;
    }
    ppp->dl_read_notify = osiNotifyCreate(atEngineGetThreadId(), _psIntfRead, netif);

    ppp->nif = netif;
    if (ppp->nif == NULL)
    {
        OSI_LOGE(0x10005639, "PPP netif is NULL");
        return;
    }
    if (ppp->nif->pspathIntf == NULL)
    {
        OSI_LOGE(0x1000563a, "PPP pspath interface is NULL");
        return;
    }
    if (ppp->nif->link_mode != NETIF_LINK_MODE_LWIP)
    {
        OSI_LOGE(0, "_pppStart wrong mode");
        return;
    }
    ppp->old_psintf_cb = drvPsIntfSetDataArriveCB(ppp->nif->pspathIntf, _psIntfCB);
    ppp->old_output = ppp->nif->output;
    ppp->nif->output = _psIntfWrite;

    ppp->nif->state = ppp->pcb;
    ppp->nif->is_ppp_mode = 1;
    ppp->nif->link_mode = NETIF_LINK_MODE_PPP;

    OSI_LOGI(0, "set mtu");
    ppp_pcb *pcb = ppp->pcb;
    lcp_options *wo = &pcb->lcp_wantoptions;
    lcp_options *ho = &pcb->lcp_hisoptions;
    lcp_options *go = &pcb->lcp_gotoptions;
    lcp_options *ao = &pcb->lcp_allowoptions;
    int mtu, mru;

    if (!go->neg_magicnumber)
        go->magicnumber = 0;
    if (!ho->neg_magicnumber)
        ho->magicnumber = 0;

    /*
     * Set our MTU to the smaller of the MTU we wanted and
     * the MRU our peer wanted.  If we negotiated an MRU,
     * set our MRU to the larger of value we wanted and
     * the value we got in the negotiation.
     * Note on the MTU: the link MTU can be the MRU the peer wanted,
     * the interface MTU is set to the lowest of that, the
     * MTU we want to use, and our link MRU.
     */
    mtu = ho->neg_mru ? ho->mru : PPP_MRU;
    mru = go->neg_mru ? LWIP_MAX(wo->mru, go->mru) : PPP_MRU;

    pcb->netif = ppp->nif;
    pcb->netif->mtu = LWIP_MIN(LWIP_MIN(mtu, mru), ao->mru);

#if !LWIP_SINGLE_NETIF
    if (netif_default == ppp->nif && netif_list != NULL)
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
}

static void _pppAuthActRsp(pppSession_t *ppp, const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;

    OSI_LOGI(0, "_pppAuthActRsp EV_CFW_GPRS_ACT_RSP");
    if (cfw_event->nType != CFW_GPRS_ACTIVED)
    {
        return;
    }

    //send event to netthread and wait for netif created
    OSI_LOGI(0x10005667, "We got CFW_GPRS_ACTIVED");
    osiEvent_t tcpip_event = *event;
    tcpip_event.id = EV_TCPIP_CFW_GPRS_ACT;
    CFW_EVENT *tcpip_cfw_event = (CFW_EVENT *)&tcpip_event;
    tcpip_cfw_event->nUTI = 0;
    osiEventSend(netGetTaskID(), (const osiEvent_t *)&tcpip_event);

    _pppAuthActDone(ppp, 1);
}

static void _pppAuthActReq(pppSession_t *ppp)
{

    uint8_t act_state = 0;
    uint8_t sim = ppp->sim;
    uint8_t cid = ppp->cid;
    uint16_t uti = 0;
    uint32_t ret;

    CFW_GetGprsActState(cid, &act_state, sim);
    if (act_state != CFW_GPRS_ACTIVED)
    {
        uint8_t oper_id[6];
        uint8_t mode;
        CFW_GPRS_PDPCONT_INFO_V2 sPdpCont;
        CFW_GPRS_PDPCONT_INFO_V2 sPdpContSet;
        memset(&sPdpContSet, 0x00, sizeof(CFW_GPRS_PDPCONT_INFO_V2));
        memset(&sPdpCont, 0x00, sizeof(CFW_GPRS_PDPCONT_INFO_V2));
        ret = CFW_GprsGetPdpCxtV2(cid, &sPdpCont, sim);
        if (ret != 0)
        {
            if (CFW_GprsSetPdpCxtV2(cid, &sPdpContSet, sim) != 0)
            {
                return;
            }
        }
        CFW_GprsGetPdpCxtV2(cid, &sPdpCont, sim);
        if (sPdpCont.nApnSize == 0)
        {
            if (CFW_NwGetCurrentOperator(oper_id, &mode, sim) != 0)
            {
                return;
            }
            const char *defaultApnInfo = NULL;
            if (AT_GetOperatorDefaultApn(oper_id, &defaultApnInfo))
            {
                sPdpCont.nApnSize = strlen(defaultApnInfo);
                OSI_LOGXI(OSI_LOGPAR_S, 0x00000000, "sPdpCont.defaultApnInfo %s", defaultApnInfo);
                OSI_LOGI(0, "sPdpCont.nApnSize: %d", sPdpCont.nApnSize);
                if (sPdpCont.nApnSize >= THE_APN_MAX_LEN)
                {
                    return;
                }
                memcpy(sPdpCont.pApn, defaultApnInfo, sPdpCont.nApnSize);
            }
        }
        if (sPdpCont.nPdpType == 0)
        {
            sPdpCont.nPdpType = CFW_GPRS_PDP_TYPE_IPV4V6; /* Default IPV4 */
        }
        sPdpCont.nDComp = 0;
        sPdpCont.nHComp = 0;
        sPdpCont.nNSLPI = 0;
        CFW_GPRS_QOS Qos = {3, 4, 3, 4, 16};
        CFW_GprsSetReqQos(cid, &Qos, sim);
        OSI_LOGI(0, "ppp->pcb->auth_type: %d", ppp->pcb->auth_type);
        if (ppp->pcb->auth_type == 1)
        {
            memcpy(sPdpCont.pApnUser, ppp->pcb->peer_username, ppp->pcb->len_username);
            OSI_LOGXI(OSI_LOGPAR_S, 0, "ppp->pcb->peer_username: %s", ppp->pcb->peer_username);
            sPdpCont.nApnUserSize = ppp->pcb->len_username;
            memcpy(sPdpCont.pApnPwd, ppp->pcb->peer_response, ppp->pcb->len_response);
            OSI_LOGXI(OSI_LOGPAR_S, 0, "ppp->pcb->peer_response: %s", ppp->pcb->peer_response);
            sPdpCont.nApnPwdSize = ppp->pcb->len_response;
            sPdpCont.nAuthProt = 1;
            OSI_LOGI(0, "sPdpCont.nApnUserSize: %d, sPdpCont.nApnPwdSize: %d sPdpCont.nAuthProt %d", sPdpCont.nApnUserSize, sPdpCont.nApnPwdSize, sPdpCont.nAuthProt);
        }
        else if (ppp->pcb->auth_type == 2)
        {
            memcpy(sPdpCont.pApnUser, ppp->pcb->peer_username, ppp->pcb->len_username);
            OSI_LOGXI(OSI_LOGPAR_S, 0, "ppp->pcb->peer_username: %s", ppp->pcb->peer_username);
            sPdpCont.nApnUserSize = ppp->pcb->len_username;

            memcpy(sPdpCont.pApnPwd, ppp->pcb->peer_response, ppp->pcb->len_response);
            int dumplen = ppp->pcb->len_response;
            OSI_LOGI(0, "ppp->pcb->len_response: %d", dumplen);
            OSI_LOGXI(OSI_LOGPAR_M, 0x0, "ppp->pcb->peer_response : %*s", dumplen, ppp->pcb->peer_response);
            dumplen = ppp->pcb->chap_challenge_len;
            memcpy(sPdpCont.pApnPwd + ppp->pcb->len_response, ppp->pcb->chap_challenge, ppp->pcb->chap_challenge_len);
            OSI_LOGXI(OSI_LOGPAR_M, 0x0, "ppp->pcb->chap_challenge : %*s", dumplen, ppp->pcb->chap_challenge);

            sPdpCont.nApnPwdSize = ppp->pcb->len_response + ppp->pcb->chap_challenge_len;
            sPdpCont.nAuthProt = 3;
            OSI_LOGI(0, "sPdpCont.nApnUserSize: %d, sPdpCont.nApnPwdSize: %d sPdpCont.nAuthProt %d", sPdpCont.nApnUserSize, sPdpCont.nApnPwdSize, sPdpCont.nAuthProt);
            dumplen = ppp->pcb->chap_challenge_len + ppp->pcb->len_response;
            OSI_LOGXI(OSI_LOGPAR_M, 0x0, "sPdpCont.pApnPwd : %*s", dumplen, sPdpCont.pApnPwd);
        }
        if (CFW_GprsSetPdpCxtV2(cid, &sPdpCont, sim) != 0)
        {
            return;
        }
        OSI_LOGI(0, "_pppAuthActReq current Task ID %lu\n", (uint32_t)osiThreadCurrent());
        uti = cfwRequestUTI((osiEventCallback_t)_pppAuthActRsp, ppp);
        if ((ret = CFW_GprsAct(CFW_GPRS_ACTIVED, cid, uti, sim)) != 0)
        {
            cfwReleaseUTI(uti);
            return;
        }

        ppp->pcb->acted = 1;
        return;
    }
    _pppAuthActDone(ppp, 1);
}

static void _pppAuthAttRsp(pppSession_t *ppp, const osiEvent_t *event)
{
    // EV_CFW_GPRS_ATT_RSP
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    OSI_LOGI(0, "_pppAuthAttRsp att_state %p", ppp);
    if (cfw_event->nType != CFW_GPRS_ATTACHED)
    {
        return;
    }
    OSI_LOGI(0, "_pppAuthAttRsp att_state %p", ppp);
    _pppAuthActReq(ppp);
}

void pppAuthDoActive(void *_ppp)
{
    pppSession_t *ppp = (pppSession_t *)_ppp;
    uint8_t cid = ppp->cid;
    OSI_LOGI(0, "current Task ID %lu\n", (uint32_t)osiThreadCurrent());
    OSI_LOGXI(OSI_LOGPAR_IS, 0, "enter PPP mode cid/%d", cid);
    uint8_t sim = ppp->sim;
    uint32_t ret;
    uint16_t uti = 0;

    uint8_t att_state = 0;
    CFW_GetGprsAttState(&att_state, sim);
    OSI_LOGI(0, "pppAuthDoActive att_state %d", att_state);
    if (att_state != CFW_GPRS_ATTACHED)
    {
        uti = cfwRequestUTI((osiEventCallback_t)_pppAuthAttRsp, ppp);
        if ((ret = CFW_GprsAtt(CFW_GPRS_ATTACHED, uti, sim)) != 0)
        {
            cfwReleaseUTI(uti);
        }
        return;
    }

    _pppAuthActReq(ppp);
}
#endif
#if 0
void pppSetCmdEngine(pppSession_t *ppp, void *cmd)
{
	if(ppp == NULL || cmd == NULL)
		goto LEAVE;
	ppp->cmd = (atCommand_t *)cmd;
LEAVE:
	return;
}
#endif
static void ppp_notify_phase_cb(ppp_pcb *pcb, u8_t phase, void *ctx)
{
    pppSession_t *ppp = (pppSession_t *)ctx;

#if PPP_IPV6_SUPPORT
    OSI_LOGI(0x10005636, "PPP status phase:%d lcp_fsm:%d, lpcp_fsm:%d, ipv6cp_fsm:%d",
             phase, pcb->lcp_fsm.state, pcb->ipcp_fsm.state, pcb->ipv6cp_fsm.state);
#else
    OSI_LOGI(0x10005637, "PPP status phase:%d lcp_fsm:%d, lpcp_fsm:%d",
             phase, pcb->lcp_fsm.state, pcb->ipcp_fsm.state);
#endif

    switch (phase)
    {
    case PPP_PHASE_NETWORK:
        if (pcb->lcp_fsm.state == PPP_FSM_STOPPING)
        {
            //Terminating
            //CFW_GprsAct(CFW_GPRS_DEACTIVED, ppp->cid, Act_UTI, ppp->sim);
            //cfwRespSetUtiCB(gAtCfwCtx.resp_man, Act_UTI, cfwActRsp_CB, ppp);
        }
        else if (pcb->lcp_fsm.state == PPP_FSM_OPENED)
        {
            if (pcb->ipcp_fsm.state == PPP_FSM_CLOSED
#if PPP_IPV6_SUPPORT
                || pcb->ipv6cp_fsm.state == PPP_FSM_CLOSED
#endif
            )
            {
                if (ppp->nif != NULL)
                    _setPppAddress(ppp);
            }
        }
        break;
    /* Session is down (either permanently or briefly) */
    case PPP_PHASE_DEAD:
        break;

    /* We are between two sessions */
    case PPP_PHASE_HOLDOFF:
        //led_set(PPP_LED, LED_SLOW_BLINK);
        break;

    /* Session just started */
    case PPP_PHASE_INITIALIZE:
        //led_set(PPP_LED, LED_FAST_BLINK);
        break;

    /* Session is running */
    case PPP_PHASE_RUNNING:
        //led_set(PPP_LED, LED_ON);
        break;
#if PPP_AUTHGPRS_SUPPORT
    /* Session is on PPP_PHASE_AUTHENTICATE */
    case PPP_PHASE_CALLBACK:
        sys_arch_printf("ppp_notify_phase_cb PPP_PHASE_CALLBACK\n");
#if 0		
        while ((CFW_GetGprsActState(ppp->cid, &act_state, ppp->sim) == 0) && retryCount++ <= 20)
        {
            sys_arch_printf("ppp_notify_phase_cb waiting for gprs act_state = %d actived:%d\n", act_state, retryCount);

            if (act_state == CFW_GPRS_ACTIVED && getGprsNetIf(ppp->sim, ppp->cid) != NULL)
            {
                //if (ppp->nif != NULL)
                //    _setPppAddress(ppp);

                uint8_t sim = ppp->sim;
                uint8_t cid = ppp->cid;

                //save cid for deactive gprs
                AT_Gprs_CidInfo *pinfo = &gAtCfwCtx.sim[sim].cid_info[cid];
                OSI_LOGI(0, "_pppActReq sim = %d async->cid = %d", sim, cid);
                pinfo->uCid = cid;
                pinfo->uState = CFW_GPRS_ACTIVED;

                //osiThreadSleep(100);

                _pppAuthActDone(ppp, 1);

                break;
            }
            else if ((act_state != CFW_GPRS_ACTIVED) && retryCount == 1)
            {
                //pppAuthDoActive(ppp);
                sys_arch_printf("current Task ID %lu\n", (uint32_t)osiThreadCurrent());
#if LWIP_TCPIP_CORE_LOCKING //ÌáÇ°½âËø
                UNLOCK_TCPIP_CORE();
#endif
                sys_timeout(500, pppAuthDoActive, ppp);
            }
            osiThreadSleep(1000);
        }
#else
        pppAuthDoActive(ppp);
#endif
        break;
#endif
    default:
        break;
    }
}

static int _pppStart(pppSession_t *ppp)
{
    OSI_LOGI(0x10005638, "PPP start ...");
    //#ifndef PPP_AUTHGPRS_SUPPORT
    if (ppp->cgact_activated >> 7 == 1)
    {
        uint8_t cid = ppp->cid;
        uint8_t sim = ppp->sim;

        ppp->nif = getGprsNetIf(sim, cid);
        if (ppp->nif == NULL)
        {
            OSI_LOGE(0x10005639, "PPP netif is NULL");
            return -1;
        }
        if (ppp->nif->pspathIntf == NULL)
        {
            OSI_LOGE(0x1000563a, "PPP pspath interface is NULL");
            return -1;
        }
        if (ppp->nif->link_mode != NETIF_LINK_MODE_LWIP)
        {
            OSI_LOGE(0, "_pppStart wrong mode");
            return -1;
        }
        ppp->old_psintf_cb = drvPsIntfSetDataArriveCB(ppp->nif->pspathIntf, _psIntfCB);
        ppp->old_output = ppp->nif->output;
        ppp->nif->output = _psIntfWrite;
    }
    //#endif
    ppp->pcb = pppos_create(ppp->nif, pppos_output_cb, pppos_linkstate_cb, (void *)ppp);
    if (ppp->pcb == NULL)
    {
        ppp->nif = NULL;
        OSI_LOGE(0x1000563b, "PPP pppos_create failed");
        return -1;
    }
    ppp->pcb->ppp_session = ppp;
#if PPP_AUTHGPRS_SUPPORT
    ppp->pcb->cid_id = ppp->cid;
    ppp->pcb->sim_id = ppp->sim;
#endif
    //#ifndef PPP_AUTHGPRS_SUPPORT
    if (ppp->cgact_activated >> 7 == 1)
    {
        ppp->nif->state = ppp->pcb;
        ppp->nif->is_ppp_mode = 1;
        ppp->nif->link_mode = NETIF_LINK_MODE_PPP;
#if !LWIP_SINGLE_NETIF
        if (netif_default == ppp->nif && netif_list != NULL)
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
        _setPppAddress(ppp);
    }
    //#endif
    ppp_set_notify_phase_callback(ppp->pcb, ppp_notify_phase_cb);

#if PAP_FOR_SIM_AUTH && PPP_AUTH_SUPPORT
    /* Auth configuration, this is pretty self-explanatory */
    ppp_set_auth(ppp->pcb, PPPAUTHTYPE_ANY, "login", "password");

    /* Require peer to authenticate */
    ppp_set_auth_required(ppp->pcb, 1);
#endif

    /*
     * Only for PPPoS, the PPP session should be up and waiting for input.
     *
     * Note: for PPPoS, ppp_connect() and ppp_listen() are actually the same thing.
     * The listen call is meant for future support of PPPoE and PPPoL2TP server
     * mode, where we will need to negotiate the incoming PPPoE session or L2TP
     * session before initiating PPP itself. We need this call because there is
     * two passive modes for PPPoS, ppp_set_passive and ppp_set_silent.
     */
    ppp_set_silent(ppp->pcb, 1);

    /*
     * Initiate PPP listener (i.e. wait for an incoming connection), can only
     * be called if PPP session is in the dead state (i.e. disconnected).
     */
    ppp_listen(ppp->pcb);

    OSI_LOGI(0x1000563c, "PPP start done");
    return 0;
}
#if 0
static void _freeppppcb(void *ctx)
{
    if (ctx != NULL)
    {
        ppp_pcb *pcb = (ppp_pcb *)ctx;
        ppp_free(pcb);
    }
}
#endif
static int _pppStop(pppSession_t *ppp)
{
    if (ppp == NULL)
        return -1;

    OSI_LOGI(0x1000563d, "PPP stop %p pcb/%p nif/%p", ppp, ppp->pcb, ppp->nif);
    if (ppp->pcb != NULL)
    {
        //ppp_close(ppp->pcb, 0);
        if (ppp->nif != NULL)
        {
            ppp->nif->state = NULL;
        }
        ppp_free(ppp->pcb);
        //osiThreadCallback(netGetTaskID(), _freeppppcb, (void *)ppp->pcb);
        ppp->pcb = NULL;
    }

    if (ppp->nif != NULL && ppp->nif->is_used == 1)
    {
        if (ppp->nif->link_mode == NETIF_LINK_MODE_PPP)
        {
            drvPsIntfSetDataArriveCB(ppp->nif->pspathIntf, lwip_pspathDataInput);
            ppp->nif->link_mode = NETIF_LINK_MODE_LWIP;
        }
        ppp->nif->output = ppp->old_output;
        uint8_t addr[THE_PDP_ADDR_MAX_LEN];
        uint8_t length;
        CFW_GprsGetPdpAddr(ppp->cid, &length, addr, ppp->sim);
        uint32_t ip_addr = (addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0];
        OSI_LOGI(0x1000563e, "PPP restore netif ip address 0x%08x", ip_addr);

        ip4_addr_t ip = {0};
        ip.addr = ip_addr;
        netif_set_addr(ppp->nif, &ip, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
        netif_set_link_up(ppp->nif);
        ppp->nif->is_ppp_mode = 0;
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
    }
#endif
    ppp->output_cb = NULL;
    return 0;
}

int pppSessionSuspend(pppSession_t *ppp)
{
    return 0;
}

int pppSessionResume(pppSession_t *ppp)
{
    return 0;
}

int pppSessionInput(pppSession_t *ppp, const void *data, size_t size)
{
    int iRet = -1;
#if LWIP_TCPIP_CORE_LOCKING
    LOCK_TCPIP_CORE();
#endif
    if (ppp == NULL || ppp->pcb == NULL)
    {
        goto LEAVE;
    }

#ifdef CONFIG_NET_TRACE_IP_PACKET
    if (size > 30)
    {
        uint8_t *ipdata = (uint8_t *)data + 2;
        uint16_t identify = (ipdata[4] << 8) + ipdata[5];
        OSI_LOGI(0x10005663, "PPP UL AT input identify %04x", identify);
    }
#endif

    pppos_input(ppp->pcb, (uint8_t *)data, (int)size);
    iRet = 0;
LEAVE:
#if LWIP_TCPIP_CORE_LOCKING
    UNLOCK_TCPIP_CORE();
#endif
    return iRet;
}

pppSession_t *pppSessionCreate(uint8_t cid, uint8_t sim, osiThread_t *dl_thread,
                               pppOutputCallback_t output_cb, void *output_cb_ctx,
                               pppEndCallback_t end_cb, void *end_cb_ctx,
                               pppFlowControlCallback_t flowctrl_cb, void *flowctrl_cb_ctx,
                               int _iActiavedbyPPP)
{
    OSI_LOGI(0x10005640, "PPP session create");
    pppSession_t *ppp = NULL;
#if LWIP_TCPIP_CORE_LOCKING
    LOCK_TCPIP_CORE();
#endif

    struct netif *netif = getGprsNetIf(sim, cid);
    if (netif != NULL && netif->is_ppp_mode)
    {
        OSI_LOGE(0x10005641, "PPP create already in ppp mode");
        goto LEAVE;
    }

    ppp = (pppSession_t *)calloc(1, sizeof(pppSession_t));
    if (ppp == NULL)
        goto LEAVE;
    memset(ppp, 0, sizeof(pppSession_t));

    ppp->cid = cid;
    ppp->sim = sim;
    ppp->output_cb = output_cb;
    ppp->output_cb_ctx = output_cb_ctx;
    ppp->end_cb = end_cb;
    ppp->end_cb_ctx = end_cb_ctx;
    ppp->is_deleted = 1;
    ppp->cgact_activated = _iActiavedbyPPP;
    OSI_LOGE(0, "ppp->cgact_activated = %d", _iActiavedbyPPP);
    ppp->dl_thread = dl_thread;
    //#ifndef PPP_AUTHGPRS_SUPPORT
    if (_iActiavedbyPPP >> 7 == 1)
        ppp->dl_read_notify = osiNotifyCreate(dl_thread, _psIntfRead, netif);
    //#endif
    ppp->flowctrl_cb = flowctrl_cb;
    ppp->flowctrl_cb_ctx = flowctrl_cb_ctx;

    int ret = _pppStart(ppp);
    if (ret != 0)
    {
        free(ppp);
        ppp = NULL;
        goto LEAVE;
    }
    OSI_LOGI(0x10005642, "PPP created %p", ppp);
LEAVE:
#if LWIP_TCPIP_CORE_LOCKING
    UNLOCK_TCPIP_CORE();
#endif
    return ppp;
}

bool pppSessionClose(pppSession_t *ppp, uint8_t termflag)
{
    bool bRet = false;
#if LWIP_TCPIP_CORE_LOCKING
    LOCK_TCPIP_CORE();
#endif
    OSI_LOGI(0x10005643, "PPP close %p, termflag/%d", ppp, termflag);

    if (ppp == NULL)
    {
        goto LEAVE;
    }
    if (termflag != 1 && termflag != 0)
        termflag = 1; //default set to disconnect rigth now
    ppp_close(ppp->pcb, termflag);
    bRet = true;
LEAVE:
#if LWIP_TCPIP_CORE_LOCKING
    UNLOCK_TCPIP_CORE();
#endif
    return bRet;
}

static void _pppDeactiveRsp(pppSession_t *ppp, const osiEvent_t *event)
{
    // EV_CFW_GPRS_ACT_RSP
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;

    OSI_LOGI(0x10005644, "PPP deactive response, nType/%d", cfw_event->nType);
    if (cfw_event->nType == CFW_GPRS_DEACTIVED)
    {
        OSI_LOGI(0, "PPP delete by cid = %d sim = %d", event->param1, (uint8_t)event->param3);
        pppSessionDeleteByNetifDestoryed(event->param3, event->param1);

        osiEvent_t tcpip_event = *event;
        tcpip_event.id = EV_TCPIP_CFW_GPRS_DEACT;
        CFW_EVENT *tcpip_cfw_event = (CFW_EVENT *)&tcpip_event;
        tcpip_cfw_event->nUTI = 0;
        osiEventSend(netGetTaskID(), (const osiEvent_t *)&tcpip_event);
    }
}

bool pppSessionDelete(pppSession_t *ppp)
{
    OSI_LOGI(0x10005645, "PPP %p delete ...", ppp);

    if (ppp == NULL)
        return false;

    if (ppp->is_deleted == 0)
    {
        OSI_LOGE(0x10005646, "PPP double delete");
        return true;
    }

    ppp->is_deleted = 0;
    _pppStop(ppp);

    //deactivate net if activated by PPP session
    OSI_LOGI(0x10005647, "PPP cgact_activated/%d cid/%d", ppp->cgact_activated, ppp->cid);
    if ((ppp->cgact_activated & 0x01) == 1 && ppp->cid != 0xFF)
    {
        uint8_t act_state = 0;
        int ret = CFW_GetGprsActState(ppp->cid, &act_state, ppp->sim);
        if (act_state != CFW_GPRS_DEACTIVED && ret == 0)
        {
            uint16_t uti = cfwRequestUTI((osiEventCallback_t)_pppDeactiveRsp, ppp);
            uint32_t res = CFW_GprsAct(CFW_GPRS_DEACTIVED, ppp->cid, uti, ppp->sim);
            if (res != 0)
            {
                cfwReleaseUTI(uti);
                OSI_LOGE(0x10005648, "PPP deactive failed res/%d", res);
            }
        }
    }

    osiNotifyDelete(ppp->dl_read_notify);
    free(ppp);

    OSI_LOGI(0x10005649, "PPP delete done");
    return true;
}

void pppSessionDeleteByNetifDestoryed(uint8_t nSimId, uint8_t nCid)
{
    OSI_LOGI(0x1000564a, "PPP delete by sim/%d cid/%d", nSimId, nCid);
#if LWIP_TCPIP_CORE_LOCKING
    LOCK_TCPIP_CORE();
#endif
    struct netif *netif;
    netif = getGprsNetIf(nSimId, nCid);
    if (netif == NULL)
    {
        OSI_LOGE(0x1000564b, "PPP delete netif is NULL");
        goto LEAVE;
    }
    if (netif->is_ppp_mode == 0)
    {
        OSI_LOGE(0x1000564c, "PPP delete netif is not ppp mode");
        goto LEAVE;
    }
    if (netif->state == NULL)
    {
        OSI_LOGE(0x1000564d, "PPP delete netif state is NULL");
        goto LEAVE;
    }

    pppSession_t *pppSession = ((ppp_pcb *)netif->state)->ppp_session;
    if (pppSession == NULL)
    {
        OSI_LOGE(0x1000564e, "PPP delete session is NULL");
        goto LEAVE;
    }

    pppSessionClose(pppSession, 0);
LEAVE:
#if LWIP_TCPIP_CORE_LOCKING
    UNLOCK_TCPIP_CORE();
#endif
}

void pppSessionDeleteBySimCid(uint8_t nSimId, uint8_t nCid, uint8_t forceClose)
{
    OSI_LOGI(0x1000564a, "PPP delete by sim/%d cid/%d", nSimId, nCid);

    struct netif *netif;
    netif = getGprsNetIf(nSimId, nCid);
    if (netif == NULL)
    {
        OSI_LOGE(0x1000564b, "PPP delete netif is NULL");
        return;
    }
    if (netif->is_ppp_mode == 0)
    {
        OSI_LOGE(0x1000564c, "PPP delete netif is not ppp mode");
        return;
    }
    if (netif->state == NULL)
    {
        OSI_LOGE(0x1000564d, "PPP delete netif state is NULL");
        return;
    }

    pppSession_t *pppSession = ((ppp_pcb *)netif->state)->ppp_session;
    if (pppSession == NULL)
    {
        OSI_LOGE(0x1000564e, "PPP delete session is NULL");
        return;
    }

    pppSessionClose(pppSession, forceClose);
}
#else
pppSession_t *pppSessionCreate(uint8_t cid, uint8_t sim, osiThread_t *dl_thread,
                               pppOutputCallback_t output_cb, void *output_cb_ctx,
                               pppEndCallback_t end_cb, void *end_cb_ctx,
                               pppFlowControlCallback_t flowctrl_cb, void *flowctrl_cb_ctx,
                               int _iActiavedbyPPP)
{
    return NULL;
}
bool pppSessionDelete(pppSession_t *ppp)
{
    return NULL;
}
bool pppSessionClose(pppSession_t *ppp, uint8_t termflag)
{
    return NULL;
}
int pppSessionSuspend(pppSession_t *ppp)
{
    return 0;
}
int pppSessionResume(pppSession_t *ppp)
{
    return 0;
}
int pppSessionInput(pppSession_t *ppp, const void *data, size_t size)
{
    return -1;
}
void pppSessionDeleteByNetifDestoryed(uint8_t nSimId, uint8_t nCid)
{
    return;
}
void pppSessionDeleteBySimCid(uint8_t nSimId, uint8_t nCid, uint8_t forceClose)
{
    return;
}
#endif
