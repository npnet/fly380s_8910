
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include "abup_fota.h"

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * fota_list_create(lv_obj_t * parent, lv_area_t display_area);

static void fota_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_poc_win_t * fota_win = NULL;

static lv_obj_t * activity_list = NULL;

lv_poc_activity_t * poc_fota_update_activity = NULL;

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    fota_win = lv_poc_win_create(display, "软件更新", fota_list_create);
    return (lv_obj_t *)fota_win;
}

static void activity_destory(lv_obj_t *obj)
{
}

static void * fota_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, fota_list_config);
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
} lv_poc_fota_label_struct_t;


static char lv_poc_fota_text_cur_version[64] = {0};
static char lv_poc_fota_text_cur_status[64] = {0};
static lv_task_t * monitor_check_update = NULL;
static bool is_enter_fota_acti = true;
static bool fota_check_act_status = false;

lv_poc_fota_label_struct_t lv_poc_fota_label_array[] = {
    {
        NULL,
        "状态"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        lv_poc_fota_text_cur_status, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
    },

    {
        NULL,
        "版本"                   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        lv_poc_fota_text_cur_version, LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
    },
};

static void lv_poc_fota_check_update_cb(lv_task_t * task)
{
	static uint8_t fota_status = ABUP_FOTA_START;
	static uint8_t last_fota_status = ABUP_FOTA_END;

	fota_status = abup_update_status();

	if(last_fota_status == fota_status
		|| task->user_data == NULL)
	{
		return;
	}
	last_fota_status = fota_status;
	fota_check_act_status = false;

	switch(fota_status)
	{
		case ABUP_FOTA_IDLEI://FOTA BUSY
		{
			strcpy(lv_poc_fota_text_cur_status, "FOTA BUSY");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_ERROR://升级异常
		{
			strcpy(lv_poc_fota_text_cur_status, "升级异常");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_CHECK://检查版本
		{
			strcpy(lv_poc_fota_text_cur_status, "检查版本");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			OSI_LOGI(0, "[abup](%d):check version", __LINE__);
			break;
		}

		case ABUP_FOTA_READY://准备环境
		{
			strcpy(lv_poc_fota_text_cur_status, "准备环境");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			fota_check_act_status = true;
			break;
		}

		case ABUP_FOTA_RMI://注册设备信息
		{
			strcpy(lv_poc_fota_text_cur_status, "注册设备信息");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			fota_check_act_status = true;
			break;
		}

		case ABUP_FOTA_CVI://检测版本
		{
			strcpy(lv_poc_fota_text_cur_status, "检测版本");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			fota_check_act_status = true;
			OSI_LOGI(0, "[abup](%d):check version", __LINE__);
			break;
		}

		case ABUP_FOTA_DLI:
		{
			strcpy(lv_poc_fota_text_cur_status, "下载升级包");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			OSI_LOGI(0, "[abup](%d):download package", __LINE__);
			break;
		}

		case ABUP_FOTA_NO_NETWORK://无网络
		{
			strcpy(lv_poc_fota_text_cur_status, "无网络");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			OSI_LOGI(0, "[abup](%d):no network", __LINE__);
			break;
		}

		case ABUP_FOTA_DOWNLOAD_FAILED://下载失败
		{
			strcpy(lv_poc_fota_text_cur_status, "下载失败");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			OSI_LOGI(0, "[abup](%d):no network", __LINE__);
			break;
		}

		case ABUP_FOTA_NO_NEW_VERSION://无新版本
		{
			strcpy(lv_poc_fota_text_cur_status, "无新版本");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_NOT_ENOUGH_SPACE://空间不足
		{
			strcpy(lv_poc_fota_text_cur_status, "空间不足");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_NO_ACCESS_TIMES://当天访问次数上限
		{
			strcpy(lv_poc_fota_text_cur_status, "当天访问次数上限");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_UART_TIMEOUT: //串口超时
		{
			strcpy(lv_poc_fota_text_cur_status, "串口超时");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_DNS_FAIL://DNS解析失败
		{
			strcpy(lv_poc_fota_text_cur_status, "DNS解析失败");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_CREATE_SOCKET_FAIL://建立socket失败
		{
			strcpy(lv_poc_fota_text_cur_status, "建立socket失败");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}
		case ABUP_FOTA_NETWORK_ERROR://网络错误
		{
			strcpy(lv_poc_fota_text_cur_status, "网络错误");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_MD5_NOT_MATCH://MD5校验失败
		{
			strcpy(lv_poc_fota_text_cur_status, "MD5校验失败");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_ERASE_FLASH://擦除flash
		{
			strcpy(lv_poc_fota_text_cur_status, "擦除flash");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_WRITE_FLASH://写flash
		{
			strcpy(lv_poc_fota_text_cur_status, "写flash");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		case ABUP_FOTA_REBOOT_UPDATE://准备重启更新
		{
			strcpy(lv_poc_fota_text_cur_status, "准备重启更新");
			lv_label_set_text((lv_obj_t *)task->user_data, lv_poc_fota_text_cur_status);
			lv_task_del(monitor_check_update);
			monitor_check_update = NULL;
			break;
		}

		default:
			break;
	}
}

static void lv_poc_fota_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
    if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
    {
        //open software update
        if(!(abup_update_status() == ABUP_FOTA_IDLEI
			|| abup_update_status() == ABUP_FOTA_CHECK
			|| abup_update_status() == ABUP_FOTA_READY
			|| abup_update_status() == ABUP_FOTA_RMI
			|| abup_update_status() == ABUP_FOTA_CVI
			|| abup_update_status() == ABUP_FOTA_DLI))
        {
			if(obj->user_data != NULL)
			{
				fota_check_act_status = true;
				//fota
				abup_check_version();
				//view
				strcpy(lv_poc_fota_text_cur_status, "正在更新");
				lv_label_set_text((lv_obj_t *)obj->user_data, lv_poc_fota_text_cur_status);
				if(monitor_check_update == NULL)
				{
					monitor_check_update = lv_task_create(lv_poc_fota_check_update_cb, 500, LV_TASK_PRIO_MID, (void *)obj->user_data);
				}
			}
		}
    }
}

static void fota_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn = NULL;
    lv_obj_t *label = NULL;
    lv_obj_t *btn_label = NULL;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_style_t * style_label = NULL;
    poc_setting_conf = lv_poc_setting_conf_read();
    style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;
    style_label->text.font = (lv_font_t *)poc_setting_conf->font.fota_label_current_font;

    int label_array_size = sizeof(lv_poc_fota_label_array)/sizeof(lv_poc_fota_label_struct_t);
    lv_obj_t ** btn_array = (lv_obj_t **)lv_mem_alloc(sizeof(lv_obj_t *) * label_array_size);

	strcpy(lv_poc_fota_text_cur_version, "8910DM_FT02_IDT_V1.3_0.0.1");
	strcpy(lv_poc_fota_text_cur_status, "检查版本");

    for(int i = 0; i < label_array_size; i++)
    {
        btn = lv_list_add_btn(list, NULL, lv_poc_fota_label_array[i].title);
        btn_array[i] = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn_label = lv_list_get_btn_label(btn);
        label = lv_label_create(btn, NULL);
        btn->user_data = (void *)label;

        lv_label_set_text(label, lv_poc_fota_label_array[i].content);

        lv_label_set_long_mode(btn_label, lv_poc_fota_label_array[i].content_long_mode);
        lv_label_set_align(btn_label, lv_poc_fota_label_array[i].content_text_align);
        lv_label_set_long_mode(label, lv_poc_fota_label_array[i].content_long_mode);
        lv_label_set_align(label, lv_poc_fota_label_array[i].content_text_align);

        lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);

        lv_obj_set_width(btn_label, btn_width/4);
        lv_obj_set_width(label, btn_width - lv_obj_get_width(btn_label));
        lv_obj_align(btn_label, btn, lv_poc_fota_label_array[i].title_align, lv_poc_fota_label_array[i].content_align_x, lv_poc_fota_label_array[i].content_align_y);
        lv_obj_align(label, btn_label, lv_poc_fota_label_array[i].content_align, lv_poc_fota_label_array[i].content_align_x, lv_poc_fota_label_array[i].content_align_y);
    }
    lv_list_set_btn_selected(list, btn_array[0]);

#ifdef CONFIG_POC_FOTA_SUPPORT
    lv_obj_set_click(btn_array[0], true);
    lv_obj_set_event_cb(btn_array[0], lv_poc_fota_pressed_cb);
#endif
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
					if((monitor_check_update == NULL
						|| abup_system_run_status())
						&& fota_check_act_status == false)
					{
						lv_poc_del_activity(poc_fota_update_activity);
						abup_set_status(ABUP_FOTA_START);
					}
					else
					{
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LAUNCH_NOTE_MSG, (const uint8_t *)"正在检查更新", (const uint8_t *)"请勿退出");
					}
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


void lv_poc_fota_update_open(void)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_UPDATE_SOFTWARE,
											activity_create,
											activity_destory};
    poc_fota_update_activity = lv_poc_create_activity(&activity_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_fota_update_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_fota_update_activity, design_func);
	is_enter_fota_acti ? abup_set_status(ABUP_FOTA_START) : 0;
	is_enter_fota_acti = false;
}



#ifdef __cplusplus
}
#endif

