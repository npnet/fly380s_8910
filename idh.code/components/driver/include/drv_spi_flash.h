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

#ifndef _DRV_SPI_FLASH_H_
#define _DRV_SPI_FLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "drv_names.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief opaque data structure for SPI flash
 */
typedef struct drvSpiFlash drvSpiFlash_t;

/**
 * @brief open SPI flash
 *
 * SPI flash instance is singleton for each flash. So, for each device name
 * the returned instance pointer is the same. And there are no \a close
 * API.
 *
 * At the first open, the flash will be initialized (for faster speed).
 *
 * @param name      SPI flash device name
 * @return
 *      - SPI flash instance pointer
 *      - NULL if the name is invalid
 */
drvSpiFlash_t *drvSpiFlashOpen(uint32_t name);

/**
 * @brief prohibit flash erase/program for a certain range
 *
 * By default, flash erase/program is not prohibited. If it is known that
 * certain range will never be changed, this can be called. This range will
 * be used to check erase and program parameters. When the erase or program
 * address is located in the range, erase or program API will return false.
 * For example, it the first block (usually it is bootloader) will never be
 * modified:
 *
 * \code{.cpp}
 *      drvSpiFlashSetRangeWriteProhibit(flash, 0, 0x10000);
 * \endcode
 *
 * For multiple non-coninuous ranges, this can be called multple times.
 *
 * Inside, there is a flag for each block (64KB). So, the check unit is 64KB
 * block. Also, \p start will be aligned up, \p end will be aligned down.
 * So, the followings won't set any prohibit.
 *
 * \code{.cpp}
 *      drvSpiFlashSetRangeWriteProhibit(flash, 0x10000, 0x18000);
 *      drvSpiFlashSetRangeWriteProhibit(flash, 0x18000, 0x20000);
 * \endcode
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param start     range start (inclusive)
 * @param end       range end (exclusive)
 */
void drvSpiFlashSetRangeWriteProhibit(drvSpiFlash_t *d, uint32_t start, uint32_t end);

/**
 * @brief clear range write prohibit flag
 *
 * This is not recommended, just for completeness.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param start     range start (inclusive)
 * @param end       range end (exclusive)
 */
void drvSpiFlashClearRangeWriteProhibit(drvSpiFlash_t *d, uint32_t start, uint32_t end);

/**
 * @brief get flash ID
 *
 * The contents of the ID:
 * - id[23:16]: capacity
 * - id[15:8]: memory type
 * - id[7:0]: manufacturer
 *
 * @param d         SPI flash instance pointer, must be valid
 * @return
 *      - flash ID
 */
uint32_t drvSpiFlashGetID(drvSpiFlash_t *d);

/**
 * @brief get flash capacity in byte
 *
 * @param d         SPI flash instance pointer, must be valid
 * @return
 *      - flash capacity in byte
 */
uint32_t drvSpiFlashCapacity(drvSpiFlash_t *d);

/**
 * @brief lock (disable) flash program or erase
 *
 * When there are flash writing in progress, it will wait the current
 * operation finish.
 *
 * @param d         SPI flash instance pointer, must be valid
 */
void drvSpiFlashWriteLock(drvSpiFlash_t *d);

/**
 * @brief unlock (enable) flash program or erase
 *
 * @param d         SPI flash instance pointer, must be valid
 */
void drvSpiFlashWriteUnlock(drvSpiFlash_t *d);

/**
 * @brief map flash offset to accessible address
 *
 * The returned pointer should be accessed as read only.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param offset    flash offset
 * @return
 *      - accessible pointer
 *      - NULL if \a offset is invalid
 */
const void *drvSpiFlashMapAddress(drvSpiFlash_t *d, uint32_t offset);

/**
 * @brief convert mapped address to flash offset
 *
 * When the address is invalid, return -1U.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param address   mapped flash accessible pointer
 * @return
 *      - flash offset
 *      - -1U if \a address is not mapped flash address
 */
uint32_t drvSpiFlashOffset(drvSpiFlash_t *d, const void *address);

/**
 * @brief write to flash
 *
 * This will just use flash PROGRAM command to write. Caller should ensure
 * the write sector or block is erased before. Otherwise, the data on
 * flash may be different from the input data.
 *
 * \a offset and \a size are not needed to be sector or block aligned.
 *
 * It will wait flash write finish. On success return, all data are written
 * to flash.
 *
 * It won't read back to verify the result.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param offset    flash offset
 * @param data      memory for write
 * @param size      write size
 * @return
 *      - true on success
 *      - false on invalid parameters
 */
bool drvSpiFlashWrite(drvSpiFlash_t *d, uint32_t offset, const void *data, size_t size);

/**
 * @brief erase flash sectors or blocks
 *
 * Both \a offset and \a size must be sector (4KB) aligned.
 *
 * When \a size is larger than sector or block, it will loop inside until
 * all requested sector or block are erased.
 *
 * It will wait flash erase finish. On success return, the whole region is
 * erased.
 *
 * It won't read back to verify the result.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param offset    flash offset
 * @param size      erase size
 * @return
 *      - true on success
 *      - false on invalid parameters
 */
bool drvSpiFlashErase(drvSpiFlash_t *d, uint32_t offset, size_t size);

/**
 * @brief erase the whole flash chip
 *
 * In normal case, it shouldn't be used by application.
 *
 * @param d         SPI flash instance pointer, must be valid
 */
void drvSpiFlashChipErase(drvSpiFlash_t *d);

/**
 * @brief read flash status register
 *
 * @param d         SPI flash instance pointer, must be valid
 * @return
 *      - flash status register
 */
uint16_t drvSpiFlashReadStatus(drvSpiFlash_t *d);

/**
 * @brief read flash non-volatile status register
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param status    value to be written to flash status register
 */
void drvSpiFlashWriteStatus(drvSpiFlash_t *d, uint16_t status);

/**
 * @brief read flash volatile status register
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param status    value to be written to flash status register
 */
void drvSpiFlashWriteVolatileStatus(drvSpiFlash_t *d, uint16_t status);

/**
 * @brief enter deep power down mode
 *
 * Enter deep power down mode for power saving. After it is called, flash
 * is inaccessible. Before flash access, \p drvSpiFlashReleaseDeepPowerDown
 * should be called.
 *
 * It **must** be called with interrupt disabled.
 *
 * @param d         SPI flash instance pointer, must be valid
 */
void drvSpiFlashDeepPowerDown(drvSpiFlash_t *d);

/**
 * @brief release from deep power down mode
 *
 * It **must** be called with interrupt disabled.
 *
 * @param d         SPI flash instance pointer, must be valid
 */
void drvSpiFlashReleaseDeepPowerDown(drvSpiFlash_t *d);

/**
 * @brief read unique id number
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param id        8 bytes id
 */
void drvSpiFlashReadUniqueId(drvSpiFlash_t *d, uint8_t id[8]);

/**
 * @brief read cp id
 *
 * This is only for Giga Device.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @return
 *      - cp id
 *      - 0 if not supported
 */
uint16_t drvSpiFlashReadCpId(drvSpiFlash_t *d);

/**
 * @brief read security register
 *
 * There are no reliable method to determine whether there are security
 * registers, and the size of security registers. So, caller should
 * ensure that it is called only on flash with security registers, and
 * \p num and \p address are valid.
 *
 * Inside, it is assumed that there are 3 security registers.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param num       security register number
 * @param address   address inside security register
 * @param data      pointer to output data
 * @param size      read size
 * @return
 *      - true on success
 *      - false on invalid parameter
 */
bool drvSpiFlashReadSecurityRegister(drvSpiFlash_t *d, uint8_t num, uint16_t address, void *data, uint32_t size);

/**
 * @brief program security register
 *
 * Refer to \p drvSpiFlashReadSecurityRegister for \p num and \p address.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param num       security register number
 * @param address   address inside security register
 * @param data      data to be programmed
 * @param size      program size
 * @return
 *      - true on success
 *      - false on invalid parameter
 */
bool drvSpiFlashProgramSecurityRegister(drvSpiFlash_t *d, uint8_t num, uint16_t address, const void *data, uint32_t size);

/**
 * @brief program security register
 *
 * Refer to \p drvSpiFlashReadSecurityRegister for \p num.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param num       security register number
 * @return
 *      - true on success
 *      - false on invalid parameter
 */
bool drvSpiFlashEraseSecurityRegister(drvSpiFlash_t *d, uint8_t num);

/**
 * @brief lock security register
 *
 * Refer to \p drvSpiFlashReadSecurityRegister for \p num.
 *
 * Lock are OTP bits in status register. Once it is locked, there are
 * no ways to erase/program the security register.
 *
 * @param d         SPI flash instance pointer, must be valid
 * @param num       security register number
 * @return
 *      - true on success
 *      - false on invalid parameter
 */
bool drvSpiFlashLockSecurityRegister(drvSpiFlash_t *d, uint8_t num);

#ifdef __cplusplus
}
#endif
#endif
