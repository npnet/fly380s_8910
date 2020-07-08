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

#ifndef _DRV_KEYPAD_H_
#define _DRV_KEYPAD_H_

#include <stdint.h>
#include "hal_keypad_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief the key state
 */
typedef enum
{
    KEY_STATE_PRESS = (1 << 0),
    KEY_STATE_RELEASE = (1 << 1),
} keyState_t;

typedef struct
{
    uint8_t debunceTime;
    uint8_t kpItvTime;
    uint8_t kp_in_mask;
    uint8_t kp_out_mask;
} drvKeyPadCfg;

/**
 * @brief function type, key event handler
 */
typedef void (*keyEventCb_t)(keyMap_t key, keyState_t evt, void *p);

/**
 * @brief init the keypad driver
 */
void drvKeypadInit();

/**
 * @brief set key event handler
 *
 * @param evtmsk    the event mask caller care about
 * @param cb        the handler
 * @param ctx       caller context
 */
void drvKeypadSetCB(keyEventCb_t cb, uint32_t mask, void *ctx);

/**
 * @brief get key current state
 *
 * @param key   the key indicator
 * @return
 *      - (-1)              fail
 *      - KEY_STATE_PRESS   the key is pressed
 *      - KEY_STATE_RELEASE the key is released
 */
int drvKeypadState(keyMap_t key);

#ifdef __cplusplus
}
#endif

#endif
