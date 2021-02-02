
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include "stdlib.h"

#define MAIN_MENU_TITLE ("主菜单")

static lv_obj_t * lv_poc_main_menu_activity_ext_create_cb(lv_poc_display_t *display);

static void lv_poc_main_menu_activity_ext_destory_cb(lv_obj_t *obj);

static void * lv_poc_main_menu_list_create(lv_obj_t * parent, lv_area_t display_area);

static void lv_poc_main_menu_press_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t lv_poc_main_menu_signal_cb(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool lv_poc_main_menu_design_cb(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_obj_t * main_menu_list = NULL;

static lv_poc_win_t * main_menu_win = NULL;

static osiMutex_t * mutex = NULL;

static char is_poc_main_menu_update_UI_task_running = 0;

lv_poc_oem_member_list * poc_member_list = NULL;

lv_poc_oem_group_list  * poc_group_list = NULL;

lv_poc_activity_t * main_menu_activity = NULL;

static int main_menu_selecte_item_index = 0;

static const int8_t lv_poc_main_menu_item_text_member_list[] = "成员列表";

static const int8_t lv_poc_main_menu_item_text_group_list[] = "群组";

static const int8_t lv_poc_main_menu_item_text_new_tempgrp[] = "新建临时组";

static const int8_t lv_poc_main_menu_item_text_setting[] = "设置";

static const int8_t lv_poc_main_menu_item_text_about[] = "关于本机";

static lv_obj_t * lv_poc_main_menu_activity_ext_create_cb(lv_poc_display_t *display)
{
    main_menu_win = lv_poc_win_create(display, MAIN_MENU_TITLE, lv_poc_main_menu_list_create);
    return (lv_obj_t *)main_menu_win;
}

static void lv_poc_main_menu_activity_ext_destory_cb(lv_obj_t *obj)
{
    main_menu_selecte_item_index = 0;
    main_menu_activity = NULL;
	if(main_menu_win != NULL)
	{
		lv_mem_free(main_menu_win);
		main_menu_win = NULL;
	}
}

static void lv_poc_main_menu_press_cb(lv_obj_t * obj, lv_event_t event)
{
	const char * text =  lv_list_get_btn_text(obj);
	if(text == NULL)
	{
		return;
	}
#ifdef CONFIG_POC_TTS_SUPPORT
	poc_broadcast_play_rep((uint8_t *)text, strlen(text), poc_setting_conf->voice_broadcast_switch, false);
#else
	poc_broadcast_play_rep((uint8_t *)text, strlen(text), true, false);
#endif
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
		if(0 == strcmp(text, (const char *)lv_poc_main_menu_item_text_member_list))
		{
			lv_poc_member_list_open(NULL, NULL, false);
		}
		else if(0 == strcmp(text, (const char *)lv_poc_main_menu_item_text_group_list))
		{
			lv_poc_group_list_open(NULL);
		}
		else if(0 == strcmp(text, (const char *)lv_poc_main_menu_item_text_new_tempgrp))
		{
			lv_poc_build_tempgrp_open();
		}
		else if(0 == strcmp(text, (const char *)lv_poc_main_menu_item_text_setting))
		{
			lv_poc_setting_open();
		}
		else if(0 == strcmp(text, (const char *)lv_poc_main_menu_item_text_about))
		{
			lv_poc_about_open();
		}
	}
}

static void * lv_poc_main_menu_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    main_menu_list = lv_poc_list_create(parent, NULL, display_area, lv_poc_list_config);
	lv_poc_notation_refresh();//把弹框显示在最顶层
	return (void *)main_menu_list;
}

static void  lv_poc_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn = NULL;
    lv_coord_t btn_height = (int16_t)abs(list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_obj_t * btns[5];

	if(list == NULL)
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"空窗口列表", NULL);
		return;
	}
    lv_list_clean(list);

    btn = lv_list_add_btn(list, &ic_menu_friend, (const char *)lv_poc_main_menu_item_text_member_list);
    lv_obj_set_event_cb(btn, lv_poc_main_menu_press_cb);
    lv_obj_set_click(btn, true);
    btns[0] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);

    btn = lv_list_add_btn(list, &ic_group, (const char *)lv_poc_main_menu_item_text_group_list);
    lv_obj_set_event_cb(btn, lv_poc_main_menu_press_cb);
    lv_obj_set_click(btn, true);
    btns[1] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);

    btn = lv_list_add_btn(list, &ic_group, (const char *)lv_poc_main_menu_item_text_new_tempgrp);
    lv_obj_set_event_cb(btn, lv_poc_main_menu_press_cb);
    lv_obj_set_click(btn, true);
    btns[2] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);

    btn = lv_list_add_btn(list, &ic_setting, (const char *)lv_poc_main_menu_item_text_setting);
    lv_obj_set_event_cb(btn, lv_poc_main_menu_press_cb);
    lv_obj_set_click(btn, true);
    btns[2] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);

    btn = lv_list_add_btn(list, &ic_menu_about, (const char *)lv_poc_main_menu_item_text_about);
    lv_obj_set_event_cb(btn, lv_poc_main_menu_press_cb);
    lv_obj_set_click(btn, true);
    btns[3] = btn;
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);

    lv_list_set_btn_selected(list, btns[main_menu_selecte_item_index]);
}

static void poc_main_menu_update_UI_task(lv_task_t * task)
{
	if(is_poc_main_menu_update_UI_task_running == 1)
	{
		return;
	}
	is_poc_main_menu_update_UI_task_running = 1;
	if(main_menu_win->header != NULL)
	{
		lv_obj_del(main_menu_win->header);
	}
	if(main_menu_list != NULL)
	{
		lv_obj_del(main_menu_list);
	}
	if(main_menu_win != NULL)
	{
		lv_mem_free(main_menu_win);
		main_menu_win = NULL;
	}
	if(main_menu_win == NULL)
	{
		main_menu_win = lv_poc_win_create(main_menu_activity->display, MAIN_MENU_TITLE, lv_poc_main_menu_list_create);
	}
	is_poc_main_menu_update_UI_task_running = 0;
}

static lv_res_t lv_poc_main_menu_signal_cb(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	switch(sign)
	{
		case LV_SIGNAL_CONTROL:
		{
			if(NULL == param) return LV_RES_OK;
			switch((*(uint32_t *)param))
			{
				case LV_GROUP_KEY_ENTER:
				{
					lv_signal_send(main_menu_list, LV_SIGNAL_PRESSED, NULL);
					break;
				}

				case LV_GROUP_KEY_DOWN:

				case LV_GROUP_KEY_UP:
				{
					lv_signal_send(main_menu_list, LV_SIGNAL_CONTROL, param);
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

				case LV_GROUP_KEY_ESC:
				{
					lv_poc_del_activity(main_menu_activity);
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			if(lv_poc_get_refresh_ui())
			{
				poc_main_menu_update_UI_task(NULL);
			}
			break;
		}

		default:
		{
			break;
		}
	}
	return LV_RES_OK;
}

static bool lv_poc_main_menu_design_cb(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}

void lv_poc_main_menu_open(void)
{
    static lv_poc_activity_ext_t  activity_main_menu_ext = {ACT_ID_POC_MAIN_MENU,
															lv_poc_main_menu_activity_ext_create_cb,
															lv_poc_main_menu_activity_ext_destory_cb};
	main_menu_selecte_item_index = 0;
	if(main_menu_activity != NULL)
	{
		return;
	}

	if(mutex == NULL)
	{
		mutex = osiMutexCreate();
	}

	mutex ? osiMutexLock(mutex) : 0;
    main_menu_activity = lv_poc_create_activity(&activity_main_menu_ext, true, false, NULL);
   	lv_poc_activity_set_signal_cb(main_menu_activity, lv_poc_main_menu_signal_cb);
   	lv_poc_activity_set_design_cb(main_menu_activity, lv_poc_main_menu_design_cb);
	mutex ? osiMutexUnlock(mutex) : 0;
}

#ifdef __cplusplus
}
#endif

