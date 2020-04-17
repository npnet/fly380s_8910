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

#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "diag_config.h"
#include "diag.h"
#include "diag_device.h"
#include "diag_internal.h"
#include "osi_api.h"
#include "osi_log.h"
#include "osi_sysnv.h"
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
#include "cmddef.h"
#include <stdlib.h>
#include <string.h>

#define DIAG_THREAD_PRIORITY OSI_PRIORITY_BELOW_NORMAL
#define DIAG_THREAD_STACK_SIZE (4096)

#ifdef CONFIG_DIAG_DEVICE_USRL_SUPPORT
#define DIAG_RX_BUF_SIZE (64 * 1024)
#define DIAG_TX_BUF_SIZE (16 * 1024)
#else
#define DIAG_RX_BUF_SIZE (8 * 1024)
#define DIAG_TX_BUF_SIZE (8 * 1024)
#endif

#define DIAG_PIECE_BUF_SIZE (256)
#define DIAG_PACKET_SIZE (1024)

#define FLAG_BYTE (0x7e)
#define ESCAPE_BYTE (0x7d)
#define COMPLEMENT_BYTE (0x20)

typedef enum diagParseState
{
    DIAG_ST_SEEK_START,
    DIAG_ST_DATA,
    DIAG_ST_ESCAPED,
    DIAG_ST_SEEK_END
} diagParseState_t;

typedef struct diag_handle
{
    diagCmdHandle_t cb;
    void *ctx;
} diagHandlerCtx_t;

typedef struct diagContext
{
    diagHandlerCtx_t handlers[REQ_MAX_F];
    osiWork_t *rx_work;
    osiWorkQueue_t *wq;
    diagDevice_t *dev;
    diagDevType_t dev_type;

    diagParseState_t state;
    diagMsgHead_t head;
    diagMsgHead_t *cmd;
    unsigned packet_size;
} diagContext_t;

static diagContext_t *gDiagCtx;

bool diagRegisterCmdHandle(uint8_t type, diagCmdHandle_t handler, void *ctx)
{
    diagContext_t *d = gDiagCtx;
    if (type >= REQ_MAX_F)
        return false;

    d->handlers[type].cb = handler;
    d->handlers[type].ctx = ctx;
    return true;
}

bool diagUnregisterCmdHandle(uint8_t type)
{
    diagContext_t *d = gDiagCtx;
    if (type >= REQ_MAX_F)
        return false;

    d->handlers[type].cb = NULL;
    d->handlers[type].ctx = NULL;
    return true;
}

void diagBadCommand(const diagMsgHead_t *cmd)
{
    diagMsgHead_t head = *cmd;
    head.len = sizeof(diagMsgHead_t);
    diagOutputPacket(&head, sizeof(head));
}

static uint8_t *_diagSend(uint8_t *p, uint8_t *pstart, uint8_t *pend, const void *data, unsigned size)
{
    diagContext_t *d = gDiagCtx;
    const uint8_t *in = (const uint8_t *)data;
    while (size > 0)
    {
        uint8_t ch = *in++;
        --size;
        if (ch == FLAG_BYTE || ch == ESCAPE_BYTE)
        {
            *p++ = ESCAPE_BYTE;
            *p++ = ch ^ COMPLEMENT_BYTE;
        }
        else
        {
            *p++ = ch;
        }

        if (p >= pend)
        {
            diagDeviceSend(d->dev, pstart, p - pstart, OSI_WAIT_FOREVER);
            p = pstart;
        }
    }
    return p;
}

bool diagOutputPacket(const void *data, unsigned size)
{
    if (data == NULL || size == 0)
        return false;

    diagContext_t *d = gDiagCtx;

    uint8_t buf[DIAG_PIECE_BUF_SIZE + 1];
    uint8_t *p = buf;
    uint8_t *pstart = p;
    uint8_t *pend = p + DIAG_PIECE_BUF_SIZE;

    *p++ = FLAG_BYTE;
    p = _diagSend(p, pstart, pend, data, size);
    *p++ = FLAG_BYTE;
    diagDeviceSend(d->dev, pstart, p - pstart, OSI_WAIT_FOREVER);
    return true;
}

bool diagOutputPacket2(const diagMsgHead_t *cmd, const void *data, unsigned size)
{
    if (cmd == NULL)
        return false;

    diagContext_t *d = gDiagCtx;

    uint8_t buf[DIAG_PIECE_BUF_SIZE + 1];
    uint8_t *p = buf;
    uint8_t *pstart = p;
    uint8_t *pend = p + DIAG_PIECE_BUF_SIZE;

    diagMsgHead_t head = *cmd;
    head.len = sizeof(diagMsgHead_t) + size;

    *p++ = FLAG_BYTE;
    p = _diagSend(p, pstart, pend, &head, sizeof(diagMsgHead_t));
    p = _diagSend(p, pstart, pend, data, size);
    *p++ = FLAG_BYTE;
    diagDeviceSend(d->dev, pstart, p - pstart, OSI_WAIT_FOREVER);
    return true;
}

bool diagOutputPacket3(const diagMsgHead_t *cmd, const void *sub_header, unsigned sub_header_size,
                       const void *data, unsigned size)
{
    if (cmd == NULL)
        return false;

    diagContext_t *d = gDiagCtx;
    uint8_t buf[DIAG_PIECE_BUF_SIZE + 1];
    uint8_t *p = buf;
    uint8_t *pstart = p;
    uint8_t *pend = p + DIAG_PIECE_BUF_SIZE;

    diagMsgHead_t head = *cmd;
    head.len = sizeof(diagMsgHead_t) + sub_header_size + size;

    *p++ = FLAG_BYTE;
    p = _diagSend(p, pstart, pend, &head, sizeof(diagMsgHead_t));
    p = _diagSend(p, pstart, pend, sub_header, sub_header_size);
    p = _diagSend(p, pstart, pend, data, size);
    *p++ = FLAG_BYTE;
    diagDeviceSend(d->dev, pstart, p - pstart, OSI_WAIT_FOREVER);
    return true;
}

static void _responseSendOk(const diagMsgHead_t *cmd)
{
    diagMsgHead_t head = {
        .seq_num = cmd->seq_num,
        .len = sizeof(diagMsgHead_t),
        .type = MSG_SEND_OK,
        .subtype = 0,
    };
    diagOutputPacket(&head, sizeof(head));
}

static void _diagHandle(diagContext_t *d)
{
    uint8_t type = d->cmd->type;
    if (type < REQ_MAX_F)
    {
        diagHandlerCtx_t *hc = &d->handlers[type];
        OSI_LOGD(0, "diag cmd %d/%p", type, hc->cb);
        if (hc->cb != NULL)
        {
            if (hc->cb(d->cmd, hc->ctx))
                return;
        }
        diagBadCommand(d->cmd);
    }
    else
    {
        _responseSendOk(d->cmd);
    }
}

static void _diagProcess(void *param)
{
    diagContext_t *d = (diagContext_t *)param;
    uint8_t buf[DIAG_PACKET_SIZE];
    for (;;)
    {
        int size = diagDeviceRecv(d->dev, buf, DIAG_PACKET_SIZE);
        if (size <= 0)
            break;

        OSI_LOGD(0, "DIAG: read len/%d", size);
        for (int n = 0; n < size; n++)
        {
            uint8_t ch = buf[n];
            if (d->state == DIAG_ST_SEEK_START)
            {
                if (ch != FLAG_BYTE)
                    continue;

                OSI_LOGD(0, "DIAG: start found");
                d->state = DIAG_ST_DATA;
                d->packet_size = 0;
            }
            else if (d->state == DIAG_ST_SEEK_END)
            {
                if (ch == FLAG_BYTE)
                {
                    OSI_LOGD(0, "DIAG: end found");
                    _diagHandle(d);
                    d->state = DIAG_ST_DATA;
                    d->packet_size = 0;
                }
                else
                {
                    OSI_LOGD(0, "DIAG: end not found");
                    d->state = DIAG_ST_SEEK_START;
                }

                free(d->cmd);
                d->cmd = NULL;
            }
            else
            {
                if (ch == FLAG_BYTE)
                {
                    OSI_LOGD(0, "DIAG: unexpected flag len/%d", d->packet_size);
                    free(d->cmd);
                    d->cmd = NULL;
                    d->state = DIAG_ST_DATA;
                    d->packet_size = 0;
                    continue;
                }

                if (d->state == DIAG_ST_DATA && ch == ESCAPE_BYTE)
                {
                    d->state = DIAG_ST_ESCAPED;
                    continue;
                }

                if (d->state == DIAG_ST_ESCAPED)
                {
                    ch ^= COMPLEMENT_BYTE;
                    d->state = DIAG_ST_DATA;
                }

                if (d->packet_size < sizeof(diagMsgHead_t))
                {
                    uint8_t *p = (uint8_t *)&d->head;
                    p[d->packet_size++] = ch;

                    if (d->packet_size == sizeof(diagMsgHead_t))
                    {
                        OSI_LOGD(0, "DIAG: msg head %d/%d/%d/%d",
                                 d->head.seq_num, d->head.len,
                                 d->head.type, d->head.subtype);
                        if (d->head.len < sizeof(diagMsgHead_t))
                        {
                            d->state = DIAG_ST_SEEK_START;
                            continue;
                        }
                        d->cmd = (diagMsgHead_t *)malloc(d->head.len);
                        *(d->cmd) = d->head;

                        if (d->packet_size == d->head.len)
                            d->state = DIAG_ST_SEEK_END;
                    }
                }
                else
                {
                    uint8_t *p = (uint8_t *)d->cmd;
                    p[d->packet_size++] = ch;

                    if (d->packet_size == d->head.len)
                        d->state = DIAG_ST_SEEK_END;
                }
            }
        }
    }
}

static void _diagDataCB(void *param)
{
    diagContext_t *d = (diagContext_t *)param;
    osiWorkEnqueue(d->rx_work, d->wq);
}

diagDevType_t diagDeviceType(void)
{
    diagContext_t *d = gDiagCtx;
    return d->dev_type;
}

bool diagWaitWriteFinish(unsigned timeout)
{
    diagContext_t *d = gDiagCtx;
    return diagDeviceWaitTxFinished(d->dev, timeout);
}

void diagInit(void)
{
    gDiagCtx = (diagContext_t *)calloc(1, sizeof(diagContext_t));
    diagContext_t *d = (diagContext_t *)gDiagCtx;

#ifdef CONFIG_SOC_6760
    // Layer1_RegCaliDiagHandle();
    diagCurrentTestInit();
    diagSwverInit();
    diagLogInit();
    diagNvInit();
    // diagNvmInit();
    // diagSysInit();
#else
    Layer1_RegCaliDiagHandle();
    diagCurrentTestInit();
    diagSwverInit();
    diagLogInit();
    diagNvmInit();
    diagSysInit();
#endif
    d->wq = osiWorkQueueCreate("diag", 1, DIAG_THREAD_PRIORITY, DIAG_THREAD_STACK_SIZE);
    d->rx_work = osiWorkCreate(_diagProcess, NULL, d);
    d->state = DIAG_ST_SEEK_START;

    diagDevCfg_t cfg = {
        .rx_buf_size = DIAG_RX_BUF_SIZE,
        .tx_buf_size = DIAG_TX_BUF_SIZE,
        .cb = _diagDataCB,
        .param = (void *)d,
    };

    if (gSysnvDiagDevice == DIAG_DEVICE_UART)
    {
        d->dev = diagDeviceUartCreate(&cfg);
        d->dev_type = DIAG_DEVICE_UART;
    }
#ifdef CONFIG_DIAG_DEVICE_USRL_SUPPORT
    else
    {
        d->dev = diagDeviceUserialCreate(&cfg);
        d->dev_type = DIAG_DEVICE_USERIAL;
    }
#endif
}
