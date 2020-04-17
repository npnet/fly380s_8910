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
//#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_ERROR

#include "atr_config.h"

#ifdef CONFIG_AT_CAMERA_SUPPORT

#include "osi_log.h"
#include "osi_api.h"
#include "at_cfw.h"
#include "at_command.h"
#include "at_engine.h"
#include "at_response.h"
#include "ats_config.h"
#include "drv_uart.h"
#include "drv_serial.h"
#include "vfs.h"
#include "image_sensor.h"
#include "fs_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/errno.h>

#define PHOTO_BUFF_MAX_SIZE 640 * 480 * 2

#define CAM_USB_SERIAL_NAME (DRV_NAME_USRL_COM2)
#define CAM_RX_BUF_SIZE (512 * 2)
#define CAM_RX_DMA_SIZE (512)
#define CAM_USB_WRITE_TIMEOUT (5000)

#define CAM_THREAD_PRIORITY (OSI_PRIORITY_LOW)
#define CAM_THREAD_STACK_SIZE (8192 * 4)
#define CAM_THREAD_MAX_EVENT (32)

typedef struct
{
    CAM_DEV_T CamDev;
    osiThread_t *camTask;
    bool regstat;
    bool MemoryState;
    bool gCamPowerOnFlag;
    bool issnapshot;
} camastae_t;

camastae_t camAt;
osiMutex_t *g_CamMutex = NULL;

static void atCamStateInit(camastae_t *st)
{
    memset(&st->CamDev, 0, sizeof(CAM_DEV_T));
    st->camTask = NULL;
    st->regstat = false;
    st->MemoryState = true;
    st->gCamPowerOnFlag = false;
    st->issnapshot = false;
    if (NULL == g_CamMutex)
    {
        g_CamMutex = osiMutexCreate();
    }
}

//deal data

__attribute__((unused)) static char *getPictureName(void)
{
    struct timeval tv;
    struct tm ltm;
    static char str[80];
    if (gettimeofday(&tv, NULL) < 0)
    {
        return NULL;
    }
    memset(str, 0x00, sizeof(str));
    time_t lt = tv.tv_sec;
    gmtime_r(&lt, &ltm);
    snprintf(str, sizeof(str), "/%04d%02d%02d%02d%02d%02d_%06ld.yuv",
             ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
             ltm.tm_hour, ltm.tm_min, ltm.tm_sec, tv.tv_usec);
    return str;
}

__attribute__((unused)) static const char *path_add(char *root, char *p)
{
    static char buff_path[256] = "";
    if (root != NULL)
    {
        char *pr = &buff_path[0];
        OSI_LOGD(0, "fattest: pr %c", *pr);
        OSI_LOGD(0, "fattest: root %c", *root);

        while (1)
        {

            (*pr++) = (*root++);
            OSI_LOGD(0, "fattest: pr %c", *(pr - 1));
            OSI_LOGD(0, "fattest: root %c", *(root - 1));

            if (*(pr - 1))
            {
            }
            else
            {
                break;
            }
        }

        if (*(pr - 1) != '/' && *p != '/')
        {
            *(pr - 1) = '/';
            *pr = '\0';
        }
    }
    else
    {
        buff_path[0] = '\0';
    }
    strcat(buff_path, p);
    return buff_path;
}

void atCmdHandleCAM(atCommand_t *cmd)
{
    char resp[80];
    bzero(resp, sizeof(resp));

    if (AT_CMD_TEST == cmd->type)
    {
        sprintf(resp, "%s", "+cam(0-2); 0: Open Camera, 1: capture image, 2: Close Camera");
        atCmdRespInfoText(cmd->engine, resp);
        RETURN_OK(cmd->engine);
    }
    else if (AT_CMD_SET == cmd->type)
    {
        bool paramok = true;
        uint32_t mode = atParamUint(cmd->params[0], &paramok);
        if (!paramok || cmd->param_count > 1)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        if (0 > mode || mode > 3)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        switch (mode)
        {
        case 0:
            if (camAt.regstat == false)
            {
                atCamStateInit(&camAt);
                if (drvCamInit() == false)
                {
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);
                }
                drvCamGetSensorInfo(&camAt.CamDev);
                camAt.regstat = true;
                if (!camAt.gCamPowerOnFlag)
                {
                    if (!drvCamPowerOn())
                    {
                        camAt.regstat = false;
                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
                    }
                    else
                    {
                        camAt.gCamPowerOnFlag = true;

                        sprintf(resp, "+cam(0): Open Camera");
                        atCmdRespInfoText(cmd->engine, resp);
                        RETURN_OK(cmd->engine);
                    }
                }
            }
            else
            {
                sprintf(resp, "+cam(0): camera is already opened");
                atCmdRespInfoText(cmd->engine, resp);
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
            }
            break;
        case 1:
            if (camAt.gCamPowerOnFlag)
            {
                //atCmdRespUrcText(cmd->engine, getPictureName());
#ifdef CONFIG_FS_MOUNT_SDCARD
                int _fd = vfs_open(path_add(CONFIG_FS_SDCARD_MOUNT_POINT, getPictureName()), O_RDWR | O_CREAT | O_TRUNC, 0);
                if (_fd == -1)
                {
                    OSI_LOGE(0, "error numer %d", errno);
                    camAt.MemoryState = false;
                }
                else
                {
                    camAt.MemoryState = true;
                }
                if (camAt.MemoryState)
                {
                    uint16_t *pCamPreviewDataBuffer = NULL;

                    osiMutexLock(g_CamMutex);
                    camAt.issnapshot = true;
                    if (drvCamCaptureImage(&pCamPreviewDataBuffer))
                    {
                        OSI_LOGI(0, "write image in sdcard");
                        ssize_t size = vfs_write(_fd, pCamPreviewDataBuffer, camAt.CamDev.nPixcels);
                        if (size != camAt.CamDev.nPixcels)
                        {
                            OSI_LOGE(0, "wirte size error %d", size);
                        }
                        vfs_close(_fd);
                        OSI_LOGI(0, "write data size %d", size);
                    }
                    else
                    {
                        osiMutexUnlock(g_CamMutex);
                        sprintf(resp, "+cam(1):Capture image error");
                        camAt.issnapshot = false;
                        atCmdRespInfoText(cmd->engine, resp);
                        RETURN_OK(cmd->engine);
                    }
                    camAt.issnapshot = false;
                    osiMutexUnlock(g_CamMutex);

                    sprintf(resp, "+cam(1):Capture an image");
                    atCmdRespInfoText(cmd->engine, resp);
                    RETURN_OK(cmd->engine);
                }
                else
                {
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);
                }
#else
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);
#endif
            }
            else
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
            break;
        case 2:
            if (camAt.gCamPowerOnFlag)
            {
                osiMutexLock(g_CamMutex);
                drvCamClose();
                camAt.gCamPowerOnFlag = false;
                camAt.regstat = false;
                osiMutexUnlock(g_CamMutex);
                sprintf(resp, "+cam(2): Close  the  camera");
                atCmdRespInfoText(cmd->engine, resp);
                RETURN_OK(cmd->engine);
            }
            else
            {
                sprintf(resp, "+cam(2): camera is already closed");
                atCmdRespInfoText(cmd->engine, resp);
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
            }
            break;

        default:
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            break;
        }
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_NOT_SURPORT);
    }
}

#endif
