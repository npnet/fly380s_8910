
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * list_create(lv_obj_t * parent, lv_area_t display_area);

static void list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_build_group_pressed_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_obj_t * activity_list;

static lv_poc_win_t * activity_win;

static bool * online_list_array;

static lv_poc_member_list_t *member_list;

lv_poc_activity_t * poc_build_group_activity;



static lv_obj_t * activity_create(lv_poc_display_t *display)
{
#if 1
    activity_win = lv_poc_win_create(display, "成员列表", list_create);
#endif
    return (lv_obj_t *)activity_win;
}

static void activity_destory(lv_obj_t *obj)
{
    lv_mem_free(online_list_array);
}

static void * list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, list_config);
    return (void *)activity_list;
}

static void list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn;
    lv_obj_t *cb;
    lv_obj_t *btn_label;
    list_element_t * p_cur;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/3;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);

    lv_list_clean(list);

    p_cur = member_list->online_list;
    while(p_cur)
    {
        btn = lv_list_add_btn(list, NULL, p_cur->name);
        lv_obj_set_click(btn, true);
        lv_obj_set_event_cb(btn, lv_poc_build_group_pressed_cb);
        p_cur->list_item = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn_label = lv_list_get_btn_label(btn);
        cb = lv_cb_create(btn, NULL);
        lv_cb_set_text(cb, "");
        //lv_obj_set_size(cb, lv_obj_get_width(cb)*9/17, btn_height*9/17);
        btn->user_data = (void *)cb;
        lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(cb));
        lv_btn_set_state(cb, LV_BTN_STATE_TGL_REL);
        lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
        lv_obj_align(cb, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
        p_cur = p_cur->next;
    }
}

static void lv_poc_build_group_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
	if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
	{
	    int index = lv_list_get_btn_index(activity_list, obj);
	    bool is_check = online_list_array[index];
	    is_check = !is_check;
	    if(is_check)
	    {
	        lv_btn_set_state((lv_obj_t *)obj->user_data, LV_BTN_STATE_TGL_PR);
	    }
	    else
	    {
	        lv_btn_set_state((lv_obj_t *)obj->user_data, LV_BTN_STATE_TGL_REL);
	    }
	    online_list_array[index] = is_check;
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

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}


void lv_poc_build_group_open(void)
{
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_MAKE_GROUP,
															activity_create,
															activity_destory};

    if(poc_build_group_activity != NULL)
    {
    	return;
    }

    member_list = (lv_poc_member_list_t *)lv_mem_alloc(sizeof(lv_poc_member_list_t));
    member_list->offline_list = NULL;
    member_list->offline_number = 0;
    member_list->online_list = NULL;
    member_list->online_number = 0;
    online_list_array = (bool *)lv_mem_alloc(sizeof(bool) * member_list->online_number);
    memset(online_list_array, false, member_list->online_number);

    poc_build_group_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_build_group_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_build_group_activity, design_func);
}





#ifdef __cplusplus
}
#endif
