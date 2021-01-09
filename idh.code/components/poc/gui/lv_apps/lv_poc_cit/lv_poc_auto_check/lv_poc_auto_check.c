#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include "stdlib.h"

static lv_obj_t * activity_list = NULL;

lv_poc_activity_t * poc_cit_result_activity = NULL;

static lv_poc_win_t * cit_result_win = NULL;

static void * cit_result_list_create(lv_obj_t * parent, lv_area_t display_area);

static void cit_result_list_config(lv_obj_t * list, lv_area_t list_area);

struct lv_poc_cit_auto_test_t
{
	bool ready;
	int testindex;
	int result[LV_POC_CIT_OPRATOR_TYPE_END];
};

struct lv_poc_cit_auto_test_t poc_auto_test_attr = {0};

typedef struct
{
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
} lv_poc_cit_result_label_struct_t;

lv_poc_cit_result_label_struct_t lv_poc_cit_result_label_array[] = {
    {
        "1.系统版本测试"                     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        ""                 , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
    },

    {
        "2.校准测试"                   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
        ""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
    },

	{
		"3.RTC测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"4.背光灯测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"5.LCD测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"6.音量测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"7.麦克风测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"8.信号测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"9.按键测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"10.充电测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"11.GPS测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"12.SIM卡测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

#ifdef CONFIG_POC_GUI_TOUCH_SUPPORT
	{
		"13.手电筒测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},
#endif

	{
		"14.三色灯测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"15.耳机测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""				   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},

	{
		"16.Flash测试"				   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_LEFT, LV_ALIGN_IN_LEFT_MID  ,
		""			   , LV_LABEL_LONG_SROLL, LV_LABEL_ALIGN_LEFT, LV_ALIGN_OUT_RIGHT_MID, 0, 0,
	},
};

#define CIT_TEST_RESULT_ITEMS sizeof(lv_poc_cit_result_label_array)/sizeof(lv_poc_cit_result_label_struct_t)

static
void prvlvPocCitAutoTestComRefresh(lv_task_t *task)
{
	int *id = (int*)task->user_data;
	if(id == NULL)
	{
		OSI_LOGI(0, "[cit][autotest](%d):rec msg, NULL", __LINE__);
		return;
	}
	OSI_LOGI(0, "[cit][autotest](%d):rec msg(%d)", __LINE__, (*id) - 1);
	lv_poc_cit_auto_test_items_perform((*id) - 1);
}

void lvPocCitAutoTestActiOpen(void)
{
	memset(&poc_auto_test_attr, 0, sizeof(struct lv_poc_cit_auto_test_t));
	poc_auto_test_attr.testindex = LV_POC_CIT_OPRATOR_TYPE_CALIBATE;
	poc_auto_test_attr.ready = true;
	lv_poc_cit_auto_test_items_register_cb(true);
	lv_poc_cit_auto_test_items_perform(poc_auto_test_attr.testindex - 1);//auto enter first items
}

void lvPocCitAutoTestActiClose(void)
{
	memset(&poc_auto_test_attr, 0, sizeof(struct lv_poc_cit_auto_test_t));
	lv_poc_cit_auto_test_items_register_cb(false);
}

void lvPocCitAutoTestCom_Msg(lv_poc_cit_auto_test_type result)
{
	if(result == 0 || !poc_auto_test_attr.ready)
	{
		return;
	}
	poc_auto_test_attr.result[poc_auto_test_attr.testindex - 1] = result;//record last result
	poc_auto_test_attr.testindex++;
	OSI_LOGI(0, "[cit][autotest](%d):send msg(%d)", __LINE__, poc_auto_test_attr.testindex - 1);
	if(poc_auto_test_attr.testindex > LV_POC_CIT_OPRATOR_TYPE_END)//auto test finish
	{
		lv_poc_refr_task_once(lv_poc_cit_result_open, LVPOCLISTIDTCOM_LIST_PERIOD_10, LV_TASK_PRIO_HIGH);
	}
	else
	{
		lv_task_t *task = lv_task_create(prvlvPocCitAutoTestComRefresh, 10, LV_TASK_PRIO_HIGH, (void *)&poc_auto_test_attr.testindex);
		lv_task_once(task);
	}
}

bool pubPocCitAutoTestMode(void)
{
	return poc_auto_test_attr.ready;
}

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
	int i;
	for(i = 0; i < CIT_TEST_RESULT_ITEMS; i++)
	{
		OSI_LOGI(0, "[cit][result][title](%d)", poc_auto_test_attr.result[i]);
		if(poc_auto_test_attr.result[i] != LV_POC_CIT_AUTO_TEST_TYPE_SUCCESS)
		{
			break;
		}
	}
	if(i == CIT_TEST_RESULT_ITEMS)
	{
		lv_style_t style_win = {0};
		lv_style_copy(&style_win,&lv_style_scr);
		style_win.text.color = LV_COLOR_GREEN;
		cit_result_win = lv_poc_win_create(display, "测试结果(通过)", cit_result_list_create);
	}
	else
	{
		lv_style_t style_win = {0};
		lv_style_copy(&style_win,&lv_style_scr);
		style_win.text.color = LV_COLOR_RED;
		cit_result_win = lv_poc_win_create(display, "测试结果(失败)", cit_result_list_create);
	}
	return (lv_obj_t *)cit_result_win;
}

static void activity_destory(lv_obj_t *obj)
{
	lv_style_t * style_label;
	style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
	style_label->text.color = LV_COLOR_BLACK;
	poc_cit_result_activity = NULL;
}

static void * cit_result_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, cit_result_list_config);
    lv_poc_notation_refresh();
    return (void *)activity_list;
}

static void cit_result_list_config(lv_obj_t * list, lv_area_t list_area)
{
	lv_obj_t *btn;
	lv_obj_t *label;
	lv_obj_t *btn_label;
	lv_coord_t btn_height = (list_area.y2 - list_area.y1)/LV_POC_LIST_COLUM_COUNT;
	lv_coord_t btn_width = (list_area.x2 - list_area.x1);
	lv_style_t * style_label;
	static lv_style_t error_style_label;
	style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
	style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;

    lv_style_copy(&error_style_label, style_label);
	error_style_label.text.color = LV_COLOR_RED;

	lv_obj_t ** btn_array = (lv_obj_t **)lv_mem_alloc(sizeof(lv_obj_t *) * CIT_TEST_RESULT_ITEMS);

	for(int i = 0; i < CIT_TEST_RESULT_ITEMS; i++)
	{
		btn = lv_list_add_btn(list, NULL, lv_poc_cit_result_label_array[i].title);
		btn_array[i] = btn;
		lv_btn_set_fit(btn, LV_FIT_NONE);
		lv_obj_set_height(btn, btn_height);
		btn_label = lv_list_get_btn_label(btn);
		label = lv_label_create(btn, NULL);
		btn->user_data = (void *)label;

		lv_label_set_long_mode(btn_label, lv_poc_cit_result_label_array[i].content_long_mode);
		lv_label_set_align(btn_label, lv_poc_cit_result_label_array[i].content_text_align);
		lv_label_set_long_mode(label, lv_poc_cit_result_label_array[i].content_long_mode);
		lv_label_set_align(label, lv_poc_cit_result_label_array[i].content_text_align);

		if(poc_auto_test_attr.result[i] == LV_POC_CIT_AUTO_TEST_TYPE_SUCCESS)
		{
			style_label->text.color = LV_COLOR_GREEN;
			lv_label_set_text(label, "成功");
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
		}
		else
		{
			lv_label_set_text(label, "失败");
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, &error_style_label);
		}

		lv_obj_set_width(btn_label, btn_width*2/3);
		lv_obj_set_width(label, btn_width - lv_obj_get_width(btn_label) - 12);
		lv_obj_align(btn_label, btn, lv_poc_cit_result_label_array[i].title_align, lv_poc_cit_result_label_array[i].content_align_x, lv_poc_cit_result_label_array[i].content_align_y);
		lv_obj_align(label, btn_label, lv_poc_cit_result_label_array[i].content_align, lv_poc_cit_result_label_array[i].content_align_x, lv_poc_cit_result_label_array[i].content_align_y);
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
					lvPocCitAutoTestActiClose();
					lv_poc_del_activity(poc_cit_result_activity);
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

void lv_poc_cit_result_open(lv_task_t *task)
{
	lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_CIT_RESULT_UI,
											activity_create,
											activity_destory};
	lv_poc_control_text_t control = {"", "", "返回"};

	poc_setting_conf = lv_poc_setting_conf_read();
	poc_cit_result_activity = lv_poc_create_activity(&activity_ext, true, true, &control);

	lv_poc_activity_set_signal_cb(poc_cit_result_activity, signal_func);
	lv_poc_activity_set_design_cb(poc_cit_result_activity, design_func);
}


#ifdef __cplusplus
}
#endif

