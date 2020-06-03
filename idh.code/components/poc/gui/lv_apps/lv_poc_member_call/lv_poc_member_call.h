#ifndef __INCLUDE_LV_POC_SINGLE_CALL_H
#define __INCLUDE_LV_POC_SINGLE_CALL_H

#include "lv_include/lv_poc_type.h"

#ifdef __cplusplus
extern "C"{
#endif

void lv_poc_member_call_open(void * information);

void lv_poc_member_call_close(void);

lv_poc_status_t lv_poc_member_call_add(lv_poc_member_list_t *member_list_obj, const char * name, bool is_online, void * information);

void lv_poc_member_call_remove(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

void lv_poc_member_call_clear(lv_poc_member_list_t *member_list_obj);

int lv_poc_member_call_get_information(lv_poc_member_list_t *member_list_obj, const char * name, void *** information);

void lv_poc_member_call_refresh(lv_poc_member_list_t *member_list_obj);

lv_poc_status_t lv_poc_member_call_move_top(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_call_move_bottom(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_call_move_up(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_call_move_down(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_call_set_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information, bool is_online);

lv_poc_status_t lv_poc_member_call_is_exists(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_call_get_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

#ifdef __cplusplus
}
#endif

#endif //__INCLUDE_LV_POC_SINGLE_CALL_H
