/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('T', 'B', 'L', 'K')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_INFO

#include "tflash_block_device.h"
#include "drv_sdmmc.h"
#include <sys/errno.h>
#include "string.h"
#include <stdlib.h>
#include "hal_config.h"
#include "osi_compiler.h"
#include "osi_log.h"

typedef struct
{
    blockDevice_t bdev;
    drvSdmmc_t *sdmmc;
    uint32_t max_rdsec_cnt;
    uint32_t max_wrsec_cnt;
    void *rd_buf;
    void *wr_buf;
} tfBdev_t;

#define TCARD_SECTOR_SIZE (512)

static tfBdev_t *prvBdevToTf(blockDevice_t *bdev)
{
    return (tfBdev_t *)bdev->priv;
}

static int prvReadBlocks(tfBdev_t *tf, uint64_t nr, int count, void *buf)
{
    if (count > tf->max_rdsec_cnt)
        count = tf->max_rdsec_cnt;

    void *rdbuf = buf;
    if (!OSI_IS_ALIGNED(buf, CONFIG_CACHE_LINE_SIZE))
        rdbuf = tf->rd_buf;

    bool ret = drvSdmmcRead(tf->sdmmc, (uint32_t)nr, rdbuf, TCARD_SECTOR_SIZE * count);
    if (ret)
    {
        if (rdbuf == tf->rd_buf)
            memcpy(buf, tf->rd_buf, TCARD_SECTOR_SIZE * count);
        return count;
    }
    return -1;
}

static int tflash_read(blockDevice_t *dev, uint64_t nr, int count, void *buf_)
{
    const uint32_t total_count = count;
    tfBdev_t *tf = prvBdevToTf(dev);
    uint8_t *buf = (uint8_t *)buf_;
    while (count > 0)
    {
        int rc = prvReadBlocks(tf, nr, count, buf);
        if (rc == -1)
        {
            OSI_LOGE(0, "read nr %u fail.", (unsigned)nr);
            return -1;
        }

        buf += rc * TCARD_SECTOR_SIZE;
        nr += rc;
        count -= rc;
    }
    return total_count;
}

static int prvWriteBlocks(tfBdev_t *tf, uint64_t nr, int count, const void *data)
{
    if (count > tf->max_wrsec_cnt)
        count = tf->max_wrsec_cnt;

    const void *wrbuf = data;
    if (!OSI_IS_ALIGNED(wrbuf, CONFIG_CACHE_LINE_SIZE))
    {
        memcpy(tf->wr_buf, data, TCARD_SECTOR_SIZE * count);
        wrbuf = tf->wr_buf;
    }

    bool ret = drvSdmmcWrite(tf->sdmmc, (uint32_t)nr, wrbuf, TCARD_SECTOR_SIZE * count);
    return ret ? count : -1;
}

static int tflash_write(blockDevice_t *dev, uint64_t nr, int count, const void *data_)
{
    tfBdev_t *tf = prvBdevToTf(dev);
    const unsigned total_count = count;
    const uint8_t *data = (const uint8_t *)data_;
    while (count > 0)
    {
        int wr = prvWriteBlocks(tf, nr, count, data);
        if (wr < 0)
        {
            OSI_LOGE(0, "write nr %u fail.", (unsigned)nr);
            return -1;
        }
        data += wr * TCARD_SECTOR_SIZE;
        nr += wr;
        count -= wr;
    }
    return total_count;
}

static void tflash_destroy(blockDevice_t *dev)
{
    tfBdev_t *tf = prvBdevToTf(dev);
    drvSdmmcClose(tf->sdmmc);
    drvSdmmcDestroy(tf->sdmmc);
    free(dev);
}

blockDevice_t *tflash_device_create(uint32_t name)
{
    const unsigned rd_max_sector = 64;
    const unsigned wr_max_sector = 64;
    const unsigned rdmaxlen = TCARD_SECTOR_SIZE * rd_max_sector;
    const unsigned wrmaxlen = TCARD_SECTOR_SIZE * wr_max_sector;
    const unsigned alloclen = rdmaxlen + wrmaxlen + sizeof(tfBdev_t) + CONFIG_CACHE_LINE_SIZE;
    tfBdev_t *tf = (tfBdev_t *)malloc(alloclen);
    if (tf == NULL)
    {
        OSI_LOGE(0, "fail to create tblock device (not enough memory)");
        return NULL;
    }

    memset(tf, 0, sizeof(*tf));
    uint8_t *aligned = (uint8_t *)OSI_ALIGN_UP(((uint8_t *)tf + sizeof(tfBdev_t)), CONFIG_CACHE_LINE_SIZE);
    tf->rd_buf = (void *)aligned;
    tf->wr_buf = (void *)(aligned + rdmaxlen);

    tf->sdmmc = drvSdmmcCreate(name);
    if (tf->sdmmc == NULL)
    {
        OSI_LOGE(0, "fail to create %4c sdmmc instance", name);
        goto fail;
    }

    bool ret = drvSdmmcOpen(tf->sdmmc);
    if (!ret)
    {
        OSI_LOGE(0, "fail to open sdmmc");
        goto fail_open;
    }

    tf->max_rdsec_cnt = rd_max_sector;
    tf->max_wrsec_cnt = wr_max_sector;
    tf->bdev.block_count = drvSdmmcGetBlockNum(tf->sdmmc);
    tf->bdev.block_size = TCARD_SECTOR_SIZE;
    tf->bdev.ops.read = tflash_read;
    tf->bdev.ops.write = tflash_write;
    tf->bdev.ops.destroy = tflash_destroy;
    tf->bdev.priv = tf;
    return &tf->bdev;

fail_open:
    drvSdmmcDestroy(tf->sdmmc);

fail:
    free(tf);
    return NULL;
}
