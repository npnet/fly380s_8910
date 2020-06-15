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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('U', 'S', 'B', 'E')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_INFO

#include <osi_log.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <machine/endian.h>

#include "osi_compiler.h"
#include "drv_names.h"
#include "usb_ether.h"

#define ETHER_PACKET_SIZE (2048)
#define ETHER_TX_QUEUE_COUNT 64
#define ETHER_RX_QUEUE_COUNT 16

typedef struct usb_ether_req usbEthReq_t;
typedef struct usb_ether usbEther_t;
typedef TAILQ_ENTRY(usb_ether_req) usbEthReqEntry_t;
typedef TAILQ_HEAD(usb_ether_req_head, usb_ether_req) usbEthReqHead_t;

struct usb_ether_req
{
    drvEthReq_t req; // keep first
    usbEther_t *ue;
    usbXfer_t *xfer;
    usbEthReqEntry_t anchor;
};

struct usb_ether
{
    drvEther_t eth; // keep first
    usbEthCfg_t cfg;
    drvEthEventCB_t event_cb;
    drvEthULDataCB_t ul_cb;
    void *event_cb_ctx;
    void *ul_cb_ctx;
    bool open;
    bool rx_working;

    usbEthReqHead_t tx_free_reqs;
    usbEthReqHead_t rx_reqs;
    usbEthReqHead_t rx_frames;

    osiWorkQueue_t *rx_work_queue;
    osiWork_t *rx_work;
    drvEthStats_t stats;
};

static inline usbEthReq_t *prvReq2U(drvEthReq_t *req) { return (usbEthReq_t *)req; }
static inline usbEther_t *prvEth2U(drvEther_t *eth) { return (usbEther_t *)eth; }

static bool prvEtherRxStart(usbEther_t *ue);

static inline bool prvEtherReady(usbEther_t *ue)
{
    return ue->cfg.ops.ready(&ue->eth);
}

static void prvEtherRxWork(void *param)
{
    usbEther_t *ue = (usbEther_t *)param;
    uint32_t critical = osiEnterCritical();
    ue->rx_working = true;
    osiExitCritical(critical);

rx_continue:
    critical = osiEnterCritical();
    while (!TAILQ_EMPTY(&ue->rx_frames))
    {
        usbEthReq_t *ur = TAILQ_FIRST(&ue->rx_frames);
        TAILQ_REMOVE(&ue->rx_frames, ur, anchor);
        osiExitCritical(critical);

        if (ue->ul_cb)
            ue->ul_cb(ur->req.payload, ur->req.payload_size, ue->ul_cb_ctx);

        critical = osiEnterCritical();
        TAILQ_INSERT_TAIL(&ue->rx_reqs, ur, anchor);

        if (prvEtherReady(ue) && ue->open)
        {
            osiExitCritical(critical);
            prvEtherRxStart(ue);
            critical = osiEnterCritical();
        }
    }
    osiExitCritical(critical);

    if (!TAILQ_EMPTY(&ue->rx_frames))
        goto rx_continue;

    if (prvEtherReady(ue) && ue->open)
    {
        if (!TAILQ_EMPTY(&ue->rx_reqs))
            prvEtherRxStart(ue);

        // retry again
        if (!TAILQ_EMPTY(&ue->rx_frames))
            goto rx_continue;
    }

    critical = osiEnterCritical();
    ue->rx_working = false;
    osiExitCritical(critical);
}

static void prvEtherRxComplete(usbEp_t *ep, usbXfer_t *xfer)
{
    uint32_t critical = osiEnterCritical();
    usbEthReq_t *ur = (usbEthReq_t *)xfer->param;
    usbEther_t *ue = ur->ue;

    if (xfer->status == 0)
    {
        int retval = ue->cfg.ops.unwrap(&ue->eth, &ur->req, ur->xfer->buf, ur->xfer->actual);
        if (retval == 0)
        {
            ue->stats.rx_packets++;
            ue->stats.rx_bytes += xfer->actual;
            TAILQ_INSERT_TAIL(&ue->rx_frames, ur, anchor);
            if (!ue->rx_working)
                osiWorkEnqueue(ue->rx_work, ue->rx_work_queue);
        }
        else
        {
            OSI_LOGE(0, "unwrap ether packet fail %d", retval);
            ue->stats.rx_errors++;
            ue->stats.rx_length_errors++;
            TAILQ_INSERT_TAIL(&ue->rx_reqs, ur, anchor);
        }
    }
    else
    {
        OSI_LOGE(0, "rx complete fail %d", xfer->status);
        if (xfer->status != ECANCELED)
            ue->stats.rx_errors++;
        TAILQ_INSERT_TAIL(&ue->rx_reqs, ur, anchor);
    }

    osiExitCritical(critical);
    if (xfer->status != -ECANCELED && !TAILQ_EMPTY(&ue->rx_reqs))
    {
        prvEtherRxStart(ue);
    }
}

static void prvEtherTxComplete(usbEp_t *ep, usbXfer_t *xfer)
{
    usbEthReq_t *ur = (usbEthReq_t *)xfer->param;
    usbEther_t *ue = ur->ue;

    if (xfer->status == 0)
    {
        ue->stats.tx_bytes += xfer->actual;
    }
    else if (xfer->status != -ECANCELED)
    {
        ue->stats.tx_errors++;
        OSI_LOGE(0, "usb ether tx done fail, %d/%lu", xfer->status, ue->stats.tx_errors);
    }
    ue->stats.tx_packets++;
    TAILQ_INSERT_TAIL(&ue->tx_free_reqs, ur, anchor);
}

static bool prvEtherRxSubmit(usbEther_t *ue, usbEthReq_t *ur)
{
    usbXfer_t *xfer = ur->xfer;
    xfer->length = ETHER_PACKET_SIZE;
    xfer->param = ur;
    xfer->complete = prvEtherRxComplete;

    int retval = udcEpQueue(ue->cfg.udc, ue->cfg.rx_ep, xfer);
    if (retval < 0)
        OSI_LOGE(0, "enqueue rx failed return (%d)", retval);

    return retval == 0;
}

static bool prvEtherRxStart(usbEther_t *ue)
{
    uint32_t critical = osiEnterCritical();
    while (!TAILQ_EMPTY(&ue->rx_reqs))
    {
        usbEthReq_t *ur = TAILQ_FIRST(&ue->rx_reqs);
        TAILQ_REMOVE(&ue->rx_reqs, ur, anchor);
        osiExitCritical(critical);

        if (!prvEtherRxSubmit(ue, ur))
        {
            critical = osiEnterCritical();
            TAILQ_INSERT_TAIL(&ue->rx_reqs, ur, anchor);
            osiExitCritical(critical);
            return false;
        }

        critical = osiEnterCritical();
    }
    osiExitCritical(critical);
    return true;
}

const drvEthStats_t *drvEtherStatus(drvEther_t *eth)
{
    return eth ? &prvEth2U(eth)->stats : NULL;
}

static void prvEtherFreeOneQueue(usbEthReqHead_t *h, const usbEthCfg_t *cfg)
{
    usbEthReq_t *r;
    while ((r = TAILQ_FIRST(h)))
    {
        TAILQ_REMOVE(h, r, anchor);
        udcXferFree(cfg->udc, r->xfer);
    }
}

static bool prvEtherInitRequest(usbEther_t *ue, const usbEthCfg_t *cfg,
                                usbEthReq_t *ur, uint8_t *buffer)
{
    TAILQ_INIT(&ue->tx_free_reqs);
    TAILQ_INIT(&ue->rx_reqs);
    TAILQ_INIT(&ue->rx_frames);

    usbEthReq_t *r;
    for (unsigned i = 0; i < ETHER_TX_QUEUE_COUNT; ++i)
    {
        r = ur++;
        r->ue = ue;
        r->xfer = udcXferAlloc(cfg->udc);
        if (r->xfer == NULL)
            goto fail;
        r->xfer->buf = buffer;
        TAILQ_INSERT_TAIL(&ue->tx_free_reqs, r, anchor);
        buffer += ETHER_PACKET_SIZE;
    }

    for (unsigned i = 0; i < ETHER_RX_QUEUE_COUNT; ++i)
    {
        r = ur++;
        r->ue = ue;
        r->xfer = udcXferAlloc(cfg->udc);
        if (r->xfer == NULL)
            goto fail;
        r->xfer->buf = buffer;
        TAILQ_INSERT_TAIL(&ue->rx_reqs, r, anchor);
        buffer += ETHER_PACKET_SIZE;
    }
    return true;

fail:
    prvEtherFreeOneQueue(&ue->tx_free_reqs, cfg);
    prvEtherFreeOneQueue(&ue->rx_reqs, cfg);
    return false;
}

static void prvEtherExit(usbEther_t *ue)
{
    osiWorkDelete(ue->rx_work);
    osiWorkQueueDelete(ue->rx_work_queue);
    ue->rx_work_queue = NULL;
    ue->rx_work = NULL;

    prvEtherFreeOneQueue(&ue->tx_free_reqs, &ue->cfg);
    prvEtherFreeOneQueue(&ue->rx_reqs, &ue->cfg);
    prvEtherFreeOneQueue(&ue->rx_frames, &ue->cfg);
}

static bool prvEtherInit(usbEther_t *ue, const usbEthCfg_t *cfg)
{
    ue->rx_work_queue = osiWorkQueueCreate("ether_rx", 1, OSI_PRIORITY_HIGH, 2048);
    if (ue->rx_work_queue == NULL)
    {
        OSI_LOGE(0, "Ether init fail, create rx work queue");
        return false;
    }

    ue->rx_work = osiWorkCreate(prvEtherRxWork, NULL, ue);
    if (ue->rx_work == NULL)
    {
        OSI_LOGE(0, "Ether init fail, create rx work");
        osiWorkQueueDelete(ue->rx_work_queue);
        ue->rx_work_queue = NULL;
        return false;
    }

    return true;
}

static bool prvImplOpen(drvEther_t *eth)
{
    OSI_LOGI(0, "ether open");
    uint32_t critical = osiEnterCritical();
    usbEther_t *ue = prvEth2U(eth);
    if (ue->open)
    {
        osiExitCritical(critical);
        OSI_LOGW(0, "ether already open");
        return true;
    }
    osiExitCritical(critical);

    if (prvEtherReady(ue))
    {
        if (TAILQ_EMPTY(&ue->rx_reqs))
        {
            OSI_LOGE(0, "ether open rx submit empty");
            return false;
        }

        if (!prvEtherRxStart(ue))
        {
            OSI_LOGE(0, "ether open fail submit rx transfer");
            return false;
        }
    }

    if (ue->cfg.ops.on_open)
    {
        int retval = ue->cfg.ops.on_open(&ue->eth);
        if (retval < 0)
        {
            OSI_LOGE(0, "open the ether fail. (return %d)", retval);
            return false;
        }
    }
    ue->open = true;
    return true;
}

static void prvImplClose(drvEther_t *eth)
{
    OSI_LOGI(0, "ether close");
    uint32_t critical = osiEnterCritical();
    usbEther_t *ue = prvEth2U(eth);
    if (!ue->open)
    {
        osiExitCritical(critical);
        OSI_LOGW(0, "ether already close");
        return;
    }

    while (!TAILQ_EMPTY(&ue->rx_frames))
    {
        usbEthReq_t *r = TAILQ_FIRST(&ue->rx_frames);
        TAILQ_REMOVE(&ue->rx_frames, r, anchor);
        TAILQ_INSERT_TAIL(&ue->rx_reqs, r, anchor);
    }

    ue->open = false;
    osiExitCritical(critical);

    udcEpDequeueAll(ue->cfg.udc, ue->cfg.tx_ep);
    udcEpDequeueAll(ue->cfg.udc, ue->cfg.rx_ep);

    if (ue->cfg.ops.on_close)
        ue->cfg.ops.on_close(&ue->eth);
}

static drvEthReq_t *prvImplTxReqAlloc(drvEther_t *eth)
{
    uint32_t critical = osiEnterCritical();
    usbEther_t *ue = prvEth2U(eth);
    usbEthReq_t *ur = TAILQ_FIRST(&ue->tx_free_reqs);
    if (ur == NULL)
    {
        osiExitCritical(critical);
        ue->stats.tx_dropped++; // TODO
        OSI_LOGW(0, "usb eth allocate req fail");
        return NULL;
    }

    ur->req.payload = (drvEthPacket_t *)(ur->xfer->buf + ue->cfg.header_len);
    TAILQ_REMOVE(&ue->tx_free_reqs, ur, anchor);
    osiExitCritical(critical);
    return &ur->req;
}

static bool prvImplTxReqSubmit(drvEther_t *eth, drvEthReq_t *req, size_t size)
{
    usbEther_t *ue = prvEth2U(eth);
    usbEthReq_t *ur = prvReq2U(req);
    if (!ue->open || !prvEtherReady(ue))
    {
        ue->stats.tx_dropped++;
        return false;
    }

    ethHdr_t *ehdr = (ethHdr_t *)(ur->xfer->buf + ue->cfg.header_len);
    memcpy(ehdr->h_dest, eth->host_mac, ETH_ALEN);
    memcpy(ehdr->h_source, eth->dev_mac, ETH_ALEN);

    uint8_t *ipdata = (uint8_t *)ehdr + ETH_HLEN;
    if ((ipdata[0] & 0xF0) == 0x40)
        ehdr->h_proto = __htons(ETH_P_IP);
    else if ((ipdata[0] & 0xF0) == 0x60)
        ehdr->h_proto = __htons(ETH_P_IPV6);
    else if (ipdata[0] == 0x00 && ipdata[1] == 0x01 && ipdata[2] == 0x08 && ipdata[3] == 0x00)
        ehdr->h_proto = __htons(ETH_P_ARP);
    else
    {
        OSI_LOGE(0, "unknown ip version(%8x)", *((uint32_t *)ipdata));
        ue->stats.tx_dropped++;
        return false;
    }

    uint32_t length = size + ETH_HLEN + ue->cfg.header_len;
    ue->cfg.ops.wrap(&ue->eth, ur->xfer->buf, length);

    ur->xfer->zlp = 1;
    ur->xfer->length = length;
    ur->xfer->param = (void *)ur;
    ur->xfer->complete = prvEtherTxComplete;

    OSI_LOGD(0, "tx_xfer 0x%x, %p %u", ur->xfer, ur->xfer->buf, length);
    int retval = udcEpQueue(ue->cfg.udc, ue->cfg.tx_ep, ur->xfer);
    if (retval < 0)
    {
        OSI_LOGE(0, "usb ether enqueue tx failed return (%d)", retval);
        return false;
    }

    return true;
}

static void prvImplTxReqFree(drvEther_t *eth, drvEthReq_t *req)
{
    uint32_t critical = osiEnterCritical();
    TAILQ_INSERT_TAIL(&prvEth2U(eth)->tx_free_reqs, prvReq2U(req), anchor);
    osiExitCritical(critical);
}

static void prvImplSetEventCB(drvEther_t *eth, drvEthEventCB_t cb, void *priv)
{
    uint32_t critical = osiEnterCritical();
    usbEther_t *ue = prvEth2U(eth);
    ue->event_cb = cb;
    ue->event_cb_ctx = priv;
    osiExitCritical(critical);
}

static void prvImplSetUldataCB(drvEther_t *eth, drvEthULDataCB_t cb, void *priv)
{
    uint32_t critical = osiEnterCritical();
    usbEther_t *ue = prvEth2U(eth);
    ue->ul_cb = cb;
    ue->ul_cb_ctx = priv;
    osiExitCritical(critical);
}

static bool prvImplEnable(drvEther_t *eth)
{
    usbEther_t *ue = prvEth2U(eth);
    OSI_LOGI(0, "usb ether enable, %p", ue->event_cb);
    if (ue->event_cb)
        ue->event_cb(DRV_ETHER_EVENT_CONNECT, ue->event_cb_ctx);
    return true;
}

static void prvImplDisable(drvEther_t *eth)
{
    usbEther_t *ue = prvEth2U(eth);
    OSI_LOGI(0, "usb ether disable, %p", ue->event_cb);
    if (ue->event_cb)
        ue->event_cb(DRV_ETHER_EVENT_DISCONNECT, ue->event_cb_ctx);
}

static void prvImplDestroy(drvEther_t *eth)
{
    usbEther_t *ue = prvEth2U(eth);
    prvImplClose(eth);
    prvEtherExit(ue);
    free(ue);
}

drvEther_t *usbEthCreate(const usbEthCfg_t *cfg)
{
    if (!(cfg->ops.ready && cfg->ops.unwrap && cfg->ops.wrap))
    {
        OSI_LOGE(0, "Ether create ops constraint.");
        return NULL;
    }

    const uint32_t req_cnt = ETHER_TX_QUEUE_COUNT + ETHER_RX_QUEUE_COUNT;
    const uint32_t alloc_size = sizeof(usbEther_t) + CONFIG_CACHE_LINE_SIZE +
                                req_cnt * (sizeof(usbEthReq_t) + ETHER_PACKET_SIZE);
    usbEther_t *ue = (usbEther_t *)malloc(alloc_size);
    if (ue == NULL)
    {
        OSI_LOGE(0, "USB ether create fail. Not enough memory");
        return NULL;
    }

    memset(ue, 0, sizeof(*ue) + req_cnt * sizeof(usbEthReq_t));
    if (!prvEtherInit(ue, cfg))
        goto fail_init;

    usbEthReq_t *urs = (usbEthReq_t *)((uint8_t *)ue + sizeof(*ue));
    uint8_t *buffer = (uint8_t *)OSI_ALIGN_UP((uint32_t)urs + sizeof(usbEthReq_t) * req_cnt,
                                              CONFIG_CACHE_LINE_SIZE);
    if (!prvEtherInitRequest(ue, cfg, urs, buffer))
        goto fail_init_request;

    ue->cfg = *cfg;
    ue->eth.impl.open = prvImplOpen;
    ue->eth.impl.close = prvImplClose;
    ue->eth.impl.tx_req_alloc = prvImplTxReqAlloc;
    ue->eth.impl.tx_req_free = prvImplTxReqFree;
    ue->eth.impl.tx_req_submit = prvImplTxReqSubmit;
    ue->eth.impl.set_event_cb = prvImplSetEventCB;
    ue->eth.impl.set_uldata_cb = prvImplSetUldataCB;
    ue->eth.impl.enable = prvImplEnable;
    ue->eth.impl.disable = prvImplDisable;
    ue->eth.impl.destroy = prvImplDestroy;
    ue->eth.impl_ctx = cfg->priv;
    ue->eth.host_mac = ue->cfg.host_mac;
    ue->eth.dev_mac = ue->cfg.dev_mac;
    return &ue->eth;

fail_init_request:
    prvEtherExit(ue);

fail_init:
    free(ue);

    return NULL;
}
