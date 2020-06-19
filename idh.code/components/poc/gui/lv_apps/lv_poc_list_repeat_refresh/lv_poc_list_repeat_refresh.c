#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"

/*本地变量*/
static void lv_poc_list_update_task(lv_task_t * task);
static lv_obj_t * activity_list;

/*全局变量*/
bool list_refresh_status = true;/*若需要使用刷新机制(lv_poc_list_refresh)，
									请在按键处加入该状态判断是否为真，等待
									列表刷新完成，否则导致死机*/

/*
	  name : lv_poc_list_refresh
	 param : TYPE {@ 刷新类型 @}
	author : wangls
  describe : 刷新列表
	  date : 2020-06-17
*/
void lv_poc_list_repeat_refresh(poc_update_type LIST_TYPE , lv_obj_t * list , LVPOCIDTCOM_List_Period_t list_update_period)
{
	if(list_update_period == 0)return;
	list_refresh_status = false;//刷新机制开启
	lv_task_t * task = lv_task_create(lv_poc_list_update_task, list_update_period, LV_TASK_PRIO_LOWEST, (void *)LIST_TYPE);
	activity_list = list;
	lv_task_once(task);
}

/*
	  name : lv_poc_list_update_task
	 param : none
	author : wangls
  describe : callback update
	  date : 2020-06-17
*/
static void lv_poc_list_update_task(lv_task_t * task)
{
	poc_update_type user_data_type;

	user_data_type = (poc_update_type)task->user_data;

	if(user_data_type == LVPOCUPDATE_TYPE_MEMBERLIST)
	{
		LV_ASSERT_OBJ(activity_list, LV_OBJX_NAME);

		lv_list_ext_t * ext = lv_obj_get_ext_attr(activity_list);
		/*If there is a valid selected button the make the next selected*/
		if(ext->selected_btn != NULL) {
			lv_obj_t * btn_next = lv_list_get_prev_btn(activity_list, ext->selected_btn);
			/*强制刷新 上移一次*/
			lv_list_set_btn_selected(activity_list, btn_next);

			//刷新下移4次
			for(int i = 0; i < 4; i++)
			{
				btn_next = lv_list_get_next_btn(activity_list, ext->selected_btn);
	            if(btn_next)
	            {
					lv_list_set_btn_selected(activity_list, btn_next);
	            }
			}

			//归位焦点
			for(int i = 0; i < 3; i++)
			{
				btn_next = lv_list_get_prev_btn(activity_list, ext->selected_btn);
				if(btn_next)
	            {
					lv_list_set_btn_selected(activity_list, btn_next);
				}
			}
		}
		/*If there is no selected button the make the first selected*/
		else {
			lv_obj_t * btn = lv_list_get_next_btn(activity_list, NULL);
			if(btn) lv_list_set_btn_selected(activity_list, btn);
		}
	}
	else if(user_data_type == LVPOCUPDATE_TYPE_GROUPLIST)//群组列表刷新机制
	{
		LV_ASSERT_OBJ(activity_list, LV_OBJX_NAME);

		lv_list_ext_t * ext = lv_obj_get_ext_attr(activity_list);
		/*If there is a valid selected button the make the next selected*/
		if(ext->selected_btn != NULL) {
			lv_obj_t * btn_next = lv_list_get_prev_btn(activity_list, ext->selected_btn);
			//if(btn_next) /* 注释此处 --- 强制向上刷新*/
			lv_list_set_btn_selected(activity_list, btn_next);

			//归位焦点
			btn_next = lv_list_get_next_btn(activity_list, ext->selected_btn);
            if(btn_next)
			lv_list_set_btn_selected(activity_list, btn_next);
		}
		/*If there is no selected button the make the first selected*/
		else {
			lv_obj_t * btn = lv_list_get_next_btn(activity_list, NULL);
			if(btn) lv_list_set_btn_selected(activity_list, btn);
		}
	}
	else if(user_data_type == LVPOCUPDATE_TYPE_BUILD_GROUPLIST)//群组列表里的成员列表刷新机制
	{
		LV_ASSERT_OBJ(activity_list, LV_OBJX_NAME);

		lv_list_ext_t * ext = lv_obj_get_ext_attr(activity_list);
		/*If there is a valid selected button the make the next selected*/
		if(ext->selected_btn != NULL) {
			lv_obj_t * btn_next = lv_list_get_prev_btn(activity_list, ext->selected_btn);
			/*强制刷新 上移一次*/
			lv_list_set_btn_selected(activity_list, btn_next);

			//刷新下移4次
			for(int i = 0; i < 4; i++)
			{
				btn_next = lv_list_get_next_btn(activity_list, ext->selected_btn);
	            if(btn_next)
	            {
					lv_list_set_btn_selected(activity_list, btn_next);
	            }
			}

			//归位焦点
			for(int i = 0; i < 3; i++)
			{
				btn_next = lv_list_get_prev_btn(activity_list, ext->selected_btn);
				if(btn_next)
	            {
					lv_list_set_btn_selected(activity_list, btn_next);
				}
			}
		}
		/*If there is no selected button the make the first selected*/
		else {
			lv_obj_t * btn = lv_list_get_next_btn(activity_list, NULL);
			if(btn) lv_list_set_btn_selected(activity_list, btn);
		}

	}
	list_refresh_status = true;//刷新机制关闭

}

#ifdef __cplusplus
}
#endif
