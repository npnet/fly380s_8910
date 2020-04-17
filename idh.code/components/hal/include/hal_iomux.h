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

#ifndef _HAL_IOMUX_H_
#define _HAL_IOMUX_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hal_config.h"

#if defined(CONFIG_SOC_8910) || defined(CONFIG_SOC_8909) || defined(CONFIG_SOC_8955)
#include "hal_iomux_pindef.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * special value to indicate end of batch request
 */
#define HAL_IOMUX_REQUEST_END (0xffffffff)

/**
 * @brief IOMUX module initialization
 */
void halIomuxInit(void);

/**
 * @brief request pin function
 *
 * Change IOMUX setting. When the \a pinfunc is invalid, it will be ignored
 * silently.
 *
 * @param pinfunc   pin function
 */
void halIomuxRequest(unsigned pinfunc);

/**
 * @brief batch request pin function
 *
 * Change multiple IOMUX settings. The variadic variables should be ended
 * with \a HAL_IOMUX_REQUEST_END.
 *
 * @param pinfunc   pin function
 */
void halIomuxRequestBatch(unsigned pinfunc, ...);

/**
 * @brief reset IOMUX to default value
 *
 * Change IOMUX setting to default to avoid power leakage. If current
 * pin is not the \a pinfunc, do nothing.
 *
 * @param pinfunc   pin function
 */
void halIomuxRelease(unsigned pinfunc);

/**
 * @brief batch reset IOMUX to default value
 *
 * Reset multiple IOMUX settings. The variadic variables should be ended
 * with \a HAL_IOMUX_REQUEST_END.
 *
 * @param pinfunc   pin function
 */
void halIomuxReleaseBatch(unsigned pinfunc, ...);

#ifdef __cplusplus
}
#endif
#endif
