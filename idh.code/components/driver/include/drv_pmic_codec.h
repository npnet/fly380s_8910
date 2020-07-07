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

#ifndef _DRV_PMIC_CODEC_H_
#define _DRV_PMIC_CODEC_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t A;
    uint32_t B;
    uint32_t E1;
    uint32_t E2;
    uint32_t cal_type;
} SC2720_HEADSET_AUXADC_CAL_T;

typedef enum
{
    UNKNOWN_MIC_TYPE,
    HEADSET_NO_MIC,
    HEADSET_4POLE_NORMAL,
    HEADSET_4POLE_NOT_NORMAL,
    HEADSET_APPLE,
    HEADSET_TYPE_ERR = -1,
} SC2720_HEADSET_TYPE_T;

typedef struct HeadPhoneSt
{
    uint32_t Connected;
    SC2720_HEADSET_TYPE_T MicType;
} T_HEADPHONE_STATUS;

typedef struct
{
    uint32_t adc_min;
    uint32_t adc_max;
    uint32_t code;
} SC2720_HEADSET_BUTTONS_T;
typedef struct
{
    uint32_t headset_stable_max_value;           // 2.65v
    uint32_t headset_one_half_adc_gnd;           //0.15V
    uint32_t headset_adc_threshold_3pole_detect; //0.9V
    uint32_t headset_half_adc_gnd;               //0.05V
    uint32_t headset_adc_gnd;                    // 0.1v
    uint32_t headset_key_send_min_vol;
    uint32_t headset_key_send_max_vol;
    uint32_t headset_key_volup_min_vol;
    uint32_t headset_key_volup_max_vol;
    uint32_t headset_key_voldown_min_vol;
    uint32_t headset_key_voldown_max_vol;
    uint32_t coefficient;
} HEADSET_INFO_T, *HEADSET_INFO_T_PTR;

void __sprd_codec_headset_related_oper(uint32_t heaset_type);
uint32_t __sprd_codec_headset_type_detect(void);
void __sprd_codec_set_headset_connection_status(uint32_t status);
bool drvPmicAudHeadsetInsertConfirm(void);
bool drvPmicAudHeadButtonPressConfirm(void);
uint32_t drvPmicAudGetHmicAdcValue(void);
void drvPmicAudHeadsetDectRegInit(void);

#ifdef __cplusplus
}
#endif
#endif
