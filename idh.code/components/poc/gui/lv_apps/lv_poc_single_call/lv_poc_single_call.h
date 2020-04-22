#ifndef __INCLUDE_LV_POC_SINGLE_CALL_H
#define __INCLUDE_LV_POC_SINGLE_CALL_H

#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C"{
#endif

void lv_poc_member_call_open(void * information);

lv_poc_status_t lv_poc_member_call_add(const char * name, void * information);

void lv_poc_member_call_clear(void);

lv_poc_status_t lv_poc_member_call_is_exists(const char * name, void * information);

void lv_poc_member_call_refresh(void);

#ifdef __cplusplus
}
#endif

#endif //__INCLUDE_LV_POC_SINGLE_CALL_H
