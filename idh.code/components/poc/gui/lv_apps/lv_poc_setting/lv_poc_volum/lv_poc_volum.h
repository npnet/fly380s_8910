#ifndef __LV_POC_VOLUM_H_
#define __LV_POC_VOLUM_H_

#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_poc_volum_open(void);

bool lv_poc_set_volum(POC_MMI_VOICE_TYPE_E type, uint8_t volume, bool play, bool display);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_VOLUM_H_
