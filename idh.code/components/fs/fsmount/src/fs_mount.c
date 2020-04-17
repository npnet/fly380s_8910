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

#include "osi_log.h"
#include "osi_api.h"
#include "fs_mount.h"
#include "drv_spi_flash.h"
#include "flash_block_device.h"
#include "partition_block_device.h"
#include "sffs_vfs.h"
#include "vfs.h"
#include "hal_config.h"
#include "calclib/crc32.h"
#include <string.h>
#include <stdlib.h>
#include <sys/queue.h>

// #define MOUNT_FAILED OSI_DO_WHILE0(osiDebugEvent(__LINE__); osiPanic();)
#define MOUNT_FAILED return false

#define PARTINFO_SIZE_MAX (64 * 1024)
#define HEADER_VERSION (0x100)
#define HEADER_MAGIC OSI_MAKE_TAG('P', 'R', 'T', 'I')
#define TYPE_FBDEV1 OSI_MAKE_TAG('F', 'B', 'D', '1')
#define TYPE_FBDEV2 OSI_MAKE_TAG('F', 'B', 'D', '2')
#define TYPE_SUBPART OSI_MAKE_TAG('B', 'P', 'R', 'T')
#define TYPE_SFFS OSI_MAKE_TAG('S', 'F', 'F', 'S')

enum
{
    PARTINFO_BOOTLOADER_RO = (1 << 0),
    PARTINFO_APPLICATION_RO = (1 << 1),
    PARTINFO_BOOTLOADER_IGNORE = (1 << 2),
    PARTINFO_APPLICATION_IGNORE = (1 << 3)
};

typedef struct
{
    uint32_t magic;
    uint32_t crc;
    uint32_t size;
    uint32_t version;
} descHeader_t;

typedef struct
{
    uint32_t type;
    uint32_t flash;
    uint32_t name;
    uint32_t offset;
    uint32_t size;
    uint32_t eb_size;
    uint32_t pb_size;
    uint32_t flags;
} descFbdev_t;

typedef struct
{
    uint32_t type;
    uint32_t device;
    uint32_t name;
    uint32_t offset;
    uint32_t count;
    uint32_t flags;
} descSubpart_t;

typedef struct
{
    uint32_t type;
    uint32_t device;
    char mount[64];
    uint32_t reserve_block;
    uint32_t flags;
} descSffs_t;

typedef SLIST_ENTRY(fsBdevDesc) fsBdevDescIter_t;
typedef SLIST_HEAD(fsBdevDescHead, fsBdevDesc) fsBdevDescHead_t;
typedef struct fsBdevDesc
{
    fsBdevDescIter_t iter;
    uint32_t type;
    uint32_t name;
    blockDevice_t *bdev;
} fsBdevDesc_t;

static fsBdevDescHead_t gBdevList = SLIST_HEAD_INITIALIZER(gBdevList);

static bool prvAddBlockDevice(uint32_t type, uint32_t name, blockDevice_t *bdev)
{
    fsBdevDesc_t *desc = calloc(1, sizeof(fsBdevDesc_t));
    if (desc == NULL)
        return false;

    desc->type = type;
    desc->name = name;
    desc->bdev = bdev;
    SLIST_INSERT_HEAD(&gBdevList, desc, iter);
    return true;
}

static blockDevice_t *prvFindBlockDevice(uint32_t name)
{
    fsBdevDesc_t *desc;
    SLIST_FOREACH(desc, &gBdevList, iter)
    {
        if (desc->name == name)
            return desc->bdev;
    }

    return NULL;
}

// There will only exist one variation for each elf. The code size will be
// smaller if inlined.
static OSI_FORCE_INLINE bool prvMountAll(const void *parti, unsigned ro_mask,
                                         unsigned ignore_mask, bool format_on_fail,
                                         bool force_format)
{
    if (parti == NULL || !OSI_IS_ALIGNED(parti, 4))
    {
        OSI_LOGE(0x10006ff5, "invalid partition description address");
        MOUNT_FAILED;
    }

    uintptr_t ptr = (uintptr_t)parti;
    descHeader_t *header = (descHeader_t *)OSI_PTR_INCR_POST(ptr, sizeof(descHeader_t));
    if (header->magic != HEADER_MAGIC ||
        header->version != HEADER_VERSION ||
        header->size > PARTINFO_SIZE_MAX)
    {
        OSI_LOGE(0x10006ff6, "invalid partition description header");
        MOUNT_FAILED;
    }

    if (header->crc != crc32Calc((const char *)parti + 8, header->size - 8))
    {
        OSI_LOGE(0x10006ff7, "invalid partition description crc");
        MOUNT_FAILED;
    }

    uintptr_t ptr_end = (uintptr_t)parti + header->size;
    while (ptr < ptr_end)
    {
        uint32_t type = *(uint32_t *)ptr;
        if (type == TYPE_FBDEV1)
        {
#ifdef CONFIG_FS_FBDEV_V1_SUPPORTED
            descFbdev_t *desc = (descFbdev_t *)OSI_PTR_INCR_POST(ptr, sizeof(descFbdev_t));
            if (ptr > ptr_end)
            {
                OSI_LOGE(0x10006ff8, "invalid partition description exceed end");
                MOUNT_FAILED;
            }

            // Skip it if it is marked as ignored
            if ((desc->flags & ignore_mask) != 0)
                continue;

            drvSpiFlash_t *flash = drvSpiFlashOpen(desc->flash);
            if (flash == NULL)
            {
                OSI_LOGE(0x10006ff9, "failed to open spi flash %4c", desc->flash);
                MOUNT_FAILED;
            }

            if (force_format)
            {
                if (!flashBlockDeviceFormat(
                        flash, desc->offset, desc->size,
                        desc->eb_size, desc->pb_size))
                {
                    OSI_LOGE(0x10006ffc, "failed to format flash block device %4c", desc->name);
                    MOUNT_FAILED;
                }
            }

            blockDevice_t *fbdev = flashBlockDeviceCreate(
                flash, desc->offset, desc->size,
                desc->eb_size, desc->pb_size, false);

            if (fbdev == NULL)
            {
                if (force_format || !format_on_fail)
                {
                    OSI_LOGE(0x10006ffa, "failed to mount flash block device %4c", desc->name);
                    MOUNT_FAILED;
                }
                else
                {
                    OSI_LOGE(0x10006ffb, "failed to mount flash block device %4c, try format", desc->name);

                    if (!flashBlockDeviceFormat(
                            flash, desc->offset, desc->size,
                            desc->eb_size, desc->pb_size))
                    {
                        OSI_LOGE(0x10006ffc, "failed to format flash block device %4c", desc->name);
                        MOUNT_FAILED;
                    }

                    fbdev = flashBlockDeviceCreate(
                        flash, desc->offset, desc->size,
                        desc->eb_size, desc->pb_size, false);
                    if (fbdev == NULL)
                    {
                        OSI_LOGE(0x10006ffa, "failed to mount flash block device %4c after format", desc->name);
                        MOUNT_FAILED;
                    }
                }
            }

            if (!prvAddBlockDevice(desc->type, desc->name, fbdev))
            {
                OSI_LOGE(0x10006ffd, "failed to add block device %4c", desc->name);
                MOUNT_FAILED;
            }
#else
            MOUNT_FAILED;
#endif
        }
        else if (type == TYPE_FBDEV2)
        {
#ifdef CONFIG_FS_FBDEV_V2_SUPPORTED
            descFbdev_t *desc = (descFbdev_t *)OSI_PTR_INCR_POST(ptr, sizeof(descFbdev_t));
            if (ptr > ptr_end)
            {
                OSI_LOGE(0x10006ff8, "invalid partition description exceed end");
                MOUNT_FAILED;
            }

            // Skip it if it is marked as ignored
            if ((desc->flags & ignore_mask) != 0)
                continue;

            drvSpiFlash_t *flash = drvSpiFlashOpen(desc->flash);
            if (flash == NULL)
            {
                OSI_LOGE(0x10006ff9, "failed to open spi flash %4c", desc->flash);
                MOUNT_FAILED;
            }

            if (force_format)
            {
                if (!flashBlockDeviceFormatV2(
                        flash, desc->offset, desc->size,
                        desc->eb_size, desc->pb_size))
                {
                    OSI_LOGE(0x10006ffc, "failed to format flash block device %4c", desc->name);
                    MOUNT_FAILED;
                }
            }

            blockDevice_t *fbdev = flashBlockDeviceCreateV2(
                flash, desc->offset, desc->size,
                desc->eb_size, desc->pb_size, false);

            if (fbdev == NULL)
            {
                if (force_format || !format_on_fail)
                {
                    OSI_LOGE(0x10006ffa, "failed to mount flash block device %4c", desc->name);
                    MOUNT_FAILED;
                }
                else
                {
                    OSI_LOGE(0x10006ffb, "failed to mount flash block device %4c, try format", desc->name);

                    if (!flashBlockDeviceFormatV2(
                            flash, desc->offset, desc->size,
                            desc->eb_size, desc->pb_size))
                    {
                        OSI_LOGE(0x10006ffc, "failed to format flash block device %4c", desc->name);
                        MOUNT_FAILED;
                    }

                    fbdev = flashBlockDeviceCreateV2(
                        flash, desc->offset, desc->size,
                        desc->eb_size, desc->pb_size, false);
                    if (fbdev == NULL)
                    {
                        OSI_LOGE(0x10006ffa, "failed to mount flash block device %4c after format", desc->name);
                        MOUNT_FAILED;
                    }
                }
            }

            if (!prvAddBlockDevice(desc->type, desc->name, fbdev))
            {
                OSI_LOGE(0x10006ffd, "failed to add block device %4c", desc->name);
                MOUNT_FAILED;
            }
#else
            MOUNT_FAILED;
#endif
        }
        else if (type == TYPE_SUBPART)
        {
            descSubpart_t *desc = (descSubpart_t *)OSI_PTR_INCR_POST(ptr, sizeof(descSubpart_t));
            if (ptr > ptr_end)
            {
                OSI_LOGE(0x10006ff8, "invalid partition description exceed end");
                MOUNT_FAILED;
            }

            // Skip it if it is marked as ignored
            if ((desc->flags & ignore_mask) != 0)
                continue;

            blockDevice_t *parent = prvFindBlockDevice(desc->device);
            if (parent == NULL)
            {
                OSI_LOGE(0x10006ffe, "failed to find block device %4c", desc->device);
                MOUNT_FAILED;
            }

            blockDevice_t *bdev = partBlockDeviceCreate(parent, desc->offset, desc->count);
            if (bdev == NULL)
            {
                OSI_LOGE(0x10006fff, "failed to create sub partition %4c", desc->name);
                MOUNT_FAILED;
            }

            if (!prvAddBlockDevice(desc->type, desc->name, bdev))
            {
                OSI_LOGE(0x10006ffd, "failed to add block device %4c", desc->name);
                MOUNT_FAILED;
            }
        }
        else if (type == TYPE_SFFS)
        {
            descSffs_t *desc = (descSffs_t *)OSI_PTR_INCR_POST(ptr, sizeof(descSffs_t));
            if (ptr > ptr_end)
            {
                OSI_LOGE(0x10006ff8, "invalid partition description exceed end");
                MOUNT_FAILED;
            }

            // Skip it if it is marked as ignored
            if ((desc->flags & ignore_mask) != 0)
                continue;

            blockDevice_t *bdev = prvFindBlockDevice(desc->device);
            if (bdev == NULL)
            {
                OSI_LOGE(0x10006ffe, "failed to find block device %4c", desc->device);
                MOUNT_FAILED;
            }

            if (force_format)
            {
                if (sffsVfsMkfs(bdev) != 0)
                {
                    OSI_LOGE(0x10007002, "failed to format");
                    MOUNT_FAILED;
                }
            }

            bool read_only = ((desc->flags & ro_mask) != 0);
            if (sffsVfsMount(desc->mount, bdev, 4, desc->reserve_block, read_only) != 0)
            {
                if (force_format || !format_on_fail)
                {
                    OSI_LOGXE(OSI_LOGPAR_S, 0x10007000, "failed to mount %s", desc->mount);
                    MOUNT_FAILED;
                }
                else
                {
                    OSI_LOGXE(OSI_LOGPAR_S, 0x10007001, "failed to mount %s, try format", desc->mount);

                    if (sffsVfsMkfs(bdev) != 0)
                    {
                        OSI_LOGE(0x10007002, "failed to format");
                        MOUNT_FAILED;
                    }

                    if (sffsVfsMount(desc->mount, bdev, 4, desc->reserve_block, read_only) != 0)
                    {
                        OSI_LOGE(0x10007003, "failed to mount after format");
                        MOUNT_FAILED;
                    }
                }
            }
        }
        else
        {
            OSI_LOGE(0x10007004, "invalid partition description type %4c", type);
            MOUNT_FAILED;
        }
    }

    return true;
}

bool fsMountAllFdl(const void *parti)
{
    return prvMountAll(parti, 0, 0, false, false);
}

bool fsMountAllBoot(const void *parti)
{
    return prvMountAll(parti, PARTINFO_BOOTLOADER_RO, PARTINFO_BOOTLOADER_IGNORE, false, false);
}

bool fsMountAllApp(const void *parti)
{
#ifdef CONFIG_FS_FORMAT_FLASH_ON_MOUNT_FAIL
    bool format = true;
#else
    bool format = false;
#endif
    return prvMountAll(parti, PARTINFO_APPLICATION_RO, PARTINFO_APPLICATION_IGNORE, format, false);
}

bool fsMountFormatAll(const void *parti)
{
    return prvMountAll(parti, 0, 0, false, true);
}

void fsRemountFactory(unsigned flags)
{
#ifdef CONFIG_FS_FACTORY_MOUNT_POINT
    if (vfs_remount(CONFIG_FS_FACTORY_MOUNT_POINT, flags) < 0)
    {
        OSI_LOGXE(OSI_LOGPAR_SI, 0x10007005, "failed to remount %s, flags/%d",
                  CONFIG_FS_FACTORY_MOUNT_POINT, flags);
    }
#endif
}

void fsUmountAllFdl(void)
{
    vfs_umount_all();

    while (!SLIST_EMPTY(&gBdevList))
    {
        fsBdevDesc_t *desc = SLIST_FIRST(&gBdevList);
        SLIST_REMOVE_HEAD(&gBdevList, iter);

        blockDeviceDestroy(desc->bdev);
        free(desc);
    }
}
