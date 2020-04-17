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

// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_VERBOSE

#include "osi_log.h"
#include "at_engine.h"
#include "calclib/cmux_fcs.h"
#include <stdlib.h>
#include <string.h>

#define BASIC_FLAG 0xf9

#define ADV_FLAG 0x7e
#define ADV_ESCAPE_CTRL 0x7d
#define ADV_ESCAPE_MASK 0x20

#define CTRL_PF 0x10
#define FRAME_TYPE_SABM 0x2f
#define FRAME_TYPE_UA 0x63
#define FRAME_TYPE_DM 0x0f
#define FRAME_TYPE_DISC 0x43
#define FRAME_TYPE_UIH 0xef
#define FRAME_TYPE_UI 0x03

#define CTRL_MSG_PN 0x20
#define CTRL_MSG_PSC 0x10
#define CTRL_MSG_CLD 0x30
#define CTRL_MSG_TEST 0x08
#define CTRL_MSG_FCON 0x28
#define CTRL_MSG_FCOFF 0x18
#define CTRL_MSG_MSC 0x38
#define CTRL_MSG_NSC 0x04
#define CTRL_MSG_RPN 0x24
#define CTRL_MSG_RLS 0x14
#define CTRL_MSG_SNC 0x34

#define MSC_FC (1 << 1)
#define MSC_DSR_DTR (1 << 2)
#define MSC_CTS_RFR (1 << 3)
#define MSC_RI (1 << 6)
#define MSC_DCD (1 << 7)

// FOR UE
#define CR_COMMAND 0
#define CR_RESPONSE 1

#define CTRL_COMMAND 1
#define CTRL_RESPONSE 0

#define MAX_DLC_NUM gAtCmuxDlcNum
#define CTRL_DLCI 0

#define ADV_MIN_FRAME_SIZE 5   // FLAG, ADDRESS, CONTROL, INFO(0), FCS, FLAG
#define BASIC_MIN_FRAME_SIZE 6 // FLAG, ADDRESS, CONTROL, LENGTH(1), INFO(0), FCS, FLAG
#define MAX_TEST_SIZE 32       // implementation limitation

#define LENGTH_ONE_MAX 0x7f
#define LENGTH_TWO_MAX 0x7fff

#define SET_BITS(v, s, l) (((v) & ((1 << (l)) - 1)) << (s))
#define SET_BITS26(v) SET_BITS(v, 2, 6) // [7:2]
#define SET_BITS17(v) SET_BITS(v, 1, 7) // [7:1]
#define SET_CR(v) SET_BITS(v, 1, 1)
#define SET_EA SET_BITS(1, 0, 1)

#define GET_BITS(v, s, l) (((v) >> (s)) & ((1 << (l)) - 1))
#define GET_BITS26(v) GET_BITS(v, 2, 6) // [7:2]
#define GET_BITS17(v) GET_BITS(v, 1, 7) // [7:1]
#define GET_BIT1(v) GET_BITS(v, 1, 1)
#define GET_EA(v) GET_BITS(v, 0, 1)

typedef enum
{
    AT_CMUX_BASIC_OPTION,
    AT_CMUX_ADV_OPTION
} atCmuxOption_t;

typedef enum
{
    AT_CMUX_CHANNEL_DISCONNECT,
    AT_CMUX_CHANNEL_CONNECT,
    AT_CMUX_CHANNEL_FLOW_STOPPED
} atCmuxChannelState_t;

typedef struct
{
    uint8_t frameType;
    uint8_t convergence;
    uint8_t priority;
    uint8_t ack_timer;
    uint16_t frameSize;
    uint8_t maxRetrans;
    uint8_t window_size;
} atCmuxParam_t;

typedef struct
{
    atDispatch_t *dispatch;
    atCmuxChannelState_t state;
    atCmuxParam_t param;
} atCmuxChannel_t;

struct atCmuxEngine
{
    atDispatch_t *dispatch; // upstream dispatch
    atCmuxOption_t mode;
    int channel_id;

    atCmuxConfig_t cfg;
    int max_frame_size;
    uint8_t *buff;
    int buff_len;
    int buff_size;
    bool escaped;

    uint8_t *outBuff;
    int outLen;
    bool outOverflow;
    atCmuxChannel_t *channels;
};

typedef struct
{
    uint8_t dlci;
    uint8_t ctrl;
    int infoLen;
    uint8_t *info;
} atCmuxFrame_t;

// =============================================================================
// Tiny helper to push a byte to buffer
// =============================================================================
static void _pushByte(atCmuxEngine_t *th, uint8_t c)
{
    // buffer overflow should be checked before call this.
    th->buff[th->buff_len++] = c;
}

// =============================================================================
// Check valid DLCI
// =============================================================================
static bool _isDlciValid(atCmuxEngine_t *th, uint8_t dlci)
{
    // 27010 Clause 5.6
    return ((th->mode == AT_CMUX_BASIC_OPTION && dlci <= 61) ||
            (th->mode == AT_CMUX_ADV_OPTION && dlci <= 62));
}

// =============================================================================
// Default priority of DLC
// =============================================================================
static uint8_t _defaultPriority(uint8_t dlci)
{
    // 27010 Clause 5.6
    return dlci == 0 ? 0 : dlci > 55 ? 61 : (dlci | 7);
}

// =============================================================================
// Start output, put the FLAG to output buffer
// =============================================================================
static void _outputStart(atCmuxEngine_t *th)
{
    th->outBuff[0] = (th->mode == AT_CMUX_BASIC_OPTION) ? BASIC_FLAG : ADV_FLAG;
    th->outLen = 1;
    th->outOverflow = false;
}

// =============================================================================
// Finish output, put the FLAG to output buffer and send.
// On output overflow, output buffer is dropped silently.
// =============================================================================
static void _outputEnd(atCmuxEngine_t *th)
{
    if (!th->outOverflow)
    {
        th->outBuff[th->outLen++] = (th->mode == AT_CMUX_BASIC_OPTION) ? BASIC_FLAG : ADV_FLAG;
        if (th->dispatch != NULL)
            atDispatchWrite(th->dispatch, th->outBuff, th->outLen);
    }
    th->outLen = 0;
}

// =============================================================================
// Push data to output buffer, and escape in advanced option is handled.
// Return false at output buffer overflow.
// =============================================================================
static bool _outputPush(atCmuxEngine_t *th, const uint8_t *data, unsigned length)
{
    if (th->outOverflow)
        return false;

    if (th->mode == AT_CMUX_BASIC_OPTION)
    {
        if (th->buff_len + length >= gAtCmuxOutBuffSize - 1)
        {
            th->outOverflow = true;
            return false;
        }
        memcpy(th->outBuff + th->outLen, data, length);
        th->outLen += length;
    }
    else
    {
        while (length--)
        {
            uint8_t c = *data++;
            if (c == ADV_ESCAPE_CTRL || c == ADV_FLAG)
            {
                th->outBuff[th->outLen++] = ADV_ESCAPE_CTRL;
                th->outBuff[th->outLen++] = c ^ ADV_ESCAPE_MASK;
            }
            else
            {
                th->outBuff[th->outLen++] = c;
            }
        }
    }
    return true;
}

// =============================================================================
// Send DM frame
// =============================================================================
static void _sendDM(atCmuxEngine_t *th, uint8_t dlci)
{
    OSI_LOGI(0x1000009f, "CMUX send DM, dlci/%d", dlci);

    _outputStart(th);
    if (th->mode == AT_CMUX_BASIC_OPTION)
    {
        uint8_t data[4];
        data[0] = SET_BITS26(dlci) | SET_CR(CR_RESPONSE) | SET_EA;
        data[1] = FRAME_TYPE_DM | CTRL_PF;
        data[2] = SET_BITS17(0) | SET_EA;
        data[3] = cmuxFcsCalc(&data[0], 3);
        _outputPush(th, data, 4);
    }
    else
    {
        uint8_t data[3];
        data[0] = SET_BITS26(dlci) | SET_CR(CR_RESPONSE) | SET_EA;
        data[1] = FRAME_TYPE_DM | CTRL_PF;
        data[2] = cmuxFcsCalc(&data[0], 2);
        _outputPush(th, data, 3);
    }
    _outputEnd(th);
}

// =============================================================================
// Send UA frame
// =============================================================================
static void _sendUA(atCmuxEngine_t *th, uint8_t dlci)
{
    OSI_LOGI(0x100000a0, "CMUX send UA, dlci/%d", dlci);

    _outputStart(th);
    if (th->mode == AT_CMUX_BASIC_OPTION)
    {
        uint8_t data[4];
        data[0] = SET_BITS26(dlci) | SET_CR(CR_RESPONSE) | SET_EA;
        data[1] = FRAME_TYPE_UA | CTRL_PF;
        data[2] = SET_BITS17(0) | SET_EA;
        data[3] = cmuxFcsCalc(&data[0], 3);
        _outputPush(th, data, 4);
    }
    else
    {
        uint8_t data[3];
        data[0] = SET_BITS26(dlci) | SET_CR(CR_RESPONSE) | SET_EA;
        data[1] = FRAME_TYPE_UA | CTRL_PF;
        data[2] = cmuxFcsCalc(&data[0], 2);
        _outputPush(th, data, 3);
    }
    _outputEnd(th);
}

// =============================================================================
// Send UIH frame
// =============================================================================
static void _sendUIH(atCmuxEngine_t *th, const uint8_t *indata, unsigned length, uint8_t dlci)
{
    OSI_LOGD(0x100000a1, "CMUX send UIH, dlci/%d, length/%d", dlci, length);

    while (length > 0)
    {
        unsigned trans = OSI_MIN(unsigned, length, th->max_frame_size);
        _outputStart(th);
        if (th->mode == AT_CMUX_BASIC_OPTION)
        {
            uint8_t data[5];
            uint8_t fcs;

            data[0] = SET_BITS26(dlci) | SET_CR(CR_COMMAND) | SET_EA;
            data[1] = FRAME_TYPE_UIH;
            if (trans <= LENGTH_ONE_MAX)
            {
                data[2] = SET_BITS17(trans) | SET_EA;
                fcs = cmuxFcsCalc(data, 3);
                _outputPush(th, data, 3);
            }
            else
            {
                data[2] = SET_BITS17(trans);
                data[3] = (trans >> 7);
                fcs = cmuxFcsCalc(data, 4);
                _outputPush(th, data, 4);
            }

            _outputPush(th, indata, trans);
            _outputPush(th, &fcs, 1);
        }
        else
        {
            uint8_t data[2];
            data[0] = SET_BITS26(dlci) | SET_CR(CR_COMMAND) | SET_EA;
            data[1] = FRAME_TYPE_UIH;
            uint8_t fcs = cmuxFcsCalc(data, 2);
            _outputPush(th, data, 2);
            _outputPush(th, indata, trans);
            _outputPush(th, &fcs, 1);
        }
        _outputEnd(th);
        length -= trans;
        indata += trans;
    }
}

// =============================================================================
// Parse PN data to parameter
// =============================================================================
static void _PNDataGet(const uint8_t *data, atCmuxParam_t *param)
{
    param->frameType = GET_BITS(data[1], 0, 4);
    param->convergence = GET_BITS(data[1], 4, 4);
    param->priority = GET_BITS(data[2], 2, 6);
    param->ack_timer = data[3];
    param->frameSize = data[4] | (data[5] << 8);
    param->maxRetrans = data[6];
    param->window_size = GET_BITS(data[7], 5, 3);
}

// =============================================================================
// Construct PN data from parameter
// =============================================================================
static void _PNDataSet(uint8_t *data, atCmuxParam_t *param)
{
    data[1] = SET_BITS(param->frameType, 0, 4) | SET_BITS(param->convergence, 4, 4);
    data[2] = SET_BITS(param->priority, 2, 6);
    data[3] = param->ack_timer;
    data[4] = param->frameSize & 0xff;
    data[5] = (param->frameSize >> 8) & 0xff;
    data[6] = param->maxRetrans;
    data[7] = SET_BITS(param->window_size, 5, 3);
}

// =============================================================================
// Send CTRL msg NSC
// =============================================================================
static void _sendCtrlNSC(atCmuxEngine_t *th, uint8_t cr, uint8_t intype)
{
    OSI_LOGI(0x100000a2, "CMUX send ctrl NSC");
    uint8_t data[3];
    data[0] = SET_BITS26(CTRL_MSG_NSC) | SET_CR(cr) | SET_EA;
    data[1] = SET_BITS17(1) | SET_EA;
    data[2] = intype;
    _sendUIH(th, &data[0], 3, CTRL_DLCI);
}

// =============================================================================
// Send CTRL msg PN
// =============================================================================
static void _sendCtrlPN(atCmuxEngine_t *th, atCmuxParam_t *param, uint8_t cr, uint8_t dlci)
{
    OSI_LOGI(0x100000a3, "CMUX send ctrl PN");
    uint8_t data[10];
    data[0] = SET_BITS26(CTRL_MSG_PN) | SET_CR(cr) | SET_EA;
    data[1] = SET_BITS17(8) | SET_EA;
    _PNDataSet(&data[2], param); // 8 bytes
    data[2] = SET_BITS26(dlci);
    _sendUIH(th, data, 10, CTRL_DLCI);
}

// =============================================================================
// Send CTRL msg PSC
// =============================================================================
static void _sendCtrlPSC(atCmuxEngine_t *th, uint8_t cr)
{
    OSI_LOGI(0x100000a4, "CMUX send ctrl PSC");
    uint8_t data[2];
    data[0] = SET_BITS26(CTRL_MSG_PSC) | SET_CR(cr) | SET_EA;
    data[1] = SET_BITS17(0) | SET_EA;
    _sendUIH(th, data, 2, CTRL_DLCI);
}

// =============================================================================
// Send CTRL msg CLD
// =============================================================================
static void _sendCtrlCLD(atCmuxEngine_t *th, uint8_t cr)
{
    OSI_LOGI(0x100000a5, "CMUX send ctrl CLD");
    uint8_t data[2];
    data[0] = SET_BITS26(CTRL_MSG_CLD) | SET_CR(cr) | SET_EA;
    data[1] = SET_BITS17(0) | SET_EA;
    _sendUIH(th, data, 2, CTRL_DLCI);
}

// =============================================================================
// Send CTRL msg TEST
// =============================================================================
static void _sendCtrlTEST(atCmuxEngine_t *th, uint8_t cr, uint8_t *indata, unsigned length)
{
    if (length > MAX_TEST_SIZE)
    {
        OSI_LOGW(0x100000a6, "CMUX ctrl test info too long, drop it");
        return;
    }
    OSI_LOGI(0x100000a7, "CMUX send ctrl TEST");
    uint8_t data[2 + MAX_TEST_SIZE];
    data[0] = SET_BITS26(CTRL_MSG_TEST) | SET_CR(cr) | SET_EA;
    data[1] = SET_BITS17(length) | SET_EA;
    memcpy(&data[2], indata, length);
    _sendUIH(th, data, 2 + length, CTRL_DLCI);
}

// =============================================================================
// Send CTRL msg FCON
// =============================================================================
static void _sendCtrlFCON(atCmuxEngine_t *th, uint8_t cr)
{
    OSI_LOGI(0x100000a8, "CMUX send ctrl FCON");
    uint8_t data[2];
    data[0] = SET_BITS26(CTRL_MSG_FCON) | SET_CR(cr) | SET_EA;
    data[1] = SET_BITS17(0) | SET_EA;
    _sendUIH(th, data, 2, CTRL_DLCI);
}

// =============================================================================
// Send CTRL msg FCOFF
// =============================================================================
static void _sendCtrlFCOFF(atCmuxEngine_t *th, uint8_t cr)
{
    OSI_LOGI(0x100000a9, "CMUX send ctrl FCOFF");
    uint8_t data[2];
    data[0] = SET_BITS26(CTRL_MSG_FCOFF) | SET_CR(cr) | SET_EA;
    data[1] = SET_BITS17(0) | SET_EA;
    _sendUIH(th, data, 2, CTRL_DLCI);
}

// =============================================================================
// Send CTRL msg MSC
// =============================================================================
static void _sendCtrlMSC(atCmuxEngine_t *th, uint8_t cr, uint8_t dlci, uint8_t v24, uint8_t br)
{
    OSI_LOGI(0x100000aa, "CMUX send ctrl MSC");
    if (br != 0)
    {
        uint8_t data[5];
        data[0] = SET_BITS26(CTRL_MSG_MSC) | SET_CR(cr) | SET_EA;
        data[1] = SET_BITS17(3) | SET_EA;
        data[2] = SET_BITS26(dlci) | SET_CR(1) | SET_EA;
        data[3] = v24 | SET_EA;
        data[4] = br | SET_EA;
        _sendUIH(th, data, 5, CTRL_DLCI);
    }
    else
    {
        uint8_t data[4];
        data[0] = SET_BITS26(CTRL_MSG_MSC) | SET_CR(cr) | SET_EA;
        data[1] = SET_BITS17(2) | SET_EA;
        data[2] = SET_BITS26(dlci) | SET_CR(1) | SET_EA;
        data[3] = v24 | SET_EA;
        _sendUIH(th, data, 4, CTRL_DLCI);
    }
}

// =============================================================================
// Send CTRL msg SNC
// =============================================================================
static void _sendCtrlSNC(atCmuxEngine_t *th, uint8_t cr, uint8_t dlci, uint8_t srv, uint8_t codec)
{
    OSI_LOGI(0x100000ab, "CMUX send ctrl SNC");
    uint8_t data[5];
    data[0] = SET_BITS26(CTRL_MSG_SNC) | SET_CR(cr) | SET_EA;
    data[1] = SET_BITS17(3) | SET_EA;
    data[2] = SET_BITS26(dlci) | SET_CR(1) | SET_EA;
    data[3] = SET_BITS17(srv) | SET_EA;
    data[4] = SET_BITS17(codec) | SET_EA;
    _sendUIH(th, data, 4, CTRL_DLCI);
}

// =============================================================================
// Parse and handle CTRL msg
// =============================================================================
static void _parseCtrl(atCmuxEngine_t *th, atCmuxFrame_t *frame)
{
    if (GET_BIT1(frame->info[0]) != CTRL_COMMAND)
    {
        OSI_LOGW(0x100000ac, "CMUX unexpected ctrl msg response, drop it");
        return;
    }

    uint8_t type = GET_BITS26(frame->info[0]);
    int valLen = GET_BITS17(frame->info[1]);
    uint8_t *val = &frame->info[2];
    if (GET_EA(frame->info[1]) == 0) // 2 bytes length
    {
        valLen |= (frame->info[2] << 7);
        val++;
    }

    switch (type)
    {
    case CTRL_MSG_PN:
    {
        OSI_LOGI(0x100000ad, "CMUX recv parameter negotiation msg");
        if (valLen != 8)
        {
            OSI_LOGW(0x100000ae, "CMUX invalid PN value length %d, drop it", valLen);
            return;
        }

        atCmuxParam_t param;
        _PNDataGet(val, &param);

        uint8_t dlci = GET_BITS26(val[0]);
        atCmuxChannel_t *ch = &th->channels[dlci];

        // update frame size if peer needs smaller
        if (param.frameSize < ch->param.frameSize)
            ch->param.frameSize = param.frameSize;
        _sendCtrlPN(th, &ch->param, CTRL_RESPONSE, dlci);
        break;
    }
    case CTRL_MSG_PSC:
    {
        OSI_LOGI(0x100000af, "CMUX recv power saving msg");
        _sendCtrlPSC(th, CTRL_RESPONSE);
        break;
    }
    case CTRL_MSG_CLD:
    {
        OSI_LOGI(0x100000b0, "CMUX recv close down msg");
        _sendCtrlCLD(th, CTRL_RESPONSE);
        atEngineSchedule((osiCallback_t)atDispatchEndCmuxMode, th->dispatch);
        break;
    }
    case CTRL_MSG_TEST:
    {
        OSI_LOGI(0x100000b1, "CMUX recv test msg");
        _sendCtrlTEST(th, CTRL_RESPONSE, val, valLen);
        break;
    }
    case CTRL_MSG_FCON:
    {
        OSI_LOGI(0x100000b2, "CMUX recv flow ctrl on msg");
        for (int n = 1; n < MAX_DLC_NUM; n++)
        {
            atCmuxChannel_t *ch = &th->channels[n];
            if (ch->state == AT_CMUX_CHANNEL_FLOW_STOPPED)
                ch->state = AT_CMUX_CHANNEL_CONNECT;
        }
        _sendCtrlFCON(th, CTRL_RESPONSE);
        break;
    }
    case CTRL_MSG_FCOFF:
    {
        OSI_LOGI(0x100000b3, "CMUX recv flow ctrl off msg");
        for (int n = 1; n < MAX_DLC_NUM; n++)
        {
            atCmuxChannel_t *ch = &th->channels[n];
            if (ch->state == AT_CMUX_CHANNEL_CONNECT)
                ch->state = AT_CMUX_CHANNEL_FLOW_STOPPED;
        }
        _sendCtrlFCOFF(th, CTRL_RESPONSE);
        break;
    }
    case CTRL_MSG_MSC:
    {
        uint8_t dlci = GET_BITS26(val[0]);
        OSI_LOGI(0x100000b4, "CMUX recv modem status msg dlci=%d", dlci);

        atCmuxChannel_t *ch = &th->channels[dlci];
        if (val[1] & MSC_FC)
        {
            if (ch->state == AT_CMUX_CHANNEL_CONNECT)
                ch->state = AT_CMUX_CHANNEL_FLOW_STOPPED;
        }
        else
        {
            if (ch->state == AT_CMUX_CHANNEL_FLOW_STOPPED)
                ch->state = AT_CMUX_CHANNEL_CONNECT;
        }
        uint8_t v24 = (ch->state == AT_CMUX_CHANNEL_CONNECT || AT_CMUX_CHANNEL_FLOW_STOPPED) ? (MSC_DSR_DTR | MSC_CTS_RFR) : 0;
        _sendCtrlMSC(th, CTRL_RESPONSE, dlci, v24, 0);
        break;
    }
    case CTRL_MSG_SNC:
    {
        OSI_LOGI(0x100000b5, "CMUX recv service negotiation msg");
        uint8_t dlci = GET_BITS26(val[0]);
        uint8_t srv;
        uint8_t codec;
        // only data is supported
        if (valLen == 1)
        {
            srv = SET_CR(1);
            codec = 0;
        }
        else
        {
            srv = val[1] & SET_CR(1);
            codec = 0;
        }
        _sendCtrlSNC(th, CTRL_RESPONSE, dlci, srv, codec);
        break;
    }

    case CTRL_MSG_NSC:
    case CTRL_MSG_RPN:
    case CTRL_MSG_RLS:
    default:
        OSI_LOGW(0x100000b6, "CMUX unsupported msg %d", type);
        _sendCtrlNSC(th, CR_RESPONSE, frame->info[0]);
        break;
    }
}

// =============================================================================
// Parse and handle UIH frame
// =============================================================================
static void _handleUIH(atCmuxEngine_t *th, atCmuxFrame_t *frame)
{
    if (!_isDlciValid(th, frame->dlci))
    {
        OSI_LOGW(0x100000b7, "CMUX UIH invalid dlci=%d", frame->dlci);
        _sendDM(th, frame->dlci);
        return;
    }

    atCmuxChannel_t *ch = &th->channels[frame->dlci];
    if (ch->state != AT_CMUX_CHANNEL_CONNECT && ch->state != AT_CMUX_CHANNEL_FLOW_STOPPED)
    {
        OSI_LOGW(0x100000b8, "CMUX UIH dlci=%d not connected", frame->dlci);
        _sendDM(th, frame->dlci);
        return;
    }

    if (frame->dlci == CTRL_DLCI)
    {
        _parseCtrl(th, frame);
    }
    else
    {
        atDispatchPushData(ch->dispatch, frame->info, frame->infoLen);
    }
}

// =============================================================================
// Parse and handle frame
// =============================================================================
static void _parseFrame(atCmuxEngine_t *th, atCmuxFrame_t *frame)
{
    OSI_LOGD(0x100000b9, "CMUX frame dlci=%d ctrl=0x%02x len=%d", frame->dlci, frame->ctrl, frame->infoLen);

    switch (frame->ctrl & ~CTRL_PF)
    {
    case FRAME_TYPE_SABM:
    {
        OSI_LOGI(0x100000ba, "CMUX SAMB received");

        if (!_isDlciValid(th, frame->dlci))
        {
            OSI_LOGW(0x100000bb, "CMUX invalid dlci");
            _sendDM(th, frame->dlci);
            return;
        }

        atCmuxChannel_t *ch = &th->channels[frame->dlci];
        if (frame->dlci == CTRL_DLCI)
        {
            ch->state = AT_CMUX_CHANNEL_CONNECT;
            _sendUA(th, frame->dlci);
        }
        else
        {
            if (ch->dispatch != NULL)
            {
                // already estabilished
                _sendUA(th, frame->dlci);
                return;
            }

            ch->dispatch = atDispatchDlcCreate(NULL, th, frame->dlci);
            if (ch->dispatch != NULL)
            {
                ch->state = AT_CMUX_CHANNEL_CONNECT;
                _sendUA(th, frame->dlci);
            }
            else
            {
                // create dispatch failed
                _sendDM(th, frame->dlci);
            }
        }
        break;
    }
    case FRAME_TYPE_UA:
    {
        OSI_LOGW(0x100000bc, "CMUX unexpected UA response and dropped, dlci/%d", frame->dlci);
        break;
    }
    case FRAME_TYPE_DM:
    {
        OSI_LOGW(0x100000bd, "CMUX unexpected DM response and dropped, dlci/%d", frame->dlci);
        break;
    }
    case FRAME_TYPE_DISC:
    {
        if (!_isDlciValid(th, frame->dlci))
        {
            OSI_LOGW(0x100000be, "CMUX invalid dlci=%d", frame->dlci);
            _sendDM(th, frame->dlci);
            return;
        }

        OSI_LOGI(0x100000bf, "CMUX DISC received");
        atCmuxChannel_t *ch = &th->channels[frame->dlci];
        if (ch->state == AT_CMUX_CHANNEL_DISCONNECT)
        {
            _sendDM(th, frame->dlci);
        }
        else
        {
            _sendUA(th, frame->dlci);
            if (frame->dlci == CTRL_DLCI)
            {
                atDispatchEndCmuxMode(th->dispatch);
            }
            else
            {
                ch->state = AT_CMUX_CHANNEL_DISCONNECT;
                atDispatchDelete(ch->dispatch);
                ch->dispatch = NULL;

                // TODO: (27010 Clause 5.8.2) When all DLC expcet 0 are closed,
                // UE should send CLD to TE. This isn't implemented. And we will
                // wait TE to send CLD or DISC/0.
            }
        }
        break;
    }
    case FRAME_TYPE_UIH:
        _handleUIH(th, frame);
        break;
    default:
        OSI_LOGW(0x100000c0, "CMUX invalid frame type 0x%02x in dlci/%d", frame->ctrl, frame->dlci);
        break;
    }
}

// =============================================================================
// Check frame FCS
// =============================================================================
static bool _checkFrame(atCmuxEngine_t *th, atCmuxFrame_t *frame)
{
    uint8_t *data = th->buff;
    unsigned length = th->buff_len;

    frame->dlci = GET_BITS26(data[1]);
    frame->ctrl = data[2];

    int fcslen = (th->mode == AT_CMUX_ADV_OPTION) ? 2 : GET_EA(data[3]) ? 3 : 4;
    if (!cmuxFcsCheck(data + 1, fcslen, data[length - 2]))
    {
        OSI_LOGW(0x100000c1, "CMUX frame FCS error, drop len=%d", th->buff_len);
        OSI_LOGXD(OSI_LOGPAR_S, 0x10000101, "CMUX buffer %*s", th->buff_len, th->buff);
        return false;
    }
    frame->infoLen = th->buff_len - (fcslen + 3); // SYNC ... FCS SYNC
    frame->info = &data[fcslen + 1];
    return true;
}

// =============================================================================
// Handle input data in BASIC option
// =============================================================================
static int _basicPushData(atCmuxEngine_t *th, const uint8_t *data, unsigned length)
{
    OSI_LOGD(0x100000c2, "CMUX basic push data len=%d", length);
    unsigned total = length;

    while (length > 0)
    {
        uint8_t c = *data++;
        length--;

        // Drop whole buffer at overflow, maybe caused by data error
        // or buffer too small
        if (th->buff_len >= th->buff_size)
        {
            OSI_LOGW(0x100000c3, "CMUX basic overflow len=%d", th->buff_len);
            th->buff_len = 0;
        }

        if (th->buff_len == 0 && c != BASIC_FLAG)
        {
            OSI_LOGV(0x100000c4, "CMUX basic drop byte before sync");
            continue;
        }
        if (th->buff_len == 1 && c == BASIC_FLAG)
        {
            OSI_LOGD(0x100000c5, "CMUX basic drop dup sync byte");
            continue;
        }

        if (th->buff_len < BASIC_MIN_FRAME_SIZE - 1)
        {
            _pushByte(th, c);
            continue;
        }

        unsigned lenexp = GET_BITS17(th->buff[3]);
        if (GET_EA(th->buff[3]) == 0)
            lenexp = (lenexp | (th->buff[4] << 7)) + 7;
        else
            lenexp += 6;

        // In case an invalid length is received, it will make error recovery
        // faster by dropping. Otherwise, the invalid data will be dropped at
        // input buffer overflow.
        if (lenexp > th->buff_size)
        {
            OSI_LOGW(0, "CMUX basic invalid len=%d", lenexp);
            th->buff_len = 0;
            continue;
        }

        _pushByte(th, c);
        if (th->buff_len >= lenexp)
        {
            if (c != BASIC_FLAG)
            {
                OSI_LOGW(0x100000c6, "CMUX missing end flag drop len=%d", th->buff_len);
                th->buff_len = 0;
            }
            else
            {
                atCmuxFrame_t frame = {};
                if (_checkFrame(th, &frame))
                {
                    atDispatch_t *dispatch = th->dispatch;

                    _parseFrame(th, &frame);

                    // It is possible CMUX is ended at procssing frame
                    if (atDispatchIsCmuxMode(dispatch))
                    {
                        th->buff_len = 0; // drop handled data
                        _pushByte(th, c);
                    }
                    atDispatchReadLater(dispatch);
                    return total - length;
                }
                th->buff_len = 0;
                _pushByte(th, c);
            }
        }
    }
    return total - length;
}

// =============================================================================
// Handle input data in ADVANCED option
// =============================================================================
static int _advPushData(atCmuxEngine_t *th, const uint8_t *data, unsigned length)
{
    OSI_LOGD(0x100000c7, "CMUX adv push data len=%d", length);
    unsigned total = length;

    while (length > 0)
    {
        uint8_t c = *data++;
        length--;

        // Drop whole buffer at overflow, maybe caused by data error
        // or buffer too small
        if (th->buff_len >= th->buff_size)
        {
            th->escaped = false;
            th->buff_len = 0;
        }

        if (th->buff_len == 0 && c != ADV_FLAG)
        {
            th->escaped = false;
            continue;
        }

        if (c == ADV_FLAG)
        {
            th->escaped = false;
            _pushByte(th, c);
            if (th->buff_len == 1)
            {
                ; // that is fine
            }
            else if (th->buff_len >= ADV_MIN_FRAME_SIZE)
            {
                atCmuxFrame_t frame = {};
                if (_checkFrame(th, &frame))
                {
                    atDispatch_t *dispatch = th->dispatch;

                    _parseFrame(th, &frame);

                    // It is possible CMUX is ended at procssing frame
                    if (atDispatchIsCmuxMode(dispatch))
                    {
                        th->buff_len = 0; // drop handled data
                        _pushByte(th, c);
                    }
                    atDispatchReadLater(dispatch);
                    return total - length;
                }
                th->buff_len = 0;
                _pushByte(th, c);
            }
            else
            {
                th->buff_len = 0;
                _pushByte(th, c);
            }
        }
        else
        {
            if (th->escaped)
            {
                _pushByte(th, c ^ ADV_ESCAPE_MASK);
                th->escaped = false;
            }
            else if (c == ADV_ESCAPE_CTRL)
            {
                th->escaped = true;
            }
            else
            {
                _pushByte(th, c);
            }
        }
    }
    return total - length;
}

// =============================================================================
// atCmuxPushData
// =============================================================================
int atCmuxPushData(atCmuxEngine_t *th, const void *data, size_t length)
{
    OSI_LOGXD(OSI_LOGPAR_M, 0x10000102, "CMUX push data %*s", length, data);
    if (th->mode == AT_CMUX_BASIC_OPTION)
        return _basicPushData(th, data, length);
    if (th->mode == AT_CMUX_ADV_OPTION)
        return _advPushData(th, data, length);
    return -1; // not reachable
}

// =============================================================================
// Output DLC data as UIH
// =============================================================================
void atCmuxDlcWrite(atCmuxEngine_t *th, const void *data, size_t length, int dlci)
{
    if (dlci <= 0 || dlci > MAX_DLC_NUM)
    {
        OSI_LOGE(0x100000c8, "CMUX invalid dlci %d for write, drop it", dlci);
        return;
    }

    _sendUIH(th, (const uint8_t *)data, length, dlci);
}

// =============================================================================
// atCmuxEngineCreate
// =============================================================================
atCmuxEngine_t *atCmuxEngineCreate(atDispatch_t *dispatch, const atCmuxConfig_t *cfg)
{
    OSI_LOGI(0x100000c9, "CMUX engine create");

    unsigned mem_size = sizeof(atCmuxEngine_t) +
                        OSI_ALIGN_UP(gAtCmuxInBuffSize, 4) +
                        OSI_ALIGN_UP(gAtCmuxOutBuffSize, 4) +
                        OSI_ALIGN_UP(gAtCmuxDlcNum * sizeof(atCmuxChannel_t), 4);
    uintptr_t mem = (uintptr_t)calloc(1, mem_size);
    if (mem == 0)
        return NULL;

    atCmuxEngine_t *th = (atCmuxEngine_t *)OSI_PTR_INCR_POST(mem, sizeof(atCmuxEngine_t));
    th->buff = (uint8_t *)OSI_PTR_INCR_POST(mem, OSI_ALIGN_UP(gAtCmuxInBuffSize, 4));
    th->outBuff = (uint8_t *)OSI_PTR_INCR_POST(mem, OSI_ALIGN_UP(gAtCmuxOutBuffSize, 4));
    th->channels = (atCmuxChannel_t *)OSI_PTR_INCR_POST(mem, OSI_ALIGN_UP(gAtCmuxDlcNum * sizeof(atCmuxChannel_t), 4));

    th->cfg = *cfg;
    th->mode = (cfg->transparency == 0) ? AT_CMUX_BASIC_OPTION : AT_CMUX_ADV_OPTION;
    th->dispatch = dispatch;
    th->buff_len = 0;
    th->buff_size = gAtCmuxInBuffSize;
    th->escaped = false;
    th->outLen = 0;
    th->max_frame_size = cfg->max_frame_size;
    for (int n = 0; n < MAX_DLC_NUM; n++)
    {
        th->channels[n].dispatch = NULL;
        th->channels[n].state = AT_CMUX_CHANNEL_DISCONNECT;
        th->channels[n].param.frameType = 0;   // UIH
        th->channels[n].param.convergence = 0; // Type 1
        th->channels[n].param.priority = _defaultPriority(n);
        th->channels[n].param.ack_timer = cfg->ack_timer;
        th->channels[n].param.frameSize = gAtCmuxInBuffSize / 2;
        th->channels[n].param.maxRetrans = cfg->max_retrans_count;
        th->channels[n].param.window_size = cfg->window_size;
    }
    return th;
}

// =============================================================================
// atCmuxEngineDelete
// =============================================================================
void atCmuxEngineDelete(atCmuxEngine_t *th)
{
    if (th == NULL)
        return;

    OSI_LOGI(0x100000ca, "CMUX engine destroy");
    for (int n = 0; n < MAX_DLC_NUM; n++)
        atDispatchDelete(th->channels[n].dispatch);
    free(th);
}

atCmuxConfig_t atCmuxGetConfig(atCmuxEngine_t *th) { return th->cfg; }
int atCmuxMaxFrameSize(atCmuxEngine_t *th) { return th->max_frame_size; }
atDevice_t *atCmuxGetDevice(atCmuxEngine_t *th) { return atDispatchGetDevice(th->dispatch); }
atDispatch_t *atCmuxGetDispatch(atCmuxEngine_t *th) { return th->dispatch; }
