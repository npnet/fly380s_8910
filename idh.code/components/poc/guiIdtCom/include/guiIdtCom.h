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

#ifndef _INCLUDE_GUIIDTCOM_H_
#define _INCLUDE_GUIIDTCOM_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "osi_api.h"
#include "osi_event_hub.h"
#include "osi_generic_list.h"

#include "osi_compiler.h"
#include "poc_config.h"
#include "poc_types.h"
#include "poc_audio_recorder.h"
#include "poc_audio_player.h"
#include "poc_keypad.h"
#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc_lib.h"

#define FREERTOS
#define T_TINY_MODE

#include "IDT.h"

OSI_EXTERN_C_BEGIN



OSI_EXTERN_C_END

#endif
