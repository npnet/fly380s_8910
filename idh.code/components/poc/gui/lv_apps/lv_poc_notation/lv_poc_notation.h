#ifndef _LV_POC_NOTATION_H_
#define _LV_POC_NOTATION_H_

#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C"{
#endif

lv_obj_t * lv_poc_notation_create(void);

void lv_poc_notation_destory(void);

void lv_poc_notation_refresh(void);

void lv_poc_notation_hide(const bool hide);

lv_obj_t * lv_poc_notation_listenning(const int8_t * text_1, const int8_t * text_2);

lv_obj_t * lv_poc_notation_speaking(const int8_t * text_1, const int8_t * text_2);

bool lv_poc_notation_msg(lv_poc_notation_msg_type_t msg_type, const uint8_t *text_1, const uint8_t *text_2);

void lv_poc_notation_del(void);

#ifdef __cplusplus
}
#endif

#endif //_LV_POC_NOTATION_H_

