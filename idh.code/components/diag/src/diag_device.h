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

#ifndef _DIAG_DEVICE_H_
#define _DIAG_DEVICE_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct diag_device;
typedef struct diag_device diagDevice_t;

typedef void (*dataNotifyCB_t)(void *param);

typedef struct
{
    int (*send)(diagDevice_t *d, const void *data, unsigned size, unsigned timeout);
    int (*recv)(diagDevice_t *d, void *buffer, unsigned size);
    void (*destroy)(diagDevice_t *d);
    bool (*wait_tx_finish)(diagDevice_t *d, unsigned timeout);
} diagDevOps_t;

struct diag_device
{
    diagDevOps_t ops;
    dataNotifyCB_t notify_cb;
    void *param;
};

typedef struct
{
    uint32_t rx_buf_size;
    uint32_t tx_buf_size;
    dataNotifyCB_t cb;
    void *param;
} diagDevCfg_t;

static inline int diagDeviceSend(diagDevice_t *d, const void *data, unsigned size, unsigned timeout)
{
    if (data == NULL || size == 0)
        return 0;
    if (d && d->ops.send)
        return d->ops.send(d, data, size, timeout);
    return -1;
}

static inline int diagDeviceRecv(diagDevice_t *d, void *buffer, unsigned size)
{
    if (buffer == NULL || size == 0)
        return 0;
    if (d && d->ops.recv)
        return d->ops.recv(d, buffer, size);
    return -1;
}

static inline void diagDeviceDestroy(diagDevice_t *d)
{
    if (d && d->ops.destroy)
        d->ops.destroy(d);
}

static inline bool diagDeviceWaitTxFinished(diagDevice_t *d, unsigned timeout)
{
    if (d && d->ops.wait_tx_finish)
        return d->ops.wait_tx_finish(d, timeout);
    return false;
}

diagDevice_t *diagDeviceUartCreate(const diagDevCfg_t *cfg);
diagDevice_t *diagDeviceUserialCreate(const diagDevCfg_t *cfg);

#ifdef __cplusplus
}
#endif

#endif
