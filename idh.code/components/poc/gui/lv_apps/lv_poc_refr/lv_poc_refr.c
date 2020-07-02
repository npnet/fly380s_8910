#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"
#include <stdlib.h>

static void lv_poc_member_list_refr(lv_task_t * task);
static void lv_poc_group_list_refr(lv_task_t * task);
static void lv_poc_build_group_list_refr(lv_task_t * task);

/*
	  name : lv_poc_refr_task_create
	 param : none
	author : wangls
  describe : 创建LV刷新任务
	  date : 2020-07-02
*/
void lv_poc_refr_task_create( lv_poc_refr_task_cb_t func,
							 uint32_t period,
							 lv_task_prio_t prio)
{
	lv_task_t * task = lv_task_create(func, period,
		prio, NULL);
	lv_task_once(task);
}

/*
	  name : lv_poc_refr_func
	 param : type_t:刷新类型
	 		 period:刷新周期
	 		 prio:优先级
	author : wangls
  describe : 刷新UI信息
	  date : 2020-07-02
*/
void lv_poc_refr_func(poc_update_type type_t,
							 LVPOCIDTCOM_Refr_Period_t period,
							 lv_task_prio_t prio)
{
	if(type_t == LVPOCUPDATE_TYPE_MEMBERLIST)
	{
		lv_poc_refr_task_create(lv_poc_member_list_refr,period,prio);
	}
	else if(type_t == LVPOCUPDATE_TYPE_GROUPLIST)
	{
		lv_poc_refr_task_create(lv_poc_group_list_refr,period,prio);
	}
	else if(type_t == LVPOCUPDATE_TYPE_BUILD_GROUPLIST)
	{
		lv_poc_refr_task_create(lv_poc_build_group_list_refr,period,prio);
	}
	else if(type_t == LVPOCUPDATE_TYPE_MEMBERLIST_CALL)
	{
		lv_poc_refr_task_create(lv_poc_member_call_list_refr,period,prio);
	}
	else if(type_t == LVPOCUPDATE_TYPE_GROUPLIST_TITLE)
	{
		lv_poc_refr_task_create(lv_poc_group_list_title_refr,period,prio);
	}
}

/*
	  name : lv_poc_member_list_refr
	 param : none
	author : wangls
  describe : 刷新成员列表信息
	  date : 2020-07-02
*/
static
void lv_poc_member_list_refr(lv_task_t * task)
{
	lv_poc_member_list_refresh(NULL);
}

/*
	  name : lv_poc_group_list_refr
	 param : none
	author : wangls
  describe : 刷新群组列表信息
	  date : 2020-07-02
*/
static
void lv_poc_group_list_refr(lv_task_t * task)
{
	lv_poc_group_list_refresh(NULL);
}

/*
	  name : lv_poc_build_group_list_refr
	 param : none
	author : wangls
  describe : 刷新新建群组列表信息
	  date : 2020-07-02
*/
static
void lv_poc_build_group_list_refr(lv_task_t * task)
{
	lv_poc_build_group_refresh(NULL);
}

#ifdef __cplusplus
}
#endif
