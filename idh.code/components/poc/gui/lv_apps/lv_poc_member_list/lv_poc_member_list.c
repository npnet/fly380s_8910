
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

//static void lv_pov_member_list_get_list_cb(int msg_type);


static lv_obj_t * activity_list;

static lv_poc_win_t * activity_win;

static lv_area_t member_list_display_area;

static int8_t lv_poc_member_list_title[100] = {0};

static const int8_t lv_poc_member_list_default_title[] = "成员列表";

lv_poc_activity_t * poc_member_list_activity;



static lv_obj_t * lv_poc_member_list_activity_create(lv_poc_display_t *display)
{
    activity_win = lv_poc_win_create(display, (const char *)lv_poc_member_list_title, lv_poc_member_list_list_create);
    return (lv_obj_t *)activity_win;
}

static void lv_poc_member_list_activity_destory(lv_obj_t *obj)
{
	if(activity_win != NULL)
	{
		lv_mem_free(activity_win);
		activity_win = NULL;
	}

	if(lv_poc_member_list_obj != NULL)
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
	}
	lv_poc_member_list_obj = NULL;
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

static void lv_poc_member_list_prssed_btn_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
		lv_poc_member_call_open(obj->user_data);
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
					lv_poc_del_activity(poc_member_list_activity);
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			if(lv_poc_member_list_obj != NULL && current_activity == poc_member_list_activity)
			{
				lv_poc_member_list_refresh();
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
void lv_pov_member_list_get_list_cb(int msg_type)
{
	//add your information
	if(msg_type==1)
	{
		OSI_LOGE(0, "[lml]callback msg ok");
	}
	else
	{
		OSI_LOGE(0, "[lml]callback msg falied");
	}	
}

void lv_poc_member_list_open(IN char * title, IN lv_poc_member_list_t *members, IN bool hide_offline)
{
    lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_MEMBER_LIST,
    		lv_poc_member_list_activity_create,
			lv_poc_member_list_activity_destory};
    if(lv_poc_member_list_obj != NULL)
    {
    	return;
    }

    lv_poc_member_list_obj = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));

    if(title != NULL)
	{
    	strcpy((char *)lv_poc_member_list_title, (const char *)title);
	}
    else
    {
    	strcpy((char *)lv_poc_member_list_title, (const char *)lv_poc_member_list_default_title);
    }

    if(members != NULL)
    {
    	list_element_t * p_cur = NULL;
    	memcpy(lv_poc_member_list_obj, members, sizeof(lv_poc_member_list_t));

    	p_cur = members->online_list;
    	while(p_cur == NULL)
    	{
    		lv_poc_member_list_add(p_cur->name, true, p_cur->information);
    		p_cur = p_cur->next;
    	}

    	if(hide_offline == false)
    	{
			p_cur = members->offline_list;
			while(p_cur == NULL)
			{
				lv_poc_member_list_add(p_cur->name, false, p_cur->information);
				p_cur = p_cur->next;
			}
    	}
    	else
    	{
    		lv_poc_member_list_obj->offline_list = NULL;
    		lv_poc_member_list_obj->offline_number = 0;
    	}
    }
    else
    {
        lv_poc_member_list_obj->offline_list = NULL;
        lv_poc_member_list_obj->offline_number = 0;
        lv_poc_member_list_obj->online_list = NULL;
        lv_poc_member_list_obj->online_number = 0;
    }

    lv_poc_member_list_obj->hide_offline = hide_offline;
    poc_member_list_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_member_list_activity, lv_poc_member_list_signal_func);
    lv_poc_activity_set_design_cb(poc_member_list_activity, lv_poc_member_list_design_func);
}

lv_poc_status_t lv_poc_member_list_add(const char * name, bool is_online, void * information)
{
    lv_poc_status_t status = lv_poc_member_list_is_exists(name, information);
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
        if(lv_poc_member_list_obj->online_list)
        {
            new_element->next = lv_poc_member_list_obj->online_list;
        }
        else
        {
            new_element->next = NULL;
        }
        lv_poc_member_list_obj->online_list = new_element;
        lv_poc_member_list_obj->online_number = lv_poc_member_list_obj->online_number + 1;
    }
    else
    {
        if(lv_poc_member_list_obj->offline_list)
        {
            new_element->next = lv_poc_member_list_obj->offline_list;
        }
        else
        {
            new_element->next = NULL;
        }
        lv_poc_member_list_obj->offline_list = new_element;
        lv_poc_member_list_obj->offline_number = lv_poc_member_list_obj->offline_number + 1;
    }

    //if(member_list != NULL && current_activity == poc_member_list_activity)
    //{
	//	lv_poc_member_list_refresh();
    //}
    return POC_OPERATE_SECCESS;
}

void lv_poc_member_list_remove(const char * name, void * information)
{
    list_element_t * p_cur;
    list_element_t * p_prv;
    if(lv_poc_member_list_obj->online_list == NULL && lv_poc_member_list_obj->offline_list == NULL)
    {
        return;
    }

    if(NULL != lv_poc_member_list_obj->online_list)
    {
        p_cur = lv_poc_member_list_obj->online_list;
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
        	lv_poc_member_list_obj->online_list = p_cur->next;
            lv_obj_del(p_cur->list_item);
            lv_mem_free(p_cur);
            lv_poc_member_list_obj->online_number = lv_poc_member_list_obj->online_number - 1;
            return;
        }
        p_prv = p_cur;
        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
            {
                p_prv->next = p_cur->next;
                lv_obj_del(p_cur->list_item);
                lv_mem_free(p_cur);
                lv_poc_member_list_obj->online_number = lv_poc_member_list_obj->online_number - 1;
                return;
            }

            p_prv = p_cur;
            p_cur = p_cur->next;
        }
    }

    if(NULL != lv_poc_member_list_obj->offline_list)
    {
        p_cur = lv_poc_member_list_obj->offline_list;
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
        	lv_poc_member_list_obj->offline_list = p_cur->next;
            lv_obj_del(p_cur->list_item);
            lv_mem_free(p_cur);
            lv_poc_member_list_obj->offline_number = lv_poc_member_list_obj->offline_number - 1;
            return;
        }
        p_prv = p_cur;
        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
            {
                p_prv->next = p_cur->next;
                lv_obj_del(p_cur->list_item);
                lv_mem_free(p_cur);
                lv_poc_member_list_obj->offline_number = lv_poc_member_list_obj->offline_number - 1;
                return;
            }

            p_prv = p_cur;
            p_cur = p_cur->next;
        }
    }
}

void lv_poc_member_list_clear(void)
{
	if(lv_poc_member_list_obj != NULL)
	{
		list_element_t * cur_p = lv_poc_member_list_obj->online_list;
		list_element_t * temp_p;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			//lv_obj_del(temp_p->list_item);
			lv_mem_free(temp_p);
		}
		lv_poc_member_list_obj->online_list = NULL;

		cur_p = lv_poc_member_list_obj->offline_list;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			//lv_obj_del(temp_p->list_item);
			lv_mem_free(temp_p);
		}
		lv_poc_member_list_obj->offline_list = NULL;

	}
	else
	{
		lv_poc_member_list_obj = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));
		if(lv_poc_member_list_obj == NULL)
		{
			return;
		}
		memset(lv_poc_member_list_obj, 0, sizeof(lv_poc_member_list_t));
	}
}

int lv_poc_member_list_get_information(const char * name, void *** information)
{
    list_element_t * p_cur = lv_poc_member_list_obj->online_list;
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

    p_cur = lv_poc_member_list_obj->offline_list;
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

void lv_poc_member_list_refresh(void)
{
    list_element_t * p_cur;
    lv_obj_t * btn;
    lv_coord_t btn_height = (member_list_display_area.y2 - member_list_display_area.y1)/LV_POC_LIST_COLUM_COUNT;
    char member_list_is_first_item = 1;
    lv_list_clean(activity_list);


    p_cur = lv_poc_member_list_obj->online_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_member_online, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_member_list_prssed_btn_cb);
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

    p_cur = lv_poc_member_list_obj->offline_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_member_offline, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_member_list_prssed_btn_cb);
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



lv_poc_status_t lv_poc_member_list_move_top(const char * name, void * information)
{
    list_element_t * p_cur;
    list_element_t * p_prv;
    lv_poc_status_t status = lv_poc_member_list_is_exists(name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = lv_poc_member_list_obj->online_list;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
            p_prv->next = p_cur->next;
            p_cur->next = lv_poc_member_list_obj->online_list;
            lv_poc_member_list_obj->online_list = p_cur;
            return POC_OPERATE_SECCESS;
        }
        p_prv = p_cur;
        p_cur = p_cur->next;
    }

    p_cur = lv_poc_member_list_obj->offline_list;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
            p_prv->next = p_cur->next;
            p_cur->next = lv_poc_member_list_obj->offline_list;
            lv_poc_member_list_obj->offline_list = p_cur;
            return POC_OPERATE_SECCESS;
        }
        p_prv = p_cur;
        p_cur = p_cur->next;
    }

    return POC_UNKNOWN_FAULT;
}

lv_poc_status_t lv_poc_member_list_move_bottom(const char * name, void * information)
{
    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_scr;
    bool is_find = false;
    lv_poc_status_t status = lv_poc_member_list_is_exists(name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = lv_poc_member_list_obj->online_list;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        is_find = true;
        p_scr = p_cur;
        lv_poc_member_list_obj->online_list = p_cur->next;
    }
    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(false == is_find && (MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0)))
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

    p_cur = lv_poc_member_list_obj->offline_list;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        is_find = true;
        p_scr = p_cur;
        lv_poc_member_list_obj->offline_list = p_cur->next;
    }
    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(false == is_find && (MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0)))
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

lv_poc_status_t lv_poc_member_list_move_up(const char * name, void * information)
{
    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_prv_prv;
    lv_poc_status_t status = lv_poc_member_list_is_exists(name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = lv_poc_member_list_obj->online_list;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        p_prv->next = p_cur->next;
        p_cur->next = p_prv;
        lv_poc_member_list_obj->online_list = p_cur;
        return POC_OPERATE_SECCESS;
    }

    p_prv_prv = p_prv;
    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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

    p_cur = lv_poc_member_list_obj->offline_list;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        p_prv->next = p_cur->next;
        p_cur->next = p_prv;
        lv_poc_member_list_obj->offline_list = p_cur;
        return POC_OPERATE_SECCESS;
    }

    p_prv_prv = p_prv;
    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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

lv_poc_status_t lv_poc_member_list_move_down(const char * name, void * information)
{
    list_element_t * p_cur;
    list_element_t * p_prv;
    list_element_t * p_prv_prv;
    lv_poc_status_t status = lv_poc_member_list_is_exists(name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    p_cur = lv_poc_member_list_obj->online_list;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        if(NULL != p_cur->next)
        {
            p_cur = p_cur->next;
            lv_poc_member_list_obj->online_list->next = p_cur->next;
            p_cur->next = lv_poc_member_list_obj->online_list;
            lv_poc_member_list_obj->online_list = p_cur;
        }
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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

    p_cur = lv_poc_member_list_obj->offline_list;
    if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
    {
        if(NULL != p_cur->next)
        {
            p_cur = p_cur->next;
            lv_poc_member_list_obj->offline_list->next = p_cur->next;
            p_cur->next = lv_poc_member_list_obj->offline_list;
            lv_poc_member_list_obj->offline_list = p_cur;
        }
        return POC_OPERATE_SECCESS;
    }

    p_prv = p_cur;
    p_cur = p_cur->next;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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

lv_poc_status_t lv_poc_member_list_set_state(const char * name, void * information, bool is_online)
{
    list_element_t * p_cur;
    lv_poc_status_t status = lv_poc_member_list_is_exists(name, information);
    if(status == POC_OPERATE_FAILD || status == POC_MEMBER_NONENTITY)
    {
        return status;
    }

    if(true == is_online)
    {
        p_cur = lv_poc_member_list_obj->offline_list;
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
        	lv_poc_member_list_obj->offline_list = p_cur->next;
            p_cur->next = lv_poc_member_list_obj->online_list;
            lv_poc_member_list_obj->online_list = p_cur;
            return POC_OPERATE_SECCESS;
        }

        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
            {
            	lv_poc_member_list_obj->offline_list = p_cur->next;
                p_cur->next = lv_poc_member_list_obj->online_list;
                lv_poc_member_list_obj->online_list = p_cur;
                return POC_OPERATE_SECCESS;
            }
            p_cur = p_cur->next;
        }
    }
    else
    {
        p_cur = lv_poc_member_list_obj->online_list;
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
        	lv_poc_member_list_obj->online_list = p_cur->next;
            p_cur->next = lv_poc_member_list_obj->offline_list;
            lv_poc_member_list_obj->offline_list = p_cur;
            return POC_OPERATE_SECCESS;
        }

        p_cur = p_cur->next;
        while(p_cur)
        {
            if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
            {
            	lv_poc_member_list_obj->online_list = p_cur->next;
                p_cur->next = lv_poc_member_list_obj->offline_list;
                lv_poc_member_list_obj->offline_list = p_cur;
                return POC_OPERATE_SECCESS;
            }
            p_cur = p_cur->next;
        }
    }

    return POC_UNKNOWN_FAULT;
}

lv_poc_status_t lv_poc_member_list_is_exists(const char * name, void * information)
{
    list_element_t * p_cur;
    if(NULL == name)
    {
        return POC_OPERATE_FAILD;
    }

    p_cur = lv_poc_member_list_obj->online_list;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
            return POC_MEMBER_EXISTS;
        }
        p_cur = p_cur->next;
    }

    p_cur = lv_poc_member_list_obj->offline_list;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
            return POC_MEMBER_EXISTS;
        }
        p_cur = p_cur->next;
    }

    return POC_MEMBER_NONENTITY;
}

lv_poc_status_t lv_poc_member_list_get_state(const char * name, void * information)
{
    list_element_t * p_cur;
    if(NULL == name)
    {
        return POC_OPERATE_FAILD;
    }

    p_cur = lv_poc_member_list_obj->online_list;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
        {
            return POC_MEMBER_ONLINE;
        }
        p_cur = p_cur->next;
    }

    p_cur = lv_poc_member_list_obj->offline_list;
    while(p_cur)
    {
        if(MEMBER_EQUATION(p_cur->name, name, p_cur->information, information, 0))
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

