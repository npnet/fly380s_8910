/* Copyright (C) 2020 RDA Technologies Limited and/or its affiliates("RDA").
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

#ifndef _SRV_KEYPAD_H_
#define _SRV_KEYPAD_H_

#include "osi_compiler.h"
#include "drv_keypad.h"

OSI_EXTERN_C_BEGIN

/**
 * \brief function type, override keypad event
 */
typedef void (*kpdHook_t)(keyMap_t id, keyState_t event, void *ctx);

/**
 * \brief add a hook temporarily override keypad event
 *
 * \param id            key id
 * \param hook_func     the hook function
 * \param ctx           caller context
 * \param cur_pressed   current key state
 * \return
 *      - true on succeed
 *      - false if invalid key id
 */
bool srvKeypadAddHook(keyMap_t id, kpdHook_t hook_func, void *ctx, bool *cur_pressed);

/**
 * keypad service init
 */
void srvKeypadInit();

OSI_EXTERN_C_END

#endif