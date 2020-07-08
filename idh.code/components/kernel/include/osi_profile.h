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

#ifndef _OSI_PROFILE_H_
#define _OSI_PROFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

void osiProfileInit(void);
void osiProfileCode(unsigned code);
void osiProfileEnter(unsigned code);
void osiProfileExit(unsigned code);

void osiProfileThreadEnter(unsigned id);
void osiProfileThreadExit(unsigned id);
void osiProfileIrqEnter(unsigned id);
void osiProfileIrqExit(unsigned id);

#define CPEXITFLAG 0x8000
#define CPIRQ_START 0x3f00
#define CPIRQ_END 0x3f7f
#define CPTHREAD_START 0x3f80
#define CPTHREAD_END 0x3fdf
#define CPJOB_START 0x3fe0
#define CPJOB_END 0x3ffe

#define PROFCODE_BLUE_SCREEN 0x2f02
#define PROFCODE_PANIC 0x2f58

#define PROFCODE_LIGHT_SLEEP 0x3703
#define PROFCODE_DEEP_SLEEP 0x3702
#define PROFCODE_SUSPEND 0x3704

#define PROFCODE_FLASH_ERASE 0x3705
#define PROFCODE_FLASH_PROGRAM 0x3706

#ifdef __cplusplus
}
#endif
#endif
