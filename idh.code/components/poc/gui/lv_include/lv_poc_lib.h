#ifndef __LV_POC_LIB_H_
#define __LV_POC_LIB_H_

#include "lvgl.h"
#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc_activity.h"

/*
      name : lv_poc_get_keypad_dev
    return : get keypad indev
      date : 2020-04-22
*/
OUT lv_indev_t *
lv_poc_get_keypad_dev(void);

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
OUT uint32_t
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
OUT int8_t
poc_get_lcd_status(void);

/*
      name : poc_set_lcd_blacklight
     param : get lcd state
      date : 2020-03-30
*/
void
poc_set_lcd_blacklight(IN int8_t blacklight);

/*
      name : poc_set_lcd_blacklight
     param : get lcd state
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
      name : poc_battery_get_values
     param : point a battery buff
      date : 2020-03-30
*/
void
poc_battery_get_values(OUT battery_values_t *values);

/*
      name : poc_battery_charge_status
    return : POC_CHG_STATUS
      date : 2020-03-30
*/
OUT POC_CHG_STATUS
poc_battery_charge_status(void);

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
poc_get_operator_req(IN POC_SIM_ID sim, OUT int8_t * operator, OUT POC_MMI_MODEM_PLMN_RAT * rat);

/*
      name : poc_mmi_poc_setting_config
     param : [poc_setting] IN param
      date : 2020-03-30
*/
void
poc_mmi_poc_setting_config(OUT nv_poc_setting_msg_t * poc_setting);

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


#endif //__LV_POC_LIB_H_
