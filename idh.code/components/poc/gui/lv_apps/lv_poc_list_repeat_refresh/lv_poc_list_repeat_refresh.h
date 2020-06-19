#ifndef __LV_POC_LIST_REPEAT_REFRESH_H_
#define __LV_POC_LIST_REPEAT_REFRESH_H_


#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool list_refresh_status;

/*
	  name : lv_poc_list_refresh
	 param : TYPE {@ 刷新类型 @}
	author : wangls
  describe : 刷新列表
	  date : 2020-06-17
*/
void lv_poc_list_repeat_refresh(poc_update_type LIST_TYPE , lv_obj_t * list , LVPOCIDTCOM_List_Period_t list_update_period);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_SNTP_H_
