#ifndef __LV_POC_LIB_H_
#define __LV_POC_LIB_H_

#include "lvgl.h"
#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc_activity.h"
#include "lv_apps/lv_poc_member_list/lv_poc_member_list.h"
#include "lv_apps/lv_poc_sntp/lv_poc_sntp.h"
#include "hwregs.h"
#include "drv_gpio.h"
#include "hal_iomux.h"
#include "lv_apps/lv_poc_member_list/lv_poc_member_list.h"
#include "guiIdtCom_api.h"
#include "../../cfw/include/cfw.h"
#include "../../newlib/include/stdlib.h"

#define LV_POC_ACTIVITY_ATTRIBUTE_CB_SET_SIZE (10)

#define AP_ASSERT_ENABLE/*enable assert*/

#ifdef AP_ASSERT_ENABLE
#define Ap_OSI_ASSERT(expect_true, info) OSI_ASSERT(expect_true, info)
#endif

enum {
	poc_torch_led   = 2,//IO touch
	poc_red_led     = 10,//IO redled
	poc_green_led   = 13,//IO greenled
	poc_horn_sound  = 9,//IO horn
	poc_head_set  = 8,//IO headset
	poc_gps_int   = 12,//IO ant
	poc_iic_scl   = 14,//IO scl
	poc_iic_sda   = 15,//IO sda
};

//字符串长度:
// 1.32字节，号码，参考号(当前时间+序号,包括呼叫参考号等唯一标识，在一个机器内唯一，不同机器不唯一)
typedef unsigned char UC_32[32];
// 2.64字节，用户名，密码，鉴权参数
typedef unsigned char UC_64[64];

//通过消息获取的群组信息
typedef struct Msg_GROUP_MEMBER_s
{
    uint8_t           ucPrio;             //优先级
    uint8_t           ucUTType;           //UTType,UT_TYPE_NONE
    uint8_t           ucAttr;             //终端属性,UT_ATTR_HS
    UC_32           ucNum;              //号码
    UC_64           ucName;             //名字
    UC_32           ucAGNum;            //关联组信息
    uint8_t           ucChanNum;          //摄像头通道个数
    uint8_t           ucStatus;           //主状态 UT_STATUS_OFFLINE之类
    uint8_t           ucFGCount;          //父组个数
}Msg_GROUP_MEMBER_s;

//通过消息获取的组数据
typedef struct Msg_GData_s
{
    unsigned long dwNum;                      //用户个数
    Msg_GROUP_MEMBER_s  member[1024];   //组成员
}Msg_GData_s;

//组信息
typedef struct _CGroup
{
    uint8_t   m_ucGNum[32];     //组号码
    uint8_t   m_ucGName[64];    //组名字
    uint8_t   m_ucPriority;     //组优先级
}CGroup;

/*
	  name :回调函数
	  param :
	  date : 2020-05-12
*/
typedef void (*get_group_list_cb)(int result_type);
typedef void (*lv_poc_get_group_list_cb_t)(int msg_type, uint32_t num, CGroup *group);

/*
	  name :回调函数
	  param :
	  date : 2020-05-12
*/
typedef void (*get_member_list_cb)(int msg_type);
typedef void (*lv_poc_get_member_list_cb_t)(int msg_type, unsigned long num, Msg_GData_s *pGroup);
/*
	  name :回调函数
	  param :
	  date : 2020-05-20
*/
typedef void (*poc_build_group_cb)(int result_type);
/*
	  name :回调函数
	  param :
	  date : 2020-05-21
*/
typedef void (*poc_get_member_status_cb)(int status);

/*
	  name :回调函数
	  param :
	  date : 2020-05-22
*/
typedef void (*poc_set_member_call_status_cb)(int current_status, int dest_status);
/*
	  name :回调函数
	  param :
	  date : 2020-05-21
*/
typedef void (*poc_set_current_group_cb)(int result_type);

typedef lv_poc_status_t (*lv_poc_member_list_add_cb)(lv_poc_member_list_t *member_list_obj, const char * name, bool is_online, void * information);

typedef void (*lv_poc_member_list_remove_cb)(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

typedef void (*lv_poc_member_list_clear_cb)(lv_poc_member_list_t *member_list_obj);

typedef int (*lv_poc_member_list_get_information_cb)(lv_poc_member_list_t *member_list_obj, const char * name, void *** information);

typedef void (*lv_poc_member_list_refresh_cb)(lv_task_t *task);

typedef void (*lv_poc_member_list_refresh_with_data_cb)(lv_poc_member_list_t *member_list_obj);

typedef lv_poc_status_t (*lv_poc_member_list_move_top_cb)(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_member_list_move_bottom_cb)(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_member_list_move_up_cb)(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_member_list_move_down_cb)(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_member_list_set_state_cb)(lv_poc_member_list_t *member_list_obj, const char * name, void * information, bool is_online);

typedef lv_poc_status_t (*lv_poc_member_list_is_exists_cb)(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_member_list_get_state_cb)(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

typedef void (*lv_poc_group_list_member_act_cb)(lv_task_t * task);

typedef lv_poc_status_t (*lv_poc_group_list_add_cb)(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

typedef void (*lv_poc_group_list_remove_cb)(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

typedef int (*lv_poc_group_list_get_information_cb)(lv_poc_group_list_t *group_list_obj, const char * name, void *** information);

typedef void (*lv_poc_group_list_refresh_cb)(lv_task_t *task);

typedef void (*lv_poc_group_list_refresh_with_data_cb)(lv_poc_group_list_t *group_list_obj);

typedef lv_poc_status_t (*lv_poc_group_list_move_top_cb)(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_group_list_move_bottom_cb)(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_group_list_move_up_cb)(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_group_list_move_down_cb)(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_group_list_is_exists_cb)(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

typedef lv_poc_status_t (*lv_poc_group_list_lock_group_cb)(lv_poc_group_list_t *group_list_obj, lv_poc_group_oprator_type opt);

/*
	  name : lv_poc_notation_msg
	  param : msg_type  (1,2)
	          text_1
	          text_2     msg_type = 1
	                                  listen text , display name and group name of speaker
	                     msg_type = 2
	                                  speaking text , you are free.
	                     msg_type = 3
	                                  refresh notation ui
	                     msg_type = 0
	                                  destory notation ui
	  date : 2020-05-22
*/
typedef bool (*lv_poc_notation_msg_cb)(lv_poc_notation_msg_type_t msg_type, const uint8_t *text_1, const uint8_t *text_2);

typedef void (*lv_poc_idle_set_page2_cb)(lv_poc_idle_page2_display_t msg_type, int num, ...);

typedef bool (*lvPocLedIdtCom_Msg_cb)(LVPOCIDTCOM_Led_SignalType_t signal, LVPOCIDTCOM_Led_Period_t ctx, LVPOCIDTCOM_Led_Jump_Count_t count);

typedef struct _lv_poc_activity_attribute_cb_set
{
	struct{
		lv_poc_member_list_add_cb add;
		lv_poc_member_list_remove_cb remove;
		lv_poc_member_list_clear_cb clear;
		lv_poc_member_list_get_information_cb get_info;
		lv_poc_member_list_refresh_with_data_cb refresh;
		lv_poc_member_list_refresh_with_data_cb refresh_with_data;
		lv_poc_member_list_move_top_cb move_to_top;
		lv_poc_member_list_move_bottom_cb move_to_bottom;
		lv_poc_member_list_move_up_cb move_up;
		lv_poc_member_list_move_down_cb move_down;
		lv_poc_member_list_set_state_cb set_state;
		lv_poc_member_list_is_exists_cb exists;
		lv_poc_member_list_get_state_cb get_state;
		lv_poc_group_list_member_act_cb group_member_act;
	} member_list;

	struct{
		lv_poc_group_list_add_cb add;
		lv_poc_group_list_remove_cb remove;
		lv_poc_group_list_get_information_cb get_info;
		lv_poc_group_list_refresh_with_data_cb refresh;
		lv_poc_group_list_refresh_with_data_cb refresh_with_data;
		lv_poc_group_list_move_top_cb move_to_top;
		lv_poc_group_list_move_bottom_cb move_to_bottom;
		lv_poc_group_list_move_up_cb move_up;
		lv_poc_group_list_move_down_cb move_down;
		lv_poc_group_list_is_exists_cb exists;
		lv_poc_group_list_lock_group_cb lock_group;
	} group_list;

	lv_poc_notation_msg_cb window_note;
	lv_poc_idle_set_page2_cb idle_note;
	lvPocLedIdtCom_Msg_cb status_led;
	void (*member_call_open)(void * information);
	void (*member_call_close)(void);
} lv_poc_activity_attribute_cb_set;

typedef struct _lv_poc_build_new_group_t
{
	lv_poc_member_info_t *members;
	int32_t num;
} lv_poc_build_new_group_t;

typedef struct _lv_poc_member_call_config_t
{
	lv_poc_member_info_t members;
	bool enable;
	poc_set_member_call_status_cb func;
} lv_poc_member_call_config_t;

#ifdef __cplusplus
extern "C" {
#endif

extern lv_poc_activity_attribute_cb_set lv_poc_activity_func_cb_set;


/*
      name : lv_poc_get_keypad_dev
    return : get keypad indev
      date : 2020-04-22
*/
OUT lv_indev_t *
lv_poc_get_keypad_dev(void);

/*
      name : lv_poc_get_volum_key_dev
    return : get volum keypad indev
      date : 2020-12-03
*/
OUT lv_indev_t *
lv_poc_get_volum_key_dev(void);

/*
      name : lv_poc_setting_conf_init
    return : bool
      date : 2020-03-30
*/
OUT bool
lv_poc_setting_conf_init(void);

/*
      name : lv_poc_setting_conf_read
    return : point a buff
      date : 2020-03-30
*/
OUT nv_poc_setting_msg_t *
lv_poc_setting_conf_read(void);

/*
      name : lv_poc_setting_conf_write
    return : length of buff
      date : 2020-03-30
*/
OUT int32_t
lv_poc_setting_conf_write(void);

/*
      name : lv_poc_setting_get_current_volume
     param : POC_MMI_VOICE_TYPE_E
    return : current volum
      date : 2020-03-30
*/
OUT uint8_t
lv_poc_setting_get_current_volume(IN POC_MMI_VOICE_TYPE_E type);

/*
      name : lv_poc_setting_set_current_volume
     param :
     			[type]  POC_MMI_VOICE_TYPE_E
     			[volum] config volum
     			[play]  need paly msg audio
      date : 2020-03-30
*/
void
lv_poc_setting_set_current_volume(IN POC_MMI_VOICE_TYPE_E type, IN uint8_t volume, IN bool play);

/*
      name : lv_poc_get_time
    return : get local time
      date : 2020-03-30
*/
void
lv_poc_get_time(OUT lv_poc_time_t * time);

/*
      name : poc_set_lcd_bright_time
     param : config lcd bright time
             [0]5秒 [1]15秒 [2]30秒 [3]1分钟 [4]2分钟 [5]5分钟 [6]10分钟 [7]30分钟     [default 2]
      date : 2020-03-30
*/
void
poc_set_lcd_bright_time(IN uint8_t timeout);

/*
      name : watch_set_lcd_status
     param : config lcd state
      date : 2020-03-30
*/
void
poc_set_lcd_status(IN int8_t wakeup);

/*
      name : watch_get_lcd_status
    return : get lcd state
      date : 2020-03-30
*/
OUT bool
poc_get_lcd_status(void);

/*
      name : poc_set_lcd_blacklight
     param : set lcd state
      date : 2020-03-30
*/
void
poc_set_lcd_blacklight(IN int8_t blacklight);

/*
      name : poc_play_btn_voice_one_time
     param :
     			[volum] volum when playing
     			[quiet] quiet mode
      date : 2020-03-30
*/
void
poc_play_btn_voice_one_time(IN int8_t volum, IN bool quiet);

/*
      name : poc_play_voice_one_time
     param :
     			[voice_type] type
      date : 2020-06-11
*/
void
poc_play_voice_one_time(IN LVPOCAUDIO_Type_e voice_type, IN uint8_t volume, IN bool isBreak);

/*
      name : poc_battery_get_status
     param : point a battery buff
      date : 2020-03-30
*/
void
poc_battery_get_status(OUT battery_values_t *values);

/*
      name : poc_check_sim_prsent
     param : SIM_ID
    return :
    			[true]  SIM is presentlv_poc_activity_ext_t
    			[false] SIM isn't present
      date : 2020-03-30
*/
OUT bool
poc_check_sim_prsent(IN POC_SIM_ID sim);


/*
      name : poc_get_signal_dBm
     param : nSignal
    return : POC_SIGNAL_DBM
      date : 2020-10-12
*/
OUT uint8_t
poc_get_signal_dBm(uint8_t *nSignal);

/*
      name : poc_get_signal_bar_strenth
     param : SIM_ID
    return : POC_MMI_MODEM_SIGNAL_BAR
      date : 2020-03-30
*/
OUT POC_MMI_MODEM_SIGNAL_BAR
poc_get_signal_bar_strenth(POC_SIM_ID sim);

/*
      name : get Operator(short name)
     param :
     			[sim] POC_SIM_ID
         		[operator]
         		[rat] network type
      date : 2020-03-30
*/
void
poc_get_operator_req(IN POC_SIM_ID sim, OUT int8_t * operat, OUT POC_MMI_MODEM_PLMN_RAT * rat);

/*
      name : get status of ps attach
     param :
     			[sim] POC_SIM_ID
      date : 2020-05-06
*/
bool
poc_get_network_ps_attach_status(IN POC_SIM_ID sim);

/*
      name : get status of register network
     param :
     			[sim] POC_SIM_ID
      date : 2020-05-06
*/
bool
poc_get_network_register_status(IN POC_SIM_ID sim);

/*
      name : poc_net_work_config
      param : config network to log idt server
      date : 2020-05-11
*/
void
poc_net_work_config(IN POC_SIM_ID sim);

/*
      name : poc_mmi_poc_setting_config
     param : [poc_setting] IN param
      date : 2020-03-30
*/
void
poc_mmi_poc_setting_config(OUT nv_poc_setting_msg_t * poc_setting);

/*
      name : poc_mmi_poc_setting_config_restart
     param : [poc_setting] IN param
      date : 2020-03-30
*/
void
poc_mmi_poc_setting_config_restart(OUT nv_poc_setting_msg_t * poc_setting);

/*
      name : poc_sys_delay
     param : time in Ms
      date : 2020-03-30
*/
void
poc_sys_delay(IN uint32_t ms);

/*
      name : poc_get_device_imei_rep
     param : get device imei
      date : 2020-03-30
*/
void
poc_get_device_imei_rep(OUT int8_t * imei);

/*
      name : poc_get_device_iccid_rep
     param : get device iccid
      date : 2020-03-30
*/
void
poc_get_device_iccid_rep(int8_t * iccid);

/*
      name : poc_get_device_imsi_rep
     param : get device imsi
      date : 2020-11-18
*/
void
poc_get_device_imsi_rep(int8_t * imsi);

/*
      name : poc_get_device_account_rep
     param : get device account
      date : 2020-04-29
*/
char *
poc_get_device_account_rep(POC_SIM_ID nSim);

/*
      name : poc_broadcast_play_rep
     param : play text in some method
      date : 2020-04-29
*/
bool
poc_broadcast_play_rep(uint8_t * text, uint32_t length, uint8_t broadcast_switch, bool force);

/*
      name : poc_set_torch_status
     param : none
      date : 2020-04-30
*/
bool
poc_get_torch_status(void);

/*
      name : poc_torch_init
     param : none
      date : 2020-04-30
*/
void
poc_torch_init(void);

#ifdef CONFIG_POC_GUI_KEYPAD_LIGHT_SUPPORT
/*
      name : poc_set_keypad_led_status
     param : open  true is open keypad led
      date : 2020-04-30
*/
bool
poc_set_keypad_led_status(bool open);

/*
      name : poc_get_keypad_led_status
     param : none
      date : 2020-04-30
*/
bool
poc_get_keypad_led_status(void);

/*
      name : poc_keypad_led_init
     param : none
      date : 2020-04-30
*/
void
poc_keypad_led_init(void);
#endif

/*
      name : poc_set_PA_status
     param : open  true is open external PA
      date : 2020-04-30
*/
bool
poc_set_ext_pa_status(bool open);

/*
      name : poc_get_ext_pa_status
     param : none
      date : 2020-04-30
*/
bool
poc_get_ext_pa_status(void);

/*
      name : poc_ext_pa_init
     param : none
      date : 2020-04-30
*/
void
poc_ext_pa_init(void);

/*
      name : poc_port_init
      param : port is the IO that needs to be set
      date : 2020-05-08
*/
drvGpioConfig_t *
poc_port_init(void);

/*
      name : poc_set_port_status
      param : open true is open port
      date : 2020-05-08
*/
bool
poc_set_port_status(uint32_t port, drvGpioConfig_t *config,bool open);

/*
      name : poc_set_torch_status
     param : open  true is open torch
      date : 2020-04-30
*/
bool
poc_set_torch_status(bool open);

/*
	  name : poc_set_red_status
	  param : if the status is true,open green led,but...
	  date : 2020-05-09
*/
bool
poc_set_red_status(bool ledstatus);

/*
	  name : poc_set_green_status
	  param : if the status is true,open green led,but...
	  date : 2020-05-09
*/
bool
poc_set_green_status(bool ledstatus);

/*
	  name : lv_poc_get_group_list
	  param :member_list{@group information} func{@callback GUI}
	  date : 2020-05-14
*/
bool
lv_poc_get_group_list(lv_poc_group_list_t * member_list, get_group_list_cb func);

/*
	  name : lv_poc_check_group_equation
	  param :
	  date : 2020-05-19
*/
bool
lv_poc_check_group_equation(void * A, void *B, void *C, void *D, void *E);

/*
	  name : lv_poc_get_member_list
	  param :member_list{@member information} type{@status } func{@callback GUI}
	  date : 2020-05-12
*/
bool
lv_poc_get_member_list(lv_poc_group_info_t group_info, lv_poc_member_list_t * member_list, int type, get_member_list_cb func);

/*
	  name : lv_poc_check_member_equation
	  param :
	  date : 2020-05-19
*/
bool
lv_poc_check_member_equation(void * A, void *B, void *C, void *D, void *E);

/*
	  name : lv_poc_build_new_group
	  param :
	  date : 2020-05-19
*/
bool
lv_poc_build_new_group(lv_poc_member_info_t *members, int32_t num, poc_build_group_cb func);

/*
	  name : lv_poc_get_self_info
	  param :
	  date : 2020-05-20
*/
lv_poc_member_info_t
lv_poc_get_self_info(void);

/*
	  name : lv_poc_get_current_group
	  param :
	  date : 2020-05-22
*/
lv_poc_group_info_t
lv_poc_get_current_group(void);

/*
	  name : lv_poc_set_current_group
	  param :
	  date : 2020-05-22
*/
bool
lv_poc_set_current_group(lv_poc_group_info_t group, poc_set_current_group_cb func);

/*
	  name : lv_poc_get_group_name
	  param :
	  date : 2020-05-22
*/
char *
lv_poc_get_group_name(lv_poc_group_info_t group);

/*
	  name : lv_poc_get_member_name
	  param :
	  date : 2020-05-20
*/
char *
lv_poc_get_member_name(lv_poc_member_info_t members);

/*
	  name : lv_poc_get_member_status
	  param :
	  date : 2020-05-21
*/
bool
lv_poc_get_member_status(lv_poc_member_info_t members, poc_get_member_status_cb func);

/*
	  name : lv_poc_get_member_status
	  param :
	  date : 2020-05-21
*/
bool
lv_poc_set_member_call_status(lv_poc_member_info_t member, bool enable, poc_set_member_call_status_cb func);

/*
	  name : lv_poc_get_lock_group
	  param :
	  date : 2020-06-30
*/
lv_poc_group_info_t
lv_poc_get_lock_group(void);

/*
	  name : lv_poc_set_lock_group
	  param :
	  date : 2020-06-30
*/
bool
lv_poc_set_lock_group(lv_poc_group_oprator_type opt, lv_poc_group_info_t group, void (*func)(lv_poc_group_oprator_type opt));

/*
	  name : poc_get_operator_network_type_req
	 param : none
	author : wangls
  describe : 获取网络类型
	  date : 2020-07-07
*/
void
poc_get_operator_network_type_req(IN POC_SIM_ID sim, OUT int8_t * operat, OUT POC_MMI_MODEM_PLMN_RAT * rat);

/*
	  name : lv_poc_delete_group
	  param :
	  date : 2020-07-09
*/
bool
lv_poc_delete_group(lv_poc_group_info_t group, void (*func)(int result_type));

/*
	  name : lv_poc_ear_ppt_key_init
	 param : none
	author : wangls
  describe : 耳机ppt配置
	  date : 2020-07-30
*/
void lv_poc_ear_ppt_key_init(void);

/*
	  name : lv_poc_get_earppt_state
	 param : none
	author : wangls
  describe : 获取耳机ppt
	  date : 2020-08-03
*/
bool lv_poc_get_earppt_state(void);

/*
	  name : lv_poc_opt_refr_status
	  param :
	  date : 2020-08-24
*/
LVPOCIDTCOM_UNREFOPT_SignalType_t
lv_poc_opt_refr_status(LVPOCIDTCOM_UNREFOPT_SignalType_t status);

/*
	name : lv_poc_get_record_mic_gain
   param : none
  author : wangls
describe : 获取record mic增益
	date : 2020-09-01
*/
bool lv_poc_get_record_mic_gain(void);

/*
	name : lv_poc_set_record_mic_gain
   param : none
  author : wangls
describe : 设置record mic增益
	date : 2020-09-01
*/
bool lv_poc_set_record_mic_gain(lv_poc_record_mic_mode mode, lv_poc_record_mic_path path, lv_poc_record_mic_anaGain anaGain, lv_poc_record_mic_adcGain adcGain);

/*
	  name : poc_config_Lcd_power_vol
	return : none
	  date : 2020-09-12
*/
void poc_config_Lcd_power_vol(void);

/*
	  name : lv_poc_set_charge_status
	  param :
	  date : 2020-09-10
*/
void
lv_poc_set_charge_status(bool status);

/*
	  name : lv_poc_get_charge_status
	  param :
	  date : 2020-09-10
*/
bool
lv_poc_get_charge_status(void);

/*
	  name : lv_poc_set_power_on_status
	  param :
	  date : 2020-08-27
*/
void
lv_poc_set_power_on_status(bool status);

/*
	  name : lv_poc_get_power_on_status
	  param :
	  date : 2020-08-27
*/
bool
lv_poc_get_poweron_is_ready(void);

/*
	  name : lv_poc_get_charge_status
	  describe :设置设备在组里还是外,true为组内
	  param :
	  date : 2020-09-22
*/
void lv_poc_set_group_status(bool status);

/*
	  name : lv_poc_get_charge_status
	  describe :设备是否在组里面
	  param :
	  date : 2020-09-22
*/
bool lv_poc_is_inside_group(void);

/*
	  name : lv_poc_set_group_refr
	  describe :记录当设备在组内返回群组列表时是否有刷新信息
	  example  :设备在组内时的群组列表需刷新的消息，设备弹出锁组信息、设备弹出关机、
				设备弹出删组弹框时由于其他设备建组、删组导致的刷新问题
	  param :
	  date : 2020-09-22
*/
void lv_poc_set_group_refr(bool status);

/*
	  name : lv_poc_set_grouplist_refr_is_complete,lv_poc_set_memberlist_refr_is_complete
	  describe :记录群组或成员列表UI是否刷新准备完成
	  example  :禁止群组或成员列表未刷新完成时退出UI、刷新群组信息导致的死机
	  param :
	  date : 2020-11-05
*/
void lv_poc_set_grouplist_refr_is_complete(bool status);
void lv_poc_set_memberlist_refr_is_complete(bool status);

/*
	  name : lv_poc_is_grouplist_refr_complete,lv_poc_is_memberlist_refr_complete
	  describe :返回群组或成员列表已经刷新完成
	  param :
	  date : 2020-11-05
*/
bool lv_poc_is_grouplist_refr_complete(void);
bool lv_poc_is_memberlist_refr_complete(void);

/*
	  name : lv_poc_is_group_list_refr
	  describe :返回群组列表是否有刷新信息
	  param :
	  date : 2020-09-22
*/
bool lv_poc_is_group_list_refr(void);

/*
      name : poc_set_keypad_led_status
     param : open  true is open keypad led
      date : 2020-04-30
*/
bool
poc_set_gps_ant_status(bool open);

/*
	  name : lv_poc_recorder_Thread
	  param :
	  date : 2020-10-22
*/
void *lv_poc_recorder_Thread(void);

/*
	  name : lv_poc_watchdog_status
	  param :
	  date : 2020-10-30
*/
bool lv_poc_watchdog_status(void);

/*
	  name : lv_poc_play_voice_status
	  param :
	  date : 2020-11-04
*/
bool lv_poc_play_voice_status(void);

/*
	  name : lv_poc_set_refr_error_info
	  describe :记录成员或群组列表异常状态
	  param :
	  date : 2020-11-05
*/
void lv_poc_set_refr_error_info(bool status);

/*
	  name : lv_poc_get_refr_error_info
	  describe :返回群组或成员刷新异常状态
	  param :
	  date : 2020-11-05
*/
bool lv_poc_get_refr_error_info(void);

/*
	  name : lv_poc_set_buildgroup_refr_is_complete
	  describe :记录新建群组列表UI是否刷新准备完成
	  param :
	  date : 2020-11-05
*/
void lv_poc_set_buildgroup_refr_is_complete(bool status);

/*
	  name : lv_poc_is_buildgroup_refr_complete
	  describe :返回新建群组已经刷新完成
	  param :
	  date : 2020-11-05
*/
bool lv_poc_is_buildgroup_refr_complete(void);

/*
	 name : lv_poc_set_auto_deepsleep
	 param :
	 date : 2020-09-29
*/
void
lv_poc_set_auto_deepsleep(bool status);

/*
	  name : poc_set_iic_status
	  param :
	  date : 2020-11-23
*/
bool
poc_set_iic_status(bool iicstatus);

/*
     name : lv_poc_set_headset_status
     param :
     date : 2020-09-29
*/
void
lv_poc_set_headset_status(bool status);

/*
     name : lv_poc_get_headset_is_ready
     param :
     date : 2020-09-29
*/
bool
lv_poc_get_headset_is_ready(void);

/*	  name : lv_poc_set_first_membercall
	  param :
	  date : 2020-11-25
*/
void lv_poc_set_first_membercall(bool status);

/*
	  name : lv_poc_get_first_membercall
	  param :
	  date : 2020-11-25
*/
bool lv_poc_get_first_membercall(void);

/*
	  name : lv_poc_set_screenon_status
	  describe :
	  param :
	  date : 2020-11-28
*/
void lv_poc_set_screenon_status(bool status);

/*
	  name : lv_poc_get_screenon_status
	  describe :
	  param :
	  date : 2020-11-28
*/
bool lv_poc_get_screenon_status(void);

/*
     name : lv_poc_get_audio_voice_status
     param :
     date : 2020-12-04
*/
bool
lv_poc_get_audio_voice_status(void);

/*
	  name : lv_poc_set_loopback_recordplay
	  describe :
	  param :
	  date : 2020-12-09
*/
void lv_poc_set_loopback_recordplay(bool status);

/*
	  name : lv_poc_get_loopback_recordplay_status
	  describe :
	  param :
	  date : 2020-12-09
*/
bool lv_poc_get_loopback_recordplay_status(void);

/*
	 name : lv_poc_volum_get_reconfig_status
	 param :
	 date : 2020-12-04
*/
bool
lv_poc_volum_get_reconfig_status(void);

/*
     name : lv_poc_get_mobile_card_operator
     describe :获取SIM卡运营商
     param :
     date : 2020-10—14
*/
void
lv_poc_get_mobile_card_operator(char *operator_name, bool abbr);

/*
     name : lv_poc_virt_at_init
     param :
     date : 2020-12-08
*/
void lv_poc_virt_at_init(void);

/*
	 name : lv_poc_virt_at_resp_send
	 param :
	 date : 2020-12-08
*/
void lv_poc_virt_at_resp_send(char *cmd);

/*
	 name : lv_poc_get_calib_status
	 param :
	 date : 2020-12-08
*/
int lv_poc_get_calib_status(void);

#ifdef CONFIG_POC_CIT_KEY_SUPPORT
/*
	 name : lv_poc_key_param_init_cb
	 param :
	 date : 2020-12-10
*/
void lv_poc_key_param_init_cb(void);

/*
	 name : lv_poc_type_key_up_cb
	 param :
	 date : 2020-12-10
*/
int lv_poc_type_key_up_cb(bool status);

/*
	 name : lv_poc_type_key_down_cb
	 param :
	 date : 2020-12-10
*/
int lv_poc_type_key_down_cb(bool status);

/*
	 name : lv_poc_type_key_volum_up_cb
	 param :
	 date : 2020-12-10
*/
int lv_poc_type_key_volum_up_cb(bool status);

/*
	 name : lv_poc_type_key_volum_down_cb
	 param :
	 date : 2020-12-10
*/
int lv_poc_type_key_volum_down_cb(bool status);

/*
	 name : lv_poc_type_key_poc_cb
	 param :
	 date : 2020-12-10
*/
int lv_poc_type_key_poc_cb(bool status);

/*
	 name : lv_poc_type_key_power_cb
	 param :
	 date : 2020-12-10
*/
int lv_poc_type_key_power_cb(bool status);
#endif

/*
	 name : lv_poc_type_gps_cb
	 param :
	 date : 2020-12-10
*/
void lv_poc_type_gps_cb(int status);

/*
	 name : lv_poc_type_volum_cb
	 param :
	 date : 2020-12-10
*/
bool lv_poc_type_volum_cb(int status);

/*
	 name : lv_poc_type_rgb_cb
	 param :
	 date : 2020-12-10
*/
void lv_poc_type_rgb_cb(int status);

/*
	 name : lv_poc_type_flash_cb
	 param :
	 date : 2020-12-10
*/
float lv_poc_type_flash_cb(bool status);

/*
	 name : lv_poc_type_plog_switch_cb
	 param :
	 date : 2020-12-12
*/
void lv_poc_type_plog_switch_cb(bool status);

/*
	 name : lv_poc_type_modemlog_switch_cb
	 param :
	 date : 2020-12-12
*/
void lv_poc_type_modemlog_switch_cb(bool status);

#ifdef __cplusplus
}
#endif


#endif //__LV_POC_LIB_H_
