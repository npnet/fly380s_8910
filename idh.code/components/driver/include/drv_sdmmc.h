/* Copyright (C) 2019 RDA Technologies Limited and/or its affiliates("RDA").
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

#ifndef _SDMMC_FUNCTION_H__
#define _SDMMC_FUNCTION_H__
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "drv_names.h"

#ifdef __cplusplus
extern "C" {
#endif

// Command 8 things: cf spec Vers.2 section 4.3.13
#define SDMMC_CMD8_CHECK_PATTERN 0xaa
#define SDMMC_CMD8_VOLTAGE_SEL (0x1 << 8)
#define SDMMC_OCR_HCS (1 << 30)
#define SDMMC_OCR_CCS_MASK (1 << 30)

#define SECOND *1000000
// Timeouts for V1
#define SDMMC_CMD_TIMEOUT (10000)
#define SDMMC_RESP_TIMEOUT (10000)
#define SDMMC_TRAN_TIMEOUT (2 SECOND)
#define SDMMC_READ_TIMEOUT (5000)
#define SDMMC_WRITE_TIMEOUT (5000)

#define SDMMC_SDMMC_OCR_TIMEOUT (1 SECOND) // the card is supposed to answer within 1s

#define SDMMC_CSD_VERSION_1 0
#define SDMMC_CSD_VERSION_2 1

struct drvSdmmc;
typedef struct drvSdmmc drvSdmmc_t;

drvSdmmc_t *drvSdmmcCreate(uint32_t name);

/**
 * @brief open SDMMC driver
 *
 * Open the SDMMC driver, done it can read/write data via the SDMMC.
 *
 * @param sdmmc  the SDMMC driver
 * @return
 *      - true  success
 *      - false fail
 */
bool drvSdmmcOpen(drvSdmmc_t *sdmmc);

/**
 * @brief close SDMMC driver
 *
 * Close the SDMMC driver, stop all data transfer.
 * release resource.
 *
 * @param sdmmc the SDMMC driver
 */
void drvSdmmcClose(drvSdmmc_t *sdmmc);

uint32_t drvSdmmcGetBlockNum(drvSdmmc_t *d);

void drvSdmmcDestroy(drvSdmmc_t *sdmmc);

/**
 * @brief Write blocks of data to MMC.
 * This function is used to write blocks of data on the MMC.
 *
 * @param block_number Start Adress  of the SD memory block where the
 * 		data will be written
 * @param buffer Pointer to the block of data to write. Must be aligned
 * 		on a 32 bits boundary.
 * @param size Number of bytes to write. Must be an interger multiple of the
 *      sector size of the card
 * @return
 *      - true      write sucess.
 *      - false     write failed.
 */
bool drvSdmmcWrite(drvSdmmc_t *d, uint32_t block_number, const void *buffer, uint32_t size);

/**
 * @brief Read blocks of data from MMC.
 * This function is used to Read blocks of data on the MMC.
 *
 * @param block_number Start Adress  of the SD memory block where the
 * 		data will be readed
 * @param buffer Pointer to the block of data to read. Must be aligned
 * 		on a 32 bits boundary.
 * @param size Number of bytes to read. Must be an interger multiple of the
 *      sector size of the card
 * @return
 *      - true      read sucess.
 *      - false         read failed.
 */
bool drvSdmmcRead(drvSdmmc_t *d, uint32_t block_number, void *buffer, uint32_t size);

#endif /* _SDMMC_FUNCTION_H_ */
