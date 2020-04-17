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

#ifndef _FS_MOUNT_H_
#define _FS_MOUNT_H_

#include <stdint.h>
#include <stdbool.h>
#include "osi_compiler.h"

OSI_EXTERN_C_BEGIN

/**
 * \brief global partition information
 *
 * The real data structure is a binary partition table. "unsigned" just
 * indicates that it is 4 bytes aligned.
 */
extern const unsigned gPartInfo;

/**
 * \brief mount all file systems in bootloader
 *
 * \p parti is the partition descriptions.
 *
 * \param parti     partition descriptions
 * \return
 *      - true on success
 *      - false on failed
 */
bool fsMountAllBoot(const void *parti);

/**
 * \brief mount all file systems in FDL
 *
 * \p parti is the partition descriptions. When mount is faile, the block
 * device or file system will be formatted.
 *
 * \param parti     partition descriptions
 * \return
 *      - true on success
 *      - false on failed
 */
bool fsMountAllFdl(const void *parti);

/**
 * \brief mount all file systems in application
 *
 * \p parti is the partition descriptions.
 *
 * \param parti     partition descriptions
 * \return
 *      - true on success
 *      - false on failed
 */
bool fsMountAllApp(const void *parti);

/**
 * \brief format and mount all file systems
 *
 * \p parti is the partition descriptions.
 *
 * It is DANGERUOUS! Only call it absolutely needed. Also, Don't call this
 * in application.
 *
 * \param parti     partition descriptions
 * \return
 *      - true on success
 *      - false on failed
 */
bool fsMountFormatAll(const void *parti);

/**
 * \brief umount all file system and block devices in FDL
 *
 * \p parti is the partition descriptions.
 *
 * \param parti     partition descriptions
 */
void fsUmountAllFdl(void);

/**
 * \brief remount factory file system
 *
 * Refer to \p vfs_remount.
 *
 * \param flags     file system mount flags
 */
void fsRemountFactory(unsigned flags);

/**
 * \brief get the default partition information
 *
 * The default partition information is auto generated at build
 * from the configuration json file.
 *
 * \return      the default partition information
 */
const void *fsMountPartInfo(void);

OSI_EXTERN_C_END
#endif
