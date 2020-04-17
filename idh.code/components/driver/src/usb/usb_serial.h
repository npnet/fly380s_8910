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

#ifndef _USB_SERIAL_H_
#define _USB_SERIAL_H_

#include <usb/usb_device.h>
#include <usb/usb_composite_device.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief struct of usb serial
 */
typedef struct
{
    usb_endpoint_descriptor_t *epin_desc;  ///< usb in endpoint descriptor
    usb_endpoint_descriptor_t *epout_desc; ///< usb out endpoint descriptor
    copsFunc_t *func;                      ///< the composite function
    unsigned long priv;                    ///< private data for usb serial;
} usbSerial_t;

/**
 * @brief bind a usb serial
 *
 * @param name      the serial name
 * @param serial    the usb serial
 * @return
 *      - (-1)  fail
 *      - 0     success
 */
int usbSerialBind(uint32_t name, usbSerial_t *serial);

/**
 * @brief unbind a usb serial
 *
 * @param serial    the usb serial
 */
void usbSerialUnbind(usbSerial_t *serial);

/**
 * @brief enable a usb serial
 *
 * @param serial    the usb serial
 * @return
 *      - (-1)  fail
 *      - 0     success
 */
int usbSerialEnable(usbSerial_t *serial);

/**
 * @brief disable a usb serial
 *
 * @param serial    the usb serial
 */
void usbSerialDisable(usbSerial_t *serial);

/**
 * @brief serial open by host
 *
 * @param serial    the usb serial
 * @return
 *      - true on success else false
 */
bool usbSerialOpen(usbSerial_t *serial);

/**
 * @brief serial close by host
 *
 * @param serial    the usb serial
 */
void usbSerialClose(usbSerial_t *serial);

#ifdef __cplusplus
}
#endif

#endif
