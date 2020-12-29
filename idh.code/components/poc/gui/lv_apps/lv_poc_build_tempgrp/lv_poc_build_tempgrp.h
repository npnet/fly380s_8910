#ifndef __LV_POC_BUILD_TEMPGRP_H_
#define __LV_POC_BUILD_TEMPGRP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	void        * item_information;
	lv_obj_t    * checkbox;
	bool          is_selected;
	bool          is_self;
	bool          online;
} lv_poc_build_tempgrp_item_info_t;

extern lv_poc_activity_t * poc_build_tempgrp_activity;

void lv_poc_build_tempgrp_open(void);

lv_poc_status_t lv_poc_build_tempgrp_add(lv_poc_oem_member_list *member_list_obj, const char * name, bool is_online, void * information);

void lv_poc_build_tempgrp_clear(lv_poc_oem_member_list *member_list_obj);

int lv_poc_build_tempgrp_get_information(lv_poc_oem_member_list *member_list_obj, const char * name, void *** information);

void lv_poc_build_tempgrp_refresh(lv_task_t * task);

void lv_poc_build_tempgrp_refresh_with_data(lv_poc_oem_member_list *member_list_obj);

lv_poc_status_t lv_poc_build_tempgrp_set_state(lv_poc_oem_member_list *member_list_obj, const char * name, void * information, bool is_online);

lv_poc_status_t lv_poc_build_tempgrp_is_exists(lv_poc_oem_member_list *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_build_tempgrp_get_state(lv_poc_oem_member_list *member_list_obj, const char * name, void * information);

lv_poc_tmpgrp_t lv_poc_build_tempgrp_progress(lv_poc_tmpgrp_t status);

void lv_poc_tmpgrp_multi_call_open(void);

#ifdef __cplusplus
}
#endif

#endif
