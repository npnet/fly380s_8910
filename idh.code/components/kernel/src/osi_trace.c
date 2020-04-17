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

#include "osi_trace.h"
#include "osi_api.h"
#include <string.h>
#include <sys/queue.h>
#include <assert.h>

static_assert(OSI_IS_ALIGNED(CONFIG_KERNEL_TRACE_BUF_SIZE, CONFIG_CACHE_LINE_SIZE),
              "CONFIG_KERNEL_TRACE_BUF_SIZE must be cache line aligned");
static_assert(CONFIG_KERNEL_TRACE_BUF_COUNT >= 2,
              "CONFIG_KERNEL_TRACE_BUF_COUNT must be greater than or equal to 2");

#define TRACE_BUF_SIZE (CONFIG_KERNEL_TRACE_BUF_SIZE * CONFIG_KERNEL_TRACE_BUF_COUNT)
#define TRACE_THREAD_STACK_BYTES (1024)
#define TRACE_OUTPUT_TIMEOUT (50)

typedef TAILQ_ENTRY(osiTraceBuf) osiTraceBufIter_t;
typedef TAILQ_HEAD(osiTraceBufHead, osiTraceBuf) osiTraceBufHead_t;
typedef struct osiTraceBuf
{
    uint8_t *start;
    unsigned len;
    osiTraceBufIter_t iter;
} osiTraceBuf_t;

typedef struct
{
    osiTraceSender_t sender;
    osiTraceSender_t blue_screen_sender;
    osiWork_t *work;
    osiWorkQueue_t *work_queue;
    osiTimer_t *timer;
    osiSemaphore_t *finish_sema;

    unsigned pending_count;
    osiTraceBufHead_t work_buf_list; // head is to be filled, tail is to be transfer
    osiTraceBufHead_t idle_buf_list;
    osiTraceBuf_t bufs[CONFIG_KERNEL_TRACE_BUF_COUNT];
} osiTraceContext_t;

#ifdef CONFIG_SOC_8955
static uint8_t gTraceBuf[TRACE_BUF_SIZE] OSI_ALIGNED(4) OSI_SECTION_SRAM_UC_BSS;
#elif defined(CONFIG_SOC_8909)
static uint8_t gTraceBuf[TRACE_BUF_SIZE] OSI_ALIGNED(4) OSI_SECTION_RAM_UC_BSS;
#else
static uint8_t gTraceBuf[TRACE_BUF_SIZE] OSI_CACHE_LINE_ALIGNED;
#endif
static osiTraceContext_t gTraceCtx;

static bool prvSendNone(const void *data, unsigned size) { return false; }

void osiTraceEarlyInit(void)
{
    osiTraceContext_t *d = &gTraceCtx;

    d->sender = prvSendNone;
    d->blue_screen_sender = prvSendNone;
    d->work = NULL;
    d->work_queue = NULL;
    d->timer = NULL;
    d->finish_sema = NULL;

    TAILQ_INIT(&d->work_buf_list);
    TAILQ_INIT(&d->idle_buf_list);
    d->pending_count = 0;

    for (int n = 0; n < CONFIG_KERNEL_TRACE_BUF_COUNT; n++)
    {
        d->bufs[n].start = &gTraceBuf[n * CONFIG_KERNEL_TRACE_BUF_SIZE];
        d->bufs[n].len = 0;
        TAILQ_INSERT_TAIL(&d->idle_buf_list, &d->bufs[n], iter);
    }
}

uint32_t *osiTraceBufRequestLocked(uint32_t size)
{
    osiTraceContext_t *d = &gTraceCtx;
    size = OSI_ALIGN_UP(size, 4);

    osiTraceBuf_t *buf = TAILQ_FIRST(&d->work_buf_list);
    if (buf == NULL || buf->len + size > CONFIG_KERNEL_TRACE_BUF_SIZE)
    {
        buf = TAILQ_FIRST(&d->idle_buf_list);
        if (buf == NULL)
            return NULL;

        buf->len = 0;
        TAILQ_REMOVE(&d->idle_buf_list, buf, iter);
        TAILQ_INSERT_HEAD(&d->work_buf_list, buf, iter);
    }

    d->pending_count++;

    uint32_t *req = (uint32_t *)(buf->start + buf->len);
    buf->len += size;
    req[size / 4 - 1] = 0;
    return req;
}

uint32_t *osiTraceBufRequest(uint32_t size)
{
    uint32_t critical = osiEnterCritical();
    uint32_t *buf = osiTraceBufRequest(size);
    osiExitCritical(critical);
    return buf;
}

void osiTraceBufFilled(void)
{
    osiTraceContext_t *d = &gTraceCtx;
    uint32_t critical = osiEnterCritical();

    if (d->pending_count > 1)
        d->pending_count--;
    else
        d->pending_count = 0;
    osiExitCritical(critical);
}

static osiTraceBuf_t *prvFetchBuf(void)
{
    osiTraceContext_t *d = &gTraceCtx;
    uint32_t critical = osiEnterCritical();

    if (d->pending_count == 0)
    {
        osiTraceBuf_t *buf = TAILQ_LAST(&d->work_buf_list, osiTraceBufHead);
        if (buf != NULL)
        {
            TAILQ_REMOVE(&d->work_buf_list, buf, iter);
            osiExitCritical(critical);
            return buf;
        }
    }

    osiExitCritical(critical);
    return NULL;
}

static void prvTransferDone(osiTraceBuf_t *buf)
{
    if (buf == NULL)
        return;

    osiTraceContext_t *d = &gTraceCtx;
    uint32_t critical = osiEnterCritical();
    buf->len = 0;
    TAILQ_INSERT_TAIL(&d->idle_buf_list, buf, iter);
    osiExitCritical(critical);
}

static void prvTraceOutput(void *param)
{
    osiTraceContext_t *d = &gTraceCtx;

    osiTimerStop(d->timer);

    // avoid infinite loop
    for (int n = 0; n < 64; n++)
    {
        osiTraceBuf_t *buf = prvFetchBuf();
        if (buf == NULL)
        {
            osiSemaphoreRelease(d->finish_sema);
            if (gTraceEnabled)
                osiTimerStartRelaxed(d->timer, TRACE_OUTPUT_TIMEOUT, OSI_DELAY_MAX);
            return;
        }

        d->sender(buf->start, buf->len);
        prvTransferDone(buf);
    }

    // maybe there are more trace data
    osiWorkEnqueue(d->work, d->work_queue);
}

void osiTraceSetEnable(bool enable)
{
    osiTraceContext_t *d = &gTraceCtx;

    gTraceEnabled = enable;
    if (enable)
        osiWorkEnqueue(d->work, d->work_queue);
}

void osiTraceWaitFinish(void)
{
    osiTraceContext_t *d = &gTraceCtx;
    if (!gTraceEnabled)
        return;

    for (;;)
    {
        if (TAILQ_EMPTY(&d->work_buf_list))
            return;

        osiWorkEnqueue(d->work, d->work_queue);
        osiSemaphoreAcquire(d->finish_sema);
    }
}

void osiTraceSenderInit(osiTraceSender_t sender, osiTraceSender_t blue_screen_sender)
{
    if (sender == NULL)
        sender = prvSendNone;
    if (blue_screen_sender == NULL)
        blue_screen_sender = prvSendNone;

    osiTraceContext_t *d = &gTraceCtx;
    uint32_t critical = osiEnterCritical();
    d->sender = sender;
    d->blue_screen_sender = blue_screen_sender;
    d->work_queue = osiWorkQueueCreate("wq_trace", 1, OSI_PRIORITY_BELOW_NORMAL,
                                       TRACE_THREAD_STACK_BYTES);
    d->work = osiWorkCreate(prvTraceOutput, NULL, d);
    d->timer = osiTimerCreateWork(d->work, d->work_queue);
    d->finish_sema = osiSemaphoreCreate(1, 0);
    osiExitCritical(critical);

    osiWorkEnqueue(d->work, d->work_queue);
}

osiWorkQueue_t *osiTraceWorkQueue(void)
{
    return gTraceCtx.work_queue;
}

void osiTraceBlueScreenEnter(void)
{
    osiTraceContext_t *d = &gTraceCtx;
    d->pending_count = 0;
    gTraceEnabled = true;
}

void osiTraceBlueScreenPoll(void)
{
    osiTraceContext_t *d = &gTraceCtx;

    osiTraceBuf_t *buf = prvFetchBuf();
    if (buf == NULL)
        return;

    d->blue_screen_sender(buf->start, buf->len);
    prvTransferDone(buf);
}
