
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

typedef enum{
	poc_display_edeg_big_font,
	poc_display_edeg_bright,
	poc_display_edeg_bright_time,
#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
	poc_display_edeg_theme_switch,
#endif
	poc_display_edeg_item_max_num
} poc_display_edeg_item_t;

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * display_list_create(lv_obj_t * parent, lv_area_t display_area);

static void display_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_display_press_btn_action(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_poc_win_t * display_win = NULL;

static lv_obj_t * activity_list = NULL;

static poc_display_edeg_item_t display_selected_item = poc_display_edeg_big_font;

static char is_poc_display_update_UI_task_running = 0;

static const char * bright_str[POC_MAX_BRIGHT] = {"0","1","2","3","4","5"};
static const char * bright_time_str[] = {"5秒","15秒","30秒","1分钟","2分钟","5分钟","10分钟","30分钟"};
#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
static const char * theme_str[] = {"白色","黑色"};
#endif

lv_poc_activity_t * poc_display_activity = NULL;



static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    display_win = lv_poc_win_create(display, "显示", display_list_create);
    return (lv_obj_t *)display_win;
}

static void activity_destory(lv_obj_t *obj)
{
	poc_display_activity = NULL;
	display_selected_item = poc_display_edeg_big_font;
}

static void * display_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, display_list_config);
    return (void *)activity_list;
}

static void display_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn = NULL;
    lv_obj_t *sw = NULL;
    lv_obj_t *label = NULL;
    lv_obj_t *btn_label = NULL;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_coord_t btn_sw_height = (list_area.y2 - list_area.y1)/3;
    lv_obj_t * btns[poc_display_edeg_item_max_num];
    /*Create styles for the switch*/
    lv_style_t * bg_style = NULL;
    lv_style_t * indic_style = NULL;
    lv_style_t * knob_on_style = NULL;
    lv_style_t * knob_off_style = NULL;
   	poc_setting_conf = lv_poc_setting_conf_read();
	bg_style = (lv_style_t *)poc_setting_conf->theme.current_theme->style_switch_bg;
	indic_style = (lv_style_t *)poc_setting_conf->theme.current_theme->style_switch_indic;
	knob_on_style = (lv_style_t *)poc_setting_conf->theme.current_theme->style_switch_knob_off;
	knob_off_style = (lv_style_t *)poc_setting_conf->theme.current_theme->style_switch_knob_on;
	lv_list_clean(list);

    btn = lv_list_add_btn(list, NULL, "大号字体");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_display_press_btn_action);
    btns[poc_display_edeg_big_font] = btn;
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

    btn = lv_list_add_btn(list, NULL, "亮度");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_display_press_btn_action);
    btns[poc_display_edeg_bright] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    label = lv_label_create(btn, NULL);
    btn->user_data = (void *)label;
    lv_label_set_text(label, bright_str[poc_setting_conf->screen_brightness]);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(label) - 20);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(label, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    btn = lv_list_add_btn(list, NULL, "亮屏时间");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_display_press_btn_action);
    btns[poc_display_edeg_bright_time] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    label = lv_label_create(btn, NULL);
    btn->user_data= (void *)label;
    lv_label_set_text(label, bright_time_str[poc_setting_conf->screen_bright_time]);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(label) - 20);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(label, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
    btn = lv_list_add_btn(list, NULL, "主题切换");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_display_press_btn_action);
    btns[poc_display_edeg_theme_switch] = btn;
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

    lv_list_set_btn_selected(list, btns[display_selected_item]);

}


static void poc_display_update_UI_task(lv_task_t * task)
{
	if(is_poc_display_update_UI_task_running == 1)
		return;
	is_poc_display_update_UI_task_running = 1;
	lv_obj_del(display_win->header);
	lv_obj_del(activity_list);
	lv_mem_free(display_win);
	display_win = lv_poc_win_create(poc_display_activity->display, "显示", display_list_create);
	poc_display_activity->ext_data = (void *)display_win;
	is_poc_display_update_UI_task_running = 0;
}

static void lv_poc_display_press_btn_action(lv_obj_t * obj, lv_event_t event)
{
    display_selected_item = (poc_display_edeg_item_t)lv_list_get_btn_index((lv_obj_t *)display_win->display_obj, obj);
    lv_obj_t * ext_obj = NULL;
    if(LV_EVENT_CLICKED == event)
    {
        switch(display_selected_item)
        {
    		case poc_display_edeg_big_font:
    		{
    			ext_obj = (lv_obj_t *)obj->user_data;
    			if(poc_setting_conf->font.big_font_switch == 1)
    			{
    				lv_sw_off(ext_obj, LV_ANIM_OFF);
    				poc_setting_conf->font.big_font_switch = 0;
    				poc_setting_conf->font.list_page_colum_count = 4;
    				poc_setting_conf->font.list_btn_current_font = poc_setting_conf->font.list_btn_small_font;
    			}
    			else
    			{
    				lv_sw_on(ext_obj, LV_ANIM_OFF);
    				poc_setting_conf->font.big_font_switch = 1;
    				poc_setting_conf->font.list_page_colum_count = 3;
    				poc_setting_conf->font.list_btn_current_font = poc_setting_conf->font.list_btn_big_font;
    			}

    			lv_poc_setting_conf_write();
    			lv_poc_refresh_ui_next();
    			lv_task_t * task = lv_task_create(poc_display_update_UI_task, 100, LV_TASK_PRIO_LOWEST, NULL);
    			lv_task_once(task);

    			break;
    		}

    		case poc_display_edeg_bright:
    		{
                //char *ext_str;
                int cur_bright;
                ext_obj = (lv_obj_t *)obj->user_data;
                cur_bright = poc_setting_conf->screen_brightness;
                cur_bright = (cur_bright + 1) % POC_MAX_BRIGHT;
                poc_setting_conf->screen_brightness = cur_bright;
    		    lv_poc_setting_conf_write();
                lv_label_set_text(ext_obj, bright_str[cur_bright]);
                //lcd_backlight(cur_bright);
    			break;
    		}

    		case poc_display_edeg_bright_time:
    		{
                lv_poc_bright_time_open();
    			break;
    		}
#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
    		case poc_display_edeg_theme_switch:
    		{
                lv_poc_theme_switch_open();
    			break;
    		}
#endif

    		default:
    		{
    			break;
    		}
        }
    }
}

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	switch(sign)
	{
		case LV_SIGNAL_PRESSED:
		{
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:

				case LV_GROUP_KEY_DOWN:

				case LV_GROUP_KEY_UP:
				{
					activity_list->signal_cb(activity_list, LV_SIGNAL_CONTROL, param);
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
					lv_poc_del_activity(poc_display_activity);
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_LONG_PRESS:
		{
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				{
					break;
				}

				case LV_GROUP_KEY_ESC:
				{
					break;
				}

				case LV_GROUP_KEY_DOWN:
				{
					break;
				}

				case LV_GROUP_KEY_UP:
				{
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
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			if(lv_poc_get_refresh_ui())
			{
				lv_task_t * task = lv_task_create(poc_display_update_UI_task, 100, LV_TASK_PRIO_LOWEST, NULL);
				lv_task_once(task);
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


void lv_poc_display_open(void)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_DISPLAY,
											activity_create,
											activity_destory};
	if(poc_display_activity != NULL)
	{
		return;
	}
    poc_display_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_display_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_display_activity, design_func);
}



#ifdef __cplusplus
}
#endif

