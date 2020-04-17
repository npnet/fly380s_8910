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

#ifndef _HAL_SPI_FLASH_H_
#define _HAL_SPI_FLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief read manufacture/device id
 *
 * \param hwp       hardware register base
 * \return      3 bytes manufacture id and device id
 */
uint32_t halSpiFlashReadId(uintptr_t hwp);

/**
 * \brief write enable
 *
 * \param hwp       hardware register base
 */
void halSpiFlashWriteEnable(uintptr_t hwp);

/**
 * \brief write disable
 *
 * \param hwp       hardware register base
 */
void halSpiFlashWriteDisable(uintptr_t hwp);

/**
 * \brief program suspend
 *
 * \param hwp       hardware register base
 */
void halSpiFlashProgramSuspend(uintptr_t hwp);

/**
 * \brief erase suspend
 *
 * \param hwp       hardware register base
 */
void halSpiFlashEraseSuspend(uintptr_t hwp);

/**
 * \brief program resume
 *
 * \param hwp       hardware register base
 */
void halSpiFlashProgramResume(uintptr_t hwp);

/**
 * \brief erase resume
 *
 * \param hwp       hardware register base
 */
void halSpiFlashEraseResume(uintptr_t hwp);

/**
 * \brief chip erase
 *
 * \param hwp       hardware register base
 */
void halSpiFlashChipErase(uintptr_t hwp);

/**
 * \brief reset enable
 *
 * \param hwp       hardware register base
 */
void halSpiFlashResetEnable(uintptr_t hwp);

/**
 * \brief reset
 *
 * \param hwp       hardware register base
 */
void halSpiFlashReset(uintptr_t hwp);

/**
 * \brief read status register-1
 *
 * \param hwp       hardware register base
 * \return      status register-1
 */
uint8_t halSpiFlashReadStatus(uintptr_t hwp);

/**
 * \brief read status register-2
 *
 * \param hwp       hardware register base
 * \return      status register-2
 */
uint8_t halSpiFlashReadStatus2(uintptr_t hwp);

/**
 * \brief read status register-1 and 2
 *
 * \param hwp       hardware register base
 * \return      status register-1 in LSB and 2
 */
uint16_t halSpiFlashReadStatus12(uintptr_t hwp);

/**
 * \brief write enable for volatile status register
 *
 * \param hwp       hardware register base
 */
void halSpiFlashWriteVolatileStatusEnable(uintptr_t hwp);

/**
 * \brief write status register
 *
 * \param hwp       hardware register base
 * \param status    status register value, register 1 in LSB
 */
void halSpiFlashWriteStatus12(uintptr_t hwp, uint16_t status);

/**
 * \brief page program
 *
 * \p data shouldn't be allocated in flash. \p size should be less than
 * hardware TX fifo size.
 *
 * \param hwp       hardware register base
 * \param offset    flash address
 * \param data      data to be programmed
 * \param size      data size
 */
void halSpiFlashPageProgram(uintptr_t hwp, uint32_t offset, const void *data, uint32_t size);

/**
 * \brief sector erase
 *
 * \p offset should be 4K aligned.
 *
 * \param hwp       hardware register base
 * \param offset    flash address
 */
void halSpiFlashErase4K(uintptr_t hwp, uint32_t offset);

/**
 * \brief 32K block erase
 *
 * \p offset should be 32K aligned.
 *
 * \param hwp       hardware register base
 * \param offset    flash address
 */
void halSpiFlashErase32K(uintptr_t hwp, uint32_t offset);

/**
 * \brief 64K block erase
 *
 * \p offset should be 64K aligned.
 *
 * \param hwp       hardware register base
 * \param offset    flash address
 */
void halSpiFlashErase64K(uintptr_t hwp, uint32_t offset);

/**
 * \brief 4K/32K/64K erase
 *
 * \p size can only be 4K, 32K or 64K. \p offset should be \p size aligned.
 *
 * \param hwp       hardware register base
 * \param offset    flash address
 * \param size      erase size
 */
void halSpiFlashErase(uintptr_t hwp, uint32_t offset, uint32_t size);

/**
 * \brief read unique id number
 *
 * The unique id is 8 bytes.
 *
 * \param hwp       hardware register base
 * \param id        unique id
 */
void halSpiFlashReadUniqueId(uintptr_t hwp, uint8_t uid[8]);

/**
 * \brief read cp id
 *
 * This is only for Giga Device.
 *
 * \param hwp       hardware register base
 * \return      cp id
 */
uint16_t halSpiFlashReadCpId(uintptr_t hwp);

/**
 * \brief deep power down
 *
 * \param hwp       hardware register base
 */
void halSpiFlashDeepPowerDown(uintptr_t hwp);

/**
 * \brief release from deep power down
 *
 * There are delay inside, to make sure flash is accessible at return.
 *
 * \param hwp       hardware register base
 */
void halSpiFlashReleaseDeepPowerDown(uintptr_t hwp);

/**
 * \brief read security register
 *
 * Refer to datasheet about the format of \p address.
 *
 * The maximum \p size is 4. When more bytes are needed, caller should
 * use loop for multiple read.
 *
 * \param hwp       hardware register base
 * \param address   security register address
 * \param data      pointer to output data
 * \param size      read size
 */
void halSpiFlashReadSecurityRegister(uintptr_t hwp, uint32_t address, void *data, uint32_t size);

/**
 * \brief program security register
 *
 * Refer to datasheet about the format of \p address.
 *
 * \p data shouldn't be allocated in flash. \p size should be less than
 * hardware TX fifo size.
 *
 * \param hwp       hardware register base
 * \param address   security register address
 * \param data      data to be programed
 * \param size      program size
 */
void halSpiFlashProgramSecurityRegister(uintptr_t hwp, uint32_t address, const void *data, uint32_t size);

/**
 * \brief erase security register
 *
 * Refer to datasheet about the format of \p address.
 *
 * \param hwp       hardware register base
 * \param address   security register address
 */
void halSpiFlashEraseSecurityRegister(uintptr_t hwp, uint32_t address);

/**
 * \brief read status register and check WIP bit
 *
 * \param hwp       hardware register base
 * \return
 *      - true if WIP is not set in status register
 *      - false WIP is set in status register
 */
bool halSpiFlashIsWipFinished(uintptr_t hwp);

/**
 * \brief wait WIP bit in status register unset
 *
 * \param hwp       hardware register base
 */
void halSpiFlashWaitWipFinish(uintptr_t hwp);

/**
 * \brief status register value to allow range program/erase
 *
 * For a given range to be program/erasse, this will return status register
 * value allowing range program erase and protect as more as possible.
 *
 * Write protection may be different for various flash model. So, \p mid
 * parameter is needed.
 *
 * \param mid       manufacture id and device id
 * \param status    original status register value
 * \param offset    range start to be program/erase
 * \param size      range size to be program/erase
 * \return
 *      - status register value, allow range program/erase, and write protect
 *        as more as possible
 */
uint16_t halSpiFlashStatusWpRange(uint32_t mid, uint16_t status, uint32_t offset, uint32_t size);

/**
 * \brief status register value for protect all
 *
 * Write protection may be different for various flash model. So, \p mid
 * parameter is needed.
 *
 * \param mid       manufacture id and device id
 * \param status    original status register value
 * \return
 *      - status register value, write protect all
 */
uint16_t halSpiFlashStatusWpAll(uint32_t mid, uint16_t status);

/**
 * \brief status register value for protect none
 *
 * Write protection may be different for various flash model. So, \p mid
 * parameter is needed.
 *
 * \param mid       manufacture id and device id
 * \param status    original status register value
 * \return
 *      - status register value, write protect none
 */
uint16_t halSpiFlashStatusWpNone(uint32_t mid, uint16_t status);

/**
 * \brief handling before program/erase
 *
 * Check status register to ensure the range is not write protected, and
 * send write enable command.
 *
 * Write protection may be different for various flash model. So, \p mid
 * parameter is needed.
 *
 * \param hwp       hardware register base
 * \param mid       manufacture id and device id
 * \param offset    range start to be program/erase
 * \param size      range size to be program/erase
 */
void halSpiFlashPrepareEraseProgram(uintptr_t hwp, uint32_t mid, uint32_t offset, uint32_t size);

/**
 * \brief handling after program/erase is completed
 *
 * After program/erase is completed, status register will be set to
 * protect all.
 *
 * Write protection may be different for various flash model. So, \p mid
 * parameter is needed.
 *
 * \param hwp       hardware register base
 * \param mid       manufacture id and device id
 */
void halSpiFlashFinishEraseProgram(uintptr_t hwp, uint32_t mid);

/**
 * \brief check status register
 *
 * Check status register:
 * - QE should be 1
 * - WP ALL
 * - When there are suspend bits, reset
 *
 * It should be called at the begining of boot to make sure flash is
 * in desired status.
 *
 * When \p hwp is 0, the default flash controller hardware register
 * base will be used. It will simplify header include.
 *
 * \param hwp       hardware register base, 0 for default flash
 */
void halSpiFlashStatusCheck(uintptr_t hwp);

#ifdef __cplusplus
}
#endif
#endif
