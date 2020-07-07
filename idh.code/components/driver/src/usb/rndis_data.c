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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('E', 'T', 'H', 'R')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_VERBOSE

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/errno.h>
#include <osi_api.h>
#include <osi_log.h>

#include "rndis_data.h"
#include "drv_usb.h"
#include "drv_names.h"
#include "rndis.h"
#include "netdev_interface.h"

typedef struct
{
    usbEthCfg_t cfg;
    bool enable; ///< ether port state
} rndisPriv_t;

static inline rndisData_t *prvEth2Rndis(drvEther_t *eth)
{
    return (rndisData_t *)eth->impl_ctx;
}

static inline rndisPriv_t *prvRndis2Priv(rndisData_t *rnd)
{
    return (rndisPriv_t *)rnd->priv;
}

static int prvRndisOpen(drvEther_t *eth)
{
    rndisData_t *rnd = prvEth2Rndis(eth);
    rndisPriv_t *priv = prvRndis2Priv(rnd);

    if (!priv->enable)
        return -ENODEV;

    rnd->rndis_open(rnd->func);

    return 0;
}

static void prvRndisClose(drvEther_t *eth)
{
    rndisData_t *rnd = prvEth2Rndis(eth);
    rnd->rndis_close(rnd->func);
}

static bool prvRndisReadyCB(drvEther_t *eth)
{
    rndisData_t *rnd = prvEth2Rndis(eth);
    return prvRndis2Priv(rnd)->enable;
}

static void prvRndisAddHead(drvEther_t *eth, uint8_t *buf, uint32_t rndis_len)
{
    struct rndis_packet_msg_type *header;

    header = (struct rndis_packet_msg_type *)buf;
    memset(header, 0, sizeof *header);
    header->MessageType = cpu_to_le32(RNDIS_MSG_PACKET);
    header->MessageLength = cpu_to_le32(rndis_len);
    header->DataOffset = cpu_to_le32(36);
    header->DataLength = cpu_to_le32(rndis_len - sizeof(*header));
}

static int prvRndisRemoveHead(drvEther_t *eth, drvEthReq_t *req, uint8_t *buf, uint32_t size)
{
    struct rndis_packet_msg_type *header;

    /* MessageType, MessageLength */
    header = (struct rndis_packet_msg_type *)buf;
    if (header->MessageType != RNDIS_MSG_PACKET)
    {
        return -EINVAL;
    }

    /* DataOffset, DataLength */
    req->payload = (drvEthPacket_t *)(buf + header->DataOffset + 8);
    req->payload_size = size - sizeof(*header);
    return 0;
}

static rndisPriv_t *prvRndisPrivCreate(rndisData_t *rnd)
{
    rndisPriv_t *p = calloc(1, sizeof(rndisPriv_t));
    if (p == NULL)
    {
        OSI_LOGE(0, "rndis data fail to allocate rndis priv");
        return NULL;
    }

    /* For PC */
    p->cfg.host_mac[0] = 0x02;
    p->cfg.host_mac[1] = 0x4b;
    p->cfg.host_mac[2] = 0xb3;
    p->cfg.host_mac[3] = 0xb9;
    p->cfg.host_mac[4] = 0xeb;
    p->cfg.host_mac[5] = 0xe5;

    /* For Module */
    p->cfg.dev_mac[0] = 0xfa;
    p->cfg.dev_mac[1] = 0x32;
    p->cfg.dev_mac[2] = 0x47;
    p->cfg.dev_mac[3] = 0x15;
    p->cfg.dev_mac[4] = 0xe1;
    p->cfg.dev_mac[5] = 0x88;

    udc_t *udc = rnd->func->controller;
    p->cfg.tx_ep = udcEpAlloc(udc, rnd->epin_desc);
    if (p->cfg.tx_ep == NULL)
        goto fail_tx_ep;

    p->cfg.rx_ep = udcEpAlloc(udc, rnd->epout_desc);
    if (p->cfg.rx_ep == NULL)
        goto fail_rx_ep;

    p->cfg.priv = rnd;
    p->cfg.udc = udc;
    p->cfg.ops.wrap = prvRndisAddHead;
    p->cfg.ops.unwrap = prvRndisRemoveHead;
    p->cfg.ops.ready = prvRndisReadyCB;
    p->cfg.ops.on_open = prvRndisOpen;
    p->cfg.ops.on_close = prvRndisClose;
    p->cfg.header_len = sizeof(struct rndis_packet_msg_type);

    return p;

fail_rx_ep:
    udcEpFree(udc, p->cfg.tx_ep);

fail_tx_ep:
    free(p);
    return NULL;
}

static void prvRndisPrivDestroy(rndisData_t *rnd, rndisPriv_t *p)
{
    udc_t *udc = rnd->func->controller;
    if (p->cfg.tx_ep)
        udcEpFree(udc, p->cfg.tx_ep);
    if (p->cfg.rx_ep)
        udcEpFree(udc, p->cfg.rx_ep);
    free(p);
}

int rndisEtherBind(rndisData_t *rnd)
{
    if (!rnd || !rnd->func || !rnd->func->controller ||
        !rnd->epin_desc || !rnd->epout_desc)
        return -1;

    OSI_LOGI(0, "CDC Ethernet bind\n");
    rndisPriv_t *priv = prvRndisPrivCreate(rnd);
    if (priv == NULL)
    {
        OSI_LOGE(0, "rndis data fail to create priv");
        return -1;
    }

    rnd->eth = usbEthCreate(&priv->cfg);
    if (rnd->eth == NULL)
        goto failed;

    rnd->epin_desc->bEndpointAddress = priv->cfg.tx_ep->address;
    rnd->epout_desc->bEndpointAddress = priv->cfg.rx_ep->address;
    rnd->priv = (unsigned long)priv;
    netdevInit(rnd->eth);

    return 0;

failed:
    prvRndisPrivDestroy(rnd, priv);
    return -1;
}

void rndisEtherUnbind(rndisData_t *rnd)
{
    OSI_LOGV(0, "Rndis Ethernet unbind\n");
    if (!rnd || !rnd->func)
        return;

    uint32_t critical = osiEnterCritical();
    rndisPriv_t *priv = prvRndis2Priv(rnd);
    drvEther_t *eth = rnd->eth;
    rnd->eth = NULL;
    rnd->priv = 0;
    osiExitCritical(critical);

    netdevExit();
    drvEtherDestroy(eth);
    prvRndisPrivDestroy(rnd, priv);
}

int rndisEtherEnable(rndisData_t *rnd)
{
    rndisPriv_t *priv = prvRndis2Priv(rnd);
    OSI_LOGI(0, "rndis ether enable");
    if (priv->enable)
    {
        OSI_LOGW(0, "rndis data enable, already enabled");
        return 0;
    }

    udc_t *udc = rnd->func->controller;
    int retval = udcEpEnable(udc, priv->cfg.tx_ep);
    if (retval < 0)
        return retval;

    retval = udcEpEnable(udc, priv->cfg.rx_ep);
    if (retval < 0)
        goto fail_rx;

    priv->enable = true;
    drvEtherEnable(rnd->eth);
    return 0;

fail_rx:
    udcEpDisable(udc, priv->cfg.tx_ep);

    return retval;
}

void rndisEtherDisable(rndisData_t *rnd)
{
    OSI_LOGI(0, "rndis ether disable");
    rndisPriv_t *priv = prvRndis2Priv(rnd);
    if (priv == NULL)
        return;

    uint32_t critical = osiEnterCritical();
    if (priv->enable == false)
    {
        osiExitCritical(critical);
        return;
    }
    priv->enable = false;
    osiExitCritical(critical);

    drvEtherDisable(rnd->eth);
    udc_t *udc = rnd->func->controller;
    udcEpDisable(udc, priv->cfg.tx_ep);
    udcEpDisable(udc, priv->cfg.rx_ep);
}
