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

#ifndef _USB_ETHERNET_H_
#define _USB_ETHERNET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <osi_compiler.h>
#include <usb/usb_device.h>
#include <usb/usb_composite_device.h>
#include <sys/queue.h>
#include <osi_api.h>

#include "drv_ether.h"
#include "drv_if_ether.h"

typedef struct
{
    drvEthOps_t ops;
    udc_t *udc;
    usbEp_t *tx_ep;
    usbEp_t *rx_ep;
    uint32_t header_len;
    uint8_t host_mac[ETH_ALEN];
    uint8_t dev_mac[ETH_ALEN];
    void *priv;
} usbEthCfg_t;

drvEther_t *usbEthCreate(const usbEthCfg_t *cfg);

#ifdef __cplusplus
}
#endif

#endif
