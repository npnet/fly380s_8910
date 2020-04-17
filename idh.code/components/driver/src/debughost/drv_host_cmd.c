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

#include "drv_config.h"
#include "drv_debughost.h"
#include "drv_host_cmd.h"
#include "osi_api.h"
#include "osi_trace.h"
#include "osi_log.h"
#include "osi_mem.h"
#include "osi_sysnv.h"
#include "osi_byte_buf.h"
#include "hal_hw_map.h"
#include "hal_adi_bus.h"
#include <string.h>
#include <stdlib.h>

#define PACKET_TIMEOUT (100)

struct drvHostCmdEngine
{
    uint8_t packet[CONFIG_HOST_CMD_ENGINE_MAX_PACKET_SIZE];
    unsigned pos;
    osiElapsedTimer_t elapsed;
    drvHostCmdSender_t sender;
    void *sender_ctx;
    drvHostCmdHandler_t handlers[256];
};

static drvHostCmdEngine_t gDebughostCmdEngine;
static drvHostCmdEngine_t gUartBlueScreenCmdEngine;
static drvHostCmdEngine_t gUsbCmdEngine;

drvHostCmdEngine_t *drvHostCmdUartBlueScreenEngine(void) { return &gUartBlueScreenCmdEngine; }
drvHostCmdEngine_t *drvHostCmdDebughostEngine(void) { return &gDebughostCmdEngine; }
drvHostCmdEngine_t *drvHostCmdUsbEngine(void) { return &gUsbCmdEngine; }

static void prvHostCmdSendNone(void *ctx, uint8_t *packet, unsigned packet_len)
{
}

static uint8_t prvHostCrc(const uint8_t *p, unsigned size)
{
    uint8_t crc = 0;
    for (int n = 0; n < size; n++)
        crc ^= p[n];
    return crc;
}

void drvHostCmdEngineInit(drvHostCmdEngine_t *d)
{
    d->sender = prvHostCmdSendNone;
    d->sender_ctx = NULL;
    d->pos = 0;
    osiElapsedTimerStart(&d->elapsed);
    for (int n = 0; n < 256; n++)
        d->handlers[n] = drvHostInvalidCmd;
}

void drvHostCmdSetSender(drvHostCmdEngine_t *d, drvHostCmdSender_t sender, void *sender_ctx)
{
    if (d == NULL)
        return;

    if (sender == NULL)
    {
        d->sender = prvHostCmdSendNone;
        d->sender_ctx = NULL;
    }
    else
    {
        d->sender = sender;
        d->sender_ctx = sender_ctx;
    }
}

void drvHostCmdRegisterHander(drvHostCmdEngine_t *d, uint8_t flow_id, drvHostCmdHandler_t handler)
{
    if (d == NULL)
        return;

    if (handler == NULL)
        d->handlers[flow_id] = drvHostInvalidCmd;
    else
        d->handlers[flow_id] = handler;
}

void drvHostCmdSendResponse(drvHostCmdEngine_t *d, uint8_t *packet, unsigned packet_len)
{
    if (d == NULL || packet == NULL || packet_len == 0)
        return;

    osiBytesPutBe16(packet + 1, packet_len - 4);
    packet[packet_len - 1] = prvHostCrc(&packet[3], packet_len - 4);
    d->sender(d->sender_ctx, packet, packet_len);
}

void drvHostCmdSendResultCode(drvHostCmdEngine_t *d, uint8_t *packet, uint16_t error)
{
    if (d == NULL || packet == NULL)
        return;

    osiBytesPutLe16(packet + 5, error);
    drvHostCmdSendResponse(d, packet, 5 + 2 + 1);
}

void drvHostCmdPushData(drvHostCmdEngine_t *d, const void *data, unsigned size)
{
    const uint8_t *data8 = (const uint8_t *)data;

    OSI_LOGD(0, "HOST push data size/%d pos/%d", size, d->pos);

    if (d->pos > 0 && osiElapsedTime(&d->elapsed) > PACKET_TIMEOUT)
    {
        OSI_LOGD(0, "HOST packet timeout pos/%d elapsed/%d", d->pos, osiElapsedTime(&d->elapsed));
        d->pos = 0;
    }

    uint16_t frame_len = (d->pos >= 4) ? osiBytesGetBe16(&d->packet[1]) : 0;
    for (unsigned n = 0; n < size; n++)
    {
        uint8_t c = *data8++;
        if (d->pos == 0)
        {
            if (c != 0xAD)
                continue;
            d->packet[d->pos++] = c;
            osiElapsedTimerStart(&d->elapsed);
        }
        else if (d->pos == 1 || d->pos == 2)
        {
            d->packet[d->pos++] = c;
        }
        else if (d->pos == 3)
        {
            d->packet[d->pos++] = c;
            frame_len = osiBytesGetBe16(&d->packet[1]);
            if (frame_len > 0x2020 - 4)
                d->pos = 0;
        }
        else if (d->pos < frame_len + 4 - 1)
        {
            d->packet[d->pos++] = c;
        }
        else
        {
            d->packet[d->pos++] = c;
            uint8_t flowid = d->packet[3];

            OSI_LOGD(0, "HOST packet flowid/0x%02x framelen/%d", flowid, frame_len);
            if (c == prvHostCrc(&d->packet[3], frame_len))
            {
                drvHostCmdHandler_t handler = d->handlers[flowid];
                handler(d, d->packet, d->pos);
            }
            else
            {
                OSI_LOGD(0, "HOST packet CRC mismatch");
            }
            d->pos = 0;
        }
    }
}

void drvHostInvalidCmd(drvHostCmdEngine_t *cmd, uint8_t *packet, unsigned packet_len)
{
    osiHostPacketHeader_t *header = (osiHostPacketHeader_t *)packet;
    osiFillHostHeader(header, 0xfd, 2);
    packet[4] = HOST_SYSCMD_INVALID;
    drvHostCmdSendResponse(cmd, packet, 6);
}
