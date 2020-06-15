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

#include "cfw.h"
#include "osi_api.h"
#include "osi_log.h"
#include "lwip/ip_addr.h"
#include "lwip/ip4.h"
#include "lwip/ip6_addr.h"
#include "lwip/prot/icmp6.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/ip6.h"
#include "lwip/prot/nd6.h"
#include "lwip/inet_chksum.h"
#include "lwip/dns.h"
#include "drv_ether.h"
#include <machine/endian.h>

static u8_t router_advertisement[] = {
    0x60, 0x0f, 0x4e, 0x00, 0x00, 0x58, 0x3a, 0xff,
    0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, /* IP src */
    0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbd, 0xb7, 0x35, 0x25, 0x47, 0x98, 0x99, 0x66, /* IP dst */
    0x86, 0x00, 0x00, 0x00, 0x40, 0x08, 0x0e, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ICMPV6 RA*/

    0x01, 0x01, 0x02, 0x4b, 0xb3, 0xb9, 0xeb, 0xe5,                                                 /*ICMPV6 Opt source link-layer addr*/
    0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x05, 0x78,                                                 /*ICMPV6 Opt MTU 1400*/
    0x03, 0x04, 0x40, 0xc0, 0x00, 0x00, 0x0e, 0x10, 0x00, 0x00, 0x0e, 0x10, 0x00, 0x00, 0x00, 0x00, /*ICMPV6 Opt Prefix*/
    0x24, 0x08, 0x84, 0xe3, 0x00, 0x0c, 0xb4, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*ICMPV6 Opt Prefix addr*/
    0x19, 0x03, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x10,                                                 /*ICMPV6 Opt DNS*/
    0x24, 0x08, 0x84, 0xe3, 0x00, 0x0c, 0xb4, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa9, /*ICMPV6 Opt DNS addr*/
};

static u8_t Neighbor_advertisement[] = {
    0x60, 0x00, 0x00, 0x00, 0x00, 0x20, 0x3a, 0xff,
    0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, /* IP src */
    0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbd, 0xb7, 0x35, 0x25, 0x47, 0x98, 0x99, 0x66, /* IP dst */
    0x88, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00,                                                 /* ICMPV6 NA*/
    0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, /* ICMPV6 NA Target Addr*/

    0x02, 0x01, 0x86, 0x27, 0x52, 0x72, 0x36, 0xc1, /*ICMPV6 Opt Target link-layer addr*/
};

bool isRouterSolicitationPackage(drvEthPacket_t *pkt, uint32_t size)
{
    bool isIPV6package = false;
    if (__ntohs(pkt->ethhd.h_proto) == ETH_P_IPV6)
    {
        isIPV6package = true;
    }

    if (!isIPV6package)
        return false;

    bool isV6 = false;
    bool isICMP6 = false;
    uint8_t *ipData = pkt->data;
    if (ipData[0] == 0x60) //V6
    {
        isV6 = true;
    }
    if (ipData[6] == 0x3a)
    {
        isICMP6 = true;
    }
    if (!isV6)
        return false;
    if (!isICMP6)
        return false;

    struct icmp6_hdr *icmp_pkg = (struct icmp6_hdr *)(ipData + 40);
    if (icmp_pkg->type == ICMP6_TYPE_RS)
    {
        OSI_LOGI(0x100075a7, "isRSPackage isICMP6 RS");
        return true;
    }
    return false;
}

bool isRouterAdvertisementPackage(drvEthPacket_t *pkt, uint32_t size)
{
    bool isV6 = false;
    bool isICMP6 = false;
    uint8_t *ipData = pkt->data;
    if (ipData[0] == 0x60) //V6
    {
        isV6 = true;
    }
    if (ipData[6] == 0x3a)
    {
        isICMP6 = true;
    }
    if (!isV6)
        return false;
    if (!isICMP6)
        return false;

    struct icmp6_hdr *icmp_pkg = (struct icmp6_hdr *)(ipData + 40);
    if (icmp_pkg->type == ICMP6_TYPE_RA)
    {
        return true;
    }
    return false;
}

void routerAdvertisement_reply(drvEther_t *ether, drvEthPacket_t *pkt, uint32_t size)
{
    drvEthReq_t *tx_req = drvEtherTxReqAlloc(ether);
    if (!tx_req)
        return;

    uint8_t *RS_reply = tx_req->payload->data;
    memcpy(RS_reply, router_advertisement, sizeof(router_advertisement));

    struct ip6_hdr *ip6_pkg_in = (struct ip6_hdr *)(pkt->data);
    struct ip6_hdr *ip6_pkg_out = (struct ip6_hdr *)(RS_reply);

    memcpy(&(ip6_pkg_out->dest), &(ip6_pkg_in->src), sizeof(ip6_addr_p_t));
    ip6_addr_t ip6addr_prefix;
    IP6_ADDR(&ip6addr_prefix, netif_ip6_addr(netif_default, 1)->addr[0], netif_ip6_addr(netif_default, 1)->addr[1], 0, 0);
    memcpy(RS_reply + 88, &(ip6addr_prefix), sizeof(ip6_addr_p_t));

    struct pbuf *p, *q;
    int totalLen = sizeof(router_advertisement) - 40;
    uint8_t *pData = RS_reply;
    int offset = 40;
    p = (struct pbuf *)pbuf_alloc(PBUF_RAW, totalLen, PBUF_POOL);
    struct ra_header *ra_hdr;
    ra_hdr = (struct ra_header *)(pData + 40);
    ra_hdr->chksum = 0;
    if (p != NULL)
    {
        if (totalLen > p->len)
        {
            for (q = p; totalLen > q->len; q = q->next)
            {
                memcpy(q->payload, &(pData[offset]), q->len);
                totalLen -= q->len;
                offset += q->len;
            }
            if (totalLen != 0)
            {
                memcpy(q->payload, &(pData[offset]), totalLen);
            }
        }
        else
        {
            memcpy(p->payload, &(pData[offset]), totalLen);
        }

        ip6_addr_t src_ip = {0};
        ip6_addr_copy_from_packed(src_ip, ip6_pkg_out->src);
        ip6_addr_t dst_ip = {0};
        ip6_addr_copy_from_packed(dst_ip, ip6_pkg_out->dest);

        ra_hdr->chksum = ip6_chksum_pseudo(p, IP6_NEXTH_ICMP6, p->len, &src_ip, &dst_ip);
        OSI_LOGI(0x100075a8, "RA chksum 0x%x", ra_hdr->chksum);
        pbuf_free(p);
    }

    if (!drvEtherTxReqSubmit(ether, tx_req, sizeof(router_advertisement)))
    {
        drvEtherTxReqFree(ether, tx_req);
    }

    sys_arch_dump(RS_reply, sizeof(router_advertisement));
}

bool isNeighborAdvertisementPackage(drvEthPacket_t *pkt, uint32_t size)
{
    bool isV6 = false;
    bool isICMP6 = false;
    uint8_t *ipData = pkt->data;
    if (ipData[0] == 0x60) //V6
    {
        isV6 = true;
    }
    if (ipData[6] == 0x3a)
    {
        isICMP6 = true;
    }
    if (!isV6)
        return false;
    if (!isICMP6)
        return false;

    struct icmp6_hdr *icmp_pkg = (struct icmp6_hdr *)(ipData + 40);
    if (icmp_pkg->type == ICMP6_TYPE_NA)
    {
        return true;
    }
    return false;
}

bool isNeighborSolicitationPackage(drvEthPacket_t *pkt, uint32_t size)
{
    bool isIPV6package = false;
    if (__ntohs(pkt->ethhd.h_proto) == ETH_P_IPV6)
    {
        isIPV6package = true;
    }
    if (!isIPV6package)
        return false;

    bool isV6 = false;
    bool isICMP6 = false;
    uint8_t *ipData = pkt->data;
    if (ipData[0] == 0x60) //V6
    {
        isV6 = true;
    }
    if (ipData[6] == 0x3a)
    {
        isICMP6 = true;
    }
    if (!isV6)
        return false;
    if (!isICMP6)
        return false;

    struct icmp6_hdr *icmp_pkg = (struct icmp6_hdr *)(ipData + 40);
    if (icmp_pkg->type == ICMP6_TYPE_NS)
    {
        OSI_LOGI(0x100075a9, "isRSPackage isICMP6 NS");
        return true;
    }
    return false;
}

void neighborAdvertisement_reply(drvEther_t *ether, drvEthPacket_t *pkt, uint32_t size)
{
    drvEthReq_t *tx_req = drvEtherTxReqAlloc(ether);
    if (!tx_req)
        return;

    uint8_t *NS_reply = tx_req->payload->data;
    memcpy(NS_reply, Neighbor_advertisement, sizeof(Neighbor_advertisement));

    struct ip6_hdr *ip6_pkg_in = (struct ip6_hdr *)(pkt->data);
    struct ip6_hdr *ip6_pkg_out = (struct ip6_hdr *)(NS_reply);
    ip6_addr_t src_ip = {0};
    ip6_addr_copy_from_packed(src_ip, ip6_pkg_in->src);
    if (ip6_addr_isany_val(src_ip))
    {
        drvEtherTxReqFree(ether, tx_req);
        return;
    }

    memcpy(&(ip6_pkg_out->dest), &(ip6_pkg_in->src), sizeof(ip6_addr_p_t));

    struct ns_header *ns_hdr = (struct ns_header *)(pkt->data + 40);

    memcpy(&(ip6_pkg_out->src), &(ns_hdr->target_address), sizeof(ip6_addr_p_t));
    memcpy(NS_reply + 48, &(ns_hdr->target_address), sizeof(ip6_addr_p_t));
    memcpy(NS_reply + 80, ether->host_mac, 6); /*ICMPV6 Opt Target link-layer addr*/

    struct pbuf *p, *q;
    int totalLen = sizeof(Neighbor_advertisement) - 40;
    uint8_t *pData = NS_reply;
    int offset = 40;
    p = (struct pbuf *)pbuf_alloc(PBUF_RAW, totalLen, PBUF_POOL);
    struct na_header *na_hdr;
    na_hdr = (struct na_header *)(pData + 40);
    na_hdr->chksum = 0;
    if (p != NULL)
    {
        if (totalLen > p->len)
        {
            for (q = p; totalLen > q->len; q = q->next)
            {
                memcpy(q->payload, &(pData[offset]), q->len);
                totalLen -= q->len;
                offset += q->len;
            }
            if (totalLen != 0)
            {
                memcpy(q->payload, &(pData[offset]), totalLen);
            }
        }
        else
        {
            memcpy(p->payload, &(pData[offset]), totalLen);
        }

        ip6_addr_t src_ip = {0};
        ip6_addr_copy_from_packed(src_ip, ip6_pkg_out->src);
        ip6_addr_t dst_ip = {0};
        ip6_addr_copy_from_packed(dst_ip, ip6_pkg_out->dest);

        na_hdr->chksum = ip6_chksum_pseudo(p, IP6_NEXTH_ICMP6, p->len, &src_ip, &dst_ip);
        OSI_LOGI(0x100075aa, "NA chksum 0x%x", na_hdr->chksum);
        pbuf_free(p);
    }

    if (!drvEtherTxReqSubmit(ether, tx_req, sizeof(Neighbor_advertisement)))
    {
        drvEtherTxReqFree(ether, tx_req);
    }

    sys_arch_dump(NS_reply, sizeof(Neighbor_advertisement));
}
