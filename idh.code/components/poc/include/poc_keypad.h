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

#ifndef _POC_KEYPAD_H_
#define _POC_KEYPAD_H_

#include "osi_compiler.h"
#include "poc_config.h"
#include "drv_keypad.h"
#include "lvgl.h"

#define POC_KEY_PTT
#define POC_KEY_VOLUM_DOWN
#define POC_KEY_VOLUM_UP
#define POC_KEY_ENTER
#define POC_KEY_UP
#define POC_KEY_ESC
#define POC_KEY_GROUP
#define POC_KEY_DOWN
#define POC_KEY_MEMBER

OSI_EXTERN_C_BEGIN

bool pocKeypadHandle(uint32_t id, lv_indev_state_t evt, void *p);

bool pocGetPttKeyState(void);

OSI_EXTERN_C_END

#endif

