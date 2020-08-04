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
#include "hal_chip.h"
#include "image_sensor.h"

#define camd_assert(b)  \
    do                  \
    {                   \
        if (!(b))       \
            osiPanic(); \
    } while (0)
#define MAX_TEST_FRAME_COUNTS (30)

typedef struct
{
    CAM_DEV_T CamDev;
} camExample_t;

static bool prvCamInit(camExample_t *cam);
#ifndef CONFIG_CAMERA_SINGLE_BUFFER
static void prvCamContinuousFrameModeTest(camExample_t *cam)
{
    uint8_t *pCamPreviewDataBuffer = NULL;
    if (!prvCamInit(cam))
    {
        OSI_LOGE(0, "cam prvCamContinuousFrameModeTest fail");
        return;
    }
    drvCamStartPreview();

    for (unsigned i = 0; i < MAX_TEST_FRAME_COUNTS; i++)
    {
        pCamPreviewDataBuffer = (uint8_t *)drvCamPreviewDQBUF();
        // deal data
        OSI_LOGXI(OSI_LOGPAR_S, 0, "%s deal data start", __func__);
        memset(pCamPreviewDataBuffer, 0, cam->CamDev.nPixcels);
        OSI_LOGXI(OSI_LOGPAR_S, 0, "%s deal data stop", __func__);

        drvCamPreviewQBUF((uint16_t *)pCamPreviewDataBuffer);
    }

    drvCamStopPreview();
    osiThreadSleep(2);

    drvCamClose();
}
#endif
static void prvCamOneShotFrameModeTest(camExample_t *cam)
{
    uint16_t *pCamPreviewDataBuffer = NULL;
    if (!prvCamInit(cam))
    {
        OSI_LOGE(0, "cam prvCamOneShotFrameModeTest fail");
        return;
    }
    for (unsigned i = 0; i < MAX_TEST_FRAME_COUNTS; i++)
    {
        if (drvCamCaptureImage(&pCamPreviewDataBuffer))
        {
            memset(pCamPreviewDataBuffer, 0, cam->CamDev.nPixcels);
        }
    }

    osiThreadSleep(2);

    drvCamClose();
}

static bool prvCamInit(camExample_t *cam)
{
    char *pSensorName = NULL;
    uint16_t sensor_image_width, sensor_image_height;

    // find sensor
    if (drvCamInit() == false)
    {
        OSI_LOGE(0, "cam drvCamInit error");
        return false;
    }
    OSI_LOGI(OSI_LOGPAR_S, 0, "drvCamInit ok");
    // get sensor info
    drvCamGetSensorInfo(&cam->CamDev);
    pSensorName = (char *)cam->CamDev.pNamestr;
    sensor_image_width = cam->CamDev.img_width;
    sensor_image_height = cam->CamDev.img_height;
    OSI_LOGXI(OSI_LOGPAR_SII, 0, "Find ImgSensor:%s W%dxH%d", pSensorName, sensor_image_width, sensor_image_height);

    // poweron sensor
    if (drvCamPowerOn() == false)
    {
        OSI_LOGE(OSI_LOGPAR_S, 0, "drvCamPowerOn fail");
        return false;
    }
    return true;
}

static void prvCamThread(void *param)
{
    camExample_t cam;
    using testcase_t = void (*)(camExample_t *);
    testcase_t testlist[] = {
#ifndef CONFIG_CAMERA_SINGLE_BUFFER
        prvCamContinuousFrameModeTest,
#endif
        prvCamOneShotFrameModeTest,
    };

    for (unsigned i = 0;; ++i)
    {
        testlist[i % OSI_ARRAY_SIZE(testlist)](&cam);
        osiThreadSleep(5000);
    }

    osiThreadExit();
}

OSI_EXTERN_C int appimg_enter(void *param)
{
    OSI_LOGI(0, "Camera capture example entry");
    osiThreadCreate("cam-example", prvCamThread, NULL, OSI_PRIORITY_LOW, 1024, 1);
    return 0;
}

OSI_EXTERN_C void appimg_exit(void *param)
{
    OSI_LOGI(0, "Camera capture example exit");
}
