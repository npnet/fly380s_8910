#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include <stdio.h>
#include <string.h>

#define CURRENR_GROUP_NAME_EXTERN 20
#define GUIIDTCOM_IDTGROUPLOCKINFO_DEBUG_LOG 1

enum
{
	POC_REFRESH_TYPE_START,

	POC_REFRESH_TYPE_GRP_TITLE,
	POC_REFRESH_TYPE_SET_CUR_INFO,
	POC_REFRESH_TYPE_LOCK_GRP,
	POC_REFRESH_TYPE_GROUP_LIST,
};

typedef struct
{
	osiMutex_t *mutex;
 	lv_task_t *task;
 	uint8_t grp_refresh_type;
 	void *user_data;
	uint8_t lock_type;
	uint8_t grp_refresh_queue[10];
	uint8_t write_index;
	uint8_t read_index;
}lv_poc_grp_refresh_t;

static lv_poc_group_list_t * group_list = NULL;

static lv_poc_member_list_t * member_list = NULL;

static lv_obj_t * lv_poc_group_list_activity_create(lv_poc_display_t *display);

static void lv_poc_group_list_activity_destory(lv_obj_t *obj);

static void lv_poc_group_list_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_group_list_press_btn_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t lv_poc_group_list_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool lv_poc_group_list_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static void lv_poc_get_group_list_cb(int result_type);

static void lv_poc_group_lock_oprator_cb(lv_poc_group_oprator_type opt);

static void lv_poc_group_delete_oprator_cb(int result_type);

static void lv_poc_lock_group_question_OK_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_lock_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_unlock_group_question_OK_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_unlock_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_delete_group_question_OK_cb(lv_obj_t * obj, lv_event_t event);

static void lv_poc_delete_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event);

static lv_obj_t * activity_list = NULL;

static lv_poc_group_list_item_info_t * lv_poc_group_list_info = NULL;

static lv_poc_group_list_item_info_t * lv_poc_group_lock_info = NULL;

static lv_poc_group_list_item_info_t * lv_poc_group_delete_info = NULL;

static lv_poc_group_list_item_info_t * lv_poc_group_current_lock_info = NULL;

static lv_poc_group_list_item_info_t * lv_poc_group_current_info = NULL;

lv_poc_group_info_t *lv_poc_group_list_get_member_list_info = NULL;

static lv_poc_grp_refresh_t *grp_refresh_attr = NULL;

static lv_area_t display_area = {0};

static int refresh_task_init = true;

lv_poc_activity_t * poc_group_list_activity = NULL;

char group_member_list_is_open = 0;

static int prv_group_list_cur_opt = 0;

//hightlight
static char prv_group_list_last_index_groupname[64] = {0};

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
	lv_poc_notation_refresh();//把弹框显示在最顶层
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
		list_element_t * temp_p = NULL;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
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
		list_element_t * temp_p = NULL;
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
    if(member_list == NULL)
    {
		OSI_LOGI(0, "[grouprefr](%d):member list NULL", __LINE__);
		lv_poc_set_refr_error_info(true);
	    return;
    }

    if(msg_type == 1)
    {
		OSI_PRINTFI("[group][getmemberlist](%s)(%d):success get member list, to refr", __func__, __LINE__);
		osiMutexLock(grp_refresh_attr->mutex);
		grp_refresh_attr->grp_refresh_type = POC_REFRESH_TYPE_GRP_TITLE;
		grp_refresh_attr->grp_refresh_queue[grp_refresh_attr->write_index] = grp_refresh_attr->grp_refresh_type;
		grp_refresh_attr->write_index < 9 ? (grp_refresh_attr->write_index++) : (grp_refresh_attr->write_index = 0);
		osiMutexUnlock(grp_refresh_attr->mutex);
    }
    else
    {
		OSI_PRINTFI("[grouprefr][%s][%d]get failed memberlist", __func__, __LINE__);
		lv_poc_set_refr_error_info(true);
	    poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Member, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"成员列表", (const uint8_t *)"获取失败");
    }
}

static
void lv_poc_set_current_group_informartion(int result_type)
{
	if(result_type == 1)
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Join_Group, 50, false);
		if(lv_poc_group_current_info != NULL && activity_list != NULL)
		{
			lv_obj_t *cur_btn = lv_list_get_btn_selected(activity_list);
			if(cur_btn == NULL) return;

			lv_poc_group_list_item_info_t *group_info = lv_poc_group_current_info;
			if(group_info == NULL) return;

			list_element_t * group_item = (list_element_t *)group_info->item_information;
			if(group_item == NULL) return;
			lv_obj_t *btn_label = lv_list_get_btn_label(group_item->list_item);
			lv_label_set_text(btn_label, " ");
			lv_label_set_text(btn_label, lv_poc_get_group_name((lv_poc_group_info_t)group_item->information));
			lv_img_set_src(group_info->lock_img, &unlock);
			group_info->is_lock = false;
			/*当前选的group list*/
			group_info = (lv_poc_group_list_item_info_t *)cur_btn->user_data;
			group_item = (list_element_t *)group_info->item_information;
			btn_label = lv_list_get_btn_label(group_item->list_item);
	    	strcpy(lv_poc_group_list_current_group_title, (const char *)"[当前群组]");
          	strcat(lv_poc_group_list_current_group_title, (const char *)lv_poc_get_group_name((lv_poc_group_info_t)group_item->information));
			lv_label_set_text(btn_label, lv_poc_group_list_current_group_title);
			lv_img_set_src(group_info->lock_img, &unlock);
			group_info->is_lock = false;

			if(group_info == NULL) return;
			lv_poc_group_current_info = group_info;

			if(lv_poc_get_lock_group() != NULL)
			{
				OSI_PRINTFI("[grouproptack](%s)(%d):set current group", __func__, __LINE__);

				lv_poc_set_lock_group(LV_POC_GROUP_OPRATOR_TYPE_LOCK, (lv_poc_group_info_t)group_item->information, lv_poc_group_lock_oprator_cb);
			}
		}
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"切换群组", (const uint8_t *)"成功");
		lv_poc_set_group_status(true);
	}
	else if(result_type == 2)
	{
		lv_poc_set_group_status(true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"已在群组", (const uint8_t *)"");
	}
	else
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"切换群组", (const uint8_t *)"失败");
	}
}

static void lv_poc_group_list_set_current_group_cb(int result_type)
{
	osiMutexLock(grp_refresh_attr->mutex);
	grp_refresh_attr->grp_refresh_type = POC_REFRESH_TYPE_SET_CUR_INFO;
 	grp_refresh_attr->user_data = (void *)result_type;
	grp_refresh_attr->grp_refresh_queue[grp_refresh_attr->write_index] = grp_refresh_attr->grp_refresh_type;
	grp_refresh_attr->write_index < 9 ? (grp_refresh_attr->write_index++) : (grp_refresh_attr->write_index = 0);
	osiMutexUnlock(grp_refresh_attr->mutex);
}

static void lv_poc_group_list_press_btn_cb(lv_obj_t * obj, lv_event_t event)
{
	if(lvPocGuiIdtCom_get_listen_status()
		|| lvPocGuiIdtCom_get_speak_status())
	{
		OSI_PRINTFI("[poc][group](%s)(%d)is listen status or speak status", __func__, __LINE__);
		return;
	}

	lv_poc_group_list_item_info_t * p_info = (lv_poc_group_list_item_info_t *)obj->user_data;
	Ap_OSI_ASSERT((p_info != NULL), "[song]group_list_item NULL");

	lv_area_t lock_window_area = {0};

	list_element_t * p_element = (list_element_t *)p_info->item_information;
	Ap_OSI_ASSERT((p_element != NULL), "[song]group_list list_element_t NULL");

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
			OSI_LOGI(0, "[song]lv_poc_group_list_press_btn_cb is run");
			memset((void *)member_list, 0, sizeof(lv_poc_member_list_t));
			strcpy(lv_poc_group_member_list_title, (const char *)p_element->name);
			member_list->hide_offline = false;
			lv_poc_group_list_get_member_list_info = (lv_poc_group_info_t *)p_element->information;
			lv_poc_set_refr_error_info(false);
			lv_poc_get_member_list(lv_poc_group_list_get_member_list_info,
				member_list,
				1,
				lv_poc_group_list_get_membet_list_cb);
		}
	}
	else if(LV_EVENT_LONG_PRESSED == event)
	{
		if(prv_group_list_cur_opt == 1)
		{
			lv_poc_opt_refr_status(LVPOCUNREFOPTIDTCOM_SIGNAL_LOCKORUNLOCK_GROUP_STATUS);

			Ap_OSI_ASSERT(lv_poc_group_list_info != NULL, "[song]lock group_list info NULL");

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
			lv_poc_opt_refr_status(LVPOCUNREFOPTIDTCOM_SIGNAL_DELETE_GROUP_STATUS);
			OSI_PRINTFI("[groupcb](%s)(%d):ready open warnning", __func__, __LINE__);

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
					OSI_PRINTFI("[grouprefr](%s)[%d]is_refr_complete (%d), refr_err_info (%d)", __func__, __LINE__, lv_poc_is_grouplist_refr_complete(), lv_poc_get_refr_error_info());
					if(lv_poc_is_grouplist_refr_complete()
						|| lv_poc_get_refr_error_info())
					{
						lv_poc_group_list_set_hightlight_index();
						lv_poc_del_activity(poc_group_list_activity);
					}
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			OSI_PRINTFI("[grouprefr](%s)(%d):grouplist focus", __func__, __LINE__);
			if(lv_poc_is_group_list_refr())
			{
				OSI_PRINTFI("[grouprefr](%s)(%d):grouplist focus have refr", __func__, __LINE__);
				lv_poc_activity_func_cb_set.group_list.refresh_with_data(NULL);
				lv_poc_set_group_refr(false);
			}
			break;
		}

		case LV_SIGNAL_DEFOCUS:
		{
			OSI_PRINTFI("[grouprefr](%s)(%d):grouplist defocus", __func__, __LINE__);
			break;
		}
		case LV_SIGNAL_LONG_PRESS_REP:
		{
			if(param == NULL) return LV_RES_OK;
			unsigned int c = *(unsigned int *)param;

			if(!lv_poc_is_grouplist_refr_complete())
			{
				OSI_PRINTFI("[grouprefr](%s)(%d):error", __func__, __LINE__);
				break;
			}
			OSI_PRINTFI("[grouprefr](%s)(%d):longpress id (%d)", __func__, __LINE__, c);

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
					OSI_LOGI(0, "[longpress][delgroup]rec press");
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
	if(poc_group_list_activity == NULL
		|| current_activity != poc_group_list_activity)
	{
		OSI_LOGI(0, "[grouprefr](%d):error", __LINE__);
		lv_poc_set_refr_error_info(true);
		return;
	}

	if(result_type == 1)
	{
		OSI_PRINTFI("[group][refresh](%s)(%d):goto refresh", __func__, __LINE__);
		if(grp_refresh_attr->mutex == NULL)
		{
			OSI_PRINTFI("[poc][group](%s)[%d]mutex is null", __func__, __LINE__);
		}

		osiMutexLock(grp_refresh_attr->mutex);
		grp_refresh_attr->grp_refresh_type = POC_REFRESH_TYPE_GROUP_LIST;
		grp_refresh_attr->grp_refresh_queue[grp_refresh_attr->write_index] = grp_refresh_attr->grp_refresh_type;
		grp_refresh_attr->write_index < 9 ? (grp_refresh_attr->write_index++) : (grp_refresh_attr->write_index = 0);
		osiMutexUnlock(grp_refresh_attr->mutex);
	}
	else
	{
		OSI_PRINTFI("[grouprefr][%s][%d]get failed grouplist", __func__, __LINE__);
		lv_poc_set_refr_error_info(true);
		poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Group, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
	}
}

void lv_poc_group_lock_oprator_refresh_task(lv_poc_group_oprator_type opt)
{
	OSI_PRINTFI("[idtlockgroup]%s(%d):lock group refr opt is (%d)", __func__, __LINE__, opt);
	if(opt == LV_POC_GROUP_OPRATOR_TYPE_LOCK)
	{
		if(lv_poc_group_current_info != NULL)
		{
			OSI_LOGI(0, "[song]group_lock_oprator_refresh\n");

			if(lv_poc_group_lock_info == NULL)
			{
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
	    	strcpy(lv_poc_group_list_current_group_title, (const char *)"[当前群组]");
          	strcat(lv_poc_group_list_current_group_title, (const char *)lv_poc_get_group_name((lv_poc_group_info_t)group_item->information));
			lv_label_set_text(btn_label, lv_poc_group_list_current_group_title);
			lv_img_set_src(group_info->lock_img, &locked);
			group_info->is_lock = true;

			OSI_PRINTFI("[idtlockgroup]%s(%d):lock group start refresh", __func__, __LINE__);
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

		OSI_PRINTFI("[idtlockgroup]%s(%d):unlock group start refresh", __func__, __LINE__);
	}
}

static void lv_poc_group_lock_oprator_cb(lv_poc_group_oprator_type opt)
{
	switch(opt)
	{
		case LV_POC_GROUP_OPRATOR_TYPE_LOCK_FAILED:
		{
			OSI_PRINTFI("[idtlockgroup]%s(%d):lock failed cb", __func__, __LINE__);

			lv_poc_group_lock_info = NULL;
			break;
		}

		case LV_POC_GROUP_OPRATOR_TYPE_LOCK_OK:
		{
			if(lv_poc_group_lock_info == NULL || lv_poc_group_current_info == NULL)
			{
				OSI_PRINTFI("[idtlockgroup]%s(%d):lock an empty group", __func__, __LINE__);

				break;
			}
			osiMutexLock(grp_refresh_attr->mutex);
			grp_refresh_attr->grp_refresh_type = POC_REFRESH_TYPE_LOCK_GRP;
			grp_refresh_attr->lock_type = LV_POC_GROUP_OPRATOR_TYPE_LOCK;
			grp_refresh_attr->grp_refresh_queue[grp_refresh_attr->write_index] = grp_refresh_attr->grp_refresh_type;
			grp_refresh_attr->write_index < 9 ? (grp_refresh_attr->write_index++) : (grp_refresh_attr->write_index = 0);
			osiMutexUnlock(grp_refresh_attr->mutex);

			OSI_PRINTFI("[idtlockgroup]%s(%d):lock success cb", __func__, __LINE__);

			break;
		}

		case LV_POC_GROUP_OPRATOR_TYPE_UNLOCK_FAILED:
		{
			OSI_PRINTFI("[idtlockgroup]%s(%d):unlock group fail cb", __func__, __LINE__);

			break;
		}

		case LV_POC_GROUP_OPRATOR_TYPE_UNLOCK_OK:
		{
			if(lv_poc_group_current_lock_info == NULL)
			{
				OSI_PRINTFI("[idtlockgroup]%s(%d):unlock an empty group", __func__, __LINE__);

				break;
			}

			osiMutexLock(grp_refresh_attr->mutex);
			grp_refresh_attr->grp_refresh_type = POC_REFRESH_TYPE_LOCK_GRP;
			grp_refresh_attr->lock_type = LV_POC_GROUP_OPRATOR_TYPE_UNLOCK;
			grp_refresh_attr->grp_refresh_queue[grp_refresh_attr->write_index] = grp_refresh_attr->grp_refresh_type;
			grp_refresh_attr->write_index < 9 ? (grp_refresh_attr->write_index++) : (grp_refresh_attr->write_index = 0);
			osiMutexUnlock(grp_refresh_attr->mutex);

			OSI_PRINTFI("[idtlockgroup]%s(%d):unlock group success cb", __func__, __LINE__);

			break;
		}

		default:
			break;
	}
}

static void lv_poc_group_delete_oprator_cb(int result_type)
{
	if(poc_group_list_activity == NULL
		|| current_activity != poc_group_list_activity)
	{
		OSI_PRINTFI("[del]%s(%d):error", __func__, __LINE__);
		return;
	}

	if(result_type == 0)//refr delete group
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
				poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Group, 50, true);
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

		lv_poc_opt_refr_status(LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS);
	}
}

static void lv_poc_lock_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_CANCEL)
	{
		lv_poc_opt_refr_status(LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS);
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
		lv_poc_opt_refr_status(LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS);
	}
}

static void lv_poc_unlock_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_CANCEL)
	{
		lv_poc_opt_refr_status(LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS);
	}
}

static void lv_poc_delete_group_question_OK_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_APPLY)
	{
		lv_poc_opt_refr_status(LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS);

		Ap_OSI_ASSERT((lv_poc_group_delete_info != NULL), "[song]delete group NULL"); /*assert verified*/

		list_element_t * item_info = (list_element_t *)lv_poc_group_delete_info->item_information;
		lv_poc_group_info_t * group_info = (lv_poc_group_info_t)item_info->information;
		lv_poc_delete_group((lv_poc_group_info_t)group_info, lv_poc_group_delete_oprator_cb);
	}
}

static void lv_poc_delete_group_question_CANCEL_cb(lv_obj_t * obj, lv_event_t event)
{
	if(event == LV_EVENT_CANCEL)
	{
		lv_poc_opt_refr_status(LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS);
	}
}


void lv_poc_group_list_open(lv_poc_group_list_t *group_list_obj)
{
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_GROUP_LIST,
															lv_poc_group_list_activity_create,
															lv_poc_group_list_activity_destory};

    if(poc_group_list_activity != NULL || group_list != NULL || activity_list != NULL)
    {
    	OSI_PRINTFI("[poc][group](%s)[%d]error", __func__, __LINE__);
		lv_poc_set_refr_error_info(true);
    	return;
    }
	OSI_PRINTFI("[poc][group](%s)[%d]open grp list", __func__, __LINE__);

	group_list = (lv_poc_group_list_t *)lv_mem_alloc(sizeof(lv_poc_group_list_t));

	if(group_list == NULL)
	{
		lv_poc_set_refr_error_info(true);
		return;
	}

	lv_poc_set_refr_error_info(false);
	lv_poc_set_grouplist_refr_is_complete(false);

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

	if(refresh_task_init == true)
	{
		if(grp_refresh_attr == NULL)
		{
		   grp_refresh_attr = (lv_poc_grp_refresh_t *)lv_mem_alloc(sizeof(lv_poc_grp_refresh_t));
		}
	 	refresh_task_init = false;
		memset(grp_refresh_attr, 0, sizeof(lv_poc_grp_refresh_t));
		grp_refresh_attr->task = NULL;
		grp_refresh_attr->mutex = NULL;
		grp_refresh_attr->user_data = NULL;
	}

	if(grp_refresh_attr->task == NULL)
	{
		grp_refresh_attr->task = lv_task_create(lv_poc_group_list_refresh_task, 30, LV_TASK_PRIO_MID, (void *)grp_refresh_attr);
	}

	if(grp_refresh_attr->mutex == NULL)
	{
		grp_refresh_attr->mutex = osiMutexCreate();
		OSI_PRINTFI("[poc][group](%s)[%d]create mutex", __func__, __LINE__);
	}

    if(group_list_obj == NULL)
    {
		if(!lv_poc_get_group_list(group_list, lv_poc_get_group_list_cb))
		{
			OSI_LOGI(0, "[poc][group](%d)error", __LINE__);
			lv_poc_set_refr_error_info(true);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Group, 50, true);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
		}
    }
    else
    {
		osiMutexLock(grp_refresh_attr->mutex);
		grp_refresh_attr->grp_refresh_type = POC_REFRESH_TYPE_GROUP_LIST;
		grp_refresh_attr->grp_refresh_queue[grp_refresh_attr->write_index] = grp_refresh_attr->grp_refresh_type;
		grp_refresh_attr->write_index < 9 ? (grp_refresh_attr->write_index++) : (grp_refresh_attr->write_index = 0);
		osiMutexUnlock(grp_refresh_attr->mutex);
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
    if(p_cur != NULL && GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        group_list_obj->group_list = p_cur->next;
        lv_obj_del(p_cur->list_item);
        lv_mem_free(p_cur);
        group_list_obj->group_number = group_list_obj->group_number - 1;
        return;
    }
    p_prv = p_cur;
	if(p_cur != NULL)
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
	list_element_t * temp_p = NULL;
	while(cur_p != NULL)
	{
		temp_p = cur_p;
		cur_p =cur_p->next;
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

void lv_poc_group_list_refresh(lv_poc_group_list_t *grouplist)
{
	lv_poc_group_list_t *group_list_obj = NULL;

	group_list_obj = grouplist;

	if(group_list_obj == NULL)
	{
		OSI_LOGI(0, "[grouprefr](%d):get refr info", __LINE__);
		group_list_obj = group_list;
	}

	if(current_activity != poc_group_list_activity
		|| group_list_obj == NULL)
	{
		OSI_LOGI(0, "[grouprefr](%d):error", __LINE__);
		lv_poc_set_refr_error_info(true);
		return;
	}

	int current_index = 0;
	int list_btn_count = 0;

    list_element_t * p_cur = NULL;
    lv_obj_t * btn = NULL;
	lv_obj_t * btn_index[32];//assume group number is 32
    lv_obj_t * img = NULL;
	lv_obj_t * btn_label = NULL;
    lv_coord_t btn_height = (display_area.y2 - display_area.y1)/(LV_POC_LIST_COLUM_COUNT + 1);
	lv_coord_t btn_width = (display_area.x2 - display_area.x1);

    char is_set_current_group = 1;
    char is_set_lock_group = 1;
	char is_set_btn_selected = 0;

    lv_list_clean(activity_list);

    if(group_list_obj->group_list == NULL)
    {
		OSI_LOGI(0, "[grouprefr](%d):error no member", __LINE__);
	    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无成员列表", NULL);
		lv_poc_set_refr_error_info(true);
		return;
    }

    lv_poc_group_info_t current_group = lv_poc_get_current_group();
    char * current_group_name = lv_poc_get_group_name(current_group);
    lv_poc_group_info_t lock_group = lv_poc_get_lock_group();
    char * lock_group_name = lv_poc_get_group_name(lock_group);

    if(lock_group == NULL)
    {
		OSI_LOGI(0, "[grouprefr](%d):no lock", __LINE__);
	    is_set_lock_group = 0;
    }
    lv_poc_group_current_lock_info = NULL;

    if(lv_poc_group_list_info != NULL)
    {
		OSI_LOGI(0, "[grouprefr](%d):free group list info", __LINE__);
	    lv_mem_free(lv_poc_group_list_info);
	    lv_poc_group_list_info = NULL;
    }

    lv_poc_group_list_info = (lv_poc_group_list_item_info_t *)lv_mem_alloc(sizeof(lv_poc_group_list_item_info_t) * group_list_obj->group_number);
    if(lv_poc_group_list_info == NULL)
    {
		OSI_LOGI(0, "[grouprefr](%d):group list info mem alloc failed", __LINE__);
		lv_poc_set_refr_error_info(true);
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
		btn_index[list_btn_count] = btn;
        list_btn_count++;

		#if 0
		OSI_LOGXI(OSI_LOGPAR_S, 0, "[song]refresh index name %s", p_cur->name);
		OSI_LOGXI(OSI_LOGPAR_S, 0, "[song]refresh cp name %s", prv_group_list_last_index_groupname);
		#endif
		if(NULL != prv_group_list_last_index_groupname
			&& NULL != strstr(p_cur->name, prv_group_list_last_index_groupname)
			&& is_set_btn_selected == 0)
		{
			lv_list_set_btn_selected(activity_list, btn);
			is_set_btn_selected = 1;
		}

		btn_label = lv_list_get_btn_label(btn);
        if(is_set_current_group == 1
	        && GROUP_EQUATION((void *)p_cur->name, (void *)current_group_name, (void *)p_cur->information, (void *)current_group, NULL))
        {
	        is_set_current_group = 0;
        	strcpy(lv_poc_group_list_current_group_title, (const char *)"[当前群组]");
        	strcat(lv_poc_group_list_current_group_title, (const char *)p_cur->name);
        	lv_label_set_text(btn_label, (const char *)lv_poc_group_list_current_group_title);
        	lv_poc_group_current_info = p_group_info;
			current_index = list_btn_count - 1;
		}

		img = lv_img_create(btn, NULL);
		p_group_info->lock_img = img;
        if(is_set_lock_group == 1
	        && GROUP_EQUATION((void *)p_cur->name, (void *)lock_group_name, (void *)p_cur->information, (void *)lock_group, NULL))
        {
	        is_set_lock_group = 0;
			lv_img_set_src(img, &locked);//lock
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
	//not find group
	if(0 == is_set_btn_selected)
	{
		lv_list_set_btn_selected(activity_list, btn_index[current_index]);
	}
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND, NULL);
	lv_poc_set_grouplist_refr_is_complete(true);
}

void lv_poc_group_list_refresh_with_data(lv_poc_group_list_t *group_list_obj)
{
	if(!lv_poc_get_poweron_is_ready())
	{
		return;
	}

	if(current_activity != poc_group_list_activity)
	{
		OSI_LOGI(0, "[grouprefr](%d):The current group has not been refreshed", __LINE__);
		lv_poc_set_group_refr(true);//记录有信息待刷新
		return;
	}

	if(lv_poc_is_inside_group())//若当前设备在某个群组里,禁止群组的所有更新(包括添组、删组、组信息更新)
	{
		OSI_LOGI(0, "[grouprefr](%d):currently in the group", __LINE__);
		lv_poc_set_group_refr(true);//记录有信息待刷新
		return;
	}

	if(lv_poc_opt_refr_status(false) != LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS)//防止一些界面刷新数据混乱导致死机问题
	{
		OSI_LOGI(0, "[grouprefr](%d):Prohibit to refresh", __LINE__);
		lv_poc_set_group_refr(true);//记录有信息待刷新
		return;
	}

	if(group_list_obj == NULL)
	{
		group_list_obj = group_list;
	}

	if(group_list_obj == NULL)
	{
		return;
	}
	//set hight index
	lv_poc_group_list_set_hightlight_index();

	lv_list_clean(activity_list);
	lv_poc_group_list_clear(group_list_obj);

	lv_poc_get_group_list(group_list, lv_poc_get_group_list_cb);
	OSI_LOGI(0, "[grouprefr](%d):refresh grouplist", __LINE__);
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
    if(p_cur != NULL && GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
	if(p_cur != NULL)
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
    if(p_scr != NULL && GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        is_find = true;
        p_scr = p_cur;
        group_list_obj->group_list = p_cur->next;
    }
    p_prv = p_cur;
	if(p_cur != NULL)
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
    if(p_cur != NULL && GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    if(p_cur != NULL && GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        p_prv->next = p_cur->next;
        p_cur->next = p_prv;
        group_list_obj->group_list = p_cur;
        return POC_OPERATE_SECCESS;
    }

    p_prv_prv = p_prv;
    p_prv = p_cur;
	if(p_cur != NULL)
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

    p_cur = group_list_obj->group_list;/*p_cur != NULL 防止内存泄露*/
    if(p_cur != NULL && GROUP_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
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
	if(p_cur != NULL)
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
	osiMutexLock(grp_refresh_attr->mutex);
	grp_refresh_attr->grp_refresh_type = POC_REFRESH_TYPE_LOCK_GRP;
	grp_refresh_attr->lock_type = opt;
	grp_refresh_attr->grp_refresh_queue[grp_refresh_attr->write_index] = grp_refresh_attr->grp_refresh_type;
	grp_refresh_attr->write_index < 9 ? (grp_refresh_attr->write_index++) : (grp_refresh_attr->write_index = 0);
	osiMutexUnlock(grp_refresh_attr->mutex);
    return POC_GROUP_NONENTITY;
}

static
void lv_poc_group_list_title_refr(void)
{
	OSI_PRINTFI("[member][refresh](%s)(%d):ready open", __func__, __LINE__);
	lv_poc_member_list_open(lv_poc_group_member_list_title,
		member_list,
		member_list->hide_offline);
}

void lv_poc_group_list_set_hightlight_index(void)
{
	lv_obj_t *current_btn = lv_list_get_btn_selected(activity_list);

	if(current_btn != NULL)
	{
		strcpy(prv_group_list_last_index_groupname, lv_list_get_btn_text(current_btn));
	}
}

void lv_poc_group_list_refresh_task(lv_task_t *task)
{
	if(grp_refresh_attr->write_index == grp_refresh_attr->read_index)
	{
		return;
	}

	switch(grp_refresh_attr->grp_refresh_queue[grp_refresh_attr->read_index])
	{
		case POC_REFRESH_TYPE_GRP_TITLE:
		{
			OSI_PRINTFI("[grpref][cb]%s(%d):set grp title lv task refresh", __func__, __LINE__);
			lv_poc_group_list_title_refr();
			break;
		}

		case POC_REFRESH_TYPE_SET_CUR_INFO:
		{
			OSI_PRINTFI("[grpref][cb]%s(%d):set cur grp lv task refresh", __func__, __LINE__);
			int type = (int)grp_refresh_attr->user_data;
			lv_poc_set_current_group_informartion(type);
			break;
		}

		case POC_REFRESH_TYPE_LOCK_GRP:
		{
			OSI_PRINTFI("[grpref][cb]%s(%d):lock grp lv task refresh", __func__, __LINE__);
			lv_poc_group_lock_oprator_refresh_task(grp_refresh_attr->lock_type);
			grp_refresh_attr->lock_type = 0;
			break;
		}

		case POC_REFRESH_TYPE_GROUP_LIST:
		{
			OSI_PRINTFI("[grpref][cb]%s(%d) grp list refr lv task refresh", __func__, __LINE__);
			lv_poc_group_list_refresh(NULL);
			break;
		}

		default:
		{
			break;
		}
	}
	grp_refresh_attr->grp_refresh_type = POC_REFRESH_TYPE_START;
	grp_refresh_attr->grp_refresh_queue[grp_refresh_attr->read_index] = grp_refresh_attr->grp_refresh_type;
	grp_refresh_attr->read_index < 9 ? (grp_refresh_attr->read_index++) : (grp_refresh_attr->read_index = 0);
}

#ifdef __cplusplus
}
#endif
