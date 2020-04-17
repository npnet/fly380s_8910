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

#ifndef __OSI_CHIP_H__
#define __OSI_CHIP_H__

#include "kernel_config.h"
#include "osi_api.h"
#include "osi_tick_unit.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void osiChipPmReorder(void);

bool osiChipSuspendPermitted(void);
void osiChipLightSleepEnter(void);
void osiChipLightSleepExit(void);
void osiChipSuspend(osiSuspendMode_t mode);
void osiChipResume(osiSuspendMode_t mode, uint32_t source);
void osiChipCpResume(osiSuspendMode_t mode, uint32_t source);

static inline bool osiChipShutdownModeSupported(osiShutdownMode_t mode) { return true; }

static inline int64_t osiChipSecondToTick(int64_t sec) { return OSI_SECOND_TO_HWTICK(sec); }
static inline int64_t osiChipMsToTick(int64_t ms) { return OSI_MS_TO_HWTICK(ms); }
static inline int64_t osiChipUsToTick(int64_t us) { return OSI_US_TO_HWTICK(us); }
static inline int64_t osiChip16kToTick(int64_t tick) { return osiTick16kToTick1m(tick); }
static inline int64_t osiChipTickToSecond(int64_t tick) { return OSI_HWTICK_TO_SECOND(tick); }
static inline int64_t osiChipTickToMs(int64_t tick) { return OSI_HWTICK_TO_MS(tick); }
static inline int64_t osiChipTickToUs(int64_t tick) { return OSI_HWTICK_TO_US(tick); }
static inline int64_t osiChipTickTo16k(uint64_t tick) { return osiTick1mToTick16k(tick); }

#ifdef __cplusplus
}
#endif

#if defined(CONFIG_SOC_8910)
#include "8910/osi_chip_8910.h"
#elif defined(CONFIG_SOC_6760)
#include "6760/osi_chip_6760.h"
#elif defined(CONFIG_SOC_8955)
#include "8955/osi_chip_8955.h"
#elif defined(CONFIG_SOC_8909)
#include "8909/osi_chip_8909.h"
#endif

#endif
