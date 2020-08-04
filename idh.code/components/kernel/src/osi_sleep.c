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

#include "osi_log.h"
#include "osi_api.h"
#include "osi_profile.h"
#include "osi_internal.h"
#include "osi_chip.h"
#include "osi_sysnv.h"
#include "osi_byte_buf.h"
#include "cmsis_core.h"
#include "hwregs.h"
#include "hal_chip.h"
#include "drv_rtc.h"
#include "srv_wdt.h"
#include <string.h>
#include <stdlib.h>
#include <sys/queue.h>

#define DEBUG_SUSPEND_MODE OSI_SUSPEND_PM1
#define SUSPEND_MIN_TIME (5)            // ms
#define SUSPEND_WDT_MARGIN_TIME (20000) // 20s
#define PSM_MIN_TIME (150)

#ifdef CONFIG_WDT_ENABLE
#define WDT_ENTER_DEEPSLEEP(reset_ms) srvWdtEnterDeepSleep(reset_ms)
#define WDT_EXIT_DEEPSLEEP() srvWdtExitDeepSleep()
#else
#define WDT_ENTER_DEEPSLEEP(reset_ms)
#define WDT_EXIT_DEEPSLEEP()
#endif

typedef TAILQ_ENTRY(osiPmSource) osiPmSourceIter_t;
typedef TAILQ_HEAD(osiPmSourceHead, osiPmSource) osiPmSourceHead_t;
struct osiPmSource
{
    osiPmSourceIter_t resume_iter;
    osiPmSourceIter_t state_iter;

    uint32_t tag;
    void *cb_ctx;
    bool active;
    osiPmSourceOps_t ops;
};

typedef SLIST_ENTRY(osiShutdownReg) osiShutdownRegIter_t;
typedef SLIST_HEAD(osiShutdownRegHead, osiShutdownReg) osiShutdownRegHead_t;
typedef struct osiShutdownReg
{
    osiShutdownRegIter_t iter;
    osiShutdownCallback_t cb;
    void *ctx;
} osiShutdownReg_t;

typedef struct
{
    bool started;
    osiBootMode_t boot_mode;
    osiPmSourceHead_t resume_list;
    osiPmSourceHead_t active_list;
    osiPmSourceHead_t inactive_list;
    osiPmSourceHead_t prepare_list;
    osiShutdownRegHead_t shutdown_reg_list;

    uint32_t boot_causes;
    uint32_t sleep32k_flags;
} osiPmContext_t;

static osiPmContext_t gOsiPmCtx;

static inline void prvCpuSleep(void)
{
#ifdef CONFIG_CPU_ARM
    osiChipLightSleepEnter();

    __DSB();
    __ISB();
    __WFI();
    osiChipLightSleepExit();
#else
    asm volatile(".align 4\n"
                 ".set noreorder\n"
                 "li $6, 1\n"
                 "sw $6, (%0)\n"
                 "lw $6, (%0)\n"
                 "nop\n"
                 ".set reorder\n"
                 :
                 : "r"(&(hwp_sysIrq->cpu_sleep))
                 : "$6");
#endif
}

static osiPmSource_t *prvFindSourceByTag(osiPmContext_t *d, uint32_t tag)
{
    osiPmSource_t *p;
    TAILQ_FOREACH(p, &d->active_list, state_iter)
    {
        if (p->tag == tag)
            return p;
    }
    TAILQ_FOREACH(p, &d->inactive_list, state_iter)
    {
        if (p->tag == tag)
            return p;
    }
    TAILQ_FOREACH(p, &d->prepare_list, state_iter)
    {
        if (p->tag == tag)
            return p;
    }
    return NULL;
}

void osiPmInit(void)
{
    osiPmContext_t *d = &gOsiPmCtx;
    d->started = false;
    TAILQ_INIT(&d->resume_list);
    TAILQ_INIT(&d->active_list);
    TAILQ_INIT(&d->inactive_list);
    TAILQ_INIT(&d->prepare_list);
    SLIST_INIT(&d->shutdown_reg_list);
}

void osiPmStart(void)
{
    osiPmContext_t *d = &gOsiPmCtx;
    osiChipPmReorder();
    d->started = true;
}

void osiPmStop(void)
{
    osiPmContext_t *d = &gOsiPmCtx;
    d->started = false;
}

osiPmSource_t *osiPmSourceCreate(uint32_t tag, const osiPmSourceOps_t *ops, void *ctx)
{
    uint32_t critical = osiEnterCritical();

    osiPmContext_t *d = &gOsiPmCtx;
    osiPmSource_t *p = prvFindSourceByTag(d, tag);
    if (p != NULL)
    {
        osiExitCritical(critical);
        return p;
    }

    p = (osiPmSource_t *)calloc(1, sizeof(osiPmSource_t));
    if (p == NULL)
    {
        osiExitCritical(critical);
        return NULL;
    }

    p->tag = tag;
    p->cb_ctx = ctx;
    p->active = false;
    if (ops != NULL)
        p->ops = *ops;

    TAILQ_INSERT_TAIL(&d->inactive_list, p, state_iter);
    if (p->ops.suspend != NULL || p->ops.resume != NULL)
        TAILQ_INSERT_TAIL(&d->resume_list, p, resume_iter);

    osiExitCritical(critical);
    return p;
}

void osiPmSourceDelete(osiPmSource_t *ps)
{
    if (ps == NULL)
        return;

    uint32_t critical = osiEnterCritical();

    osiPmContext_t *d = &gOsiPmCtx;
    if (!ps->active)
        TAILQ_REMOVE(&d->inactive_list, ps, state_iter);
    else if (ps->ops.prepare != NULL)
        TAILQ_REMOVE(&d->prepare_list, ps, state_iter);
    else
        TAILQ_REMOVE(&d->active_list, ps, state_iter);

    if (ps->ops.suspend != NULL || ps->ops.resume != NULL)
        TAILQ_REMOVE(&d->resume_list, ps, resume_iter);

    osiExitCritical(critical);
}

void osiPmWakeLock(osiPmSource_t *ps)
{
    if (ps == NULL)
        return;

    uint32_t critical = osiEnterCritical();

    osiPmContext_t *d = &gOsiPmCtx;
    if (!ps->active)
    {
        TAILQ_REMOVE(&d->inactive_list, ps, state_iter);
        ps->active = true;
        if (ps->ops.prepare != NULL)
            TAILQ_INSERT_TAIL(&d->prepare_list, ps, state_iter);
        else
            TAILQ_INSERT_TAIL(&d->active_list, ps, state_iter);
    }

    osiExitCritical(critical);
}

void osiPmWakeUnlock(osiPmSource_t *ps)
{
    if (ps == NULL)
        return;

    uint32_t critical = osiEnterCritical();

    osiPmContext_t *d = &gOsiPmCtx;
    if (ps->active)
    {
        if (ps->ops.prepare != NULL)
            TAILQ_REMOVE(&d->prepare_list, ps, state_iter);
        else
            TAILQ_REMOVE(&d->active_list, ps, state_iter);
        ps->active = false;
        TAILQ_INSERT_TAIL(&d->inactive_list, ps, state_iter);
    }

    osiExitCritical(critical);
}

void osiPmResumeReorder(uint32_t tag_later, uint32_t tag_earlier)
{
    if (tag_later == tag_earlier)
        return;

    uint32_t critical = osiEnterCritical();

    osiPmContext_t *d = &gOsiPmCtx;
    osiPmSource_t *p_earlier = NULL;
    osiPmSource_t *p_later = NULL;
    osiPmSource_t *p;
    TAILQ_FOREACH(p, &d->resume_list, resume_iter)
    {
        if (p->tag == tag_earlier)
        {
            p_earlier = p;
            if (p_later == NULL)
            {
                osiExitCritical(critical);
                return;
            }

            TAILQ_REMOVE(&d->resume_list, p_earlier, resume_iter);
            TAILQ_INSERT_BEFORE(p_earlier, p_later, resume_iter);
            osiExitCritical(critical);
            return;
        }

        if (p->tag == tag_later)
        {
            p_later = p;
            if (p_earlier != NULL)
            {
                osiExitCritical(critical);
                return;
            }
        }
    }

    osiExitCritical(critical);
    return;
}

void osiPmResumeFirst(uint32_t tag)
{
    uint32_t critical = osiEnterCritical();

    osiPmContext_t *d = &gOsiPmCtx;
    osiPmSource_t *p;
    TAILQ_FOREACH(p, &d->resume_list, resume_iter)
    {
        if (p->tag == tag)
        {
            TAILQ_REMOVE(&d->resume_list, p, resume_iter);
            TAILQ_INSERT_HEAD(&d->resume_list, p, resume_iter);
            break;
        }
    }

    osiExitCritical(critical);
    return;
}

int osiPmSourceDump(void *mem, int count)
{
    uint32_t critical = osiEnterCritical();
    osiPmContext_t *d = &gOsiPmCtx;

    uint8_t *pmem_head = (uint8_t *)mem;
    int prepare_count = 0;
    int active_count = 0;
    int inactive_count = 0;
    int resume_count = 0;

    osiPmSource_t *p;
    TAILQ_FOREACH(p, &d->prepare_list, state_iter)
    {
        prepare_count++;
    }
    TAILQ_FOREACH(p, &d->active_list, state_iter)
    {
        active_count++;
    }
    TAILQ_FOREACH(p, &d->inactive_list, state_iter)
    {
        inactive_count++;
    }
    TAILQ_FOREACH(p, &d->resume_list, resume_iter)
    {
        resume_count++;
    }

    int total = prepare_count + active_count + inactive_count + resume_count;
    if (total > count)
    {
        osiExitCritical(critical);
        return -1;
    }

    uint8_t *pmem = pmem_head + 4;
    TAILQ_FOREACH(p, &d->prepare_list, state_iter)
    {
        OSI_STRM_WLE32(pmem, p->tag);
    }
    TAILQ_FOREACH(p, &d->active_list, state_iter)
    {
        OSI_STRM_WLE32(pmem, p->tag);
    }
    TAILQ_FOREACH(p, &d->inactive_list, state_iter)
    {
        OSI_STRM_WLE32(pmem, p->tag);
    }
    TAILQ_FOREACH(p, &d->resume_list, resume_iter)
    {
        OSI_STRM_WLE32(pmem, p->tag);
    }

    osiExitCritical(critical);

    OSI_STRM_W8(pmem_head, prepare_count);
    OSI_STRM_W8(pmem_head, active_count);
    OSI_STRM_W8(pmem_head, inactive_count);
    OSI_STRM_W8(pmem_head, resume_count);
    return 4 + total * 4;
}

static bool prvSuspendPermitted(osiPmContext_t *d)
{
    if (!d->started)
        return false;

    if (!TAILQ_EMPTY(&d->active_list))
        return false;

    if (!osiChipSuspendPermitted())
        return false;

    bool prepare_ok = true;
    osiPmSource_t *p;
    TAILQ_FOREACH(p, &d->prepare_list, state_iter)
    {
        if (p->ops.prepare == NULL)
            osiPanic();

        if (!p->ops.prepare(p->cb_ctx))
        {
            OSI_LOGD(0, "prepare cb %4c failed", p->tag);
            prepare_ok = false;
            break;
        }
        OSI_LOGD(0, "prepare cb %4c ok", p->tag);
    }

    if (prepare_ok)
        return true;

    TAILQ_FOREACH_REVERSE_FROM(p, &d->prepare_list, osiPmSourceHead, state_iter)
    {
        if (p->ops.prepare_abort != NULL)
        {
            OSI_LOGD(0, "prepare abort cb %4c", p->tag);
            p->ops.prepare_abort(p->cb_ctx);
        }
    }
    return false;
}

static bool prv32KSleepPermitted(osiPmContext_t *d)
{
    if (!d->started)
        return false;

    if (!TAILQ_EMPTY(&d->active_list))
        return false;

    if (!osiChipSuspendPermitted())
        return false;

    return true;
}

static void prvSuspend(osiPmContext_t *d, osiSuspendMode_t mode, int64_t sleep_ms)
{
    // when PSM sleep condition isn't satisfied, it will return harmless
    osiShutdown(OSI_SHUTDOWN_PSM_SLEEP);

    OSI_LOGD(0, "deep sleep mode/%d sleep/%u", mode, (unsigned)sleep_ms);

    osiPmSource_t *p;
    TAILQ_FOREACH_REVERSE(p, &d->resume_list, osiPmSourceHead, resume_iter)
    {
        if (p->ops.suspend != NULL)
        {
            OSI_LOGD(0, "suspend cb %4c", p->tag);
            p->ops.suspend(p->cb_ctx, mode);
        }
    }

    WDT_ENTER_DEEPSLEEP(OSI_MIN(int64_t, osiCpDeepSleepTime(), sleep_ms) + SUSPEND_WDT_MARGIN_TIME);
    osiProfileEnter(PROFCODE_DEEP_SLEEP);

    osiChipSuspend(mode);

    osiProfileExit(PROFCODE_DEEP_SLEEP);

    uint32_t source = osiPmCpuSuspend(mode, sleep_ms);
    OSI_LOGI(0, "suspend resume source 0x%08x", source);

    osiChipResume(mode, source);
    WDT_EXIT_DEEPSLEEP();

    TAILQ_FOREACH(p, &d->resume_list, resume_iter)
    {
        if (p->ops.resume != NULL)
        {
            OSI_LOGD(0, "resume cb %4c", p->tag);
            p->ops.resume(p->cb_ctx, mode, source);
        }
    }
    osiChipCpResume(mode, source);

    osiTimerWakeupProcess();
}

static void prv32KSleep(osiPmContext_t *d, int64_t sleep_ms)
{
    OSI_LOGI(0, "32K sleep sleep/%u", (unsigned)sleep_ms);

    osiProfileEnter(PROFCODE_DEEP_SLEEP);

    WDT_ENTER_DEEPSLEEP(OSI_MIN(int64_t, osiCpDeepSleepTime(), sleep_ms) + SUSPEND_WDT_MARGIN_TIME);
    uint32_t source = osiChip32KSleep(sleep_ms);
    OSI_LOGI(0, "suspend resume source 0x%08x", source);

    WDT_EXIT_DEEPSLEEP();
    osiProfileExit(PROFCODE_DEEP_SLEEP);
}

static void prvLightSleep(osiPmContext_t *d, uint32_t idle_tick)
{
    bool timer_moved = osiTimerLightSleep(idle_tick);

    osiProfileEnter(PROFCODE_LIGHT_SLEEP);
    prvCpuSleep();
    osiProfileExit(PROFCODE_LIGHT_SLEEP);

    if (timer_moved)
        osiTimerWakeupProcess();
}

static void prvSleepLocked(uint32_t idle_tick)
{
    osiPmContext_t *d = &gOsiPmCtx;
    osiSuspendMode_t mode = DEBUG_SUSPEND_MODE;

    if (osiIsSleepAbort())
        return;

    if (gOsiPmCtx.sleep32k_flags != 0)
    {
        if (prv32KSleepPermitted(d))
        {
            int64_t deep_sleep_ms = osiTimerDeepSleepTime(idle_tick);
            if (deep_sleep_ms > SUSPEND_MIN_TIME)
            {
                prv32KSleep(d, deep_sleep_ms);
                return;
            }
        }
    }
    else
    {
        if (prvSuspendPermitted(d))
        {
            int64_t deep_sleep_ms = osiTimerDeepSleepTime(idle_tick);
            if (deep_sleep_ms > SUSPEND_MIN_TIME)
            {
                prvSuspend(d, mode, deep_sleep_ms);
                return;
            }
        }
    }

    prvLightSleep(d, idle_tick);
}

void osiPmSleep(uint32_t idle_tick)
{
    uint32_t critical = osiEnterCritical();
    prvSleepLocked(idle_tick);
    osiExitCritical(critical);
}

bool osiRegisterShutdownCallback(osiShutdownCallback_t cb, void *ctx)
{
    if (cb == NULL)
        return false;

    uint32_t critical = osiEnterCritical();
    osiPmContext_t *d = &gOsiPmCtx;

    osiShutdownReg_t *reg;
    SLIST_FOREACH(reg, &d->shutdown_reg_list, iter)
    {
        if (reg->cb == cb && reg->ctx == ctx)
        {
            osiExitCritical(critical);
            return false;
        }
    }

    reg = (osiShutdownReg_t *)malloc(sizeof(osiShutdownReg_t));
    if (reg == NULL)
    {
        osiExitCritical(critical);
        return false;
    }

    reg->cb = cb;
    reg->ctx = ctx;
    SLIST_INSERT_HEAD(&d->shutdown_reg_list, reg, iter);
    osiExitCritical(critical);
    return true;
}

bool osiUnregisterShutdownCallback(osiShutdownCallback_t cb, void *ctx)
{
    uint32_t critical = osiEnterCritical();
    osiPmContext_t *d = &gOsiPmCtx;

    osiShutdownReg_t *reg = SLIST_FIRST(&d->shutdown_reg_list);
    if (reg != NULL)
    {
        if (reg->cb == cb && reg->ctx == ctx)
        {
            SLIST_REMOVE_HEAD(&d->shutdown_reg_list, iter);
            osiExitCritical(critical);
            return true;
        }

        osiShutdownReg_t *preg = reg;
        reg = SLIST_NEXT(reg, iter);
        SLIST_FOREACH(reg, &d->shutdown_reg_list, iter)
        {
            if (reg->cb == cb && reg->ctx == ctx)
            {
                SLIST_REMOVE_AFTER(preg, iter);
                osiExitCritical(critical);
                return true;
            }
            preg = reg;
        }
    }

    osiExitCritical(critical);
    return false;
}

static void prvInvokeShutdownCb(osiPmContext_t *d, osiShutdownMode_t mode)
{
    osiShutdownReg_t *reg;
    SLIST_FOREACH(reg, &d->shutdown_reg_list, iter)
    {
        reg->cb(reg->ctx, mode);
    }
}

bool osiShutdown(osiShutdownMode_t mode)
{
    if (mode == OSI_SHUTDOWN_PSM_SLEEP && !gSysnvPsmSleepEnabled)
        return false;

    uint32_t critical = osiEnterCritical();

    if (!osiChipShutdownModeSupported(mode))
    {
        osiExitCritical(critical);
        return false;
    }

    int64_t psm_wake_uptime = osiGetPsmWakeUpTime();
    if (mode == OSI_SHUTDOWN_PSM_SLEEP && psm_wake_uptime == INT64_MAX)
    {
        // Though it should be checked before calling this, check it again
        osiExitCritical(critical);
        return false;
    }

    int64_t wake_uptime = INT64_MAX;
    if (mode == OSI_SHUTDOWN_PSM_SLEEP || mode == OSI_SHUTDOWN_POWER_OFF)
    {
        int64_t rtc_epoch = drvRtcGetWakeupTime();
        if (rtc_epoch != INT64_MAX)
        {
            int64_t rtc_uptime = osiEpochToUpTime(rtc_epoch);
            wake_uptime = OSI_MIN(int64_t, wake_uptime, rtc_uptime);
        }
    }

    if (mode == OSI_SHUTDOWN_PSM_SLEEP)
    {
        wake_uptime = OSI_MIN(int64_t, wake_uptime, psm_wake_uptime);

        // timer shall only be considered in PSM sleep
        int64_t timer_uptime = osiTimerPsmWakeUpTime();
        wake_uptime = OSI_MIN(int64_t, wake_uptime, timer_uptime);
    }

    int64_t sleep_time = wake_uptime - osiUpTime();
    if (sleep_time < PSM_MIN_TIME)
    {
        osiExitCritical(critical);
        return false;
    }

    osiPsmSavePrepare(mode);

    osiPmContext_t *d = &gOsiPmCtx;
    prvInvokeShutdownCb(d, mode);

    osiPsmSave(mode);
    halShutdown(mode, wake_uptime);
}

uint32_t osiGetBootCauses(void)
{
    return gOsiPmCtx.boot_causes;
}

void osiSetBootCause(osiBootCause_t cause)
{
    gOsiPmCtx.boot_causes |= cause;
}

void osiClearBootCause(osiBootCause_t cause)
{
    gOsiPmCtx.boot_causes &= ~cause;
}

osiBootMode_t osiGetBootMode(void)
{
    return gOsiPmCtx.boot_mode;
}

void osiSetBootMode(osiBootMode_t mode)
{
    gOsiPmCtx.boot_mode = mode;
}

void osiSet32KSleepFlag(uint32_t flag)
{
    gOsiPmCtx.sleep32k_flags |= flag;
}

void osiClear32KSleepFlag(uint32_t flag)
{
    gOsiPmCtx.sleep32k_flags &= ~flag;
}
