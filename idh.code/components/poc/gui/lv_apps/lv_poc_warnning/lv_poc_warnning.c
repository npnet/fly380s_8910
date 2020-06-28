#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"
#include <stdlib.h>
#include "sockets.h"

#define LV_POC_WARNNING_STACK_SIZE (20)

extern lv_group_t *poc_indev_group;

typedef struct{
	char title[32];
	char context[64];
	char opt_left_str[32];
	char opt_right_str[32];
	void (*opt_left_cb)(lv_obj_t * obj, lv_event_t event);
	void (*opt_right_cb)(lv_obj_t * obj, lv_event_t event);

	lv_poc_activity_t *activity;
	lv_obj_t *prv_base_obj;
	lv_obj_t *prv_title_obj;
	lv_obj_t *prv_line_obj;
	lv_point_t prv_line_points[2];
	lv_obj_t *prv_context_obj;
	lv_obj_t *prv_opt_left_obj;
	lv_obj_t *prv_opt_right_obj;
} lv_poc_warnning_t;

typedef struct{
	lv_poc_warnning_t *warnning_stack[LV_POC_WARNNING_STACK_SIZE];
	int cur_index;
} lv_poc_warnning_stack_t;

static lv_poc_warnning_stack_t prv_lv_poc_warnning_stack = {0};

static lv_style_t lv_poc_lockgroup_style = {0};
static lv_style_t lv_poc_lockgroupbutton_style = {0};
static lv_style_t style_line;

static lv_obj_t * prv_lv_poc_warnning_activity_create(lv_poc_display_t *display);

static void prv_lv_poc_warnning_activity_destory(lv_obj_t *obj);

static lv_poc_warnning_t *prv_lv_poc_warnning_stack_top_obj(void);

static bool prv_lv_poc_warnning_stack_push_obj(lv_poc_warnning_t *warnning);

static void prv_lv_poc_warnning_stack_pop_obj(void);

static lv_obj_t * prv_lv_poc_warnning_activity_create(lv_poc_display_t *display)
{
	lv_poc_warnning_t * warnning = prv_lv_poc_warnning_stack_top_obj();
	poc_setting_conf = lv_poc_setting_conf_read();//获取字体
	memset(&lv_poc_lockgroup_style, 0, sizeof(lv_style_t));
	lv_style_copy(&lv_poc_lockgroup_style, &lv_style_pretty_color);
	lv_poc_lockgroup_style.text.color = LV_COLOR_BLACK;
	lv_poc_lockgroup_style.text.font = (lv_font_t *)(poc_setting_conf->font.idle_lockgroupwindows_msg_font);
	lv_poc_lockgroup_style.text.opa = 255;
	lv_poc_lockgroup_style.body.main_color = LV_COLOR_WHITE;
	lv_poc_lockgroup_style.body.grad_color = LV_COLOR_WHITE;
	lv_poc_lockgroup_style.body.opa = 255;
	lv_poc_lockgroup_style.body.radius = 0;
	lv_poc_lockgroup_style.body.border.part = LV_BORDER_NONE;//去除边缘

	warnning->prv_base_obj = lv_obj_create(display, NULL);
	lv_obj_set_style(warnning->prv_base_obj, &lv_poc_lockgroup_style);
	lv_obj_set_size(warnning->prv_base_obj, lv_obj_get_width(display), lv_obj_get_height(display));
	lv_obj_align(warnning->prv_base_obj, display, LV_ALIGN_CENTER, 0, 0);

	//第一行text
	warnning->prv_title_obj = lv_label_create(warnning->prv_base_obj, NULL);
	lv_obj_set_size(warnning->prv_title_obj, lv_obj_get_width(warnning->prv_base_obj) - 4, lv_obj_get_height(warnning->prv_base_obj) / 3);
	lv_label_set_text(warnning->prv_title_obj,warnning->title);
	lv_obj_set_style(warnning->prv_title_obj,&lv_poc_lockgroup_style);
	lv_obj_align(warnning->prv_title_obj,warnning->prv_base_obj,LV_ALIGN_IN_TOP_LEFT,10,5);

	//创建线条
	lv_style_copy(&style_line, &lv_style_plain);
	style_line.line.color = LV_COLOR_GRAY;
	style_line.line.width = 1;
	style_line.line.rounded = 1;

	warnning->prv_line_points[0].x = 0;
	warnning->prv_line_points[0].y = lv_obj_get_height(warnning->prv_title_obj);
	warnning->prv_line_points[1].x = 300;
	warnning->prv_line_points[1].y = lv_obj_get_height(warnning->prv_title_obj);

	warnning->prv_line_obj = lv_line_create(warnning->prv_base_obj, NULL);
	lv_line_set_points(warnning->prv_line_obj, warnning->prv_line_points, 2);
	lv_line_set_style(warnning->prv_line_obj, LV_LINE_STYLE_MAIN, &style_line);
	lv_obj_align(warnning->prv_line_obj, warnning->prv_title_obj, LV_ALIGN_IN_BOTTOM_MID, 0, 3);

	//第二行text
	warnning->prv_context_obj = lv_label_create(warnning->prv_base_obj, NULL);
	lv_obj_set_size(warnning->prv_context_obj, lv_obj_get_width(warnning->prv_base_obj) - 4, lv_obj_get_height(warnning->prv_base_obj) / 3);
	lv_label_set_text(warnning->prv_context_obj, warnning->context);
	lv_obj_set_style(warnning->prv_context_obj, &lv_poc_lockgroup_style);
	lv_obj_align(warnning->prv_context_obj,warnning->prv_base_obj,LV_ALIGN_IN_LEFT_MID, 10, -2);

	//第三行字体
	memset(&lv_poc_lockgroupbutton_style, 0, sizeof(lv_style_t));
	lv_style_copy(&lv_poc_lockgroupbutton_style, &lv_style_pretty_color);
	lv_poc_lockgroupbutton_style.text.color = LV_COLOR_WHITE;
	lv_poc_lockgroupbutton_style.text.font = (lv_font_t *)(poc_setting_conf->font.idle_lockgroupwindows_msg_font);
	lv_poc_lockgroupbutton_style.text.opa = 255;
	lv_poc_lockgroupbutton_style.body.main_color = LV_COLOR_BLUE;
	lv_poc_lockgroupbutton_style.body.grad_color = LV_COLOR_BLUE;
	lv_poc_lockgroupbutton_style.body.opa = 255;
	lv_poc_lockgroupbutton_style.body.radius = 0;

	warnning->prv_opt_left_obj = lv_label_create(warnning->prv_base_obj, NULL);
	lv_obj_set_size(warnning->prv_opt_left_obj, lv_obj_get_width(warnning->prv_base_obj), lv_obj_get_height(warnning->prv_base_obj) / 3);
	lv_obj_set_style(warnning->prv_opt_left_obj,&lv_poc_lockgroupbutton_style);
	lv_obj_set_click(warnning->prv_opt_left_obj, true);
	lv_obj_set_event_cb(warnning->prv_opt_left_obj, warnning->opt_left_cb);
	lv_label_set_text(warnning->prv_opt_left_obj, warnning->opt_left_str);
	lv_label_set_body_draw(warnning->prv_opt_left_obj,true);//重刷背景颜色

	//第三行text
	if(warnning->opt_right_str[0] == 0)
	{
		lv_obj_align(warnning->prv_opt_left_obj,warnning->prv_base_obj,LV_ALIGN_IN_BOTTOM_MID,0,-3);
	}
	else
	{
		warnning->prv_opt_right_obj = lv_label_create(warnning->prv_base_obj, NULL);
		lv_obj_set_size(warnning->prv_opt_right_obj, lv_obj_get_width(warnning->prv_base_obj), lv_obj_get_height(warnning->prv_base_obj) / 3);
		lv_obj_set_style(warnning->prv_opt_right_obj,&lv_poc_lockgroupbutton_style);
		lv_obj_set_click(warnning->prv_opt_right_obj, true);
		lv_obj_set_event_cb(warnning->prv_opt_right_obj, warnning->opt_right_cb);
		lv_label_set_text(warnning->prv_opt_right_obj, warnning->opt_right_str);
		lv_label_set_body_draw(warnning->prv_opt_right_obj,true);//重刷背景颜色

		lv_obj_align(warnning->prv_opt_left_obj, warnning->prv_base_obj, LV_ALIGN_IN_BOTTOM_LEFT, 4, 0);
		lv_obj_align(warnning->prv_opt_right_obj, warnning->prv_base_obj, LV_ALIGN_IN_BOTTOM_RIGHT, -4, 0);
	}
	return warnning->prv_base_obj;
}

static void prv_lv_poc_warnning_activity_destory(lv_obj_t *obj)
{
	lv_poc_warnning_t *warnning = prv_lv_poc_warnning_stack_top_obj();
	lv_mem_free(warnning);
	prv_lv_poc_warnning_stack_pop_obj();
}


static lv_poc_warnning_t *prv_lv_poc_warnning_stack_top_obj(void)
{
	int index = prv_lv_poc_warnning_stack.cur_index - 1;
	if(index < 0)
	{
		return NULL;
	}

	return prv_lv_poc_warnning_stack.warnning_stack[index];
}

static bool prv_lv_poc_warnning_stack_push_obj(lv_poc_warnning_t *warnning)
{
	if(prv_lv_poc_warnning_stack.cur_index >= LV_POC_WARNNING_STACK_SIZE)
	{
		return false;
	}

	prv_lv_poc_warnning_stack.warnning_stack[prv_lv_poc_warnning_stack.cur_index] = warnning;
	prv_lv_poc_warnning_stack.cur_index++;
	return true;
}

static void prv_lv_poc_warnning_stack_pop_obj(void)
{
	if(prv_lv_poc_warnning_stack.cur_index < 1)
	{
		return;
	}

	prv_lv_poc_warnning_stack.cur_index--;
}

/*
	  name : prv_lv_poc_warnning_signal_func
	 param : none
	author : wangls
  describe : 回调
	  date : 2020-06-19
*/
static
lv_res_t prv_lv_poc_warnning_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	lv_poc_warnning_t *warnning = prv_lv_poc_warnning_stack_top_obj();
	if(warnning == NULL)
	{
		return LV_RES_INV;
	}

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
					if(warnning->prv_opt_left_obj != NULL && warnning->opt_left_cb != NULL)
					{
						warnning->opt_left_cb(warnning->activity->display, LV_EVENT_APPLY);
					}
					lv_poc_del_activity(warnning->activity);
					break;
				}

				case LV_GROUP_KEY_DOWN:

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

				case LV_KEY_ESC:
				{
					if(warnning->opt_right_cb != NULL)
					{
						warnning->opt_right_cb(warnning->activity->display, LV_EVENT_CANCEL);
					}
					lv_poc_del_activity(warnning->activity);
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

/*
	  name : lv_poc_warnning_open
	 param : title: 弹窗标题
	         context:弹窗提示内容
	         opt_left_str:弹窗左边提示内容
	         opt_left_cb:弹窗左边回调函数
	         opt_right_str:弹窗右边提示内容
	         opt_right_cb:弹窗右边回调函数
	         warnningarea:弹窗大小
	author : lugj
  describe : 打开警告弹窗
	  date : 2020-06-22
*/
void lv_poc_warnning_open(const char *title,
	const char *context,
	const char *opt_left_str,
	void (*opt_left_cb)(lv_obj_t * obj, lv_event_t event),
	const char *opt_right_str,
	void (*opt_right_cb)(lv_obj_t * obj, lv_event_t event),
	lv_area_t warnningarea)
{
    lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_WARNNING,
											prv_lv_poc_warnning_activity_create,
											prv_lv_poc_warnning_activity_destory};

	if(title == NULL || context == NULL || opt_left_str == NULL || opt_left_cb == NULL || (opt_right_str != NULL && opt_right_cb == NULL))
	{
		return;
	}

	lv_poc_warnning_t * warnning = (lv_poc_warnning_t *)lv_mem_alloc(sizeof(lv_poc_warnning_t));
	if(warnning == NULL)
	{
		return;
	}
	memset(warnning, 0, sizeof(lv_poc_warnning_t));
	strcpy(warnning->title, title);
	strcpy(warnning->context, context);
	strcpy(warnning->opt_left_str, opt_left_str);
	if(opt_right_str != NULL)
	{
		strcpy(warnning->opt_right_str, opt_right_str);
	}else
	{
		memset(warnning->opt_right_str, 0, sizeof(warnning->opt_right_str));
	}
	warnning->opt_left_cb = opt_left_cb;
	warnning->opt_right_cb = opt_right_cb;

	if(!prv_lv_poc_warnning_stack_push_obj(warnning))
	{
		lv_mem_free(warnning);
		return;
	}

	lv_area_t area = {20, 30, 140, 105};

	if(warnningarea.x1 != 0
		|| warnningarea.x2 != 0
		|| warnningarea.y1 != 0
		|| warnningarea.y2 != 0)
	{
		area = warnningarea;
	}

	warnning->activity = lv_poc_create_mini_activity(&activity_ext, &area);
	lv_poc_activity_set_signal_cb(warnning->activity, prv_lv_poc_warnning_signal_func);
}



#ifdef __cplusplus
}
#endif
