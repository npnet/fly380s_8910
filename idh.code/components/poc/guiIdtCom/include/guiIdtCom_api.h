﻿/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#ifndef _INCLUDE_GUIIDTCOM_API_H_
#define _INCLUDE_GUIIDTCOM_API_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

OSI_EXTERN_C_BEGIN
;
typedef enum
{
	LVPOCGUIIDTCOM_SIGNAL_START = (1 << 8) - 1,

    LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND,
    LVPOCGUIIDTCOM_SIGNAL_LOGIN_REP,

    LVPOCGUIIDTCOM_SIGNAL_EXIT_IND,
    LVPOCGUIIDTCOM_SIGNAL_EXIT_REP,

    LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_IND,
    LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP,
    LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP_RECORD_IND,

    LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_IND,
    LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP,

    LVPOCGUIIDTCOM_SIGNAL_MIC_IND,
    LVPOCGUIIDTCOM_SIGNAL_MIC_REP,

    LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_IND,
    LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_REP,
    LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND,
    LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND,

    LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_IND,
    LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_REP,
    LVPOCGUIIDTCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND,
    LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND,

    LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_IND,
    LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_REP,
    LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND,
    LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND,

    LVPOCGUIIDTCOM_SIGNAL_SET_CURRENT_GROUP_IND,
    LVPOCGUIIDTCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND,
    LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND,

    LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_IND,
    LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP,

    LVPOCGUIIDTCOM_SIGNAL_MEMBER_STATUS_REP,
    LVPOCGUIIDTCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP,
    LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_MEMBER_STATUS_CB_REP,

    LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND,
    LVPOCGUIIDTCOM_SIGNAL_START_PLAY_IND,

    LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND,
    LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND,

    LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_IND,
    LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP,
    LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP,

    LVPOCGUIIDTCOM_SIGNAL_LISTEN_START_REP,
    LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP,
    LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP,

    LVPOCGUIIDTCOM_SIGNAL_GU_STATUS_REP,

    LVPOCGUIIDTCOM_SIGNAL_GROUP_OPERATOR_REP,

    LVPOCGUIIDTCOM_SIGNAL_RELEASE_LISTEN_TIMER_REP,

    LVPOCGUIIDTCOM_SIGNAL_DELAY_IND,

    LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP,

    LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF,

    LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND,

    LVPOCGUIIDTCOM_SIGNAL_SCREEN_ON_IND,
    LVPOCGUIIDTCOM_SIGNAL_SCREEN_OFF_IND,

    LVPOCGUIIDTCOM_SIGNAL_START_QUERY_SELF_MEMBERLIST_IND,
    LVPOCGUIIDTCOM_SIGNAL_STOP_QUERY_SELF_MEMBERLIST_IND,

    LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_IND,
    LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_REP,

    LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_IND,
    LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_REP,

    LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_IND,
    LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_REP,
	LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_IND,
    LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_REP,

	LVPOCGUIIDTCOM_SIGNAL_GET_SPEAK_CALL_CASE,
	LVPOCGUIIDTCOM_SIGNAL_SET_SHUTDOWN_POC,

	LVPOCGUIIDTCOM_SIGNAL_GPS_UPLOADING_IND,

	LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND,
	LVPOCGUIIDTCOM_SIGNAL_RESUME_IND,

	LVPOCGUIIDTCOM_SIGNAL_HEADSET_INSERT,
	LVPOCGUIIDTCOM_SIGNAL_HEADSET_PULL_OUT,

	LVPOCGUIIDTCOM_SIGNAL_DELAY_OPEN_PA_IND,

	LVPOCGUIIDTCOM_SIGNAL_PING_SUCCESS_REP,
	LVPOCGUIIDTCOM_SIGNAL_PING_FAILED_REP,

    LVPOCGUIIDTCOM_SIGNAL_LOOPBACK_RECORDER_IND,
    LVPOCGUIIDTCOM_SIGNAL_LOOPBACK_PLAYER_IND,
    LVPOCGUIIDTCOM_SIGNAL_TEST_VLOUM_PLAY_IND,

    LVPOCGUIIDTCOM_SIGNAL_END,
} LvPocGuiIdtCom_SignalType_t;

typedef enum{/*登陆状态*/

	LVPOCLEDIDTCOM_SIGNAL_LOGIN_START = 0,

	LVPOCLEDIDTCOM_SIGNAL_LOGIN_FAILED = 1 ,
	LVPOCLEDIDTCOM_SIGNAL_LOGIN_ING = 2 ,
	LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS = 3 ,
	LVPOCLEDIDTCOM_SIGNAL_LOGIN_EXIT = 4 ,

	LVPOCLEDIDTCOM_SIGNAL_LOGIN_END	,
}LVPOCIDTCOM_LOGIN_STATUS_T;

typedef enum{/*组状态*/

	LVPOCGROUPIDTCOM_SIGNAL_SELF_START = 0,

	LVPOCGROUPIDTCOM_SIGNAL_SELF_NO    = 1 ,
	LVPOCGROUPIDTCOM_SIGNAL_SELF_EXIST = 2 ,

	LVPOCGROUPIDTCOM_SIGNAL_SELF_END,
}LVPOCIDTCOM_SELFGROUP_STATUS_T;

typedef struct
{
	int status;
	unsigned short cause;
} LvPocGuiIdtCom_login_t;

typedef struct
{
	int opt;
	void *group_info;
	void (*cb)(lv_poc_group_oprator_type opt);
} LvPocGuiIdtCom_lock_group_t;

typedef struct
{
	void *group_info;
	void (*cb)(int result_type);
} LvPocGuiIdtCom_delete_group_t;

void lvPocGuiIdtCom_Init(void);

bool lvPocGuiIdtCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx);

void lvPocGuiIdtCom_log(void);

bool lvPocGuiIdtCom_get_status(void);

void *lvPocGuiIdtCom_get_self_info(void);

void *lvPocGuiIdtCom_get_current_group_info(void);

bool lvPocGuiIdtCom_get_speak_status(void);

bool lvPocGuiIdtCom_get_listen_status(void);

void *lvPocGuiIdtCom_get_current_lock_group(void);

bool lvPocGuiIdtCase_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx, void * cause_str);

int lvPocGuiIdtCom_get_current_exist_selfgroup(void);

void lvPocGuiComLogSwitch(bool status);

OSI_EXTERN_C_END

#endif
