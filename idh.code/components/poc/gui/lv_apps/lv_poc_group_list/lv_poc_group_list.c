#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

#define CURRENR_GROUP_NAME_EXTERN 20

static lv_poc_group_list_t * group_list;

static lv_poc_member_list_t * member_list;

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_group_list_press_btn_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);



static lv_obj_t * activity_list;

static lv_area_t display_area;

lv_poc_activity_t * poc_group_list_activity;

char group_member_list_is_open = 0;

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    display_area.x1 = 0;
    display_area.x2 = lv_obj_get_x(display) + lv_poc_get_display_width(display);
    display_area.y1 = 0;
    display_area.y2 = 0 + lv_poc_get_display_height(display);
    activity_list = lv_poc_list_create(display, NULL, display_area, list_config);
    return (lv_obj_t *)activity_list;
}

static void activity_destory(lv_obj_t *obj)
{
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
			//lv_obj_del(temp_p->list_item);
			lv_mem_free(temp_p);
		}
		#if 0
		cur_p = member_list->offline_list;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			//lv_obj_del(temp_p->list_item);
			lv_mem_free(temp_p);
		}
		#endif

		lv_mem_free(member_list);
	}
	member_list = NULL;
}

static void list_config(lv_obj_t * list, lv_area_t list_area)
{
}

static void lv_poc_group_list_press_btn_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
	}
}

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
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




void lv_poc_group_list_open(lv_poc_group_list_t *group_list_obj)
{
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_GROUP_LIST,
															activity_create,
															activity_destory};

    if(poc_group_list_activity != NULL)
    {
    	return;
    }

	group_list = (lv_poc_group_list_t *)lv_mem_alloc(sizeof(lv_poc_group_list_t));
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
    lv_poc_activity_set_signal_cb(poc_group_list_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_group_list_activity, design_func);

    if(group_list_obj == NULL)
    {

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
    if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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
        if((GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0)))
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

	if(current_activity == poc_group_list_activity)
	{
		return;
	}

    list_element_t * p_cur;
    lv_obj_t * btn;
    lv_coord_t btn_height = (display_area.y2 - display_area.y1)/(LV_POC_LIST_COLUM_COUNT + 1);
    char is_first_item = 1;

    lv_list_clean(activity_list);
    p_cur = group_list_obj->group_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_group, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_group_list_press_btn_cb);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        p_cur = p_cur->next;
        if(is_first_item == 1)
        {
        	is_first_item = 0;
        	lv_list_set_btn_selected(activity_list, btn);
        }
    }
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
    if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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
    if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        is_find = true;
        p_scr = p_cur;
        group_list_obj->group_list = p_cur->next;
    }
    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(false == is_find && (GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0)))
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
    if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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
        if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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
    if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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
        if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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
        if(GROUP_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
            return POC_GROUP_EXISTS;
        }
        p_cur = p_cur->next;
    }

    return POC_GROUP_NONENTITY;
}

//lv_poc_status lv_poc_group_list_get_state(const char * name);

#ifdef __cplusplus
}
#endif
