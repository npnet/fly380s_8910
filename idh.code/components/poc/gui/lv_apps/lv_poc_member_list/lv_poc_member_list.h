﻿#ifndef __LV_POC_MEMBER_LIST_H_
#define __LV_POC_MEMBER_LIST_H_

#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc_activity.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_poc_member_list_open(IN char * title, IN lv_poc_member_list_t *members, IN bool hide_offline);

void lv_poc_memberlist_activity_open(lv_task_t * task);

lv_poc_status_t lv_poc_member_list_add(lv_poc_member_list_t *member_list_obj, const char * name, bool is_online, void * information);

void lv_poc_member_list_remove(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

void lv_poc_member_list_clear(lv_poc_member_list_t *member_list_obj);

int lv_poc_member_list_get_information(lv_poc_member_list_t *member_list_obj, const char * name, void *** information);

void lv_poc_member_list_refresh(lv_poc_member_list_t *member_list_obj);

void lv_poc_member_list_refresh_with_data(lv_poc_member_list_t *member_list_obj);

lv_poc_status_t lv_poc_member_list_move_top(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_list_move_bottom(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_list_move_up(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_list_move_down(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_list_set_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information, bool is_online);

lv_poc_status_t lv_poc_member_list_is_exists(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_member_list_get_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

void lv_poc_member_list_set_hightlight_index(void);

void lv_poc_member_list_refresh_task(lv_task_t *task);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_MEMBER_LIST_H_
