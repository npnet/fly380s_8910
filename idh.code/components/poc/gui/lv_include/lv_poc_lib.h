#ifndef __LV_POC_LIB_H_
#define __LV_POC_LIB_H_

#include "lvgl.h"
#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc_activity.h"
#include "lv_apps/lv_poc_member_list/lv_poc_member_list.h"
#include "hwregs.h"
#include "drv_gpio.h"
#include "hal_iomux.h"
#include "lv_apps/lv_poc_member_list/lv_poc_member_list.h"
#include "guiIdtCom_api.h"

enum {
	poc_torch_led   = 2,//IO touch
	poc_red_led     = 10,//IO redled
	poc_green_led   = 13,//IO greenled
	poc_horn_sound  = 9,//IO horn
};

//字符串长度:
// 1.32字节，号码，参考号(当前时间+序号,包括呼叫参考号等唯一标识，在一个机器内唯一，不同机器不唯一)
typedef unsigned char UC_32[32];
// 2.64字节，用户名，密码，鉴权参数
typedef unsigned char UC_64[64];

//通过消息获取的群组信息
typedef struct Msg_GROUP_MEMBER_s
{
    UC_32           ucNum;              //号码
    UC_64           ucName;             //名字

    uint8_t         ucStatus;           //主状态 UT_STATUS_OFFLINE之类
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

#ifdef __cplusplus
extern "C" {
#endif


/*
      name : lv_poc_get_keypad_dev
    return : get keypad indev
      date : 2020-04-22
*/
OUT lv_indev_t *
lv_poc_get_keypad_dev(void);

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
lv_poc_get_member_list(lv_poc_group_info_t *group_info, lv_poc_member_list_t * member_list, int type, get_member_list_cb func);

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
	  name : lv_poc_get_member_name
	  param :
	  date : 2020-05-20
*/
char *
lv_poc_get_member_name(lv_poc_member_info_t members);

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
	  date : 2020-05-09
*/
extern bool
lv_poc_notation_msg(lv_poc_notation_msg_type_t msg_type, const uint8_t *text_1, const uint8_t *text_2);


#ifdef __cplusplus
}
#endif


#endif //__LV_POC_LIB_H_
