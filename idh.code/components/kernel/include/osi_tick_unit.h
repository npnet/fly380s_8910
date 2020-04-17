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

#ifndef _OSI_TICK_UNIT_H_
#define _OSI_TICK_UNIT_H_

#include "kernel_config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OSI_TICK32K_TO_SECOND(tick) ((tick) / 32768)
#define OSI_TICK32K_TO_MS(tick) (((tick) * (1000 / 8)) / (32768 / 8))
#define OSI_TICK32K_TO_US(tick) (((tick) * (1000000 / 64)) / (32768 / 64))
#define OSI_SECOND_TO_TICK32K(sec) ((sec)*32768)
#define OSI_MS_TO_TICK32K(ms) (((ms) * (32768 / 8)) / (1000 / 8))
#define OSI_US_TO_TICK32K(us) (((us) * (32768 / 64)) / (1000000 / 64))

#define OSI_TICK16K_TO_SECOND(tick) ((tick) / 16384)
#define OSI_TICK16K_TO_MS(tick) (((tick) * (1000 / 8)) / (16384 / 8))
#define OSI_TICK16K_TO_US(tick) (((tick) * (1000000 / 64)) / (16384 / 64))
#define OSI_SECOND_TO_TICK16K(sec) ((sec)*16384)
#define OSI_MS_TO_TICK16K(ms) (((ms) * (16384 / 8)) / (1000 / 8))
#define OSI_US_TO_TICK16K(us) (((us) * (16384 / 64)) / (1000000 / 64))

#define OSI_TICK2M_TO_SECOND(tick) ((tick) / 2000000)
#define OSI_TICK2M_TO_MS(tick) ((tick) / 2000)
#define OSI_TICK2M_TO_US(tick) ((tick) / 2)
#define OSI_SECOND_TO_TICK2M(sec) ((sec)*2000000)
#define OSI_MS_TO_TICK2M(ms) ((ms)*2000)
#define OSI_US_TO_TICK2M(us) ((us)*2)

#define OSI_TICK13M_TO_SECOND(tick) ((tick) / 13000000)
#define OSI_TICK13M_TO_MS(tick) ((tick) / 13000)
#define OSI_TICK13M_TO_US(tick) ((tick) / 13)
#define OSI_SECOND_TO_TICK13M(sec) ((sec)*13000000)
#define OSI_MS_TO_TICK13M(ms) ((ms)*13000)
#define OSI_US_TO_TICK13M(us) ((us)*13)

#define OSI_TICK1M_TO_SECOND(tick) ((tick) / 1000000)
#define OSI_TICK1M_TO_MS(tick) ((tick) / 1000)
#define OSI_TICK1M_TO_US(tick) ((tick) / 1)
#define OSI_SECOND_TO_TICK1M(sec) ((sec)*1000000)
#define OSI_MS_TO_TICK1M(ms) ((ms)*1000)
#define OSI_US_TO_TICK1M(us) ((us)*1)

#define OSI_TICK32K_TO_TICK2M(tick) (((tick) * (2000000 / 128)) / (32768 / 128))
#define OSI_TICK2M_TO_TICK32K(tick) (((tick) * (32768 / 128)) / (2000000 / 128))

#define OSI_TICK16K_TO_TICK2M(tick) (((tick) * (2000000 / 128)) / (16384 / 128))
#define OSI_TICK2M_TO_TICK16K(tick) (((tick) * (16384 / 128)) / (2000000 / 128))

#define OSI_TICK16K_TO_TICK1M(tick) (((tick) * (1000000 / 64)) / (16384 / 64))
#define OSI_TICK1M_TO_TICK16K(tick) (((tick) * (16384 / 64)) / (1000000 / 64))

#if (CONFIG_KERNEL_HWTICK_FREQ == 16384)
#define OSI_HWTICK_TO_SECOND(tick) OSI_TICK16K_TO_SECOND(tick)
#define OSI_HWTICK_TO_MS(tick) OSI_TICK16K_TO_MS(tick)
#define OSI_HWTICK_TO_US(tick) OSI_TICK16K_TO_US(tick)
#define OSI_SECOND_TO_HWTICK(sec) OSI_SECOND_TO_TICK16K(sec)
#define OSI_MS_TO_HWTICK(ms) OSI_MS_TO_TICK16K(ms)
#define OSI_US_TO_HWTICK(us) OSI_US_TO_TICK16K(us)
#define OSI_HWTICK_TO_16K(tick) (tick)
#define OSI_16K_TO_HWTICK(tick) (tick)
#endif

#if (CONFIG_KERNEL_HWTICK_FREQ == 2000000)
#define OSI_HWTICK_TO_SECOND(tick) OSI_TICK2M_TO_SECOND(tick)
#define OSI_HWTICK_TO_MS(tick) OSI_TICK2M_TO_MS(tick)
#define OSI_HWTICK_TO_US(tick) OSI_TICK2M_TO_US(tick)
#define OSI_SECOND_TO_HWTICK(sec) OSI_SECOND_TO_TICK2M(sec)
#define OSI_MS_TO_HWTICK(ms) OSI_MS_TO_TICK2M(ms)
#define OSI_US_TO_HWTICK(us) OSI_US_TO_TICK2M(us)
#define OSI_HWTICK_TO_16K(tick) OSI_TICK2M_TO_TICK16K(tick)
#define OSI_16K_TO_HWTICK(tick) OSI_TICK16K_TO_TICK2M(tick)
#endif

#if (CONFIG_KERNEL_HWTICK_FREQ == 1000000)
#define OSI_HWTICK_TO_SECOND(tick) OSI_TICK1M_TO_SECOND(tick)
#define OSI_HWTICK_TO_MS(tick) OSI_TICK1M_TO_MS(tick)
#define OSI_HWTICK_TO_US(tick) OSI_TICK1M_TO_US(tick)
#define OSI_SECOND_TO_HWTICK(sec) OSI_SECOND_TO_TICK1M(sec)
#define OSI_MS_TO_HWTICK(ms) OSI_MS_TO_TICK1M(ms)
#define OSI_US_TO_HWTICK(us) OSI_US_TO_TICK1M(us)
#define OSI_HWTICK_TO_16K(tick) OSI_TICK1M_TO_TICK16K(tick)
#define OSI_16K_TO_HWTICK(tick) OSI_TICK16K_TO_TICK1M(tick)
#endif

// trivial helpers to convert various tick and time unit
static inline int64_t osiTick32kToSecond(int64_t tick) { return OSI_TICK32K_TO_SECOND(tick); }
static inline int64_t osiTick32kToMs(int64_t tick) { return OSI_TICK32K_TO_MS(tick); }
static inline int64_t osiTick32kToUs(int64_t tick) { return OSI_TICK32K_TO_US(tick); }
static inline int64_t osiSecondToTick32k(int64_t sec) { return OSI_SECOND_TO_TICK32K(sec); }
static inline int64_t osiMsToTick32k(int64_t ms) { return OSI_MS_TO_TICK32K(ms); }
static inline int64_t osiUsToTick32k(int64_t us) { return OSI_US_TO_TICK32K(us); }

static inline int64_t osiTick16kToSecond(int64_t tick) { return OSI_TICK16K_TO_SECOND(tick); }
static inline int64_t osiTick16kToMs(int64_t tick) { return OSI_TICK16K_TO_MS(tick); }
static inline int64_t osiTick16kToUs(int64_t tick) { return OSI_TICK16K_TO_US(tick); }
static inline int64_t osiSecondToTick16k(int64_t sec) { return OSI_SECOND_TO_TICK16K(sec); }
static inline int64_t osiMsToTick16k(int64_t ms) { return OSI_MS_TO_TICK16K(ms); }
static inline int64_t osiUsToTick16k(int64_t us) { return OSI_US_TO_TICK16K(us); }

static inline int64_t osiTick2mToSecond(int64_t tick) { return OSI_TICK2M_TO_SECOND(tick); }
static inline int64_t osiTick2mToMs(int64_t tick) { return OSI_TICK2M_TO_MS(tick); }
static inline int64_t osiTick2mToUs(int64_t tick) { return OSI_TICK2M_TO_US(tick); }
static inline int64_t osiSecondToTick2m(int64_t sec) { return OSI_SECOND_TO_TICK2M(sec); }
static inline int64_t osiMsToTick2m(int64_t ms) { return OSI_MS_TO_TICK2M(ms); }
static inline int64_t osiUsToTick2m(int64_t us) { return OSI_US_TO_TICK2M(us); }

static inline int64_t osiTick13mToSecond(int64_t tick) { return OSI_TICK13M_TO_SECOND(tick); }
static inline int64_t osiTick13mToMs(int64_t tick) { return OSI_TICK13M_TO_MS(tick); }
static inline int64_t osiTick13mToUs(int64_t tick) { return OSI_TICK13M_TO_US(tick); }
static inline int64_t osiSecondToTick13m(int64_t sec) { return OSI_SECOND_TO_TICK13M(sec); }
static inline int64_t osiMsToTick13m(int64_t ms) { return OSI_MS_TO_TICK13M(ms); }
static inline int64_t osiUsToTick13m(int64_t us) { return OSI_US_TO_TICK13M(us); }

static inline int64_t osiTick1mToSecond(int64_t tick) { return OSI_TICK1M_TO_SECOND(tick); }
static inline int64_t osiTick1mToMs(int64_t tick) { return OSI_TICK1M_TO_MS(tick); }
static inline int64_t osiTick1mToUs(int64_t tick) { return OSI_TICK1M_TO_US(tick); }
static inline int64_t osiSecondToTick1m(int64_t sec) { return OSI_SECOND_TO_TICK1M(sec); }
static inline int64_t osiMsToTick1m(int64_t ms) { return OSI_MS_TO_TICK1M(ms); }
static inline int64_t osiUsToTick1m(int64_t us) { return OSI_US_TO_TICK1M(us); }

static inline int64_t osiTick32kToTick2m(int64_t tick) { return OSI_TICK32K_TO_TICK2M(tick); }
static inline int64_t osiTick2mToTick32k(int64_t tick) { return OSI_TICK2M_TO_TICK32K(tick); }

static inline int64_t osiTick16kToTick2m(int64_t tick) { return OSI_TICK16K_TO_TICK2M(tick); }
static inline int64_t osiTick2mToTick16k(int64_t tick) { return OSI_TICK2M_TO_TICK16K(tick); }

static inline int64_t osiTick16kToTick1m(int64_t tick) { return OSI_TICK16K_TO_TICK1M(tick); }
static inline int64_t osiTick1mToTick16k(int64_t tick) { return OSI_TICK1M_TO_TICK16K(tick); }

#ifdef __cplusplus
}
#endif
#endif
