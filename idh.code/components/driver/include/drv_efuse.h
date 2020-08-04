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

#ifndef _DRV_EFUSE_H_
#define _DRV_EFUSE_H_

#include "osi_compiler.h"
#include "hal_efuse.h"
#include <stdbool.h>

OSI_EXTERN_C_BEGIN

#define DRV_EFUSE_ACCESS_END (0xEFEFEFEF)

/**
 * \brief batch read EFUSE
 *
 * \param double_block  access DOUBLE BLOCK or not
 * \param index         efuse block index
 * \param value         room to store result
 * \return
 *      - true on success else false
 */
bool drvEfuseBatchRead(bool double_block, uint32_t index, uint32_t *value, ...);

/**
 * \brief batch write EFUSE
 *
 * \param double_block  access DOUBLE BLOCK or not
 * \param index         efuse block index
 * \param value         value to write
 * \return
 *      - true on success else false
 */
bool drvEfuseBatchWrite(bool double_block, uint32_t index, uint32_t value, ...);

/**
 * \brief read one EFUSE block
 *
 * \param double_block  access DOUBLE BLOCK or not
 * \param index         efuse block index
 * \param value         room to store result
 * \return
 *      - true on success else false
 */
bool drvEfuseRead(bool double_block, uint32_t index, uint32_t *value);

/**
 * \brief write one EFUSE
 *
 * \param double_block  access DOUBLE BLOCK or not
 * \param index         efuse block index
 * \param value         value to write
 * \return
 *      - true on success else false
 */
bool drvEfuseWrite(bool double_block, uint32_t index, uint32_t value);

OSI_EXTERN_C_END

#endif