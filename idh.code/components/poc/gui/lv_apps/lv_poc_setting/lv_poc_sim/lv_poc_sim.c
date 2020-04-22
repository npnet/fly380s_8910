
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"


static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * list_create(lv_obj_t * parent, lv_area_t display_area);

static void list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_choice_sim_press_btn_action(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);



static lv_poc_win_t * activity_win;
static lv_obj_t * activity_list;

lv_poc_activity_t * poc_main_sim_choice_activity;

static lv_poc_rb_t * main_sim_choice_rb;




static lv_obj_t * activity_create(lv_poc_display_t *display)
{
#if 1
    activity_win = lv_poc_win_create(display, "主卡选择", list_create);
#endif
    return (lv_obj_t *)activity_win;
}

static void activity_destory(lv_obj_t *obj)
{
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
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
     lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_coord_t btn_cb_height = (list_area.y2 - list_area.y1)/3;
    lv_obj_t * btn_array[2];
    main_sim_choice_rb = lv_poc_rb_create();

    btn = lv_list_add_btn(list, NULL, "卡1");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_choice_sim_press_btn_action);
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    cb = lv_cb_create(btn, NULL);
    lv_cb_set_text(cb, "");
    lv_obj_set_size(cb, lv_obj_get_width(cb)*9/17, btn_height*9/17);
    btn->user_data = (void *)cb;
    lv_obj_set_height(cb, btn_cb_height);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(cb));
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(cb, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_poc_rb_add(main_sim_choice_rb, cb);
    btn_array[0] = btn;
    
    btn = lv_list_add_btn(list, NULL, "卡2");
    lv_obj_set_click(btn, true);
    lv_obj_set_event_cb(btn, lv_poc_choice_sim_press_btn_action);
    lv_btn_set_fit(btn, LV_FIT_NONE);
    lv_obj_set_height(btn, btn_height);
    btn_label = lv_list_get_btn_label(btn);
    cb = lv_cb_create(btn, NULL);
    lv_cb_set_text(cb, "");
    btn->user_data = (void *)cb;
    lv_obj_set_height(cb, btn_cb_height);
    lv_obj_set_width(btn_label, btn_width - lv_obj_get_width(cb));
    lv_obj_align(btn_label, btn, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_obj_align(cb, btn_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    lv_poc_rb_add(main_sim_choice_rb, cb);
    btn_array[1] = btn;
    
    lv_list_set_btn_selected(list, btn_array[poc_setting_conf->main_SIM]);
    lv_poc_rb_press(main_sim_choice_rb, btn_array[poc_setting_conf->main_SIM]->user_data);
}

static void lv_poc_choice_sim_press_btn_action(lv_obj_t * obj, lv_event_t event)
{
    int index = lv_list_get_btn_index(activity_list, obj);
    if(LV_EVENT_CLICKED == event)
    {
		if(poc_setting_conf->main_SIM != index)
		{
			poc_setting_conf->main_SIM = index;
			lv_poc_setting_conf_write();
			lv_poc_rb_press(main_sim_choice_rb, obj->user_data);
		}
    }
}

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	switch(sign)
	{
		case LV_SIGNAL_PRESSED:
		{
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_KEY_ENTER:
				
				case LV_KEY_DOWN:
				
				case LV_KEY_UP:
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
					lv_poc_del_activity(poc_main_sim_choice_activity);
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



void lv_poc_main_sim_choice_open(void)
{
#if 1
    static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_MAIN_SIM_CHOICE,
															activity_create,
															activity_destory};
	poc_setting_conf = lv_poc_setting_conf_read();
    poc_main_sim_choice_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
#endif
	lv_poc_activity_set_signal_cb(poc_main_sim_choice_activity, signal_func);
	lv_poc_activity_set_design_cb(poc_main_sim_choice_activity, design_func);
}


#ifdef __cplusplus
}
#endif
