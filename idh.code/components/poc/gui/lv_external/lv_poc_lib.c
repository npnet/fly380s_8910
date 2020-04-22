#include "lv_include/lv_poc_lib.h"
#include "lv_objx/lv_poc_obj/lv_poc_font_resource.h"
#include "lv_objx/lv_poc_obj/lv_poc_obj.h"
#include "lv_gui_main.h"


static nv_poc_setting_msg_t poc_setting_conf_local = {0};
static int8_t poc_lcd_state = 1;
static int8_t poc_lcd_blacklight = 0;
static nv_poc_theme_msg_node_t theme_white = {0};
static nv_poc_theme_msg_node_t theme_black = {0};

/*
      name : lv_poc_get_keypad_dev
    return : get keypad indev
      date : 2020-04-22
*/
OUT lv_indev_t *
lv_poc_get_keypad_dev(void)
{
	return lvGuiGetKeyPad();
}

/*
      name : lv_poc_setting_conf_read
    return : point a buff
      date : 2020-03-30
*/
OUT nv_poc_setting_msg_t *
lv_poc_setting_conf_read(void)
{
    return &poc_setting_conf_local;
}

/*
      name : lv_poc_setting_conf_write
    return : length of buff
      date : 2020-03-30
*/
OUT uint32_t
lv_poc_setting_conf_write(void)
{
    return sizeof(nv_poc_setting_msg_t);
}

/*
      name : lv_poc_setting_get_current_volume
     param : POC_MMI_VOICE_TYPE_E
    return : current volum
      date : 2020-03-30
*/
OUT uint8_t
lv_poc_setting_get_current_volume(IN POC_MMI_VOICE_TYPE_E type)
{
	if(type == POC_MMI_VOICE_MSG)
	{
		return 4;
	}
	else
	{
		return 1;
	}
}

/*
      name : lv_poc_setting_set_current_volume
     param :
     			[type]  POC_MMI_VOICE_TYPE_E
     			[volum] config volum
     			[play]  need paly msg audio
      date : 2020-03-30
*/
void
lv_poc_setting_set_current_volume(IN POC_MMI_VOICE_TYPE_E type, IN uint8_t volume, IN bool play)
{
	uint8_t local_volum = 0;
	if(type == POC_MMI_VOICE_MSG)
	{
		local_volum = volume;
	}

	if(play)
	{
		//play a audio to note user change volum
		(int)local_volum;
	}
}

/*
      name : lv_poc_get_time
    return : get local time
      date : 2020-03-30
*/
void
lv_poc_get_time(OUT lv_poc_time_t * time)
{
	if(time == NULL)
	{
		return;
	}
	time->tm_year = 2020;
	time->tm_mon = 3;
	time->tm_mday = 31;
	time->tm_hour = 10;
	time->tm_min = 5;
	time->tm_sec = 45;
	time->tm_wday = 2;
}

/*
      name : poc_set_lcd_status
     param : config lcd state
      date : 2020-03-30
*/
void
poc_set_lcd_status(IN int8_t wakeup)
{
	poc_lcd_state = wakeup;
}

/*
      name : poc_get_lcd_status
    return : get lcd state
      date : 2020-03-30
*/
OUT int8_t
poc_get_lcd_status(void)
{
	return poc_lcd_state;
}

/*
      name : poc_set_lcd_blacklight
    return : set lcd blacklight
      date : 2020-03-30
*/
void
poc_set_lcd_blacklight(IN int8_t blacklight)
{
	poc_lcd_blacklight = blacklight;
}

/*
      name : poc_play_btn_voice_one_time
     param :
     			[volum] volum when playing
     			[quiet] quiet mode
      date : 2020-03-30
*/
void
poc_play_btn_voice_one_time(IN int8_t volum, IN bool quiet)
{
	if(!quiet)
	{
		// playing
		(int)volum;
	}
}

/*
      name : poc_battery_get_values
     param : point a battery buff
      date : 2020-03-30
*/
void
poc_battery_get_values(OUT battery_values_t *values)
{
	values->battery_status = 1;
	values->battery_temp = 34;
	values->battery_val_mV = 4;
	values->charge_cur_mA = 123;
	values->battery_value = 56;
	values->charging = 0;
}

/*
      name : poc_battery_charge_status
    return : POC_CHG_STATUS
      date : 2020-03-30
*/
OUT POC_CHG_STATUS
poc_battery_charge_status(void)
{
	return POC_CHG_DISCONNECTED;
}

/*
      name : poc_check_sim_prsent
     param : check SIM
    return :
    			[true]  SIM is present
    			[false] SIM isn't present
      date : 2020-03-30
*/
OUT bool
poc_check_sim_prsent(IN POC_SIM_ID sim)
{
	switch(sim)
	{
		case POC_SIM_1:
		{
			return true;
		}

		case POC_SIM_2:
		{
			return true;
		}

		default:
		{
			return false;
		}
	}
}

/*
      name : poc_get_signal_bar_strenth
     param : SIM_ID
    return : POC_MMI_MODEM_SIGNAL_BAR
      date : 2020-03-30
*/
OUT POC_MMI_MODEM_SIGNAL_BAR
poc_get_signal_bar_strenth(POC_SIM_ID sim)
{
	return MMI_MODEM_SIGNAL_BAR_3;
}

/*
      name : get Operator(short name)
     param :
     			[sim] POC_SIM_ID
         		[operator]
         		[rat] network type
  return  void
*/
void
poc_get_operator_req(IN POC_SIM_ID sim, OUT int8_t * operator, OUT POC_MMI_MODEM_PLMN_RAT * rat)
{
	if(!poc_check_sim_prsent(sim))
	{
		return;
	}
	char net_type_name[34] = "LTE NETWORK";

	strcpy(operator, net_type_name);
	*rat = MMI_MODEM_PLMN_RAT_LTE;
}

/*
      name : poc_mmi_poc_setting_config
     param : [poc_setting] IN param
      date : 2020-03-30
*/
void
poc_mmi_poc_setting_config(OUT nv_poc_setting_msg_t * poc_setting)
{
#if 0
	poc_setting->font.list_btn_big_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.list_btn_small_font = (uint32_t)&lv_poc_font_noto_sans_cjk_16;
	poc_setting->font.about_label_big_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	poc_setting->font.about_label_small_font = (uint32_t)&lv_poc_font_noto_sans_cjk_14;
	poc_setting->font.win_title_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	poc_setting->font.activity_control_font = (uint32_t)&lv_poc_font_noto_sans_cjk_16;
	poc_setting->font.status_bar_time_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	//poc_setting->font.status_bar_signal_type_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.idle_big_clock_font = (uint32_t)&lv_poc_font_noto_sans_cjk_40;
	poc_setting->font.idle_date_label_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.idle_page2_msg_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
#else
	poc_setting->font.list_btn_big_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.list_btn_small_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.about_label_big_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.about_label_small_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.win_title_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.activity_control_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.status_bar_time_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	//poc_setting->font.status_bar_signal_type_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.idle_big_clock_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.idle_date_label_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.idle_page2_msg_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
#endif

	poc_setting->theme.white = &theme_white;
	poc_setting->theme.white->style_list_scroll = (uint32_t)&theme_white_style_list_scroll;
	poc_setting->theme.white->style_list_page = (uint32_t)&theme_white_style_list_page;
	poc_setting->theme.white->style_list_btn_rel = (uint32_t)&theme_white_style_list_btn_rel;
	poc_setting->theme.white->style_list_btn_pr = (uint32_t)&theme_white_style_list_btn_pr;
	poc_setting->theme.white->style_list_btn_ina = (uint32_t)&theme_white_style_list_btn_ina;
	poc_setting->theme.white->style_idle_big_clock = (uint32_t)&theme_white_style_idle_big_clock;
	poc_setting->theme.white->style_idle_date_label = (uint32_t)&theme_white_style_idle_date_label;
	poc_setting->theme.white->style_idle_msg_label = (uint32_t)&theme_white_style_idle_msg_label;
	poc_setting->theme.white->style_about_label = (uint32_t)&theme_white_style_about_label;
	poc_setting->theme.white->style_cb = (uint32_t)&theme_white_style_rb;
	poc_setting->theme.white->style_rb = (uint32_t)&theme_white_style_cb;
	poc_setting->theme.white->style_switch_bg = (uint32_t)&theme_white_style_switch_bg;
	poc_setting->theme.white->style_switch_indic = (uint32_t)&theme_white_style_switch_indic;
	poc_setting->theme.white->style_switch_knob_off = (uint32_t)&theme_white_style_switch_knob_off;
	poc_setting->theme.white->style_switch_knob_on = (uint32_t)&theme_white_style_switch_knob_on;
	poc_setting->theme.white->style_status_bar = (uint32_t)&theme_white_style_status_bar;
	poc_setting->theme.white->style_status_bar_time = (uint32_t)&theme_white_style_status_bar_time;
	poc_setting->theme.white->style_win_header = (uint32_t)&theme_white_style_win_header;
	poc_setting->theme.white->style_control = (uint32_t)&theme_white_style_control;


	poc_setting->theme.black = &theme_black;
	poc_setting->theme.black->style_list_scroll = (uint32_t)&theme_black_style_list_scroll;
	poc_setting->theme.black->style_list_page = (uint32_t)&theme_black_style_list_page;
	poc_setting->theme.black->style_list_btn_rel = (uint32_t)&theme_black_style_list_btn_rel;
	poc_setting->theme.black->style_list_btn_pr = (uint32_t)&theme_black_style_list_btn_pr;
	poc_setting->theme.black->style_list_btn_ina = (uint32_t)&theme_black_style_list_btn_ina;
	poc_setting->theme.black->style_idle_big_clock = (uint32_t)&theme_black_style_idle_big_clock;
	poc_setting->theme.black->style_idle_date_label = (uint32_t)&theme_black_style_idle_date_label;
	poc_setting->theme.black->style_idle_msg_label = (uint32_t)&theme_black_style_idle_msg_label;
	poc_setting->theme.black->style_about_label = (uint32_t)&theme_black_style_about_label;
	poc_setting->theme.black->style_cb = (uint32_t)&theme_black_style_rb;
	poc_setting->theme.black->style_rb = (uint32_t)&theme_black_style_cb;
	poc_setting->theme.black->style_switch_bg = (uint32_t)&theme_black_style_switch_bg;
	poc_setting->theme.black->style_switch_indic = (uint32_t)&theme_black_style_switch_indic;
	poc_setting->theme.black->style_switch_knob_off = (uint32_t)&theme_black_style_switch_knob_off;
	poc_setting->theme.black->style_switch_knob_on = (uint32_t)&theme_black_style_switch_knob_on;
	poc_setting->theme.black->style_status_bar = (uint32_t)&theme_black_style_status_bar;
	poc_setting->theme.black->style_status_bar_time = (uint32_t)&theme_black_style_status_bar_time;
	poc_setting->theme.black->style_win_header = (uint32_t)&theme_black_style_win_header;
	poc_setting->theme.black->style_control = (uint32_t)&theme_black_style_control;

	poc_setting->theme.type = 0;
	poc_setting->theme.current_theme = poc_setting->theme.black;
	poc_setting->read_and_write_check = 0x3f;
	poc_setting->btn_voice_switch = 0;
	poc_setting->voice_broadcast_switch = 0;
	poc_setting->keypad_led_switch = 0;
	poc_setting->GPS_switch = 0;
	poc_setting->electric_torch_switch = 0;
	poc_setting->screen_brightness = 3;
	poc_setting->screen_bright_time = 2;
	//poc_setting->current_theme = 0;
	poc_setting->main_SIM = 0;
	poc_setting->net_type = 0;
	poc_setting->font.big_font_switch = 1;
	poc_setting->font.list_page_colum_count = 3;
	poc_setting->font.list_btn_current_font = poc_setting->font.list_btn_big_font;
	poc_setting->font.about_label_current_font = poc_setting->font.about_label_big_font;
	poc_setting->volume = 5;
	poc_setting->language = 0;
}

/*
      name : poc_sys_delay
     param : time in Ms
      date : 2020-03-30
*/
void
poc_sys_delay(IN uint32_t ms)
{
	SDL_Delay(ms);
}

/*
      name : poc_get_device_imei_rep
     param : get device imei
      date : 2020-03-30
*/
void
poc_get_device_imei_rep(OUT int8_t * imei)
{
	strcpy(imei, "12234566577");
}

/*
      name : poc_get_device_iccid_rep
     param : get device iccid
      date : 2020-03-30
*/
void
poc_get_device_iccid_rep(int8_t * iccid)
{
	strcpy(iccid, "52345165461");
}
