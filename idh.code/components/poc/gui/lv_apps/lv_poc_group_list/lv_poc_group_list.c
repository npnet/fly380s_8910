#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include <stdio.h>
#include <string.h>

#define CURRENR_GROUP_NAME_EXTERN 20

static lv_poc_group_list_t * group_list;

static lv_poc_member_list_t * member_list;

static lv_obj_t * lv_poc_group_list_activity_create(lv_poc_display_t *display);

static void lv_poc_group_list_activity_destory(lv_obj_t *obj);

static void lv_poc_group_list_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_group_list_press_btn_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t lv_poc_group_list_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool lv_poc_group_list_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static void lv_poc_get_group_list_cb(int result_type);

static void lv_poc_group_list_locked_activity_destory(void);

static void lv_poc_group_list_locked_activity_open(void);

static lv_res_t lv_poc_lockgroup_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static lv_obj_t * activity_list;

static lv_area_t display_area;

lv_poc_activity_t * poc_group_list_activity;

char group_member_list_is_open = 0;

/*锁组or解锁*/
char group_list_locked_unlock = 0;


static char lv_poc_group_member_list_title[LIST_ELEMENT_NAME_MAX_LENGTH];

static char lv_poc_group_list_current_group_title[LIST_ELEMENT_NAME_MAX_LENGTH * 2];

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
	    lv_poc_member_list_open(lv_poc_group_member_list_title,
			member_list,
			member_list->hide_offline);
    }
    else
    {
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"成员列表", (const uint8_t *)"获取失败");
    }
}

static void lv_poc_group_list_set_current_group_cb(int result_type)
{
	if(result_type == 1)
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Join_Group, false);
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
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
		list_element_t * p_element = (list_element_t *)obj->user_data;
		if(p_element == NULL)
		{
			return;
		}

		lv_poc_set_current_group((lv_poc_group_info_t)p_element->information, lv_poc_group_list_set_current_group_cb);

		if(member_list == NULL)
		{
			member_list = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));
		}

		if(member_list != NULL)
		{
			memset((void *)member_list, 0, sizeof(lv_poc_member_list_t));
			strcpy(lv_poc_group_member_list_title, (const char *)p_element->name);
			member_list->hide_offline = true;
			lv_poc_get_member_list((lv_poc_group_info_t)p_element->information,
				member_list,
				2,
				lv_poc_group_list_get_membet_list_cb);
		}
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
					if(false != list_refresh_status)
					{
						lv_signal_send(activity_list, LV_SIGNAL_PRESSED, NULL);
						lv_poc_list_repeat_refresh(LVPOCUPDATE_TYPE_MEMBERLIST,activity_list,LVPOCLISTIDTCOM_LIST_PERIOD_50);//刷新列表
					}
				}

				case LV_GROUP_KEY_DOWN:

				case LV_GROUP_KEY_UP:
				{
					if(false != list_refresh_status)
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
					if(false != list_refresh_status)
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
			case LV_GROUP_KEY_MB:
			{
				group_list_locked_unlock++;

				if(2 == group_list_locked_unlock)
				{
					lv_poc_group_list_locked_activity_open();
				}
				break;
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
		lv_poc_group_list_refresh(NULL);
	}
	else
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Fail_Update_Group, true);
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"获取失败", NULL);
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
	    lv_poc_group_list_refresh(NULL);
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

void lv_poc_group_list_refresh(lv_poc_group_list_t *group_list_obj)
{
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

    list_element_t * p_cur = NULL;
    lv_obj_t * btn;
	lv_obj_t * btn_label = NULL;
    lv_coord_t btn_height = (display_area.y2 - display_area.y1)/(LV_POC_LIST_COLUM_COUNT + 1);
	lv_coord_t btn_width = (display_area.x2 - display_area.x1);

    char is_first_item = 1;
    char is_set_current_group = 1;

    lv_list_clean(activity_list);

    if(group_list_obj->group_list == NULL)
    {
	    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无成员列表", NULL);
	    return;
    }

    lv_poc_group_info_t current_group = lv_poc_get_current_group();
    char * current_group_name = lv_poc_get_group_name(current_group);

    p_cur = group_list_obj->group_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_group, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_group_list_press_btn_cb);
        p_cur->list_item = btn;
        btn->user_data = (lv_obj_user_data_t)p_cur;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        if(is_first_item == 1)
	    {
        	is_first_item = 0;
        	lv_list_set_btn_selected(activity_list, btn);
        }

		btn_label = lv_list_get_btn_label(btn);
        if(is_set_current_group == 1
	        && GROUP_EQUATION((void *)p_cur->name, (void *)current_group_name, (void *)p_cur->information, (void *)current_group, NULL))
        {
	        is_set_current_group = 0;
        	strcpy(lv_poc_group_list_current_group_title, (const char *)p_cur->name);
        	strcat(lv_poc_group_list_current_group_title, (const char *)"[当前群组]");
        	lv_label_set_text(btn_label, (const char *)lv_poc_group_list_current_group_title);
		}

		lv_obj_t * img = lv_img_create(btn, NULL);
		lv_img_set_src(img, &unlock);//unlock
		lv_img_set_auto_size(img, false);
		lv_obj_set_width(btn_label, btn_width - (lv_coord_t)unlock.header.w - (lv_coord_t)ic_group.header.w - 15);//-15是减去间隙(两个图标)
		lv_obj_align(img, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

        p_cur = p_cur->next;
    }

	lv_poc_list_repeat_refresh(LVPOCUPDATE_TYPE_GROUPLIST,activity_list,LVPOCLISTIDTCOM_LIST_PERIOD_10);//刷新列表
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

//lv_poc_status lv_poc_group_list_get_state(const char * name);

static lv_obj_t * lv_poc_lockgroupwindow_obj = NULL;
static lv_obj_t * lv_poc_lockgroupwindow_label_1 = NULL;
static lv_obj_t * lv_poc_lockgroupwindow_label_2 = NULL;
static lv_obj_t * lv_poc_lockgroupwindow_label_3 = NULL;


static const char * lv_poc_lockgroupwindow_label_1_text = "锁组";
static const char * lv_poc_lockgroupwindow_label_2_text = "是否锁组？";
static const char * lv_poc_lockgroupwindow_label_3_text = "          确定          ";

static lv_style_t lv_poc_lockgroup_style = {0};
static lv_style_t lv_poc_lockgroupbutton_style = {0};

static lv_style_t style_line;
static lv_obj_t *line1;
static lv_point_t line_points[] = {{5,26},{125,26}};

/*
	  name : lv_poc_group_list_lock_open
	 param : none
	author : wangls
  describe : 打开锁组弹跳框
	  date : 2020-06-19
*/
static
void lv_poc_group_list_locked_activity_open(void)
{

	if(lv_poc_lockgroupwindow_obj != NULL)
	{
		return ;
	}

	poc_setting_conf = lv_poc_setting_conf_read();//获取字体
	memset(&lv_poc_lockgroup_style, 0, sizeof(lv_style_t));
    lv_style_copy(&lv_poc_lockgroup_style, &lv_style_pretty_color);
    lv_poc_lockgroup_style.text.color = LV_COLOR_BLACK;
    lv_poc_lockgroup_style.text.font = (lv_font_t *)(poc_setting_conf->font.idle_lockgroupwindows_msg_font);
    lv_poc_lockgroup_style.text.opa = 255;
    lv_poc_lockgroup_style.body.main_color = LV_COLOR_WHITE;
    lv_poc_lockgroup_style.body.grad_color = LV_COLOR_WHITE;
    lv_poc_lockgroup_style.body.opa = 255;
	lv_poc_lockgroup_style.body.radius = 0;

	lv_poc_lockgroupwindow_obj = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_style(lv_poc_lockgroupwindow_obj,&lv_poc_lockgroup_style);
    lv_obj_set_size(lv_poc_lockgroupwindow_obj, LV_HOR_RES - 40, LV_VER_RES - 40);
    lv_obj_set_pos(lv_poc_lockgroupwindow_obj, LV_HOR_RES -140, LV_VER_RES - 98);

	//第一行text
	lv_poc_lockgroupwindow_label_1 = lv_label_create(lv_poc_lockgroupwindow_obj, NULL);
	lv_obj_set_size(lv_poc_lockgroupwindow_label_1, lv_obj_get_width(lv_poc_lockgroupwindow_obj) - 4, lv_obj_get_height(lv_poc_lockgroupwindow_obj) / 3);
	lv_label_set_text(lv_poc_lockgroupwindow_label_1,lv_poc_lockgroupwindow_label_1_text);
	lv_obj_set_style(lv_poc_lockgroupwindow_label_1,&lv_poc_lockgroup_style);
	lv_obj_align(lv_poc_lockgroupwindow_label_1,lv_poc_lockgroupwindow_obj,LV_ALIGN_IN_TOP_LEFT,10,6);

    //创建线条
	lv_style_copy(&style_line, &lv_style_plain);
	style_line.line.color = LV_COLOR_GRAY;
	style_line.line.width = 1;
	style_line.line.rounded = 1;

	line1 = lv_line_create(lv_poc_lockgroupwindow_obj, NULL);
	lv_line_set_points(line1, line_points, 2);
	lv_line_set_style(line1, LV_LINE_STYLE_MAIN, &style_line);
	lv_obj_align(line1, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

	//第二行text
	lv_poc_lockgroupwindow_label_2 = lv_label_create(lv_poc_lockgroupwindow_obj, NULL);
	lv_obj_set_size(lv_poc_lockgroupwindow_label_2, lv_obj_get_width(lv_poc_lockgroupwindow_obj) - 4, lv_obj_get_height(lv_poc_lockgroupwindow_obj) / 3);
	lv_label_set_text(lv_poc_lockgroupwindow_label_2, lv_poc_lockgroupwindow_label_2_text);
	lv_obj_set_style(lv_poc_lockgroupwindow_label_2,&lv_poc_lockgroup_style);
	lv_obj_align(lv_poc_lockgroupwindow_label_2,lv_poc_lockgroupwindow_obj,LV_ALIGN_IN_LEFT_MID,10,-3);

	//第三行字体
	memset(&lv_poc_lockgroupbutton_style, 0, sizeof(lv_style_t));
    lv_style_copy(&lv_poc_lockgroupbutton_style, &lv_style_pretty_color);
    lv_poc_lockgroupbutton_style.text.color = LV_COLOR_WHITE;
    lv_poc_lockgroupbutton_style.text.font = (lv_font_t *)(poc_setting_conf->font.idle_lockgroupwindows_msg_font);
    lv_poc_lockgroupbutton_style.text.opa = 255;
    lv_poc_lockgroupbutton_style.body.main_color = LV_COLOR_BLUE;
    lv_poc_lockgroupbutton_style.body.grad_color = LV_COLOR_BLUE;
	lv_poc_lockgroupbutton_style.body.opa = 255;
	lv_poc_lockgroupbutton_style.body.radius = 0;

	//第三行text
	lv_poc_lockgroupwindow_label_3 = lv_label_create(lv_poc_lockgroupwindow_obj, NULL);
	lv_obj_set_size(lv_poc_lockgroupwindow_label_3, LV_HOR_RES - 40, lv_obj_get_height(lv_poc_lockgroupwindow_obj) / 3);
	lv_obj_set_style(lv_poc_lockgroupwindow_label_3,&lv_poc_lockgroupbutton_style);
	lv_label_set_text(lv_poc_lockgroupwindow_label_3, lv_poc_lockgroupwindow_label_3_text);
	lv_label_set_body_draw(lv_poc_lockgroupwindow_label_3,true);//重刷背景颜色
	lv_obj_align(lv_poc_lockgroupwindow_label_3,lv_poc_lockgroupwindow_obj,LV_ALIGN_IN_BOTTOM_MID,0,-6);

    lv_poc_group_list_cb_set_active(ACT_ID_POC_LOCK_GROUP, true);
	lv_poc_lockgroupwindow_obj->signal_cb = lv_poc_lockgroup_signal_func;

}

/*
	  name : lv_poc_lockgroup_signal_func
	 param : none
	author : wangls
  describe : 回调
	  date : 2020-06-19
*/
static
lv_res_t lv_poc_lockgroup_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
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
					/*处理锁组信息*/

					break;
				}

				case LV_GROUP_KEY_DOWN:

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

				case LV_KEY_ESC:
				{
					lv_poc_group_list_locked_activity_destory();
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

/*
	  name : lv_poc_group_list_locked_event_handler
	 param : none
	author : wangls
  describe : 摧毁锁组弹跳框
	  date : 2020-06-19
*/
static
void lv_poc_group_list_locked_activity_destory(void)
{
	if(lv_poc_lockgroupwindow_obj == NULL)
	{
		return;
	}
	group_list_locked_unlock = 0;
	lv_obj_del(lv_poc_lockgroupwindow_obj);

	lv_poc_lockgroupwindow_obj = NULL;
	lv_poc_lockgroupwindow_label_1 = NULL;
	lv_poc_lockgroupwindow_label_2 = NULL;
	lv_poc_lockgroupwindow_label_3 = NULL;
}

#ifdef __cplusplus
}
#endif
