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

//#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_INFO

#include "osi_log.h"
#include "osi_api.h"
#include "hwregs.h"
#include <stdlib.h>
#include "image_sensor.h"
#include "drv_camera.h"

static IMAGE_DEV_T ImageSensor;
static SensorOps_t SensorOpsApi = {
    0,
};

extern bool camGc032aReg(SensorOps_t *pSensorOpsCB);
extern bool camGc0310Reg(SensorOps_t *pSensorOpsCB);

static uint8_t g_cam_index = 0xff;
bool (*pCamRegInit[])(SensorOps_t *) =
    {
        camGc032aReg,
        camGc0310Reg,
};

static uint32_t prvCamGetCfgCount(void)
{
    uint8_t count = sizeof(pCamRegInit) / sizeof(uint32_t);
    OSI_LOGI(0, "cam:  _drvCamGetCfgCount count=%d \n", count);
    return count;
}

static SensorOps_t *prvCamLoad(void)
{
    uint32_t count = 0;
    uint32_t i = 0;
    OSI_LOGI(0, "cam: _drvCamLoad \n");
    count = prvCamGetCfgCount();
    //temp fix for i2c open fail sensor not work
    if ((count != 0) && (g_cam_index == 0xff))
    {
        for (i = 0; i < count; i++)
        {
            if (pCamRegInit[i](&SensorOpsApi))
            {
                g_cam_index = i;
                return &SensorOpsApi;
            }
        }
    }
    else if (g_cam_index != 0xff)
    {
        return &SensorOpsApi;
    }
    OSI_LOGI(0, "cam: _drvCamLoad null ! \n");
    return NULL;
}

static bool prvCamSetFrameBuffer(uint16_t *FrameBuf[], uint8_t nBufcounts)
{
    ImageSensor.pSensorInfo->previewdata[0] = NULL;
    ImageSensor.pSensorInfo->previewdata[1] = NULL;
    ImageSensor.pSensorInfo->capturedata = NULL;

    if (nBufcounts > 2)
        return false;
    if (FrameBuf[0] == NULL)
        return false;

    ImageSensor.framebuffer_count = nBufcounts;

    for (uint8_t i = 0; i < nBufcounts; i++)
    {
        ImageSensor.pSensorInfo->previewdata[i] = (uint8_t *)(OSI_ALIGN_UP(FrameBuf[i], CONFIG_CACHE_LINE_SIZE));
    }
    //OSI_ALIGN_UP for ptr addr dma cache line
    ImageSensor.pSensorInfo->capturedata = (uint8_t *)(OSI_ALIGN_UP(FrameBuf[0], CONFIG_CACHE_LINE_SIZE));

    //create semphore for task sync
    ImageSensor.pSensorInfo->cam_sem_capture = osiSemaphoreCreate(1, 0);
    ImageSensor.pSensorInfo->cam_sem_preview = osiSemaphoreCreate(2, 0);

    return true;
}

void drvCamClose(void)
{
    if (ImageSensor.poweron_flag)
    {
        ImageSensor.poweron_flag = false;
        ImageSensor.pSensorOpsApi->cameraClose();
        if (ImageSensor.poweron_flag == false)
        {
            for (uint8_t i = 0; i < ImageSensor.framebuffer_count; i++)
            {
                if (ImageSensor.pFramebuffer[i] != NULL)
                {
                    free(ImageSensor.pFramebuffer[i]);
                }
            }
            ImageSensor.framebuffer_count = 0;

            OSI_LOGI(0, "cam:drvCamClose,osiSemaphoreDelete");
            osiSemaphoreDelete(ImageSensor.pSensorInfo->cam_sem_capture);
            osiSemaphoreDelete(ImageSensor.pSensorInfo->cam_sem_preview);
        }
    }
}

bool drvCamCaptureImage(uint16_t **pFrameBuf)
{
    if (ImageSensor.poweron_flag)
    {
        ImageSensor.pSensorOpsApi->cameraCaptureImage(ImageSensor.nPixcels);
        if (osiSemaphoreTryAcquire(ImageSensor.pSensorInfo->cam_sem_capture, 5000))
        {
            *pFrameBuf = (uint16_t *)ImageSensor.pSensorInfo->capturedata;
            return true;
        }
        else
        {
            OSI_LOGI(0, "cam:drvCamCaptureImage timeout");
            *pFrameBuf = NULL;
            return false;
        }
    }
    else
    {
        *pFrameBuf = NULL;
        return false;
    }
}

bool drvCamStopPreview(void)
{
    OSI_LOGI(0, "cam: drvCamStopPreview");
    if (ImageSensor.poweron_flag)
    {
        ImageSensor.pSensorOpsApi->cameraStopPrev();
        return true;
    }
    else
        return true;
}

void drvCamPreviewQBUF(uint16_t *pFrameBuf)
{
    OSI_LOGD(0, "cam: drvCamPreviewQBUF");
    uint32_t critical = osiEnterCritical();
    ImageSensor.pSensorOpsApi->cameraPrevNotify();
    osiExitCritical(critical);
}

uint16_t *drvCamPreviewDQBUF(void)
{
    uint8_t turn = 0;
    OSI_LOGD(0, "cam: drvCamPreviewDQBUF");
    osiSemaphoreAcquire(ImageSensor.pSensorInfo->cam_sem_preview);
    //get valid bufnumber
    turn = 1 - ImageSensor.pSensorInfo->page_turn;
    OSI_LOGI(0, "cam: drvCamPreviewDataAcquire turn =%d buf=%08x\n", turn, ImageSensor.pSensorInfo->previewdata[turn]);
    return (uint16_t *)ImageSensor.pSensorInfo->previewdata[turn];
}

bool drvCamStartPreview(void)
{
    OSI_LOGI(0, "cam: drvCamStartPreview");
    if (ImageSensor.poweron_flag)
    {
        ImageSensor.pSensorOpsApi->cameraStartPrev(ImageSensor.nPixcels);
        return true;
    }
    else
        return false;
}

bool drvCamPowerOn(void)
{
    OSI_LOGI(0, "cam: drvCamPowerOn");
    if (ImageSensor.poweron_flag)
        return true;
    else
        ImageSensor.poweron_flag = ImageSensor.pSensorOpsApi->cameraOpen();

    if (ImageSensor.poweron_flag == false)
    {
        for (uint8_t i = 0; i < ImageSensor.framebuffer_count; i++)
        {
            if (ImageSensor.pFramebuffer[i] != NULL)
            {
                free(ImageSensor.pFramebuffer[i]);
            }
        }
        osiSemaphoreDelete(ImageSensor.pSensorInfo->cam_sem_capture);
        osiSemaphoreDelete(ImageSensor.pSensorInfo->cam_sem_preview);
    }
    return ImageSensor.poweron_flag;
}

void drvCamGetSensorInfo(CAM_DEV_T *pCamDevice)
{
    OSI_LOGI(0, "cam: drvCamGetSensorInfo");
    if (ImageSensor.pSensorOpsApi != NULL)
    {
        pCamDevice->pNamestr = (char *)ImageSensor.pSensorInfo->name;
        pCamDevice->img_width = ImageSensor.pSensorInfo->spi_pixels_per_line;
        pCamDevice->img_height = ImageSensor.pSensorInfo->spi_pixels_per_column;
        pCamDevice->nPixcels = ImageSensor.nPixcels;
    }
    else
    {
        pCamDevice->pNamestr = " ";
        pCamDevice->img_width = 0;
        pCamDevice->img_height = 0;
        pCamDevice->nPixcels = 0;
        ImageSensor.nPixcels = 0;
    }
}

bool drvCamInit(void)
{
    OSI_LOGI(0, "cam: drvCamInit");
    SensorOps_t *campoint = NULL;
    //find sensor
    if (ImageSensor.poweron_flag)
        return true;
    campoint = prvCamLoad();
    if (!campoint)
        return false;
    ImageSensor.pSensorOpsApi = campoint;
    if (ImageSensor.pSensorOpsApi != NULL)
    {
        //get sensor info
        ImageSensor.pSensorOpsApi->cameraGetSensorInfo(&(ImageSensor.pSensorInfo));
        //calc sensor output data size in bytes
        ImageSensor.nPixcels = ImageSensor.pSensorInfo->spi_pixels_per_line * ImageSensor.pSensorInfo->spi_pixels_per_column * 2;

        //malloc sensor framedata buffer
        ImageSensor.pFramebuffer[0] = NULL;
        ImageSensor.pFramebuffer[1] = NULL;
        //malloc more data size for dma cache line
        uint32_t alloc_size = ImageSensor.nPixcels + CONFIG_CACHE_LINE_SIZE;
        ImageSensor.pFramebuffer[0] = (uint16_t *)calloc(alloc_size, sizeof(uint8_t));
        if (ImageSensor.pFramebuffer[0] == NULL)
        {
            return false;
        }
        ImageSensor.pFramebuffer[1] = (uint16_t *)calloc(alloc_size, sizeof(uint8_t));
        if (ImageSensor.pFramebuffer[1] == NULL)
        {
            free(ImageSensor.pFramebuffer[0]);
            ImageSensor.pFramebuffer[0] = NULL;
            return false;
        }
        //set sensor framebuffer
        prvCamSetFrameBuffer(&(ImageSensor.pFramebuffer[0]), SENSOR_FRAMEBUFFER_NUM);

        return true;
    }
    else
        return false;
}
