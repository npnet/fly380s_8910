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

#ifndef _DRV_DEBUGHOST_H_
#define _DRV_DEBUGHOST_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief initialize debughost
 *
 * After initialization, debughost is ready to process host command.
 */
void drvDhostInit(void);

/**
 * \brief restore debughost configuration
 *
 * It is for debug purpose only.
 */
void drvDhostRestoreConfig(void);

/**
 * \brief send all data through debughost
 *
 * When it returns, \p data shouldn't be used by debughost any more.
 * And it is safe for caller to access (read and write) \p data.
 *
 * \param data      data to be sent
 * \param size      data size
 * \return
 *      - true if \p data is sent
 *      - false if output fails
 */
bool drvDhostSendAll(const void *data, unsigned size);

/**
 * \brief debughost enter blue screen mode
 */
void drvDhostBlueScreenEnter(void);

/**
 * \brief send data through debughost in blue screen mode
 *
 * \param data      data to be sent
 * \param size      data size
 * \return
 *      - true if \p data is sent
 *      - false if output fails
 */
bool drvDhostBlueScreenSend(const void *data, unsigned size);

/**
 * \brief polling in blue screen
 */
void drvDhostBlueScreenPoll(void);

#ifdef __cplusplus
}
#endif
#endif
