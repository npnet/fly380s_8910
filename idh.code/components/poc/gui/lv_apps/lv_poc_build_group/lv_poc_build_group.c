
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

static lv_obj_t * lv_poc_build_group_activity_create(lv_poc_display_t *display);

static void lv_poc_build_group_activity_destory(lv_obj_t *obj);

static void * lv_poc_build_group_list_create(lv_obj_t * parent, lv_area_t display_area);

static void lv_poc_build_group_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_build_group_pressed_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t lv_poc_build_group_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool lv_poc_build_group_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static void lv_poc_build_group_success_refresh(lv_task_t *task);

static void lv_poc_build_group_is_exist(lv_task_t *task);

static lv_obj_t * activity_list;

static lv_poc_build_group_item_info_t * lv_poc_build_group_info = NULL;

static int32_t lv_poc_build_group_selected_num = 0;

static lv_poc_member_info_t *lv_poc_build_group_selected_members = NULL;

static lv_poc_win_t * activity_win;

static lv_area_t build_group_display_area;

static lv_poc_member_list_t *lv_poc_build_group_member_list;

lv_poc_activity_t * poc_build_group_activity;

static char * lv_poc_build_group_success_text = "创建群组成功";

static char * lv_poc_build_group_failed_text = "创建群组失败";

static char * lv_poc_build_group_few_member_text1 = "成员数量不";

static char * lv_poc_build_group_few_member_text2 = "能少于两人";

static char * lv_poc_build_group_error_text = "错误";

static lv_obj_t * lv_poc_build_group_activity_create(lv_poc_display_t *display)
{
#if 1
    activity_win = lv_poc_win_create(display, "成员列表", lv_poc_build_group_list_create);
#endif
	lv_poc_notation_refresh();/*把弹框显示在最顶层*/
    return (lv_obj_t *)activity_win;
}

static void lv_poc_build_group_activity_destory(lv_obj_t *obj)
{
	lv_poc_member_list_cb_set_active(ACT_ID_POC_MAKE_GROUP, false);
    if(lv_poc_build_group_member_list != NULL)
    {
		list_element_t * cur_p = lv_poc_build_group_member_list->online_list;
		list_element_t * temp_p;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		cur_p = lv_poc_build_group_member_list->offline_list;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		lv_mem_free(lv_poc_build_group_member_list);
    }
    lv_poc_build_group_member_list = NULL;

    if(lv_poc_build_group_info != NULL)
    {
	    lv_mem_free(lv_poc_build_group_info);
    }
    lv_poc_build_group_info = NULL;
    lv_poc_build_group_selected_num = 0;

    if(lv_poc_build_group_selected_members != NULL)
    {
	    lv_mem_free(lv_poc_build_group_selected_members);
    }
    lv_poc_build_group_selected_members = NULL;

    poc_build_group_activity = NULL;
}

static void * lv_poc_build_group_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    build_group_display_area.x1 = display_area.x1;
    build_group_display_area.x2 = display_area.x2;
    build_group_display_area.y1 = display_area.y1;
    build_group_display_area.y2 = display_area.y2;
    activity_list = lv_poc_list_create(parent, NULL, display_area, lv_poc_build_group_list_config);
    return (void *)activity_list;
}

static void lv_poc_build_group_list_config(lv_obj_t * list, lv_area_t list_area)
{
}

static
void lv_poc_build_group_exit(lv_task_t *task)
{
	lv_poc_del_activity(poc_build_group_activity);
}

static void lv_poc_build_group_new_group_cb_refresh(lv_task_t *task)
{
	int result_type = (int)task->user_data;

	if(result_type == 1)
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Success_Build_Group, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)lv_poc_build_group_success_text, NULL);
	}
	else
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_To_Build_Group, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)lv_poc_build_group_failed_text, NULL);
	}
	//delay
	lv_poc_refr_task_once(lv_poc_build_group_exit, LVPOCLISTIDTCOM_LIST_PERIOD_500, LV_TASK_PRIO_HIGH);
}

static void lv_poc_build_group_new_group_cb(int result_type)
{
	lv_poc_refr_func_ui(lv_poc_build_group_new_group_cb_refresh, LVPOCLISTIDTCOM_LIST_PERIOD_10, LV_TASK_PRIO_HIGH, (void *)result_type);
}

static bool lv_poc_build_group_operator(lv_poc_build_group_item_info_t * info, int32_t info_num, int32_t selected_num)
{
	if(info == NULL || info_num < selected_num)
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,
			(const uint8_t *)lv_poc_build_group_error_text,
			NULL);
		lv_poc_del_activity(poc_build_group_activity);
		return false;
	}

	if(selected_num < 2)
	{
		if(selected_num == 1)
		{
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,
				(const uint8_t *)lv_poc_build_group_few_member_text1,
				(const uint8_t *)lv_poc_build_group_few_member_text2);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_To_Build_Group_Due_To_Less_Than_Two_People, 50, true);
		}
		lv_poc_del_activity(poc_build_group_activity);
		return false;
	}

	lv_poc_build_group_selected_members = (lv_poc_member_info_t *)lv_mem_alloc(sizeof(lv_poc_member_info_t) * selected_num);
	memset(lv_poc_build_group_selected_members, 0, sizeof(lv_poc_member_info_t) * selected_num);
	if(lv_poc_build_group_selected_members == NULL)
	{
		return false;
	}

	lv_poc_build_group_item_info_t * p_info = info;
	void ** p_sel = lv_poc_build_group_selected_members;
	int32_t real_num = 0;
	for(int i = 0; i < info_num; i++)
	{
		if(p_info->item_information == NULL)
		{
			return false;
		}

		if(p_info->is_selected == true)
		{
			*p_sel = p_info->item_information;
			p_sel++;
			real_num++;
		}
		p_info++;
	}

	if(lv_poc_build_group_selected_members[0] == 0 || real_num < 1)
	{
		return false;
	}

	return lv_poc_build_new_group(lv_poc_build_group_selected_members, real_num, lv_poc_build_group_new_group_cb);
}

static void lv_poc_build_group_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
		if(lv_poc_build_group_info == NULL)
		{
			return;
		}

		lv_poc_build_group_item_info_t * p_info = (lv_poc_build_group_item_info_t *)obj->user_data;
		if(p_info == NULL || p_info->is_self == true)
		{
			return;
		}

		p_info->is_selected = !p_info->is_selected;
	    if(p_info->is_selected)
	    {
	        lv_btn_set_state((lv_obj_t *)p_info->checkbox, LV_BTN_STATE_TGL_PR);
	        lv_poc_build_group_selected_num++;
	    }
	    else
	    {
	        lv_btn_set_state((lv_obj_t *)p_info->checkbox, LV_BTN_STATE_REL);
	        lv_poc_build_group_selected_num--;
	    }
    }
}

static lv_res_t lv_poc_build_group_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
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
						if(lvPocGuiIdtCom_get_current_exist_selfgroup() == 2)
						{
							lv_task_t *task = lv_task_create(lv_poc_build_group_is_exist, 200, LV_TASK_PRIO_HIGH, NULL);
							lv_task_once(task);

							lv_poc_del_activity(poc_build_group_activity);
						}
						else
						{
							lv_poc_refr_task_once(lv_poc_build_group_success_refresh, LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH);
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

static bool lv_poc_build_group_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}

static void lv_poc_build_group_get_list_cb(int msg_type)
{
    if(poc_build_group_activity == NULL)
    {
    	return;
    }

	if(msg_type==1)//显示
	{
		lv_poc_refr_task_once(lv_poc_build_group_refresh, LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH);
	}
	else
	{
		lv_poc_set_refr_error_info(true);
		poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Member, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
	}
}

void lv_poc_build_group_open(void)
{
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_MAKE_GROUP,
															lv_poc_build_group_activity_create,
															lv_poc_build_group_activity_destory};

    if(poc_build_group_activity != NULL)
    {
    	return;
    }

	if(lv_poc_build_group_member_list == NULL)
	{
	    lv_poc_build_group_member_list = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));
	    if(lv_poc_build_group_member_list == NULL)
	    {
		    return;
	    }
	}

	if(lv_poc_build_group_member_list != NULL)
	{
		memset(lv_poc_build_group_member_list, 0, sizeof(lv_poc_member_list_t));
	}

	lv_poc_set_buildgroup_refr_is_complete(false);
    poc_build_group_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_member_list_cb_set_active(ACT_ID_POC_MAKE_GROUP, true);
    lv_poc_activity_set_signal_cb(poc_build_group_activity, lv_poc_build_group_signal_func);
    lv_poc_activity_set_design_cb(poc_build_group_activity, lv_poc_build_group_design_func);

	if(!lv_poc_get_member_list(NULL, lv_poc_build_group_member_list, 1, lv_poc_build_group_get_list_cb))
	{
		lv_poc_set_refr_error_info(true);
		poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Member, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
	}
}

lv_poc_status_t lv_poc_build_group_add(lv_poc_member_list_t *member_list_obj, const char * name, bool is_online, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    return lv_poc_member_list_add(member_list_obj, name, is_online, information);
}

void lv_poc_build_group_remove(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

	lv_poc_member_list_remove(member_list_obj, name, information);
}

void lv_poc_build_group_clear(lv_poc_member_list_t *member_list_obj)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

	lv_poc_member_list_clear(member_list_obj);
}

int lv_poc_build_group_get_information(lv_poc_member_list_t *member_list_obj, const char * name, void *** information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
		return 0;
	}

	return lv_poc_member_list_get_information(member_list_obj, name, information);
}

void lv_poc_build_group_refresh(lv_task_t * task)
{
	lv_poc_member_list_t *member_list_obj = NULL;

	member_list_obj = (lv_poc_member_list_t *)task->user_data;

	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(current_activity != poc_build_group_activity
		|| member_list_obj == NULL)
	{
		lv_poc_set_refr_error_info(true);
		return;
	}

    list_element_t * p_cur = NULL;
    lv_obj_t * btn;
    lv_obj_t * btn_checkbox = NULL;
    lv_obj_t * btn_label = NULL;
    lv_poc_build_group_item_info_t * p_info = NULL;
    lv_coord_t btn_height = (build_group_display_area.y2 - build_group_display_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (build_group_display_area.x2 - build_group_display_area.x1);
    char member_list_is_first_item = 1;
    lv_list_clean(activity_list);

    if(!(member_list_obj->online_list != NULL || member_list_obj->offline_list != NULL))
    {
		lv_poc_set_refr_error_info(true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无成员列表", NULL);
	    return;
    }

    if(lv_poc_build_group_info != NULL)
    {
	    lv_mem_free(lv_poc_build_group_info);
	    lv_poc_build_group_info = NULL;
    }

    lv_poc_build_group_info = (lv_poc_build_group_item_info_t *)lv_mem_alloc(sizeof(lv_poc_build_group_item_info_t) * (member_list_obj->online_number + member_list_obj->offline_number));

	if(lv_poc_build_group_info == NULL)
	{
		lv_poc_set_refr_error_info(true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"刷新失败", NULL);
	    return;
	}
	lv_poc_build_group_selected_num = 0;

	p_info = lv_poc_build_group_info;

    p_cur = member_list_obj->online_list;

    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_member_online, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_build_group_pressed_cb);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);

        btn_label = lv_list_get_btn_label(btn);
        lv_label_set_long_mode(btn_label, LV_LABEL_LONG_SROLL);

        btn_checkbox = lv_cb_create(btn, NULL);
		lv_cb_set_text(btn_checkbox, "");
		lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(btn_checkbox) - ic_member_online.header.w - 9);/*此处修改图标位置*/
		lv_obj_align(btn_checkbox, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

		//把自己选上
		if(NULL != strstr(lv_list_get_btn_text(btn),"我"))//如果是自己
		{
			p_info->is_self = true;
			p_info->is_selected = true;
			lv_btn_set_state(btn_checkbox, LV_BTN_STATE_TGL_PR);

			lv_poc_build_group_selected_num++;
		}
		else
		{
			p_info->is_self = false;
			p_info->is_selected = false;
			lv_btn_set_state(btn_checkbox, LV_BTN_STATE_REL);
		}

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
        lv_obj_set_event_cb(btn, lv_poc_build_group_pressed_cb);
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
	lv_poc_set_buildgroup_refr_is_complete(true);
}

void lv_poc_build_group_refresh_with_data(lv_poc_member_list_t *member_list_obj)
{
	if(lv_poc_opt_refr_status(false) != LVPOCUNREFOPTIDTCOM_SIGNAL_NUMBLE_STATUS)/*防止一些界面刷新数据混乱导致死机问题*/
	{
		lv_poc_set_group_refr(true);//记录有信息待刷新
		return;
	}

	OSI_LOGI(0, "[song]build grouplist refreshing");

	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
		return;
	}
	lv_poc_build_group_clear(lv_poc_build_group_member_list);

	lv_poc_get_member_list(NULL, member_list_obj, 1, lv_poc_build_group_get_list_cb);
}

lv_poc_status_t lv_poc_build_group_move_top(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_move_top(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_build_group_move_bottom(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_move_bottom(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_build_group_move_up(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_move_up(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_build_group_move_down(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_move_down(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_build_group_set_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information, bool is_online)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur = NULL;
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
            goto LV_POC_BUILD_GROUP_SET_STATE_SUCCESS;
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
                goto LV_POC_BUILD_GROUP_SET_STATE_SUCCESS;
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
            goto LV_POC_BUILD_GROUP_SET_STATE_SUCCESS;
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
                goto LV_POC_BUILD_GROUP_SET_STATE_SUCCESS;
            }
            p_cur = p_cur->next;
        }
    }

    return POC_UNKNOWN_FAULT;

    LV_POC_BUILD_GROUP_SET_STATE_SUCCESS:
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

lv_poc_status_t lv_poc_build_group_is_exists(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_is_exists(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_build_group_get_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_get_state(member_list_obj, name, information);
}

static
void lv_poc_build_group_success_refresh(lv_task_t *task)
{
	lv_poc_build_group_operator(lv_poc_build_group_info,
							lv_poc_build_group_member_list->offline_number + lv_poc_build_group_member_list->online_number,
							lv_poc_build_group_selected_num);
}

static
void lv_poc_build_group_is_exist(lv_task_t *task)
{
	poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Due_To_Already_Exist_Selfgroup, 50, false);
	lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)"已有自建群组", (const uint8_t *)"");
}

#ifdef __cplusplus
}
#endif
