#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"
#include <stdlib.h>
#include "lv_gui_main.h"

#define POC_SHUTDOWN_PREPARE_TIME 5000

static void lv_poc_shutdown_note_event_handler(lv_obj_t *obj, lv_event_t event);
static lv_res_t lv_poc_shutdown_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);
static void lv_poc_shutdown_note_activity_destory(lv_obj_t *obj);
static void lv_poc_shutdown_animation(lv_task_t * task);
static void lv_poc_shutdown_task(lv_task_t * task);
static void lv_poc_power_off_warning_apply_event_handler(lv_obj_t *obj, lv_event_t event);
static void lv_poc_power_off_warning_cancel_event_handler(lv_obj_t *obj, lv_event_t event);
static void lv_poc_reboot_warning_apply_event_handler(lv_obj_t *obj, lv_event_t event);
static void lv_poc_reboot_warning_cancel_event_handler(lv_obj_t *obj, lv_event_t event);
static void lv_poc_shutdown_note_reboot_warning_create(lv_task_t * task);
static void lv_poc_shutdown_note_power_off_warning_create(lv_task_t * task);

static lv_obj_t *lv_poc_shutdown_note_obj = NULL;//消息框
lv_poc_activity_t * poc_shutdown_list_activity = NULL;//活动窗口

static lv_style_t lv_poc_shutdown_note_style = {0};
static lv_style_t lv_poc_list_style_background = {0};
static lv_style_t lv_poc_list_style_rel = {0};
static lv_style_t lv_poc_list_style_pr = {0};
static lv_style_t lv_poc_list_style_scroll = {0};



static lv_obj_t * lv_poc_shutdown_note_label = NULL;
static lv_obj_t * lv_poc_shutdown_note_labe2 = NULL;

static const char * lv_poc_shutdown_note_label_1_text = "关机";
static const char * lv_poc_shutdown_note_label_2_text = "重新启动";

static lv_area_t msg_display_area;//屏幕消息框大小
static lv_area_t shutdown_display_area;//关机消息框大小

//关机类型
typedef enum{
	LVPOCSHUTDOWN_TYPE_START = 0 ,
	LVPOCSHUTDOWN_TYPE_REBOOT ,
	LVPOCSHUTDOWN_TYPE_POWER_OFF ,
	LVPOCSHUTDOWN_TYPE_END ,
}poc_shutdown_type;

/*
	  name : lv_poc_shutdown_note_create
	 param : none
	author : wangls
  describe : 创建关机提示框
	  date : 2020-06-22
*/
static
void lv_poc_shutdown_note_activity_area(void)
{
	//根据160*128（屏幕宽度）和弹窗130*98（弹窗宽度）

	//屏幕弹跳消息框
	msg_display_area.x1 = 15;
	msg_display_area.x2 = 145;
	msg_display_area.y1 = 20;
	msg_display_area.y2 = 118;

	//消息框内容框大小
	shutdown_display_area.x1 = 15;
	shutdown_display_area.x2 = 145;
	shutdown_display_area.y1 = 20;
	shutdown_display_area.y2 = 118;
}

/*
	  name : lv_poc_shutdown_note_create
	 param : none
	author : wangls
  describe : 创建关机提示框
	  date : 2020-06-22
*/
static
lv_obj_t * lv_poc_shutdown_note_activity_create(lv_poc_display_t *display)
{
	poc_setting_conf = lv_poc_setting_conf_read();//获取字体
	memset(&lv_poc_shutdown_note_style, 0, sizeof(lv_style_t));

	//背景
	lv_style_copy(&lv_poc_list_style_background, &lv_style_pretty_color);
	lv_poc_list_style_background.body.padding.top = 0;
    lv_poc_list_style_background.body.padding.bottom = 0;
    lv_poc_list_style_background.body.padding.left = 0;
    lv_poc_list_style_background.body.padding.right = 0;
    lv_poc_list_style_background.body.padding.inner = 0;

	lv_poc_list_style_background.body.radius = 0;
	lv_poc_list_style_background.body.main_color = LV_COLOR_BLACK;
	lv_poc_list_style_background.body.grad_color = LV_COLOR_BLACK;

	//创建列表
	lv_poc_shutdown_note_obj = lv_list_create(display, NULL);
	lv_obj_t * scroll = lv_page_get_scrl(lv_poc_shutdown_note_obj);
	//消息框大小
	lv_obj_set_size(lv_poc_shutdown_note_obj, shutdown_display_area.x2 - shutdown_display_area.x1,
	shutdown_display_area.y2 - shutdown_display_area.y1);
	lv_obj_set_size(scroll, shutdown_display_area.x2 - shutdown_display_area.x1,
	shutdown_display_area.y2 - shutdown_display_area.y1);
	//位置
    lv_obj_align(lv_poc_shutdown_note_obj, display, LV_ALIGN_CENTER, 0, 0);

	lv_list_set_style(lv_poc_shutdown_note_obj,LV_LIST_STYLE_BG, &lv_poc_list_style_background);
	//滚动区域
	lv_style_copy(&lv_poc_list_style_scroll, &lv_poc_list_style_background);
    lv_poc_list_style_scroll.body.padding.top = 0;
    lv_poc_list_style_scroll.body.padding.bottom = 0;
    lv_poc_list_style_scroll.body.padding.left = 0;
    lv_poc_list_style_scroll.body.padding.right = 0;
    lv_poc_list_style_scroll.body.padding.inner = 0;
    lv_poc_list_style_scroll.body.border.width = 0;
    lv_poc_list_style_scroll.body.main_color = LV_COLOR_BLACK;
    lv_poc_list_style_scroll.body.grad_color = LV_COLOR_BLACK;
	lv_list_set_style(lv_poc_shutdown_note_obj,LV_LIST_STYLE_SCRL, &lv_poc_list_style_scroll);
	//滚动条
	//lv_list_set_style(lv_poc_shutdown_note_obj,LV_LIST_STYLE_SB,&lv_poc_list_style_background);
	//松开
    lv_style_copy(&lv_poc_list_style_rel, &lv_poc_list_style_scroll);
    lv_poc_list_style_rel.body.main_color = LV_COLOR_BLACK;
    lv_poc_list_style_rel.body.grad_color = LV_COLOR_BLACK;
	lv_poc_list_style_rel.text.color = LV_COLOR_WHITE;
    lv_poc_list_style_rel.text.font = (lv_font_t *)poc_setting_conf->font.idle_shutdownwindows_msg_font;
	lv_poc_list_style_rel.body.opa = 0xff;
	lv_poc_list_style_rel.body.radius = 0;

	lv_list_set_style(lv_poc_shutdown_note_obj,LV_LIST_STYLE_BTN_REL, &lv_poc_list_style_rel);
	lv_list_set_style(lv_poc_shutdown_note_obj,LV_LIST_STYLE_BTN_TGL_REL, &lv_poc_list_style_rel);
	//按下
    lv_style_copy(&lv_poc_list_style_pr, &lv_poc_list_style_rel);
    lv_poc_list_style_pr.body.main_color = LV_COLOR_BLUE;
    lv_poc_list_style_pr.body.grad_color = LV_COLOR_BLUE;
    lv_poc_list_style_pr.body.opa = 0xff;
	lv_poc_list_style_pr.body.radius = 0;

	lv_poc_list_style_pr.text.color = LV_COLOR_WHITE;
    lv_poc_list_style_pr.text.font = (lv_font_t *)poc_setting_conf->font.idle_shutdownwindows_msg_font;

	lv_list_set_style(lv_poc_shutdown_note_obj,LV_LIST_STYLE_BTN_PR, &lv_poc_list_style_pr);
	lv_list_set_style(lv_poc_shutdown_note_obj,LV_LIST_STYLE_BTN_TGL_PR, &lv_poc_list_style_pr);

    lv_list_set_sb_mode(lv_poc_shutdown_note_obj, LV_SB_MODE_OFF);

	return lv_poc_shutdown_note_obj;

}

/*
	  name : lv_poc_shutdown_note
	 param : none
	author : wangls
  describe : 关机提示
	  date : 2020-06-22
*/
void lv_poc_shutdown_note_activity_open(void)
{
	static lv_poc_activity_ext_t  activity_ext = {ACT_ID_POC_SHUTDOWN,
																lv_poc_shutdown_note_activity_create,
																lv_poc_shutdown_note_activity_destory};

    if(poc_shutdown_list_activity != NULL    || lv_poc_shutdown_note_obj != NULL)
    {
    	return;
    }

	lv_poc_shutdown_note_activity_area();//设置坐标
    poc_shutdown_list_activity = lv_poc_create_mini_activity(&activity_ext, &msg_display_area);
    lv_poc_activity_set_signal_cb(poc_shutdown_list_activity, lv_poc_shutdown_signal_func);

	lv_list_clean(lv_poc_shutdown_note_obj);

	/*添加选项*/
	//关机
	lv_poc_shutdown_note_label = lv_list_add_btn(lv_poc_shutdown_note_obj, &power_off, lv_poc_shutdown_note_label_1_text);
	lv_obj_set_click(lv_poc_shutdown_note_label, true);
	lv_obj_set_event_cb(lv_poc_shutdown_note_label, lv_poc_shutdown_note_event_handler);
	lv_btn_set_fit(lv_poc_shutdown_note_label, LV_FIT_NONE);
	lv_obj_set_height(lv_poc_shutdown_note_label, 98/2);

    //使能按钮
	lv_list_set_btn_selected(lv_poc_shutdown_note_obj, lv_poc_shutdown_note_label);

	//重新启动
	lv_poc_shutdown_note_labe2 = lv_list_add_btn(lv_poc_shutdown_note_obj, &reboot, lv_poc_shutdown_note_label_2_text);
	lv_obj_set_click(lv_poc_shutdown_note_labe2, true);
	lv_obj_set_event_cb(lv_poc_shutdown_note_labe2, lv_poc_shutdown_note_event_handler);
	lv_btn_set_fit(lv_poc_shutdown_note_labe2, LV_FIT_NONE);
	lv_obj_set_height(lv_poc_shutdown_note_labe2, 98/2);

}

/*
	  name : lv_poc_shutdown_note
	 param : none
	author : wangls
  describe : 关机提示事件
	  date : 2020-06-22
*/
static
void lv_poc_shutdown_note_event_handler(lv_obj_t *obj, lv_event_t event)
{
	if(obj == NULL) return;
	//回调事件
	if(event == LV_EVENT_PRESSED){

		uint16_t shutdown_list_keyvalue = 0;
		lv_obj_t * shutdown_list;

		//获取列表信息
		shutdown_list = lv_list_get_btn_selected(lv_poc_shutdown_note_obj);
		shutdown_list_keyvalue = lv_list_get_btn_index(lv_poc_shutdown_note_obj,shutdown_list);

		if(shutdown_list_keyvalue == 0)//关机
		{
			//提示是否确认关机
			lv_task_t * task = lv_task_create(lv_poc_shutdown_note_power_off_warning_create, 50,
				LV_TASK_PRIO_HIGH, (void *)LVPOCSHUTDOWN_TYPE_POWER_OFF);
			lv_task_once(task);
		}else if(shutdown_list_keyvalue == 1)//重新启动
		{
			//提示是否确认重新启动
			lv_task_t * task = lv_task_create(lv_poc_shutdown_note_reboot_warning_create, 50,
				LV_TASK_PRIO_HIGH, (void *)LVPOCSHUTDOWN_TYPE_REBOOT);
			lv_task_once(task);
		}

	}
}

/*
	  name : lv_poc_group_list_locked_event_handler
	 param : none
	author : wangls
  describe : 摧毁关机提示
	  date : 2020-06-22
*/
static
void lv_poc_shutdown_note_activity_destory(lv_obj_t *obj)
{
	if(lv_poc_shutdown_note_obj == NULL)
	{
		return;
	}
	//销毁
	lv_poc_shutdown_note_obj = NULL;
	poc_shutdown_list_activity = NULL;
}

/*
	  name : lv_poc_shutdown_signal_func
	 param : none
	author : wangls
  describe : 关机提示按键回调
	  date : 2020-06-22
*/
static
lv_res_t lv_poc_shutdown_signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
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
					lv_signal_send(lv_poc_shutdown_note_obj, LV_SIGNAL_PRESSED, NULL);
					lv_signal_send(lv_poc_shutdown_note_obj, LV_SIGNAL_CONTROL, param);
					//删除窗口
					lv_poc_del_activity(poc_shutdown_list_activity);
					break;
				}

				case LV_GROUP_KEY_DOWN:

				case LV_GROUP_KEY_UP:
				{
					lv_signal_send(lv_poc_shutdown_note_obj, LV_SIGNAL_CONTROL, param);
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
					lv_poc_del_activity(poc_shutdown_list_activity);
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
	  name : lv_poc_shutdown_animation
	 param : none
	author : wangls
  describe : 关机动画
	  date : 2020-06-23
*/
static
void lv_poc_shutdown_animation(lv_task_t * task)
{
	poc_shutdown_type user_data_type;
	user_data_type = (poc_update_type)task->user_data;

	lv_obj_t * shutdown_animation;
	shutdown_animation = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_size(shutdown_animation, 160, 128);
    lv_obj_set_pos(shutdown_animation,  0, 0);
	lv_obj_t * poc_shut_down_backgroup_image = lv_img_create(shutdown_animation, NULL);
	lv_img_set_auto_size(poc_shut_down_backgroup_image, false);
	lv_obj_set_size(poc_shut_down_backgroup_image, 160, 128);
	lv_img_set_src(poc_shut_down_backgroup_image, &img_poweroff_poc_logo_unicom);
	lv_obj_align( poc_shut_down_backgroup_image, shutdown_animation, LV_ALIGN_CENTER,  0,  0);

	if(user_data_type == LVPOCSHUTDOWN_TYPE_POWER_OFF)
	{
		lv_task_create(lv_poc_shutdown_task, POC_SHUTDOWN_PREPARE_TIME,
			LV_TASK_PRIO_HIGH, (void *)LVPOCSHUTDOWN_TYPE_POWER_OFF);
	}
	else if(user_data_type == LVPOCSHUTDOWN_TYPE_REBOOT)
	{
		lv_task_create(lv_poc_shutdown_task, POC_SHUTDOWN_PREPARE_TIME,
			LV_TASK_PRIO_HIGH, (void *)LVPOCSHUTDOWN_TYPE_REBOOT);
	}
}

/*
	  name : lv_poc_shutdown_task
	 param : none
	author : wangls
  describe : 关机
	  date : 2020-06-23
*/
static
void lv_poc_shutdown_task(lv_task_t * task)
{
	poc_shutdown_type user_data_type;
	user_data_type = (poc_update_type)task->user_data;

	if(user_data_type == LVPOCSHUTDOWN_TYPE_POWER_OFF)
	{
		osiShutdown(OSI_SHUTDOWN_POWER_OFF);
	}
	else if(user_data_type == LVPOCSHUTDOWN_TYPE_REBOOT)
	{
		osiShutdown(OSI_SHUTDOWN_RESET);
	}

}

/*
	  name : lv_poc_shutdown_note_power_off_warning_create
	 param : none
	author : wangls
  describe : 确认关机提示窗口
	  date : 2020-06-23
*/
static
void lv_poc_shutdown_note_power_off_warning_create(lv_task_t * task)
{
	char *title = "关机";
	char *context = "您的对讲机将会\n关机。";
	char *opt_left_str = "            确定            ";//28格才能填满
	lv_area_t warnningarea = {15, 20, 145, 118};//提示框大小

	//新建提示框
	lv_poc_warnning_open(title, context, opt_left_str,lv_poc_power_off_warning_apply_event_handler,
		NULL, lv_poc_power_off_warning_cancel_event_handler, warnningarea);

}

/*
	  name : lv_poc_power_off_warning_event_handler
	 param : none
	author : wangls
  describe : 关机 确定事件
	  date : 2020-06-23
*/
static
void lv_poc_power_off_warning_apply_event_handler(lv_obj_t *obj, lv_event_t event)
{
	//回调事件
	if(event == LV_EVENT_APPLY){//确认关机
		lv_task_t * task = lv_task_create(lv_poc_shutdown_animation, 50,
			LV_TASK_PRIO_HIGH, (void *)LVPOCSHUTDOWN_TYPE_POWER_OFF);
		lv_task_once(task);
	}
}

/*
	  name : lv_poc_power_off_warning_event_handler
	 param : none
	author : wangls
  describe : 关机 取消事件
	  date : 2020-06-23
*/
static
void lv_poc_power_off_warning_cancel_event_handler(lv_obj_t *obj, lv_event_t event)
{
	//回调事件
	if(event == LV_EVENT_CANCEL){//取消关机

	}
}

/*
	  name : lv_poc_shutdown_note_reboot_warning_create
	 param : none
	author : wangls
  describe : 确认重新启动提示窗口
	  date : 2020-06-24
*/
static
void lv_poc_shutdown_note_reboot_warning_create(lv_task_t * task)
{
	char *title = "重新启动";
	char *context = "您确定需要重新\n启动对讲机吗？";
	char *opt_left_str = "            确定            ";//28格才能填满
	lv_area_t warnningarea = {15, 20, 145, 118};//提示框大小

	//新建提示框
	lv_poc_warnning_open(title, context, opt_left_str,lv_poc_reboot_warning_apply_event_handler,
		NULL, lv_poc_reboot_warning_cancel_event_handler, warnningarea);

}

/*
	  name : lv_poc_reboot_warning_apply_event_handler
	 param : none
	author : wangls
  describe : 重新启动 确定/取消 事件
	  date : 2020-06-23
*/
static
void lv_poc_reboot_warning_apply_event_handler(lv_obj_t *obj, lv_event_t event)
{
	//回调事件
	if(event == LV_EVENT_APPLY){//确认重新启动
		lv_task_t * task = lv_task_create(lv_poc_shutdown_animation, 50,
			LV_TASK_PRIO_HIGH, (void *)LVPOCSHUTDOWN_TYPE_REBOOT);
		lv_task_once(task);
	}
}

/*
	  name : lv_poc_reboot_warning_cancel_event_handler
	 param : none
	author : wangls
  describe : 重新启动 取消事件
	  date : 2020-06-23
*/
static
void lv_poc_reboot_warning_cancel_event_handler(lv_obj_t *obj, lv_event_t event)
{
	//回调事件
	if(event == LV_EVENT_CANCEL){//取消重新启动

	}
}

#ifdef __cplusplus
}
#endif
