/* Copyright (C) 2017 RDA Technologies Limited and/or its affiliates("RDA").
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

#ifndef _BOOT_PLATFORM_H_
#define _BOOT_PLATFORM_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "boot_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bootResetMode
{
    BOOT_RESET_NORMAL,         ///< normal reset
    BOOT_RESET_FORCE_DOWNLOAD, ///< reset to force download mode
} bootResetMode_t;

void bootPanic(void);
void bootDelayUS(uint32_t us);
void bootDebugEvent(uint32_t event);
uint32_t bootHWTick16K(void);
void bootReset(bootResetMode_t mode);
void bootPowerOff(void);
bool bootIsFromPsmSleep(void);
int bootPowerOnCause(void);

#ifdef __cplusplus
}
#endif
#endif
