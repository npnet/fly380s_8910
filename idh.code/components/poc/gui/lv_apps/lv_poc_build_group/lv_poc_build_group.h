#ifndef __LV_POC_BUILD_GROUP_H_
#define __LV_POC_BUILD_GROUP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	void        * item_information;
	lv_obj_t    * checkbox;
	bool          is_selected;
} lv_poc_build_group_item_info_t;

extern lv_poc_activity_t * poc_build_group_activity;

void lv_poc_build_group_open(void);

lv_poc_status_t lv_poc_build_group_add(lv_poc_member_list_t *member_list_obj, const char * name, bool is_online, void * information);

void lv_poc_build_group_remove(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

void lv_poc_build_group_clear(lv_poc_member_list_t *member_list_obj);

int lv_poc_build_group_get_information(lv_poc_member_list_t *member_list_obj, const char * name, void *** information);

void lv_poc_build_group_refresh(lv_poc_member_list_t *member_list_obj);

lv_poc_status_t lv_poc_build_group_move_top(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_build_group_move_bottom(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_build_group_move_up(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_build_group_move_down(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_build_group_set_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information, bool is_online);

lv_poc_status_t lv_poc_build_group_is_exists(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_build_group_get_state(lv_poc_member_list_t *member_list_obj, const char * name, void * information);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_BUILD_GROUP_H_
