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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('E', 'C', 'M', 'D')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_INFO

#include "ecm_data.h"
#include "usb_ether.h"
#include <stdlib.h>
#include <sys/errno.h>
#include "osi_log.h"
#include "netdev_interface.h"

typedef struct
{
    usbEp_t *ep_in;
    usbEp_t *ep_out;
    drvEther_t *eth;
    bool enable;
} ecmPriv_t;

static inline ecmData_t *prvEth2Data(drvEther_t *eth)
{
    return (ecmData_t *)eth->impl_ctx;
}

static inline ecmPriv_t *prvEth2Priv(drvEther_t *eth)
{
    return (ecmPriv_t *)(prvEth2Data(eth)->priv);
}

static int prvEcmOpen(drvEther_t *eth)
{
    ecmData_t *ecm = prvEth2Data(eth);
    ecmPriv_t *p = prvEth2Priv(eth);
    if (!p->enable)
    {
        OSI_LOGE(0, "ECM open not enable");
        return -ENODEV;
    }

    ecm->ecm_open(ecm->func);

    return 0;
}

static void prvEcmClose(drvEther_t *eth)
{
    ecmData_t *ecm = prvEth2Data(eth);
    ecm->ecm_close(ecm->func);
}

static bool prvEcmCheckReady(drvEther_t *eth)
{
    return prvEth2Priv(eth)->enable;
}

static void prvEcmAddHead(drvEther_t *eth, uint8_t *buf, uint32_t datalen)
{
    ;
}

static int prvEcmRemoveHead(drvEther_t *eth, drvEthReq_t *req, uint8_t *buf, uint32_t size)
{
    req->payload = (drvEthPacket_t *)buf;
    req->payload_size = size;
    return 0;
}

static drvEther_t *prvEthCreate(ecmData_t *ecm, ecmPriv_t *p)
{
    usbEthCfg_t cfg = {};
    cfg.ops.wrap = prvEcmAddHead;
    cfg.ops.unwrap = prvEcmRemoveHead;
    cfg.ops.ready = prvEcmCheckReady;
    cfg.ops.on_open = prvEcmOpen;
    cfg.ops.on_close = prvEcmClose;

    cfg.udc = ecm->func->controller;
    cfg.tx_ep = p->ep_in;
    cfg.rx_ep = p->ep_out;
    cfg.header_len = 0;

    cfg.host_mac[0] = 0x02;
    cfg.host_mac[1] = 0x4b;
    cfg.host_mac[2] = 0xb3;
    cfg.host_mac[3] = 0xb9;
    cfg.host_mac[4] = 0xeb;
    cfg.host_mac[5] = 0xe5;

    cfg.dev_mac[0] = 0xfa;
    cfg.dev_mac[1] = 0x32;
    cfg.dev_mac[2] = 0x47;
    cfg.dev_mac[3] = 0x15;
    cfg.dev_mac[4] = 0xe1;
    cfg.dev_mac[5] = 0x88;
    cfg.priv = ecm;

    return usbEthCreate(&cfg);
}

ecmPriv_t *prvCreate(ecmData_t *ecm)
{
    ecmPriv_t *p = calloc(1, sizeof(ecmPriv_t));
    if (p == NULL)
        return NULL;

    p->ep_in = udcEpAlloc(ecm->func->controller, ecm->epin_desc);
    if (p->ep_in == NULL)
        goto fail_ep_in;

    p->ep_out = udcEpAlloc(ecm->func->controller, ecm->epout_desc);
    if (p->ep_out == NULL)
        goto fail_ep_out;

    p->eth = prvEthCreate(ecm, p);
    if (p->eth == NULL)
        goto fail_ether;

    return p;

fail_ether:
    udcEpFree(ecm->func->controller, p->ep_out);
    p->ep_out = NULL;

fail_ep_out:
    udcEpFree(ecm->func->controller, p->ep_in);
    p->ep_in = NULL;

fail_ep_in:
    free(p);

    return NULL;
}

bool ecmDataBind(ecmData_t *ecm)
{
    ecmPriv_t *priv = prvCreate(ecm);
    if (priv == NULL)
    {
        OSI_LOGE(0, "ecm data bind create private data fail");
        return false;
    }

    ecm->epin_desc->bEndpointAddress = priv->ep_in->address;
    ecm->epout_desc->bEndpointAddress = priv->ep_out->address;
    ecm->priv = priv;
    netdevInit(priv->eth);
    return true;
}

void ecmDataUnbind(ecmData_t *ecm)
{
    if (ecm || ecm->priv == NULL)
        return;

    ecmPriv_t *p = (ecmPriv_t *)ecm->priv;
    netdevExit();
    drvEtherDestroy(p->eth);
    udcEpFree(ecm->func->controller, p->ep_in);
    udcEpFree(ecm->func->controller, p->ep_out);
    ecm->priv = NULL;
    free(p);
}

bool ecmDataEnable(ecmData_t *ecm)
{
    OSI_LOGV(0, "ecm data enable");
    ecmPriv_t *p = (ecmPriv_t *)ecm->priv;
    if (p->enable)
    {
        OSI_LOGW(0, "ecm data enable, already enabled");
        return true;
    }

    udc_t *udc = ecm->func->controller;
    int retval = udcEpEnable(udc, p->ep_in);
    if (retval < 0)
    {
        OSI_LOGE(0, "ecm data enable ep in fail");
        return false;
    }

    retval = udcEpEnable(udc, p->ep_out);
    if (retval < 0)
        goto fail_out;

    p->enable = true;
    drvEtherEnable(p->eth);
    return true;

fail_out:
    udcEpDisable(udc, p->ep_in);
    return false;
}

void ecmDataDisable(ecmData_t *ecm)
{
    OSI_LOGV(0, "ecm data disable");
    uint32_t critical = osiEnterCritical();
    ecmPriv_t *p = (ecmPriv_t *)ecm->priv;
    if (!p->enable)
    {
        osiExitCritical(critical);
        return; // already disable
    }
    p->enable = false;
    osiExitCritical(critical);

    drvEtherDisable(p->eth);
    udc_t *udc = ecm->func->controller;
    udcEpDisable(udc, p->ep_in);
    udcEpDisable(udc, p->ep_out);
}
