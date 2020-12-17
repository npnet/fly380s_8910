﻿
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "lv_include/lv_poc.h"
#include <stdlib.h>
#include <string.h>
#include "guiIdtCom_api.h"
#include "lv_objx/lv_poc_obj/lv_poc_obj.h"


static lv_poc_member_list_t * lv_poc_member_list_obj;

static lv_obj_t * lv_poc_member_list_activity_create(lv_poc_display_t *display);

static void lv_poc_member_list_activity_destory(lv_obj_t *obj);

static void * lv_poc_member_list_list_create(lv_obj_t * parent, lv_area_t display_area);

static void lv_poc_member_list_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_member_list_prssed_btn_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t lv_poc_member_list_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool lv_poc_member_list_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static void lv_poc_member_list_get_list_cb(int msg_type);

static lv_obj_t * activity_list;

static lv_poc_win_t * activity_win;

static lv_area_t member_list_display_area;

static int8_t lv_poc_member_list_title[100] = {0};

//hightlight
static char prv_member_list_last_index_membername[64] = {0};

static const int8_t lv_poc_member_list_default_title[] = "成员列表";

static bool lv_poc_member_list_need_free_member_list = true;

lv_poc_activity_t * poc_member_list_activity;

static void * lv_poc_member_call_obj_information = NULL;

static lv_obj_t * lv_poc_member_call_obj = NULL;

static int lv_poc_member_list_get_member_type = -1;

static lv_obj_t * lv_poc_member_list_activity_create(lv_poc_display_t *display)
{
    activity_win = lv_poc_win_create(display, (const char *)lv_poc_member_list_title, lv_poc_member_list_list_create);
	lv_poc_notation_refresh();/*把弹框显示在最顶层*/
    return (lv_obj_t *)activity_win;
}

static void lv_poc_member_list_activity_destory(lv_obj_t *obj)
{
	OSI_LOGI(0, "[grouprefr]start destory memberlist");
	lv_poc_member_list_cb_set_active(ACT_ID_POC_MEMBER_LIST, false);
	lv_poc_member_call_obj = NULL;

	lv_poc_member_list_get_member_type = -1;
	activity_list = NULL;
	if(activity_win != NULL)
	{
		OSI_LOGI(0, "[grouprefr]free activity_win");
		lv_mem_free(activity_win);
		activity_win = NULL;
	}

	if(lv_poc_member_list_need_free_member_list == true && lv_poc_member_list_obj != NULL)
	{
		list_element_t * cur_p = lv_poc_member_list_obj->online_list;
		list_element_t * temp_p;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		cur_p = lv_poc_member_list_obj->offline_list;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		lv_mem_free(lv_poc_member_list_obj);
		lv_poc_member_list_need_free_member_list = false;
		OSI_LOGI(0, "[grouprefr]free memberlist node");
	}
	lv_poc_member_list_obj = NULL;
	poc_member_list_activity = NULL;
	lv_poc_member_call_obj_information = NULL;
	OSI_LOGI(0, "[grouprefr]finish destory memberlist");
}

static void * lv_poc_member_list_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    member_list_display_area.x1 = display_area.x1;
    member_list_display_area.x2 = display_area.x2;
    member_list_display_area.y1 = display_area.y1;
    member_list_display_area.y2 = display_area.y2;
    activity_list = lv_poc_list_create(parent, NULL, display_area, lv_poc_member_list_list_config);
    return (void *)activity_list;
}

static void lv_poc_member_list_list_config(lv_obj_t * list, lv_area_t list_area)
{
#if 0
    lv_obj_t *btn;
    lv_obj_t *cb;
    lv_obj_t *btn_label;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/3;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
#endif
}

void prv_lv_poc_member_list_change_to_online(lv_task_t *task)
{
	lv_obj_t *btn = (lv_obj_t *)task->user_data;
	if(btn == NULL)
	{
		return;
	}

	lv_obj_t *btn_img = lv_list_get_btn_img(btn);
	if(btn_img == NULL)
	{
		return;
	}

	lv_img_set_src(btn_img, &ic_member_online);
}

void prv_lv_poc_member_list_change_to_offline(lv_task_t *task)
{
	lv_obj_t *btn = (lv_obj_t *)task->user_data;
	if(btn == NULL)
	{
		return;
	}

	lv_obj_t *btn_img = lv_list_get_btn_img(btn);
	if(btn_img == NULL)
	{
		return;
	}

	lv_img_set_src(btn_img, &ic_member_offline);
}

static void lv_poc_member_list_get_member_status_cb(int status)
{
	if(lv_poc_member_call_obj != NULL || lv_poc_member_call_obj_information != NULL)
	{
		if(status == 1)
		{
			lv_poc_member_list_set_state(lv_poc_member_list_obj, lv_poc_get_member_name(lv_poc_member_call_obj_information), lv_poc_member_call_obj_information, true);
			lv_task_t *once_task = lv_task_create(prv_lv_poc_member_list_change_to_online, 10, LV_TASK_PRIO_HIGH, lv_poc_member_call_obj);
			lv_task_once(once_task);
			lv_poc_activity_func_cb_set.member_call_open(lv_poc_member_call_obj_information);
			lv_poc_member_list_set_hightlight_index();
		}
		else
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Offline_Member, 50, true);
			lv_poc_member_list_set_state(lv_poc_member_list_obj, lv_poc_get_member_name(lv_poc_member_call_obj_information), lv_poc_member_call_obj_information, false);
			lv_task_t *once_task = lv_task_create(prv_lv_poc_member_list_change_to_offline, 10, LV_TASK_PRIO_HIGH, lv_poc_member_call_obj);
			lv_task_once(once_task);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"对方不在线", NULL);
		}
	}
	lv_poc_member_call_obj_information = NULL;
	lv_poc_member_call_obj = NULL;
}

static void lv_poc_member_list_prssed_btn_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
		if(lv_poc_member_call_obj_information != NULL || lv_poc_member_call_obj != NULL)
		{
			return;
		}

		lv_poc_member_call_obj_information = obj->user_data;

		/*获取该列表单呼对象是不是自己*/
		lv_obj_t * sigal_member_list;
		sigal_member_list = lv_list_get_btn_selected(activity_list);
		if(NULL != strstr(lv_list_get_btn_text(sigal_member_list),"我"))//如果是自己
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Unable_To_Call_Yourself, 50, true);
			lv_poc_member_call_obj_information = NULL;
			return;
		}
		/*获取结束*/

		lv_poc_member_call_obj = obj;
		if(!lv_poc_get_member_status(lv_poc_member_call_obj_information, lv_poc_member_list_get_member_status_cb))
		{
			lv_poc_member_call_obj = NULL;
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"错误", NULL);
			lv_poc_member_call_obj_information = NULL;
		}
	}
}

static lv_res_t lv_poc_member_list_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	OSI_LOGI(0, "[poc][signal][member list] sign <- %d  param <- 0x%p\n", sign, param);
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
					if(lv_poc_is_memberlist_refr_complete()
						|| lv_poc_get_refr_error_info())
					{
						lv_poc_set_group_status(false);
						lv_poc_member_list_set_hightlight_index();
						OSI_LOGI(0, "[grouprefr]exit memberlist\n");
						lv_poc_del_activity(poc_member_list_activity);
					}
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			if(lv_poc_member_list_obj != NULL && current_activity == poc_member_list_activity)
			{
				lv_poc_refr_task_once(lv_poc_member_list_refresh, LVPOCLISTIDTCOM_LIST_PERIOD_10, LV_TASK_PRIO_HIGH);
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

static bool lv_poc_member_list_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}

//处理成员列表，GUI显示
static void lv_poc_member_list_get_list_cb(int msg_type)
{
	if(poc_member_list_activity == NULL)
	{
		return;
	}
	//add your information
	if(msg_type==1)//显示
	{
		lv_poc_refr_task_once(lv_poc_member_list_refresh, LVPOCLISTIDTCOM_LIST_PERIOD_10, LV_TASK_PRIO_HIGH);
	}
	else
	{
		lv_poc_set_refr_error_info(true);
		poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Member, 50, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
	}

}

void lv_poc_member_list_open(IN char * title, IN lv_poc_member_list_t *members, IN bool hide_offline)
{
    if(lv_poc_member_list_obj != NULL)
    {
    	return;
    }

    if(members == NULL)
    {
		if(poc_member_list_activity != NULL)
		{
			lv_poc_set_refr_error_info(true);
			return;
		}

		lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_MEMBER_LIST,
    		lv_poc_member_list_activity_create,
			lv_poc_member_list_activity_destory};

		lv_poc_set_refr_error_info(false);
		lv_poc_set_memberlist_refr_is_complete(false);
	    lv_poc_member_list_obj = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));
        lv_poc_member_list_obj->offline_list = NULL;
        lv_poc_member_list_obj->offline_number = 0;
        lv_poc_member_list_obj->online_list = NULL;
        lv_poc_member_list_obj->online_number = 0;
        lv_poc_member_list_need_free_member_list = true;
        lv_poc_member_list_get_member_type = 1;

	    strcpy((char *)lv_poc_member_list_title, (const char *)lv_poc_member_list_default_title);

		poc_member_list_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
	    lv_poc_activity_set_signal_cb(poc_member_list_activity, lv_poc_member_list_signal_func);
	    lv_poc_activity_set_design_cb(poc_member_list_activity, lv_poc_member_list_design_func);
	    lv_poc_member_list_cb_set_active(ACT_ID_POC_MEMBER_LIST, true);
    }
    else
    {
	    lv_poc_member_list_obj = (lv_poc_member_list_t *)members;
	    lv_poc_member_list_need_free_member_list = false;
	    lv_poc_member_list_get_member_type = 2;
    }

    if(lv_poc_member_list_obj == NULL)
    {
		lv_poc_set_refr_error_info(true);
	    return;
    }

    lv_poc_member_list_obj->hide_offline = hide_offline;
    if(members == NULL)
    {
		OSI_LOGI(0, "[member]member null to get\n");
		if(!lv_poc_get_member_list(NULL, lv_poc_member_list_obj,1,lv_poc_member_list_get_list_cb))
		{
			lv_poc_set_refr_error_info(true);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Member, 50, true);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
		}
    }
    else
    {
		OSI_LOGI(0, "[member]have member to refr\n");
		lv_poc_refr_task_once(lv_poc_member_list_refresh, LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH);
	}
}

lv_poc_status_t lv_poc_member_list_add(lv_poc_member_list_t *member_list_obj, const char * name, bool is_online, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status != POC_MEMBER_NONENTITY)
    {
        return status;
    }

    list_element_t * new_element = (list_element_t *)lv_mem_alloc(sizeof(list_element_t));
    if(NULL == new_element)
    {
        return POC_OPERATE_FAILD;
    }
    memset(new_element, 0, sizeof(list_element_t));
    strcpy(new_element->name, name);
    new_element->information = information;

    if(is_online)
    {
        if(member_list_obj->online_list)
        {
            new_element->next = member_list_obj->online_list;
        }
        else
        {
            new_element->next = NULL;
        }
        member_list_obj->online_list = new_element;
        member_list_obj->online_number = member_list_obj->online_number + 1;
    }
    else
    {
        if(member_list_obj->offline_list)
        {
            new_element->next = member_list_obj->offline_list;
        }
        else
        {
            new_element->next = NULL;
        }
        member_list_obj->offline_list = new_element;
        member_list_obj->offline_number = member_list_obj->offline_number + 1;
    }

    //if(member_list != NULL && current_activity == poc_member_list_activity)
    //{
	//	lv_poc_member_list_refresh();
    //}
    return POC_OPERATE_SECCESS;
}

void lv_poc_member_list_remove(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    if(member_list_obj->online_list == NULL && member_list_obj->offline_list == NULL)
    {
        return;
    }

    if(NULL != member_list_obj->online_list)
    {
        p_cur = member_list_obj->online_list;
        if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
        	member_list_obj->online_list = p_cur->next;
            lv_obj_del(p_cur->list_item);
            lv_mem_free(p_cur);
            member_list_obj->online_number = member_list_obj->online_number - 1;
            return;
        }
        p_prv = p_cur;
		if(p_cur != NULL)
        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
            {
                p_prv->next = p_cur->next;
                lv_obj_del(p_cur->list_item);
                lv_mem_free(p_cur);
                member_list_obj->online_number = member_list_obj->online_number - 1;
                return;
            }

            p_prv = p_cur;
            p_cur = p_cur->next;
        }
    }

    if(NULL != member_list_obj->offline_list)
    {
        p_cur = member_list_obj->offline_list;
        if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
        	member_list_obj->offline_list = p_cur->next;
            lv_obj_del(p_cur->list_item);
            lv_mem_free(p_cur);
            member_list_obj->offline_number = member_list_obj->offline_number - 1;
            return;
        }
        p_prv = p_cur;
		if(p_cur != NULL)
        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
            {
                p_prv->next = p_cur->next;
                lv_obj_del(p_cur->list_item);
                lv_mem_free(p_cur);
                member_list_obj->offline_number = member_list_obj->offline_number - 1;
                return;
            }

            p_prv = p_cur;
            p_cur = p_cur->next;
        }
    }
}

void lv_poc_member_list_clear(lv_poc_member_list_t *member_list_obj)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

	list_element_t * cur_p = member_list_obj->online_list;
	list_element_t * temp_p;
	while(cur_p != NULL)
	{
		temp_p = cur_p;
		cur_p =cur_p->next;
		//lv_obj_del(temp_p->list_item);
		lv_mem_free(temp_p);
	}
	member_list_obj->online_list = NULL;

	cur_p = member_list_obj->offline_list;
	while(cur_p != NULL)
	{
		temp_p = cur_p;
		cur_p =cur_p->next;
		//lv_obj_del(temp_p->list_item);
		lv_mem_free(temp_p);
	}
	member_list_obj->offline_list = NULL;
}

int lv_poc_member_list_get_information(lv_poc_member_list_t *member_list_obj, const char * name, void *** information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return 0;
	}

    list_element_t * p_cur = member_list_obj->online_list;
    *information = (void **)lv_mem_alloc(sizeof(void *) * 10);
    unsigned char number = 0;
    unsigned char current_size = 10;

    while(p_cur)
    {
        if(0 == strcmp(p_cur->name, name))
        {
            if(number >= current_size)
            {
                *information = (void **)lv_mem_realloc(*information, sizeof((*information)[0]) * (current_size + 10));
                current_size = current_size + 10;
            }
            (*information)[number] = p_cur->information;
            number = number + 1;
        }
        p_cur = p_cur->next;
    }

    p_cur = member_list_obj->offline_list;
    while(p_cur)
    {
        if(0 == strcmp(p_cur->name, name))
        {
            if(number >= current_size)
            {
                *information = (void **)lv_mem_realloc(*information, sizeof((*information)[0]) * (current_size + 10));
                current_size = current_size + 10;
            }
            (*information)[number] = p_cur->information;
            number = number + 1;
        }
        p_cur = p_cur->next;
    }
    return number;

}

void lv_poc_member_list_refresh(lv_task_t * task)
{
	lv_poc_member_list_t *member_list_obj = NULL;

	member_list_obj = (lv_poc_member_list_t *)task->user_data;

	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(current_activity != poc_member_list_activity
		|| member_list_obj == NULL)
	{
		lv_poc_set_refr_error_info(true);
		return;
	}

    list_element_t * p_cur = NULL;
    lv_obj_t * btn = NULL;
	lv_obj_t * btn_index[64];//assume member number is 64
    lv_coord_t btn_height = (member_list_display_area.y2 - member_list_display_area.y1)/LV_POC_LIST_COLUM_COUNT;
	int list_item_count = 0;
	char is_set_btn_selected = 0;

    lv_list_clean(activity_list);

    if(!(member_list_obj->online_list != NULL || member_list_obj->offline_list != NULL))
    {
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
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_member_list_prssed_btn_cb);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn->user_data = p_cur->information;
		//set member index
		if(NULL != prv_member_list_last_index_membername
			&& NULL != strstr(p_cur->name, prv_member_list_last_index_membername)
			&& is_set_btn_selected == 0)
		{
			lv_list_set_btn_selected(activity_list, btn);
			is_set_btn_selected = 1;
		}

		p_cur = p_cur->next;
    }

    p_cur = member_list_obj->offline_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_member_offline, p_cur->name);
        btn_index[list_item_count] = btn;
		list_item_count++;
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_member_list_prssed_btn_cb);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn->user_data = p_cur->information;
		//set member index
        if(NULL != prv_member_list_last_index_membername
			&& NULL != strstr(p_cur->name, prv_member_list_last_index_membername)
			&& is_set_btn_selected == 0)
		{
			lv_list_set_btn_selected(activity_list, btn);
			is_set_btn_selected = 1;
		}

		p_cur = p_cur->next;
    }
	//not find member,index 1
	if(0 == is_set_btn_selected)
	{
		lv_list_set_btn_selected(activity_list, btn_index[0]);
	}
	lv_poc_set_memberlist_refr_is_complete(true);
}

void lv_poc_member_list_refresh_with_data(lv_poc_member_list_t *member_list_obj)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

	if(current_activity != poc_member_list_activity)
	{
		OSI_LOGI(0, "[grouprefr](%d):The current memberlist has not been refreshed", __LINE__);
		lv_poc_set_group_refr(true);//记录有信息待刷新
		return;
	}

	if(lv_poc_is_inside_group())//若当前设备在某个群组里,禁止群组的所有更新(包括添组、删组、组信息更新)
	{
		lv_poc_set_group_refr(true);//记录有信息待刷新
		return;
	}

	lv_poc_member_list_set_hightlight_index();
	extern lv_poc_group_info_t *lv_poc_group_list_get_member_list_info;
	if(lv_poc_member_list_get_member_type == 1)/*成员列表为空*/
	{
		lv_poc_member_list_clear(member_list_obj);
		lv_poc_get_member_list(NULL, member_list_obj, 1,lv_poc_member_list_get_list_cb);
	}
	else if(lv_poc_member_list_get_member_type == 2 && lv_poc_group_list_get_member_list_info != NULL)
	{
		lv_poc_member_list_clear(member_list_obj);
		lv_poc_get_member_list(lv_poc_group_list_get_member_list_info, member_list_obj, 1,lv_poc_member_list_get_list_cb);
	}
	//one
   	lv_poc_member_list_get_member_type = -1;
}

lv_poc_status_t lv_poc_member_list_move_top(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = member_list_obj->online_list;
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            p_prv->next = p_cur->next;
            p_cur->next = member_list_obj->online_list;
            member_list_obj->online_list = p_cur;
            return POC_OPERATE_SECCESS;
        }
        p_prv = p_cur;
        p_cur = p_cur->next;
    }

    p_cur = member_list_obj->offline_list;
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            p_prv->next = p_cur->next;
            p_cur->next = member_list_obj->offline_list;
            member_list_obj->offline_list = p_cur;
            return POC_OPERATE_SECCESS;
        }
        p_prv = p_cur;
        p_cur = p_cur->next;
    }

    return POC_UNKNOWN_FAULT;
}

lv_poc_status_t lv_poc_member_list_move_bottom(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_scr;
    bool is_find = false;
    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = member_list_obj->online_list;
    p_scr = p_cur;
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        is_find = true;
        p_scr = p_cur;
        member_list_obj->online_list = p_cur->next;
    }
    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(false == is_find && (MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL)))
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

    p_cur = member_list_obj->offline_list;
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        is_find = true;
        p_scr = p_cur;
        member_list_obj->offline_list = p_cur->next;
    }
    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(false == is_find && (MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL)))
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
        p_prv->next = p_scr;
        p_scr->next = NULL;
        return POC_OPERATE_SECCESS;
    }

    return POC_UNKNOWN_FAULT;
}

lv_poc_status_t lv_poc_member_list_move_up(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_prv_prv;
    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = member_list_obj->online_list;
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        p_prv->next = p_cur->next;
        p_cur->next = p_prv;
        member_list_obj->online_list = p_cur;
        return POC_OPERATE_SECCESS;
    }

    p_prv_prv = p_prv;
    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
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

    p_cur = member_list_obj->offline_list;
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        p_prv->next = p_cur->next;
        p_cur->next = p_prv;
        member_list_obj->offline_list = p_cur;
        return POC_OPERATE_SECCESS;
    }

    p_prv_prv = p_prv;
    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
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

lv_poc_status_t lv_poc_member_list_move_down(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_prv_prv;
    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = member_list_obj->online_list;/*p_cur != NULL 防止内存泄露*/
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        if(NULL != p_cur->next)
        {
            p_cur = p_cur->next;
            member_list_obj->online_list->next = p_cur->next;
            p_cur->next = member_list_obj->online_list;
            member_list_obj->online_list = p_cur;
        }
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
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

    p_cur = member_list_obj->offline_list;
    if(p_cur != NULL && MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        if(NULL != p_cur->next)
        {
            p_cur = p_cur->next;
            member_list_obj->offline_list->next = p_cur->next;
            p_cur->next = member_list_obj->offline_list;
            member_list_obj->offline_list = p_cur;
        }
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
	if(p_cur != NULL)
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
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

lv_poc_status_t lv_poc_member_list_set_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information, bool is_online)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur = NULL;
	list_element_t * p_temp = NULL;

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

            return POC_OPERATE_SECCESS;
        }
		if(p_cur == NULL)/*solve haven't offline on group*/
        {
            return POC_UNKNOWN_FAULT;
        }
        while(p_cur->next)
        {
            if(MEMBER_EQUATION((void *)p_cur->next->name, (void *)name, (void *)p_cur->next->information, (void *)information, NULL))
            {
				p_temp = p_cur->next;
            	p_cur->next = p_temp->next;
                p_temp->next = member_list_obj->online_list;
                member_list_obj->online_list = p_temp;

                return POC_OPERATE_SECCESS;
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

            return POC_OPERATE_SECCESS;
        }
		if(p_cur == NULL)
        {
            return POC_UNKNOWN_FAULT;
        }
        while(p_cur->next)
        {
            if(MEMBER_EQUATION((void *)p_cur->next->name, (void *)name, (void *)p_cur->next->information, (void *)information, NULL))
            {
            	p_temp = p_cur->next;
            	p_cur->next = p_temp->next;
                p_temp->next = member_list_obj->offline_list;
                member_list_obj->offline_list = p_temp;

                return POC_OPERATE_SECCESS;
            }
            p_cur = p_cur->next;
        }
    }

    return POC_UNKNOWN_FAULT;
}

lv_poc_status_t lv_poc_member_list_is_exists(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    if(NULL == name)
    {
        return POC_OPERATE_FAILD;
    }

    p_cur = member_list_obj->online_list;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            return POC_MEMBER_EXISTS;
        }
        p_cur = p_cur->next;
    }

    p_cur = member_list_obj->offline_list;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            return POC_MEMBER_EXISTS;
        }
        p_cur = p_cur->next;
    }

    return POC_MEMBER_NONENTITY;
}

lv_poc_status_t lv_poc_member_list_get_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
        return POC_OPERATE_FAILD;
	}

    list_element_t * p_cur;
    if(NULL == name)
    {
        return POC_OPERATE_FAILD;
    }

    p_cur = member_list_obj->online_list;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            return POC_MEMBER_ONLINE;
        }
        p_cur = p_cur->next;
    }

    p_cur = member_list_obj->offline_list;
    while(p_cur)
    {
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
            return POC_MEMBER_OFFLINE;
        }
        p_cur = p_cur->next;
    }

    return POC_MEMBER_NONENTITY;
}

void lv_poc_member_list_set_hightlight_index(void)
{
	lv_obj_t *current_btn = lv_list_get_btn_selected(activity_list);

	if(current_btn != NULL)
	{
		strcpy(prv_member_list_last_index_membername, lv_list_get_btn_text(current_btn));
	}
}

void lv_poc_memberlist_activity_open(lv_task_t * task)
{
	lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_MEMBER_LIST,
		lv_poc_member_list_activity_create,
		lv_poc_member_list_activity_destory};

	strcpy((char *)lv_poc_member_list_title, (const char *)task->user_data);

	lv_poc_set_refr_error_info(false);
	lv_poc_set_memberlist_refr_is_complete(false);
	poc_member_list_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
	lv_poc_activity_set_signal_cb(poc_member_list_activity, lv_poc_member_list_signal_func);
	lv_poc_activity_set_design_cb(poc_member_list_activity, lv_poc_member_list_design_func);
	lv_poc_member_list_cb_set_active(ACT_ID_POC_MEMBER_LIST, true);

}
#ifdef __cplusplus
}
#endif

