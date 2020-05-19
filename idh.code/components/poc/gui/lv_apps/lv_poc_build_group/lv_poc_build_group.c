
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

static lv_obj_t * activity_list;

static lv_obj_t *lv_poc_build_group_noattion = NULL;

static lv_poc_win_t * activity_win;

static lv_area_t build_group_display_area;

static bool * members_status_array;

static lv_poc_member_list_t *lv_poc_build_group_member_list;

lv_poc_activity_t * poc_build_group_activity;



static lv_obj_t * lv_poc_build_group_activity_create(lv_poc_display_t *display)
{
#if 1
    activity_win = lv_poc_win_create(display, "成员列表", lv_poc_build_group_list_create);
#endif
    return (lv_obj_t *)activity_win;
}

static void lv_poc_build_group_activity_destory(lv_obj_t *obj)
{
	if(members_status_array != NULL)
	{
	    lv_mem_free(members_status_array);
    }
    members_status_array = NULL;

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
    lv_poc_build_group_noattion = NULL;
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

static void lv_poc_build_group_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
		if(members_status_array == NULL)
		{
			return;
		}
		return;

	    int index = lv_list_get_btn_index(activity_list, obj);
	    bool is_check = members_status_array[index];
	    is_check = !is_check;
	    if(is_check)
	    {
	        lv_btn_set_state((lv_obj_t *)obj->user_data, LV_BTN_STATE_TGL_PR);
	    }
	    else
	    {
	        lv_btn_set_state((lv_obj_t *)obj->user_data, LV_BTN_STATE_TGL_REL);
	    }
	    members_status_array[index] = is_check;
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
					lv_poc_del_activity(poc_build_group_activity);
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
		lv_poc_build_group_refresh(NULL);
	}
	else
	{
		if(lv_poc_build_group_noattion == NULL)
		{
			lv_poc_build_group_noattion = lv_label_create(poc_build_group_activity->display, NULL);
		}

		if(lv_poc_build_group_noattion != NULL)
		{
			lv_label_set_text(lv_poc_build_group_noattion, "获取失败");
		    lv_obj_align(lv_poc_build_group_noattion, activity_list, LV_ALIGN_CENTER, 0, 0);
		}
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

    poc_build_group_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_build_group_activity, lv_poc_build_group_signal_func);
    lv_poc_activity_set_design_cb(poc_build_group_activity, lv_poc_build_group_design_func);

    lv_poc_build_group_noattion = lv_label_create(poc_build_group_activity->display, NULL);
    lv_label_set_text(lv_poc_build_group_noattion, "正在获取成员列表");
    lv_obj_align(lv_poc_build_group_noattion, activity_list, LV_ALIGN_CENTER, 0, 0);
	if(!lv_poc_get_member_list(NULL, lv_poc_build_group_member_list, 1, lv_poc_build_group_get_list_cb))
	{
		lv_label_set_text(lv_poc_build_group_noattion, "获取失败");
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
    return POC_OPERATE_SECCESS;
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

    list_element_t * p_cur;
    list_element_t * p_prv;
    if(member_list_obj->online_list == NULL && member_list_obj->offline_list == NULL)
    {
        return;
    }

    if(NULL != member_list_obj->online_list)
    {
        p_cur = member_list_obj->online_list;
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
        	member_list_obj->online_list = p_cur->next;
            lv_obj_del(p_cur->list_item);
            lv_mem_free(p_cur);
            member_list_obj->online_number = member_list_obj->online_number - 1;
            return;
        }
        p_prv = p_cur;
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
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
        	member_list_obj->offline_list = p_cur->next;
            lv_obj_del(p_cur->list_item);
            lv_mem_free(p_cur);
            member_list_obj->offline_number = member_list_obj->offline_number - 1;
            return;
        }
        p_prv = p_cur;
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

	list_element_t * cur_p = member_list_obj->online_list;
	list_element_t * temp_p;
	while(cur_p != NULL)
	{
		temp_p = cur_p;
		cur_p =cur_p->next;
		lv_mem_free(temp_p);
	}
	member_list_obj->online_list = NULL;

	cur_p = member_list_obj->offline_list;
	while(cur_p != NULL)
	{
		temp_p = cur_p;
		cur_p =cur_p->next;
		lv_mem_free(temp_p);
	}
	member_list_obj->offline_list = NULL;
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

void lv_poc_build_group_refresh(lv_poc_member_list_t *member_list_obj)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_build_group_member_list;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

	if(current_activity != poc_build_group_activity)
	{
		return;
	}

    list_element_t * p_cur;
    lv_obj_t * btn;
    lv_coord_t btn_height = (build_group_display_area.y2 - build_group_display_area.y1)/LV_POC_LIST_COLUM_COUNT;
    char member_list_is_first_item = 1;
    lv_list_clean(activity_list);

    if(member_list_obj->online_list != NULL || member_list_obj->offline_list != NULL)
    {
	    if(lv_poc_build_group_noattion != NULL)
	    {
		    lv_obj_del(lv_poc_build_group_noattion);
	    }
	    lv_poc_build_group_noattion = NULL;
    }
    else
    {
		if(lv_poc_build_group_noattion == NULL)
		{
			lv_poc_build_group_noattion = lv_label_create(poc_build_group_activity->display, NULL);
		}

		if(lv_poc_build_group_noattion != NULL)
		{
		    lv_label_set_text(lv_poc_build_group_noattion, "无成员列表");
		    lv_obj_align(lv_poc_build_group_noattion, activity_list, LV_ALIGN_CENTER, 0, 0);
	    }
	    return;
    }

    p_cur = member_list_obj->online_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_member_online, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_build_group_pressed_cb);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn->user_data = p_cur->information;
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
        btn->user_data = p_cur->information;
        p_cur = p_cur->next;
        if(member_list_is_first_item == 1)
        {
        	member_list_is_first_item = 0;
        	lv_list_set_btn_selected(activity_list, btn);
        }
    }

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

    list_element_t * p_cur;
    list_element_t * p_prv;
    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = member_list_obj->online_list;
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
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
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
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
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        is_find = true;
        p_scr = p_cur;
        member_list_obj->online_list = p_cur->next;
    }
    p_prv = p_cur;
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
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        is_find = true;
        p_scr = p_cur;
        member_list_obj->offline_list = p_cur->next;
    }
    p_prv = p_cur;
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

    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_prv_prv;
    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = member_list_obj->online_list;
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        p_prv->next = p_cur->next;
        p_cur->next = p_prv;
        member_list_obj->online_list = p_cur;
        return POC_OPERATE_SECCESS;
    }

    p_prv_prv = p_prv;
    p_prv = p_cur;
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
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
    {
        p_prv->next = p_cur->next;
        p_cur->next = p_prv;
        member_list_obj->offline_list = p_cur;
        return POC_OPERATE_SECCESS;
    }

    p_prv_prv = p_prv;
    p_prv = p_cur;
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

    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_prv_prv;
    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = member_list_obj->online_list;
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
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
    if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
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

    list_element_t * p_cur;
    lv_poc_status_t status = lv_poc_member_list_is_exists(member_list_obj, name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    if(true == is_online)
    {
        p_cur = member_list_obj->offline_list;
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
        	member_list_obj->offline_list = p_cur->next;
            p_cur->next = member_list_obj->online_list;
            member_list_obj->online_list = p_cur;
            return POC_OPERATE_SECCESS;
        }

        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
            {
            	member_list_obj->offline_list = p_cur->next;
                p_cur->next = member_list_obj->online_list;
                member_list_obj->online_list = p_cur;
                return POC_OPERATE_SECCESS;
            }
            p_cur = p_cur->next;
        }
    }
    else
    {
        p_cur = member_list_obj->online_list;
        if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
        {
        	member_list_obj->online_list = p_cur->next;
            p_cur->next = member_list_obj->offline_list;
            member_list_obj->offline_list = p_cur;
            return POC_OPERATE_SECCESS;
        }

        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION((void *)p_cur->name, (void *)name, (void *)p_cur->information, (void *)information, NULL))
            {
            	member_list_obj->online_list = p_cur->next;
                p_cur->next = member_list_obj->offline_list;
                member_list_obj->offline_list = p_cur;
                return POC_OPERATE_SECCESS;
            }
            p_cur = p_cur->next;
        }
    }

    return POC_UNKNOWN_FAULT;
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



#ifdef __cplusplus
}
#endif
