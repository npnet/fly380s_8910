#ifndef __LV_POC_REFR_H_
#define __LV_POC_REFR_H_


#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*lv_poc_refr_task_cb_t)(struct _lv_task_t *);
typedef void (*lv_poc_refr_task_cb_tp)();

/*
	  name : lv_poc_refr_task_create
	 param : none
	author : wangls
  describe : 刷新一次，不带参数
	  date : 2020-07-03
*/
void lv_poc_refr_task_once( lv_poc_refr_task_cb_tp func,
							 LVPOCIDTCOM_Refr_Period_t period,
							 lv_task_prio_t prio);

/*
	  name : lv_poc_refr_func_np
	 param : task_cb_t:回调函数
	 		 period:刷新周期
	 		 prio:优先级
	 		 param:参数
	author : wangls
  describe : 刷新UI信息
	  date : 2020-07-03
*/
void lv_poc_refr_func_ui(lv_poc_refr_task_cb_t task_cb_t,
							 LVPOCIDTCOM_Refr_Period_t period,
							 lv_task_prio_t prio,
							 void *param);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_WARNNING_H_