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
#include "hal_chip.h"
#include "drv_i2c.h"
#include "image_sensor.h"
#include "drv_camera.h"
#include "drv_names.h"
#include "osi_clock.h"

static osiClockConstrainRegistry_t gcCamCLK = {.tag = HAL_NAME_CAM};

const cameraReg_t RG_InitVgaSPI[] =
    {
        /*System*/
        {0xf3, 0x83}, //ff//1f//01 data output
        {0xf5, 0x08},
        {0xf7, 0x01},
        {0xf8, 0x04}, ////pll-div----frank
        {0xf9, 0x4e},
        {0xfa, 0x00},
        {0xfc, 0x02},
        {0xfe, 0x02},
        {0x81, 0x03},

        {0xfe, 0x00},
        {0x77, 0x64},
        {0x78, 0x40},
        {0x79, 0x60},
        /*Analog&Cisctl*/
        {0xfe, 0x00},
        {0x03, 0x01},
        {0x04, 0xcb},
        {0x05, 0x01},
        {0x06, 0xb2},
        {0x07, 0x00},
        {0x08, 0x10},

        {0x0a, 0x00},
        {0x0c, 0x00},
        {0x0d, 0x01},
        {0x0e, 0xe8},
        {0x0f, 0x02},
        {0x10, 0x88},

        {0x17, 0x54},
        {0x19, 0x08},
        {0x1a, 0x0a},
        {0x1f, 0x40},
        {0x20, 0x30},
        {0x2e, 0x80},
        {0x2f, 0x2b},
        {0x30, 0x1a},
        {0xfe, 0x02},
        {0x03, 0x02},
        {0x05, 0xd7},
        {0x06, 0x60},
        {0x08, 0x80},
        {0x12, 0x89},

        /*SPI*/
        {0xfe, 0x03},
        {0x52, 0xba},
        {0x53, 0x24},
        {0x54, 0x20},
        {0x55, 0x00},
        {0x59, 0x1f}, // {0x59,0x10}, 20190627 scaler output error
        {0x5a, 0x40}, //00 //yuv
        {0x5b, 0x80},
        {0x5c, 0x02},
        {0x5d, 0xe0},
        {0x5e, 0x01},
        {0x51, 0x03},
        {0x64, 0x04},
        {0xfe, 0x00},

        /*blk*/
        {0xfe, 0x00},
        {0x18, 0x02},
        {0xfe, 0x02},
        {0x40, 0x22},
        {0x45, 0x00},
        {0x46, 0x00},
        {0x49, 0x20},
        {0x4b, 0x3c},
        {0x50, 0x20},
        {0x42, 0x10},

        /*isp*/
        {0xfe, 0x01},
        {0x0a, 0xc5},
        {0x45, 0x00},
        {0xfe, 0x00},
        {0x40, 0xff},
        {0x41, 0x25},
        {0x42, 0xef},
        {0x43, 0x10},
        {0x44, 0x82},
        {0x46, 0x22},
        {0x49, 0x03},
        {0x52, 0x02},
        {0x54, 0x00},
        {0xfe, 0x02},
        {0x22, 0xf6},

        /*Shading*/
        {0xfe, 0x01},
        {0xc1, 0x38},
        {0xc2, 0x4c},
        {0xc3, 0x00},
        {0xc4, 0x2c},
        {0xc5, 0x24},
        {0xc6, 0x18},
        {0xc7, 0x28},
        {0xc8, 0x11},
        {0xc9, 0x15},
        {0xca, 0x20},
        {0xdc, 0x7a},
        {0xdd, 0xa0},
        {0xde, 0x80},
        {0xdf, 0x88},

        /*AWB*/ /*20170110*/
        {0xfe, 0x01},
        {0x50, 0xc1},
        {0x56, 0x34},
        {0x58, 0x04},
        {0x65, 0x06},
        {0x66, 0x0f},
        {0x67, 0x04},
        {0x69, 0x20},
        {0x6a, 0x40},
        {0x6b, 0x81},
        {0x6d, 0x12},
        {0x6e, 0xc0},
        {0x7b, 0x2a},
        {0x7c, 0x0c},
        {0xfe, 0x01},
        {0x90, 0xe3},
        {0x91, 0xc2},
        {0x92, 0xff},
        {0x93, 0xe3},
        {0x95, 0x1c},
        {0x96, 0xff},
        {0x97, 0x44},
        {0x98, 0x1c},
        {0x9a, 0x44},
        {0x9b, 0x1c},
        {0x9c, 0x64},
        {0x9d, 0x44},
        {0x9f, 0x71},
        {0xa0, 0x64},
        {0xa1, 0x00},
        {0xa2, 0x00},
        {0x86, 0x00},
        {0x87, 0x00},
        {0x88, 0x00},
        {0x89, 0x00},
        {0xa4, 0xc2},
        {0xa5, 0x9b},
        {0xa6, 0xc8},
        {0xa7, 0x92},
        {0xa9, 0xc9},
        {0xaa, 0x96},
        {0xab, 0xa9},
        {0xac, 0x99},
        {0xae, 0xce},
        {0xaf, 0xa9},
        {0xb0, 0xcf},
        {0xb1, 0x9d},
        {0xb3, 0xcf},
        {0xb4, 0xac},
        {0xb5, 0x00},
        {0xb6, 0x00},
        {0x8b, 0x00},
        {0x8c, 0x00},
        {0x8d, 0x00},
        {0x8e, 0x00},
        {0x94, 0x55},
        {0x99, 0xa6},
        {0x9e, 0xaa},
        {0xa3, 0x0a},
        {0x8a, 0x00},
        {0xa8, 0x55},
        {0xad, 0x55},
        {0xb2, 0x55},
        {0xb7, 0x05},
        {0x8f, 0x00},
        {0xb8, 0xc7},
        {0xb9, 0xa0},

        {0xfe, 0x01},
        {0xd0, 0x40},
        {0xd1, 0x00},
        {0xd2, 0x00},
        {0xd3, 0xfa},
        {0xd4, 0x4a},
        {0xd5, 0x02},

        {0xd6, 0x44},
        {0xd7, 0xfa},
        {0xd8, 0x04},
        {0xd9, 0x08},
        {0xda, 0x5c},
        {0xdb, 0x02},
        {0xfe, 0x00},

        /*Gamma*/
        {0xfe, 0x00},
        {0xba, 0x00},
        {0xbb, 0x06},
        {0xbc, 0x0b},
        {0xbd, 0x10},
        {0xbe, 0x19},
        {0xbf, 0x26},
        {0xc0, 0x33},
        {0xc1, 0x3f},
        {0xc2, 0x54},
        {0xc3, 0x68},
        {0xc4, 0x7c},
        {0xc5, 0x8c},
        {0xc6, 0x9c},
        {0xc7, 0xb4},
        {0xc8, 0xc9},
        {0xc9, 0xd8},
        {0xca, 0xe4},
        {0xcb, 0xee},
        {0xcc, 0xf4},
        {0xcd, 0xf9},
        {0xce, 0xfa},
        {0xcf, 0xff},

        /*Auto Gamma*/
        {0xfe, 0x00},
        {0x5a, 0x08},
        {0x5b, 0x0f},
        {0x5c, 0x15},
        {0x5d, 0x1c},
        {0x5e, 0x28},
        {0x5f, 0x36},
        {0x60, 0x45},
        {0x61, 0x51},
        {0x62, 0x6a},
        {0x63, 0x7d},
        {0x64, 0x8d},
        {0x65, 0x98},
        {0x66, 0xa2},
        {0x67, 0xb5},
        {0x68, 0xc3},
        {0x69, 0xcd},
        {0x6a, 0xd4},
        {0x6b, 0xdc},
        {0x6c, 0xe3},
        {0x6d, 0xf0},
        {0x6e, 0xf9},
        {0x6f, 0xff},

        /*Gain*/
        {0xfe, 0x00},
        {0x70, 0x50},

        /*AEC*/
        {0xfe, 0x00},
        {0x4f, 0x01},
        {0xfe, 0x01},
        {0x0c, 0x01},
        {0x0d, 0x00}, //08 add 20170110
        {0x12, 0xa0},
        {0x13, 0x4a},
        {0x44, 0x04},
        {0x1f, 0x40},
        {0x20, 0x40},
        {0x23, 0x0a},
        {0x26, 0x9a},
        {0x3e, 0x20},
        {0x3f, 0x2d},
        {0x40, 0x40},
        {0x41, 0x5b},
        {0x42, 0x82},
        {0x43, 0xb7},
        {0x04, 0x0a},
        {0x02, 0x79},
        {0x03, 0xc0},

        /*measure window*/
        {0xfe, 0x01},
        {0xcc, 0x08},
        {0xcd, 0x08},
        {0xce, 0xa4},
        {0xcf, 0xec},

        /*DNDD*/
        {0xfe, 0x00},
        {0x81, 0xb8},
        {0x82, 0x04},
        {0x83, 0x10},
        {0x84, 0x01},
        {0x86, 0x50},
        {0x87, 0x18},
        {0x88, 0x10},
        {0x89, 0x70},
        {0x8a, 0x20},
        {0x8b, 0x10},
        {0x8c, 0x08},
        {0x8d, 0x0a},

        /*Intpee*/
        {0xfe, 0x00},
        {0x8f, 0xaa},
        {0x90, 0x1c},
        {0x91, 0x52},
        {0x92, 0x03},
        {0x93, 0x03},
        {0x94, 0x08},
        {0x95, 0x6a},
        {0x97, 0x00},
        {0x98, 0x00},

        /*ASDE*/
        {0xfe, 0x00},
        {0x9a, 0x30},
        {0x9b, 0x50},
        {0xa1, 0x30},
        {0xa2, 0x66},
        {0xa4, 0x28},
        {0xa5, 0x30},
        {0xaa, 0x28},
        {0xac, 0x32},

        /*YCP*/
        {0xfe, 0x00},
        {0xd1, 0x3f},
        {0xd2, 0x3f},
        {0xd3, 0x38},
        {0xd6, 0xf4},
        {0xd7, 0x1d},
        {0xdd, 0x72},
        {0xde, 0x84},

        {0xfe, 0x00},
        {0x05, 0x01},
        {0x06, 0xad},
        {0x07, 0x00},
        {0x08, 0x10},

        {0xfe, 0x01},
        {0x25, 0x00},
        {0x26, 0x4d},

        {0x27, 0x01},
        {0x28, 0xce}, //16.6fps
        {0x29, 0x01},
        {0x2a, 0xce}, //12.5fps
        {0x2b, 0x01},
        {0x2c, 0xce}, //10fps
        {0x2d, 0x01},
        {0x2e, 0xce}, //8.33fps
        {0x2f, 0x01},
        {0x30, 0xce}, //5.88fps
        {0x31, 0x01},
        {0x32, 0xce}, //4.34fps
        {0x33, 0x01},
        {0x34, 0xce}, //3.99fps
        {0x3c, 0x10}, //{0x3c,0x00}
        {0xfe, 0x00},
};

sensorInfo_t gc032aInfo =
    {
        "gc032a",         //		const char *name; // name of sensor
        DRV_I2C_BPS_100K, //		drvI2cBps_t baud;
        0x42 >> 1,        //		uint8_t salve_i2c_addr_w;	 // salve i2c write address
        0x43 >> 1,        //		uint8_t salve_i2c_addr_r;	 // salve i2c read address
        0,                //		uint8_t reg_addr_value_bits; // bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
        {0x23, 0x2a},     //		uint8_t sensorid[2];
        640,              //		uint16_t spi_pixels_per_line;	// max width of source image
        480,              //		uint16_t spi_pixels_per_column; // max height of source image
        1,                //		uint16_t rstActiveH;	// 1: high level valid; 0: low level valid
        100,              //		uint16_t rstPulseWidth; // Unit: ms. Less than 200ms
        1,                //		uint16_t pdnActiveH;	// 1: high level valid; 0: low level valid
        0,                //		uint16_t dstWinColStart;
        640,              //		uint16_t dstWinColEnd;
        0,                //		uint16_t dstWinRowStart;
        480,              //		uint16_t dstWinRowEnd;
        1,                //		uint16_t spi_ctrl_clk_div;
        DRV_NAME_I2C1,    //		uint32_t i2c_name;
        0,                //		uint32_t nPixcels;
        1,                //		uint8_t captureDrops;
        0,
        0,
        NULL, //		uint8_t *buffer;
        {NULL, NULL},
        true,
        true,
        false, //		bool isCamIfcStart;
        false, //		bool scaleEnable;
        true,  //		bool cropEnable;
        false, //		bool dropFrame;
        false, //		bool spi_href_inv;
        false, //		bool spi_little_endian_en;
        false,
        false,
        SENSOR_VDD_2800MV,     //		cameraAVDD_t avdd_val; // voltage of avdd
        SENSOR_VDD_1800MV,     //		cameraAVDD_t iovdd_val;
        CAMA_CLK_OUT_FREQ_12M, //		cameraClk_t sensorClk;
        ROW_RATIO_1_1,         //		camRowRatio_t rowRatio;
        COL_RATIO_1_1,         //		camColRatio_t colRatio;
        CAM_FORMAT_YUV,        //		cameraImageFormat_t image_format; // define in SENSOR_IMAGE_FORMAT_E enum,
        SPI_MODE_MASTER2_2,    //		camSpiMode_t camSpiMode;
        SPI_OUT_Y1_U0_Y0_V0,   //		camSpiYuv_t camYuvMode;
        camCaptureIdle,        //		camCapture_t camCapStatus;
        camSpi_In,
        NULL, //		drvIfcChannel_t *camp_ipc;
        NULL, //		drvI2cMaster_t *i2c_p;
        NULL, //		CampCamptureCB captureCB;
        NULL,
};

static void prvCamGc032aPowerOn(void)
{
    halPmuSetPowerLevel(HAL_POWER_CAMD, gc032aInfo.dvdd_val);
    halPmuSwitchPower(HAL_POWER_CAMD, true, false);
    osiDelayUS(1000);
    halPmuSetPowerLevel(HAL_POWER_CAMA, gc032aInfo.avdd_val);
    halPmuSwitchPower(HAL_POWER_CAMA, true, false);
    osiDelayUS(1000);
}

static void prvCamGc032aPowerOff(void)
{
    halPmuSwitchPower(HAL_POWER_CAMA, false, false);
    osiDelayUS(1000);
    halPmuSwitchPower(HAL_POWER_CAMD, false, false);
    osiDelayUS(1000);
}

static bool prvCamGc032aI2cOpen(uint32_t name, drvI2cBps_t bps)
{
    if (name == 0 || bps != DRV_I2C_BPS_100K || gc032aInfo.i2c_p != NULL)
    {
        return false;
    }
    gc032aInfo.i2c_p = drvI2cMasterAcquire(name, bps);
    if (gc032aInfo.i2c_p == NULL)
    {
        OSI_LOGE(0, "cam i2c open fail");
        return false;
    }
    return true;
}

static void prvCamGc032aI2cClose()
{
    if (gc032aInfo.i2c_p != NULL)
        drvI2cMasterRelease(gc032aInfo.i2c_p);
    gc032aInfo.i2c_p = NULL;
}

static void prvCamWriteOneReg(uint8_t addr, uint8_t data)
{
    drvI2cSlave_t idAddress = {gc032aInfo.salve_i2c_addr_w, addr, 0, false};
    if (gc032aInfo.i2c_p != NULL)
    {
        drvI2cWrite(gc032aInfo.i2c_p, &idAddress, &data, 1);
    }
    else
    {
        OSI_LOGE(0, "i2c is not open");
    }
}

static void prvCamReadReg(uint8_t addr, uint8_t *data, uint32_t len)
{
    drvI2cSlave_t idAddress = {gc032aInfo.salve_i2c_addr_w, addr, 0, false};
    if (gc032aInfo.i2c_p != NULL)
    {
        drvI2cRead(gc032aInfo.i2c_p, &idAddress, data, len);
    }
    else
    {
        OSI_LOGE(0, "i2c is not open");
    }
}

static bool prvCamWriteRegList(const cameraReg_t *regList, uint16_t len)
{
    uint16_t regCount;
    drvI2cSlave_t wirte_data = {gc032aInfo.salve_i2c_addr_w, 0, 0, false};
    prvCamWriteOneReg(0xf4, 0x1c);
    osiDelayUS(1000);
    for (regCount = 0; regCount < len; regCount++)
    {
        wirte_data.addr_data = regList[regCount].addr;
        if (drvI2cWrite(gc032aInfo.i2c_p, &wirte_data, &regList[regCount].data, 1))
            osiDelayUS(5);
        else
            return false;
    }
    return true;
}

static bool prvCamGc032aRginit(sensorInfo_t *info)
{
    switch (info->sensorType)
    {
    case camSpi_In:
        if (!prvCamWriteRegList(RG_InitVgaSPI, sizeof(RG_InitVgaSPI) / sizeof(cameraReg_t)))
            return false;
        break;
    default:
        return false;
    }
    return true;
}

static void prvCamIsrCB(void *ctx)
{
    static uint8_t pictureDrop = 0;
    REG_CAMERA_IRQ_CAUSE_T cause;
    cameraIrqCause_t mask = {0, 0, 0, 0};
    cause.v = hwp_camera->irq_cause;
    hwp_camera->irq_clear = cause.v;
    switch (gc032aInfo.camCapStatus)
    {
    case camCaptureState1:
        if (cause.b.vsync_f)
        {
            drvCamClrIrqMask();
            drvCamCmdSetFifoRst();
            if (gc032aInfo.isCamIfcStart == true)
            {
                drvCampStopTransfer(gc032aInfo.nPixcels, gc032aInfo.capturedata);
                gc032aInfo.isCamIfcStart = false;
            }
            drvCampStartTransfer(gc032aInfo.nPixcels, gc032aInfo.capturedata);
            mask.fend = 1;
            drvCamSetIrqMask(mask);
            gc032aInfo.camCapStatus = camCaptureState2;
            gc032aInfo.isCamIfcStart = true;
        }
        break;
    case camCaptureState2:
        if (cause.b.vsync_f)
        {
            if (gc032aInfo.isCamIfcStart == true)
            {
                gc032aInfo.isCamIfcStart = false;
                if (drvCampStopTransfer(gc032aInfo.nPixcels, gc032aInfo.capturedata) == true)
                {
                    if (pictureDrop < gc032aInfo.captureDrops)
                    {
                        mask.fend = 1;
                        drvCamSetIrqMask(mask);
                        gc032aInfo.camCapStatus = camCaptureState1;
                        pictureDrop++;
                    }
                    else
                    {
                        gc032aInfo.camCapStatus = camCaptureIdle;
                        osiSemaphoreRelease(gc032aInfo.cam_sem_capture);
                        if (pictureDrop >= gc032aInfo.captureDrops)
                            pictureDrop = 0;
                        gc032aInfo.isFirst = false;
                    }
                }
                else
                {
                    drvCampStartTransfer(gc032aInfo.nPixcels, gc032aInfo.capturedata);
                    mask.fend = 1;
                    gc032aInfo.isCamIfcStart = true;
                    drvCamSetIrqMask(mask);
                }
            }
        }
        break;
    case campPreviewState1:
        if (cause.b.vsync_f)
        {
            drvCamClrIrqMask();
            drvCamCmdSetFifoRst();
            if (gc032aInfo.isCamIfcStart == true)
            {
                drvCampStopTransfer(gc032aInfo.nPixcels, gc032aInfo.previewdata[gc032aInfo.page_turn]);
                gc032aInfo.isCamIfcStart = false;
            }
            drvCampStartTransfer(gc032aInfo.nPixcels, gc032aInfo.previewdata[gc032aInfo.page_turn]);
            mask.fend = 1;
            drvCamSetIrqMask(mask);
            gc032aInfo.camCapStatus = campPreviewState2;
            gc032aInfo.isCamIfcStart = true;
        }
        break;
    case campPreviewState2:
        if (cause.b.vsync_f)
        {
            drvCamClrIrqMask();
            hwp_camera->spi_camera_reg4 &= ~(1 << 0);
            drvCamCmdSetFifoRst();
            hwp_camera->spi_camera_reg4 |= (1 << 0);
            if (gc032aInfo.isCamIfcStart == true)
            {
                gc032aInfo.isCamIfcStart = false;
                if (drvCampStopTransfer(gc032aInfo.nPixcels, gc032aInfo.previewdata[gc032aInfo.page_turn]) == true)
                {
                    OSI_LOGD(0, "recv data %d", gc032aInfo.page_turn);
                    gc032aInfo.page_turn = 1 - gc032aInfo.page_turn;
                    osiSemaphoreRelease(gc032aInfo.cam_sem_preview);
                    gc032aInfo.isFirst = false;
                    if (--gc032aInfo.preview_page)
                    {
                        drvCamClrIrqMask();
                        drvCamCmdSetFifoRst();
                        drvCampStartTransfer(gc032aInfo.nPixcels, gc032aInfo.previewdata[gc032aInfo.page_turn]);
                        gc032aInfo.isCamIfcStart = true;
                        mask.fend = 1;
                        drvCamSetIrqMask(mask);
                    }
                    else
                    {
                        drvCamClrIrqMask();
                        gc032aInfo.camCapStatus = camCaptureIdle;
                    }
                }
                else
                {
                    gc032aInfo.camCapStatus = campPreviewState1;
                    mask.fend = 1;
                    drvCamSetIrqMask(mask);
                }
            }
        }
        break;
    default:
        break;
    }
}

bool camGc032aOpen(void)
{
    OSI_LOGI(0, "camGc032aOpen");
    osiRequestSysClkActive(&gcCamCLK);
    drvCamSetPdn(false);
    osiDelayUS(1000);
    prvCamGc032aPowerOn();
    osiDelayUS(1000);
    drvCamSetMCLK(gc032aInfo.sensorClk);
    osiDelayUS(1000);
    drvCamSetPdn(true);
    osiDelayUS(1000);
    drvCamSetPdn(false);

    if (!prvCamGc032aI2cOpen(gc032aInfo.i2c_name, gc032aInfo.baud))
    {
        OSI_LOGE(0, "cam prvCamGc032aI2cOpen fail");
        prvCamGc032aI2cClose();
        prvCamGc032aPowerOff();
        return false;
    }
    if (!prvCamGc032aRginit(&gc032aInfo))
    {
        OSI_LOGE(0, "cam prvCamGc032aRginit fail");
        prvCamGc032aI2cClose();
        prvCamGc032aPowerOff();
        return false;
    }
    drvCampRegInit(&gc032aInfo);
    drvCamSetIrqHandler(prvCamIsrCB, NULL);
    gc032aInfo.isCamOpen = true;
    drvCameraControllerEnable(true);
    return true;
}

void camGc032aClose(void)
{
    if (gc032aInfo.isCamOpen)
    {
        osiReleaseClk(&gcCamCLK);
        drvCamSetPdn(true);
        drvCamDisableMCLK();
        prvCamGc032aPowerOff();
        drvCamSetPdn(false);
        prvCamGc032aI2cClose();
        drvCampRegDeInit();
        drvCamDisableIrq();
        gc032aInfo.isFirst = true;
        gc032aInfo.isCamOpen = false;
    }
    else
    {
        gc032aInfo.isCamOpen = false;
    }
}

void camGc032aGetId(uint8_t *data, uint8_t len)
{
    if (gc032aInfo.isCamOpen)
    {
        if (data == NULL || len < 1)
            return;
        prvCamWriteOneReg(0xf4, 0x1c);
        osiDelayUS(2);
        prvCamReadReg(0xf0, data, len);
    }
}

bool camGc032aCheckId(void)
{
    uint8_t sensorID[2] = {0, 0};
    if (!gc032aInfo.isCamOpen)
    {
        drvCamSetPdn(false);
        osiDelayUS(5);
        prvCamGc032aPowerOn();
        drvCamSetMCLK(gc032aInfo.sensorClk);
        drvCamSetPdn(true);
        osiDelayUS(5);
        drvCamSetPdn(false);
        osiDelayUS(5);
        if (!prvCamGc032aI2cOpen(gc032aInfo.i2c_name, gc032aInfo.baud))
        {
            OSI_LOGE(0, "cam prvCamGc032aI2cOpen fail");
            return false;
        }
        if (!gc032aInfo.isCamOpen)
        {
            gc032aInfo.isCamOpen = true;
        }
        camGc032aGetId(sensorID, 2);
        OSI_LOGI(0, "cam get id 0x%x,0x%x", sensorID[0], sensorID[1]);
        if ((gc032aInfo.sensorid[0] == sensorID[0]) && (gc032aInfo.sensorid[1] == sensorID[1]))
        {
            OSI_LOGI(0, "check id successful");
            camGc032aClose();
            return true;
        }
        else
        {
            camGc032aClose();
            OSI_LOGE(0, "check id error");
            return false;
        }
    }
    else
    {
        OSI_LOGE(0, "camera already opened !");
        return false;
    }
}

void camGc032aCaptureImage(uint32_t size)
{
    if (size != 0)
    {
        cameraIrqCause_t mask = {0, 0, 0, 0};
        drvCamSetIrqMask(mask);
        if (gc032aInfo.isCamIfcStart == true)
        {
            drvCampStopTransfer(gc032aInfo.nPixcels, gc032aInfo.capturedata);
            gc032aInfo.isCamIfcStart = false;
        }
        gc032aInfo.nPixcels = size;
        gc032aInfo.camCapStatus = camCaptureState1;

        mask.fend = 1;
        drvCamSetIrqMask(mask);
        drvCameraControllerEnable(true);
    }
}

void camGc032aPrevStart(uint32_t size)
{
    OSI_LOGI(0, "camGc032aPrevStart gc032aInfo.preview_page=%d ", gc032aInfo.preview_page);
    if (gc032aInfo.preview_page == 0)
    {
        cameraIrqCause_t mask = {0, 0, 0, 0};
        drvCamSetIrqMask(mask);
        if (gc032aInfo.isCamIfcStart == true)
        {
            drvCampStopTransfer(gc032aInfo.nPixcels, gc032aInfo.previewdata[gc032aInfo.page_turn]);
            gc032aInfo.isCamIfcStart = false;
        }
        gc032aInfo.nPixcels = size;
        gc032aInfo.camCapStatus = campPreviewState1;
        gc032aInfo.isStopPrev = false;
        gc032aInfo.preview_page = 2;
        mask.fend = 1;
        drvCamSetIrqMask(mask);
        drvCameraControllerEnable(true);
    }
}

void camGc032aPrevNotify(void)
{
    if (gc032aInfo.isCamOpen && !gc032aInfo.isStopPrev)
    {
        if (gc032aInfo.preview_page == 0)
        {
            cameraIrqCause_t mask = {0, 0, 0, 0};

            gc032aInfo.camCapStatus = campPreviewState1;
            gc032aInfo.preview_page++;
            mask.fend = 1;
            drvCamSetIrqMask(mask);
        }
        else
        {
            if (gc032aInfo.preview_page < 2)
                gc032aInfo.preview_page++;
        }
    }
}

void camGc032aStopPrev(void)
{
    drvCamClrIrqMask();
    if (gc032aInfo.isCamIfcStart == true)
    {
        drvCampStopTransfer(gc032aInfo.nPixcels, gc032aInfo.previewdata[gc032aInfo.page_turn]);
        gc032aInfo.isCamIfcStart = false;
    }
    drvCameraControllerEnable(false);
    gc032aInfo.camCapStatus = camCaptureIdle;
    gc032aInfo.isStopPrev = true;
    gc032aInfo.preview_page = 0;
}

void camGc032aSetFalsh(uint8_t level)
{
    if (level >= 0 && level < 16)
    {
        if (level == 0)
        {
            halPmuSwitchPower(HAL_POWER_CAMFLASH, false, false);
        }
        else
        {
            halPmuSetCamFlashLevel(level);
            halPmuSwitchPower(HAL_POWER_CAMFLASH, true, false);
        }
    }
}

void camGc032aBrightness(uint8_t level)
{
}

void camGc032aContrast(uint8_t level)
{
}

void camGc032aImageEffect(uint8_t effect_type)
{
}
void camGc032aEv(uint8_t level)
{
}

void camGc032aAWB(uint8_t mode)
{
}

void camGc032aGetSensorInfo(sensorInfo_t **pSensorInfo)
{
    OSI_LOGI(0, "CamGc032aGetSensorInfo %08x", &gc032aInfo);
    *pSensorInfo = &gc032aInfo;
}

bool camGc032aReg(SensorOps_t *pSensorOpsCB)
{
    if (camGc032aCheckId())
    {
        pSensorOpsCB->cameraOpen = camGc032aOpen;
        pSensorOpsCB->cameraClose = camGc032aClose;
        pSensorOpsCB->cameraGetID = camGc032aGetId;
        pSensorOpsCB->cameraCaptureImage = camGc032aCaptureImage;
        pSensorOpsCB->cameraStartPrev = camGc032aPrevStart;
        pSensorOpsCB->cameraPrevNotify = camGc032aPrevNotify;
        pSensorOpsCB->cameraStopPrev = camGc032aStopPrev;
        pSensorOpsCB->cameraSetAWB = camGc032aAWB;
        pSensorOpsCB->cameraSetBrightness = camGc032aBrightness;
        pSensorOpsCB->cameraSetContrast = camGc032aContrast;
        pSensorOpsCB->cameraSetEv = camGc032aEv;
        pSensorOpsCB->cameraSetImageEffect = camGc032aImageEffect;
        pSensorOpsCB->cameraGetSensorInfo = camGc032aGetSensorInfo;
        pSensorOpsCB->cameraFlashSet = camGc032aSetFalsh;
        return true;
    }
    return false;
}
