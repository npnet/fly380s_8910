#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include "audio_device.h"
#include "audio_player.h"

#define LV_POC_RECORD_PLAYBACK_ITEMS_NUM (5)

typedef void (* lv_poc_record_playback_item_func_t)(lv_obj_t * obj);

static lv_poc_win_t * poc_record_playback_win;

static lv_poc_record_playback_item_func_t lv_poc_record_playback_items_funcs[LV_POC_RECORD_PLAYBACK_ITEMS_NUM] = {0};

static lv_obj_t * poc_record_playback_create(lv_poc_display_t *display);

static void poc_record_playback_destory(lv_obj_t *obj);

static void * poc_record_playback_list_create(lv_obj_t * parent, lv_area_t display_area);

static void poc_record_playback_list_config(lv_obj_t * list, lv_area_t list_area);

static void lv_poc_record_playback_btn_1_cb(lv_obj_t * obj);

static void lv_poc_record_playback_btn_2_cb(lv_obj_t * obj);

static void lv_poc_record_playback_btn_3_cb(lv_obj_t * obj);

static void lv_poc_record_playback_btn_4_cb(lv_obj_t * obj);

static void lv_poc_record_playback_btn_5_cb(lv_obj_t * obj);

static void lv_poc_record_playback_thread(void * ctx);

static void lv_poc_record_playback_pressed_cb(lv_obj_t * obj, lv_event_t event);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_obj_t *activity_list;

lv_poc_activity_t *poc_record_playback_activity;

static const char *pcmtofile[5] = {
	"/pcmfile1.pcm",
	"/pcmfile2.pcm",
	"/pcmfile3.pcm",
	"/pcmfile4.pcm",
	"/pcmfile5.pcm",
};

struct lv_poc_cit_record_playback_t
{
    osiThread_t *thread;
	auPlayer_t *player;
	lv_obj_t *obj[2];
	int index;
};

struct lv_poc_cit_record_playback_t poc_record_playback_attr = {0};

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
	void(*cb)(lv_obj_t *obj);
} lv_poc_record_playback_label_struct_t;

lv_poc_record_playback_label_struct_t lv_poc_record_playback_label_array[] = {
    {
        NULL,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_record_playback_btn_1_cb,
    },

    {
        NULL,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_record_playback_btn_2_cb,
    },
    {
        NULL,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_record_playback_btn_3_cb,
    },

    {
        NULL,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_record_playback_btn_4_cb,
    },
    {
        NULL,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        ""                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_record_playback_btn_5_cb,
    },
};


static lv_obj_t * poc_record_playback_create(lv_poc_display_t *display)
{
    poc_record_playback_win = lv_poc_win_create(display, "录音回放", poc_record_playback_list_create);
	lv_poc_notation_refresh();
    return (lv_obj_t  *)poc_record_playback_win;
}

static void poc_record_playback_destory(lv_obj_t *obj)
{
	if(poc_record_playback_win != NULL)
	{
		lv_mem_free(poc_record_playback_win);
		poc_record_playback_win = NULL;
	}
	poc_record_playback_activity = NULL;
	memset(&poc_record_playback_attr, 0, sizeof(struct lv_poc_cit_record_playback_t));
}

static void * poc_record_playback_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, poc_record_playback_list_config);
    return (void *)activity_list;
}

static void poc_record_playback_list_config(lv_obj_t * list, lv_area_t list_area)
{
	lv_obj_t *btn;
	lv_obj_t *label;
	lv_obj_t *btn_label;
	lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
	lv_coord_t btn_width = (list_area.x2 - list_area.x1);
	lv_style_t * style_label;

	struct PocPcmToFileAttr_s fileInfo;
	fileInfo = lv_poc_pcm_file_attr();

	poc_setting_conf = lv_poc_setting_conf_read();
	style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
	style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;

	int label_array_size = 0;
	fileInfo.number == 0 ? (label_array_size = 1) : (label_array_size = fileInfo.number);

	lv_obj_t ** btn_array = (lv_obj_t **)lv_mem_alloc(sizeof(lv_obj_t *) * label_array_size);

	for(int i = 0; i < label_array_size; i++)
	{
		if(fileInfo.number == 0)
		{
			poc_record_playback_attr.index = 10;
			btn = lv_list_add_btn(list, NULL, "无录音");
		}
		else
		{
			btn = lv_list_add_btn(list, NULL, fileInfo.InfoAttr_s[i].time);
		}
		btn_array[i] = btn;
		lv_btn_set_fit(btn, LV_FIT_NONE);
		lv_obj_set_height(btn, btn_height);
		btn_label = lv_list_get_btn_label(btn);
		label = lv_label_create(btn, NULL);
		btn->user_data = (void *)label;

		if(fileInfo.number == 0)
		{
			lv_label_set_text(label, "空");
		}
		else
		{
			lv_label_set_text(label, "开始");
		}

		lv_label_set_long_mode(btn_label, lv_poc_record_playback_label_array[i].content_long_mode);
		lv_label_set_align(btn_label, lv_poc_record_playback_label_array[i].content_text_align);
		lv_label_set_long_mode(label, lv_poc_record_playback_label_array[i].content_long_mode);
		lv_label_set_align(label, lv_poc_record_playback_label_array[i].content_text_align);

		lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);

		lv_obj_set_width(btn_label, btn_width*3/5 + 10);
		lv_obj_set_width(label, btn_width - lv_obj_get_width(btn_label) - 10);
		lv_obj_align(btn_label, btn, lv_poc_record_playback_label_array[i].title_align, lv_poc_record_playback_label_array[i].content_align_x, lv_poc_record_playback_label_array[i].content_align_y);
		lv_obj_align(label, btn_label, lv_poc_record_playback_label_array[i].content_align, lv_poc_record_playback_label_array[i].content_align_x, lv_poc_record_playback_label_array[i].content_align_y);

		lv_obj_set_click(btn, true);
		lv_obj_set_event_cb(btn, lv_poc_record_playback_pressed_cb);

		lv_poc_record_playback_items_funcs[i] = lv_poc_record_playback_label_array[i].cb;
	}
	lv_list_set_btn_selected(list, btn_array[0]);
	//free
	if(btn_array != NULL)
	{
		lv_mem_free(btn_array);
		btn_array = NULL;
	}
}

static void lv_poc_record_playback_btn_1_cb(lv_obj_t * obj)
{
	if(poc_record_playback_attr.thread != NULL
		|| obj->user_data == NULL
		|| poc_record_playback_attr.index == 10)
	{
		return;
	}
	poc_record_playback_attr.index = 0;
	poc_record_playback_attr.obj[0] = obj->user_data;
	obj->user_data ? lv_label_set_text(obj->user_data, "播放") : 0;
    poc_record_playback_attr.thread = osiThreadCreate("record playback", lv_poc_record_playback_thread, NULL, OSI_PRIORITY_NORMAL, 1024*5, 64);
}

static void lv_poc_record_playback_btn_2_cb(lv_obj_t * obj)
{
	if(poc_record_playback_attr.thread != NULL
		|| obj->user_data == NULL
		|| poc_record_playback_attr.index == 10)
	{
		return;
	}
	poc_record_playback_attr.index = 1;
	poc_record_playback_attr.obj[0] = obj->user_data;
	obj->user_data ? lv_label_set_text(obj->user_data, "播放") : 0;
    poc_record_playback_attr.thread = osiThreadCreate("record playback", lv_poc_record_playback_thread, NULL, OSI_PRIORITY_NORMAL, 1024*5, 64);
}

static void lv_poc_record_playback_btn_3_cb(lv_obj_t * obj)
{
	if(poc_record_playback_attr.thread != NULL
		|| obj->user_data == NULL
		|| poc_record_playback_attr.index == 10)
	{
		return;
	}
	poc_record_playback_attr.index = 2;
	poc_record_playback_attr.obj[0] = obj->user_data;
	obj->user_data ? lv_label_set_text(obj->user_data, "播放") : 0;
    poc_record_playback_attr.thread = osiThreadCreate("record playback", lv_poc_record_playback_thread, NULL, OSI_PRIORITY_NORMAL, 1024*5, 64);
}

static void lv_poc_record_playback_btn_4_cb(lv_obj_t * obj)
{
	if(poc_record_playback_attr.thread != NULL
		|| obj->user_data == NULL
		|| poc_record_playback_attr.index == 10)
	{
		return;
	}
	poc_record_playback_attr.index = 3;
	poc_record_playback_attr.obj[0] = obj->user_data;
	obj->user_data ? lv_label_set_text(obj->user_data, "播放") : 0;
    poc_record_playback_attr.thread = osiThreadCreate("record playback", lv_poc_record_playback_thread, NULL, OSI_PRIORITY_NORMAL, 1024*5, 64);
}

static void lv_poc_record_playback_btn_5_cb(lv_obj_t * obj)
{
	if(poc_record_playback_attr.thread != NULL
		|| obj->user_data == NULL
		|| poc_record_playback_attr.index == 10)
	{
		return;
	}
	poc_record_playback_attr.index = 4;
	poc_record_playback_attr.obj[0] = obj->user_data;
	obj->user_data ? lv_label_set_text(obj->user_data, "播放") : 0;
    poc_record_playback_attr.thread = osiThreadCreate("record playback", lv_poc_record_playback_thread, NULL, OSI_PRIORITY_NORMAL, 1024*5, 64);
}

static void lv_poc_record_playback_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
	int index = 0;
    if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
    {
	    index = lv_list_get_btn_index((lv_obj_t *)poc_record_playback_win->display_obj, obj);
	    lv_poc_record_playback_item_func_t func = lv_poc_record_playback_items_funcs[index];
	    func(obj);
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

				case LV_GROUP_KEY_ESC:
				{
					lv_poc_del_activity(poc_record_playback_activity);
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

		case LV_SIGNAL_LONG_PRESS_REP:
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

void lv_poc_record_playback_open(void)
{
    static lv_poc_activity_ext_t  activity_main_menu_ext = {ACT_ID_POC_RECORD_PALYBACK,
															poc_record_playback_create,
															poc_record_playback_destory};
	if(poc_record_playback_activity != NULL)
	{
		return;
	}
    poc_record_playback_activity = lv_poc_create_activity(&activity_main_menu_ext, true, false, NULL);
    lv_poc_activity_set_signal_cb(poc_record_playback_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_record_playback_activity, design_func);
}

static
void lv_poc_record_playback_thread(void * ctx)
{
	if(poc_record_playback_attr.player != NULL)
	{
		osiThreadExit();
	}

	poc_record_playback_attr.player = auPlayerCreate();
	if(poc_record_playback_attr.player == NULL)
	{
		osiThreadExit();
	}

	auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
	auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};

	while(1)
	{
		if(poc_record_playback_attr.index >= 0
			&& poc_record_playback_attr.index <= 4)
		{
			bool status = auPlayerStartFile(poc_record_playback_attr.player, AUSTREAM_FORMAT_PCM, params, pcmtofile[poc_record_playback_attr.index]);
			poc_record_playback_attr.index = 6;//cannot 0-4 and 10
			OSI_LOGXI(OSI_LOGPAR_S, 0, "[poc][playback]play:(%s)", status ? "success" : "failed");
		}

		if(auPlayerWaitFinish(poc_record_playback_attr.player, OSI_WAIT_FOREVER))
		{
			OSI_LOGI(0, "[poc][playback]play:finish");
			auPlayerDelete(poc_record_playback_attr.player);
			poc_record_playback_attr.player = NULL;
			poc_record_playback_attr.thread = NULL;
			poc_record_playback_attr.obj[0] ? lv_label_set_text(poc_record_playback_attr.obj[0], "结束") : 0;
			poc_record_playback_attr.obj[0] = NULL;
			osiThreadExit();
		}
		osiThreadSleep(500);
	}
}

#ifdef __cplusplus
}
#endif

