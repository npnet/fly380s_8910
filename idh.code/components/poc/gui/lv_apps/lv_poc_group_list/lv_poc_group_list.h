#ifndef __LV_POC_GROUP_LIST_H_
#define __LV_POC_GROUP_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	void        * item_information;
	lv_obj_t    * lock_img;
	bool          is_lock;
} lv_poc_group_list_item_info_t;

void lv_poc_group_list_open(lv_poc_group_list_t *group_list_obj);

lv_poc_status_t lv_poc_group_list_add(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

void lv_poc_group_list_remove(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

void lv_poc_group_list_clear(lv_poc_group_list_t *group_list_obj);

int lv_poc_group_list_get_information(lv_poc_group_list_t *group_list_obj, const char * name, void *** information);

void lv_poc_group_list_refresh(lv_poc_group_list_t *grouplist);

void lv_poc_group_list_refresh_with_data(lv_poc_group_list_t *group_list_obj);

lv_poc_status_t lv_poc_group_list_move_top(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_group_list_move_bottom(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_group_list_move_up(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_group_list_move_down(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

//void lv_poc_group_list_set_state(const char * name, bool is_online);

lv_poc_status_t lv_poc_group_list_is_exists(lv_poc_group_list_t *group_list_obj, const char * name, void * information);

lv_poc_status_t lv_poc_group_list_lock_group(lv_poc_group_list_t *group_list_obj, lv_poc_group_oprator_type opt);

//lv_poc_status lv_poc_group_list_get_state(const char * name);

void lv_poc_group_list_set_hightlight_index(void);

void lv_poc_group_list_refresh_task(lv_task_t *task);

#ifdef __cplusplus
}
#endif


#endif //__LV_POC_GROUP_LIST_H_
