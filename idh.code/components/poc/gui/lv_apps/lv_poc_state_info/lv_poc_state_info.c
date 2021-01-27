#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * state_info_list_create(lv_obj_t * parent, lv_area_t display_area);

static void state_info_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_poc_win_t * state_info_win = NULL;

static lv_task_t * state_task = NULL;

static lv_obj_t * activity_list = NULL;

lv_poc_activity_t * poc_state_info_activity = NULL;

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    state_info_win = lv_poc_win_create(display, "状态信息", state_info_list_create);
    return (lv_obj_t *)state_info_win;
}

static void activity_destory(lv_obj_t *obj)
{
	if(state_info_win != NULL)
	{
		lv_mem_free(state_info_win);
		state_info_win = NULL;
	}
}

static void * state_info_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, state_info_list_config);
	lv_poc_notation_refresh();//把弹框显示在最顶层
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

typedef void (*status_info_menu_task_t)(void);
static status_info_menu_task_t status_info_menu_task_ext[LV_POC_STABAR_TASK_EXT_LENGTH];
static void lv_poc_info_menu_task(lv_task_t * task);
static lv_obj_t *label_array[9] = {};
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

bool lv_poc_status_info_menu_task_ext_add(status_info_menu_task_t task)
{
    static int stabar_task_ext_count = 0;
    if(stabar_task_ext_count >= LV_POC_STABAR_TASK_EXT_LENGTH) return false;
    status_info_menu_task_ext[stabar_task_ext_count] = task;
    stabar_task_ext_count = stabar_task_ext_count + 1;
    return true;
}

static void lv_poc_info_menu_task(lv_task_t * task)
{
    int k;

    for(k = 0; k < LV_POC_STABAR_TASK_EXT_LENGTH; k++)
    {
        if(status_info_menu_task_ext[k] != NULL)
        {
            (status_info_menu_task_ext[k])();
        }
    }
}

static int lv_poc_state_info_network_server(void)
{
	uint8_t uState;
	uint8_t nSim = POC_SIM_1;
	int nStatus = 0;

	CFW_GetGprsAttState(&uState, nSim);
	if((lvPocGuiBndCom_get_status()))
	{
		if(uState == 0)
		{
			strcpy(lv_poc_state_info_text_service_status, "Voice:正在使用中/Data:未使用");
			nStatus  = 0;
		}
		else
		{
			strcpy(lv_poc_state_info_text_service_status, "Voice:正在使用中/Data:正在使用中");
			nStatus  = 1;
		}
	}
	else if(uState == 0)
	{
		strcpy(lv_poc_state_info_text_service_status, "Voice:未使用/Data:未使用");
		nStatus  = 2;
	}
	else
	{
		strcpy(lv_poc_state_info_text_service_status, "Voice:未使用/Data:正在使用中");
		nStatus  = 3;
	}

	return nStatus;
}

static void lv_poc_state_info_network_connection(void)
{
	POC_MMI_MODEM_PLMN_RAT	rat = 0;
	int8_t	operat = 0;

	poc_get_operator_network_type_req(POC_SIM_1,&operat,&rat);
	switch(rat)
	{
		case  MMI_MODEM_PLMN_RAT_GSM:                                                 // GSM network
			strcpy(lv_poc_state_info_text_local_network, "GSM");
			strcpy(lv_poc_state_info_text_mobile_network, "已连接");
			break;
   		case MMI_MODEM_PLMN_RAT_UMTS:                                                    // UTRAN network
   			strcpy(lv_poc_state_info_text_local_network, "UTRAM");
			strcpy(lv_poc_state_info_text_mobile_network, "已连接");
			break;
    	case MMI_MODEM_PLMN_RAT_LTE:                                                    // LTE network
    		strcpy(lv_poc_state_info_text_local_network, "LTE");
			strcpy(lv_poc_state_info_text_mobile_network, "已连接");
			break;
		case MMI_MODEM_PLMN_RAT_NO_SERVICE:  											// 无服务 network
			strcpy(lv_poc_state_info_text_local_network, "无服务");
			strcpy(lv_poc_state_info_text_mobile_network, "未连接");
			break;
		default:
			strcpy(lv_poc_state_info_text_local_network, "UNKNOW");
			break;
	}

}

static void lv_poc_state_info_battery(void)
{
	char battery[24] = {0};
	battery_values_t values = {0};

	poc_battery_get_status(&values);
	__itoa(values.battery_value, (char *)&battery, 10);
	strcat(battery,"%");
	strcpy(lv_poc_state_info_text_battery_capacity, battery);
	if(values.battery_status)
	{
		if(values.charging == POC_CHG_CONNECTED)
		{
			strcpy(lv_poc_state_info_text_battery_state, "充电");
		}
		else
		{
			strcpy(lv_poc_state_info_text_battery_state, "良好");
		}
	}
	else
	{
		strcpy(lv_poc_state_info_text_battery_state, "故障");
	}
}

static void lv_poc_state_info_signalbdm(void)
{
	char sing[24] = "-";
	uint8_t nSignalDBM = 0;

	poc_get_signal_dBm(&nSignalDBM);
	__itoa(nSignalDBM, (char *)&sing[1] , 10);
	strcat(sing,"dBm");
 	strcpy(lv_poc_state_info_text_signal_strength, sing);
}

static void lv_poc_refresh_state_info_signalbdm(void)
{
	lv_poc_state_info_signalbdm();
	lv_label_set_text(label_array[5], lv_poc_state_info_label_array[5].content);
}

static void lv_poc_refresh_state_info_network_server(void)
{
	int ret = 0;
	static int cRefresh = 0;

	ret = lv_poc_state_info_network_server();
	if(cRefresh != ret)
	{
		lv_label_set_text(label_array[7], lv_poc_state_info_label_array[7].content);
		cRefresh = ret;
	}
}

static void lv_poc_refresh_state_info_network_connection(void)
{
	lv_poc_state_info_network_connection();
	lv_label_set_text(label_array[6], lv_poc_state_info_label_array[6].content);
	lv_label_set_text(label_array[8], lv_poc_state_info_label_array[8].content);
}

static void lv_poc_refresh_state_info_battery(void)
{
	lv_poc_state_info_battery();
	lv_label_set_text(label_array[3], lv_poc_state_info_label_array[3].content);
}

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
	lv_poc_state_info_signalbdm();
	lv_poc_state_info_battery();
	lv_poc_state_info_network_server();
	lv_poc_state_info_network_connection();

    for(int i = 0;i < label_array_size; i++)
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
		label_array[i] = label;
	}
	state_task = lv_task_create(lv_poc_info_menu_task, 1000, LV_TASK_PRIO_MID, NULL);
	lv_poc_status_info_menu_task_ext_add(lv_poc_refresh_state_info_network_server);
	lv_poc_status_info_menu_task_ext_add(lv_poc_refresh_state_info_signalbdm);
	lv_poc_status_info_menu_task_ext_add(lv_poc_refresh_state_info_battery);
	lv_poc_status_info_menu_task_ext_add(lv_poc_refresh_state_info_network_connection);
    lv_list_set_btn_selected(list, btn_array[0]);
	//free
	if(btn_array != NULL)
	{
		lv_mem_free(btn_array);
		btn_array = NULL;
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
					if(state_task)
					{
						lv_task_del(state_task);
					}
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

