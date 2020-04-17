/* Copyright (C) 2017 RDA Technologies Limited and/or its affiliates("RDA").
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

#ifndef _BOOT_SECURE_H_
#define _BOOT_SECURE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Check secure boot state.
 *
 * @return
 *      - false secure boot disable .
 *      - true  secure boot enable.
 */
bool checkSecureBootState(void);

/**
 * @brief Check the uimage signature.
 *
 * @header Input the address of the uimage header.
 * @return
 *      - 0 verify ok.
 *      - nonzero verify fail.
 */
int checkUimageSign(void *header);

/**
 * @brief Check the uimage signature.
 *
 * @buf Input the address of the signed data.
 * @len Input the length of the signed data.
 * @return
 *      - 0 verify ok.
 *      - nonzero verify fail.
 */
int checkDataSign(void *buf, uint32_t len);

/**
 * @brief Encrypt the data using aes algorithm.
 *
 * @buf Input the address of the data.
 * @len Input the length of the data.
 * @return
 *      - 0 verify ok.
 *      - nonzero verify fail.
 */
int antiCloneEncryption(void *buf, uint32_t len);
#ifdef __cplusplus
}
#endif

#endif
