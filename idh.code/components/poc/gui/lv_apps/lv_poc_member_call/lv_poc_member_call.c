
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

#ifndef __GNUC__
#define __attribute__(x)
#endif

static lv_poc_member_list_t * lv_poc_member_call_member_list_obj = NULL;

static lv_obj_t * lv_poc_member_call_activity_create(lv_poc_display_t *display);

static void lv_poc_member_call_activity_destory(lv_obj_t *obj);

static void * lv_poc_member_call_list_create(lv_obj_t * parent, lv_area_t display_area);

static void lv_poc_member_call_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t lv_poc_member_call_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool lv_poc_member_call_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static void lv_poc_member_call_list_press_btn_action(lv_obj_t * obj, lv_event_t event);

static void lv_poc_member_call_delay_exit_task_cb(lv_task_t * task);

static void lv_poc_member_call_delay_exit_task_create(const int delay_time_ms);

static void lv_poc_member_call_set_member_call_status_cb(int current_status, int dest_status);

static lv_obj_t * activity_list = NULL;

static lv_poc_win_t * activity_win = NULL;

lv_poc_activity_t * poc_member_call_activity = NULL;

static lv_area_t member_call_display_area = {0};

static __attribute__((unused)) char lv_poc_member_call_text_member_call[] = "单呼";

static lv_task_t * lv_poc_member_call_exit_task = NULL;

static int lv_poc_member_call_exit_by_self = 0;

static lv_obj_t * lv_poc_member_call_activity_create(lv_poc_display_t *display)
{
	activity_win = lv_poc_win_create(display, lv_poc_member_call_text_member_call, lv_poc_member_call_list_create);
    return (lv_obj_t *)activity_win;
}

static void lv_poc_member_call_activity_destory(lv_obj_t *obj)
{
	lv_poc_win_t *win = activity_win;
	activity_win = NULL;
	lv_poc_member_list_t *m_list = lv_poc_member_call_member_list_obj;
	lv_poc_member_call_member_list_obj = NULL;
	activity_list = NULL;

	lv_poc_member_list_cb_set_active(ACT_ID_POC_MEMBER_CALL, false);

	if(lv_poc_member_call_exit_by_self == 1)
	{
		lv_poc_set_member_call_status(NULL, false, lv_poc_member_call_set_member_call_status_cb);
	}
	lv_poc_member_call_exit_by_self = 0;

	if(win != NULL)
	{
		lv_mem_free(win);
	}

	if(m_list != NULL)
	{
		list_element_t * cur_p = m_list->online_list;
		list_element_t * temp_p;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		cur_p = m_list->offline_list;
		while(cur_p != NULL)
		{
			temp_p = cur_p;
			cur_p =cur_p->next;
			lv_mem_free(temp_p);
		}

		lv_mem_free(m_list);
	}
}

static void * lv_poc_member_call_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    //memcpy(&member_call_display_area, &display_area, sizeof(display_area));
    member_call_display_area.x1 = display_area.x1;
    member_call_display_area.x2 = display_area.x2;
    member_call_display_area.y1 = display_area.y1;
    member_call_display_area.y2 = display_area.y2;
    activity_list = lv_poc_list_create(parent, NULL, display_area, lv_poc_member_call_list_config);
    return (void *)activity_list;
}

static void lv_poc_member_call_list_config(lv_obj_t * list, lv_area_t list_area)
{
#if 0
    lv_obj_t *btn;
    lv_obj_t *cb;
    lv_obj_t *btn_label;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/3;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
#endif

}

static lv_res_t lv_poc_member_call_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	OSI_LOGI(0, "[poc][signal][member call] sign <- %d  param <- 0x%p\n", sign, param);
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
					//单呼界面键盘不能控制列表
					//lv_signal_send(activity_list, LV_SIGNAL_PRESSED, NULL);
				}

				case LV_GROUP_KEY_DOWN:

				case LV_GROUP_KEY_UP:
				{
					//单呼界面键盘不能控制列表
					//lv_signal_send(activity_list, LV_SIGNAL_CONTROL, param);
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
					if(lvPocGuiIdtCom_get_listen_status())//正在接听讲话时不准退出
						break;
					lv_poc_member_call_exit_by_self = 1;
					lv_poc_member_call_delay_exit_task_create(5);
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			if(lv_poc_member_call_member_list_obj != NULL && current_activity == poc_member_call_activity)
			{
				lv_poc_refr_func_ui(lv_poc_member_call_refresh,
					LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH, lv_poc_member_call_member_list_obj);
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


static bool lv_poc_member_call_design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}

static void lv_poc_member_call_list_press_btn_action(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
		lv_poc_member_call_open(obj->user_data);
	}
}

static void lv_poc_member_call_delay_exit_task_cb(lv_task_t * task)
{
	lv_poc_activity_t *activity = poc_member_call_activity;
	poc_member_call_activity = NULL;
	lv_poc_del_activity(activity);
	lv_poc_member_call_exit_task = NULL;
}

static void lv_poc_member_call_delay_exit_task_create(const int delay_time_ms)
{
	if(lv_poc_member_call_exit_task != NULL)
	{
		lv_task_del(lv_poc_member_call_exit_task);
		lv_poc_member_call_exit_task = NULL;
	}

	if(delay_time_ms <= 0)
	{
		lv_poc_member_call_exit_task = lv_task_create(lv_poc_member_call_delay_exit_task_cb, 1500, LV_TASK_PRIO_LOWEST, NULL);
	}
	else
	{
		lv_poc_member_call_exit_task = lv_task_create(lv_poc_member_call_delay_exit_task_cb, delay_time_ms, LV_TASK_PRIO_LOWEST, NULL);
	}
	lv_task_once(lv_poc_member_call_exit_task);
}

static void lv_poc_member_call_set_member_call_status_cb(int current_status, int dest_status)
{
	if(current_status == dest_status)
	{
		if(current_status == 1)
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Exit_Member_Call, 50, true);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"退出单呼", NULL);
			lv_poc_activity_func_cb_set.member_call_close();
		}
		else if(current_status == 0)
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Success_Member_Call, 50, true);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始单呼", NULL);
		}
		else
		{
			lv_poc_member_call_exit_by_self = 1;
			lv_poc_activity_func_cb_set.member_call_close();
		}
	}
	else
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"发生未知错误", NULL);
	}
}

void lv_poc_member_call_open(void * information)
{
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_MEMBER_CALL,
															lv_poc_member_call_activity_create,
															lv_poc_member_call_activity_destory};

    if(poc_member_call_activity != NULL || information == NULL)
    {
    	return;
    }

	if(lvPocGuiIdtCom_get_listen_status())/*listen status cannot member call*/
	{
		return;
	}

	if(information == NULL)
	{
		OSI_LOGI(0, "[member_call] infomation of call obj is empty\n");
		return;
	}

    lv_poc_member_call_member_list_obj = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));
	if(lv_poc_member_call_member_list_obj == NULL)
	{
		OSI_LOGI(0, "[member_call] apply memory of member list failed!\n");
		return;
	}
	lv_poc_member_call_member_list_obj->offline_list = NULL;
	lv_poc_member_call_member_list_obj->offline_number = 0;
	lv_poc_member_call_member_list_obj->online_list = NULL;
	lv_poc_member_call_member_list_obj->online_number = 0;

    poc_member_call_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    if(poc_member_call_activity == NULL)
    {
	    lv_mem_free(lv_poc_member_call_member_list_obj);
	    lv_poc_member_call_member_list_obj = NULL;
		return ;/*failed open, return*/
    }
    lv_poc_member_list_cb_set_active(ACT_ID_POC_MEMBER_CALL, true);
	lv_poc_activity_set_signal_cb(poc_member_call_activity, lv_poc_member_call_signal_func);
	lv_poc_activity_set_design_cb(poc_member_call_activity, lv_poc_member_call_design_func);

	lv_poc_member_call_add(lv_poc_member_call_member_list_obj, lv_poc_get_member_name((lv_poc_member_info_t)information), true, information);

	lv_poc_member_call_refresh(NULL);
	lv_poc_set_member_call_status(information, true, lv_poc_member_call_set_member_call_status_cb);
}

void lv_poc_member_call_close(void)
{
	if(poc_member_call_activity == NULL)
	{
		return;
	}
	OSI_LOGI(0, "poc_check_member_call close lv_poc_member_call_close");
	lv_poc_member_call_delay_exit_task_create(5);
}

lv_poc_status_t lv_poc_member_call_add(lv_poc_member_list_t *member_list_obj, const char * name, bool is_online, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_add(member_list_obj, name, is_online, information);
}

void lv_poc_member_call_remove(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

	lv_poc_member_list_remove(member_list_obj, name, information);
}

void lv_poc_member_call_clear(lv_poc_member_list_t *member_list_obj)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

	lv_poc_member_list_clear(member_list_obj);
}

int lv_poc_member_call_get_information(lv_poc_member_list_t *member_list_obj, const char * name, void *** information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return 0;
	}

	return lv_poc_member_list_get_information(member_list_obj, name, information);
}

void lv_poc_member_call_refresh(lv_task_t *task_t)
{
	lv_poc_member_list_t *member_list_obj = NULL;

	if(task_t != NULL)
	{
		member_list_obj = (lv_poc_member_list_t *)task_t->user_data;
	}

	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return;
	}

	if(current_activity != poc_member_call_activity)
	{
		return;
	}

    list_element_t * p_cur;
    lv_obj_t * btn;
    lv_coord_t btn_height = (member_call_display_area.y2 - member_call_display_area.y1)/LV_POC_LIST_COLUM_COUNT;

    lv_list_clean(activity_list);

	if(!(member_list_obj->online_list != NULL || member_list_obj->offline_list != NULL))
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无成员列表", NULL);
		return;
	}

    p_cur = member_list_obj->online_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(activity_list, &ic_playing, p_cur->name);
        lv_obj_set_click(btn, false);
        lv_obj_set_event_cb(btn, lv_poc_member_call_list_press_btn_action);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn->user_data = p_cur->information;
        p_cur = p_cur->next;
    }
}

lv_poc_status_t lv_poc_member_call_move_top(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_move_top(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_member_call_move_bottom(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_move_bottom(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_member_call_move_up(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_move_up(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_member_call_move_down(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_move_down(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_member_call_set_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information, bool is_online)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

	if(member_list_obj->online_list != NULL
		&& MEMBER_EQUATION((void *)member_list_obj->online_list->name, (void *)name, (void *)member_list_obj->online_list->information, (void *)information, NULL))
	{
		if(is_online == false)
		{
			lv_poc_member_call_delay_exit_task_create(5);
			return POC_OPERATE_SECCESS;
		}
	}

	return lv_poc_member_list_set_state(member_list_obj, name, information, is_online);
}

lv_poc_status_t lv_poc_member_call_is_exists(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_is_exists(member_list_obj, name, information);
}

lv_poc_status_t lv_poc_member_call_get_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information)
{
	if(member_list_obj == NULL)
	{
		member_list_obj = lv_poc_member_call_member_list_obj;
	}

	if(member_list_obj == NULL)
	{
		return POC_OPERATE_FAILD;
	}

	return lv_poc_member_list_get_state(member_list_obj, name, information);
}

#ifdef __cplusplus
}
#endif

