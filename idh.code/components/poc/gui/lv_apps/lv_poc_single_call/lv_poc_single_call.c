
#if 1
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

static lv_poc_member_list_t * member_list; 

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * list_create(lv_obj_t * parent, lv_area_t display_area);

static void list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t member_call_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool member_call_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static void lv_poc_single_call_press_btn_action(lv_obj_t * obj, lv_event_t event);

static lv_res_t member_call_set_title(const char * title);

static void member_call_delay_exit_task_cb(lv_task_t * task);

static void member_call_delay_exit_task_create(const int delay_time_ms);


static lv_obj_t * activity_list;

static lv_poc_win_t * activity_win;

static lv_area_t member_call_display_area;

lv_poc_activity_t * poc_member_call_activity;

static char single_call_win_title[LV_POC_NAME_LEN + 10] = {0};

#if 0
static char single_call_win_title_head[] = "正在连接";

static char single_call_text_failed_call[] = "连接失败";

static char single_call_text_success_call[] = "连接成功";

static char single_call_text_single_call[] = "单呼";

static char single_call_text_error[] = "错误";
#endif

static char single_call_text_exit_single_call[] = "退出单呼";

static lv_task_t * lv_poc_single_call_exit_task = NULL;


static lv_obj_t * activity_create(lv_poc_display_t *display)
{
	activity_win = lv_poc_win_create(display, single_call_win_title, list_create);
    return (lv_obj_t *)activity_win;
}

static void activity_destory(lv_obj_t *obj)
{
	if(activity_win != NULL)
	{
		lv_mem_free(activity_win);
		activity_win = NULL;
	}
	
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

		cur_p = member_list->offline_list;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			//lv_obj_del(temp_p->list_item);
			lv_mem_free(temp_p);
		}

		lv_mem_free(member_list);
	}
	member_list = NULL;
}

static void * list_create(lv_obj_t * parent, lv_area_t display_area)
{
    //memcpy(&member_call_display_area, &display_area, sizeof(display_area));
    member_call_display_area.x1 = display_area.x1;
    member_call_display_area.x2 = display_area.x2;
    member_call_display_area.y1 = display_area.y1;
    member_call_display_area.y2 = display_area.y2;
    activity_list = lv_poc_list_create(parent, NULL, display_area, list_config);
    return (void *)activity_list;
}

static void list_config(lv_obj_t * list, lv_area_t list_area)
{
#if 0
    lv_obj_t *btn;
    lv_obj_t *cb;
    lv_obj_t *btn_label;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/3;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
#endif
    
}

static lv_res_t member_call_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	OSI_LOGI(0, "func:%s %d\n", __func__, sign);
	switch(sign)
	{
		case LV_SIGNAL_PRESSED:
		{
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				
				case LV_GROUP_KEY_DOWN:
				
				case LV_GROUP_KEY_UP:
				{
					activity_list->signal_cb(activity_list, LV_SIGNAL_CONTROL, param);
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
					member_call_delay_exit_task_create(500);
					break;
				}
			}
			break;
		}
			
		case LV_SIGNAL_LONG_PRESS:
		{
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				{
					break;
				}
				
				case LV_GROUP_KEY_ESC:
				{
					break;
				}
				
				case LV_GROUP_KEY_DOWN:
				{
					break;
				}
				
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
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			if(member_list != NULL && current_activity == poc_member_call_activity)
			{
				lv_poc_member_call_refresh();
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


static bool member_call_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}

static void lv_poc_single_call_press_btn_action(lv_obj_t * obj, lv_event_t event)
{   
}

static lv_res_t member_call_set_title(const char * title)
{
	static char title_text[LV_POC_NAME_LEN] = {0};
	if(title == NULL)
	{
		return LV_RES_INV;
	}
	strcpy(title_text, title);
	lv_label_set_text(activity_win->title, title_text);
	return LV_RES_OK;
}

static void member_call_delay_exit_task_cb(lv_task_t * task)
{
	member_call_set_title(single_call_text_exit_single_call);
	lv_poc_del_activity(poc_member_call_activity);
	lv_poc_single_call_exit_task = NULL;
}

static void member_call_delay_exit_task_create(const int delay_time_ms)
{
	if(lv_poc_single_call_exit_task != NULL)
	{
		lv_task_del(lv_poc_single_call_exit_task);
		lv_poc_single_call_exit_task = NULL;
	}

	if(delay_time_ms <= 0)
	{
		lv_poc_single_call_exit_task = lv_task_create(member_call_delay_exit_task_cb, 1500, LV_TASK_PRIO_LOWEST, NULL);
	}
	else
	{
		lv_poc_single_call_exit_task = lv_task_create(member_call_delay_exit_task_cb, delay_time_ms, LV_TASK_PRIO_LOWEST, NULL);
	}
	lv_task_once(lv_poc_single_call_exit_task);
}

void lv_poc_member_call_open(void * information)
{
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_SINGLE_CALL,
															activity_create,
															activity_destory};

    if(poc_member_call_activity != NULL)
    {
    	return;
    }

	if(information == NULL)
	{
		OSI_LOGI(0, "[single_call] infomation of call obj is empty\n");
		return;
	}
	
    poc_member_call_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    if(poc_member_call_activity == NULL)
    {
    	OSI_LOGI(0, "[single_call] failed to create a activity\n");
    	return;
    }
    member_list = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));
	if(member_list == NULL)
	{
		OSI_LOGI(0, "[single_call] apply memory of member list failed!\n");
		return;
	}
	member_list->offline_list = NULL;
	member_list->offline_number = 0;
	member_list->online_list = NULL;
	member_list->online_number = 0;
	lv_poc_activity_set_signal_cb(poc_member_call_activity, member_call_signal_func);
	lv_poc_activity_set_design_cb(poc_member_call_activity, member_call_design_func);
}

lv_poc_status_t lv_poc_member_call_add(const char * name, void * information)
{
    lv_poc_status_t status = lv_poc_member_call_is_exists(name, information);
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
    if(member_list->online_list)
    {
        new_element->next = member_list->online_list;
    }
    else
    {
        new_element->next = NULL;
    }
    member_list->online_list = new_element;
    member_list->online_number = member_list->online_number + 1;
    return POC_OPERATE_SECCESS;
}

void lv_poc_member_call_clear(void)
{
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
		member_list->online_list = NULL;
		member_list->offline_list = NULL;

	}
	else
	{
		member_list = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));
		if(member_list == NULL)
		{
			OSI_LOGI(0, "[single_call] apply memory of member list failed!\n");
			return;
		}
		memset(member_list, 0, sizeof(lv_poc_member_list_t));
	}
}

lv_poc_status_t lv_poc_member_call_is_exists(const char * name, void * information)
{
    list_element_t * p_cur;
    if(NULL == name)
    {
        return POC_OPERATE_FAILD;
    }

    p_cur = member_list->online_list;
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

void lv_poc_member_call_refresh(void)
{
    list_element_t * p_cur;
    lv_obj_t * btn;
    lv_coord_t btn_height = (member_call_display_area.y2 - member_call_display_area.y1)/3;
    
    lv_list_clean(activity_list);

    p_cur = member_list->online_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_member_online, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_single_call_press_btn_action);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn->user_data = p_cur->information;
        p_cur = p_cur->next;
    }
}

#ifdef __cplusplus
}
#endif
#endif

