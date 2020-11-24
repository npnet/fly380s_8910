#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include "uart3_gps.h"
#include "gps_nmea.h"
#include "stdlib.h"

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * gps_list_create(lv_obj_t * parent, lv_area_t display_area);

static void gps_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_poc_win_t * gps_win;

static lv_obj_t * activity_list;

lv_poc_activity_t * poc_gps_monitor_activity;

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    gps_win = lv_poc_win_create(display, "GPS", gps_list_create);
    return (lv_obj_t *)gps_win;
}

static void activity_destory(lv_obj_t *obj)
{
	poc_gps_monitor_activity = NULL;
}

static void * gps_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, gps_list_config);
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
} lv_poc_gps_label_struct_t;

static char lv_poc_gps_text_cur_status[64] = {0};
static char lv_poc_gps_text_cur_type[64] = {0};
static char lv_poc_gps_text_cur_dat[64] = {0};
static char lv_poc_gps_text_cur_tim[64] = {0};
static char lv_poc_gps_text_cur_long[64] = {0};
static char lv_poc_gps_text_cur_lati[64] = {0};
static char lv_poc_gps_text_cur_speed[64] = {0};
static char lv_poc_gps_text_cur_last_location_time[64] = {0};
static char lv_poc_gps_text_cur_current_location_time[64] = {0};
static char lv_poc_gps_text_cur_poweron_location_number[64] = {0};
static lv_task_t * monitor_gps_info = NULL;
static osiTimer_t * gps_once_timer = NULL;

lv_poc_gps_label_struct_t lv_poc_gps_label_array[] = {
    {
        NULL,
        "状态"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        lv_poc_gps_text_cur_status, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
    },

    {
        NULL,
        "类型"                   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        lv_poc_gps_text_cur_type, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
    },

	{
		NULL,
		"DAT"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_gps_text_cur_dat, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"TIM"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_gps_text_cur_tim, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"经度"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_gps_text_cur_long, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"纬度"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_gps_text_cur_lati, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,
		"速度"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_gps_text_cur_speed, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,//倒计时20s
		"定位"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_gps_text_cur_last_location_time, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,//本轮定位20s
		"时长"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_gps_text_cur_current_location_time, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		NULL,//第n次定位
		"开机"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		lv_poc_gps_text_cur_poweron_location_number, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},
};

static void lv_poc_gps_check_monitor_cb(lv_task_t * task)
{
	lv_obj_t ** btn_array = (lv_obj_t **)task->user_data;
	//gps info
	nmea_msg *gps_info = (nmea_msg *)publvPocGpsIdtComDataInfo();
	//gps status
	lv_obj_t *gps_status_btn = lv_list_get_btn_label(btn_array[0]);
	switch(gps_info->gpssta)
	{
		case 1:
		{
			lv_label_set_text(gps_status_btn, "非差分定位");
			break;
		}

		case 2:
		{
			lv_label_set_text(gps_status_btn, "差分定位");
			break;
		}

		case 6:
		{
			lv_label_set_text(gps_status_btn, "正在估算");
			break;
		}

		default:
		{
			lv_label_set_text(gps_status_btn, "未定位");
			break;
		}
	}

	//gps type
	lv_obj_t *gps_type_btn = lv_list_get_btn_label(btn_array[1]);
	switch(gps_info->fixmode)
	{
		case 2:
		{
			lv_label_set_text(gps_type_btn, "2D定位");
			break;
		}

		case 3:
		{
			lv_label_set_text(gps_type_btn, "3D定位");
			break;
		}

		default:
		{
			lv_label_set_text(gps_type_btn, "没有定位");
			break;
		}
	}

	//gps utc--dat
	char gpsyear[12] = {0};
	char gpsmonth[12] = {0};
	char gpsdate[12] = {0};
	char gpshour[12] = {0};
	char gpsmin[12] = {0};
	char gpssec[12] = {0};

	lv_obj_t *gps_utc_dat_btn = lv_list_get_btn_label(btn_array[2]);
	__itoa(gps_info->utc.year, (char *)&gpsyear, 10);
	strcpy(lv_poc_gps_text_cur_dat, gpsyear);
	strcat(lv_poc_gps_text_cur_dat, "/");
	__itoa(gps_info->utc.month, (char *)&gpsmonth, 10);
	strcat(lv_poc_gps_text_cur_dat, gpsmonth);
	strcat(lv_poc_gps_text_cur_dat, "/");
	__itoa(gps_info->utc.date, (char *)&gpsdate, 10);
	strcat(lv_poc_gps_text_cur_dat, gpsdate);
	lv_label_set_text(gps_utc_dat_btn, lv_poc_gps_text_cur_dat);

	//gps utc--tim
	lv_obj_t *gps_utc_tim_btn = lv_list_get_btn_label(btn_array[3]);
	__itoa((gps_info->utc.hour + 8), (char *)&gpshour, 10);
	strcpy(lv_poc_gps_text_cur_tim, gpshour);
	strcat(lv_poc_gps_text_cur_tim, ":");
	__itoa(gps_info->utc.min, (char *)&gpsmin, 10);
	strcat(lv_poc_gps_text_cur_tim, gpsmin);
	strcat(lv_poc_gps_text_cur_tim, ":");
	__itoa(gps_info->utc.sec, (char *)&gpssec, 10);
	strcat(lv_poc_gps_text_cur_tim, gpssec);
	lv_label_set_text(gps_utc_tim_btn, lv_poc_gps_text_cur_tim);

	//gps long
	char longstr[24] = {0};

	lv_obj_t *gps_long_btn = lv_list_get_btn_label(btn_array[4]);
	__itoa(gps_info->longitude, (char *)&longstr, 10);
	if(gps_info->ewhemi == 'W')
	{
		strcpy(lv_poc_gps_text_cur_long, "W");
	}
	else
	{
		strcpy(lv_poc_gps_text_cur_long, "E");
	}
	strcat(lv_poc_gps_text_cur_long, "-");
	strcat(lv_poc_gps_text_cur_long, longstr);
	lv_label_set_text(gps_long_btn, lv_poc_gps_text_cur_long);

	//gps lati
	char latistr[24] = {0};

	lv_obj_t *gps_lati_btn = lv_list_get_btn_label(btn_array[5]);
	__itoa(gps_info->latitude, (char *)&latistr, 10);
	if(gps_info->nshemi == 'S')
	{
		strcpy(lv_poc_gps_text_cur_lati, "S");
	}
	else
	{
		strcpy(lv_poc_gps_text_cur_lati, "N");
	}
	strcat(lv_poc_gps_text_cur_lati, "-");
	strcat(lv_poc_gps_text_cur_lati, latistr);
	lv_label_set_text(gps_lati_btn, lv_poc_gps_text_cur_lati);

	//gps speed
	char speedstr[32] = {0};

	lv_obj_t *gps_speed_btn = lv_list_get_btn_label(btn_array[6]);
	__itoa(gps_info->speed, (char *)&speedstr, 10);
	strcpy(lv_poc_gps_text_cur_speed, speedstr);
	strcat(lv_poc_gps_text_cur_speed, "m/s");
	lv_label_set_text(gps_speed_btn, lv_poc_gps_text_cur_speed);

	//next gps location countdown time
	char countdownstr[32] = {0};

	lv_obj_t *location_countdown = lv_list_get_btn_label(btn_array[7]);
	__itoa(publvPocGpsIdtComGetLocationCountDown(), (char *)&countdownstr, 10);
	strcpy(lv_poc_gps_text_cur_last_location_time, "倒计时");
	strcat(lv_poc_gps_text_cur_last_location_time, countdownstr);
	strcat(lv_poc_gps_text_cur_last_location_time, "s");
	lv_label_set_text(location_countdown, lv_poc_gps_text_cur_last_location_time);

	//this gps location time
	char location_timestr[32] = {0};

	lv_obj_t *current_location_time= lv_list_get_btn_label(btn_array[8]);
	__itoa(publvPocGpsIdtComGetThisLocationTime(), (char *)&location_timestr, 10);
	strcpy(lv_poc_gps_text_cur_current_location_time, "本轮定位");
	strcat(lv_poc_gps_text_cur_current_location_time, location_timestr);
	strcat(lv_poc_gps_text_cur_current_location_time, "s");
	lv_label_set_text(current_location_time, lv_poc_gps_text_cur_current_location_time);

	//power on location number
	char location_numberstr[32] = {0};

	lv_obj_t *poweron_location_number= lv_list_get_btn_label(btn_array[9]);
	__itoa(publvPocGpsIdtComGetPoweronLocationNumber(), (char *)&location_numberstr, 10);
	strcpy(lv_poc_gps_text_cur_poweron_location_number, "第");
	strcat(lv_poc_gps_text_cur_poweron_location_number, location_numberstr);
	strcat(lv_poc_gps_text_cur_poweron_location_number, "次定位");
	lv_label_set_text(poweron_location_number, lv_poc_gps_text_cur_poweron_location_number);
}

#ifdef SUPPORT_PREES_CB
static void lv_poc_gps_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
    if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
    {

    }
}
#endif

static void LvGuiIdtCom_once_refresh_gpsinfo_timer_cb(void *ctx)
{
	lv_task_set_period(monitor_gps_info, 2000);
}

static void gps_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn;
    lv_obj_t *label;
    lv_obj_t *btn_label;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_style_t * style_label;
    poc_setting_conf = lv_poc_setting_conf_read();
    style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;
    style_label->text.font = (lv_font_t *)poc_setting_conf->font.fota_label_current_font;

    int label_array_size = sizeof(lv_poc_gps_label_array)/sizeof(lv_poc_gps_label_struct_t);
    lv_obj_t ** btn_array = (lv_obj_t **)lv_mem_alloc(sizeof(lv_obj_t *) * label_array_size);

    for(int i = 0; i < label_array_size; i++)
    {
        btn = lv_list_add_btn(list, NULL, lv_poc_gps_label_array[i].title);
        btn_array[i] = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn_label = lv_list_get_btn_label(btn);
        label = lv_label_create(btn, NULL);
        btn->user_data = (void *)label;

        lv_label_set_text(label, lv_poc_gps_label_array[i].content);

        lv_label_set_long_mode(btn_label, lv_poc_gps_label_array[i].content_long_mode);
        lv_label_set_align(btn_label, lv_poc_gps_label_array[i].content_text_align);
        lv_label_set_long_mode(label, lv_poc_gps_label_array[i].content_long_mode);
        lv_label_set_align(label, lv_poc_gps_label_array[i].content_text_align);

        lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);

        lv_obj_set_width(btn_label, btn_width/4);
        lv_obj_set_width(label, btn_width - lv_obj_get_width(btn_label));
        lv_obj_align(btn_label, btn, lv_poc_gps_label_array[i].title_align, lv_poc_gps_label_array[i].content_align_x, lv_poc_gps_label_array[i].content_align_y);
        lv_obj_align(label, btn_label, lv_poc_gps_label_array[i].content_align, lv_poc_gps_label_array[i].content_align_x, lv_poc_gps_label_array[i].content_align_y);
    }
    lv_list_set_btn_selected(list, btn_array[0]);

	if(monitor_gps_info == NULL)
	{
		gps_once_timer = osiTimerCreate(NULL, LvGuiIdtCom_once_refresh_gpsinfo_timer_cb, NULL);
		monitor_gps_info = lv_task_create(lv_poc_gps_check_monitor_cb, 1000, LV_TASK_PRIO_HIGH, (void **)btn_array);
		lv_task_ready(monitor_gps_info);
		osiTimerStart(gps_once_timer, 2000);
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
					if(monitor_gps_info != NULL)
					{
						lv_task_del(monitor_gps_info);
						monitor_gps_info = NULL;
					}
					if(gps_once_timer != NULL)
					{
						osiTimerDelete(gps_once_timer);
						gps_once_timer = NULL;
					}
					lv_poc_del_activity(poc_gps_monitor_activity);
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


void lv_poc_gps_monitor_open(void)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_GPS_MONITOR,
											activity_create,
											activity_destory};

	if(poc_gps_monitor_activity != NULL)
	{
		return;
	}

    poc_gps_monitor_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_gps_monitor_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_gps_monitor_activity, design_func);
}

#ifdef __cplusplus
}
#endif

