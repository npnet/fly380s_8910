
#ifndef __INCLUDE_LV_INTER_PHONE_OBJ__
#define  __INCLUDE_LV_INTER_PHONE_OBJ__

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************
*
*                  INCLUDE
*
*************************************************/
#include "lvgl.h"
#include <stdarg.h>
/*************************************************
*
*                  DEFINE
*
*************************************************/
#define FT_COUNT_DEBUG         1   //lugj专用调试宏

#define LV_POC_STABAR_TASK_EXT_LENGTH (20)

#define LV_POC_SCREEN_SCALE_HOR_RES (LV_HOR_RES)
#define LV_POC_SCREEN_SCALE_VER_RES (LV_VER_RES)


//屏幕的零点位置
#define LV_POC_SCREEN_X           (0)
#define LV_POC_SCREEN_Y           (0)


//状态栏的位置
#define LV_POC_STATUS_BAR_HOR_RES (LV_POC_SCREEN_SCALE_HOR_RES)
#define LV_POC_STATUS_BAR_VER_RES (LV_POC_SCREEN_SCALE_VER_RES/7)
#define LV_POC_STATUS_BAR_OFFSET_X   0
#define LV_POC_STATUS_BAR_OFFSET_Y   0
#define LV_POC_STATUS_BAR_X0   (LV_POC_SCREEN_X + LV_POC_STATUS_BAR_OFFSET_X)
#define LV_POC_STATUS_BAR_Y0   (LV_POC_SCREEN_Y + LV_POC_STATUS_BAR_OFFSET_Y)
#define LV_POC_STATUS_BAR_X1   (LV_POC_STATUS_BAR_X0 + LV_POC_STATUS_BAR_HOR_RES - 1)
#define LV_POC_STATUS_BAR_Y1   (LV_POC_STATUS_BAR_Y0 + LV_POC_STATUS_BAR_VER_RES - 1)
#define LV_POC_STATUS_BAR_POSITION_X  (LV_POC_STATUS_BAR_X0)
#define LV_POC_STATUS_BAR_POSITION_Y  (LV_POC_STATUS_BAR_Y0)

//控制栏尺寸以及偏移
#define LV_POC_CONTROL_HOR_RES (LV_POC_SCREEN_SCALE_HOR_RES)
#define LV_POC_CONTROL_VER_RES (LV_POC_SCREEN_SCALE_VER_RES/7)
#define LV_POC_CONTROL_OFFSET_X   (0)
#define LV_POC_CONTROL_OFFSET_Y   (0)


//显示栏偏移
#define LV_POC_DISPLAY_OFFSET_X   (0)
#define LV_POC_DISPLAY_OFFSET_Y   (0)


//有状态栏和控制栏的显示区域的位置
#define LV_POC_DISPLAY_HOR_RES (LV_POC_SCREEN_SCALE_HOR_RES)
#define LV_POC_DISPLAY_VER_RES (LV_POC_SCREEN_SCALE_VER_RES - LV_POC_STATUS_BAR_VER_RES - LV_POC_CONTROL_VER_RES)
#define LV_POC_DISPLAY_X0   (LV_POC_SCREEN_X + LV_POC_DISPLAY_OFFSET_X)
#define LV_POC_DISPLAY_Y0   (LV_POC_SCREEN_Y + LV_POC_DISPLAY_OFFSET_Y)
#define LV_POC_DISPLAY_X1   (LV_POC_DISPLAY_X0 + LV_POC_DISPLAY_HOR_RES - 1)
#define LV_POC_DISPLAY_Y1   (LV_POC_DISPLAY_Y0 + LV_POC_DISPLAY_VER_RES - 1)
#define LV_POC_DISPLAY_POSITION_X  (LV_POC_DISPLAY_X0)
#define LV_POC_DISPLAY_POSITION_Y  (LV_POC_DISPLAY_Y0)


//无控制栏有状态栏的显示区域位置
#define LV_POC_DISPLAY_NOCTR_HOR_RES (LV_POC_SCREEN_SCALE_HOR_RES)
#define LV_POC_DISPLAY_NOCTR_VER_RES (LV_POC_SCREEN_SCALE_VER_RES - LV_POC_STATUS_BAR_VER_RES)
#define LV_POC_DISPLAY_NOCTR_X0   (LV_POC_SCREEN_X + LV_POC_DISPLAY_OFFSET_X)
#define LV_POC_DISPLAY_NOCTR_Y0   (LV_POC_SCREEN_Y + LV_POC_DISPLAY_OFFSET_Y + LV_POC_STATUS_BAR_VER_RES)
#define LV_POC_DISPLAY_NOCTR_X1   (LV_POC_DISPLAY_NOCTR_X0 + LV_POC_DISPLAY_NOCTR_HOR_RES - 1)
#define LV_POC_DISPLAY_NOCTR_Y1   (LV_POC_DISPLAY_NOCTR_Y0 + LV_POC_DISPLAY_NOCTR_VER_RES - 1)
#define LV_POC_DISPLAY_NOCTR_POSITION_X  (LV_POC_DISPLAY_NOCTR_X0)
#define LV_POC_DISPLAY_NOCTR_POSITION_Y  (LV_POC_DISPLAY_NOCTR_Y0)

//有状态栏时显示base
#define LV_POC_DISPLAY_STABAR_BASE_HOR_RES (LV_POC_SCREEN_SCALE_HOR_RES)
#define LV_POC_DISPLAY_STABAR_BASE_VER_RES (LV_POC_SCREEN_SCALE_VER_RES - LV_POC_STATUS_BAR_VER_RES)
#define LV_POC_DISPLAY_STABAR_BASE_X0   (LV_POC_SCREEN_X + LV_POC_DISPLAY_OFFSET_X)
#define LV_POC_DISPLAY_STABAR_BASE_Y0   (LV_POC_SCREEN_Y + LV_POC_DISPLAY_OFFSET_Y + LV_POC_STATUS_BAR_VER_RES)
#define LV_POC_DISPLAY_STABAR_BASE_X1   (LV_POC_DISPLAY_STABAR_BASE_X0 + LV_POC_DISPLAY_STABAR_BASE_HOR_RES - 1)
#define LV_POC_DISPLAY_STABAR_BASE_Y1   (LV_POC_DISPLAY_STABAR_BASE_Y0 + LV_POC_DISPLAY_STABAR_BASE_VER_RES - 1)
#define LV_POC_DISPLAY_STABAR_BASE_POSITION_X  (LV_POC_DISPLAY_STABAR_BASE_X0)
#define LV_POC_DISPLAY_STABAR_BASE_POSITION_Y  (LV_POC_DISPLAY_STABAR_BASE_Y0)



//有状态栏的控制栏位置
#define LV_POC_CONTROL_X0   (LV_POC_SCREEN_X + LV_POC_CONTROL_OFFSET_X)
#define LV_POC_CONTROL_Y0   (LV_POC_SCREEN_Y + LV_POC_CONTROL_OFFSET_Y + LV_POC_DISPLAY_VER_RES)
#define LV_POC_CONTROL_X1   (LV_POC_CONTROL_X0 + LV_POC_CONTROL_HOR_RES - 1)
#define LV_POC_CONTROL_Y1   (LV_POC_CONTROL_Y0 + LV_POC_CONTROL_VER_RES - 1)
#define LV_POC_CONTROL_POSITION_X  (LV_POC_CONTROL_X0)
#define LV_POC_CONTROL_POSITION_Y  (LV_POC_CONTROL_Y0)


#if 1

//无状态栏有控制栏的显示区域位置
#define LV_POC_DISPLAY_NOSTABAR_HOR_RES (LV_POC_SCREEN_SCALE_HOR_RES)
#define LV_POC_DISPLAY_NOSTABAR_VER_RES (LV_POC_SCREEN_SCALE_VER_RES - LV_POC_CONTROL_VER_RES)
#define LV_POC_DISPLAY_NOSTABAR_X0   (LV_POC_SCREEN_X + LV_POC_DISPLAY_OFFSET_X)
#define LV_POC_DISPLAY_NOSTABAR_Y0   (LV_POC_SCREEN_Y + LV_POC_DISPLAY_OFFSET_Y)
#define LV_POC_DISPLAY_NOSTABAR_X1   (LV_POC_DISPLAY_NOSTABAR_X0 + LV_POC_DISPLAY_NOSTABAR_HOR_RES - 1)
#define LV_POC_DISPLAY_NOSTABAR_Y1   (LV_POC_DISPLAY_NOSTABAR_Y0 + LV_POC_DISPLAY_NOSTABAR_VER_RES - 1)
#define LV_POC_DISPLAY_NOSTABAR_POSITION_X  (LV_POC_DISPLAY_NOSTABAR_X0)
#define LV_POC_DISPLAY_NOSTABAR_POSITION_Y  (LV_POC_DISPLAY_NOSTABAR_Y0)



//无状态栏和控制栏时的显示区域位置
#define LV_POC_DISPLAY_NOCTR_STABAR_HOR_RES (LV_POC_SCREEN_SCALE_HOR_RES)
#define LV_POC_DISPLAY_NOCTR_STABAR_VER_RES (LV_POC_SCREEN_SCALE_VER_RES)
#define LV_POC_DISPLAY_NOCTR_STABAR_X0   (LV_POC_SCREEN_X + LV_POC_DISPLAY_OFFSET_X)
#define LV_POC_DISPLAY_NOCTR_STABAR_Y0   (LV_POC_SCREEN_Y + LV_POC_DISPLAY_OFFSET_Y)
#define LV_POC_DISPLAY_NOCTR_STABAR_X1   (LV_POC_DISPLAY_NOCTR_STABAR_X0 + LV_POC_DISPLAY_NOCTR_STABAR_HOR_RES - 1)
#define LV_POC_DISPLAY_NOCTR_STABAR_Y1   (LV_POC_DISPLAY_NOCTR_STABAR_Y0 + LV_POC_DISPLAY_NOCTR_STABAR_VER_RES - 1)
#define LV_POC_DISPLAY_NOCTR_STABAR_POSITION_X  (LV_POC_DISPLAY_NOCTR_STABAR_X0)
#define LV_POC_DISPLAY_NOCTR_STABAR_POSITION_Y  (LV_POC_DISPLAY_NOCTR_STABAR_Y0)

//无状态栏时显示base
#define LV_POC_DISPLAY_NOSTABAR_BASE_HOR_RES (LV_POC_SCREEN_SCALE_HOR_RES)
#define LV_POC_DISPLAY_NOSTABAR_BASE_VER_RES (LV_POC_SCREEN_SCALE_VER_RES)
#define LV_POC_DISPLAY_NOSTABAR_BASE_X0   (LV_POC_SCREEN_X + LV_POC_DISPLAY_OFFSET_X)
#define LV_POC_DISPLAY_NOSTABAR_BASE_Y0   (LV_POC_SCREEN_Y + LV_POC_DISPLAY_OFFSET_Y)
#define LV_POC_DISPLAY_NOSTABAR_BASE_X1   (LV_POC_DISPLAY_NOSTABAR_BASE_X0 + LV_POC_DISPLAY_NOSTABAR_BASE_HOR_RES - 1)
#define LV_POC_DISPLAY_NOSTABAR_BASE_Y1   (LV_POC_DISPLAY_NOSTABAR_BASE_Y0 + LV_POC_DISPLAY_NOSTABAR_BASE_VER_RES - 1)
#define LV_POC_DISPLAY_NOSTABAR_BASE_POSITION_X  (LV_POC_DISPLAY_NOSTABAR_BASE_X0)
#define LV_POC_DISPLAY_NOSTABAR_BASE_POSITION_Y  (LV_POC_DISPLAY_NOSTABAR_BASE_Y0)


//无状态栏的控制栏位置
#define LV_POC_CONTROL_NOSTABAR_X0   (LV_POC_SCREEN_X + LV_POC_CONTROL_OFFSET_X)
#define LV_POC_CONTROL_NOSTABAR_Y0   (LV_POC_SCREEN_Y + LV_POC_CONTROL_OFFSET_Y + LV_POC_DISPLAY_NOSTABAR_VER_RES)
#define LV_POC_CONTROL_NOSTABAR_X1   (LV_POC_CONTROL_NOSTABAR_X0 + LV_POC_CONTROL_HOR_RES)
#define LV_POC_CONTROL_NOSTABAR_Y1   (LV_POC_CONTROL_NOSTABAR_Y0 + LV_POC_CONTROL_VER_RES - 1)
#define LV_POC_CONTROL_NOSTABAR_POSITION_X  (LV_POC_CONTROL_NOSTABAR_X0)
#define LV_POC_CONTROL_NOSTABAR_POSITION_Y  (LV_POC_CONTROL_NOSTABAR_Y0)

#endif


//状态栏时间label位置信息
#define LV_POC_STABAR_TIME_HOR_RES       (LV_POC_SCREEN_SCALE_HOR_RES/5)
#define LV_POC_STABAR_TIME_VER_RES       (LV_POC_STATUS_BAR_VER_RES)
#define LV_POC_STABAR_TIME_OFFSET_X      (4)
#define LV_POC_STABAR_TIME_OFFSET_Y      (4)
#define LV_POC_STABAR_TIME_X0             (LV_POC_SCREEN_X + LV_POC_STABAR_TIME_OFFSET_X)
#define LV_POC_STABAR_TIME_Y0             (LV_POC_SCREEN_Y + LV_POC_STABAR_TIME_OFFSET_Y)
#define LV_POC_STABAR_TIME_X1             (LV_POC_SCREEN_X + LV_POC_STABAR_TIME_OFFSET_X + LV_POC_STABAR_TIME_HOR_RES - 1)
#define LV_POC_STABAR_TIME_X2             (LV_POC_SCREEN_Y + LV_POC_STABAR_TIME_OFFSET_Y + LV_POC_STABAR_TIME_VER_RES - 1)
#define LV_POC_STABAR_TIME_POSITION_X    (LV_POC_STABAR_TIME_X0)
#define LV_POC_STABAR_TIME_POSITION_Y    (LV_POC_STABAR_TIME_Y0)


//状态栏电池位置信息
#define LV_POC_STABAR_BATTERY_HOR_RES    (LV_POC_SCREEN_SCALE_HOR_RES/12)
#define LV_POC_STABAR_BATTERY_VER_RES    (LV_POC_STATUS_BAR_VER_RES)
#define LV_POC_STABAR_BATTERY_OFFSET_X   (0-2)
#define LV_POC_STABAR_BATTERY_OFFSET_Y   (1)
#define LV_POC_STABAR_BATTERY_X0          (LV_POC_SCREEN_X + LV_POC_STABAR_BATTERY_OFFSET_X + LV_POC_SCREEN_SCALE_HOR_RES - LV_POC_STABAR_BATTERY_HOR_RES - 1)
#define LV_POC_STABAR_BATTERY_Y0          (LV_POC_SCREEN_Y + LV_POC_STABAR_BATTERY_OFFSET_Y)
#define LV_POC_STABAR_BATTERY_X1          (LV_POC_SCREEN_X + LV_POC_STABAR_BATTERY_OFFSET_X + LV_POC_SCREEN_SCALE_HOR_RES - 1)
#define LV_POC_STABAR_BATTERY_Y1          (LV_POC_SCREEN_Y + LV_POC_STABAR_BATTERY_OFFSET_Y + LV_POC_STABAR_BATTERY_VER_RES - 1)
#define LV_POC_STABAR_BATTERY_POSITION_X (LV_POC_STABAR_BATTERY_X0)
#define LV_POC_STABAR_BATTERY_POSITION_Y (LV_POC_STABAR_BATTERY_Y0)


//状态栏SIM卡信息位置1
#define LV_POC_STABAR_SIM_1_HOR_RES    (LV_POC_SCREEN_SCALE_HOR_RES/12)
#define LV_POC_STABAR_SIM_1_VER_RES    (LV_POC_STATUS_BAR_VER_RES)
#define LV_POC_STABAR_SIM_1_OFFSET_X   (-1)
#define LV_POC_STABAR_SIM_1_OFFSET_Y   (1)
#define LV_POC_STABAR_SIM_1_X0          (LV_POC_SCREEN_X + LV_POC_STABAR_SIM_1_OFFSET_X + LV_POC_SCREEN_SCALE_HOR_RES - LV_POC_STABAR_BATTERY_HOR_RES - LV_POC_STABAR_SIM_1_HOR_RES - 1)
#define LV_POC_STABAR_SIM_1_Y0          (LV_POC_SCREEN_Y + LV_POC_STABAR_SIM_1_OFFSET_Y)
#define LV_POC_STABAR_SIM_1_X1          (LV_POC_SCREEN_X + LV_POC_STABAR_SIM_1_OFFSET_X + LV_POC_SCREEN_SCALE_HOR_RES - LV_POC_STABAR_BATTERY_HOR_RES - 1)
#define LV_POC_STABAR_SIM_1_Y1          (LV_POC_SCREEN_Y + LV_POC_STABAR_SIM_1_OFFSET_Y + LV_POC_STABAR_SIM_1_VER_RES - 1)
#define LV_POC_STABAR_SIM_1_POSITION_X (LV_POC_STABAR_SIM_1_X0)
#define LV_POC_STABAR_SIM_1_POSITION_Y (LV_POC_STABAR_SIM_1_Y0)


//状态栏SIM卡信息位置2
#define LV_POC_STABAR_SIM_2_HOR_RES    (LV_POC_SCREEN_SCALE_HOR_RES/12)
#define LV_POC_STABAR_SIM_2_VER_RES    (LV_POC_STATUS_BAR_VER_RES)
#define LV_POC_STABAR_SIM_2_OFFSET_X   (-1)
#define LV_POC_STABAR_SIM_2_OFFSET_Y   (1)
#define LV_POC_STABAR_SIM_2_X0          (LV_POC_SCREEN_X + LV_POC_STABAR_SIM_2_OFFSET_X + LV_POC_SCREEN_SCALE_HOR_RES - LV_POC_STABAR_BATTERY_HOR_RES - LV_POC_STABAR_SIM_1_HOR_RES - LV_POC_STABAR_SIM_2_HOR_RES - 1)
#define LV_POC_STABAR_SIM_2_Y0          (LV_POC_SCREEN_Y + LV_POC_STABAR_SIM_2_OFFSET_Y)
#define LV_POC_STABAR_SIM_2_X1          (LV_POC_SCREEN_X + LV_POC_STABAR_SIM_2_OFFSET_X + LV_POC_SCREEN_SCALE_HOR_RES - LV_POC_STABAR_BATTERY_HOR_RES - LV_POC_STABAR_SIM_1_HOR_RES - 1)
#define LV_POC_STABAR_SIM_2_Y1          (LV_POC_SCREEN_Y + LV_POC_STABAR_SIM_2_OFFSET_Y + LV_POC_STABAR_SIM_2_VER_RES - 1)
#define LV_POC_STABAR_SIM_2_POSITION_X (LV_POC_STABAR_SIM_2_X0)
#define LV_POC_STABAR_SIM_2_POSITION_Y (LV_POC_STABAR_SIM_2_Y0)


#define LV_POC_STABAR_TIME_FORMATE_DEFAULT  (lv_poc_time_format_hhmm)
#define LV_POC_STABAR_TIME_LEN_DEFAULT       (12)
#define LV_POC_STABAR_TIME_DEAULT_HHMM              "00:00"
#define LV_POC_STABAR_TIME_DEAULT_MMHH              "00:00"
#define LV_POC_STABAR_TIME_DEAULT_HHMMSS            "00:00:00"
#define LV_POC_STABAR_TIME_DEAULT_SSHHMM            "00:00:00"
#define LV_POC_STABAR_TIME_DEAULT_SSMMHH            "00:00:00"
#define LV_POC_STABAR_TIME_DEAULT_MMHHSS            "00:00:00"


#define LV_POC_CONTROL_LBTN_HOR_RES        (LV_POC_CONTROL_HOR_RES/12)
#define LV_POC_CONTROL_LBTN_VER_RES        (LV_POC_CONTROL_VER_RES)
#define LV_POC_CONTROL_LBTN_OFFSET_X       (0)
#define LV_POC_CONTROL_LBTN_OFFSET_Y       (0)
#define LV_POC_CONTROL_LBTN_X0              (LV_POC_SCREEN_X + LV_POC_CONTROL_LBTN_OFFSET_X)
#define LV_POC_CONTROL_LBTN_Y0              (LV_POC_SCREEN_Y + LV_POC_CONTROL_LBTN_OFFSET_Y)
#define LV_POC_CONTROL_LBTN_X1              (LV_POC_CONTROL_LBTN_X0 + LV_POC_CONTROL_LBTN_HOR_RES - 1)
#define LV_POC_CONTROL_LBTN_Y1              (LV_POC_CONTROL_LBTN_Y0 + LV_POC_CONTROL_LBTN_VER_RES - 1)
#define LV_POC_CONTROL_LBTN_POSITION_X     (LV_POC_CONTROL_LBTN_X0)
#define LV_POC_CONTROL_LBTN_POSITION_Y     (LV_POC_CONTROL_LBTN_Y0)


#define LV_POC_CONTROL_MBTN_HOR_RES        (LV_POC_CONTROL_HOR_RES/12)
#define LV_POC_CONTROL_MBTN_VER_RES        (LV_POC_CONTROL_VER_RES)
#define LV_POC_CONTROL_MBTN_OFFSET_X       (0)
#define LV_POC_CONTROL_MBTN_OFFSET_Y       (0)
#define LV_POC_CONTROL_MBTN_X0              (LV_POC_SCREEN_X + LV_POC_CONTROL_MBTN_OFFSET_X + LV_POC_CONTROL_HOR_RES/4 - LV_POC_CONTROL_MBTN_HOR_RES/4 - LV_POC_CONTROL_MBTN_HOR_RES/6)
#define LV_POC_CONTROL_MBTN_Y0              (LV_POC_SCREEN_X + LV_POC_CONTROL_MBTN_OFFSET_Y)
#define LV_POC_CONTROL_MBTN_X1              (LV_POC_CONTROL_MBTN_X0 + LV_POC_CONTROL_MBTN_HOR_RES - 1)
#define LV_POC_CONTROL_MBTN_Y1              (LV_POC_CONTROL_MBTN_Y0 + LV_POC_CONTROL_MBTN_VER_RES - 1)
#define LV_POC_CONTROL_MBTN_POSITION_X     (LV_POC_CONTROL_MBTN_X0)
#define LV_POC_CONTROL_MBTN_POSITION_Y     (LV_POC_CONTROL_MBTN_Y0)


#define LV_POC_CONTROL_RBTN_HOR_RES        (LV_POC_CONTROL_HOR_RES/12)
#define LV_POC_CONTROL_RBTN_VER_RES        (LV_POC_CONTROL_VER_RES)
#define LV_POC_CONTROL_RBTN_OFFSET_X       (0)
#define LV_POC_CONTROL_RBTN_OFFSET_Y       (0)
#define LV_POC_CONTROL_RBTN_X0              (LV_POC_SCREEN_X + LV_POC_CONTROL_RBTN_OFFSET_X + LV_POC_CONTROL_HOR_RES - LV_POC_CONTROL_RBTN_HOR_RES)
#define LV_POC_CONTROL_RBTN_Y0              (LV_POC_SCREEN_X + LV_POC_CONTROL_RBTN_OFFSET_Y)
#define LV_POC_CONTROL_RBTN_X1              (LV_POC_CONTROL_RBTN_X0 + LV_POC_CONTROL_RBTN_HOR_RES - 1)
#define LV_POC_CONTROL_RBTN_Y1              (LV_POC_CONTROL_RBTN_Y0 + LV_POC_CONTROL_RBTN_VER_RES - 1)
#define LV_POC_CONTROL_RBTN_POSITION_X     (LV_POC_CONTROL_RBTN_X0)
#define LV_POC_CONTROL_RBTN_POSITION_Y     (LV_POC_CONTROL_RBTN_Y0)

#define LV_POC_LIST_COLUM_COUNT (poc_setting_conf->font.list_page_colum_count)

#ifndef USE_CRANE_POC_KEYPAD
#define USE_CRANE_POC_KEYPAD 0
#endif

#ifndef USE_CRANE_POC_INTERRUPT_KEYPAD
#define USE_CRANE_POC_INTERRUPT_KEYPAD 1
#endif

/*************************************************
*
*                  ENUM
*
*************************************************/
typedef enum {
	lv_poc_time_format_hhmm = 0,
	lv_poc_time_format_mmhh,
	lv_poc_time_format_hhmmss,
	lv_poc_time_format_sshhmm,
	lv_poc_time_format_ssmmhh,
	lv_poc_time_format_mmhhss,
	lv_poc_time_format_invalid_time_format
} lv_poc_time_format_t;

/*************************************************
*
*                  TYPEDEF
*
*************************************************/
typedef char *             lv_poc_text ;
typedef lv_obj_t           lv_poc_status_bar_t;
typedef lv_obj_t           lv_poc_display_t;
//typedef lv_obj_t           lv_poc_control_button_t;


/*************************************************
*
*                  STRUCT
*
*************************************************/
/*******************
*     NAME:   lv_poc_status_bar_sim_obj_t
*   AUTHOR:   lugj
* DESCRIPT:   status bar of the inter phone
*     DATE:   2020-1-20
********************/
typedef struct _lv_poc_status_bar_sim_obj_t
{
	lv_obj_t ** align_l_obj, ** align_r_obj;                 //对其对象，用于排列sim卡图标
	lv_obj_t * sim_img;                   //在处错误时或者没有SIM卡时显示用的图标
	lv_obj_t * sim_up_down_img;           //上传下载时的图标
	lv_obj_t * sim_signal_strength_img;   //信号强度图标
	lv_obj_t * sim_net_type_img;          //网络信号类型图标
} lv_poc_status_bar_sim_obj_t;

/*******************
*     NAME:   lv_poc_statuc_bar_t
*   AUTHOR:   lugj
* DESCRIPT:   status bar of the inter phone
*     DATE:   2019-10-24
********************/
typedef struct _lv_poc_status_bar_fptr_t{
//	lv_coord_t area;
	bool                        has_sim1;
	bool                        has_sim2;
	bool                        has_battery;
	lv_poc_time_format_t time_formate;
	#if 1
	lv_poc_status_bar_sim_obj_t  *sim1;
	lv_poc_status_bar_sim_obj_t  *sim2;
	#else
	lv_obj_t                    *sim1;
	lv_obj_t                    *sim2;
	#endif
	lv_obj_t                    *battery_img;
	lv_obj_t                    *time_label;
    lv_obj_t                    *signal_type_label;
    lv_obj_t                    *signal_intensity_img;
} lv_poc_status_bar_fptr_t;


#if 1
typedef lv_obj_t lv_poc_control_button_t;
#else
/*******************
*     NAME:    _lv_poc_control_button_t
*   AUTHOR:    lugj
* DESCRIPT:    control button of the inter phone
*     DATE:    2019-10-24
********************/
typedef struct _lv_poc_control_button_t{
	lv_obj_t         *  button;
	lv_obj_t         *  label;
} lv_poc_control_button_t;
#endif

/*******************
*     NAME:    _lv_poc_control_t
*   AUTHOR:    lugj
* DESCRIPT:    control filed of the inter phone
*     DATE:    2019-10-24
********************/
typedef struct _lv_poc_control_t{
	lv_poc_control_button_t  *  left_button;
	lv_poc_control_button_t  *  middle_button;
	lv_poc_control_button_t  *  right_button;
//	lv_area_t                          area;
	lv_obj_t                        *  background;
} lv_poc_control_t;

#if 0
/*******************
*     NAME:    _lv_poc_display_t
*   AUTHOR:    lugj
* DESCRIPT:    display area of the inter phone
*     DATE:    2019-10-24
********************/
typedef struct _lv_poc_display_t{
	lv_obj_t                        *  display;
//	lv_area_t                          area;
//	lv_obj_t                        *  bgimg;
} lv_poc_display_t;
#endif

/*******************
*     NAME:   _lv_poc_activity_t
*   AUTHOR:   lugj
* DESCRIPT:   inter phone activity
*     DATE:   2019-10-24
********************/
typedef struct _lv_poc_activity_t{
	bool                               has_stabar;   //只标注是否显示状态栏，不代表状态栏的存在性
	bool                               has_control;     //标注该窗口是否有控制栏
	lv_obj_t                        *  base;            //activity基础，不要调用，只在创建和删除用到
	lv_poc_status_bar_t      *  status_bar;     //即使该窗口没有状态栏，该成员也会指向状态栏，因为状态栏唯一
	lv_poc_control_t         *  control;        //指向控制栏
	lv_poc_display_t         *  display;        //指向显示区域
    lv_signal_cb_t                   signal_func;    //activity的signal函数
    lv_design_cb_t                   design_func;    //activity的design函数
    lv_event_cb_t                     event_func;    //activity的event函数
    lv_poc_activity_ext_t            activity_ext;    //activity的创建和销毁
    void                            *  ext_data;       //activity的数据指针，具体数据类型由activity->base->ext_attr->create()的返回值决定，可以根据自己的需要，在创建activity后，将数据放置在此
} lv_poc_activity_t;

/*******************
*     NAME:     _lv_poc_control_text_t
*   AUTHOR:     lugj
* DESCRIPT:
*     DATE:
********************/
typedef struct _lv_poc_control_text_t{
	lv_poc_text left_text;
	lv_poc_text middle_text;
	lv_poc_text right_text;
} lv_poc_control_text_t;

/*******************
*     NAME:   _lv_poc_activity_list_node_t
*   AUTHOR:   lugj
* DESCRIPT:   存储创建的activity活动，以及查找display所属的activity
*     DATE:   2019-10-30
********************/
typedef struct _lv_poc_activity_list_node_t{
	lv_poc_activity_t * activity;
	struct _lv_poc_activity_list_node_t * next;
} lv_poc_activity_list_node_t;

typedef struct _lv_poc_activity_list_t{
	unsigned int size;
	struct _lv_poc_activity_list_node_t * head;
} lv_poc_activity_list_t;

typedef struct _lv_poc_activity_attribute_cb_set_obj
{
	struct{
		lv_poc_Activity_Id_t activity_id;
		bool active;
		lv_poc_member_list_add_cb add;
		lv_poc_member_list_remove_cb remove;
		lv_poc_member_list_clear_cb clear;
		lv_poc_member_list_get_information_cb get_info;
		lv_poc_member_list_refresh_cb refresh;
		lv_poc_member_list_move_top_cb move_to_top;
		lv_poc_member_list_move_bottom_cb move_to_bottom;
		lv_poc_member_list_move_up_cb move_up;
		lv_poc_member_list_move_down_cb move_down;
		lv_poc_member_list_set_state_cb set_state;
		lv_poc_member_list_is_exists_cb exists;
		lv_poc_member_list_get_state_cb get_state;
	} member_list[LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE];

	struct{
		lv_poc_Activity_Id_t activity_id;
		bool active;
		lv_poc_group_list_add_cb add;
		lv_poc_group_list_remove_cb remove;
		lv_poc_group_list_get_information_cb get_info;
		lv_poc_group_list_refresh_cb refresh;
		lv_poc_group_list_move_top_cb move_to_top;
		lv_poc_group_list_move_bottom_cb move_to_bottom;
		lv_poc_group_list_move_up_cb move_up;
		lv_poc_group_list_move_down_cb move_down;
		lv_poc_group_list_is_exists_cb exists;
	} group_list[LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE];

	lv_poc_notation_msg_cb note;
} lv_poc_activity_attribute_cb_set_obj;

/*******************
*     NAME:  status_bar_task_t
*   AUTHOR:  lugj
* DESCRIPT:  定义状态栏任务中额外执行的任务类型
*     DATE:
********************/
typedef void (*status_bar_task_t)(void);


/*******************
*     NAME:
*   AUTHOR:
* DESCRIPT:
*     DATE:
********************/
typedef struct
{
	int sign;
	lv_poc_activity_t * activity;
	void * parameters;
} lv_exec_parameter_t;

/*******************
*     NAME:
*   AUTHOR:
* DESCRIPT:
*     DATE:
********************/

/*******************
*     NAME:
*   AUTHOR:
* DESCRIPT:
*     DATE:
********************/


/*************************************************
*
*                  EXTERN
*
*************************************************/
extern lv_group_t *interphone_indev_group;
extern lv_poc_activity_t * current_activity;
extern lv_poc_status_bar_t      *lv_poc_status_bar;

extern lv_style_t theme_white_style_base;
extern lv_style_t theme_white_style_list_scroll;
extern lv_style_t theme_white_style_list_page;
extern lv_style_t theme_white_style_list_btn_rel;
extern lv_style_t theme_white_style_list_btn_pr;
extern lv_style_t theme_white_style_list_btn_ina;
extern lv_style_t theme_white_style_win_header;
extern lv_style_t theme_white_style_idle_big_clock;
extern lv_style_t theme_white_style_idle_date_label;
extern lv_style_t theme_white_style_idle_msg_label;
extern lv_style_t theme_white_style_switch_bg;
extern lv_style_t theme_white_style_switch_indic;
extern lv_style_t theme_white_style_switch_knob_off;
extern lv_style_t theme_white_style_switch_knob_on;
extern lv_style_t theme_white_style_rb;
extern lv_style_t theme_white_style_cb;
extern lv_style_t theme_white_style_about_label;
extern lv_style_t theme_white_style_status_bar;
extern lv_style_t theme_white_style_status_bar_time;
extern lv_style_t theme_white_style_control;

extern lv_style_t theme_black_style_base;
extern lv_style_t theme_black_style_list_scroll;
extern lv_style_t theme_black_style_list_page;
extern lv_style_t theme_black_style_list_btn_rel;
extern lv_style_t theme_black_style_list_btn_pr;
extern lv_style_t theme_black_style_list_btn_ina;
extern lv_style_t theme_black_style_win_header;
extern lv_style_t theme_black_style_idle_big_clock;
extern lv_style_t theme_black_style_idle_date_label;
extern lv_style_t theme_black_style_idle_msg_label;
extern lv_style_t theme_black_style_switch_bg;
extern lv_style_t theme_black_style_switch_indic;
extern lv_style_t theme_black_style_switch_knob_off;
extern lv_style_t theme_black_style_switch_knob_on;
extern lv_style_t theme_black_style_rb;
extern lv_style_t theme_black_style_cb;
extern lv_style_t theme_black_style_about_label;
extern lv_style_t theme_black_style_status_bar;
extern lv_style_t theme_black_style_status_bar_time;
extern lv_style_t theme_black_style_control;


/*************************************************
*
*                  STATIC
*
*************************************************/


/*************************************************
*
*                  PUBLIC DEFINE
*
*************************************************/

/*******************
*	  NAME: 	lv_poc_stabar_task
*	AUTHOR:    lugj
* DESCRIPT:    状态栏任务
*	  DATE:    2019-10-25
********************/
void lv_poc_stabar_task(lv_task_t * task);

#if 0
/*******************
*     NAME:    lv_poc_get_time
*   AUTHOR:    lugj
* DESCRIPT:    获取本地时间，保存在local_time中
*     DATE:    2019-10-25
********************/
bool lv_poc_get_time(void *local_time);
#endif

/*******************
*     NAME:    lv_poc_get_battery_state
*   AUTHOR:    lugj
* DESCRIPT:    获取电池状态，
*     DATE:    2019-10-25
********************/
UINT8 lv_poc_get_battery_state(void);

/*******************
*     NAME:    lv_poc_get_sim1_state
*   AUTHOR:    lugj
* DESCRIPT:    获取SIM1是否存在
*     DATE:    2019-10-25
********************/
bool lv_poc_get_sim1_state(void);

/*******************
*     NAME:    lv_poc_get_sim2_state
*   AUTHOR:    lugj
* DESCRIPT:    获取SIM2是否存在
*     DATE:    2019-10-25
********************/
bool lv_poc_get_sim2_state(void);

/*******************
*     NAME:    lv_poc_get_battery_img
*   AUTHOR:    lugj
* DESCRIPT:    根据电池的状态，获取正确的图标
*     DATE:    2020-01-19
********************/
lv_img_dsc_t * lv_poc_get_battery_img(void);


/*******************
*     NAME:    lv_poc_create_activity
*   AUTHOR:    lugj
* DESCRIPT:    创建一个有效的activity
*     DATE:    2019-10-25
********************/
lv_poc_activity_t *lv_poc_create_activity(lv_poc_activity_ext_t *activity_ext,
																	bool has_stabar,
																	bool has_control,
                                                                    lv_poc_control_text_t *control_text);


/*******************
*     NAME:    lv_poc_get_user_obj
*   AUTHOR:    lugj
* DESCRIPT:    从activity中获取可以显示区域
*     DATE:    2019-10-25
********************/
lv_poc_display_t *lv_poc_get_display(lv_poc_activity_t *activity);

/*******************
*     NAME:    lv_poc_del_activity
*   AUTHOR:    lugj
* DESCRIPT:    删除activity
*     DATE:    2019-10-25
********************/
bool lv_poc_del_activity(lv_poc_activity_t *activity);

/*******************
*     NAME:    lv_poc_group_remove_obj
*   AUTHOR:    lugj
* DESCRIPT:    将obj添加到keypad输入设备组中
*     DATE:    2019-10-25
********************/
void lv_poc_group_remove_obj(lv_obj_t *obj);

/*******************
*     NAME:    lv_poc_group_add_obj
*   AUTHOR:    lugj
* DESCRIPT:    将obj从keypad输入设备组中移除
*     DATE:    2019-10-25
********************/
void lv_poc_group_add_obj(lv_obj_t *obj);

/*******************
*     NAME:    lv_poc_activity_set_signal_cb
*   AUTHOR:    lugj
* DESCRIPT:    设置activity的signal函数
*     DATE:    2019-10-30
********************/
void lv_poc_activity_set_signal_cb(lv_poc_activity_t *activity,
															lv_signal_cb_t func);

/*******************
*     NAME:    lv_poc_activity_set_design_cb
*   AUTHOR:    lugj
* DESCRIPT:    设置activity的signal函数
*     DATE:    2019-10-31
********************/
void lv_poc_activity_set_design_cb(lv_poc_activity_t *activity,
															lv_design_cb_t func);

/*******************
*     NAME:    lv_poc_activity_set_background
*   AUTHOR:    lugj
* DESCRIPT:    设置背景图像
*     DATE:    2019-10-30
********************/
void lv_poc_activity_set_background(lv_poc_activity_t *activity,
															lv_img_dsc_t *bg_img);

/*******************
*     NAME:    lv_poc_activity_list_lookup
*   AUTHOR:    lugj
* DESCRIPT:    从activity查询表中查询display的activity
*     DATE:    2019-10-30
********************/
lv_poc_activity_t *	lv_poc_activity_list_lookup(lv_poc_display_t * display);

/*******************
*     NAME:    lv_poc_activity_send_sign
*   AUTHOR:    lugj
* DESCRIPT:    从activity查询表中查询activity,并给其发送一个消息
*     DATE:    2019-10-30
********************/
lv_poc_activity_t *	lv_poc_activity_send_sign(lv_poc_Activity_Id_t ID, int sign, void * param);

/*******************
*     NAME:    lv_poc_get_display_width
*   AUTHOR:    lugj
* DESCRIPT:    获取显示区域宽
*     DATE:    2019-10-31
********************/
lv_coord_t lv_poc_get_display_width(const lv_poc_display_t * display);

/*******************
*     NAME:    lv_poc_get_display_height
*   AUTHOR:    lugj
* DESCRIPT:    获取显示区域高
*     DATE:    2019-10-31
********************/
lv_coord_t lv_poc_get_display_height(lv_poc_display_t * display);


/*******************
*     NAME:    lv_poc_get_display_area
*   AUTHOR:    lugj
* DESCRIPT:    获取显示区域范围
*     DATE:    2019-10-31
********************/
void lv_poc_get_display_area(lv_poc_display_t * display, lv_area_t * area);

/*******************
*     NAME:    lv_poc_status_bar_task_ext_add
*   AUTHOR:    lugj
* DESCRIPT:    添加状态栏额外任务
*     DATE:    2019-10-31
********************/
bool lv_poc_status_bar_task_ext_add(status_bar_task_t task);


/*******************
*     NAME:    lv_poc_activity_hidden
*   AUTHOR:    lugj
* DESCRIPT:   隐藏activity
*     DATE:    2019-10-31
********************/
bool lv_poc_activity_hidden(lv_poc_activity_t * activity, bool isHidden);


/*******************
*     NAME:    lv_poc_activity_set_idle
*   AUTHOR:    lugj
* DESCRIPT:   设置idle activity
*     DATE:    2019-10-31
********************/
bool lv_poc_activity_set_idle(lv_poc_activity_t * idle_activity);

/*******************
*     NAME:    lv_poc_activity_enter_idle
*   AUTHOR:    lugj
* DESCRIPT:  回到主界面
*     DATE:    2019-10-31
********************/
void lv_poc_activity_enter_idle(void);

/*******************
*     NAME:    lv_poc_activity_pre
*   AUTHOR:    lugj
* DESCRIPT:   进入上一个activity
*     DATE:    2019-10-31
********************/
lv_poc_activity_t * lv_poc_activity_pre(bool forcs);


/*******************
*     NAME:    lv_poc_update_stabar_sim_img
*   AUTHOR:    lugj
* DESCRIPT:    更新SIM卡图标
*     DATE:    2019-10-31
********************/
void lv_poc_update_stabar_sim_img(void);

/*******************
*     NAME:    lv_poc_get_refresh_ui
*   AUTHOR:    lugj
* DESCRIPT:    获取是否刷新UI
*     DATE:    2019-10-31
********************/
int lv_poc_get_refresh_ui(void);

/*******************
*     NAME:    lv_poc_refresh_ui_next
*   AUTHOR:    lugj
* DESCRIPT:    刷新下一个UI
*     DATE:    2019-10-31
********************/
void lv_poc_refresh_ui_next(void);


/*******************
*     NAME:    lv_poc_member_list_cb_set_active
*   AUTHOR:    lugj
* DESCRIPT:    成员列表激活接口
*     DATE:    2020-05-22
********************/
void lv_poc_member_list_cb_set_active(lv_poc_Activity_Id_t activity_id, bool enable);


/*******************
*     NAME:    lv_poc_group_list_cb_set_active
*   AUTHOR:    lugj
* DESCRIPT:    群组列表激活接口
*     DATE:    2020/5/22
********************/
void lv_poc_group_list_cb_set_active(lv_poc_Activity_Id_t activity_id, bool enable);

extern nv_poc_setting_msg_t * poc_setting_conf;

#ifdef __cplusplus
}
#endif


#endif //__INCLUDE_LV_INTER_PHONE_OBJ__

