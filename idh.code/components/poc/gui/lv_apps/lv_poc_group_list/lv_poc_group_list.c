#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include <stdio.h>
#include <string.h>

#define CURRENR_GROUP_NAME_EXTERN 20

static lv_poc_group_list_t * group_list = NULL;

static lv_poc_member_list_t * member_list = NULL;

static lv_obj_t * lv_poc_group_list_activity_create(lv_poc_display_t *display);

static void lv_poc_group_list_activity_destory(lv_obj_t *obj);

static void lv_poc_group_list_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_group_list_press_btn_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t lv_poc_group_list_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool lv_poc_group_list_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static void lv_poc_get_group_list_cb(int result_type);

static void lv_poc_group_list_title_refr(lv_task_t * task);
static void lv_poc_group_lock_oprator_cb(lv_poc_group_oprator_type opt);

static void lv_poc_group_delete_oprator_cb(int result_type);

static void lv_poc_lock_group_question_OK_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_lock_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_unlock_group_question_OK_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_unlock_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_delete_group_question_OK_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_delete_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event);

static lv_obj_t * activity_list;

static lv_poc_group_list_item_info_t * lv_poc_group_list_info = NULL;

static lv_poc_group_list_item_info_t * lv_poc_group_lock_info = NULL;

static lv_poc_group_list_item_info_t * lv_poc_group_delete_info = NULL;

static lv_poc_group_list_item_info_t * lv_poc_group_current_lock_info = NULL;

static lv_poc_group_list_item_info_t * lv_poc_group_current_info = NULL;

lv_poc_group_info_t *lv_poc_group_list_get_member_list_info = NULL;


static lv_area_t display_area;

lv_poc_activity_t * poc_group_list_activity;

char group_member_list_is_open = 0;

static int prv_group_list_cur_opt = 0;


static char lv_poc_group_member_list_title[LIST_ELEMENT_NAME_MAX_LENGTH];

static char lv_poc_group_list_current_group_title[LIST_ELEMENT_NAME_MAX_LENGTH * 2];

static const char * lv_poc_lockgroupwindow_label_lock_text = "锁组";
static const char * lv_poc_lockgroupwindow_label_unlock_text = "解锁";
static const char * lv_poc_lockgroupwindow_label_lock_question_text = "是否锁组？";
static const char * lv_poc_lockgroupwindow_label_unlock_question_text = "是否解锁？";
static const char * lv_poc_lockgroupwindow_label_delete_group_text = "删除";
static const char * lv_poc_lockgroupwindow_label_delete_group_question_text = "确认删除吗？";
static const char * lv_poc_lockgroupwindow_label_OK_text = "          确定          ";

static lv_obj_t * lv_poc_group_list_activity_create(lv_poc_display_t *display)
{
    display_area.x1 = 0;
    display_area.x2 = lv_poc_get_display_width(display);
    display_area.y1 = 0;
    display_area.y2 = lv_poc_get_display_height(display);
    activity_list = lv_poc_list_create(display, NULL, display_area, lv_poc_group_list_list_config);
    return (lv_obj_t *)activity_list;
}

static void lv_poc_group_list_activity_destory(lv_obj_t *obj)
{
	lv_poc_group_list_cb_set_active(ACT_ID_POC_GROUP_LIST, false);
	lv_poc_group_list_get_member_list_info = NULL;
	prv_group_list_cur_opt = 0;
	activity_list = NULL;
	if(group_list != NULL)
	{
		list_element_t * cur_p = group_list->group_list;
		list_element_t * temp_p;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			//lv_obj_del(temp_p->list_item);
			lv_mem_free(temp_p);
		}

		lv_mem_free(group_list);
	}
	group_list = NULL;

	if(lv_poc_group_list_info != NULL)
	{
		lv_mem_free(lv_poc_group_list_info);
	}
	lv_poc_group_list_info = NULL;

	if(member_list != NULL)
	{
		list_element_t * cur_p = member_list->online_list;
		list_element_t * temp_p;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		cur_p = member_list->offline_list;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		lv_mem_free(member_list);
	}
	member_list = NULL;
	poc_group_list_activity = NULL;
}

static void lv_poc_group_list_list_config(lv_obj_t * list, lv_area_t list_area)
{
}

static void lv_poc_group_list_get_membet_list_cb(int msg_type)
{
	extern lv_poc_activity_t * poc_member_list_activity;
	if(poc_member_list_activity != NULL)
    {
    	return;
    }

    if(member_list == NULL)
    {
	    return;
    }

    if(msg_type == 1)
    {
		lv_poc_refr_task_once(lv_poc_group_list_title_refr,
			LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH);
    }
    else
    {
	    poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Member, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"成员列表", (const uint8_t *)"获取失败");
    }
}

static void lv_poc_group_list_set_current_group_cb(int result_type)
{
	if(result_type == 1)
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Join_Group, false);
		if(lv_poc_group_current_info != NULL && activity_list != NULL)
		{
			lv_obj_t *cur_btn = lv_list_get_btn_selected(activity_list);

			lv_poc_group_list_item_info_t *group_info = lv_poc_group_current_info;
			list_element_t * group_item = (list_element_t *)group_info->item_information;
			lv_obj_t *btn_label = lv_list_get_btn_label(group_item->list_item);
			lv_label_set_text(btn_label, " ");
			lv_label_set_text(btn_label, lv_poc_get_group_name((lv_poc_group_info_t)group_item->information));
			lv_img_set_src(group_info->lock_img, &unlock);
			group_info->is_lock = false;

			group_info = (lv_poc_group_list_item_info_t *)cur_btn->user_data;
			group_item = (list_element_t *)group_info->item_information;
			btn_label = lv_list_get_btn_label(group_item->list_item);
	    	strcpy(lv_poc_group_list_current_group_title, (const char *)lv_poc_get_group_name((lv_poc_group_info_t)group_item->information));
	    	strcat(lv_poc_group_list_current_group_title, (const char *)"[当前群组]");
			lv_label_set_text(btn_label, lv_poc_group_list_current_group_title);
			lv_img_set_src(group_info->lock_img, &unlock);
			group_info->is_lock = false;
			lv_poc_group_current_info = group_info;

			if(lv_poc_get_lock_group() != NULL)
			{
				lv_poc_set_lock_group(LV_POC_GROUP_OPRATOR_TYPE_LOCK, (lv_poc_group_info_t)group_item->information, lv_poc_group_lock_oprator_cb);
			}
		}
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"切换群组", (const uint8_t *)"成功");
	}
	else if(result_type == 2)
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"已在群组", NULL);
	}
	else
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"切换群组", (const uint8_t *)"失败");
	}
}

static void lv_poc_group_list_press_btn_cb(lv_obj_t * obj, lv_event_t event)
{
	lv_poc_group_list_item_info_t * p_info = (lv_poc_group_list_item_info_t *)obj->user_data;
	if(p_info == NULL)
	{
		return;
	}
	lv_area_t lock_window_area = {0};

	list_element_t * p_element = (list_element_t *)p_info->item_information;
	if(p_element == NULL)
	{
		return;
	}

	OSI_LOGI(0, "poc_group_list_signal event_cb %d", event);

	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
		lv_poc_group_lock_info = p_info;
		lv_poc_set_current_group((lv_poc_group_info_t)p_element->information, lv_poc_group_list_set_current_group_cb);

		if(member_list == NULL)
		{
			member_list = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));
		}

		if(member_list != NULL)
		{
			memset((void *)member_list, 0, sizeof(lv_poc_member_list_t));
			strcpy(lv_poc_group_member_list_title, (const char *)p_element->name);
			member_list->hide_offline = false;
			lv_poc_group_list_get_member_list_info = (lv_poc_group_info_t *)p_element->information;
			lv_poc_get_member_list((lv_poc_group_info_t)p_element->information,
				member_list,
				1,
				lv_poc_group_list_get_membet_list_cb);
		}
	}
	else if(LV_EVENT_LONG_PRESSED == event)
	{
		if(prv_group_list_cur_opt == 1)
		{
			if(lv_poc_group_list_info == NULL)
			{
				return;
			}

			if(lv_poc_get_lock_group() != NULL)  //解锁组
			{
				lv_poc_group_lock_info = NULL;
				lv_poc_warnning_open(lv_poc_lockgroupwindow_label_unlock_text,
					lv_poc_lockgroupwindow_label_unlock_question_text,
					lv_poc_lockgroupwindow_label_OK_text,
					lv_poc_unlock_group_question_OK_cb,
					NULL,
					lv_poc_unlock_group_question_CANCEL_cb,
					lock_window_area);
			}
			else  //锁组
			{
				lv_poc_group_lock_info = p_info;
				lv_poc_warnning_open(lv_poc_lockgroupwindow_label_lock_text,
					lv_poc_lockgroupwindow_label_lock_question_text,
					lv_poc_lockgroupwindow_label_OK_text,
					lv_poc_lock_group_question_OK_cb,
					NULL,
					lv_poc_lock_group_question_CANCEL_cb,
					lock_window_area);
			}
		}
		else if(prv_group_list_cur_opt == 2)
		{
			lv_poc_group_delete_info = p_info;
			lv_poc_warnning_open(lv_poc_lockgroupwindow_label_delete_group_text,
				lv_poc_lockgroupwindow_label_delete_group_question_text,
				lv_poc_lockgroupwindow_label_OK_text,
				lv_poc_delete_group_question_OK_cb,
				NULL,
				lv_poc_delete_group_question_CANCEL_cb,
				lock_window_area);
		}
		prv_group_list_cur_opt = 0;
	}
}

static lv_res_t lv_poc_group_list_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
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

				case LV_KEY_ESC:
				{
					lv_poc_del_activity(poc_group_list_activity);
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
			if(param == NULL) return LV_RES_OK;
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_MB:
				{
					if(prv_group_list_cur_opt > 0)
					{
						break;
					}
					prv_group_list_cur_opt = 1;
					lv_signal_send(activity_list, LV_SIGNAL_LONG_PRESS, NULL);
					break;
				}

				case LV_GROUP_KEY_GP:
				{
					if(prv_group_list_cur_opt > 0)
					{
						break;
					}
					prv_group_list_cur_opt = 2;
					lv_signal_send(activity_list, LV_SIGNAL_LONG_PRESS, NULL);
					break;
				}

				default:
				{
					break;
				}
			}
		}

		default:
		{
			break;
		}
	}
	return LV_RES_OK;
}

static bool lv_poc_group_list_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}

static void lv_poc_get_group_list_cb(int result_type)
{
	if(poc_group_list_activity == NULL)
	{
		return;
	}

	if(result_type == 1)
	{
		lv_poc_refr_func_ui(lv_poc_group_list_refresh,
			LVPOCLISTIDTCOM_LIST_PERIOD_50,LV_TASK_PRIO_HIGH, NULL);
	}
	else
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Group, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
	}
}

void lv_poc_group_lock_oprator_refresh_task(lv_task_t * task)
{
	lv_poc_group_oprator_type opt = (lv_poc_group_oprator_type)task->user_data;

	if(opt == LV_POC_GROUP_OPRATOR_TYPE_LOCK)
	{
		if(lv_poc_group_current_info != NULL)
		{
			if(lv_poc_group_lock_info == NULL)
			{
				if(activity_list == NULL)
				{
					return;
				}
				lv_obj_t *cur_btn = lv_list_get_btn_selected(activity_list);
				lv_poc_group_current_info = (lv_poc_group_list_item_info_t *)cur_btn->user_data;
				lv_poc_group_lock_info = lv_poc_group_current_info;
			}

			lv_poc_group_list_item_info_t *group_info = lv_poc_group_current_info;
			list_element_t * group_item = (list_element_t *)group_info->item_information;
			lv_obj_t *btn_label = lv_list_get_btn_label(group_item->list_item);
			lv_label_set_text(btn_label, lv_poc_get_group_name((lv_poc_group_info_t)group_item->information));
			lv_img_set_src(group_info->lock_img, &unlock);
			group_info->is_lock = false;

			group_info = lv_poc_group_lock_info;
			lv_poc_group_current_lock_info = lv_poc_group_lock_info;
			lv_poc_group_current_info = lv_poc_group_lock_info;
			lv_poc_group_lock_info = NULL;
			group_item = (list_element_t *)group_info->item_information;
			btn_label = lv_list_get_btn_label(group_item->list_item);
	    	strcpy(lv_poc_group_list_current_group_title, (const char *)lv_poc_get_group_name((lv_poc_group_info_t)group_item->information));
	    	strcat(lv_poc_group_list_current_group_title, (const char *)"[当前群组]");
			lv_label_set_text(btn_label, lv_poc_group_list_current_group_title);
			lv_img_set_src(group_info->lock_img, &locked);
			group_info->is_lock = true;
		}
	}
	else if(opt == LV_POC_GROUP_OPRATOR_TYPE_UNLOCK)
	{
		if(lv_poc_group_current_lock_info == NULL)
		{
			return;
		}
		lv_poc_group_list_item_info_t *group_info = lv_poc_group_current_lock_info;
		lv_poc_group_current_lock_info = NULL;
		lv_img_set_src(group_info->lock_img, &unlock);
		group_info->is_lock = false;
	}
}

static void lv_poc_group_lock_oprator_cb(lv_poc_group_oprator_type opt)
{
	switch(opt)
	{
		case LV_POC_GROUP_OPRATOR_TYPE_LOCK_FAILED:
		{
			OSI_LOGI(0, "lock group fail\n");
			lv_poc_group_lock_info = NULL;
			break;
		}

		case LV_POC_GROUP_OPRATOR_TYPE_LOCK_OK:
		{
			if(lv_poc_group_lock_info == NULL || lv_poc_group_current_info == NULL)
			{
				OSI_LOGI(0, "lock an empty group\n");
				break;
			}
			OSI_LOGI(0, "lock group success\n");
			lv_task_t *fresh_task = lv_task_create(lv_poc_group_lock_oprator_refresh_task, 10, LV_TASK_PRIO_HIGH, (void *)LV_POC_GROUP_OPRATOR_TYPE_LOCK);
			lv_task_once(fresh_task);
			break;
		}

		case LV_POC_GROUP_OPRATOR_TYPE_UNLOCK_FAILED:
		{
			OSI_LOGI(0, "unlock group fail\n");
			break;
		}

		case LV_POC_GROUP_OPRATOR_TYPE_UNLOCK_OK:
		{
			if(lv_poc_group_current_lock_info == NULL)
			{
				OSI_LOGI(0, "unlock an empty group\n");
				break;
			}
			OSI_LOGI(0, "unlock group success\n");
			lv_task_t *fresh_task = lv_task_create(lv_poc_group_lock_oprator_refresh_task, 10, LV_TASK_PRIO_HIGH, (void *)LV_POC_GROUP_OPRATOR_TYPE_UNLOCK);
			lv_task_once(fresh_task);
			break;
		}

		default:
			break;
	}
}

static void lv_poc_group_delete_oprator_cb(int result_type)
{
	if(poc_group_list_activity == NULL)
	{
		return;
	}

	if(result_type == 0)
	{
		if(lv_poc_group_delete_info == NULL)
		{
			return;
		}

		if(group_list != NULL)
		{
			lv_list_clean(activity_list);

			lv_poc_group_list_clear(group_list);

			if(!lv_poc_get_group_list(group_list, lv_poc_get_group_list_cb))
			{
				poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Group, true);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
			}
		}
	}
	else
	{
		OSI_LOGI(0, "delete group fail, cause:%d\n", result_type);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"删除群组失败", NULL);
	}
	lv_poc_group_delete_info = NULL;
}

static void lv_poc_lock_group_question_OK_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_APPLY)
	{
		if(lv_poc_group_lock_info == NULL)
		{
			return;
		}
		list_element_t * item_info = (list_element_t *)lv_poc_group_lock_info->item_information;
		lv_poc_group_info_t * group_info = (lv_poc_group_info_t)item_info->information;
		lv_poc_set_lock_group(LV_POC_GROUP_OPRATOR_TYPE_LOCK, (lv_poc_group_info_t)group_info, lv_poc_group_lock_oprator_cb);
	}
}

static void lv_poc_lock_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_CANCEL)
	{
	}
}

static void lv_poc_unlock_group_question_OK_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_APPLY)
	{
		if(lv_poc_group_current_lock_info == NULL)
		{
			return;
		}
		list_element_t * item_info = (list_element_t *)lv_poc_group_current_lock_info->item_information;
		lv_poc_group_info_t * group_info = (lv_poc_group_info_t)item_info->information;
		lv_poc_set_lock_group(LV_POC_GROUP_OPRATOR_TYPE_UNLOCK, (lv_poc_group_info_t)group_info, lv_poc_group_lock_oprator_cb);
	}
}

static void lv_poc_unlock_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_CANCEL)
	{
	}
}

static void lv_poc_delete_group_question_OK_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_APPLY)
	{
		if(lv_poc_group_delete_info == NULL)
		{
			return;
		}
		list_element_t * item_info = (list_element_t *)lv_poc_group_delete_info->item_information;
		lv_poc_group_info_t * group_info = (lv_poc_group_info_t)item_info->information;
		lv_poc_delete_group((lv_poc_group_info_t)group_info, lv_poc_group_delete_oprator_cb);
	}
}

static void lv_poc_delete_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_CANCEL)
	{
	}
}


void lv_poc_group_list_open(lv_poc_group_list_t *group_list_obj)
{
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_GROUP_LIST,
															lv_poc_group_list_activity_create,
															lv_poc_group_list_activity_destory};

    if(poc_group_list_activity != NULL || group_list != NULL || activity_list != NULL)
    {
    	return;
    }

	group_list = (lv_poc_group_list_t *)lv_mem_alloc(sizeof(lv_poc_group_list_t));

	if(group_list == NULL)
	{
		return;
	}

	group_list->group_list = NULL;
	group_list->group_number = 0;

    if(group_list_obj != NULL)
    {
		group_list->group_number = group_list_obj->group_number;
		list_element_t *p_cur = group_list_obj->group_list;
		list_element_t *p_scr = NULL;
		group_list->group_list = NULL;

		while(p_cur != NULL)
		{
			if(group_list->group_list != NULL)
			{
				p_scr->next = (list_element_t *)lv_mem_alloc(sizeof(list_element_t));
				if(p_scr->next == NULL)
				{
					break;
				}
				p_scr = p_scr->next ;
			}
			else
			{
				p_scr = (list_element_t *)lv_mem_alloc(sizeof(list_element_t));
				if(p_scr == NULL)
				{
					break;
				}
				group_list->group_list = p_scr;
			}
			p_scr->next = NULL;
			p_scr->information = p_cur->information;
			strcpy(p_scr->name, p_cur->name);
			p_cur = p_cur->next;
		}
    }

    poc_group_list_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_group_list_cb_set_active(ACT_ID_POC_GROUP_LIST, true);
    lv_poc_activity_set_signal_cb(poc_group_list_activity, lv_poc_group_list_signal_func);
    lv_poc_activity_set_design_cb(poc_group_list_activity, lv_poc_group_list_design_func);

    if(group_list_obj == NULL)
    {
		if(!lv_poc_get_group_list(group_list, lv_poc_get_group_list_cb))
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Group, true);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
		}
    }
    else
    {
		lv_poc_refr_func_ui(lv_poc_group_list_refresh,
			LVPOCLISTIDTCOM_LIST_PERIOD_50,LV_TASK_PRIO_HIGH, NULL);
    }
}


lv_poc_status_t lv_poc_group_list_add(lv_poc_group_list_t *group_list_obj, const char * name, void * information)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    lv_poc_status_t status = lv_poc_group_list_is_exists(group_list_obj, name, information);
    if(status != POC_GROUP_NONENTITY)
    {
        return status;
    }

    list_element_t * new_element = (list_element_t *)lv_mem_alloc(sizeof(list_element_t));
    if(NULL == new_element)
    {
        return POC_OPERATE_FAILD;
    }
    memset(new_element, 0, sizeof(list_element_t));
    if(strlen(name) < LIST_ELEMENT_NAME_MAX_LENGTH - 1)
    {
        strcat(new_element->name, name);
    }
    else
    {
        memcpy(new_element->name, name, LIST_ELEMENT_NAME_MAX_LENGTH - 1);
    }
    new_element->information = information;

    if(group_list_obj->group_list)
    {
        new_element->next = group_list_obj->group_list;
    }
    else
    {
        new_element->next = NULL;
    }
    group_list_obj->group_list = new_element;
    group_list_obj->group_number = group_list_obj->group_number + 1;

    //lv_poc_group_list_refresh(group_list_obj);
    return POC_OPERATE_SECCESS;
}

void lv_poc_group_list_remove(lv_poc_group_list_t *group_list_obj, const char * name, void * information)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    if(group_list_obj->group_number == 0)
    {
        return;
    }

    p_cur = group_list_obj->group_list;
    if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        group_list_obj->group_list = p_cur->next;
        lv_obj_del(p_cur->list_item);
        lv_mem_free(p_cur);
        group_list_obj->group_number = group_list_obj->group_number - 1;
        return;
    }
    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if((GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL)))
        {
            p_prv->next = p_cur->next;
            lv_obj_del(p_cur->list_item);
            lv_mem_free(p_cur);
            group_list_obj->group_number = group_list_obj->group_number - 1;
            return;
        }

        p_prv = p_cur;
        p_cur = p_cur->next;
    }
}

void lv_poc_group_list_clear(lv_poc_group_list_t *group_list_obj)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return;
	}

	list_element_t * cur_p = group_list_obj->group_list;
	list_element_t * temp_p;
	while(cur_p != NULL)
	{
		temp_p = cur_p;
		cur_p =cur_p->next;
		//lv_obj_del(temp_p->list_item);
		lv_mem_free(temp_p);
	}
	group_list_obj->group_list = NULL;
}

int lv_poc_group_list_get_information(lv_poc_group_list_t *group_list_obj, const char * name, void *** information)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return 0;
	}

    list_element_t * p_cur = group_list_obj->group_list;
    unsigned char number = 0;
    while(p_cur)
    {
        if(0 == strcmp(p_cur->name, name))
        {
            number = number + 1;

            if(number == 1)
            {
                *information = &(p_cur->information);
            }
            else
            {
                *information = (void **)lv_mem_realloc(*information, sizeof(void *) * number);
                (*information)[number - 1] = &(p_cur->information);
            }
        }
        p_cur = p_cur->next;
    }
    return number;

}

void lv_poc_group_list_refresh(lv_task_t * task)
{
	lv_poc_group_list_t *group_list_obj = NULL;

	group_list_obj = (lv_poc_group_list_t *)task->user_data;

	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return;
	}

	if(current_activity != poc_group_list_activity)
	{
		return;
	}

	int current_index = -1;
	int list_btn_count = -1;
	lv_obj_t *current_btn = lv_list_get_btn_selected(activity_list);
	if(current_btn != NULL)
	{
		current_index = lv_list_get_btn_index(activity_list, current_btn);
	}

    list_element_t * p_cur = NULL;
    lv_obj_t * btn;
    lv_obj_t * img;
	lv_obj_t * btn_label = NULL;
    lv_coord_t btn_height = (display_area.y2 - display_area.y1)/(LV_POC_LIST_COLUM_COUNT + 1);
	lv_coord_t btn_width = (display_area.x2 - display_area.x1);

    char is_first_item = 1;
    char is_set_current_group = 1;
    char is_set_lock_group = 1;

    lv_list_clean(activity_list);

    if(group_list_obj->group_list == NULL)
    {
	    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无成员列表", NULL);
	    return;
    }

    lv_poc_group_info_t current_group = lv_poc_get_current_group();
    char * current_group_name = lv_poc_get_group_name(current_group);
    lv_poc_group_info_t lock_group = lv_poc_get_lock_group();
    char * lock_group_name = lv_poc_get_group_name(lock_group);

    if(lock_group == NULL)
    {
	    is_set_lock_group = 0;
    }
    lv_poc_group_current_lock_info = NULL;

    if(lv_poc_group_list_info != NULL)
    {
	    lv_mem_free(lv_poc_group_list_info);
	    lv_poc_group_list_info = NULL;
    }

    lv_poc_group_list_info = (lv_poc_group_list_item_info_t *)lv_mem_alloc(sizeof(lv_poc_group_list_item_info_t) * group_list_obj->group_number);
    if(lv_poc_group_list_info == NULL)
    {
	    return;
    }
    memset(lv_poc_group_list_info, 0, sizeof(lv_poc_group_list_item_info_t) * group_list_obj->group_number);
    lv_poc_group_list_item_info_t * p_group_info = lv_poc_group_list_info;

    p_cur = group_list_obj->group_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_group, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_group_list_press_btn_cb);
        p_cur->list_item = btn;
        p_group_info->item_information = p_cur;
        btn->user_data = (lv_obj_user_data_t)p_group_info;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        list_btn_count++;
        if(is_first_item == 1)
	    {
		    if(current_index != -1)
		    {
			    if(current_index == list_btn_count)
			    {
		        	is_first_item = 0;
		        	lv_list_set_btn_selected(activity_list, btn);
			    }
		    }
		    else
		    {
	        	is_first_item = 0;
	        	lv_list_set_btn_selected(activity_list, btn);
        	}
        }

		btn_label = lv_list_get_btn_label(btn);
        if(is_set_current_group == 1
	        && GROUP_EQUATION((void *)p_cur->name, (void *)current_group_name, (void *)p_cur->information, (void *)current_group, NULL))
        {
	        is_set_current_group = 0;
        	strcpy(lv_poc_group_list_current_group_title, (const char *)p_cur->name);
        	strcat(lv_poc_group_list_current_group_title, (const char *)"[当前群组]");
        	lv_label_set_text(btn_label, (const char *)lv_poc_group_list_current_group_title);
        	lv_poc_group_current_info = p_group_info;
		}

		img = lv_img_create(btn, NULL);
		p_group_info->lock_img = img;
        if(is_set_lock_group == 1
	        && GROUP_EQUATION((void *)p_cur->name, (void *)lock_group_name, (void *)p_cur->information, (void *)lock_group, NULL))
        {
	        is_set_lock_group = 0;
			lv_img_set_src(img, &locked);//unlock
			p_group_info->is_lock = true;
			lv_poc_group_current_lock_info = p_group_info;
		}
		else
		{
			lv_img_set_src(img, &unlock);//unlock
		}

		lv_img_set_auto_size(img, false);
		lv_obj_set_width(btn_label, btn_width - (lv_coord_t)unlock.header.w - (lv_coord_t)ic_group.header.w - 15);//-15是减去间隙(两个图标)
		lv_obj_align(img, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

        p_cur = p_cur->next;
        p_group_info++;
    }
}

void lv_poc_group_list_refresh_with_data(lv_poc_group_list_t *group_list_obj)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return;
	}
	lv_list_clean(activity_list);
	lv_poc_group_list_clear(group_list_obj);

	lv_poc_get_group_list(group_list, lv_poc_get_group_list_cb);
}


lv_poc_status_t lv_poc_group_list_move_top(lv_poc_group_list_t *group_list_obj, const char * name, void * information)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    lv_poc_status_t status = lv_poc_group_list_is_exists(group_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_GROUP_NONENTITY)
    {
        return status;
    }

    p_cur = group_list_obj->group_list;
    if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            p_prv->next = p_cur->next;
            p_cur->next = group_list_obj->group_list;
            group_list_obj->group_list = p_cur;
            return POC_OPERATE_SECCESS;
        }
        p_prv = p_cur;
        p_cur = p_cur->next;
    }

    return POC_UNKNOWN_FAULT;
}

lv_poc_status_t lv_poc_group_list_move_bottom(lv_poc_group_list_t *group_list_obj, const char * name, void * information)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_scr;
    bool is_find = false;
    lv_poc_status_t status = lv_poc_group_list_is_exists(group_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_GROUP_NONENTITY)
    {
        return status;
    }

    p_cur = group_list_obj->group_list;
    p_scr = p_cur;
    if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        is_find = true;
        p_scr = p_cur;
        group_list_obj->group_list = p_cur->next;
    }
    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(false == is_find && (GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL)))
        {
            is_find = true;
            if(NULL == p_cur->next)
            {
                return POC_OPERATE_SECCESS;
            }
            p_scr = p_cur;
            p_prv->next = p_cur->next;
        }
        p_prv = p_cur;
        p_cur = p_cur->next;
    }

    if(true == is_find)
    {
        p_scr->next = NULL;
        p_prv->next = p_scr;
        return POC_OPERATE_SECCESS;
    }

    return POC_UNKNOWN_FAULT;
}

lv_poc_status_t lv_poc_group_list_move_up(lv_poc_group_list_t *group_list_obj, const char * name, void * information)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_prv_prv;
    lv_poc_status_t status = lv_poc_group_list_is_exists(group_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_GROUP_NONENTITY)
    {
        return status;
    }

    p_cur = group_list_obj->group_list;
    if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        p_prv->next = p_cur->next;
        p_cur->next = p_prv;
        group_list_obj->group_list = p_cur;
        return POC_OPERATE_SECCESS;
    }

    p_prv_prv = p_prv;
    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            p_prv->next = p_cur->next;
            p_cur->next = p_prv;
            p_prv_prv->next = p_cur;
            return POC_OPERATE_SECCESS;
        }
        p_prv_prv = p_prv;
        p_prv = p_cur;
        p_cur = p_cur->next;
    }

    return POC_UNKNOWN_FAULT;
}

lv_poc_status_t lv_poc_group_list_move_down(lv_poc_group_list_t *group_list_obj, const char * name, void * information)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_prv_prv;
    lv_poc_status_t status = lv_poc_group_list_is_exists(group_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_GROUP_NONENTITY)
    {
        return status;
    }

    p_cur = group_list_obj->group_list;
    if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        if(NULL != p_cur->next)
        {
            p_cur = p_cur->next;
            group_list_obj->group_list->next = p_cur->next;
            p_cur->next = group_list_obj->group_list;
            group_list_obj->group_list = p_cur;
        }
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            if(NULL != p_cur->next)
            {
                p_prv_prv = p_prv;
                p_prv = p_cur;
                p_cur = p_cur->next;
                p_prv->next = p_cur->next;
                p_cur->next = p_prv;
                p_prv_prv->next = p_cur;
            }
            return POC_OPERATE_SECCESS;
        }
        p_prv = p_cur;
        p_cur = p_cur->next;
    }

    return POC_UNKNOWN_FAULT;
}

//void lv_poc_group_list_set_state(const char * name, bool is_online);

lv_poc_status_t lv_poc_group_list_is_exists(lv_poc_group_list_t *group_list_obj, const char * name, void * information)
{
	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    if(NULL == name)
    {
        return POC_OPERATE_FAILD;
    }

    p_cur = group_list_obj->group_list;
    while(p_cur)
    {
        if(GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            return POC_GROUP_EXISTS;
        }
        p_cur = p_cur->next;
    }

    return POC_GROUP_NONENTITY;
}

lv_poc_status_t lv_poc_group_list_lock_group(lv_poc_group_list_t *group_list_obj, lv_poc_group_oprator_type opt)
{
	lv_task_t *fresh_task = lv_task_create(lv_poc_group_lock_oprator_refresh_task, 10, LV_TASK_PRIO_HIGH, (void *)opt);
	lv_task_once(fresh_task);

    return POC_GROUP_NONENTITY;
}

/*
	  name : lv_poc_build_group_list_refr
	 param : none
	author : wangls
  describe : 刷新新建群组列表信息
	  date : 2020-07-02
*/
static
void lv_poc_group_list_title_refr(lv_task_t * task)
{
	lv_poc_member_list_open(lv_poc_group_member_list_title,
		member_list,
		member_list->hide_offline);
}

#ifdef __cplusplus
}
#endif
