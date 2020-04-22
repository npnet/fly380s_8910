#ifndef __INCLUDE_IDLE_H
#define __INCLUDE_IDLE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "lv_include/lv_poc.h"

extern lv_poc_activity_t *activity_idle;
void lv_poc_idle_set_user_name(const char * name_str);
void lv_poc_idle_set_group_name(const char * name_str);
char * lv_poc_idle_get_user_name(void);
char * lv_poc_idle_get_group_name(void);


extern lv_obj_t * idle_title_label;
extern lv_obj_t * idle_user_label;
extern lv_obj_t * idle_user_name_label;
extern lv_obj_t * idle_group_label;
extern lv_obj_t * idle_group_name_label;

lv_poc_activity_t * lv_poc_create_idle(void);

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif //__INCLUDE_IDLE_H
