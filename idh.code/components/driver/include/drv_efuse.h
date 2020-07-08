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

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief open the 8910 efuse module.
 * It should be called before reading or writing efuse.
 */
void drvEfuseOpen(void);

/**
 * @brief close the 8910 efuse module.
 * It should be called when finsh reading or writing efuse.
 */
void drvEfuseClose(void);

/**
 * @brief read the 8910 efuse.
 *
 * @block_index The index of efuse block. The ranges is from 0 to 81.
 * @val The pointer of the data reading form the block.
 *
 * @return
 *      - true          success
 *      - false         fail
 */
bool drvEfuseRead(int32_t block_index, uint32_t *val);

/**
 * @brief write the 8910 efuse.
 *
 * @block_index The index of efuse block. The ranges is from 0 to 81.
 * @val The value will write to the efuse.
 *
 * @return
 *      - true          success
 *      - false         fail
 */
bool drvEfuseWrite(int32_t block_index, uint32_t val);

#ifdef __cplusplus
}
#endif
#endif /* _DRV_EFUSE_H_ */
