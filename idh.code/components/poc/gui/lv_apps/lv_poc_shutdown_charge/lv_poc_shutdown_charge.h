#ifndef __LV_POC_SHUTDOWN_CHARGE_H_
#define __LV_POC_SHUTDOWN_CHARGE_H_


#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	  name : pocLvgl_ShutdownCharge_Start
	 param : none
	author : wangls
  describe : 创建关机充电任务
	  date : 2020-06-24
*/
void pocLvgl_ShutdownCharge_Start(void);

/*
	  name : lv_poc_charge_poweron_status
	 param : none
	author : wangls
  describe : 是否为充电开机
	  date : 2020-06-24
*/
bool lv_poc_charge_poweron_status(void);


#ifdef __cplusplus
}
#endif

#endif //__LV_POC_SNTP_H_
