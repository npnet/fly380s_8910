#ifndef __LV_POC_REFR_H_
#define __LV_POC_REFR_H_


#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*lv_poc_refr_task_cb_t)(lv_task_t * task);

void lv_poc_refr_func(poc_update_type type_t,
							 LVPOCIDTCOM_Refr_Period_t period,
							 lv_task_prio_t prio);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_WARNNING_H_