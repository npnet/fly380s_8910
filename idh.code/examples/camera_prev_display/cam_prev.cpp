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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('C', 'A', 'M', 'D')

#include <string.h>
#include <stdlib.h>
#include "osi_log.h"
#include "osi_api.h"
#include "osi_compiler.h"
#include "osi_mem.h"
#include "hal_chip.h"
#include "drv_lcd.h"
#include "image_sensor.h"

typedef struct
{
    CAM_DEV_T camdev;
    lcdSpec_t lcddev;
} camprevExample_t;

#define camd_assert(b)  \
    do                  \
    {                   \
        if (!(b))       \
            osiPanic(); \
    } while (0)

static camprevExample_t camPrev;

static void prvBlackPrint(void)
{
    OSI_LOGI(0, "cam LCD display black");

    lcdFrameBuffer_t dataBufferWin;
    lcdDisplay_t lcdRec;
    uint16_t *lcdblack = (uint16_t *)malloc(240 * 320 * 2);
    camd_assert(lcdblack);
    memset(lcdblack, 0, 240 * 320 * 2);

    dataBufferWin.buffer = lcdblack;
    dataBufferWin.colorFormat = LCD_RESOLUTION_RGB565;
    dataBufferWin.keyMaskEnable = false;
    dataBufferWin.region_x = 0;
    dataBufferWin.region_y = 0;
    dataBufferWin.height = camPrev.lcddev.height;
    dataBufferWin.width = camPrev.lcddev.width;
    dataBufferWin.rotation = 0;
    dataBufferWin.widthOriginal = camPrev.lcddev.width;
    dataBufferWin.maskColor = 0;

    lcdRec.x = 0;
    lcdRec.y = 0;
    lcdRec.width = 240;
    lcdRec.height = 320;
    drvLcdBlockTransfer(&dataBufferWin, &lcdRec);
    free(lcdblack);
}

static void prvCameraPrint(void *buff)
{
    lcdFrameBuffer_t dataBufferWin;
    lcdDisplay_t lcdRec;
    uint32_t offset;
    //if the image is yuv422 ,offset = yaddr+(height-1)*width*2
    offset = camPrev.camdev.img_width * (camPrev.camdev.img_height - 1) * 2;
    dataBufferWin.buffer = (uint16_t *)((uint32_t)buff + offset);
    dataBufferWin.colorFormat = LCD_RESOLUTION_YUV422_UYVY;
    dataBufferWin.keyMaskEnable = false;

    dataBufferWin.region_x = 0;
    dataBufferWin.region_y = 0;
    dataBufferWin.height = camPrev.lcddev.height;
    dataBufferWin.width = camPrev.lcddev.width;
    dataBufferWin.widthOriginal = camPrev.camdev.img_width;
    dataBufferWin.rotation = 1;
    dataBufferWin.maskColor = 0;

    lcdRec.x = 0;
    lcdRec.y = 0;
    lcdRec.width = camPrev.lcddev.width;
    lcdRec.height = camPrev.lcddev.height; //because the image is 248*320,if rotation 90 degree ,just can show the 240*240,

    drvLcdBlockTransfer(&dataBufferWin, &lcdRec);
    //osiDelayUS(1000 * 1000 * 3);
}

static void prvLcdInit(void)
{
    OSI_LOGI(0, "cam LCD init and start test");
    halPmuSwitchPower(HAL_POWER_VDD28, true, true);
    halPmuSwitchPower(HAL_POWER_KEYLED, true, true);
    drvLcdInit();
    drvLcdGetLcdInfo(&camPrev.lcddev);
    prvBlackPrint();
    OSI_LOGI(0, "cam lcd read width %d, height %d", camPrev.lcddev.width, camPrev.lcddev.height);
    osiThreadSleep(1000);
    halPmuSwitchPower(HAL_POWER_BACK_LIGHT, true, true);
}

static void prvCamExampleThread(void *param)
{
    uint16_t *pCamPreviewDataBuffer = NULL;
#ifndef CONFIG_CAMERA_SINGLE_BUFFER
    drvCamStartPreview();
    for (;;)
    {
        pCamPreviewDataBuffer = drvCamPreviewDQBUF();
        //deal data
        OSI_LOGI(0, "cam deal data");
        prvCameraPrint(pCamPreviewDataBuffer);
        drvCamPreviewQBUF((uint16_t *)pCamPreviewDataBuffer);
    }
#else
    for (;;)
    {
        if (drvCamCaptureImage(&pCamPreviewDataBuffer))
        {
            OSI_LOGI(0, "cam deal data");
            prvCameraPrint(pCamPreviewDataBuffer);
        }
    }
#endif
    OSI_LOGD(0, "_camPrevTask osiThreadExit");
    osiThreadExit();
}

OSI_EXTERN_C int appimg_enter(void *param)
{
    OSI_LOGI(0, "Camera prev example entry");
    if (drvCamInit() == false)
    {
        OSI_LOGI(0, "Camera init fail,maybe no memory");
        return 0;
    }
    drvCamGetSensorInfo(&camPrev.camdev);
    OSI_LOGI(0, "cam read img_width %d, img_height %d", camPrev.camdev.img_width, camPrev.camdev.img_height);
    if (!drvCamPowerOn())
    {
        OSI_LOGI(0, "Camera Power On fail");
        return 0;
    }
    prvLcdInit();
    osiThreadCreate("cam-prev-example", prvCamExampleThread, NULL, OSI_PRIORITY_LOW, 3072, 1);
    return 0;
}

OSI_EXTERN_C void appimg_exit(void *param)
{
    OSI_LOGI(0, "Camera capture example exit");
}
