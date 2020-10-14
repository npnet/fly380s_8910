#ifndef __LV_POC_SNTP_H_
#define __LV_POC_SNTP_H_


#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	  name : lv_poc_sntp_Update_Time
	 param : none
	author : wangls
  describe : 网络更新时间
	  date : 2020-06-09
*/
void lv_poc_sntp_Update_Time(void);
uint8_t cereg_Respond(bool reportN);
void _Cereg_UtoBinString(uint8_t *string, uint8_t value);
uint16_t _Cereg_MemCompare(const void *buf1, const void *buf2, uint16_t count);
void *_Cereg_MemCopy8(void *dest, const void *src, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_SNTP_H_
