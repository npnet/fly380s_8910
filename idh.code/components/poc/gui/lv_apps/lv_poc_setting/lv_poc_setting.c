
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

typedef void (* lv_poc_setting_item_func_t)(lv_obj_t * obj);
#define LV_POC_SETTING_ITEMS_NUM (12)
static lv_poc_win_t * poc_setting_win;

static const char * bright_str[POC_MAX_BRIGHT] = {"0","1","2","3","4","5","6","7","8"};
static const char * bright_time_str[] = {"5秒","15秒","30秒","1分钟","2分钟","5分钟","10分钟","30分钟"};
#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
static const char * theme_str[] = {"白色","黑色"};
#endif
#ifdef CONFIG_POC_GUI_CHOICE_SIM_SUPPORT
static const char * main_sim_str[] = {"卡1","卡2"};
#endif
#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
static const char * net_type_str[] = {"4G/3G/2G","仅3G/2G"};
#endif
static const char * lv_poc_setting_title_text = "设置";
static const char * lv_poc_setting_btn_text_btn_voice = "按键音";
#ifdef CONFIG_POC_TTS_SUPPORT
static const char * lv_poc_setting_btn_text_broadcast = "语音播报";
#endif
static const char * lv_poc_setting_btn_text_big_font = "大号字体";
#ifdef CONFIG_POC_GUI_KEYPAD_LIGHT_SUPPORT
static const char * lv_poc_setting_btn_text_key_light = "按键灯";
#endif
static const char * lv_poc_setting_btn_text_GPS = "GPS";
static const char * lv_poc_setting_btn_text_tourch = "手电筒";
static const char * lv_poc_setting_btn_text_brightness = "亮度";
static const char * lv_poc_setting_btn_text_brightness_time = "亮屏时间";
#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
static const char * lv_poc_setting_btn_text_theme_switch = "主题切换";
#endif
#ifdef CONFIG_POC_GUI_CHOICE_SIM_SUPPORT
static const char * lv_poc_setting_btn_text_sim_switch = "主卡选择";
#endif
#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
static const char * lv_poc_setting_btn_text_net_switch = "网络切换";
#endif
static char is_poc_setting_update_UI_task_running = 0;
char is_refresh_UI_for_poc = 0;
static lv_poc_setting_item_func_t lv_poc_setting_items_funcs[LV_POC_SETTING_ITEMS_NUM] = {0};

static lv_obj_t * poc_setting_create(lv_poc_display_t *display);

static void poc_setting_destory(lv_obj_t *obj);

static void * poc_setting_list_create(lv_obj_t * parent, lv_area_t display_area);

static void poc_setting_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_setting_pressed_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_obj_t *                 activity_list;
lv_poc_activity_t * poc_setting_activity;
static int setting_selected_item = 0;

static void poc_setting_update_UI_task(lv_task_t * task)
{
	if(is_poc_setting_update_UI_task_running == 1)
		return;
	is_poc_setting_update_UI_task_running = 1;
	lv_obj_del(poc_setting_win->header);
	lv_obj_del(activity_list);
	lv_mem_free(poc_setting_win);
	poc_setting_win = lv_poc_win_create(poc_setting_activity->display, "设置", poc_setting_list_create);
	poc_setting_activity->ext_data = (void *)poc_setting_win;
	is_poc_setting_update_UI_task_running = 0;
}

static void lv_poc_setting_btn_voice_btn_cb(lv_obj_t * obj)
{
    lv_obj_t * ext_obj = NULL;
	ext_obj = (lv_obj_t *)obj->user_data;
	if(poc_setting_conf->btn_voice_switch == 1)
	{
		lv_sw_off(ext_obj, LV_ANIM_OFF);
		poc_setting_conf->btn_voice_switch = 0;
	}
	else
	{
		lv_sw_on(ext_obj, LV_ANIM_OFF);
		poc_setting_conf->btn_voice_switch = 1;
	}
	lv_poc_setting_conf_write();
	//sw_state = lv_sw_get_state(ext_obj) == true? false : true;
	//lv_sw_toggle(ext_obj);
}

#ifdef CONFIG_POC_TTS_SUPPORT
static void lv_poc_setting_broadcast_btn_cb(lv_obj_t * obj)
{
    lv_obj_t * ext_obj = NULL;
	ext_obj = (lv_obj_t *)obj->user_data;
	if(poc_setting_conf->voice_broadcast_switch == 1)
	{
		lv_sw_off(ext_obj, LV_ANIM_OFF);
		poc_setting_conf->voice_broadcast_switch = 0;
	}
	else
	{
		lv_sw_on(ext_obj, LV_ANIM_OFF);
		poc_setting_conf->voice_broadcast_switch = 1;
	}
	lv_poc_setting_conf_write();
	//sw_state = lv_sw_get_state(ext_obj) == true? false : true;
	//lv_sw_toggle(ext_obj);
}
#endif

static void lv_poc_setting_big_font_btn_cb(lv_obj_t * obj)
{
    lv_obj_t * ext_obj = NULL;
	ext_obj = (lv_obj_t *)obj->user_data;
	if(poc_setting_conf->font.big_font_switch == 1)
	{
		lv_sw_off(ext_obj, LV_ANIM_OFF);
		poc_setting_conf->font.big_font_switch = 0;
		poc_setting_conf->font.list_page_colum_count = 4;
		poc_setting_conf->font.list_btn_current_font = poc_setting_conf->font.list_btn_small_font;
		poc_setting_conf->font.about_label_current_font = poc_setting_conf->font.about_label_small_font;
	}
	else
	{
		lv_sw_on(ext_obj, LV_ANIM_OFF);
		poc_setting_conf->font.big_font_switch = 1;
		poc_setting_conf->font.list_page_colum_count = 3;
		poc_setting_conf->font.list_btn_current_font = poc_setting_conf->font.list_btn_big_font;
		poc_setting_conf->font.about_label_current_font = poc_setting_conf->font.about_label_big_font;
	}

	lv_poc_setting_conf_write();
	lv_poc_refresh_ui_next();
	lv_task_t * task = lv_task_create(poc_setting_update_UI_task, 10, LV_TASK_PRIO_LOWEST, NULL);
	lv_task_once(task);
}

#ifdef CONFIG_POC_GUI_KEYPAD_LIGHT_SUPPORT
static void lv_poc_setting_key_light_btn_cb(lv_obj_t * obj)
{
    lv_obj_t * ext_obj = NULL;
	ext_obj = (lv_obj_t *)obj->user_data;
	poc_setting_conf->keypad_led_switch = poc_setting_conf->keypad_led_switch? 0:1;
	poc_set_keypad_led_status(poc_setting_conf->keypad_led_switch);
	if(poc_setting_conf->keypad_led_switch != 0)
	{
		lv_sw_on(ext_obj, LV_ANIM_OFF);
	}
	else
	{
		lv_sw_off(ext_obj, LV_ANIM_OFF);
	}
	lv_poc_setting_conf_write();
}
#endif

static void lv_poc_setting_GPS_btn_cb(lv_obj_t * obj)
{
    lv_obj_t * ext_obj = NULL;
	ext_obj = (lv_obj_t *)obj->user_data;
	if(poc_setting_conf->GPS_switch == 1)
	{
		lv_sw_off(ext_obj, LV_ANIM_OFF);
		poc_setting_conf->GPS_switch = 0;
	}
	else
	{
		lv_sw_on(ext_obj, LV_ANIM_OFF);
		poc_setting_conf->GPS_switch = 1;
	}
	lv_poc_setting_conf_write();
	//sw_state = lv_sw_get_state(ext_obj) == true? false : true;
	//lv_sw_toggle(ext_obj);
}

static void lv_poc_setting_torch_btn_cb(lv_obj_t * obj)
{
    lv_obj_t * ext_obj = NULL;
	ext_obj = (lv_obj_t *)obj->user_data;
	poc_setting_conf->electric_torch_switch = poc_setting_conf->electric_torch_switch? 0:1;
	poc_set_torch_status(poc_setting_conf->electric_torch_switch);
	if(poc_setting_conf->electric_torch_switch != 0)
	{
		lv_sw_on(ext_obj, LV_ANIM_OFF);
	}
	else
	{
		lv_sw_off(ext_obj, LV_ANIM_OFF);
	}
	lv_poc_setting_conf_write();
}

static void lv_poc_setting_brightness_btn_cb(lv_obj_t * obj)
{
    lv_obj_t * ext_obj = NULL;
	int cur_bright;
	ext_obj = (lv_obj_t *)obj->user_data;
	cur_bright = poc_setting_conf->screen_brightness;
	cur_bright = (cur_bright + 1) % POC_MAX_BRIGHT;
	poc_setting_conf->screen_brightness = cur_bright;
	lv_poc_setting_conf_write();
	lv_label_set_text(ext_obj, bright_str[cur_bright]);
	//lcd_backlight(cur_bright);
	poc_set_lcd_blacklight(poc_setting_conf->screen_brightness);
}

static void lv_poc_setting_brightness_time_btn_cb(lv_obj_t * obj)
{
	lv_poc_bright_time_open();
}

#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
static void lv_poc_setting_theme_switch_btn_cb(lv_obj_t * obj)
{
	lv_poc_theme_switch_open();
}
#endif

#ifdef CONFIG_POC_GUI_CHOICE_SIM_SUPPORT
static void lv_poc_setting_sim_switch_btn_cb(lv_obj_t * obj)
{
	lv_poc_main_sim_choice_open();
}
#endif

#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
static void lv_poc_setting_net_switch_btn_cb(lv_obj_t * obj)
{
	lv_poc_net_switch_open();
}
#endif

static lv_obj_t * poc_setting_create(lv_poc_display_t *display)
{
#if 1
    poc_setting_win = lv_poc_win_create(display, lv_poc_setting_title_text, poc_setting_list_create);
#endif
    return (lv_obj_t  *)poc_setting_win;
}

static void poc_setting_destory(lv_obj_t *obj)
{
	poc_setting_activity = NULL;
	setting_selected_item = 0;
}

static void * poc_setting_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, poc_setting_list_config);
    return (void *)activity_list;
}

static void poc_setting_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn;
    lv_obj_t *sw;
    lv_obj_t *label;
    lv_obj_t *btn_label;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_coord_t btn_sw_height = (list_area.y2 - list_area.y1)/3;
    lv_obj_t * btns[LV_POC_SETTING_ITEMS_NUM];
    uint8_t storage_index = 0;
    /*Create styles for the switch*/
    lv_style_t * bg_style;
    lv_style_t * indic_style;
    lv_style_t * knob_on_style;
    lv_style_t * knob_off_style;
   	poc_setting_conf = lv_poc_setting_conf_read();
	bg_style = (lv_style_t *)poc_setting_conf->theme.current_theme->style_switch_bg;
	indic_style = (lv_style_t *)poc_setting_conf->theme.current_theme->style_switch_indic;
	knob_on_style = (lv_style_t *)poc_setting_conf->theme.current_theme->style_switch_knob_off;
	knob_off_style = (lv_style_t *)poc_setting_conf->theme.current_theme->style_switch_knob_on;
	lv_list_clean(list);

    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_btn_voice);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_btn_voice_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    sw = lv_sw_create(btn, NULL);
    lv_sw_set_style(sw, LV_SW_STYLE_BG, bg_style);
    lv_sw_set_style(sw, LV_SW_STYLE_INDIC, indic_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_ON, knob_on_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_OFF, knob_off_style);
    lv_obj_set_size(sw, lv_obj_get_width(sw)*9/17, btn_sw_height*9/17);
    btn->user_data = (void *)sw;
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(sw)*5/4 - 10);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(sw, btn_label, LV_ALIGN_OUT_RIGHT_MID, lv_obj_get_width(sw), 0);
    if(poc_setting_conf->btn_voice_switch == 1)
    {
    	lv_sw_on(sw, LV_ANIM_OFF);
    }
    else
    {
    	lv_sw_off(sw, LV_ANIM_OFF);
    }

    lv_list_set_btn_selected(list, btn);

#ifdef CONFIG_POC_TTS_SUPPORT
    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_broadcast);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_broadcast_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    sw = lv_sw_create(btn, NULL);
    lv_sw_set_style(sw, LV_SW_STYLE_BG, bg_style);
    lv_sw_set_style(sw, LV_SW_STYLE_INDIC, indic_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_ON, knob_on_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_OFF, knob_off_style);
    lv_obj_set_size(sw, lv_obj_get_width(sw)*9/17, btn_sw_height*9/17);
    btn->user_data = (void *)sw;
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(sw)*5/4 - 10);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(sw, btn_label, LV_ALIGN_OUT_RIGHT_MID, lv_obj_get_width(sw), 0);
    if(poc_setting_conf->voice_broadcast_switch == 1)
    {
    	lv_sw_on(sw, LV_ANIM_OFF);
    }
    else
    {
    	lv_sw_off(sw, LV_ANIM_OFF);
    }
#endif

    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_big_font);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_big_font_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    sw = lv_sw_create(btn, NULL);
    lv_sw_set_style(sw, LV_SW_STYLE_BG, bg_style);
    lv_sw_set_style(sw, LV_SW_STYLE_INDIC, indic_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_ON, knob_on_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_OFF, knob_off_style);
    lv_obj_set_size(sw, lv_obj_get_width(sw)*9/17, btn_sw_height*9/17);
    btn->user_data = (void *)sw;
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(sw)*5/4 - 10);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(sw, btn_label, LV_ALIGN_OUT_RIGHT_MID, lv_obj_get_width(sw), 0);
    if(poc_setting_conf->font.big_font_switch == 1)
    {
    	lv_sw_on(sw, LV_ANIM_OFF);
    }
    else
    {
    	lv_sw_off(sw, LV_ANIM_OFF);
    }

#ifdef CONFIG_POC_GUI_KEYPAD_LIGHT_SUPPORT
    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_key_light);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_key_light_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    sw = lv_sw_create(btn, NULL);
    lv_sw_set_style(sw, LV_SW_STYLE_BG, bg_style);
    lv_sw_set_style(sw, LV_SW_STYLE_INDIC, indic_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_ON, knob_on_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_OFF, knob_off_style);
    lv_obj_set_size(sw, lv_obj_get_width(sw)*9/17, btn_sw_height*9/17);
    btn->user_data = (void *)sw;
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(sw)*5/4 - 10);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(sw, btn_label, LV_ALIGN_OUT_RIGHT_MID, lv_obj_get_width(sw), 0);
    if(poc_setting_conf->keypad_led_switch != 0)
    {
    	lv_sw_on(sw, LV_ANIM_OFF);
    }
    else
    {
    	lv_sw_off(sw, LV_ANIM_OFF);
    }
#endif

    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_GPS);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_GPS_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    sw = lv_sw_create(btn, NULL);
    lv_sw_set_style(sw, LV_SW_STYLE_BG, bg_style);
    lv_sw_set_style(sw, LV_SW_STYLE_INDIC, indic_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_ON, knob_on_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_OFF, knob_off_style);
    lv_obj_set_size(sw, lv_obj_get_width(sw)*9/17, btn_sw_height*9/17);
    btn->user_data = (void *)sw;
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(sw)*5/4 - 10);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(sw, btn_label, LV_ALIGN_OUT_RIGHT_MID, lv_obj_get_width(sw), 0);
    if(poc_setting_conf->GPS_switch == 1)
    {
    	lv_sw_on(sw, LV_ANIM_OFF);
    }
    else
    {
    	lv_sw_off(sw, LV_ANIM_OFF);
    }

    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_tourch);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_torch_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    sw = lv_sw_create(btn, NULL);
    lv_sw_set_style(sw, LV_SW_STYLE_BG, bg_style);
    lv_sw_set_style(sw, LV_SW_STYLE_INDIC, indic_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_ON, knob_on_style);
    lv_sw_set_style(sw, LV_SW_STYLE_KNOB_OFF, knob_off_style);
    lv_obj_set_size(sw, lv_obj_get_width(sw)*9/17, btn_sw_height*9/17);
    btn->user_data = (void *)sw;
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(sw)*5/4 - 10);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(sw, btn_label, LV_ALIGN_OUT_RIGHT_MID, lv_obj_get_width(sw), 0);
    if(poc_get_torch_status())
    {
    	lv_sw_on(sw, LV_ANIM_OFF);
    }
    else
    {
    	lv_sw_off(sw, LV_ANIM_OFF);
    }

    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_brightness);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_brightness_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    label = lv_label_create(btn, NULL);
    btn->user_data = (void *)label;
    lv_label_set_text(label, bright_str[poc_setting_conf->screen_brightness]);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(label) - 20);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(label, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_brightness_time);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_brightness_time_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    label = lv_label_create(btn, NULL);
    btn->user_data = (void *)label;
    lv_label_set_text(label, bright_time_str[poc_setting_conf->screen_bright_time]);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(label) - 20);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(label, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_theme_switch);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_theme_switch_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    label = lv_label_create(btn, NULL);
    btn->user_data = (void *)label;
    lv_label_set_text(label, theme_str[poc_setting_conf->theme.type]);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(label) - 20);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(label, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
#endif

#ifdef CONFIG_POC_GUI_CHOICE_SIM_SUPPORT
    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_sim_switch);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_sim_switch_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    label = lv_label_create(btn, NULL);
    btn->user_data = (void *)label;
    lv_label_set_text(label, main_sim_str[poc_setting_conf->main_SIM]);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(label) - 20);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(label, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
#endif

#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
    btn = lv_list_add_btn(list, NULL, lv_poc_setting_btn_text_net_switch);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_setting_pressed_cb);
    lv_poc_setting_items_funcs[storage_index] = lv_poc_setting_net_switch_btn_cb;
    btns[storage_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    label = lv_label_create(btn, NULL);
    btn->user_data = (void *)label;
    lv_label_set_text(label, net_type_str[poc_setting_conf->net_type]);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(label) - 20);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(label, btn_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_label_set_long_mode(btn_label, LV_LABEL_LONG_DOT);
#endif

	lv_list_set_btn_selected(list, btns[setting_selected_item]);

}

static void lv_poc_setting_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
    if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
    {
	    int index = lv_list_get_btn_index((lv_obj_t *)poc_setting_win->display_obj, obj);
	    char * text = (char *)lv_list_get_btn_text((const lv_obj_t *)obj);
#ifdef CONFIG_POC_TTS_SUPPORT
	    poc_broadcast_play_rep((uint8_t *)text, strlen(text), poc_setting_conf->voice_broadcast_switch, false);
#else
	    poc_broadcast_play_rep((uint8_t *)text, strlen(text), true, false);
#endif
	    setting_selected_item = index;
	    lv_poc_setting_item_func_t func = lv_poc_setting_items_funcs[index];
	    func(obj);
    }
}

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	switch(sign)
	{
		case LV_SIGNAL_CONTROL:
		{
			if(param == NULL) return LV_RES_OK;
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				{
					lv_signal_send(activity_list, LV_SIGNAL_PRESSED, NULL);
				}

				case LV_GROUP_KEY_DOWN:

				case LV_GROUP_KEY_UP:
				{
					lv_signal_send(activity_list, LV_SIGNAL_CONTROL, param);
					break;
				}

				case LV_GROUP_KEY_GP:
				{

					break;
				}

				case LV_GROUP_KEY_MB:
				{

					break;
				}

				case LV_GROUP_KEY_VOL_DOWN:
				{
					break;
				}

				case LV_GROUP_KEY_VOL_UP:
				{
					break;
				}

				case LV_GROUP_KEY_POC:
				{
					break;
				}

				case LV_GROUP_KEY_ESC:
				{
					lv_poc_del_activity(poc_setting_activity);
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			if(lv_poc_get_refresh_ui())
			{
				//lv_task_t * task = lv_task_create(poc_setting_update_UI_task, 10, LV_TASK_PRIO_LOWEST, NULL);
				//lv_task_once(task);
				poc_setting_update_UI_task(NULL);
				lv_poc_refresh_ui_next();
			}
			break;
		}

		case LV_SIGNAL_DEFOCUS:
		{
			break;
		}

		default:
		{
			break;
		}
	}
	return LV_RES_OK;
}

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}

void lv_poc_setting_open(void)
{
    static lv_poc_activity_ext_t  activity_main_menu_ext = {ACT_ID_POC_MAIN_SETTING,
															poc_setting_create,
															poc_setting_destory};
	if(poc_setting_activity != NULL)
	{
		return;
	}
	poc_setting_conf = lv_poc_setting_conf_read();
	setting_selected_item = 0;
    poc_setting_activity = lv_poc_create_activity(&activity_main_menu_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_setting_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_setting_activity, design_func);
}

#ifdef __cplusplus
}
#endif
