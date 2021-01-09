
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * volum_list_create(lv_obj_t * parent, lv_area_t display_area);

static void volum_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_volum_press_btn_action(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_poc_win_t * volume_win;

static lv_obj_t * activity_list;

lv_poc_activity_t * poc_volum_activity;

static const char * volum_str[] = {"0","1","2","3","4","5","6","7","8","9","10","11"};



static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    volume_win = lv_poc_win_create(display, "音量", volum_list_create);
    return (lv_obj_t *)volume_win;
}

static void activity_destory(lv_obj_t *obj)
{
	if(volume_win != NULL)
	{
		lv_mem_free(volume_win);
		volume_win = NULL;
	}
	poc_volum_activity = NULL;
}

static void * volum_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, volum_list_config);
    return (void *)activity_list;
}

static void volum_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn;
    lv_obj_t *label;
    lv_obj_t *btn_label;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    poc_setting_conf = lv_poc_setting_conf_read();

    btn = lv_list_add_btn(list, NULL, "媒体音量");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_volum_press_btn_action);
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    label = lv_label_create(btn, NULL);
    btn->user_data = (void *)label;
    lv_label_set_text(label, volum_str[poc_setting_conf->volume]);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(label) - 20);
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(label, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_list_set_btn_selected(list, btn);
}

static void lv_poc_volum_press_btn_action(lv_obj_t * obj, lv_event_t event)
{
	char index = lv_list_get_btn_index((lv_obj_t *)volume_win->display_obj, obj);
	if(LV_EVENT_CLICKED == event)
	{
		switch(index)
		{
			case 0:
			{
				lv_obj_t * label = (lv_obj_t *)obj->user_data;
				poc_setting_conf->volume = (poc_setting_conf->volume + 1) % 12;
				lv_label_set_text(label, volum_str[poc_setting_conf->volume]);
				lv_poc_setting_conf_write();
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
					lv_poc_del_activity(poc_volum_activity);
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


void lv_poc_volum_open(void)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_VOLUM,
											activity_create,
											activity_destory};
	if(poc_volum_activity != NULL)
	{
		return;
	}
    poc_volum_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_volum_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_volum_activity, design_func);
}

typedef struct
{
	uint8_t volum;
	uint8_t count;
	lv_obj_t * slider;
	char * title;
} lv_poc_volum_display_slider_t;

static void prv_lv_poc_volum_display_slider_task(lv_task_t * task)
{
	lv_poc_volum_display_slider_t * info = (lv_poc_volum_display_slider_t *)task->user_data;
	if(info == NULL || info->title == NULL)
	{
		return;
	}

	if(!lv_poc_get_poweron_is_ready() && info->slider != NULL)
	{
		lv_obj_set_hidden(info->slider, true);
		return;
	}

	if(info->slider == NULL)
	{
		info->slider = lv_slider_create(lv_scr_act(), NULL);
		if(info->slider == NULL)
		{
			return;
		}
		lv_disp_t * disp = lv_disp_get_default();
		lv_obj_set_size(info->slider, lv_disp_get_hor_res(disp) / 10 * 7, lv_disp_get_ver_res(disp) / 6);
		lv_obj_align(info->slider, lv_scr_act(), LV_ALIGN_CENTER, 0, -(lv_disp_get_ver_res(disp) / 6));
		lv_slider_set_range(info->slider, 0, 10);
	}

	if(info->count == 0)
	{
		lv_obj_set_hidden(info->slider, true);
		return;
	}
	info->count = info->count - 1;
	lv_obj_set_parent(info->slider, lv_scr_act());
	lv_obj_set_hidden(info->slider, false);
	lv_slider_set_value(info->slider, info->volum, LV_ANIM_OFF);
	//info->title
}

static bool prv_lv_poc_volum_display(POC_MMI_VOICE_TYPE_E type, uint8_t volume)
{
	static char * volum_title[] = {"消息", "提示音", "按键音", "媒体", "通话"};
	static char * title = NULL;
	static POC_MMI_VOICE_TYPE_E old_type = POC_MMI_VOICE_MSG;
	static lv_poc_volum_display_slider_t info = {0, 0, NULL, NULL};
	static lv_task_t * volum_slider_task = NULL;
	if(old_type != type)
	{
		old_type = type;
		title = volum_title[old_type];
	}

	info.title = title;
	info.volum = volume;
	info.count = 4;

	if(volum_slider_task == NULL)
	{
		volum_slider_task = lv_task_create(prv_lv_poc_volum_display_slider_task, 500, LV_TASK_PRIO_HIGHEST, &info);
		if(volum_slider_task == NULL)
		{
			return false;
		}
	}
	return true;
}

bool lv_poc_set_volum(POC_MMI_VOICE_TYPE_E type, uint8_t volume, bool play, bool display)
{
	lv_poc_setting_set_current_volume(type, volume, play);
	if(display)
	{
		return prv_lv_poc_volum_display(type , volume);
	}
	return true;
}


#ifdef __cplusplus
}
#endif

