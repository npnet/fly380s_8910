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
    ACT_ID_POC_MAKE_TEMPGRP,            /*7: make tempgroup*/
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
    ACT_ID_POC_GPS_MONITOR,             /*21: gps monitor*/
    ACT_ID_POC_STATE_INFO,				/*22: state info*/
    ACT_ID_POC_CIT_MAIN,				/*23: cit main mecu*/
    ACT_ID_POC_CIT_TERM_INFO,			/*24: cit teaminal info*/
    ACT_ID_POC_CIT_AUTO_TEST,			/*25: cit auto test*/
    ACT_ID_POC_CIT_PART_TEST,			/*26: cit part test*/
    ACT_ID_POC_CIT_LOG_SWITCH,			/*27: cit log switch*/
    ACT_ID_POC_CIT_TEST_UI,			    /*28: cit test ui*/
    ACT_ID_POC_CIT_RESULT_UI,			/*29: cit result ui*/
    ACT_ID_POC_RECORD_PALYBACK,			/*30: record playback*/
    ACT_ID_POC_SOUND_QUALITY,           /*31: sound quality*/
    ACT_ID_POC_TMPGRP_MEMBER_LIST,      /*32: tempgroup member list*/
	ACT_ID_POC_PING_TIME,				/*22: ping time*/
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

