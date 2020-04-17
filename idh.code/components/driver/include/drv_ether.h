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

#ifndef _DRV_ETHER_H_
#define _DRV_ETHER_H_

#include "osi_compiler.h"

OSI_EXTERN_C_BEGIN

#include <stdbool.h>
#include "drv_if_ether.h"

typedef struct drv_ether drvEther_t;

typedef struct
{
    ethHdr_t ethhd;
    uint8_t data[0];
} drvEthPacket_t;

typedef struct
{
    drvEthPacket_t *payload;
    uint32_t payload_size;
} drvEthReq_t;

typedef enum
{
    DRV_ETHER_EVENT_CONNECT = (1 << 0),    ///< ethernet connected
    DRV_ETHER_EVENT_DISCONNECT = (1 << 1), ///< ethernet disconnected
} drvEthEvent_t;

typedef struct
{
    int (*on_open)(drvEther_t *ether);
    void (*on_close)(drvEther_t *ether);
    void (*wrap)(drvEther_t *ether, uint8_t *buf, uint32_t len);
    int (*unwrap)(drvEther_t *ether, drvEthReq_t *req, uint8_t *buf, uint32_t size);
    bool (*ready)(drvEther_t *ether);
} drvEthOps_t;

typedef void (*drvEthEventCB_t)(uint32_t evt, void *priv);
typedef void (*drvEthULDataCB_t)(drvEthPacket_t *pkt, uint32_t len, void *priv);

typedef struct
{
    unsigned long rx_packets;
    unsigned long tx_packets;
    unsigned long rx_bytes;
    unsigned long tx_bytes;
    unsigned long rx_errors;
    unsigned long tx_errors;
    unsigned long rx_dropped;
    unsigned long tx_dropped;
    unsigned long multicast;
    unsigned long collisions;
    unsigned long rx_length_errors;
    unsigned long rx_over_errors;
    unsigned long rx_crc_errors;
    unsigned long rx_frame_errors;
    unsigned long rx_fifo_errors;
    unsigned long rx_missed_errors;
    unsigned long tx_aborted_errors;
    unsigned long tx_carrier_errors;
    unsigned long tx_fifo_errors;
    unsigned long tx_heartbeat_errors;
    unsigned long tx_window_errors;
    unsigned long rx_compressed;
    unsigned long tx_compressed;
} drvEthStats_t;

typedef struct
{
    bool (*open)(drvEther_t *ether);
    void (*close)(drvEther_t *ether);
    drvEthReq_t *(*tx_req_alloc)(drvEther_t *ether);
    void (*tx_req_free)(drvEther_t *ether, drvEthReq_t *req);
    bool (*tx_req_submit)(drvEther_t *ether, drvEthReq_t *req, size_t size);
    void (*set_event_cb)(drvEther_t *ether, drvEthEventCB_t cb, void *priv);
    void (*set_uldata_cb)(drvEther_t *ether, drvEthULDataCB_t cb, void *priv);
    bool (*enable)(drvEther_t *ether);
    void (*disable)(drvEther_t *ether);
    void (*destroy)(drvEther_t *ether);
} drvEthImpl_t;

struct drv_ether
{
    drvEthImpl_t impl;
    void *impl_ctx;
    const uint8_t *host_mac;
    const uint8_t *dev_mac;
};

/**
 * \brief open the ether net device, called by netlink
 *
 * \param eth   the net device
 * \return
 *      - true on succeed else fail
 */
static inline bool drvEtherOpen(drvEther_t *eth)
{
    return (eth ? eth->impl.open(eth) : false);
}

/**
 * \brief close the net device, called by netlink
 *
 * \param eth   the net device
 */
static inline void drvEtherClose(drvEther_t *eth)
{
    if (eth)
        eth->impl.close(eth);
}

/**
 * \brief allocate a tx request from net device, called by netlink
 * \note besure call `drvEtherTxReqFree` if the request not use or submit fail
 *
 * \param eth   the net device
 * \return
 *      - (non-null) the tx request else fail
 */
static inline drvEthReq_t *drvEtherTxReqAlloc(drvEther_t *eth)
{
    return (eth ? eth->impl.tx_req_alloc(eth) : NULL);
}

/**
 * \brief release a tx transfer request, called by netlink
 * \note do not call me if a request had been submitted
 *
 * \param eth   the net device
 * \param req   the transfer request
 */
static inline void drvEtherTxReqFree(drvEther_t *eth, drvEthReq_t *req)
{
    if (eth && req)
        eth->impl.tx_req_free(eth, req);
}

/**
 * \brief submit a tx transfer request, called by netlink
 *
 * \param eth   the net device
 * \param req   the request
 * \param size  data size
 * \return
 *      - true on succeed else fail
 */
static inline bool drvEtherTxReqSubmit(drvEther_t *eth, drvEthReq_t *req, size_t size)
{
    return ((eth && req) ? eth->impl.tx_req_submit(eth, req, size) : false);
}

/**
 * \brief register event callback to be notified net device state change
 *        like connect/disconnected, called by netlink
 *
 * \param eth   the net device
 * \param cb    the callback
 * \param priv  caller private context
 */
static inline void drvEtherSetEventCB(drvEther_t *eth, drvEthEventCB_t cb, void *priv)
{
    if (eth)
        eth->impl.set_event_cb(eth, cb, priv);
}

/**
 * \brief register data upload callback, the callbcak will be call every time
 *        data coming from net device, called by netlink
 *
 * \param eth   the net device
 * \param cb    the callback
 * \param priv  caller private context
 */
static inline void drvEtherSetULDataCB(drvEther_t *eth, drvEthULDataCB_t cb, void *priv)
{
    if (eth)
        eth->impl.set_uldata_cb(eth, cb, priv);
}

/**
 * \brief set net device enable(ready to work), called by net device implementation
 *
 * \param eth   the net device
 * \return
 *      - true on succeed else fail
 */
static inline bool drvEtherEnable(drvEther_t *eth)
{
    return (eth ? eth->impl.enable(eth) : false);
}

/**
 * \brief set net device disabled, called by net device implementation
 *
 * \param eth   the net device
 */
static inline void drvEtherDisable(drvEther_t *eth)
{
    if (eth)
        eth->impl.disable(eth);
}

/**
 * \brief destroy the net device, better called bt net devuce implementation
 *
 * \param eth   the net device
 */
static inline void drvEtherDestroy(drvEther_t *eth)
{
    if (eth)
        eth->impl.destroy(eth);
}

/**
 * \brief get net device status
 *
 * \param eth   the net device
 * \return
 *      - (non-null) the status pointer else fail
 */
const drvEthStats_t *drvEtherStatus(drvEther_t *eth);

OSI_EXTERN_C_END

#endif
