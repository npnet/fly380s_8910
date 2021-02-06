#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include "stdlib.h"

#define SUPPORT_PREES_CB_CIT
#define POCCITLISTITEMMAX 5

typedef void(* lv_poc_cit_btn_func_t)(lv_obj_t *obj);

static lv_poc_cit_btn_func_t lv_poc_cit_btn_func_items[POCCITLISTITEMMAX] = {0};

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * cit_list_create(lv_obj_t * parent, lv_area_t display_area);

static void cit_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static void lv_poc_teaminal_info_cb(lv_obj_t * obj);

static void lv_poc_auto_check_cb(lv_obj_t * obj);

static void lv_poc_part_check_cb(lv_obj_t * obj);

static void lv_poc_log_switch_cb(lv_obj_t * obj);

static lv_poc_win_t * cit_win = NULL;

static lv_obj_t * activity_list = NULL;

static osiMutex_t * mutex = NULL;

lv_poc_activity_t * poc_cit_activity = NULL;

struct lv_poc_cit_main_t
{
	bool ready;
	bool autotest;
};

struct lv_poc_cit_main_t poc_cit_main_attr = {0};

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
    cit_win = lv_poc_win_create(display, "工厂测试工具", cit_list_create);
    return (lv_obj_t *)cit_win;
}

static void activity_destory(lv_obj_t *obj)
{
	if(cit_win != NULL)
	{
		lv_mem_free(cit_win);
		cit_win = NULL;
	}
	poc_cit_activity = NULL;
}

static void * cit_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, cit_list_config);
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
	void(* lv_poc_item_press_cb)(lv_obj_t *obj);
} lv_poc_cit_label_struct_t;

lv_poc_cit_label_struct_t lv_poc_cit_label_array[] = {
    {
        NULL,
        "检1"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        "终端信息"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
        lv_poc_teaminal_info_cb,
    },

    {
        NULL,
        "检2"                   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        "自动检测"					 , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
        lv_poc_auto_check_cb,
    },

	{
		NULL,
		"检3"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"部件测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_part_check_cb,
	},

	{
		NULL,
		"检4"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		"log开关"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
		lv_poc_log_switch_cb,
	},
};

static void lv_poc_teaminal_info_cb(lv_obj_t * obj)
{
	lv_poc_terminfo_open();
}

static void lv_poc_auto_check_cb(lv_obj_t * obj)
{
	lvPocCitAutoTestActiOpen();
}

static void lv_poc_part_check_cb(lv_obj_t * obj)
{
	lv_poc_cit_part_test_open();
}

static void lv_poc_log_switch_cb(lv_obj_t * obj)
{
	lv_poc_logswitch_open();
}

#ifdef SUPPORT_PREES_CB_CIT
static void lv_poc_cit_pressed_cb(lv_obj_t * obj, lv_event_t event)
{
    if(LV_EVENT_CLICKED == event || LV_EVENT_PRESSED == event)
    {
		int index = lv_list_get_btn_index((lv_obj_t *)cit_win->display_obj, obj);

		lv_poc_cit_btn_func_t func = lv_poc_cit_btn_func_items[index];
		func(obj);
    }
}
#endif

static void cit_list_config(lv_obj_t * list, lv_area_t list_area)
{
    lv_obj_t *btn = NULL;
    lv_obj_t *label = NULL;
    lv_obj_t *btn_label = NULL;
    lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
    lv_coord_t btn_width = (list_area.x2 - list_area.x1);
    lv_style_t * style_label = NULL;
    poc_setting_conf = lv_poc_setting_conf_read();
    style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
    style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;

    int label_array_size = sizeof(lv_poc_cit_label_array)/sizeof(lv_poc_cit_label_struct_t);
    lv_obj_t ** btn_array = (lv_obj_t **)lv_mem_alloc(sizeof(lv_obj_t *) * label_array_size);

    for(int i = 0; i < label_array_size; i++)
    {
        btn = lv_list_add_btn(list, NULL, lv_poc_cit_label_array[i].title);
        btn_array[i] = btn;
        lv_btn_set_fit(btn, LV_FIT_NONE);
        lv_obj_set_height(btn, btn_height);
        btn_label = lv_list_get_btn_label(btn);
        label = lv_label_create(btn, NULL);
        btn->user_data = (void *)label;

        lv_label_set_text(label, lv_poc_cit_label_array[i].content);

        lv_label_set_long_mode(btn_label, lv_poc_cit_label_array[i].content_long_mode);
        lv_label_set_align(btn_label, lv_poc_cit_label_array[i].content_text_align);
        lv_label_set_long_mode(label, lv_poc_cit_label_array[i].content_long_mode);
        lv_label_set_align(label, lv_poc_cit_label_array[i].content_text_align);

        lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);

        lv_obj_set_width(btn_label, btn_width/4);
        lv_obj_set_width(label, btn_width - lv_obj_get_width(btn_label));
        lv_obj_align(btn_label, btn, lv_poc_cit_label_array[i].title_align, lv_poc_cit_label_array[i].content_align_x, lv_poc_cit_label_array[i].content_align_y);
        lv_obj_align(label, btn_label, lv_poc_cit_label_array[i].content_align, lv_poc_cit_label_array[i].content_align_x, lv_poc_cit_label_array[i].content_align_y);
#ifdef SUPPORT_PREES_CB_CIT
		lv_obj_set_click(btn, true);
		lv_obj_set_event_cb(btn, lv_poc_cit_pressed_cb);
#endif
		lv_poc_cit_btn_func_items[i] = lv_poc_cit_label_array[i].lv_poc_item_press_cb;
	}
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
					if(pubPocCitAutoTestMode())
					{
						OSI_PRINTFI("[cit]auto moding");
					}
					else
					{
						OSI_PRINTFI("[cit]cit main");
						lv_signal_send(activity_list, LV_SIGNAL_PRESSED, NULL);
					}
				}

				case LV_GROUP_KEY_DOWN:

				case LV_GROUP_KEY_UP:
				{
					if(pubPocCitAutoTestMode())
					{
						OSI_PRINTFI("[cit]auto moding");
					}
					else
					{
						OSI_PRINTFI("[cit]cit main");
						lv_signal_send(activity_list, LV_SIGNAL_CONTROL, param);
					}
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
					lvPocGuiBndCom_cit_status(POC_CIT_EXIT);
					lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, false);
					lv_poc_del_activity(poc_cit_activity);
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


void lv_poc_cit_open(void)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_CIT_MAIN,
											activity_create,
											activity_destory};
	lv_poc_control_text_t control = {"确定", "", "取消"};
	if(poc_cit_activity != NULL)
	{
		return;
	}

	if(mutex == NULL)
	{
		mutex = osiMutexCreate();
	}

	mutex ? osiMutexLock(mutex) : 0;
	lvPocGuiBndCom_cit_status(POC_CIT_ENTER);
    poc_cit_activity = lv_poc_create_activity(&activity_ext, true, true, &control);
    lv_poc_activity_set_signal_cb(poc_cit_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_cit_activity, design_func);
	mutex ? osiMutexUnlock(mutex) : 0;
}

#ifdef __cplusplus
}
#endif
























