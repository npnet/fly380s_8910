#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"
#include <stdlib.h>

lv_task_t * lv_poc_refr_obj = NULL;

/*
	  name : lv_poc_refr_task_create
	 param : none
	author : wangls
  describe : 创建LV刷新任务
	  date : 2020-07-02
*/
static
void lv_poc_refr_task_create( lv_poc_refr_task_cb_t func,
							 uint32_t period,
							 lv_task_prio_t prio,
							 void *param)
{
	lv_task_t * task = lv_task_create(func, period,
		prio, param);
	lv_task_once(task);
}

/*
	  name : lv_poc_refr_task_create
	 param : none
	author : wangls
  describe : 刷新一次，不带参数
	  date : 2020-07-03
*/
void lv_poc_refr_task_once( lv_poc_refr_task_cb_tp func,
							 LVPOCIDTCOM_Refr_Period_t period,
							 lv_task_prio_t prio)
{
	lv_task_t * task = lv_task_create(func, period,
		prio, NULL);
	lv_task_once(task);
}

/*
	  name : lv_poc_refr_func_np
	 param : task_cb_t:回调函数
	 		 period:刷新周期
	 		 prio:优先级
	 		 param:参数
	author : wangls
  describe : 刷新UI信息{@ 带参数的刷新机制 @}
	  date : 2020-07-03
*/
void lv_poc_refr_func_ui(lv_poc_refr_task_cb_t task_cb_t,
							 LVPOCIDTCOM_Refr_Period_t period,
							 lv_task_prio_t prio,
							 void *param)
{
	if(task_cb_t == NULL)
		return;

	lv_poc_refr_obj = malloc(sizeof(lv_task_t));
	if(lv_poc_refr_obj == NULL)
		return;

	lv_poc_refr_obj->task_cb = task_cb_t;
	lv_poc_refr_obj->period = period;
	lv_poc_refr_obj->prio = prio;
	lv_poc_refr_obj->user_data = param;

	lv_poc_refr_task_create(lv_poc_refr_obj->task_cb, lv_poc_refr_obj->period, lv_poc_refr_obj->prio, lv_poc_refr_obj->user_data);
}

#ifdef __cplusplus
}
#endif
