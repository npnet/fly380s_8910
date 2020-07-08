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

#define BBAT_NAME OSI_MAKE_TAG('B', 'B', 'A', 'T')
#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('B', 'B', 'A', 'T')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "hwregs.h"
#undef CAMERA_PWDN // HACK: duplicated macro in camera.h and pin_reg.h

#include "pin_reg.h"
#include "drv_gpio.h"
#include "cfw.h"
#include "hal_adi_bus.h"
#include <drv_names.h>
#include <drv_uart.h>
#include <drv_axidma.h>
#include <osi_api.h>
#include "diag.h"
#include "string.h"
#include <stdlib.h>
#include "../cmddef.h"
#include "drv_adc.h"
#include "image_sensor.h"
#include "osi_mem.h"
#include "osi_compiler.h"
#include "hal_chip.h"
#include "audio_device.h"
#include "audio_recorder.h"
#include "audio_writer.h"
#include "drv_rtc.h"
#include <osi_log.h>

#define BBAT_FAILURE 0x01
#define BBAT_SUCCESS 0x00
#define AUTOTEST_NVRAM_LEN 8
#define AUTO_TEST_ALARM OSI_MAKE_TAG('A', 'U', 'T', 'A')

static osiPmSource_t *gBbatPmSource = NULL;

typedef uint32_t (*DIAG_AUTOTEST_CALLBACK)(
    const uint8_t *src_ptr,
    uint16_t src_len);

typedef enum _DEVICE_AUTOTEST_ID_E
{
    DEVICE_AUTOTEST_KEYPAD = 1,
    DEVICE_AUTOTEST_LCD_SPI = 3,
    DEVICE_AUTOTEST_CAMERA_IIC = 4,
    DEVICE_AUTOTEST_CAMERA_OPENCLOSE = 5,
    DEVICE_AUTOTEST_CAMERA_MIPI = 6,
    DEVICE_AUTOTEST_GPIO = 7, //and TP test
    DEVICE_AUTOTEST_SIM = 9,
    DEVICE_AUTOTEST_MIC = 10,
    DEVICE_AUTOTEST_SPEAK = 11,    //speak&&receiver&&earphone
    DEVICE_AUTOTEST_CAMFLASH = 12, //DEVICE_AUTOTEST_MISC = 12,  //lcd backlight,vibrator,keypadbacklight
    DEVICE_AUTOTEST_FM = 13,
    DEVICE_AUTOTEST_IIC_DEV = 17, //speak&&receiver&&earphone
    DEVICE_AUTOTEST_CHARGE = 18,
    DEVICE_AUTOTEST_NVRAM_R = 19,
    DEVICE_AUTOTEST_NVRAM_W = 20,
    DEVICE_AUTOTEST_RTC = 23,
    DEVICE_AUTOTEST_ADC = 24, //0x18
    DEVICE_AUTOTEST_CHECK = 28,
    DEVICE_AUTOTEST_VIB = 30,
    DEVICE_AUTOTEST_USB = 31,
    DEVICE_AUTOTEST_UART = 32,
    DEVICE_AUTOTEST_MAX_F
} DEVICE_AUTOTEST_ID_E;

typedef enum
{
    MN_DUAL_SYS_1 = 0,
    MN_DUAL_SYS_2 = 1,
    MN_DUAL_SYS_MAX
} MN_DUAL_SYS_E;

typedef enum
{
    KEYPAD_REST_N = 0x01,
    KEYPAD_KEYIN0,
    KEYPAD_EXT_RSTN_IN,
    KEYPAD_PWR

} AUTOTEST_KEYPAD_E;

typedef enum
{
    SIM_OK = 0x00,
    SIM_FAIL
} AUTOTEST_SIM_E;

typedef enum
{
    ADC_GET_VALUE = 0x0,
    ADC_GET_RAW
} AUTOTEST_ADC_E;

typedef enum
{
    VIB_OPEN = 0x01,
    VIB_CLOSE
} AUTOTEST_VIB_E;

typedef enum
{
    USB_OTG_DISABLE = 0x01,
    USB_READ_ID = 0x2,
    USB_VBUS_ON = 0x3,
    USB_VBUS_OFF
} AUTOTEST_USB_E;

typedef enum
{
    CAM_OPEN = 0x01,
    CAM_CLOSE = 0x03
} AUTOTEST_CAMOPENCLOSE_E;

typedef enum
{
    CAM_MIPI_OPEN = 0x01,
    CAM_MIPI_READ,
    CAM_MIPI_CLOSE
} AUTOTEST_MIPICAM_E;

typedef enum
{
    GPIO_GET_VALUE = 0x0,
    GPIO_SET_VALUE = 0x1,
    GPIO_SETBACK = 0x2
} AUTOTEST_GPIO_E;

typedef enum
{
    RTC_OPEN = 0x01,
    RTC_SEARCH,
    RTC_COUNT,
    RTC_CLOSE
} AUTOTEST_RTC_E;

/**---------------------------------------------------------------------------*
 **                         Data Structures                                   *
 **---------------------------------------------------------------------------*/
typedef struct
{
    diagMsgHead_t msg_head;
    uint8_t sim_select;
} diagMsgSim_t;

typedef struct
{
    diagMsgHead_t msg_head;
    uint8_t command;
    uint8_t gpio_id;
    uint8_t gpio_value;
    uint8_t reserved;
} diagMsgGPIO_t;

typedef struct
{
    diagMsgHead_t msg_head;
    uint8_t command;
    uint8_t level;
} diagMsgCAMFLASH_t;

typedef struct
{
    diagMsgHead_t msg_head;
    uint8_t channel;
    uint8_t scale;
} diagMsgADC_t;

typedef struct
{
    diagMsgHead_t msg_head;
    uint8_t command; // mode : 00 speaker ; 01 receiver : 02 eraphone
    uint8_t data;    // operate : 01 paly the inner data ; 02 play use  data from PC ; 03 stop 05 set volume
} diagMsgAudio_t;

typedef struct
{
    diagMsgHead_t msg_head;
    uint16_t factory_name;
    uint16_t worker_name;
    uint16_t work_name;
    uint16_t reserved;
} diagMsgNvram_t;

typedef struct
{
    uint8_t pin_num;
    uint8_t gpio_real; // real gpio
    volatile uint32_t *reg;
    uint32_t value;
} autoTestPmPinFunc_t;

typedef struct
{
    diagMsgHead_t msg_head;
    uint8_t value;
} diagMsgRTC_t;

typedef struct
{
    diagMsgRTC_t rtc_msg;
    bool opened;
} diagRtcContext_t;

#define PIN_NUM_MAX 67
const autoTestPmPinFunc_t autodev_pm_func[] = {
    // virtual gpio NO.          |     real gpio      |     Register addr                  |     goio func |
    {PIN_GPIO0, 0, &hwp_iomux->pad_gpio_0_cfg_reg, 0},
    {PIN_GPIO1, 1, &hwp_iomux->pad_gpio_1_cfg_reg, 0},
    {PIN_GPIO2, 2, &hwp_iomux->pad_gpio_2_cfg_reg, 0},
    {PIN_GPIO3, 3, &hwp_iomux->pad_gpio_3_cfg_reg, 0},
    {PIN_GPIO4, 4, &hwp_iomux->pad_gpio_4_cfg_reg, 0},
    {PIN_GPIO5, 5, &hwp_iomux->pad_gpio_5_cfg_reg, 0},
    {PIN_GPIO6, 6, NULL, 0},
    {PIN_GPIO7, 7, &hwp_iomux->pad_gpio_7_cfg_reg, 0},
    {PIN_GPO0, 8, &hwp_iomux->pad_gpio_8_cfg_reg, 0},
    {PIN_GPO1, 9, &hwp_iomux->pad_gpio_9_cfg_reg, 0},
    {PIN_GPO2, 10, &hwp_iomux->pad_gpio_10_cfg_reg, 0},
    {PIN_GPO3, 11, &hwp_iomux->pad_gpio_11_cfg_reg, 0},
    {PIN_GPO4, 12, &hwp_iomux->pad_gpio_12_cfg_reg, 0},
    {PWM_LPG_OUT, 13, &hwp_iomux->pad_gpio_13_cfg_reg, 0}, //13
    {I2C_M2_SCL, 14, &hwp_iomux->pad_gpio_14_cfg_reg, 0},
    {I2C_M2_SDA, 15, &hwp_iomux->pad_gpio_15_cfg_reg, 0},
    {DEBUG_HOST_RX, 16, &hwp_iomux->pad_debug_host_rx_cfg_reg, 1},
    {DEBUG_HOST_CLK, 17, &hwp_iomux->pad_debug_host_clk_cfg_reg, 1},
    {UART_1_CTS, 18, &hwp_iomux->pad_gpio_18_cfg_reg, 0},
    {UART_1_RTS, 19, &hwp_iomux->pad_gpio_19_cfg_reg, 0},
    {UART_2_RXD, 20, &hwp_iomux->pad_gpio_20_cfg_reg, 0},
    {UART_2_TXD, 21, &hwp_iomux->pad_gpio_21_cfg_reg, 0},
    {UART_2_CTS, 22, &hwp_iomux->pad_gpio_22_cfg_reg, 0},
    {UART_2_RTS, 23, &hwp_iomux->pad_gpio_23_cfg_reg, 0},
    {SDMMC1_CMD, 24, &hwp_iomux->pad_sdmmc1_cmd_cfg_reg, 1},
    {SDMMC1_DATA_0, 25, &hwp_iomux->pad_sdmmc1_data_0_cfg_reg, 1},
    {SDMMC1_DATA_1, 26, &hwp_iomux->pad_sdmmc1_data_1_cfg_reg, 1},
    {SDMMC1_DATA_2, 27, &hwp_iomux->pad_sdmmc1_data_2_cfg_reg, 1},
    {SDMMC1_DATA_3, 28, &hwp_iomux->pad_sdmmc1_data_3_cfg_reg, 1},
    {SPI_FLASH_SIO_1, 29, &hwp_iomux->pad_spi_flash_sio_1_cfg_reg, 1},
    {SPI_FLASH_SIO_2, 30, &hwp_iomux->pad_spi_flash_sio_2_cfg_reg, 1},
    {SPI_FLASH_SIO_3, 31, &hwp_iomux->pad_spi_flash_sio_3_cfg_reg, 1},
    {SPI_LCD_SIO, 0, &hwp_iomux->pad_spi_lcd_sio_cfg_reg, 1},
    {SPI_LCD_SDC, 1, &hwp_iomux->pad_spi_lcd_sdc_cfg_reg, 1},
    {SPI_LCD_CLK, 2, &hwp_iomux->pad_spi_lcd_clk_cfg_reg, 1},
    {SPI_LCD_CS, 3, &hwp_iomux->pad_spi_lcd_cs_cfg_reg, 1},
    {SPI_LCD_SELECT, 4, &hwp_iomux->pad_spi_lcd_select_cfg_reg, 1},
    {LCD_FMARK, 5, &hwp_iomux->pad_lcd_fmark_cfg_reg, 1},
    {LCD_RSTB, 6, &hwp_iomux->pad_lcd_rstb_cfg_reg, 1},
    {AP_JTAG_TCK, 8, &hwp_iomux->pad_ap_jtag_tck_cfg_reg, 5},
    {AP_JTAG_TRST, 9, &hwp_iomux->pad_ap_jtag_trst_cfg_reg, 5},
    {AP_JTAG_TMS, 10, &hwp_iomux->pad_ap_jtag_tms_cfg_reg, 5},
    {AP_JTAG_TDI, 11, &hwp_iomux->pad_ap_jtag_tdi_cfg_reg, 5},
    {AP_JTAG_TDO, 12, &hwp_iomux->pad_ap_jtag_tdo_cfg_reg, 5},
    {I2C_M1_SCL, 16, &hwp_iomux->pad_i2c_m1_scl_cfg_reg, 4},
    {I2C_M1_SDA, 17, &hwp_iomux->pad_i2c_m1_sda_cfg_reg, 4},
    {CAMERA_RST_L, 18, &hwp_iomux->pad_camera_rst_l_cfg_reg, 4},
    {CAMERA_PWDN, 19, &hwp_iomux->pad_camera_pwdn_cfg_reg, 4},
    {CAMERA_REF_CLK, 20, &hwp_iomux->pad_camera_ref_clk_cfg_reg, 4},
    {SPI_CAMERA_SI_0, 21, &hwp_iomux->pad_spi_camera_si_0_cfg_reg, 4},
    {SPI_CAMERA_SI_1, 22, &hwp_iomux->pad_spi_camera_si_1_cfg_reg, 4},
    {SPI_CAMERA_SCK, 23, &hwp_iomux->pad_spi_camera_sck_cfg_reg, 4},
    {AUD_DA_SYNC, 27, &hwp_iomux->pad_aud_ad_sync_cfg_reg, 2},
    {AUD_DA_D1, 28, &hwp_iomux->pad_aud_da_d1_cfg_reg, 2},
    {AUD_DA_D0, 29, &hwp_iomux->pad_aud_da_d0_cfg_reg, 2},
    {AUD_AD_SYNC, 30, &hwp_iomux->pad_aud_ad_sync_cfg_reg, 2},
    {AUD_AD_D0, 31, &hwp_iomux->pad_aud_ad_d0_cfg_reg, 2},
    {SIM_2_CLK, 29, &hwp_iomux->pad_sim_2_clk_cfg_reg, 2},
    {SIM_2_DIO, 30, &hwp_iomux->pad_sim_2_dio_cfg_reg, 2},
    {SIM_2_RST, 31, &hwp_iomux->pad_sim_2_rst_cfg_reg, 2}};

autoTestPmPinFunc_t autodev_restore[PIN_NUM_MAX];

static uint16_t keypadcode = 0;

static DIAG_AUTOTEST_CALLBACK diagAutotestCallback[DEVICE_AUTOTEST_MAX_F] = {NULL};

static const unsigned char sin1k_44k_88bytes[] = {
    0x92, 0x02, 0xcb, 0x0b, 0xd0, 0x14, 0x1d, 0x1d,
    0xfc, 0x24, 0x17, 0x2c, 0x4a, 0x32, 0x69, 0x37,
    0x92, 0x3b, 0x4e, 0x3e, 0x22, 0x40, 0x56, 0x40,
    0x92, 0x3f, 0x12, 0x3d, 0x88, 0x39, 0x10, 0x35,
    0xf0, 0x2e, 0x51, 0x28, 0xce, 0x20, 0x7f, 0x18,
    0xd5, 0x0f, 0xdA, 0x06, 0xdf, 0xfd, 0xa4, 0xf4,
    0xa2, 0xeb, 0x39, 0xe3, 0x57, 0xdb, 0x3d, 0xd4,
    0x1f, 0xce, 0xe2, 0xc8, 0xb1, 0xc4, 0xc0, 0xc1,
    0xec, 0xbf, 0xc1, 0xbf, 0xa4, 0xc0, 0xf2, 0xc2,
    0x18, 0xc6, 0xc2, 0xca, 0xc8, 0xd0, 0x36, 0xd7,
    0xbb, 0xde, 0xe6, 0xe6, 0xa5, 0xef, 0xa6, 0xf8, //
};

static diagRtcContext_t diagRtc_ctx = {.opened = false};

static void _restoreIomuxValue(void)
{
    uint8_t k;

    memcpy((uint8_t *)&autodev_restore[0], (uint8_t *)&autodev_pm_func[0], sizeof(autodev_pm_func));
    for (k = 0; k < PIN_NUM_MAX; k++)
    {
        autodev_restore[k].value = 0xffffffff;
    }
}

void _autotestGenerateRspMsg(diagMsgHead_t *msgHeadPtr) // Inputed pointer of message header struct.
{
    diagMsgHead_t head =
        {
            .seq_num = msgHeadPtr->seq_num,
            .len = sizeof(diagMsgHead_t),
            .type = msgHeadPtr->type,
            .subtype = msgHeadPtr->subtype,
        };
    //diagOutputPacket(&head, sizeof(head));
    diagOutputPacket2(&head, NULL, 0);
    return;
}

void _autotestGenerateRspDataMsg(diagMsgHead_t *msgHeadPtr, const void *data, unsigned size) // Inputed pointer of message header struct.
{
    diagMsgHead_t head =
        {
            .seq_num = msgHeadPtr->seq_num,
            .len = sizeof(diagMsgHead_t),
            .type = msgHeadPtr->type,
            .subtype = msgHeadPtr->subtype,
        };
    //diagOutputPacket(&head, sizeof(head));
    diagOutputPacket2(&head, data, size);
    return;
}

static uint8_t _getIndexOfGpio(uint8_t virtualGpio)
{
    uint8_t k;
    for (k = 0; k < OSI_ARRAY_SIZE(autodev_pm_func); k++)
    {
        if (virtualGpio == autodev_pm_func[k].pin_num)
            return k;
    }
    return 0xff;
}

static uint32_t _handleGpioAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    uint8_t value = 0, k;
    uint8_t virtuGpio = 0;
    uint8_t result = BBAT_SUCCESS;
    uint8_t realGpio = 0;

    diagMsgHead_t respMsg;

    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;
    diagMsgGPIO_t *msgHeadGpio = (diagMsgGPIO_t *)src_ptr;
    OSI_LOGI(0, "bbat: _handleGpioAutotest");

    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));

    if (msgHeadGpio->gpio_id > PIN_NUM_MAX)
    {
        result = BBAT_FAILURE;
        goto Fail;
    }
    virtuGpio = _getIndexOfGpio(msgHeadGpio->gpio_id);

    if (virtuGpio == 0xff)
    {
        return 0;
    }
    realGpio = autodev_pm_func[virtuGpio].gpio_real;

    _restoreIomuxValue();
    switch ((AUTOTEST_GPIO_E)(msgHeadGpio->command))
    {
    case GPIO_GET_VALUE:
        if ((autodev_pm_func[virtuGpio].reg) != NULL)
        {
            (autodev_restore[virtuGpio].value) = *(autodev_pm_func[virtuGpio].reg);
            *(autodev_pm_func[virtuGpio].reg) = (autodev_pm_func[virtuGpio].value);
        }

        hwp_gpio1->gpio_oen_set_in = (1 << realGpio);
        value = hwp_gpio1->gpio_val_reg & (1 << realGpio);
        break;

    case GPIO_SET_VALUE:

        if ((autodev_pm_func[virtuGpio].reg) != NULL)
        {

            (autodev_restore[virtuGpio].value) = *(autodev_pm_func[virtuGpio].reg);

            *(autodev_pm_func[virtuGpio].reg) = (autodev_pm_func[virtuGpio].value);
        }

        if (msgHeadGpio->gpio_value)
            hwp_gpio1->gpio_set_reg = (1 << realGpio);
        else
            hwp_gpio1->gpio_clr_reg = (1 << realGpio);
        hwp_gpio1->gpio_oen_set_out = (1 << realGpio);

        break;
    case GPIO_SETBACK:
    {
        for (k = 0; k < PIN_NUM_MAX; k++)
        {
            if (0xffffffff != autodev_restore[k].value)
            {

                if ((autodev_pm_func[virtuGpio].reg) != NULL)
                    *(autodev_restore[k].reg) = autodev_restore[k].value;
            }
        }
    }
    break;

    default:
        result = BBAT_FAILURE;
        break;
    }
Fail:
    if (BBAT_FAILURE == result)
    {
        msg_head->subtype = BBAT_FAILURE;
        _autotestGenerateRspMsg(msg_head);
    }
    else
    {
        respMsg.seq_num = msg_head->seq_num;
        respMsg.len = sizeof(diagMsgHead_t) + 1;
        respMsg.type = msg_head->type,
        respMsg.subtype = result,

        diagOutputPacket2(&respMsg, &value, 1);
    }
    return 1;
}

static uint32_t _handleAdcAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    uint16_t value = 0;
    uint8_t result = BBAT_SUCCESS;

    diagMsgHead_t respMsg;
    diagMsgADC_t *msg_head = (diagMsgADC_t *)src_ptr;
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));
    OSI_LOGI(0, "bbat: _handleAdcAutotest");

    drvAdcInit();
    value = (uint16_t)drvAdcGetChannelVolt(msg_head->channel, msg_head->scale);
    OSI_LOGI(0, "bbat: _handleAdcAutotest,channel:%d,scale:%d", msg_head->channel, msg_head->scale);

    if (BBAT_FAILURE == result)
    {
        OSI_LOGI(0, "bbat: _handleAdcAutotest,failure");
        respMsg.subtype = BBAT_FAILURE;
        _autotestGenerateRspMsg(&respMsg);
    }
    else
    {
        OSI_LOGI(0, "bbat: _handleAdcAutotest,success");
        //respMsg.seq_num = msg_head->msg_head.seq_num;
        respMsg.len = sizeof(diagMsgHead_t) + 2;
        //respMsg.type = msg_head->msg_head.type;
        respMsg.subtype = result;

        diagOutputPacket2(&respMsg, &value, 2);
    }
    return 1;
}
bool gBBATMode = false;
uint8_t gPBkeyStatus = 1;

static uint32_t _handleKeypadAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)

{
    uint8_t command = 0;
    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;

    diagMsgHead_t respMsg;
    uint8_t keyvalue[6] = {0xff, 0xff, 00, 0xff, 0, 0};
    REG_KEYPAD_KP_CTRL_T keypad0;
    REG_KEYPAD_KP_CTRL_T keypad0_temp;
    command = *(uint8_t *)(msg_head + 1);
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));
    OSI_LOGI(0, "bbat: _handleKeypadAutotest");

    switch (command)
    {
    case KEYPAD_KEYIN0:
    {
        OSI_LOGI(0, "bbat:  Keyin0 test");

        keypad0.v = hwp_keypad->kp_ctrl;
        keypad0_temp.v = keypad0.v;
        keypad0.b.kp_en = 1;
        keypad0.b.kp_dbn_time = 0;
        hwp_keypad->kp_ctrl = keypad0.v;
        osiDelayUS(200000);
        respMsg.subtype = 0;
        if ((hwp_keypad->kp_data_l & 0x1) == 1)
        {
            keyvalue[3] = 1; //pass
        }
        else
        {
            keyvalue[3] = 0; //pass
        }
        keypad0.b.kp_en = 0;
        hwp_keypad->kp_ctrl = keypad0_temp.v;
        diagOutputPacket2(&respMsg, keyvalue, 6);
    }
    break;

    case KEYPAD_REST_N:
    {
        if (keypadcode)
        {
            respMsg.subtype = BBAT_SUCCESS;
            _autotestGenerateRspMsg(&respMsg);

            keypadcode = 0;
        }
        else
        {
            respMsg.subtype = BBAT_SUCCESS;
            _autotestGenerateRspMsg(&respMsg);
        }
    }
    break;

    case KEYPAD_EXT_RSTN_IN:
    {
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;
    case KEYPAD_PWR:
    {
        respMsg.subtype = gPBkeyStatus;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;
    default:
        respMsg.subtype = 1; //ATCMD_BADCOMMAND;
        //_autotestGenerateRspMsg(  msg_head);
        respMsg.subtype = 0x5A;
        _autotestGenerateRspMsg(&respMsg);
        break;
    }

    return 1;
}

static uint32_t _handleSIMAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    AUTOTEST_SIM_E ret = 0;
    uint8_t sim_status = false; //, res;
    diagMsgHead_t respMsg;

    diagMsgSim_t *msg_head = (diagMsgSim_t *)src_ptr;

    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));

    OSI_LOGI(0, "bbat: _handleSIMAutotest");
    switch ((MN_DUAL_SYS_E)(msg_head->sim_select))
    {
    case MN_DUAL_SYS_1:
        sim_status = CFW_GetSimStatus(0);
        if ((sim_status == 1) || (sim_status == 2))
        {
            if (CFW_SIM_TYPE_SOFT == CFW_GetSimPHYType(0))
                ret = SIM_FAIL;
            else
                ret = SIM_OK;
        }
        else
        {
            ret = SIM_FAIL;
        }
        break;

    case MN_DUAL_SYS_2:
        sim_status = CFW_GetSimStatus(1);
        if ((sim_status == 1) || (sim_status == 2))
        {
            if (CFW_SIM_TYPE_SOFT == CFW_GetSimPHYType(1))
                ret = SIM_FAIL;
            else
                ret = SIM_OK;
        }
        else
        {
            ret = SIM_FAIL;
        }
        break;

    default:
        ret = SIM_FAIL;
        break;
    }

    respMsg.len = sizeof(diagMsgHead_t) + 1;
    respMsg.subtype = ret;
    //res = (uint8_t)ret;
    diagOutputPacket2(&respMsg, NULL, 0);

    return 1;
}

static uint32_t _handleAutotestCheck(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;

    diagMsgHead_t respMsg;
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));
    respMsg.subtype = BBAT_SUCCESS;
    _autotestGenerateRspMsg(&respMsg);
    return 1;
}

uint32_t _autotestReadTestResult(void *result_buf_ptr, uint32_t buf_len)
{
    return BBAT_SUCCESS;
}

static int32_t _autotestReadNV(uint8_t *buffer)
{
    uint8_t nvitem[64] = {0};
    uint32_t nv_length = 64;
    uint32_t return_code = BBAT_SUCCESS;

    return_code = _autotestReadTestResult((void *)nvitem, nv_length);

    if (return_code == BBAT_SUCCESS)
    {
        memcpy(buffer, nvitem + sizeof(nvitem) - AUTOTEST_NVRAM_LEN, AUTOTEST_NVRAM_LEN);
    }

    return return_code;
}

static uint32_t _autotestResultRead(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    uint8_t result = 0;
    uint8_t read_data[AUTOTEST_NVRAM_LEN + 1];
    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;

    diagMsgHead_t respMsg;
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));
    result = _autotestReadNV(read_data);
    if (BBAT_SUCCESS == result)
    {
        //respMsg.seq_num = msg_head->seq_num;
        respMsg.len = sizeof(diagMsgHead_t) + AUTOTEST_NVRAM_LEN;
        //respMsg.type = msg_head->type;
        respMsg.subtype = BBAT_SUCCESS;

        diagOutputPacket2(&respMsg, &read_data, AUTOTEST_NVRAM_LEN);
    }
    else
    {
        respMsg.subtype = BBAT_FAILURE;
        _autotestGenerateRspMsg(&respMsg);
    }
    return 1;
}

static int32_t _autotestWriteNV(uint8_t *buffer)
{
    return BBAT_SUCCESS;
}

static uint32_t _autotestResultWrite(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    uint8_t result = BBAT_SUCCESS;
    uint8_t read_data[AUTOTEST_NVRAM_LEN + 1] = {0};
    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;
    diagMsgHead_t respMsg;
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));
    memcpy(read_data, (uint8_t *)(src_ptr + sizeof(diagMsgHead_t)), sizeof(diagMsgNvram_t) - sizeof(diagMsgHead_t));
    result = _autotestWriteNV((uint8_t *)read_data);

    if (BBAT_SUCCESS != result)
    {
        result = BBAT_FAILURE;
    }
    respMsg.subtype = result;
    _autotestGenerateRspMsg(&respMsg);
    return 1;
}

static void _autoTestAlarmCb(drvRtcAlarm_t *alarm, void *ctx)
{
    diagMsgRTC_t *respMsg = (diagMsgRTC_t *)ctx;
    diagOutputPacket2(&respMsg->msg_head, &respMsg->value, 1);
    OSI_LOGI(0, "bbat: auto test rtc alarm expired");
}

static uint32_t _handleRtcAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    diagRtcContext_t *ctx = &diagRtc_ctx;

    diagMsgRTC_t *msg_head = (diagMsgRTC_t *)src_ptr;
    diagMsgRTC_t *respMsg = &ctx->rtc_msg;
    memcpy(&respMsg->msg_head, msg_head, sizeof(diagMsgHead_t));
    respMsg->msg_head.subtype = BBAT_SUCCESS;
    respMsg->value = BBAT_SUCCESS;

    switch (msg_head->value)
    {
    case RTC_OPEN:
    {
        respMsg->msg_head.len = sizeof(diagMsgHead_t);

        if (!ctx->opened)
            ctx->opened = true;
        else
            respMsg->msg_head.subtype = BBAT_FAILURE;

        _autotestGenerateRspMsg(&respMsg->msg_head);
    }
    break;

    case RTC_SEARCH:
    {
        // todo
    }
    break;

    case RTC_COUNT:
    {
        if (ctx->opened)
        {
            drvRtcAlarmOwnerSetCB(AUTO_TEST_ALARM, respMsg, _autoTestAlarmCb);
            int64_t current = osiEpochSecond();
            if (!drvRtcSetAlarm(AUTO_TEST_ALARM, 1, NULL, 0, current + 3, true))
            {
                respMsg->value = BBAT_FAILURE;
                diagOutputPacket2(&respMsg->msg_head, &respMsg->value, 1);
            }
        }
        else
        {
            respMsg->value = BBAT_FAILURE;
            diagOutputPacket2(&respMsg->msg_head, &respMsg->value, 1);
        }
    }
    break;

    case RTC_CLOSE:
    {
        if (ctx->opened)
        {
            ctx->opened = false;
            drvRtcAlarmOwnerSetCB(AUTO_TEST_ALARM, NULL, NULL);
        }
        else
        {
            respMsg->msg_head.subtype = BBAT_FAILURE;
        }
        respMsg->msg_head.len = sizeof(diagMsgHead_t);
        _autotestGenerateRspMsg(&respMsg->msg_head);
    }
    break;

    default:
        OSI_LOGE(0, "bbat: _handleRtcAutotest error!");
        break;
    }

    return 1;
}

static void _diagInitAutotestCallback(void)
{
    int type = 0;

    for (type = 0; type < DEVICE_AUTOTEST_MAX_F; type++)
    {
        diagAutotestCallback[type] = NULL;
    }
}

static uint32_t _handleCamOpenCloseAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    uint8_t command = 0;
    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;
    command = *(uint8_t *)(msg_head + 1);
    diagMsgHead_t respMsg;
    OSI_LOGE(0, "bbat: _handleCamOpenCloseAutotest !");
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));
    switch (command)
    {
    case CAM_OPEN:
    {
        halPmuSetPowerLevel(HAL_POWER_CAMD, SENSOR_VDD_1800MV);
        halPmuSwitchPower(HAL_POWER_CAMD, true, false);
        osiDelayUS(1000);
        halPmuSetPowerLevel(HAL_POWER_CAMA, SENSOR_VDD_2800MV);
        halPmuSwitchPower(HAL_POWER_CAMA, true, false);
        osiDelayUS(1000);
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;
    case CAM_CLOSE:
    {
        halPmuSwitchPower(HAL_POWER_CAMA, false, false);
        osiDelayUS(1000);
        halPmuSwitchPower(HAL_POWER_CAMD, false, false);
        osiDelayUS(1000);
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;
    default:
        OSI_LOGE(0, "bbat: _handleCamOpenCloseAutotest error!");
        break;
    }
    return 1;
}

static uint32_t _handleMipiCamAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    uint8_t command = 0;
    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;
    command = *(uint8_t *)(msg_head + 1);
    diagMsgHead_t respMsg;
    OSI_LOGE(0, "bbat: _handleMipiCamAutotest !");
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));
    switch (command)
    {
    case CAM_MIPI_OPEN:
    {

        if (drvCamInit() == false)
        {
            respMsg.subtype = BBAT_FAILURE;
            _autotestGenerateRspMsg(&respMsg);
            return BBAT_FAILURE;
        }
        if (drvCamPowerOn() == false)
        {
            respMsg.subtype = BBAT_FAILURE;
            _autotestGenerateRspMsg(&respMsg);
            return BBAT_FAILURE;
        }
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;
    case CAM_MIPI_READ:
    {
        uint16_t *pCamPreviewDataBuffer = NULL;
        if (!drvCamCaptureImage(&pCamPreviewDataBuffer))
        {
            respMsg.subtype = BBAT_FAILURE;
            _autotestGenerateRspMsg(&respMsg);
            return BBAT_FAILURE;
        }
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspDataMsg(&respMsg, pCamPreviewDataBuffer, 768);
    }
    break;
    case CAM_MIPI_CLOSE:
    {
        drvCamClose();
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;
    default:
        break;
    }
    return BBAT_SUCCESS;
}

static void _diagRegisterAutotestCallback(uint32_t type, DIAG_AUTOTEST_CALLBACK routine)
{
    if (type < DEVICE_AUTOTEST_MAX_F)
        diagAutotestCallback[type] = routine;
}

static bool _handleDeviceAutotest(const diagMsgHead_t *cmd, void *ctx)
{
    uint8_t cmd_subtype;
    cmd_subtype = cmd->subtype;

    if (cmd_subtype < DEVICE_AUTOTEST_MAX_F)
    {
        if (NULL != diagAutotestCallback[cmd_subtype])
        {
            if (diagAutotestCallback[cmd_subtype]((uint8_t *)cmd, cmd->len)) // has been handled.
            {
                return true;
            }
        }
    }
    return true;
}
static uint32_t _handleUSBAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    uint8_t command = 0;
    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;
    diagMsgHead_t respMsg;
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));

    command = *(uint8_t *)(msg_head + 1);

    switch (command)
    {
    case USB_OTG_DISABLE:
    {

        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;
    case USB_READ_ID:
    {

        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;
    case USB_VBUS_ON:
    {

        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;

    default:
        OSI_LOGE(0, "bbat: _handleVibAutotest error!");
        break;
    }
    return 1;
}

static uint32_t _handleVibAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    uint8_t command = 0;
    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;
    REG_RDA2720M_GLOBAL_VIBR_CTRL0_T vibctrl0;
    command = *(uint8_t *)(msg_head + 1);
    diagMsgHead_t respMsg;

    OSI_LOGE(0, "bbat: _handleVibAutotest !");
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));

    switch (command)
    {
    case VIB_OPEN:
    {
        //the phone use a gpio to control vibrate

        halAdiBusBatchChange(
            &hwp_rda2720mGlobal->vibr_ctrl0,
            REG_FIELD_MASKVAL1(vibctrl0, ldo_vibr_pd, 0),
            HAL_ADI_BUS_CHANGE_END);
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(msg_head);
    }
    break;

    case VIB_CLOSE:
    {

        halAdiBusBatchChange(
            &hwp_rda2720mGlobal->vibr_ctrl0,
            REG_FIELD_MASKVAL1(vibctrl0, ldo_vibr_pd, 1),
            HAL_ADI_BUS_CHANGE_END);
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
    }
    break;

    default:
        OSI_LOGE(0, "bbat: _handleVibAutotest error!");
        break;
    }
    return 1;
}

static uint32_t _handleCamFlashAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    diagMsgHead_t *msg_head = (diagMsgHead_t *)src_ptr;
    diagMsgCAMFLASH_t *msg_falsh = (diagMsgCAMFLASH_t *)src_ptr;
    diagMsgHead_t respMsg;
    osiPanic();
    OSI_LOGE(0, "bbat: _handleCamFlashAutotest !");
    memcpy(&respMsg, msg_head, sizeof(diagMsgHead_t));
    if (msg_falsh->command == 0x04)
    {
        if (msg_falsh->level >= 0 && msg_falsh->level < 16)
        {
            if (msg_falsh->level == 0)
            {
                halPmuSwitchPower(HAL_POWER_CAMFLASH, false, false);
            }
            else
            {
                halPmuSetCamFlashLevel(msg_falsh->level);
                halPmuSwitchPower(HAL_POWER_CAMFLASH, true, false);
            }
            respMsg.subtype = BBAT_SUCCESS;
            _autotestGenerateRspMsg(&respMsg);
        }
        else
        {
            respMsg.subtype = BBAT_FAILURE;
            _autotestGenerateRspMsg(&respMsg);
        }
    }
    return 1;
}

#define UART_TXFIFO_SIZE (128)
#define UART_RXFIFO_SIZE (128)

static void prvUartBbatConfig(void)
{
    REG_ARM_UART_UART_CONF_T conf = {
        .b = {
            .rxrst = 1,
            .txrst = 1,
            .frm_stp = 1,
            .st_check = 1,
        },
    };
    // config uart1 uart3 clock
    hwp_sysCtrl->cfg_clk_uart[1] = 0x1801;
    hwp_sysCtrl->cfg_clk_uart[3] = 0x1801;

    // 0x60003 => 921600
    hwp_uart1->uart_baud = 0x60003;
    hwp_uart1->uart_conf = conf.v;

    hwp_uart3->uart_baud = 0x60003;
    hwp_uart3->uart_conf = conf.v;
}

static void prvUartFifoWriteAll(HWP_ARM_UART_T *hw, const void *data_, unsigned size)
{
    const uint8_t *data = (const uint8_t *)data_;
    uint32_t send = 0;
    while (send < size)
    {
        REG_ARM_UART_UART_TXFIFO_STAT_T uart_txfifo_stat = {hw->uart_txfifo_stat};
        uint32_t tx_fifo_cnt = uart_txfifo_stat.b.tx_fifo_cnt;
        int len = UART_TXFIFO_SIZE - tx_fifo_cnt;
        if (len > (size - send))
            len = size - send;
        for (unsigned n = 0; n < len; ++n)
            hw->uart_tx = data[send + n];
        send += len;
    }
}

static uint32_t prvUartFifoRead(HWP_ARM_UART_T *hw, void *data_, unsigned size)
{
    uint8_t *data = (uint8_t *)data_;
    REG_ARM_UART_UART_RXFIFO_STAT_T uart_rxfifo_stat = {hw->uart_rxfifo_stat};
    int bytes = uart_rxfifo_stat.b.rx_fifo_cnt;
    if (size > bytes)
        size = bytes;
    for (unsigned n = 0; n < bytes; ++n)
        *data++ = hw->uart_rx;
    return bytes;
}

static uint8_t prvUartTest(void)
{
    OSI_LOGI(0, "BBAT uart loopback test");

    const char uart_test_message[] = "bbat-uart-test\r\n";
    const uint32_t message_len = strlen(uart_test_message);
    char recv[64]; // enough buffer

    // init uart{1,3}
    prvUartBbatConfig();

    // write data 1 => 3
    prvUartFifoWriteAll(hwp_uart1, uart_test_message, message_len);

    uint32_t got = 0;
    uint32_t start_time = osiUpTimeUS();
    // read uart3
    while (got < message_len && osiUpTimeUS() - start_time < 2000)
        got += prvUartFifoRead(hwp_uart3, &recv[got], message_len - got);
    if (got != message_len)
    {
        OSI_LOGE(0, "BBAT uart test fail, uart3 got %u", got);
        return BBAT_FAILURE;
    }

    // write data 3 => 1
    prvUartFifoWriteAll(hwp_uart3, recv, got);
    got = 0;
    memset(recv, 0, 64);
    start_time = osiUpTimeUS();
    // read uart1
    while (got < message_len && osiUpTimeUS() - start_time < 2000)
        got += prvUartFifoRead(hwp_uart1, &recv[got], message_len - got);
    if (got != message_len)
    {
        OSI_LOGE(0, "BBAT uart test fail, uart1 got %u", got);
        return BBAT_FAILURE;
    }

    if (strncmp(uart_test_message, recv, message_len) != 0)
    {
        recv[got] = 0;
        OSI_LOGXE(OSI_LOGPAR_SS, 0, "BBAT uart test fail, data buffer mismatch, (%s/%s)",
                  uart_test_message, recv);
        return BBAT_FAILURE;
    }

    OSI_LOGI(0, "BBAT uart loopback success");
    return BBAT_SUCCESS;
}

static uint32_t _handleUartAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    diagMsgHead_t respMsg = *((diagMsgHead_t *)src_ptr);
    respMsg.subtype = prvUartTest();
    _autotestGenerateRspMsg(&respMsg);
    return 1;
}

#define BBAT_SPEAKER_BUF_SIZE (88 * 8)
#define BBAT_MIC_BUF_SIZE (400)
#define BBAT_MIC_BUF_COUNT (4)
#define BBAT_MIC_BUF_SIZE_ALL (BBAT_MIC_BUF_SIZE * BBAT_MIC_BUF_COUNT)

static bool prvPlayTest(audevOutput_t dev, const void *data)
{
    osiBufSize32_t *buf = (osiBufSize32_t *)malloc(sizeof(osiBufSize32_t) + BBAT_SPEAKER_BUF_SIZE);
    if (buf == NULL)
        return false;

    if (data == NULL)
        data = sin1k_44k_88bytes;

    buf->size = BBAT_SPEAKER_BUF_SIZE;
    char *bufdata = (char *)buf->data;
    for (int n = 0; n < 8; n++)
        memcpy(bufdata + (88 * n), data, 88);

    bool ok = audevStartPlayTest(dev, buf);
    free(buf);
    return ok;
}

static uint32_t _handleSpeakerAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8_t.
)
{
    unsigned char *pcm_buffer;
    audevOutput_t outdev;
    diagMsgHead_t respMsg;
    diagMsgAudio_t *msg_head_speaker = (diagMsgAudio_t *)src_ptr;
    memcpy(&respMsg, msg_head_speaker, sizeof(diagMsgHead_t));

    int pcm_len = (uint32_t)(msg_head_speaker->msg_head.len - 8 - 2);
    OSI_LOGI(0, "bbat speaker test, command=%d data=%d lens=%d",
             msg_head_speaker->command, msg_head_speaker->data, pcm_len);

    switch (msg_head_speaker->data)
    {
    case 0x1: //play local data
        if (msg_head_speaker->command == 0)
            outdev = AUDEV_OUTPUT_SPEAKER;
        else if (msg_head_speaker->command == 1)
            outdev = AUDEV_OUTPUT_RECEIVER;
        else
            outdev = AUDEV_OUTPUT_HEADPHONE;

        respMsg.subtype = prvPlayTest(outdev, NULL) ? BBAT_SUCCESS : BBAT_FAILURE;
        _autotestGenerateRspMsg(&respMsg);
        break;

    case 0x02: //play PC data
        if (msg_head_speaker->command == 0)
            outdev = AUDEV_OUTPUT_SPEAKER;
        else if (msg_head_speaker->command == 1)
            outdev = AUDEV_OUTPUT_RECEIVER;
        else
            outdev = AUDEV_OUTPUT_HEADPHONE;

        pcm_buffer = &msg_head_speaker->data + 1 + 1;
        respMsg.subtype = prvPlayTest(outdev, pcm_buffer) ? BBAT_SUCCESS : BBAT_FAILURE;
        _autotestGenerateRspMsg(&respMsg);
        break;

    case 0x03: // stop
        audevStopPlayTest();
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);

        break;
    case 0x05: // AUDIO_SetVolume(msg_head_speaker->command);
        respMsg.subtype = BBAT_SUCCESS;
        _autotestGenerateRspMsg(&respMsg);
        break;

    default:
        respMsg.subtype = BBAT_FAILURE;
        _autotestGenerateRspMsg(&respMsg);
        break;
    }
    return 1;
}

static auRecorder_t *gBbatRecorder = NULL;

static bool prvStartRecord(void)
{
    if (gBbatRecorder != NULL)
        return false;

    gBbatRecorder = auRecorderCreate();
    if (gBbatRecorder == NULL)
        goto failed;

    if (!auRecorderStartMem(gBbatRecorder, AUDEV_RECORD_TYPE_MIC,
                            AUSTREAM_FORMAT_PCM, NULL,
                            BBAT_MIC_BUF_SIZE_ALL))
        goto failed;

    return true;

failed:
    auRecorderDelete(gBbatRecorder);
    gBbatRecorder = NULL;
    return false;
}

static bool prvStopRecord(void)
{
    if (gBbatRecorder == NULL)
        return false;

    bool ok = auRecorderStop(gBbatRecorder);
    auRecorderDelete(gBbatRecorder);
    gBbatRecorder = NULL;
    return ok;
}

static uint32_t _handleMicAutotest(
    const uint8_t *src_ptr, // Pointer of the input message.
    uint16_t src_len        // Size of the source buffer in uint8.
)
{
    diagMsgHead_t respMsg;
    diagMsgAudio_t *msg_head_mic = (diagMsgAudio_t *)src_ptr;
    osiBuffer_t recbuf = {};
    memcpy(&respMsg, msg_head_mic, sizeof(diagMsgHead_t));

    OSI_LOGI(0, "bbat mic test, command=%d data=%d",
             msg_head_mic->command, msg_head_mic->data);

    auMemWriter_t *writer = (gBbatRecorder == NULL) ? NULL : (auMemWriter_t *)auRecorderGetWriter(gBbatRecorder);
    switch (msg_head_mic->data)
    {
    case 0x01: // init recoding
        respMsg.subtype = prvStartRecord() ? BBAT_SUCCESS : BBAT_FAILURE;
        _autotestGenerateRspMsg(&respMsg);
        break;

    case 0x02: //get record status
        if (writer != NULL)
            recbuf = auMemWriterGetBuf(writer);

        respMsg.subtype = (recbuf.size >= BBAT_MIC_BUF_SIZE_ALL) ? BBAT_SUCCESS : BBAT_FAILURE;
        _autotestGenerateRspMsg(&respMsg);
        break;

    case 0x03: // send the record dada.
        if (writer != NULL)
        {
            recbuf = auMemWriterGetBuf(writer);
            uint8_t count_num = *(&msg_head_mic->data + 1);
            count_num %= BBAT_MIC_BUF_COUNT;
            uintptr_t address = recbuf.ptr + BBAT_MIC_BUF_SIZE * count_num;
            respMsg.subtype = BBAT_SUCCESS;
            _autotestGenerateRspDataMsg(&respMsg, (void *)address, BBAT_MIC_BUF_SIZE);
        }
        else
        {
            respMsg.subtype = BBAT_FAILURE;
            _autotestGenerateRspMsg(&respMsg);
        }
        break;

    case 0x04: // close the eraphone mic
        respMsg.subtype = prvStopRecord() ? BBAT_SUCCESS : BBAT_FAILURE;
        _autotestGenerateRspMsg(&respMsg);
        break;

    case 0x10: //set loop channel
        break;

    case 0x11:
        break;

    default:
        respMsg.subtype = BBAT_FAILURE;
        _autotestGenerateRspMsg(&respMsg);
        break;
    }

    return 1;
}

void _drvRegisterDeviceAutoTestCmdRoutine(void)
{
    OSI_LOGI(0, "bbat: Register Cmd Routine!");

    _diagInitAutotestCallback();

    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_KEYPAD, _handleKeypadAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_GPIO, _handleGpioAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_SIM, _handleSIMAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_VIB, _handleVibAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_USB, _handleUSBAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_ADC, _handleAdcAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_UART, _handleUartAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_CAMFLASH, _handleCamFlashAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_CAMERA_OPENCLOSE, _handleCamOpenCloseAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_CAMERA_MIPI, _handleMipiCamAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_NVRAM_R, _autotestResultRead);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_NVRAM_W, _autotestResultWrite);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_RTC, _handleRtcAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_CHECK, _handleAutotestCheck);

    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_MIC, _handleMicAutotest);
    _diagRegisterAutotestCallback(DEVICE_AUTOTEST_SPEAK, _handleSpeakerAutotest);
}

void diagAutoTestInit(void)
{
    OSI_LOGI(0, "bbat: diagAutoTestInit ");

    gBbatPmSource = osiPmSourceCreate(BBAT_NAME, NULL, NULL);
    osiPmWakeLock(gBbatPmSource);

    _drvRegisterDeviceAutoTestCmdRoutine();
    diagRegisterCmdHandle(DIAG_DEVICE_AUTOTEST_F, _handleDeviceAutotest, NULL);

    // io select UART3 rx/tx, for uart{1,3} loopback test
    hwp_iomux->pad_keyout_4_cfg_reg = 4;
    hwp_iomux->pad_keyout_5_cfg_reg = 4;

    halPmuSwitchPower(HAL_POWER_SD, true, true);
}

void drvSetPBkeyStatus(uint32_t key_status)
{
    if (key_status == 1)
        gPBkeyStatus = 0;
    else
        gPBkeyStatus = 1;
}
