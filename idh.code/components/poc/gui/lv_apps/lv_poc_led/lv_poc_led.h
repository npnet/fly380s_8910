#ifndef __LV_POC_LED_H_
#define __LV_POC_LED_H_


#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

void poc_status_led_task(void);
bool lvPocLedCom_Msg(LVPOCIDTCOM_Led_SignalType_t signal, bool valid);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_SETTING_H_
