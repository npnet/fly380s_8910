
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

enum
{
	POC_REFRESH_TYPE_TMPGRP_START,

	POC_REFRESH_TYPE_TMPGRP_MEMBER_SELECT,
	POC_REFRESH_TYPE_TMPGRP_SUCCESS,
	POC_REFRESH_TYPE_TMPGRP_MEMBER_LIST,
};

typedef struct
{
	osiMutex_t *mutex;
	lv_task_t *task;
	uint8_t refresh_type;
	void *user_data;
}lv_poc_tmpgrp_refresh_t;

static lv_obj_t * lv_poc_build_tempgrp_activity_create(lv_poc_display_t *display);

static void lv_poc_build_tempgrp_activity_destory(lv_obj_t *obj);

static void * lv_poc_build_tempgrp_list_create(lv_obj_t * parent, lv_area_t display_area);

static void lv_poc_build_tempgrp_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_build_tempgrp_pressed_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t lv_poc_build_tempgrp_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool lv_poc_build_tempgrp_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static void lv_poc_build_tempgrp_success(void);

static lv_obj_t * activity_list = NULL;

static lv_poc_build_tempgrp_item_info_t * lv_poc_build_tempgrp_info = NULL;

static int32_t lv_poc_build_tempgrp_selected_num = 0;

static lv_poc_tmpgrp_t is_lv_poc_build_tempgrp = 0;

static void *lv_poc_build_tempgrp_selected_members[6] = {0};

static lv_poc_win_t * activity_win = NULL;

static lv_area_t build_tempgrp_display_area = {0};

static lv_poc_oem_member_list *lv_poc_build_tempgrp_member_list = NULL;

static lv_poc_oem_member_list * lv_poc_member_list = NULL;

static lv_poc_tmpgrp_refresh_t *tmpgrp_refresh_attr = NULL;

static int refresh_task_init = true;

lv_poc_activity_t * poc_build_tempgrp_activity = NULL;

static char * lv_poc_build_tempgrp_failed_text = "用户均忙线中";

static char * lv_poc_build_tempgrp_few_member_text1 = "成员数量不";

static char * lv_poc_build_tempgrp_few_member_text2 = "能少于三人";

static char * lv_poc_build_tempgrp_error_text = "错误";

static lv_obj_t * lv_poc_build_tempgrp_activity_create(lv_poc_display_t *display)
{
    activity_win = lv_poc_win_create(display, "成员列表", lv_poc_build_tempgrp_list_create);
	lv_poc_notation_refresh();//把弹框显示在最顶层
    return (lv_obj_t *)activity_win;
}

static void lv_poc_build_tempgrp_activity_destory(lv_obj_t *obj)
{
	lv_poc_member_list_cb_set_active(ACT_ID_POC_MAKE_TEMPGRP, false);
    if(lv_poc_build_tempgrp_member_list != NULL)
    {
		oem_list_element_t * cur_p = lv_poc_build_tempgrp_member_list->online_list;
		oem_list_element_t * temp_p = NULL;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		cur_p = lv_poc_build_tempgrp_member_list->offline_list;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		lv_mem_free(lv_poc_build_tempgrp_member_list);
    }
    lv_poc_build_tempgrp_member_list = NULL;

    if(lv_poc_build_tempgrp_info != NULL)
    {
	    lv_mem_free(lv_poc_build_tempgrp_info);
    }
    lv_poc_build_tempgrp_info = NULL;
    lv_poc_build_tempgrp_selected_num = 0;

	if(activity_win != NULL)
	{
		lv_mem_free(activity_win);
		activity_win = NULL;
	}

    poc_build_tempgrp_activity = NULL;
}

static void * lv_poc_build_tempgrp_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    build_tempgrp_display_area.x1 = display_area.x1;
    build_tempgrp_display_area.x2 = display_area.x2;
    build_tempgrp_display_area.y1 = display_area.y1;
    build_tempgrp_display_area.y2 = display_area.y2;
    activity_list = lv_poc_list_create(parent, NULL, display_area, lv_poc_build_tempgrp_list_config);
    return (void *)activity_list;
}

static
void lv_poc_build_tempgrp_list_config(lv_obj_t * list, lv_area_t list_area)
{
}

static
void lv_poc_build_tempgrp_get_membet_list_cb(int msg_type)
{
    if(lv_poc_member_list == NULL)
    {
		OSI_PRINTFI("[tmpgrp](%s)(%d):lv_poc_member_list null", __func__, __LINE__);
	    return;
    }

    if(msg_type == 1)
    {
		OSI_PRINTFI("[tmpgrp](%s)(%d):get member cb", __func__, __LINE__);
		lv_poc_build_tempgrp_progress(POC_TMPGRP_GETMEMLIST);
		lv_poc_build_tempgrp_member_list_open(lv_poc_member_list, lv_poc_member_list->hide_offline);
    }
    else
    {
	  	lv_poc_set_refr_error_info(true);
	    poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Member, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"成员列表", (const uint8_t *)"获取失败");
    }
}

static
void lv_poc_build_tempgrp_new_group_cb_refresh(lv_task_t *task)
{
	lv_poc_del_activity(poc_build_tempgrp_activity);
}

static
void lv_poc_build_tempgrp_new_group_cb(int result_type)
{
	if(result_type == 1)
	{
		lv_poc_build_tempgrp_get_tmpgrp_member_list();
	}
	else
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)lv_poc_build_tempgrp_failed_text, NULL);
	}
	lv_poc_refr_task_once(lv_poc_build_tempgrp_new_group_cb_refresh, LVPOCLISTIDTCOM_LIST_PERIOD_30, LV_TASK_PRIO_HIGH);
}

static
bool lv_poc_build_tempgrp_operator(lv_poc_build_tempgrp_item_info_t * info, int32_t info_num, int32_t selected_num)
{
	if(info == NULL || info_num < selected_num)
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,
			(const uint8_t *)lv_poc_build_tempgrp_error_text,
			NULL);
		lv_poc_del_activity(poc_build_tempgrp_activity);
		return false;
	}

	if(selected_num < 2)//临时组至少3人
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,
			(const uint8_t *)lv_poc_build_tempgrp_few_member_text1,
			(const uint8_t *)lv_poc_build_tempgrp_few_member_text2);
		lv_poc_del_activity(poc_build_tempgrp_activity);
		return false;
	}

	memset(lv_poc_build_tempgrp_selected_members, 0, sizeof(lv_poc_member_info_t) * 6);//max 5 members

	lv_poc_build_tempgrp_item_info_t * p_info = info;
	int32_t real_num = 0;
	for(int i = 0; i < info_num; i++)
	{
		if(p_info == NULL
			|| (p_info->item_information == NULL))
		{
			return false;
		}

		if(p_info->is_selected == true)
		{
			lv_poc_build_tempgrp_selected_members[real_num] = p_info->item_information;
			real_num++;
		}
		p_info++;
	}

	if(real_num < 1)
	{
		return false;
	}

	if(lvPocGuiBndCom_get_listen_status())//listen status cannot launch tmpgrp func
	{
		OSI_LOGI(0, "[build][tmpgrp] listen isn't signal");
		return false;
	}

	return lv_poc_build_new_tempgrp((void **)lv_poc_build_tempgrp_selected_members, real_num, lv_poc_build_tempgrp_new_group_cb);
}

static
void lv_poc_build_tempgrp_success(void)
{
	lv_poc_build_tempgrp_operator(lv_poc_build_tempgrp_info,
							lv_poc_build_tempgrp_member_list->offline_number + lv_poc_build_tempgrp_member_list->online_number,
							lv_poc_build_tempgrp_selected_num);
}

static
void lv_poc_build_tempgrp_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
		if(lv_poc_build_tempgrp_info == NULL)
		{
			return;
		}

		lv_poc_build_tempgrp_item_info_t * p_info = (lv_poc_build_tempgrp_item_info_t *)obj->user_data;
		if(p_info == NULL
			|| p_info->is_self == true
			|| p_info->online == false
			|| lv_poc_build_tempgrp_selected_num > 5)
		{
			return;
		}

		p_info->is_selected = !p_info->is_selected;
	    if(p_info->is_selected)
	    {
	        lv_btn_set_state((lv_obj_t *)p_info->checkbox, LV_BTN_STATE_TGL_PR);
	        lv_poc_build_tempgrp_selected_num++;
	    }
	    else
	    {
	        lv_btn_set_state((lv_obj_t *)p_info->checkbox, LV_BTN_STATE_REL);
	        lv_poc_build_tempgrp_selected_num--;
	    }
    }
}

static
lv_res_t lv_poc_build_tempgrp_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
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
					if(lv_poc_is_buildgroup_refr_complete()
						|| lv_poc_get_refr_error_info())
					{
						if(lv_poc_build_tempgrp_progress(POC_TMPGRP_READ) == POC_TMPGRP_FINISH)
						{
							if(lvPocGuiBndCom_get_listen_status())//正在接听讲话时不准退出
							{
								break;
							}
							lv_task_t *once_task = lv_task_create(lv_poc_build_tempgrp_memberlist_activity_close, 50, LV_TASK_PRIO_MID, (void *)POC_EXITGRP_INITIATIVE);
							lv_task_once(once_task);
						}
						else
						{
							tmpgrp_refresh_attr->mutex ? osiMutexLock(tmpgrp_refresh_attr->mutex) : 0;
							tmpgrp_refresh_attr->refresh_type = POC_REFRESH_TYPE_TMPGRP_SUCCESS;
							lv_task_ready(tmpgrp_refresh_attr->task);
							tmpgrp_refresh_attr->mutex ? osiMutexUnlock(tmpgrp_refresh_attr->mutex) : 0;
						}
					}
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

static
bool lv_poc_build_tempgrp_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}

static
void lv_poc_build_tempgrp_get_list_cb(int msg_type)
{
    if(poc_build_tempgrp_activity == NULL)
    {
    	return;
    }

	if(msg_type==1)//显示
	{
		tmpgrp_refresh_attr->mutex ? osiMutexLock(tmpgrp_refresh_attr->mutex) : 0;
		tmpgrp_refresh_attr->refresh_type = POC_REFRESH_TYPE_TMPGRP_MEMBER_SELECT;
		lv_task_ready(tmpgrp_refresh_attr->task);
		tmpgrp_refresh_attr->mutex ? osiMutexUnlock(tmpgrp_refresh_attr->mutex) : 0;
	}
	else
	{
		lv_poc_set_refr_error_info(true);
		poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Member, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
	}
}

void lv_poc_build_tempgrp_open(void)
{
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_MAKE_TEMPGRP,
															lv_poc_build_tempgrp_activity_create,
															lv_poc_build_tempgrp_activity_destory};

    if(poc_build_tempgrp_activity != NULL)
    {
    	return;
    }

	if(lvPocGuiBndCom_get_listen_status())//listen status cannot build tmpgrp
	{
		OSI_LOGI(0, "[membercall] listen isn't signal");
		return;
	}

	if(lv_poc_build_tempgrp_member_list == NULL)
	{
	    lv_poc_build_tempgrp_member_list = (lv_poc_oem_member_list *)lv_mem_alloc(sizeof(lv_poc_oem_member_list));
	    if(lv_poc_build_tempgrp_member_list == NULL)
	    {
		    return;
	    }
	}

	if(lv_poc_build_tempgrp_member_list != NULL)
	{
		memset(lv_poc_build_tempgrp_member_list, 0, sizeof(lv_poc_oem_member_list));
	}

	lv_poc_set_buildgroup_refr_is_complete(false);
    poc_build_tempgrp_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_member_list_cb_set_active(ACT_ID_POC_MAKE_TEMPGRP, true);
    lv_poc_activity_set_signal_cb(poc_build_tempgrp_activity, lv_poc_build_tempgrp_signal_func);
    lv_poc_activity_set_design_cb(poc_build_tempgrp_activity, lv_poc_build_tempgrp_design_func);

	if(refresh_task_init == true)
	{
		if(tmpgrp_refresh_attr == NULL)
		{
			tmpgrp_refresh_attr = (lv_poc_tmpgrp_refresh_t *)lv_mem_alloc(sizeof(lv_poc_tmpgrp_refresh_t));
		}
		refresh_task_init = false;
		tmpgrp_refresh_attr->refresh_type = 0;
		tmpgrp_refresh_attr->task = NULL;
		tmpgrp_refresh_attr->mutex = NULL;
		tmpgrp_refresh_attr->user_data = NULL;
	}

	if(tmpgrp_refresh_attr->task == NULL)
	{
		tmpgrp_refresh_attr->task = lv_task_create(lv_poc_bulid_tempgroup_refresh_task, 500, LV_TASK_PRIO_MID, (void *)tmpgrp_refresh_attr);
	}

	if(tmpgrp_refresh_attr->mutex == NULL)
	{
		tmpgrp_refresh_attr->mutex = osiMutexCreate();
	}

	if(!lv_poc_get_member_list(NULL, lv_poc_build_tempgrp_member_list, 1, lv_poc_build_tempgrp_get_list_cb))
	{
		lv_poc_set_refr_error_info(true);
		poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Member, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
	}
}

void lv_poc_build_tempgrp_get_tmpgrp_member_list(void)
{
	if(lv_poc_member_list != NULL)//free
	{
		oem_list_element_t * cur_p = lv_poc_member_list->online_list;
		oem_list_element_t * temp_p = NULL;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		cur_p = lv_poc_member_list->offline_list;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		lv_mem_free(lv_poc_member_list);
	}
	lv_poc_member_list = NULL;

	if(lv_poc_member_list == NULL)
	{
		lv_poc_member_list = (lv_poc_oem_member_list *)lv_mem_alloc(sizeof(lv_poc_oem_member_list));
	}

	if(lv_poc_member_list != NULL)
	{
		memset((void *)lv_poc_member_list, 0, sizeof(lv_poc_oem_member_list));
		lv_poc_member_list->hide_offline = false;
		lv_poc_get_member_list(NULL,
						lv_poc_member_list,
						1,
						lv_poc_build_tempgrp_get_membet_list_cb);
	}
}

void lv_poc_build_tempgrp_clear(lv_poc_oem_member_list *member_list_obj)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_tempgrp_member_list;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

	lv_poc_member_list_clear(member_list_obj);
}

int lv_poc_build_tempgrp_get_information(lv_poc_oem_member_list *member_list_obj, const char * name, void *** information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_tempgrp_member_list;
	}

	if(member_list_obj == NULL)
	{
		return 0;
	}

	return lv_poc_member_list_get_information(member_list_obj, name, information);
}

void lv_poc_build_tempgrp_refresh(lv_poc_oem_member_list *memberlist)
{
	lv_poc_oem_member_list *member_list_obj = NULL;

	member_list_obj = memberlist;

	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_tempgrp_member_list;
	}

	if(current_activity != poc_build_tempgrp_activity
		|| member_list_obj == NULL)
	{
		lv_poc_set_refr_error_info(true);
		return;
	}

    oem_list_element_t * p_cur = NULL;
    lv_obj_t * btn = NULL;
    lv_obj_t * btn_checkbox = NULL;
    lv_obj_t * btn_label = NULL;
    lv_poc_build_tempgrp_item_info_t * p_info = NULL;
    lv_coord_t btn_height = (build_tempgrp_display_area.y2 - build_tempgrp_display_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (build_tempgrp_display_area.x2 - build_tempgrp_display_area.x1);
    char member_list_is_first_item = 1;

	if(activity_list == NULL)
	{
		lv_poc_set_refr_error_info(true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"空窗口列表", NULL);
		return;
	}
    lv_list_clean(activity_list);

    if(!(member_list_obj->online_list != NULL || member_list_obj->offline_list != NULL))
    {
		lv_poc_set_refr_error_info(true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无成员列表", NULL);
	    return;
    }

    if(lv_poc_build_tempgrp_info != NULL)
    {
	    lv_mem_free(lv_poc_build_tempgrp_info);
	    lv_poc_build_tempgrp_info = NULL;
    }

    lv_poc_build_tempgrp_info = (lv_poc_build_tempgrp_item_info_t *)lv_mem_alloc(sizeof(lv_poc_build_tempgrp_item_info_t) * (member_list_obj->online_number + member_list_obj->offline_number));

	if(lv_poc_build_tempgrp_info == NULL)
	{
		lv_poc_set_refr_error_info(true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"刷新失败", NULL);
	    return;
	}
	lv_poc_build_tempgrp_selected_num = 0;

	p_info = lv_poc_build_tempgrp_info;

    p_cur = member_list_obj->online_list;

    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_member_online, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_build_tempgrp_pressed_cb);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);

        btn_label = lv_list_get_btn_label(btn);
        lv_label_set_long_mode(btn_label, LV_LABEL_LONG_SROLL);

        btn_checkbox = lv_cb_create(btn, NULL);
		lv_cb_set_text(btn_checkbox, "");
		lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(btn_checkbox) - ic_member_online.header.w - 9);/*此处修改图标位置*/
		lv_obj_align(btn_checkbox, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		if(NULL != strstr(lv_list_get_btn_text(btn),"我"))//如果是自己
		{
			p_info->is_self = true;
			p_info->is_selected = true;
			lv_btn_set_state(btn_checkbox, LV_BTN_STATE_TGL_PR);
			lv_poc_build_tempgrp_selected_num++;
		}
		else
		{
			p_info->is_self = false;
			p_info->is_selected = false;
			lv_btn_set_state(btn_checkbox, LV_BTN_STATE_REL);
		}

		p_info->online = true;
        p_info->item_information = p_cur->information;
        p_info->checkbox = btn_checkbox;
        btn->user_data = (void *)p_info;
        p_info++;
        p_cur = p_cur->next;

        if(member_list_is_first_item == 1)
        {
        	member_list_is_first_item = 0;
        	lv_list_set_btn_selected(activity_list, btn);
        }
    }

    p_cur = member_list_obj->offline_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_member_offline, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_build_tempgrp_pressed_cb);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);

        btn_label = lv_list_get_btn_label(btn);
        lv_label_set_long_mode(btn_label, LV_LABEL_LONG_SROLL);

        btn_checkbox = lv_cb_create(btn, NULL);
		lv_cb_set_text(btn_checkbox, "");
		lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(btn_checkbox) - ic_member_offline.header.w - 9);
		lv_btn_set_state(btn_checkbox, LV_BTN_STATE_REL);
		lv_obj_align(btn_checkbox, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		p_info->online = false;
		p_info->is_self = false;
        p_info->is_selected = false;
        p_info->item_information = p_cur->information;
        p_info->checkbox = btn_checkbox;
        btn->user_data = (void *)p_info;
        p_info++;
        p_cur = p_cur->next;
        if(member_list_is_first_item == 1)
        {
        	member_list_is_first_item = 0;
        	lv_list_set_btn_selected(activity_list, btn);
        }
    }
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND, NULL);
	lv_poc_set_buildgroup_refr_is_complete(true);
}

void lv_poc_build_tempgrp_refresh_with_data(lv_poc_oem_member_list *member_list_obj)
{
	if(lv_poc_opt_refr_status(false) != LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS)/*防止一些界面刷新数据混乱导致死机问题*/
	{
		lv_poc_set_group_refr(true);//记录有信息待刷新
		return;
	}

	OSI_LOGI(0, "[poc][tempgrp]build grouplist refreshing");

	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_tempgrp_member_list;
	}

	if(member_list_obj == NULL)
	{
		return;
	}
	lv_poc_build_tempgrp_clear(lv_poc_build_tempgrp_member_list);

	lv_poc_get_member_list(NULL, member_list_obj, 1, lv_poc_build_tempgrp_get_list_cb);
}

lv_poc_status_t lv_poc_build_tempgrp_set_state(lv_poc_oem_member_list *member_list_obj, const char * name, void * information, bool is_online)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_tempgrp_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    oem_list_element_t * p_cur = NULL;
    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    if(true == is_online)
    {
        p_cur = member_list_obj->offline_list;
        if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
        	member_list_obj->offline_list = p_cur->next;
            p_cur->next = member_list_obj->online_list;
            member_list_obj->online_list = p_cur;
            goto LV_POC_BUILD_TEMPGRP_SET_STATE_SUCCESS;
        }
		if(p_cur != NULL)
        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
            {
            	member_list_obj->offline_list = p_cur->next;
                p_cur->next = member_list_obj->online_list;
                member_list_obj->online_list = p_cur;
                goto LV_POC_BUILD_TEMPGRP_SET_STATE_SUCCESS;
            }
            p_cur = p_cur->next;
        }
    }
    else
    {
        p_cur = member_list_obj->online_list;
        if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
        	member_list_obj->online_list = p_cur->next;
            p_cur->next = member_list_obj->offline_list;
            member_list_obj->offline_list = p_cur;
            goto LV_POC_BUILD_TEMPGRP_SET_STATE_SUCCESS;
        }
		if(p_cur != NULL)
        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
            {
            	member_list_obj->online_list = p_cur->next;
                p_cur->next = member_list_obj->offline_list;
                member_list_obj->offline_list = p_cur;
                goto LV_POC_BUILD_TEMPGRP_SET_STATE_SUCCESS;
            }
            p_cur = p_cur->next;
        }
    }

    return POC_UNKNOWN_FAULT;

LV_POC_BUILD_TEMPGRP_SET_STATE_SUCCESS:
	if(p_cur != NULL && p_cur->list_item != NULL)
	{
		lv_obj_t *btn_item = (lv_obj_t *)p_cur->list_item;
		lv_obj_t *btn_img = lv_list_get_btn_img(btn_item);

		if(is_online)
		{
			lv_img_set_src(btn_img, &ic_member_online);
		}
		else
		{
			lv_img_set_src(btn_img, &ic_member_offline);
		}
	}
	return POC_OPERATE_SECCESS;
}

lv_poc_status_t lv_poc_build_tempgrp_is_exists(lv_poc_oem_member_list *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_tempgrp_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_is_exists(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_build_tempgrp_get_state(lv_poc_oem_member_list *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_tempgrp_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_get_state(member_list_obj, name, information);
}

static int8_t lv_poc_build_tempgrp_member_list_title[100] = {0};
static lv_area_t build_tempgrp_member_list_display_area = {0};
lv_poc_activity_t * poc_build_tempgrp_member_list_activity = NULL;
static lv_poc_oem_member_list * lv_poc_build_tempgrp_member_list_obj = NULL;

void lv_poc_build_tempgrp_member_list_refresh(lv_poc_oem_member_list *member_list_obj);

static void lv_poc_build_tempgrp_member_list_list_config(lv_obj_t * list, lv_area_t list_area)
{

}

#if 0
static void lv_poc_build_tempgrp_member_list_prssed_btn_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{

	}
}
#endif

static void * lv_poc_build_tempgrp_member_list_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    build_tempgrp_member_list_display_area.x1 = display_area.x1;
    build_tempgrp_member_list_display_area.x2 = display_area.x2;
    build_tempgrp_member_list_display_area.y1 = display_area.y1;
    build_tempgrp_member_list_display_area.y2 = display_area.y2;
    activity_list = lv_poc_list_create(parent, NULL, display_area, lv_poc_build_tempgrp_member_list_list_config);
    return (void *)activity_list;
}

static lv_obj_t * lv_poc_build_tempgrp_member_list_activity_create(lv_poc_display_t *display)
{
    activity_win = lv_poc_win_create(display, (const char *)lv_poc_build_tempgrp_member_list_title, lv_poc_build_tempgrp_member_list_list_create);
	lv_poc_notation_refresh();//把弹框显示在最顶层
	return (lv_obj_t *)activity_win;
}

static void lv_poc_build_tempgrp_member_list_activity_destory(lv_obj_t *obj)
{
	lv_poc_member_list_cb_set_active(ACT_ID_POC_TMPGRP_MEMBER_LIST, false);

	activity_list = NULL;
	if(activity_win != NULL)
	{
		lv_mem_free(activity_win);
		activity_win = NULL;
	}

	lv_poc_build_tempgrp_member_list_obj = NULL;
	poc_build_tempgrp_member_list_activity = NULL;
}

void lv_poc_build_tempgrp_member_list_activity_open(lv_task_t *task)
{
	lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_TMPGRP_MEMBER_LIST,
		lv_poc_build_tempgrp_member_list_activity_create,
		lv_poc_build_tempgrp_member_list_activity_destory};

	strcpy((char *)lv_poc_build_tempgrp_member_list_title, (const char *)task->user_data);

	lv_poc_set_buildgroup_refr_is_complete(false);
	poc_build_tempgrp_member_list_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
	lv_poc_activity_set_signal_cb(poc_build_tempgrp_member_list_activity, lv_poc_build_tempgrp_signal_func);
	lv_poc_activity_set_design_cb(poc_build_tempgrp_member_list_activity, lv_poc_build_tempgrp_design_func);
	lv_poc_member_list_cb_set_active(ACT_ID_POC_TMPGRP_MEMBER_LIST, true);
	OSI_PRINTFI("[tmpgrp](%s)(%d):activity open", __func__, __LINE__);

	if(refresh_task_init == true)
	{
		if(tmpgrp_refresh_attr == NULL)
		{
			tmpgrp_refresh_attr = (lv_poc_tmpgrp_refresh_t *)lv_mem_alloc(sizeof(lv_poc_tmpgrp_refresh_t));
		}
		refresh_task_init = false;
		tmpgrp_refresh_attr->refresh_type = 0;
		tmpgrp_refresh_attr->task = NULL;
		tmpgrp_refresh_attr->mutex = NULL;
		tmpgrp_refresh_attr->user_data = NULL;
	}

	if(tmpgrp_refresh_attr->task == NULL)
	{
		tmpgrp_refresh_attr->task = lv_task_create(lv_poc_bulid_tempgroup_refresh_task, 500, LV_TASK_PRIO_MID, (void *)tmpgrp_refresh_attr);
	}

	if(tmpgrp_refresh_attr->mutex == NULL)
	{
		tmpgrp_refresh_attr->mutex = osiMutexCreate();
	}

	osiMutexLock(tmpgrp_refresh_attr->mutex);
	tmpgrp_refresh_attr->refresh_type = POC_REFRESH_TYPE_TMPGRP_MEMBER_LIST;
	lv_task_ready(tmpgrp_refresh_attr->task);
	osiMutexUnlock(tmpgrp_refresh_attr->mutex);
	OSI_PRINTFI("[tmpgrp](%s)(%d):launch refresh", __func__, __LINE__);
}

void lv_poc_build_tempgrp_member_list_open(IN lv_poc_oem_member_list *members, IN bool hide_offline)
{
	if(members == NULL)
	{
		OSI_PRINTFI("[build_tmpgrp_memberopen](%s)(%d):null", __func__, __LINE__);
		lv_poc_set_refr_error_info(true);
		return;
	}
	lv_poc_build_tempgrp_member_list_obj = (lv_poc_oem_member_list *)members;

	if(lv_poc_build_tempgrp_member_list_obj == NULL)
	{
		lv_poc_set_refr_error_info(true);
		OSI_PRINTFI("[build_tmpgrp_memberopen](%s)(%d):null", __func__, __LINE__);
		return;
	}

	lv_poc_build_tempgrp_member_list_obj->hide_offline = hide_offline;
	OSI_PRINTFI("[tmpgrp](%s)(%d):info get finish", __func__, __LINE__);
}

void lv_poc_build_tempgrp_member_list_refresh(lv_poc_oem_member_list *member_list_obj)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_tempgrp_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		OSI_PRINTFI("[member][refresh](%s)(%d):null", __func__, __LINE__);
		lv_poc_set_refr_error_info(true);
		return;
	}

	if(current_activity != poc_build_tempgrp_member_list_activity)
	{
		OSI_PRINTFI("[member][refresh](%s)(%d):cur act error", __func__, __LINE__);
		lv_poc_set_refr_error_info(true);
		return;
	}

	oem_list_element_t * p_cur = NULL;
	lv_obj_t * btn = NULL;
	lv_obj_t * btn_index[64];//assume member number is 64
	lv_coord_t btn_height = (build_tempgrp_member_list_display_area.y2 - build_tempgrp_member_list_display_area.y1)/LV_POC_LIST_COLUM_COUNT;

	int list_item_count = 0;
	char is_set_btn_selected = 0;

	if(activity_list == NULL)
	{
		lv_poc_set_refr_error_info(true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"空窗口列表", NULL);
		return;
	}
	lv_list_clean(activity_list);

	if(!(member_list_obj->online_list != NULL || member_list_obj->offline_list != NULL))
	{
		OSI_PRINTFI("[member][refresh](%s)(%d):null", __func__, __LINE__);
		lv_poc_set_refr_error_info(true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无成员列表", NULL);
		return;
	}

	p_cur = member_list_obj->online_list;
	while(p_cur)
	{
		btn = lv_list_add_btn(activity_list, &ic_member_online, p_cur->name);
		btn_index[list_item_count] = btn;
		list_item_count++;
		//lv_obj_set_click(btn, true);
		//lv_obj_set_event_cb(btn, lv_poc_build_tempgrp_member_list_prssed_btn_cb);
		p_cur->list_item = btn;
		lv_btn_set_fit(btn, LV_FIT_NONE);
		lv_obj_set_height(btn, btn_height);
		btn->user_data = p_cur->information;

		p_cur = p_cur->next;
	}

	p_cur = member_list_obj->offline_list;
	while(p_cur)
	{
		btn = lv_list_add_btn(activity_list, &ic_member_offline, p_cur->name);
		btn_index[list_item_count] = btn;
		list_item_count++;
		//lv_obj_set_click(btn, true);
		//lv_obj_set_event_cb(btn, lv_poc_build_tempgrp_member_list_prssed_btn_cb);
		p_cur->list_item = btn;
		lv_btn_set_fit(btn, LV_FIT_NONE);
		lv_obj_set_height(btn, btn_height);
		btn->user_data = p_cur->information;

		p_cur = p_cur->next;
	}

	//not find member,index 1
	if(0 == is_set_btn_selected)
	{
		lv_list_set_btn_selected(activity_list, btn_index[0]);
	}

	lv_poc_set_buildgroup_refr_is_complete(true);
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND, NULL);

	if(lv_poc_build_tempgrp_progress(POC_TMPGRP_READ) >= POC_TMPGRP_VIEW
		&& lv_poc_build_tempgrp_progress(POC_TMPGRP_READ) <= POC_TMPGRP_OPENMEMLIST)
	{
		lv_poc_build_tempgrp_progress(POC_TMPGRP_FINISH);
	}
}

void lv_poc_build_tempgrp_memberlist_activity_close(lv_task_t *task)
{
	int type = (int)task->user_data;

	if(type == POC_EXITGRP_PASSIVE
		&& lvPocGuiBndCom_get_listen_status())
	{
		return;
	}

	lv_poc_set_group_status(false);
	OSI_PRINTFI("[tmpgrp](%s)(%d):exit memberlist", __func__, __LINE__);
	poc_build_tempgrp_member_list_activity ? lv_poc_del_activity(poc_build_tempgrp_member_list_activity) : 0;

	if(lv_poc_build_tempgrp_progress(POC_TMPGRP_READ) == POC_TMPGRP_FINISH)
	{
		OSI_PRINTFI("[tmpgrp][multi-call](%s)(%d):exit multi call", __func__, __LINE__);
		poc_play_voice_one_time(LVPOCAUDIO_Type_Exit_Temp_Group, 50, false);
		type == POC_EXITGRP_INITIATIVE ? \
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP, NULL) : 0;
		lv_poc_build_tempgrp_progress(POC_TMPGRP_START);
	}
}

lv_poc_tmpgrp_t lv_poc_build_tempgrp_progress(lv_poc_tmpgrp_t status)
{
	if(status == POC_TMPGRP_READ)
	{
		return is_lv_poc_build_tempgrp;
	}
	is_lv_poc_build_tempgrp = status;
	return 0;
}

void lv_poc_bulid_tempgroup_refresh_task(lv_task_t *task)
{
	if(tmpgrp_refresh_attr->refresh_type == POC_REFRESH_TYPE_TMPGRP_START)
	{
		return;
	}

	switch(tmpgrp_refresh_attr->refresh_type)
	{
		case POC_REFRESH_TYPE_TMPGRP_MEMBER_SELECT:
		{
			lv_poc_build_tempgrp_refresh(NULL);
			break;
		}

		case POC_REFRESH_TYPE_TMPGRP_SUCCESS:
		{
			lv_poc_build_tempgrp_success();
			break;
		}

		case POC_REFRESH_TYPE_TMPGRP_MEMBER_LIST:
		{
			lv_poc_build_tempgrp_member_list_refresh(NULL);
			break;
		}

		default:
		{
			break;
		}
	}
	tmpgrp_refresh_attr->refresh_type = POC_REFRESH_TYPE_TMPGRP_START;
}


#ifdef __cplusplus
}
#endif
