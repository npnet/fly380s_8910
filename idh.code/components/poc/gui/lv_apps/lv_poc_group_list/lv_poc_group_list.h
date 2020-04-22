#ifndef __LV_POC_GROUP_LIST_H_
#define __LV_POC_GROUP_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned int group_number;
    list_element_t * group_list;
} lv_poc_group_list_t;

void lv_poc_group_list_open(void);

lv_poc_status_t lv_poc_group_list_add(const char * name, void * information);

void lv_poc_group_list_remove(const char * name, void * information);

int lv_poc_group_list_get_information(const char * name, void *** information);

void lv_poc_group_list_refresh(void);

lv_poc_status_t lv_poc_group_list_move_top(const char * name, void * information);

lv_poc_status_t lv_poc_group_list_move_bottom(const char * name, void * information);

lv_poc_status_t lv_poc_group_list_move_up(const char * name, void * information);

lv_poc_status_t lv_poc_group_list_move_down(const char * name, void * information);

//void lv_poc_group_list_set_state(const char * name, bool is_online);

lv_poc_status_t lv_poc_group_list_is_exists(const char * name, void * information);

//lv_poc_status lv_poc_group_list_get_state(const char * name);

#ifdef __cplusplus
}
#endif


#endif //__LV_POC_GROUP_LIST_H_
