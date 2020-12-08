#ifndef __LV_POC_SHUTDOWN_NOTE_H_
#define __LV_POC_SHUTDOWN_NOTE_H_


#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LONGPRESS_SHUTDOWN_TIME 3000 //3s

extern lv_poc_activity_t * poc_shutdown_list_activity;

/*
	  name : lv_poc_shutdown_note
	 param : none
	author : wangls
  describe : 关机提示
	  date : 2020-06-22
*/
void lv_poc_shutdown_note_activity_open(lv_task_t * task);

/*
	  name : lv_poc_shutdown_animation
	 param : none
	author : wangls
  describe : 关机动画
	  date : 2020-06-23
*/
void lv_poc_shutdown_animation(lv_task_t * task);

/*
	  name : lv_poc_get_power_off_status
	 param : none
	author : wangls
  describe : get power off status
	  date : 2020-08-22
*/
bool lv_poc_get_power_off_status(void);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_SNTP_H_
