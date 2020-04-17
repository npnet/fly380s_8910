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

#include "kernel_config.h"
#include "drv_config.h"
#include "osi_log.h"
#include "osi_api.h"
#include "osi_internal.h"
#include "osi_chip.h"
#include "osi_trace.h"
#include "hwregs.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <alloca.h>

#define TRACE_PARCOUNT_MAX (16) // should larger than 8
#define TRACE_STRLEN_MAX (256)
#define TRACE_TDB_FLAG (1 << 31)

#ifdef CONFIG_KERNEL_LOG_IN_CRITICAL
#define LOG_PROTECT_ENTER uint32_t critical = osiEnterCritical()
#define LOG_PROTECT_EXIT osiExitCritical(critical)
#else
#define LOG_PROTECT_ENTER
#define LOG_PROTECT_EXIT
#endif

static const char *gLogNullString = "(null)";
bool gTraceEnabled = true;

typedef struct osiTraceSxCtrl
{
    uint32_t spy_bitmap;
    uint16_t id_bitmap[32];
} osiTraceSxCtrl_t;

uint32_t gTraceSequence = 0;
osiTraceSxCtrl_t sxs_IoCtx;
uint32_t v_tra_pubModuleControl;
uint32_t v_tra_lteModuleControl;
uint32_t v_tra_categoryControl;

typedef struct
{
    osiHostPacketHeader_t host;
#ifdef CONFIG_KERNEL_TRACE_BBC8
    osiTraPacketHeader_t tra;
#elif defined(CONFIG_KERNEL_TRACE_HOST98)
    uint16_t sn;
    uint16_t tick;
    uint32_t tag;
#elif defined(CONFIG_KERNEL_TRACE_HOST97)
    uint32_t tag;
    uint32_t tick;
#endif
} osiTagPacketHeader_t;

typedef struct
{
    osiHostPacketHeader_t host;
    osiTraPacketHeader_t tra;
} osiHostTraPacketHeader_t;

typedef struct
{
    osiHostPacketHeader_t host;
#ifdef CONFIG_KERNEL_TRACE_BBC8
    osiTraPacketHeader_t tra;
#else
    uint16_t sn;
    uint16_t tick;
    uint16_t id;
    uint16_t fn;
#endif
} osiSxPacketHeader_t;

typedef struct
{
    uint8_t count;
    uint8_t type[TRACE_PARCOUNT_MAX];
    uint8_t slen[TRACE_PARCOUNT_MAX]; // data type shall match TRACE_STRLEN_MAX
} traceParamInfo_t;

static inline uint32_t *_requestTagBuffer(unsigned tag, unsigned data_len)
{
    uint32_t critical = osiEnterCritical();

    gTraceSequence++;
    uint32_t packet_len = data_len + sizeof(osiTagPacketHeader_t);
    uint32_t *buf = osiTraceBufRequestLocked(packet_len);
    if (buf != NULL)
    {
        osiTagPacketHeader_t *head = (osiTagPacketHeader_t *)buf;

#ifdef CONFIG_KERNEL_TRACE_BBC8
        osiFillHostHeader(&head->host, 0x96, packet_len - 4);
        head->tra.sync = 0xBBBB;
        head->tra.plat_id = 1;
        head->tra.type = (tag & TRACE_TDB_FLAG) ? 0xC8 : 0xC9;
        head->tra.sn = gTraceSequence;
        head->tra.fn_wcdma = osiChipTraceTick();
        head->tra.fn_gge = tag;
        head->tra.fn_lte = osiTraceLteFrameNumber();
#elif defined(CONFIG_KERNEL_TRACE_HOST98)
        osiFillHostHeader(&head->host, 0x98, packet_len - 4);
        head->tag = tag;
        head->sn = gTraceSequence;
        head->tick = osiChipTraceTick();
#elif defined(CONFIG_KERNEL_TRACE_HOST97)
        osiFillHostHeader(&head->host, 0x97, packet_len - 4);
        head->tag = tag;
        head->tick = osiChipTraceTick();
#endif

        buf = (uint32_t *)((char *)buf + sizeof(osiTagPacketHeader_t));
    }
    osiExitCritical(critical);
    return buf;
}

static inline uint32_t *_requestTraBuffer(uint8_t type, unsigned data_len)
{
    uint32_t critical = osiEnterCritical();

    gTraceSequence++;
    uint32_t packet_len = data_len + sizeof(osiHostTraPacketHeader_t);
    uint32_t *buf = osiTraceBufRequestLocked(packet_len);
    if (buf != NULL)
    {
        osiHostTraPacketHeader_t *head = (osiHostTraPacketHeader_t *)buf;

        osiFillHostHeader(&head->host, 0x96, packet_len - 4);
        head->tra.sync = 0xBBBB;
        head->tra.plat_id = 1;
        head->tra.type = type;
        head->tra.sn = gTraceSequence;
        head->tra.fn_wcdma = osiChipTraceTick();
        head->tra.fn_gge = osiTraceGsmFrameNumber();
        head->tra.fn_lte = osiTraceLteFrameNumber();

        buf = (uint32_t *)((char *)buf + sizeof(osiHostTraPacketHeader_t));
    }
    osiExitCritical(critical);
    return buf;
}

static inline uint32_t *_requestSxBuffer(unsigned data_len, unsigned id, bool use_id)
{
    uint32_t critical = osiEnterCritical();

    gTraceSequence++;
    uint32_t packet_len = data_len + sizeof(osiSxPacketHeader_t);
    uint32_t *buf = osiTraceBufRequestLocked(packet_len);
    if (buf != NULL)
    {
        osiSxPacketHeader_t *head = (osiSxPacketHeader_t *)buf;

#ifdef CONFIG_KERNEL_TRACE_BBC8
        osiFillHostHeader(&head->host, 0x96, packet_len - 4);
        head->tra.sync = 0xBBBB;
        head->tra.plat_id = 1;
        head->tra.type = use_id ? 0xC8 : 0xC9;
        head->tra.sn = gTraceSequence;
        head->tra.fn_wcdma = osiChipTraceTick();
        head->tra.fn_gge = 0xf0000000 | ((id & 0x1ff) << 16) | (osiTraceGsmFrameNumber() % (1326 * 32));
        head->tra.fn_lte = osiTraceLteFrameNumber();
#else
        osiFillHostHeader(&head->host, 0x99, packet_len - 4);
        head->id = (id & 0x1ff) | (use_id ? 0x8000 : 0);
        head->fn = (osiTraceGsmFrameNumber() % (1326 * 32));
        head->sn = gTraceSequence;
        head->tick = osiChipTraceTick();
#endif

        buf = (uint32_t *)((char *)buf + sizeof(osiSxPacketHeader_t));
    }
    osiExitCritical(critical);
    return buf;
}

OSI_WEAK uint32_t osiTraceLteFrameNumber(void)
{
    return 0;
}

OSI_WEAK uint32_t osiTraceGsmFrameNumber(void)
{
    return 0;
}

static inline uint32_t *_textCopy(uint32_t *buf, const char *str, uint32_t slen)
{
    uint32_t wcount = (slen + 1 + 3) / 4;
#ifdef CONFIG_KERNEL_TRACE_BBC8
    buf[wcount - 1] = 0x00202020;
#else
    buf[wcount - 1] = 0x00000000;
#endif
    memcpy(buf, str, slen);
    return buf + wcount;
}

static int _paramLen(uint32_t partype, traceParamInfo_t *pinfo, va_list ap)
{
    int request_len = 0;
    int npar = 0;

    for (npar = 0; partype != 0; npar++)
    {
        uint8_t pt = (partype & 0xf);
        partype >>= 4;

        pinfo->type[npar] = pt;
        if (pt == __OSI_LOGPAR_D)
        {
            va_arg(ap, uint64_t);
            request_len += 8;
        }
        else if (pt == __OSI_LOGPAR_F)
        {
            va_arg(ap, double);
            request_len += 8;
        }
        else if (pt == __OSI_LOGPAR_S)
        {
            const char *str = va_arg(ap, const char *);
            if (str == NULL)
                str = gLogNullString;

            uint32_t slen = strnlen(str, TRACE_STRLEN_MAX - 1);
            pinfo->slen[npar] = slen;
            request_len += OSI_ALIGN_UP(slen + 1, 4);
        }
        else if (pt == __OSI_LOGPAR_M)
        {
            uint32_t size = va_arg(ap, uint32_t);
            va_arg(ap, const uint8_t *);
            request_len += OSI_ALIGN_UP(6 + size, 4);
        }
        else
        {
            va_arg(ap, uint32_t);
            request_len += 4;
        }
    }
    pinfo->count = npar;
    return request_len;
}

static int _paramLenStr(const char *fmt, traceParamInfo_t *pinfo, va_list ap)
{
    int request_len = 0;
    int npar = 0;

    for (;;)
    {
        char c = *fmt++;
        if (c == '\0')
            break;
        if (c != '%')
            continue;

        for (;;)
        {
            char cc = *fmt++;
            if (cc == 'd' || cc == 'i' || cc == 'o' || cc == 'u' ||
                cc == 'x' || cc == 'X')
            {
                if (fmt[-2] == 'l' && fmt[-3] == 'l')
                {
                    va_arg(ap, uint64_t);
                    request_len += 8;
                    pinfo->type[npar++] = __OSI_LOGPAR_D;
                }
                else
                {
                    va_arg(ap, uint32_t);
                    request_len += 4;
                    pinfo->type[npar++] = __OSI_LOGPAR_I;
                }
                break;
            }
            else if (cc == 'c' || cc == 'p')
            {
                va_arg(ap, uint32_t);
                request_len += 4;
                pinfo->type[npar++] = __OSI_LOGPAR_I;
                break;
            }
            else if (cc == 'e' || cc == 'E' || cc == 'f' || cc == 'F' ||
                     cc == 'g' || cc == 'G' || cc == 'a' || cc == 'A')
            {
                va_arg(ap, double);
                request_len += 8;
                pinfo->type[npar++] = __OSI_LOGPAR_F;
                break;
            }
            else if (cc == 's')
            {
                if (fmt[-2] == '*')
                {
                    uint32_t size = va_arg(ap, uint32_t);
                    va_arg(ap, const uint8_t *);
                    request_len += OSI_ALIGN_UP(6 + size, 4);
                    pinfo->type[npar++] = __OSI_LOGPAR_M;
                }
                else
                {
                    const char *str = va_arg(ap, const char *);
                    if (str == NULL)
                        str = gLogNullString;

                    uint32_t slen = strnlen(str, TRACE_STRLEN_MAX - 1);
                    request_len += OSI_ALIGN_UP(slen + 1, 4);
                    pinfo->slen[npar] = slen;
                    pinfo->type[npar++] = __OSI_LOGPAR_S;
                }
                break;
            }
            else if (cc == '%')
            {
                break;
            }
            else if (cc == '\0')
            {
                break;
            }
            else
            {
                continue;
            }
        }

        if (npar >= TRACE_PARCOUNT_MAX)
            break;
    }
    pinfo->count = npar;
    return request_len;
}

static uint32_t *_paramFill(uint32_t *buf, traceParamInfo_t *pinfo, va_list ap)
{
    for (int npar = 0; npar < pinfo->count; npar++)
    {
        uint8_t pt = pinfo->type[npar];
        if (pt == __OSI_LOGPAR_D)
        {
            uint64_t val = va_arg(ap, uint64_t);
            *buf++ = val;
            *buf++ = (val >> 32);
        }
        else if (pt == __OSI_LOGPAR_F)
        {
            double val = va_arg(ap, double);
            uint32_t *pval = (uint32_t *)&val;
            *buf++ = *pval++;
            *buf++ = *pval++;
        }
        else if (pt == __OSI_LOGPAR_M)
        {
            uint32_t size = va_arg(ap, uint32_t);
            const uint8_t *data = va_arg(ap, const uint8_t *);
            *buf = (uint32_t)data;
            *((uint16_t *)buf + 2) = size;
            memcpy((uint8_t *)buf + 6, data, size);
            buf += (6 + size + 3) / 4;
        }
        else if (pt == __OSI_LOGPAR_S)
        {
            const char *str = va_arg(ap, const char *);
            if (str == NULL)
                str = gLogNullString;

            buf = _textCopy(buf, str, pinfo->slen[npar]);
        }
        else
        {
            *buf++ = va_arg(ap, uint32_t);
        }
    }
    return buf;
}

void osiTraceBasic(unsigned tag, unsigned nargs, const char *fmt, ...)
{
    if (!gTraceEnabled)
        return;

    uint32_t fmt_len = strnlen(fmt, TRACE_STRLEN_MAX - 1);
    uint32_t fmt_len_aligned = OSI_ALIGN_UP(fmt_len + 1, 4);
    uint32_t request_len = fmt_len_aligned + 4 * nargs;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestTagBuffer(tag, request_len);
    if (buf != NULL)
    {
        buf = _textCopy(buf, fmt, fmt_len);

        va_list ap;
        va_start(ap, fmt);
        for (uint32_t n = 0; n < nargs; n++)
            *buf++ = va_arg(ap, uint32_t);
        va_end(ap);

        osiTraceBufFilled();
    }
    LOG_PROTECT_EXIT;
}

void osiTraceIdBasic(unsigned tag, unsigned nargs, unsigned fmtid, ...)
{
    if (!gTraceEnabled)
        return;

    uint32_t request_len = 4 + 4 * nargs;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestTagBuffer(tag | TRACE_TDB_FLAG, request_len);
    if (buf != NULL)
    {
        *buf++ = fmtid;

        va_list ap;
        va_start(ap, fmtid);
        for (uint32_t n = 0; n < nargs; n++)
            *buf++ = va_arg(ap, uint32_t);
        va_end(ap);

        osiTraceBufFilled();
    }
    LOG_PROTECT_EXIT;
}

void osiTraceEx(unsigned tag, unsigned partype, const char *fmt, ...)
{
    if (!gTraceEnabled)
        return;

    va_list apl;
    va_start(apl, fmt);
    va_list apf;
    va_copy(apf, apl);

    traceParamInfo_t pinfo;
    uint32_t fmt_len = strnlen(fmt, TRACE_STRLEN_MAX - 1);
    uint32_t fmt_len_aligned = OSI_ALIGN_UP(fmt_len + 1, 4);
    uint32_t par_len = _paramLen(partype, &pinfo, apl);
    uint32_t request_len = fmt_len_aligned + par_len;
    va_end(apl);

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestTagBuffer(tag, request_len);
    if (buf != NULL)
    {
        buf = _textCopy(buf, fmt, fmt_len);
        buf = _paramFill(buf, &pinfo, apf);
        va_end(apf);

        osiTraceBufFilled();
    }
    else
    {
        va_end(apf);
    }
    LOG_PROTECT_EXIT;
}

void osiTraceIdEx(unsigned tag, unsigned partype, unsigned fmtid, ...)
{
    if (!gTraceEnabled)
        return;

    va_list apl;
    va_start(apl, fmtid);
    va_list apf;
    va_copy(apf, apl);

    traceParamInfo_t pinfo;
    uint32_t par_len = _paramLen(partype, &pinfo, apl);
    uint32_t request_len = 4 + par_len;
    va_end(apl);

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestTagBuffer(tag | TRACE_TDB_FLAG, request_len);
    if (buf != NULL)
    {
        *buf++ = fmtid;
        buf = _paramFill(buf, &pinfo, apf);
        va_end(apf);

        osiTraceBufFilled();
    }
    else
    {
        va_end(apf);
    }
    LOG_PROTECT_EXIT;
}

void osiTracePrintf(unsigned tag, const char *fmt, ...)
{
    if (!gTraceEnabled)
        return;

    va_list ap;
    va_start(ap, fmt);
    osiTraceVprintf(tag, fmt, ap);
    va_end(ap);
}

void osiTraceVprintf(unsigned tag, const char *fmt, va_list ap)
{
    if (!gTraceEnabled)
        return;

    va_list apf;
    va_copy(apf, ap);

    traceParamInfo_t pinfo;
    uint32_t fmt_len = strnlen(fmt, TRACE_STRLEN_MAX - 1);
    uint32_t fmt_len_aligned = OSI_ALIGN_UP(fmt_len + 1, 4);
    uint32_t par_len = _paramLenStr(fmt, &pinfo, ap);
    uint32_t request_len = fmt_len_aligned + par_len;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestTagBuffer(tag, request_len);
    if (buf != NULL)
    {
        buf = _textCopy(buf, fmt, fmt_len);
        buf = _paramFill(buf, &pinfo, apf);
        va_end(apf);

        osiTraceBufFilled();
    }
    else
    {
        va_end(apf);
    }
    LOG_PROTECT_EXIT;
}

static void _traceTraBasic(unsigned nargs, const char *fmt, va_list ap)
{
    uint32_t fmt_len = strnlen(fmt, TRACE_STRLEN_MAX - 1);
    uint32_t fmt_len_aligned = OSI_ALIGN_UP(fmt_len + 1, 4);
    uint32_t request_len = fmt_len_aligned + 4 * nargs;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestTraBuffer(0xC9, request_len);
    if (buf != NULL)
    {
        buf = _textCopy(buf, fmt, fmt_len);

        for (uint32_t n = 0; n < nargs; n++)
            *buf++ = va_arg(ap, uint32_t);

        osiTraceBufFilled();
    }
    LOG_PROTECT_EXIT;
}

void osiTraceTraBasic(unsigned nargs, const char *fmt, ...)
{
    if (!gTraceEnabled)
        return;

    va_list ap;
    va_start(ap, fmt);
    _traceTraBasic(nargs, fmt, ap);
    va_end(ap);
}

void osiTracePubBasic(unsigned module, unsigned category, unsigned nargs, const char *fmt, ...)
{
    if (!gTraceEnabled ||
        !(module & v_tra_pubModuleControl) ||
        !(category & v_tra_categoryControl))
        return;

    va_list ap;
    va_start(ap, fmt);
    _traceTraBasic(nargs, fmt, ap);
    va_end(ap);
}

void osiTraceLteBasic(unsigned module, unsigned category, unsigned nargs, const char *fmt, ...)
{
    if (!gTraceEnabled ||
        !(module & v_tra_lteModuleControl) ||
        !(category & v_tra_categoryControl))
        return;

    va_list ap;
    va_start(ap, fmt);
    _traceTraBasic(nargs, fmt, ap);
    va_end(ap);
}

static void _traceTraIdBasic(unsigned nargs, unsigned fmtid, va_list ap)
{
    uint32_t request_len = 4 + 4 * nargs;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestTraBuffer(0xC8, request_len);
    if (buf != NULL)
    {
        *buf++ = fmtid;

        for (uint32_t n = 0; n < nargs; n++)
            *buf++ = va_arg(ap, uint32_t);

        osiTraceBufFilled();
    }
    LOG_PROTECT_EXIT;
}

void osiTraceTraIdBasic(unsigned nargs, unsigned fmtid, ...)
{
    if (!gTraceEnabled)
        return;

    va_list ap;
    va_start(ap, fmtid);
    _traceTraIdBasic(nargs, fmtid, ap);
    va_end(ap);
}

void osiTracePubIdBasic(unsigned module, unsigned category, unsigned nargs, unsigned fmtid, ...)
{
    if (!gTraceEnabled ||
        !(module & v_tra_pubModuleControl) ||
        !(category & v_tra_categoryControl))
        return;

    va_list ap;
    va_start(ap, fmtid);
    _traceTraIdBasic(nargs, fmtid, ap);
    va_end(ap);
}

void osiTraceLteIdBasic(unsigned module, unsigned category, unsigned nargs, unsigned fmtid, ...)
{
    if (!gTraceEnabled ||
        !(module & v_tra_lteModuleControl) ||
        !(category & v_tra_categoryControl))
        return;

    va_list ap;
    va_start(ap, fmtid);
    _traceTraIdBasic(nargs, fmtid, ap);
    va_end(ap);
}

static void _traceTraEx(unsigned partype, const char *fmt, va_list apl)
{
    va_list apf;
    va_copy(apf, apl);

    traceParamInfo_t pinfo;
    uint32_t fmt_len = strnlen(fmt, TRACE_STRLEN_MAX - 1);
    uint32_t fmt_len_aligned = OSI_ALIGN_UP(fmt_len + 1, 4);
    uint32_t par_len = _paramLen(partype, &pinfo, apl);
    uint32_t request_len = fmt_len_aligned + par_len;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestTraBuffer(0xC9, request_len);
    if (buf != NULL)
    {
        buf = _textCopy(buf, fmt, fmt_len);
        buf = _paramFill(buf, &pinfo, apf);
        va_end(apf);

        osiTraceBufFilled();
    }
    else
    {
        va_end(apf);
    }
    LOG_PROTECT_EXIT;
}

void osiTraceTraEx(unsigned partype, const char *fmt, ...)
{
    if (!gTraceEnabled)
        return;

    va_list apl;
    va_start(apl, fmt);
    _traceTraEx(partype, fmt, apl);
    va_end(apl);
}

void osiTracePubEx(unsigned module, unsigned category, unsigned partype, const char *fmt, ...)
{
    if (!gTraceEnabled ||
        !(module & v_tra_pubModuleControl) ||
        !(category & v_tra_categoryControl))
        return;

    va_list apl;
    va_start(apl, fmt);
    _traceTraEx(partype, fmt, apl);
    va_end(apl);
}

void osiTraceLteEx(unsigned module, unsigned category, unsigned partype, const char *fmt, ...)
{
    if (!gTraceEnabled ||
        !(module & v_tra_lteModuleControl) ||
        !(category & v_tra_categoryControl))
        return;

    va_list apl;
    va_start(apl, fmt);
    _traceTraEx(partype, fmt, apl);
    va_end(apl);
}

static void _traceTraIdEx(unsigned partype, unsigned fmtid, va_list apl)
{
    va_list apf;
    va_copy(apf, apl);

    traceParamInfo_t pinfo;
    uint32_t par_len = _paramLen(partype, &pinfo, apl);
    uint32_t request_len = 4 + par_len;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestTraBuffer(0xC8, request_len);
    if (buf != NULL)
    {
        *buf++ = fmtid;
        buf = _paramFill(buf, &pinfo, apf);
        va_end(apf);

        osiTraceBufFilled();
    }
    else
    {
        va_end(apf);
    }
    LOG_PROTECT_EXIT;
}

void osiTraceTraIdEx(unsigned partype, unsigned fmtid, ...)
{
    if (!gTraceEnabled)
        return;

    va_list apl;
    va_start(apl, fmtid);
    _traceTraIdEx(partype, fmtid, apl);
    va_end(apl);
}

void osiTracePubIdEx(unsigned module, unsigned category, unsigned partype, unsigned fmtid, ...)
{
    if (!gTraceEnabled ||
        !(module & v_tra_pubModuleControl) ||
        !(category & v_tra_categoryControl))
        return;

    va_list apl;
    va_start(apl, fmtid);
    _traceTraIdEx(partype, fmtid, apl);
    va_end(apl);
}

void osiTraceLteIdEx(unsigned module, unsigned category, unsigned partype, unsigned fmtid, ...)
{
    if (!gTraceEnabled ||
        !(module & v_tra_lteModuleControl) ||
        !(category & v_tra_categoryControl))
        return;

    va_list apl;
    va_start(apl, fmtid);
    _traceTraIdEx(partype, fmtid, apl);
    va_end(apl);
}

static inline bool _sxTraceEnabled(unsigned id)
{
    unsigned module = id & 0x1f;
    unsigned level = (id >> 5) & 0xf;
    unsigned out = id & (1 << 21);
    return out || (sxs_IoCtx.id_bitmap[module] & (1 << level));
}

static void _traceSxIdBasic(unsigned id, unsigned nargs, unsigned fmtid, va_list ap)
{
    uint32_t request_len = 4 + 4 * nargs;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestSxBuffer(request_len, id, true);
    if (buf != NULL)
    {
        *buf++ = fmtid;

        for (uint32_t n = 0; n < nargs; n++)
            *buf++ = va_arg(ap, uint32_t);

        osiTraceBufFilled();
    }
    LOG_PROTECT_EXIT;
}

void osiTraceSxIdBasic(unsigned id, unsigned nargs, unsigned fmtid, ...)
{
    if (!gTraceEnabled || !_sxTraceEnabled(id))
        return;

    va_list ap;
    va_start(ap, fmtid);
    _traceSxIdBasic(id, nargs, fmtid, ap);
    va_end(ap);
}

static void _traceSxIdEx(unsigned id, unsigned partype, unsigned fmtid, va_list apl)
{
    va_list apf;
    va_copy(apf, apl);

    traceParamInfo_t pinfo;
    uint32_t par_len = _paramLen(partype, &pinfo, apl);
    uint32_t request_len = 4 + par_len;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestSxBuffer(request_len, id, true);
    if (buf != NULL)
    {
        *buf++ = fmtid;
        buf = _paramFill(buf, &pinfo, apf);
        va_end(apf);

        osiTraceBufFilled();
    }
    else
    {
        va_end(apf);
    }
    LOG_PROTECT_EXIT;
}

void osiTraceSxIdEx(unsigned id, unsigned partype, unsigned fmtid, ...)
{
    if (!gTraceEnabled || !_sxTraceEnabled(id))
        return;

    va_list apl;
    va_start(apl, fmtid);
    _traceSxIdEx(id, partype, fmtid, apl);
    va_end(apl);
}

static void _traceSxBasic(unsigned id, unsigned nargs, const char *fmt, va_list ap)
{
    uint32_t fmt_len = strnlen(fmt, TRACE_STRLEN_MAX - 1);
    uint32_t fmt_len_aligned = OSI_ALIGN_UP(fmt_len + 1, 4);
    uint32_t request_len = fmt_len_aligned + 4 * nargs;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestSxBuffer(request_len, id, false);
    if (buf != NULL)
    {
        buf = _textCopy(buf, fmt, fmt_len);

        for (uint32_t n = 0; n < nargs; n++)
            *buf++ = va_arg(ap, uint32_t);

        osiTraceBufFilled();
    }
    LOG_PROTECT_EXIT;
}

void osiTraceSxBasic(unsigned id, unsigned nargs, const char *fmt, ...)
{
    if (!gTraceEnabled || !_sxTraceEnabled(id))
        return;

    va_list ap;
    va_start(ap, fmt);
    _traceSxBasic(id, nargs, fmt, ap);
    va_end(ap);
}

static void _traceSxEx(unsigned id, unsigned partype, const char *fmt, va_list apl)
{
    va_list apf;
    va_copy(apf, apl);

    traceParamInfo_t pinfo;
    uint32_t fmt_len = strnlen(fmt, TRACE_STRLEN_MAX - 1);
    uint32_t fmt_len_aligned = OSI_ALIGN_UP(fmt_len + 1, 4);
    uint32_t par_len = _paramLen(partype, &pinfo, apl);
    uint32_t request_len = fmt_len_aligned + par_len;

    LOG_PROTECT_ENTER;
    uint32_t *buf = _requestSxBuffer(request_len, id, false);
    if (buf != NULL)
    {
        buf = _textCopy(buf, fmt, fmt_len);
        buf = _paramFill(buf, &pinfo, apf);
        va_end(apf);

        osiTraceBufFilled();
    }
    else
    {
        va_end(apf);
    }
    LOG_PROTECT_EXIT;
}

void osiTraceSxEx(unsigned id, unsigned partype, const char *fmt, ...)
{
    if (!gTraceEnabled || !_sxTraceEnabled(id))
        return;

    va_list apl;
    va_start(apl, fmt);
    _traceSxEx(id, partype, fmt, apl);
    va_end(apl);
}

void osiTraceSxOutput(unsigned id, const char *fmt, va_list ap)
{
    if (!gTraceEnabled)
        return;

    unsigned tdb = id & (1 << 12);
    unsigned tsmap = (id >> 15) & 0x3f;
    unsigned nargs = (id & (1 << 22)) ? ((id >> 23) & 0x07) : 6;
    unsigned partype = OSI_TSMAP_PARTYPE(nargs, tsmap);

    if (tdb && tsmap == 0)
        _traceSxIdBasic(id, nargs, (unsigned)fmt, ap);
    else if (tdb && tsmap != 0)
        _traceSxIdEx(id, partype, (unsigned)fmt, ap);
    else if (tsmap == 0)
        _traceSxBasic(id, nargs, fmt, ap);
    else
        _traceSxEx(id, partype, fmt, ap);
}

uint32_t *osiTraceTraBufRequest(uint32_t tra_len)
{
    uint32_t critical = osiEnterCritical();

    gTraceSequence++;
    uint32_t packet_len = tra_len + sizeof(osiHostPacketHeader_t);
    uint32_t *buf = osiTraceBufRequestLocked(packet_len);
    if (buf != NULL)
    {
        osiHostTraPacketHeader_t *head = (osiHostTraPacketHeader_t *)buf;

        osiFillHostHeader(&head->host, 0x96, packet_len - 4);
        head->tra.sync = 0xBBBB;
        head->tra.plat_id = 1;
        head->tra.type = 0; // filled later
        head->tra.sn = gTraceSequence;
        head->tra.fn_wcdma = osiChipTraceTick();
        head->tra.fn_gge = osiTraceGsmFrameNumber();
        head->tra.fn_lte = osiTraceLteFrameNumber();

        buf = (uint32_t *)((char *)buf + sizeof(osiHostPacketHeader_t));
    }
    osiExitCritical(critical);
    return buf;
}
