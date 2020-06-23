#ifndef __LV_POC_ACTIVITY_H_
#define __LV_POC_ACTIVITY_H_

#include "lvgl.h"
#include "lv_include/lv_poc_type.h"

enum
{
    ACT_ID_POC_NONE,                    /* 0: invalid activity */
    ACT_ID_POC_MEMBER_LIST,             /*1: member list*/
    ACT_ID_POC_GROUP_LIST,              /*2: group list*/
    ACT_ID_POC_GROUP_MEMBER_LIST,       /*3: group member list*/
    ACT_ID_POC_MEMBER_CALL,             /*4: single call*/
    ACT_ID_POC_IDLE,                    /*5: poc idle*/
    ACT_ID_POC_MAIN_MENU,               /*6: main menu*/
    ACT_ID_POC_MAKE_GROUP,              /*7: make group*/
    ACT_ID_POC_MAIN_SETTING,            /*8: main setting*/
    ACT_ID_POC_BRIGHT_TIME,             /*9: birght time*/
    ACT_ID_POC_THEME_SWITCH,            /*10: theme switch*/
    ACT_ID_POC_MAIN_SIM_CHOICE,         /*11: main SIM*/
    ACT_ID_POC_NET_TYPE_CHOICE,         /*12: net type choice*/
    ACT_ID_POC_ABOUT,                   /*13: about self*/
    ACT_ID_POC_UPDATE_SOFTWARE,         /*14: update ui*/
    ACT_ID_POC_EDEG_KEY_SET,            /*15: edeg key steting*/
    ACT_ID_POC_VOLUM,                   /*16: turn volum*/
    ACT_ID_POC_DISPLAY,                 /*17: display*/
    ACT_ID_POC_WARNNING,                /*18: warn*/
    ACT_ID_POC_LOCK_GROUP,              /*19: display*/
    ACT_ID_POC_SHUTDOWN,                /*20: shutdown*/
    ACT_ID_ANYOBJ,                      /* : any object for example*/

    ACT_ID_MAX,
};
typedef uint8_t lv_poc_Activity_Id_t;

typedef lv_obj_t * (*lv_poc_activity_create_f_t)(lv_obj_t *);
typedef void (*lv_poc_activity_prepare_destory_f_t)(lv_obj_t *);

typedef struct
{
    lv_poc_Activity_Id_t actId;   /*activity id of the obj*/
    lv_poc_activity_create_f_t create;
    lv_poc_activity_prepare_destory_f_t prepare_destory;
} lv_poc_activity_ext_t;

#endif // __LV_POC_ACTIVITY_H_

