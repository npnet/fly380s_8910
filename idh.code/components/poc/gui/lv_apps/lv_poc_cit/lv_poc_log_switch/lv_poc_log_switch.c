#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

#define LV_POC_LOGSWITCH_ITEMS_NUM (3)

typedef void (* lv_poc_logswitch_item_func_t)(lv_obj_t * obj);

static lv_poc_win_t * poc_logswitch_win = NULL;

static lv_poc_logswitch_item_func_t lv_poc_logswitch_items_funcs[LV_POC_LOGSWITCH_ITEMS_NUM] = {0};

static lv_obj_t * poc_logswitch_create(lv_poc_display_t *display);

static void poc_logswitch_destory(lv_obj_t *obj);

static void * poc_logswitch_list_create(lv_obj_t * parent, lv_area_t display_area);

static void poc_logswitch_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_logswitch_platform_btn_cb(lv_obj_t * obj);

static void lv_poc_logswitch_modem_btn_cb(lv_obj_t * obj);

static void lv_poc_logswitch_pressed_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_obj_t *activity_list = NULL;

lv_poc_activity_t *poc_logswitch_activity = NULL;

struct lv_poc_cit_logswitch_t
{
	long plognum;
	long modemlognum;
};

struct lv_poc_cit_logswitch_t poc_logswitch_attr = {0};

typedef struct
{
    char *title;
    lv_align_t title_align;
	lv_coord_t title_x_mod;
	lv_coord_t title_y_mod;
	char *content;
	lv_align_t content_align;
	lv_coord_t content_x_mod;
	lv_coord_t content_y_mod;
	void(*cb)(bool status);
} lv_poc_logswitch_label_struct_t;

lv_poc_logswitch_label_struct_t lv_poc_logswitch_label_array[] = {
    {
        "plog开关"   , LV_ALIGN_IN_LEFT_MID, 0, 0,
		""   , LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_type_plog_switch_cb,
    },

    {
        "modem开关"   , LV_ALIGN_IN_LEFT_MID, 0, 0,
		""   , LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_type_modemlog_switch_cb,
    },
};


static lv_obj_t * poc_logswitch_create(lv_poc_display_t *display)
{
    poc_logswitch_win = lv_poc_win_create(display, "log开关", poc_logswitch_list_create);
	lv_poc_notation_refresh();
    return (lv_obj_t  *)poc_logswitch_win;
}

static void poc_logswitch_destory(lv_obj_t *obj)
{
	poc_logswitch_activity = NULL;
	memset(&poc_logswitch_attr, 0, sizeof(struct lv_poc_cit_logswitch_t));
}

static void * poc_logswitch_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, poc_logswitch_list_config);
    return (void *)activity_list;
}

static void poc_logswitch_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn = NULL;
    lv_obj_t *sw = NULL;
    lv_obj_t *btn_label = NULL;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_coord_t btn_sw_height = (list_area.y2 - list_area.y1)/3;

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


    btn = lv_list_add_btn(list, NULL, lv_poc_logswitch_label_array[0].title);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_logswitch_pressed_cb);
    lv_poc_logswitch_items_funcs[0] = lv_poc_logswitch_platform_btn_cb;
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
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(sw)*5/4 - 5);
    lv_obj_align(btn_label, btn, lv_poc_logswitch_label_array[0].title_align, lv_poc_logswitch_label_array[0].title_x_mod, lv_poc_logswitch_label_array[0].title_y_mod);
    lv_obj_align(sw, btn_label, lv_poc_logswitch_label_array[0].content_align, lv_poc_logswitch_label_array[0].content_x_mod, lv_poc_logswitch_label_array[0].content_y_mod);
    lv_sw_off(sw, LV_ANIM_OFF);
    lv_list_set_btn_selected(list, btn);

    btn = lv_list_add_btn(list, NULL, lv_poc_logswitch_label_array[1].title);
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_logswitch_pressed_cb);
    lv_poc_logswitch_items_funcs[1] = lv_poc_logswitch_modem_btn_cb;
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
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(sw)*5/4 - 5);
    lv_obj_align(btn_label, btn, lv_poc_logswitch_label_array[1].title_align, lv_poc_logswitch_label_array[1].title_x_mod, lv_poc_logswitch_label_array[1].title_y_mod);
    lv_obj_align(sw, btn_label, lv_poc_logswitch_label_array[1].content_align, lv_poc_logswitch_label_array[1].content_x_mod, lv_poc_logswitch_label_array[1].content_y_mod);
    lv_sw_off(sw, LV_ANIM_OFF);
}

static void lv_poc_logswitch_platform_btn_cb(lv_obj_t * obj)
{
	lv_obj_t * ext_obj = NULL;
	ext_obj = (lv_obj_t *)obj->user_data;
	poc_logswitch_attr.plognum++;
	if(poc_logswitch_attr.plognum%2 == 0)
	{
		lv_sw_off(ext_obj, LV_ANIM_OFF);
		lv_poc_logswitch_label_array[0].cb(false);
	}
	else
	{
		lv_sw_on(ext_obj, LV_ANIM_OFF);
		lv_poc_logswitch_label_array[0].cb(true);
	}
}

static void lv_poc_logswitch_modem_btn_cb(lv_obj_t * obj)
{
	lv_obj_t * ext_obj = NULL;
	ext_obj = (lv_obj_t *)obj->user_data;
	poc_logswitch_attr.modemlognum++;
	if(poc_logswitch_attr.modemlognum%2 == 0)
	{
		lv_sw_off(ext_obj, LV_ANIM_OFF);
		lv_poc_logswitch_label_array[1].cb(false);
	}
	else
	{
		lv_sw_on(ext_obj, LV_ANIM_OFF);
		lv_poc_logswitch_label_array[1].cb(true);
	}
}

static void lv_poc_logswitch_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
	int index = 0;
    if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
    {
	    index = lv_list_get_btn_index((lv_obj_t *)poc_logswitch_win->display_obj, obj);
	    lv_poc_logswitch_item_func_t func = lv_poc_logswitch_items_funcs[index];
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
					lv_poc_del_activity(poc_logswitch_activity);
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			break;
		}

		case LV_SIGNAL_DEFOCUS:
		{
			break;
		}

		case LV_SIGNAL_LONG_PRESS_REP:
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

void lv_poc_logswitch_open(void)
{
    static lv_poc_activity_ext_t  activity_main_menu_ext = {ACT_ID_POC_CIT_LOG_SWITCH,
															poc_logswitch_create,
															poc_logswitch_destory};
	if(poc_logswitch_activity != NULL)
	{
		return;
	}
    poc_logswitch_activity = lv_poc_create_activity(&activity_main_menu_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_logswitch_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_logswitch_activity, design_func);
}

#ifdef __cplusplus
}
#endif

