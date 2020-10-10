﻿#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * state_info_list_create(lv_obj_t * parent, lv_area_t display_area);

static void state_info_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_poc_win_t * state_info_win;

static lv_obj_t * activity_list;

lv_poc_activity_t * poc_state_info_activity;



static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    state_info_win = lv_poc_win_create(display, "状态信息", state_info_list_create);
    return (lv_obj_t *)state_info_win;
}

static void activity_destory(lv_obj_t *obj)
{
}

static void * state_info_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, state_info_list_config);
	lv_poc_notation_refresh();/*把弹框显示在最顶层*/
	return (void *)activity_list;
}

typedef struct
{
	lv_img_dsc_t * ic;
	char *title;
	lv_label_long_mode_t title_long_mode;
	lv_label_align_t title_text_align;
	lv_align_t title_align;
	char *content;
	lv_label_long_mode_t content_long_mode;
	lv_label_align_t content_text_align;
	lv_align_t content_align;
	lv_coord_t content_align_x;
	lv_coord_t content_align_y;
} lv_poc_state_info_label_struct_t;


static char lv_poc_state_info_text_account[64] = {0};
static char lv_poc_state_info_imei_str[16] = {0};
static char lv_poc_state_info_text_iccid_str[20] = {0};
static char lv_poc_state_info_text_battery_state[64] = {0};
static char lv_poc_state_info_text_battery_capacity[64] = {0};
static char lv_poc_state_info_text_signal_strength[64] = {0};
static char lv_poc_state_info_text_local_network[64] = {0};
static char lv_poc_state_info_text_service_status[64] = {0};
static char lv_poc_state_info_text_mobile_network[64] = {0};

lv_poc_state_info_label_struct_t lv_poc_state_info_label_array[] = {
	{
		NULL,
		"账号"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_state_info_text_account, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"IMEI"                    , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_state_info_imei_str, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"ICCID"                    , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_state_info_text_iccid_str, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"电池状态"                   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_state_info_text_battery_state, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"电池电量"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_state_info_text_battery_capacity, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"信号强度"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_state_info_text_signal_strength, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"本机网络"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_state_info_text_local_network, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"服务状态"                    , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_state_info_text_service_status, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"移动网络"                    , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_state_info_text_mobile_network, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},
};

static void state_info_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn;
    lv_obj_t *label;
    lv_obj_t *btn_label;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_style_t * style_label;
    poc_setting_conf = lv_poc_setting_conf_read();
    style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_about_label;
    style_label->text.font = (lv_font_t *)poc_setting_conf->font.about_label_current_font;

	int label_array_size = sizeof(lv_poc_state_info_label_array)/sizeof(lv_poc_state_info_label_struct_t);
	lv_obj_t ** btn_array = (lv_obj_t **)lv_mem_alloc(sizeof(lv_obj_t *) * label_array_size);

	char *temp_str = NULL;
	temp_str = poc_get_device_account_rep(poc_setting_conf->main_SIM);
	if(temp_str != NULL)
	{
		strcpy(lv_poc_state_info_text_account, temp_str);
	}
	else
	{
		lv_poc_state_info_text_account[0] = 0;
	}

	lv_poc_state_info_imei_str[0] = 0;
    poc_get_device_imei_rep((int8_t *)lv_poc_state_info_imei_str);
	lv_poc_state_info_text_iccid_str[0] = 0;
    poc_get_device_iccid_rep((int8_t *)lv_poc_state_info_text_iccid_str);

	strcpy(lv_poc_state_info_text_battery_state, "良好");
	strcpy(lv_poc_state_info_text_battery_capacity, "56%");
	strcpy(lv_poc_state_info_text_signal_strength, "-97 dBm");
	strcpy(lv_poc_state_info_text_local_network, "LTE");
	strcpy(lv_poc_state_info_text_service_status, "Voice:正在使用中/Data:正在使用中");
	strcpy(lv_poc_state_info_text_mobile_network, "已连接");

    for(int i = 0; i < label_array_size; i++)
    {
	    btn = lv_list_add_btn(list, NULL, lv_poc_state_info_label_array[i].title);
	    btn_array[i] = btn;
	    lv_btn_set_fit(btn, LV_FIT_NONE);
	    lv_obj_set_height(btn, btn_height);
	    btn_label = lv_list_get_btn_label(btn);
		label = lv_label_create(btn, NULL);
		btn->user_data = (void *)label;

		lv_label_set_text(label, lv_poc_state_info_label_array[i].content);

		lv_label_set_long_mode(btn_label, lv_poc_state_info_label_array[i].content_long_mode);
		lv_label_set_align(btn_label, lv_poc_state_info_label_array[i].content_text_align);
		lv_label_set_long_mode(label, lv_poc_state_info_label_array[i].content_long_mode);
		lv_label_set_align(label, lv_poc_state_info_label_array[i].content_text_align);

		lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);

		lv_obj_set_width(btn_label, btn_width/4);
		lv_obj_set_width(label, btn_width - lv_obj_get_width(btn_label));
		lv_obj_align(btn_label, btn, lv_poc_state_info_label_array[i].title_align, lv_poc_state_info_label_array[i].content_align_x, lv_poc_state_info_label_array[i].content_align_y);
		lv_obj_align(label, btn_label, lv_poc_state_info_label_array[i].content_align, lv_poc_state_info_label_array[i].content_align_x, lv_poc_state_info_label_array[i].content_align_y);
    }

    lv_list_set_btn_selected(list, btn_array[0]);
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
					lv_poc_del_activity(poc_state_info_activity);
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


void lv_poc_state_info_open(void)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_STATE_INFO,
											activity_create,
											activity_destory};
    poc_state_info_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_state_info_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_state_info_activity, design_func);
}



#ifdef __cplusplus
}
#endif
