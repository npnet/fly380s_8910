#ifndef __LV_POC_TYPR_H_
#define __LV_POC_TYPR_H_
#include "lvgl.h"

//typedef char         		 INT8;
typedef unsigned char 		 UINT8;
typedef short int     		 INT16;
typedef unsigned short int   UINT16;
typedef int          		 INT32;
typedef unsigned int  		 UINT32;
//typedef char         		 int8_t;
typedef unsigned char 		 uint8_t;
typedef short int    		 int16_t;
typedef unsigned short int   uint16_t;
//typedef int          		 int32_t;
//typedef unsigned int  	 uint32_t;

#define IN  const
#define OUT

#define POC_UI_MACRO     0
#if POC_UI_MACRO != 0
#define GROUP_EQUATION(A,B,C,D,E)
#define MEMBER_EQUATION(A,B,C,D,E)
#else
#define GROUP_EQUATION(A,B,C,D,E) (0 == strcmp((A),(B)))
#define MEMBER_EQUATION(A,B,C,D,E) (0 == strcmp((A),(B)))
#endif

#define LIST_ELEMENT_NAME_MAX_LENGTH 64

#define LV_POC_NAME_LEN LIST_ELEMENT_NAME_MAX_LENGTH

#define POC_MAX_BRIGHT 9

/*******************
*     NAME:   lv_poc_time_t
*   AUTHOR:   lugj
* DESCRIPT:   获取时间的结构体
*     DATE:   2019-11-01
********************/
typedef struct
{
    INT32 tm_sec;                                                                 // seconds [0,59]
    INT32 tm_min;                                                                 // minutes [0,59]
    INT32 tm_hour;                                                                // hour [0,23]
    INT32 tm_mday;                                                                // day of month [1,31]
    INT32 tm_mon;                                                                 // month of year [1,12]
    INT32 tm_year;                                                                // since 1970
    INT32 tm_wday;                                                                // sunday = 0
} lv_poc_time_t;



typedef struct
{
	uint8_t big_font_switch;                        //[0]close   [1]open   [default 1]
	uint8_t list_page_colum_count;                  //[3,4] [default 3]-------------------------------
	uint32_t list_btn_current_font;                 //
	uint32_t list_btn_big_font;                     //
	uint32_t list_btn_small_font;                   //   area of big_font_switch
	uint32_t about_label_current_font;              //
	uint32_t about_label_big_font;                  //
	uint32_t about_label_small_font;                //-------------------------------
	uint32_t win_title_font;
	uint32_t activity_control_font;
	uint32_t status_bar_time_font;
	//uint32_t status_bar_signal_type_font;
	uint32_t idle_big_clock_font;
	uint32_t idle_date_label_font;
	uint32_t idle_page2_msg_font;
} nv_poc_font_size_msg_t;


typedef struct
{
	uint32_t style_base;
	uint32_t style_list_scroll;
	uint32_t style_list_page;
	uint32_t style_list_btn_rel;
	uint32_t style_list_btn_pr;
	uint32_t style_list_btn_ina;
	uint32_t style_win_header;
	uint32_t style_idle_big_clock;
	uint32_t style_idle_date_label;
	uint32_t style_idle_msg_label;
	uint32_t style_switch_bg;
	uint32_t style_switch_indic;
	uint32_t style_switch_knob_off;
	uint32_t style_switch_knob_on;
	uint32_t style_rb;
	uint32_t style_cb;
	uint32_t style_about_label;
	uint32_t style_status_bar;
	uint32_t style_status_bar_time;
	uint32_t style_control;
} nv_poc_theme_msg_node_t;

typedef struct
{
	uint8_t  type;                      //[0] white theme   [1]black theme    [default 0]
	nv_poc_theme_msg_node_t * current_theme;
	nv_poc_theme_msg_node_t * black;
	nv_poc_theme_msg_node_t * white;
} nv_poc_theme_msg_t;

typedef struct
{
	uint8_t read_and_write_check;
	uint8_t btn_voice_switch;         //[0]close   [1]open   [default 0]
	uint8_t voice_broadcast_switch;   //[0]close   [1]open   [default 0]
	uint8_t keypad_led_switch;        //[0]close   [1]open   [default 0]
	uint8_t GPS_switch;               //[0]close   [1]open   [default 0]
	uint8_t electric_torch_switch;    //[0]close   [1]open   [default 0]
	uint8_t screen_brightness;        //[0 - 9]    [default 4]
	uint8_t screen_bright_time;       //[0]5秒 [1]15秒 [2]30秒 [3]1分钟 [4]2分钟 [5]5分钟 [6]10分钟 [7]30分钟     [default 2]
	//uint8_t current_theme;          //[0] white theme   [1]black theme    [default 0]
	uint8_t main_SIM;                 //[0]SIM 1   [1]SIM 2     [default 0]
	uint8_t net_type;                 //[0]4G/3G/2G  [1]only 3G/2G    [default 0]
	uint8_t volume;                   //[0-10]    [default 5]
	uint8_t language;                 //[0]简体中文       [default 0]
	nv_poc_font_size_msg_t font;
	nv_poc_theme_msg_t theme;
} nv_poc_setting_msg_t;

typedef enum
{
    POC_SIM_1 = 0,
    POC_SIM_2
} POC_SIM_ID;

typedef enum
{
    POC_CHG_CONNECTED,                                                          // charger connected
    POC_CHG_DISCONNECTED                                                        // charger disconnected
} POC_CHG_STATUS;

typedef struct{
    UINT32 battery_status;   // 0 - no battery; 1 - has battery
    UINT32 battery_val_mV;   // voltage
    UINT32 charge_cur_mA;    // current
    INT32 battery_temp;     // temperature
    int8_t battery_value;     // surplus electric quantity
    POC_CHG_STATUS charging;         // is charging
} battery_values_t;

typedef enum
{
    MMI_MODEM_SIGNAL_BAR_0,
    MMI_MODEM_SIGNAL_BAR_1,
    MMI_MODEM_SIGNAL_BAR_2,
    MMI_MODEM_SIGNAL_BAR_3,
    MMI_MODEM_SIGNAL_BAR_4,
    MMI_MODEM_SIGNAL_BAR_5
} POC_MMI_MODEM_SIGNAL_BAR;

typedef enum
{
    MMI_MODEM_PLMN_RAT_GSM = 0,                                                 // GSM network
    MMI_MODEM_PLMN_RAT_UMTS,                                                    // UTRAN network
    MMI_MODEM_PLMN_RAT_LTE,                                                     // LTE network
    MMI_MODEM_PLMN_RAT_UNKNOW
} POC_MMI_MODEM_PLMN_RAT;

typedef enum
{
	POC_MMI_VOICE_MSG,
	POC_MMI_VOICE_NOTE,
	POC_MMI_VOICE_KEY,
	POC_MMI_VOICE_PLAY,
	POC_MMI_VOICE_CALL,
} POC_MMI_VOICE_TYPE_E;

typedef enum
{
	LV_GROUP_KEY_UP             = LV_KEY_UP,  /*0x11*/
	LV_GROUP_KEY_DOWN           = LV_KEY_DOWN,  /*0x12*/
	LV_GROUP_KEY_RIGHT          = LV_KEY_RIGHT,  /*0x13*/
	LV_GROUP_KEY_LEFT           = LV_KEY_LEFT,  /*0x14*/
	LV_GROUP_KEY_ESC            = LV_KEY_ESC,  /*0x1B*/
	LV_GROUP_KEY_DEL            = LV_KEY_DEL, /*0x7F*/
	LV_GROUP_KEY_BACKSPACE      = LV_KEY_BACKSPACE,   /*0x08*/
	LV_GROUP_KEY_ENTER          = LV_KEY_ENTER,  /*0x0A, '\n'*/
	LV_GROUP_KEY_NEXT           = LV_KEY_NEXT,   /*0x09, '\t'*/
	LV_GROUP_KEY_PREV           = LV_KEY_PREV,  /*0x0B, '*/
	LV_GROUP_KEY_HOME           = LV_KEY_HOME,   /*0x02, STX*/
	LV_GROUP_KEY_END            = LV_KEY_END,   /*0x03, ETX*/
	LV_GROUP_KEY_GP             = 43,
	LV_GROUP_KEY_MB             = 44,
	LV_GROUP_KEY_VOL_DOWN       = 45,
	LV_GROUP_KEY_VOL_UP         = 46,
	LV_GROUP_KEY_POC            = 47,
	LV_GROUP_KEY_SET            = 48
} LV_GROUP_KEY_E;

typedef enum {
    LIST_TYPE_MEMBER,
    LIST_TYPE_GROUP,
    LIST_TYPE_GROUP_BUILD,
    LIST_TYPE_CALL,
    LIST_TYPE_NUM
} lv_poc_list_type_t;

typedef enum {
    POC_OPERATE_FAILD,
    POC_OPERATE_SECCESS,

    POC_MEMBER_ONLINE,
    POC_MEMBER_OFFLINE,
    POC_MEMBER_EXISTS,
    POC_MEMBER_NONENTITY,

    POC_GROUP_EXISTS,
    POC_GROUP_NONENTITY,
    POC_GROUP_EMPTY,
    POC_GROUP_NON_NULL,

    POC_UNKNOWN_FAULT
} lv_poc_status_t;

typedef struct _list_element_t{
    char name[LIST_ELEMENT_NAME_MAX_LENGTH];
    lv_obj_t * list_item;
    void * information;
    struct _list_element_t * next;
} list_element_t;

typedef struct {
    unsigned int group_number;
    list_element_t * group_list;
} lv_poc_group_list_t;

typedef struct _lv_poc_audio_dsc_t
{
	uint32_t data_size;
    uint8_t * data;
} lv_poc_audio_dsc_t;

#endif // __LV_POC_TYPR_H_
