
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

typedef enum edeg_key_set_item
{
#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
	EDEG_KEY_ITEM_NET_SWITCH,
#endif
	EDEG_KEY_ITEM_DISPLAY,
	EDEG_KEY_ITEM_VOLUM,
	EDEG_KEY_ITEM_LOCATION
} edeg_key_set_item_e;

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * edeg_key_set_list_create(lv_obj_t * parent, lv_area_t display_area);

static void edeg_key_set_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_edeg_key_set_press_btn_action(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_poc_win_t * edeg_key_set_win = NULL;

static lv_obj_t * activity_list = NULL;

static osiMutex_t * mutex = NULL;

#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
static const char * net_type_str[] = {"4G/3G/2G","仅3G/2G"};
#endif

static char is_poc_edeg_key_setting_update_UI_task_running = 0;

static edeg_key_set_item_e edeg_key_set_selected_item = 0;


lv_poc_activity_t * poc_edeg_key_set_activity = NULL;

extern lv_poc_activity_t * main_menu_activity;

extern lv_poc_activity_t * poc_setting_activity;

static void lv_poc_edeg_key_setting_update_UI_task(lv_task_t * task)
{
	if(is_poc_edeg_key_setting_update_UI_task_running == 1)
		return;
	is_poc_edeg_key_setting_update_UI_task_running = 1;
	lv_obj_del(edeg_key_set_win->header);
	lv_obj_del(activity_list);
	if(edeg_key_set_win != NULL)
	{
		lv_mem_free(edeg_key_set_win);
		edeg_key_set_win = NULL;
	}
	if(edeg_key_set_win == NULL)
	{
		edeg_key_set_win = lv_poc_win_create(poc_edeg_key_set_activity->display, "设置", edeg_key_set_list_create);
	}
	is_poc_edeg_key_setting_update_UI_task_running = 0;
}

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    edeg_key_set_win = lv_poc_win_create(display, "设置", edeg_key_set_list_create);
    return (lv_obj_t *)edeg_key_set_win;
}

static void activity_destory(lv_obj_t *obj)
{
	if(edeg_key_set_win != NULL)
	{
		lv_mem_free(edeg_key_set_win);
		edeg_key_set_win = NULL;
	}
	poc_edeg_key_set_activity = NULL;
	edeg_key_set_selected_item = 0;
}

static void * edeg_key_set_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, edeg_key_set_list_config);
    return (void *)activity_list;
}

static void lv_poc_edeg_key_set_press_btn_action(lv_obj_t * obj, lv_event_t event)
{
    edeg_key_set_item_e item = (edeg_key_set_item_e)lv_list_get_btn_index((lv_obj_t *)edeg_key_set_win->display_obj, obj);
    edeg_key_set_selected_item = item;
    if(LV_EVENT_CLICKED == event)
    {
        switch(item)
        {
#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
        	case EDEG_KEY_ITEM_NET_SWITCH:
        	{
        		lv_poc_net_switch_open();
        		break;
        	}
#endif

        	case EDEG_KEY_ITEM_DISPLAY:
        	{
        		lv_poc_display_open();
        		break;
        	}

        	case EDEG_KEY_ITEM_VOLUM:
        	{
        		lv_poc_volum_open();
        		break;
        	}

        	case EDEG_KEY_ITEM_LOCATION:
        	{
        		break;
        	}

        	default:
        	{
        		break;
        	}
        }
    }
}

static void edeg_key_set_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn = NULL;
    __attribute__((unused)) lv_obj_t *label = NULL;
    __attribute__((unused)) lv_obj_t *btn_label = NULL;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    __attribute__((unused)) lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_obj_t * btns[4];
    int item_index = 0;
    poc_setting_conf = lv_poc_setting_conf_read();

#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
    btn = lv_list_add_btn(list, NULL, "网络切换");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_edeg_key_set_press_btn_action);
    btns[item_index++] = btn;
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

    btn = lv_list_add_btn(list, NULL, "显示");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_edeg_key_set_press_btn_action);
    btns[item_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);

    btn = lv_list_add_btn(list, NULL, "音量调节");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_edeg_key_set_press_btn_action);
    btns[item_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);

    btn = lv_list_add_btn(list, NULL, "位置信息");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_edeg_key_set_press_btn_action);
    btns[item_index++] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);

    lv_list_set_btn_selected(list, btns[edeg_key_set_selected_item]);

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
				case LV_KEY_ENTER:

				case LV_KEY_DOWN:

				case LV_KEY_UP:
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

				case LV_KEY_ESC:
				{
					lv_poc_del_activity(poc_edeg_key_set_activity);
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
				case LV_KEY_ENTER:
				{
					break;
				}

				case LV_KEY_ESC:
				{
					break;
				}

				case LV_KEY_DOWN:
				{
					break;
				}

				case LV_KEY_UP:
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
			if(lv_poc_get_refresh_ui() == 1)
			{
				if(main_menu_activity != NULL)
				{
					lv_poc_refresh_ui_next();
				}
				lv_task_t * task = lv_task_create(lv_poc_edeg_key_setting_update_UI_task,
													100,
													LV_TASK_PRIO_MID,
													NULL);
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

void lv_poc_edeg_key_set_open(void)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_EDEG_KEY_SET,
											activity_create,
											activity_destory};
	if(poc_edeg_key_set_activity != NULL || poc_setting_activity != NULL)
	{
		return;
	}

	if(mutex == NULL)
	{
		mutex = osiMutexCreate();
	}

	mutex ? osiMutexLock(mutex) : 0;
    poc_edeg_key_set_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_edeg_key_set_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_edeg_key_set_activity, design_func);
	mutex ? osiMutexUnlock(mutex) : 0;
}

#ifdef __cplusplus
}
#endif
