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

static const cameraReg_t RG_InitVgaMIPI[] =
    {
        {0xfe, 0xf0},
        {0xfe, 0xf0},
        {0xfe, 0x00},
        {0xfc, 0x0e},
        {0xfc, 0x0e},
        {0xf2, 0x80},
        {0xf3, 0x00}, // output_disable
        {0xf7, 0x1b},
        {0xf8, 0x04},
        {0xf9, 0x8e},
        {0xfa, 0x11},
        /////////////////////////////////////////////////
        ///////////////////   MIPI   ////////////////////
        /////////////////////////////////////////////////
        {0xfe, 0x03},
        {0x40, 0x08},
        {0x42, 0x00},
        {0x43, 0x00},
        {0x01, 0x03},
        {0x10, 0x84},

        {0x01, 0x03},
        {0x02, 0x00},
        {0x03, 0x94},
        {0x04, 0x01},
        {0x05, 0x00},
        {0x06, 0x80},
        {0x11, 0x1e},
        {0x12, 0x00},
        {0x13, 0x05},
        {0x15, 0x10},
        {0x21, 0x10},
        {0x22, 0x01},
        {0x23, 0x10},
        {0x24, 0x02},
        {0x25, 0x10},
        {0x26, 0x03},
        {0x29, 0x02},
        {0x2a, 0x0a},
        {0x2b, 0x04},
        {0xfe, 0x00},

        /////////////////////////////////////////////////
        /////////////////	CISCTL reg	 /////////////////
        /////////////////////////////////////////////////
        {0x00, 0x2f},
        {0x01, 0x0f}, //06
        {0x02, 0x04},
        {0x03, 0x03},
        {0x04, 0x50},
        {0x09, 0x00},
        {0x0a, 0x00},
        {0x0b, 0x00},
        {0x0c, 0x04},
        {0x0d, 0x01},
        {0x0e, 0xe8},
        {0x0f, 0x02},
        {0x10, 0x88},
        {0x16, 0x00},
        {0x17, 0x14},
        {0x18, 0x1a},
        {0x19, 0x14},
        {0x1b, 0x48},
        {0x1c, 0x1c},
        {0x1e, 0x6b},
        {0x1f, 0x28},
        {0x20, 0x8b}, //0x89 travis20140801
        {0x21, 0x49},
        {0x22, 0xb0},
        {0x23, 0x04},
        {0x24, 0x16},
        {0x34, 0x20},

        /////////////////////////////////////////////////
        ////////////////////	BLK   ////////////////////
        /////////////////////////////////////////////////
        {0x26, 0x23},
        {0x28, 0xff},
        {0x29, 0x00},
        {0x32, 0x00},
        {0x33, 0x10},
        {0x37, 0x20},
        {0x38, 0x10},
        {0x47, 0x80},
        {0x4e, 0x66},
        {0xa8, 0x02},
        {0xa9, 0x80},

        /////////////////////////////////////////////////
        //////////////////  ISP reg   ///////////////////
        /////////////////////////////////////////////////
        {0x40, 0xff},
        {0x41, 0x21},
        {0x42, 0xcf},
        {0x44, 0x02},
        {0x45, 0xa8},
        {0x46, 0x02}, //sync
        {0x4a, 0x11},
        {0x4b, 0x01},
        {0x4c, 0x20},
        {0x4d, 0x05},
        {0x4f, 0x01},
        {0x50, 0x01},
        {0x55, 0x01},
        {0x56, 0xe0},
        {0x57, 0x02},
        {0x58, 0x80},

        /////////////////////////////////////////////////
        ///////////////////   GAIN   ////////////////////
        /////////////////////////////////////////////////
        {0x70, 0x70},
        {0x5a, 0x84},
        {0x5b, 0xc9},
        {0x5c, 0xed},
        {0x77, 0x74},
        {0x78, 0x40},
        {0x79, 0x5f},

        /////////////////////////////////////////////////
        ///////////////////   DNDD  /////////////////////
        /////////////////////////////////////////////////
        {0x82, 0x14},
        {0x83, 0x0b},
        {0x89, 0xf0},

        /////////////////////////////////////////////////
        //////////////////   EEINTP  ////////////////////
        /////////////////////////////////////////////////
        {0x8f, 0xaa},
        {0x90, 0x8c},
        {0x91, 0x90},
        {0x92, 0x03},
        {0x93, 0x03},
        {0x94, 0x05},
        {0x95, 0x65},
        {0x96, 0xf0},

        /////////////////////////////////////////////////
        /////////////////////	ASDE  ////////////////////
        /////////////////////////////////////////////////
        {0xfe, 0x00},

        {0x9a, 0x20},
        {0x9b, 0x80},
        {0x9c, 0x40},
        {0x9d, 0x80},

        {0xa1, 0x30},
        {0xa2, 0x32},
        {0xa4, 0x30},
        {0xa5, 0x30},
        {0xaa, 0x10},
        {0xac, 0x22},

        /////////////////////////////////////////////////
        ///////////////////   GAMMA   ///////////////////
        /////////////////////////////////////////////////
        {0xfe, 0x00}, //default
        {0xbf, 0x08},
        {0xc0, 0x16},
        {0xc1, 0x28},
        {0xc2, 0x41},
        {0xc3, 0x5a},
        {0xc4, 0x6c},
        {0xc5, 0x7a},
        {0xc6, 0x96},
        {0xc7, 0xac},
        {0xc8, 0xbc},
        {0xc9, 0xc9},
        {0xca, 0xd3},
        {0xcb, 0xdd},
        {0xcc, 0xe5},
        {0xcd, 0xf1},
        {0xce, 0xfa},
        {0xcf, 0xff},

        /////////////////////////////////////////////////
        ///////////////////   YCP	//////////////////////
        /////////////////////////////////////////////////
        {0xd0, 0x40},
        {0xd1, 0x34},
        {0xd2, 0x34},
        {0xd3, 0x40},
        {0xd6, 0xf2},
        {0xd7, 0x1b},
        {0xd8, 0x18},
        {0xdd, 0x03},

        /////////////////////////////////////////////////
        ////////////////////	AEC   ////////////////////
        /////////////////////////////////////////////////
        {0xfe, 0x01},
        {0x05, 0x30},
        {0x06, 0x75},
        {0x07, 0x40},
        {0x08, 0xb0},
        {0x0a, 0xc5},
        {0x0b, 0x11},
        {0x0c, 0x00},
        {0x12, 0x52},
        {0x13, 0x38},
        {0x18, 0x95},
        {0x19, 0x96},
        {0x1f, 0x20},
        {0x20, 0xc0},
        {0x3e, 0x40},
        {0x3f, 0x57},
        {0x40, 0x7d},
        {0x03, 0x60},
        {0x44, 0x02},

        /////////////////////////////////////////////////
        ////////////////////	AWB   ////////////////////
        /////////////////////////////////////////////////
        {0xfe, 0x01},
        {0x1c, 0x91},
        {0x21, 0x15},
        {0x50, 0x80},
        {0x56, 0x04},
        {0x59, 0x08},
        {0x5b, 0x02},
        {0x61, 0x8d},
        {0x62, 0xa7},
        {0x63, 0xd0},
        {0x65, 0x06},
        {0x66, 0x06},
        {0x67, 0x84},
        {0x69, 0x08},
        {0x6a, 0x25},
        {0x6b, 0x01},
        {0x6c, 0x00},
        {0x6d, 0x02},
        {0x6e, 0xf0},
        {0x6f, 0x80},
        {0x76, 0x80},
        {0x78, 0xaf},
        {0x79, 0x75},
        {0x7a, 0x40},
        {0x7b, 0x50},
        {0x7c, 0x0c},

        {0x90, 0xc9}, //stable AWB
        {0x91, 0xbe},
        {0x92, 0xe2},
        {0x93, 0xc9},
        {0x95, 0x1b},
        {0x96, 0xe2},
        {0x97, 0x49},
        {0x98, 0x1b},
        {0x9a, 0x49},
        {0x9b, 0x1b},
        {0x9c, 0xc3},
        {0x9d, 0x49},
        {0x9f, 0xc7},
        {0xa0, 0xc8},
        {0xa1, 0x00},
        {0xa2, 0x00},
        {0x86, 0x00},
        {0x87, 0x00},
        {0x88, 0x00},
        {0x89, 0x00},
        {0xa4, 0xb9},
        {0xa5, 0xa0},
        {0xa6, 0xba},
        {0xa7, 0x92},
        {0xa9, 0xba},
        {0xaa, 0x80},
        {0xab, 0x9d},
        {0xac, 0x7f},
        {0xae, 0xbb},
        {0xaf, 0x9d},
        {0xb0, 0xc8},
        {0xb1, 0x97},
        {0xb3, 0xb7},
        {0xb4, 0x7f},
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
        {0xb8, 0xcb},
        {0xb9, 0x9b},

        /////////////////////////////////////////////////
        ////////////////////  CC ////////////////////////
        /////////////////////////////////////////////////
        {0xfe, 0x01},

        {0xd0, 0x38}, //skin red
        {0xd1, 0x00},
        {0xd2, 0x02},
        {0xd3, 0x04},
        {0xd4, 0x38},
        {0xd5, 0x12},

        {0xd6, 0x30},
        {0xd7, 0x00},
        {0xd8, 0x0a},
        {0xd9, 0x16},
        {0xda, 0x39},
        {0xdb, 0xf8},

        /////////////////////////////////////////////////
        ////////////////////	LSC   ////////////////////
        /////////////////////////////////////////////////
        {0xfe, 0x01},
        {0xc1, 0x3c},
        {0xc2, 0x50},
        {0xc3, 0x00},
        {0xc4, 0x40},
        {0xc5, 0x30},
        {0xc6, 0x30},
        {0xc7, 0x10},
        {0xc8, 0x00},
        {0xc9, 0x00},
        {0xdc, 0x20},
        {0xdd, 0x10},
        {0xdf, 0x00},
        {0xde, 0x00},

        /////////////////////////////////////////////////
        ///////////////////  Histogram  /////////////////
        /////////////////////////////////////////////////
        {0x01, 0x10},
        {0x0b, 0x31},
        {0x0e, 0x50},
        {0x0f, 0x0f},
        {0x10, 0x6e},
        {0x12, 0xa0},
        {0x15, 0x60},
        {0x16, 0x60},
        {0x17, 0xe0},

        /////////////////////////////////////////////////
        //////////////   Measure Window   ///////////////
        /////////////////////////////////////////////////
        {0xcc, 0x0c},
        {0xcd, 0x10},
        {0xce, 0xa0},
        {0xcf, 0xe6},

        /////////////////////////////////////////////////
        /////////////////	 dark sun	//////////////////
        /////////////////////////////////////////////////
        {0x45, 0xf7},
        {0x46, 0xff},
        {0x47, 0x15},
        {0x48, 0x03},
        {0x4f, 0x60},

        /////////////////////////////////////////////////
        ///////////////////  banding  ///////////////////
        /////////////////////////////////////////////////
        {0xfe, 0x00},
        {0x05, 0x00},
        {0x06, 0xd0}, //HB
        {0x07, 0x00},
        {0x08, 0x42}, //VB
        {0xfe, 0x01},
        {0x25, 0x00}, //step
        {0x26, 0xa6},
        {0x27, 0x01}, //20fps
        {0x28, 0xf2},
        {0x29, 0x01}, //12.5fps
        {0x2a, 0xf2},
        {0x2b, 0x01}, //7.14fps
        {0x2c, 0xf2},
        {0x2d, 0x01}, //5.55fps
        {0x2e, 0xf2},
        {0x3c, 0x00},
        {0xfe, 0x00},

        {0xfe, 0x03},
        {0x10, 0x94},
        {0xfe, 0x00},
};

sensorInfo_t gc0310Info =
    {
        "gc0310",         //		const char *name; // name of sensor
        DRV_I2C_BPS_100K, //		drvI2cBps_t baud;
        0x42 >> 1,        //		uint8_t salve_i2c_addr_w;	 // salve i2c write address
        0x43 >> 1,        //		uint8_t salve_i2c_addr_r;	 // salve i2c read address
        0,                //		uint8_t reg_addr_value_bits; // bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
        {0xa3, 0x10},     //		uint8_t sensorid[2];
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
        2,                //		uint8_t captureDrops;
        0,
        0,
        NULL, //		uint8_t *buffer;
        {NULL, NULL},
        true,
        true,
        false, //		bool isCamIfcStart;
        false, //		bool scaleEnable;
        false, //		bool cropEnable;
        false, //		bool dropFrame;
        false, //		bool spi_href_inv;
        false, //		bool spi_little_endian_en;
        false,
        false,
        SENSOR_VDD_2800MV,     //		cameraAVDD_t avdd_val; // voltage of avdd
        SENSOR_VDD_1800MV,     //		cameraAVDD_t iovdd_val;
        CAMA_CLK_OUT_FREQ_26M, //		cameraClk_t sensorClk;
        ROW_RATIO_1_1,         //		camRowRatio_t rowRatio;
        COL_RATIO_1_1,         //		camColRatio_t colRatio;
        CAM_FORMAT_YUV,        //		cameraImageFormat_t image_format; // define in SENSOR_IMAGE_FORMAT_E enum,
        SPI_MODE_NO,           //		camSpiMode_t camSpiMode;
        SPI_OUT_Y1_U0_Y0_V0,   //		camSpiYuv_t camYuvMode;
        camCaptureIdle,        //		camCapture_t camCapStatus;
        camCsi_In,             //
        NULL,                  //		drvIfcChannel_t *camp_ipc;
        NULL,                  //		drvI2cMaster_t *i2c_p;
        NULL,                  //		CampCamptureCB captureCB;
        NULL,
};

static void prvCamGc0310PowerOn(void)
{
    halPmuSetPowerLevel(HAL_POWER_CAMD, gc0310Info.dvdd_val);
    halPmuSwitchPower(HAL_POWER_CAMD, true, false);
    osiDelayUS(1000);
    halPmuSetPowerLevel(HAL_POWER_CAMA, gc0310Info.avdd_val);
    halPmuSwitchPower(HAL_POWER_CAMA, true, false);
    osiDelayUS(1000);
}

static void prvCamGc0310PowerOff(void)
{
    halPmuSwitchPower(HAL_POWER_CAMA, false, false);
    osiDelayUS(1000);
    halPmuSwitchPower(HAL_POWER_CAMD, false, false);
    osiDelayUS(1000);
}

static bool prvCamGc0310I2cOpen(uint32_t name, drvI2cBps_t bps)
{
    if (name == 0 || bps != DRV_I2C_BPS_100K || gc0310Info.i2c_p != NULL)
    {
        return false;
    }
    gc0310Info.i2c_p = drvI2cMasterAcquire(name, bps);
    if (gc0310Info.i2c_p == NULL)
    {
        OSI_LOGE(0, "cam i2c open fail");
        return false;
    }
    return true;
}

static void prvCamGc0310I2cClose()
{
    if (gc0310Info.i2c_p != NULL)
        drvI2cMasterRelease(gc0310Info.i2c_p);
    gc0310Info.i2c_p = NULL;
}

static void prvCamWriteOneReg(uint8_t addr, uint8_t data)
{
    drvI2cSlave_t idAddress = {gc0310Info.salve_i2c_addr_w, addr, 0, false};
    if (gc0310Info.i2c_p != NULL)
    {
        drvI2cWrite(gc0310Info.i2c_p, &idAddress, &data, 1);
    }
    else
    {
        OSI_LOGE(0, "i2c is not open");
    }
}

static void prvCamReadReg(uint8_t addr, uint8_t *data, uint32_t len)
{
    drvI2cSlave_t idAddress = {gc0310Info.salve_i2c_addr_w, addr, 0, false};
    if (gc0310Info.i2c_p != NULL)
    {
        drvI2cRead(gc0310Info.i2c_p, &idAddress, data, len);
    }
    else
    {
        OSI_LOGE(0, "i2c is not open");
    }
}

static bool prvCamWriteRegList(const cameraReg_t *regList, uint16_t len)
{
    uint16_t regCount;
    drvI2cSlave_t wirte_data = {gc0310Info.salve_i2c_addr_w, 0, 0, false};
    for (regCount = 0; regCount < len; regCount++)
    {
        OSI_LOGI(0, "cam write reg %x,%x", regList[regCount].addr, regList[regCount].data);
        wirte_data.addr_data = regList[regCount].addr;
        if (drvI2cWrite(gc0310Info.i2c_p, &wirte_data, &regList[regCount].data, 1))
            osiDelayUS(5);
        else
            return false;
    }
    return true;
}

static bool prvCamGc0310Rginit(sensorInfo_t *info)
{
    OSI_LOGI(0, "cam prvCamGC0310Rginit %d", info->sensorType);
    switch (info->sensorType)
    {
    case camCsi_In:
        if (!prvCamWriteRegList(RG_InitVgaMIPI, sizeof(RG_InitVgaMIPI) / sizeof(cameraReg_t)))
            return false;
        break;
    default:
        return false;
    }
    return true;
}

static void prvSpiCamIsrCB(void *ctx)
{
    static uint8_t pictureDrop = 0;
    REG_CAMERA_IRQ_CAUSE_T cause;
    cameraIrqCause_t mask = {0, 0, 0, 0};
    cause.v = hwp_camera->irq_cause;
    hwp_camera->irq_clear = cause.v;
    switch (gc0310Info.camCapStatus)
    {
    case camCaptureState1:
        if (cause.b.vsync_f)
        {
            drvCamClrIrqMask();
            drvCamCmdSetFifoRst();
            if (gc0310Info.isCamIfcStart == true)
            {
                drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.capturedata);
                gc0310Info.isCamIfcStart = false;
            }
            drvCampStartTransfer(gc0310Info.nPixcels, gc0310Info.capturedata);
            mask.fend = 1;
            drvCamSetIrqMask(mask);
            gc0310Info.camCapStatus = camCaptureState2;
            gc0310Info.isCamIfcStart = true;
        }
        break;
    case camCaptureState2:
        if (cause.b.vsync_f)
        {
            if (gc0310Info.isCamIfcStart == true)
            {
                gc0310Info.isCamIfcStart = false;
                if (drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.capturedata) == true)
                {
                    if (pictureDrop < gc0310Info.captureDrops)
                    {
                        mask.fend = 1;
                        drvCamSetIrqMask(mask);
                        gc0310Info.camCapStatus = camCaptureState1;
                        pictureDrop++;
                    }
                    else
                    {
                        gc0310Info.camCapStatus = camCaptureIdle;
                        osiSemaphoreRelease(gc0310Info.cam_sem_capture);
                        if (pictureDrop >= gc0310Info.captureDrops)
                            pictureDrop = 0;
                        gc0310Info.isFirst = false;
                    }
                }
                else
                {
                    drvCampStartTransfer(gc0310Info.nPixcels, gc0310Info.capturedata);
                    mask.fend = 1;
                    gc0310Info.isCamIfcStart = true;
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
            if (gc0310Info.isCamIfcStart == true)
            {
                drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
                gc0310Info.isCamIfcStart = false;
            }
            drvCampStartTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
            mask.fend = 1;
            drvCamSetIrqMask(mask);
            gc0310Info.camCapStatus = campPreviewState2;
            gc0310Info.isCamIfcStart = true;
        }
        break;
    case campPreviewState2:
        if (cause.b.vsync_f)
        {
            drvCamClrIrqMask();
            drvCamCmdSetFifoRst();
            if (gc0310Info.isCamIfcStart == true)
            {
                gc0310Info.isCamIfcStart = false;
                if (drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]) == true)
                {
                    OSI_LOGI(0, "cam recv data %d", gc0310Info.page_turn);
                    gc0310Info.page_turn = 1 - gc0310Info.page_turn;
                    osiSemaphoreRelease(gc0310Info.cam_sem_preview);
                    gc0310Info.isFirst = false;
                    if (--gc0310Info.preview_page)
                    {
                        drvCamClrIrqMask();
                        drvCamCmdSetFifoRst();
                        drvCampStartTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
                        gc0310Info.isCamIfcStart = true;
                        mask.fend = 1;
                        drvCamSetIrqMask(mask);
                    }
                    else
                    {
                        gc0310Info.camCapStatus = camCaptureIdle;
                    }
                }
                else
                {
                    gc0310Info.camCapStatus = campPreviewState1;
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

static void prvCsiCamIsrCB(void *ctx)
{
    static uint8_t pictureDrop = 0;
    static uint8_t prev_ovf = 0;
    static uint8_t cap_ovf = 0;
    REG_CAMERA_IRQ_CAUSE_T cause;
    cameraIrqCause_t mask = {0, 0, 0, 0};
    cause.v = hwp_camera->irq_cause;
    hwp_camera->irq_clear = cause.v;

    OSI_LOGI(0, "cam gc0310 prvCsiCamIsrCB %d,%d,%d,%d", cause.b.vsync_f, cause.b.ovfl, cause.b.dma_done, cause.b.vsync_r);
    OSI_LOGI(0, "gc0310Info.camCapStatus %d", gc0310Info.camCapStatus);
    switch (gc0310Info.camCapStatus)
    {
    case camCaptureState1:
        if (cause.b.vsync_f)
        {
            drvCamClrIrqMask();
            drvCamCmdSetFifoRst();
            if (gc0310Info.isCamIfcStart == true)
            {
                drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
                gc0310Info.isCamIfcStart = false;
            }
            drvCampStartTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
            gc0310Info.isCamIfcStart = true;
            mask.overflow = 1;
            mask.dma = 1;
            drvCamSetIrqMask(mask);
            gc0310Info.camCapStatus = camCaptureState2;
        }
        break;
    case camCaptureState2:
        if (cause.b.ovfl)
        {
            OSI_LOGI(0, "cam ovfl ");
            drvCameraControllerEnable(false);
            drvCamCmdSetFifoRst();
            drvCameraControllerEnable(true);
            cap_ovf = 1;
        }
        if (cause.b.dma_done)
        {
            drvCamClrIrqMask();
            if (cap_ovf == 1)
            {
                cap_ovf = 0;
                if (gc0310Info.isCamIfcStart)
                    drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
                gc0310Info.camCapStatus = camCaptureState1;
                mask.fend = 1;
                drvCamSetIrqMask(mask);
                return;
            }
            if (gc0310Info.isCamIfcStart == true)
            {
                gc0310Info.isCamIfcStart = false;
                if (drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.capturedata))
                {
                    OSI_LOGI(0, "cam gc0310Info.captureDrops %d ", gc0310Info.captureDrops);
                    OSI_LOGI(0, "cam pictureDrop %d ", pictureDrop);
                    if (pictureDrop < gc0310Info.captureDrops)
                    {
                        mask.fend = 1;
                        drvCamSetIrqMask(mask);
                        gc0310Info.camCapStatus = camCaptureState1;
                        pictureDrop++;
                    }
                    else
                    {
                        gc0310Info.camCapStatus = camCaptureIdle;
                        osiSemaphoreRelease(gc0310Info.cam_sem_capture);
                        if (pictureDrop >= gc0310Info.captureDrops)
                            pictureDrop = 0;
                        gc0310Info.isFirst = false;
                    }
                }
            }
        }
        break;
    case campPreviewState1:
        if (cause.b.vsync_f)
        {
            drvCamClrIrqMask();
            drvCamCmdSetFifoRst();
            if (gc0310Info.isCamIfcStart == true)
            {
                drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
                gc0310Info.isCamIfcStart = false;
            }
            drvCampStartTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
            gc0310Info.isCamIfcStart = true;
            mask.overflow = 1;
            mask.dma = 1;
            drvCamSetIrqMask(mask);
            gc0310Info.camCapStatus = campPreviewState2;
        }
        break;
    case campPreviewState2:
        if (cause.b.ovfl)
        {
            OSI_LOGI(0, "cam ovfl ");
            drvCameraControllerEnable(false);
            drvCamCmdSetFifoRst();
            drvCameraControllerEnable(true);
            prev_ovf = 1;
        }
        if (cause.b.dma_done)
        {
            drvCamClrIrqMask();
            if (prev_ovf == 1)
            {
                prev_ovf = 0;
                if (gc0310Info.isCamIfcStart)
                    drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
                gc0310Info.camCapStatus = campPreviewState1;
                mask.fend = 1;
                drvCamSetIrqMask(mask);
                return;
            }
            if (gc0310Info.isCamIfcStart == true)
            {
                gc0310Info.isCamIfcStart = false;
                if (drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]))
                {
                    OSI_LOGI(0, "cam recv data %d", gc0310Info.page_turn);
                    gc0310Info.page_turn = 1 - gc0310Info.page_turn;
                    osiSemaphoreRelease(gc0310Info.cam_sem_preview);
                    gc0310Info.isFirst = false;
                    OSI_LOGI(0, "cam preview_page data %d", gc0310Info.preview_page);
                    if (--gc0310Info.preview_page)
                    {
                        gc0310Info.camCapStatus = campPreviewState1;
                        mask.fend = 1;
                        drvCamSetIrqMask(mask);
                    }
                    else
                    {
                        gc0310Info.camCapStatus = camCaptureIdle;
                    }
                }
                else
                {
                    gc0310Info.camCapStatus = campPreviewState1;
                    mask.fend = 1;
                    drvCamSetIrqMask(mask);
                    OSI_LOGI(0, "cam dma stop error");
                }
            }
        }
        break;
    default:
        break;
    }
}

bool camGc0310Open(void)
{
    OSI_LOGI(0, "camGc0310Open");
    osiRequestSysClkActive(&gcCamCLK);
    drvCamSetPdn(false);
    osiDelayUS(1000);
    prvCamGc0310PowerOn();
    drvCamSetMCLK(gc0310Info.sensorClk);
    osiDelayUS(1000);
    drvCamSetPdn(true);
    osiDelayUS(1000);
    drvCamSetPdn(false);
    if (!prvCamGc0310I2cOpen(gc0310Info.i2c_name, gc0310Info.baud))
    {
        OSI_LOGE(0, "cam prvCamGc0310I2cOpen fail");
        prvCamGc0310I2cClose();
        prvCamGc0310PowerOff();
        return false;
    }
    if (!prvCamGc0310Rginit(&gc0310Info))
    {
        OSI_LOGE(0, "cam prvCamGc0310Rginit fail");
        prvCamGc0310I2cClose();
        prvCamGc0310PowerOff();
        return false;
    }
    drvCampRegInit(&gc0310Info);
    switch (gc0310Info.sensorType)
    {
    case camSpi_In:
        drvCamSetIrqHandler(prvSpiCamIsrCB, NULL);
        break;
    case camCsi_In:
        drvCamSetIrqHandler(prvCsiCamIsrCB, NULL);
        break;
    default:
        break;
    }
    gc0310Info.isCamOpen = true;
    drvCameraControllerEnable(true);
    return true;
}

void camGc0310Close(void)
{
    if (gc0310Info.isCamOpen)
    {
        osiReleaseClk(&gcCamCLK);
        drvCamSetPdn(true);
        osiDelayUS(1000);
        drvCamDisableMCLK();
        osiDelayUS(1000);
        prvCamGc0310PowerOff();
        drvCamSetPdn(false);
        prvCamGc0310I2cClose();
        drvCampRegDeInit();
        drvCamDisableIrq();
        gc0310Info.isFirst = true;
        gc0310Info.isCamOpen = false;
    }
    else
    {
        gc0310Info.isCamOpen = false;
    }
}

void camGc0310GetId(uint8_t *data, uint8_t len)
{
    if (gc0310Info.isCamOpen)
    {
        if (data == NULL || len < 1)
            return;
        prvCamWriteOneReg(0xf4, 0x1c);
        osiDelayUS(2);
        prvCamReadReg(0xf0, data, len);
    }
}

bool camGc0310CheckId(void)
{
    uint8_t sensorID[2] = {0, 0};
    if (!gc0310Info.isCamOpen)
    {
        prvCamGc0310PowerOn();
        drvCamSetMCLK(gc0310Info.sensorClk);
        osiDelayUS(1000);
        drvCamSetPdn(false);
        osiDelayUS(1000);
        drvCamSetPdn(true);
        osiDelayUS(1000);
        drvCamSetPdn(false);
        if (!prvCamGc0310I2cOpen(gc0310Info.i2c_name, gc0310Info.baud))
        {
            OSI_LOGE(0, "cam prvCamGc0310I2cOpen failed");
            return false;
        }
        if (!gc0310Info.isCamOpen)
        {
            gc0310Info.isCamOpen = true;
        }
        camGc0310GetId(sensorID, 2);
        OSI_LOGI(0, "cam get id 0x%x,0x%x", sensorID[0], sensorID[1]);
        if ((gc0310Info.sensorid[0] == sensorID[0]) && (gc0310Info.sensorid[1] == sensorID[1]))
        {
            OSI_LOGI(0, "check id successful");
            camGc0310Close();
            return true;
        }
        else
        {
            camGc0310Close();
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

void camGc0310CaptureImage(uint32_t size)
{
    if (size != 0)
    {
        cameraIrqCause_t mask = {0, 0, 0, 0};
        if (gc0310Info.isCamIfcStart == true)
        {
            drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.capturedata);
            gc0310Info.isCamIfcStart = false;
        }
        gc0310Info.nPixcels = size;
        gc0310Info.camCapStatus = camCaptureState1;
        mask.fend = 1;
        drvCamSetIrqMask(mask);
    }
}

void camGc0310PrevStart(uint32_t size)
{
    OSI_LOGI(0, "camGc0310PrevStart gc0310Info.preview_page=%d ", gc0310Info.preview_page);
    if (gc0310Info.preview_page == 0)
    {
        cameraIrqCause_t mask = {0, 0, 0, 0};
        drvCamSetIrqMask(mask);
        if (gc0310Info.isCamIfcStart == true)
        {
            drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
            gc0310Info.isCamIfcStart = false;
        }
        gc0310Info.nPixcels = size;
        gc0310Info.camCapStatus = campPreviewState1;
        gc0310Info.isStopPrev = false;
        gc0310Info.preview_page = 2;
        mask.fend = 1;
        drvCamSetIrqMask(mask);
    }
}

void camGc0310PrevNotify(void)
{
    if (gc0310Info.isCamOpen && !gc0310Info.isStopPrev)
    {
        if (gc0310Info.preview_page == 0)
        {
            cameraIrqCause_t mask = {0, 0, 0, 0};

            gc0310Info.camCapStatus = campPreviewState1;
            gc0310Info.preview_page++;
            mask.fend = 1;
            drvCamSetIrqMask(mask);
        }
        else
        {
            if (gc0310Info.preview_page < 2)
                gc0310Info.preview_page++;
        }
    }
}

void camGc0310StopPrev(void)
{
    drvCamClrIrqMask();
    if (gc0310Info.isCamIfcStart == true)
    {
        drvCampStopTransfer(gc0310Info.nPixcels, gc0310Info.previewdata[gc0310Info.page_turn]);
        gc0310Info.isCamIfcStart = false;
    }
    drvCameraControllerEnable(false);
    gc0310Info.camCapStatus = camCaptureIdle;
    gc0310Info.isStopPrev = true;
    gc0310Info.preview_page = 0;
}

void camGc0310SetFalsh(uint8_t level)
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

void camGc0310Brightness(uint8_t level)
{
}

void camGc0310Contrast(uint8_t level)
{
}

void camGc0310ImageEffect(uint8_t effect_type)
{
}
void camGc0310Ev(uint8_t level)
{
}

void camGc0310AWB(uint8_t mode)
{
}

void camGc0310GetSensorInfo(sensorInfo_t **pSensorInfo)
{
    OSI_LOGI(0, "CamGc0310GetSensorInfo %08x", &gc0310Info);
    *pSensorInfo = &gc0310Info;
}

bool camGc0310Reg(SensorOps_t *pSensorOpsCB)
{
    if (camGc0310CheckId())
    {
        pSensorOpsCB->cameraOpen = camGc0310Open;
        pSensorOpsCB->cameraClose = camGc0310Close;
        pSensorOpsCB->cameraGetID = camGc0310GetId;
        pSensorOpsCB->cameraCaptureImage = camGc0310CaptureImage;
        pSensorOpsCB->cameraStartPrev = camGc0310PrevStart;
        pSensorOpsCB->cameraPrevNotify = camGc0310PrevNotify;
        pSensorOpsCB->cameraStopPrev = camGc0310StopPrev;
        pSensorOpsCB->cameraSetAWB = camGc0310AWB;
        pSensorOpsCB->cameraSetBrightness = camGc0310Brightness;
        pSensorOpsCB->cameraSetContrast = camGc0310Contrast;
        pSensorOpsCB->cameraSetEv = camGc0310Ev;
        pSensorOpsCB->cameraSetImageEffect = camGc0310ImageEffect;
        pSensorOpsCB->cameraGetSensorInfo = camGc0310GetSensorInfo;
        pSensorOpsCB->cameraFlashSet = camGc0310SetFalsh;
        return true;
    }
    return false;
}
