#ifndef __LV_POC_SHUTDOWN_NOTE_H_
#define __LV_POC_SHUTDOWN_NOTE_H_


#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LONGPRESS_SHUTDOWN_TIME 2000 /*2s*/

/*
	  name : lv_poc_shutdown_note
	 param : none
	author : wangls
  describe : 关机提示
	  date : 2020-06-22
*/
void lv_poc_shutdown_note_activity_open(void);



#ifdef __cplusplus
}
#endif

#endif //__LV_POC_SNTP_H_
