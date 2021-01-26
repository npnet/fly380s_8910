
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * list_create(lv_obj_t * parent, lv_area_t display_area);

static void list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_theme_press_btn_action(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_poc_win_t * activity_win = NULL;
static lv_obj_t * activity_list = NULL;

lv_poc_activity_t * poc_theme_switch_activity = NULL;

static lv_poc_rb_t * theme_switch_rb = NULL;

static char is_poc_theme_update_UI_task_running = 0;

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
#if 1
    activity_win = lv_poc_win_create(display, "主题切换", list_create);
#endif
    return (lv_obj_t *)activity_win;
}

static void activity_destory(lv_obj_t *obj)
{
	if(activity_win != NULL)
 	{
		lv_mem_free(activity_win);
		activity_win = NULL;
	}

	if(theme_switch_rb != NULL)
	{
		lv_poc_rb_del(theme_switch_rb);
		theme_switch_rb = NULL;
	}
	poc_theme_switch_activity = NULL;
}

static void * list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, list_config);
    return (void *)activity_list;
}

static void list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn = NULL;
    lv_obj_t *cb = NULL;
    lv_obj_t *btn_label = NULL;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_coord_t btn_cb_height = (list_area.y2 - list_area.y1)/3;
    lv_obj_t * btn_array[2];

    theme_switch_rb = lv_poc_rb_create();

    btn = lv_list_add_btn(list, NULL, "白色");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_theme_press_btn_action);
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    cb = lv_cb_create(btn, NULL);
    lv_cb_set_text(cb, "");
    btn->user_data = (void *)cb;
    lv_obj_set_height(cb, btn_cb_height);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(cb));
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(cb, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_poc_rb_add(theme_switch_rb, cb);
    btn_array[0] = btn;

    btn = lv_list_add_btn(list, NULL, "黑色");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_theme_press_btn_action);
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    cb = lv_cb_create(btn, NULL);
    lv_cb_set_text(cb, "");
    btn->user_data = (void *)cb;
    lv_obj_set_height(cb, btn_cb_height);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(cb));
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(cb, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_poc_rb_add(theme_switch_rb, cb);
    btn_array[1] = btn;

    lv_list_set_btn_selected(list, btn_array[poc_setting_conf->theme.type]);
    lv_poc_rb_press(theme_switch_rb, btn_array[poc_setting_conf->theme.type]->user_data);
}

static void poc_theme_update_UI_task(lv_task_t * task)
{
	if(is_poc_theme_update_UI_task_running == 1)
	{
		return;
	}
	is_poc_theme_update_UI_task_running = 1;
	if(activity_win->header != NULL)
	{
		lv_obj_del(activity_win->header);
	}
	if(activity_list != NULL)
	{
		lv_obj_del(activity_list);
	}
	if(activity_win != NULL)
 	{
		lv_mem_free(activity_win);
		activity_win = NULL;
	}
	activity_win = lv_poc_win_create(poc_theme_switch_activity->display, "主题切换", list_create);
	poc_theme_switch_activity->ext_data = (void *)activity_win;
	is_poc_theme_update_UI_task_running = 0;
}

static void lv_poc_theme_press_btn_action(lv_obj_t * obj, lv_event_t event)
{
    if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
    {
	    int index = lv_list_get_btn_index(activity_list, obj);
		if(poc_setting_conf->theme.type != index)
		{
			if(index == 0)
			{
				poc_setting_conf->theme.current_theme = poc_setting_conf->theme.white;
				poc_setting_conf->theme.type = 0;
			}
			else
			{
				poc_setting_conf->theme.current_theme = poc_setting_conf->theme.black;
				poc_setting_conf->theme.type = 1;
			}
			lv_poc_rb_press(theme_switch_rb, obj);
			lv_poc_setting_conf_write();
			lv_task_t * task = lv_task_create(poc_theme_update_UI_task, 10, LV_TASK_PRIO_LOWEST, NULL);
			lv_task_once(task);
			lv_poc_refresh_ui_next();
		}
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
					lv_poc_del_activity(poc_theme_switch_activity);
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


void lv_poc_theme_switch_open(void)
{
#if 1
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_THEME_SWITCH,
															activity_create,
															activity_destory};
    if(poc_theme_switch_activity != NULL)
    {
    	return;
    }
	poc_setting_conf = lv_poc_setting_conf_read();
    poc_theme_switch_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
#endif
	lv_poc_activity_set_signal_cb(poc_theme_switch_activity, signal_func);
	lv_poc_activity_set_design_cb(poc_theme_switch_activity, design_func);
}

#ifdef __cplusplus
}
#endif

#endif
