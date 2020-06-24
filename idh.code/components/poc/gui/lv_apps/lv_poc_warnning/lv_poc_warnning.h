#ifndef __LV_POC_WARNNING_H_
#define __LV_POC_WARNNING_H_


#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	  name : lv_poc_warnning_open
	 param : title: 弹窗标题
	         context:弹窗提示内容
	         opt_left_str:弹窗左边提示内容
	         opt_left_cb:弹窗左边回调函数
	         opt_right_str:弹窗右边提示内容
	         opt_right_cb:弹窗右边回调函数
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
	lv_area_t warnningarea);


#ifdef __cplusplus
}
#endif

#endif //__LV_POC_WARNNING_H_