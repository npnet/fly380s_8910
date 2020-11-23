/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
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

#include "osi_log.h"
#include "osi_api.h"
#include "osi_event_hub.h"
#include "osi_generic_list.h"
#include "osi_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "poc_config.h"
#include "poc_types.h"
#include "poc_keypad.h"
#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc_lib.h"
#include "poc_audio_player.h"
#include "poc_audio_recorder.h"
#include "guiBndCom_api.h"
#include "audio_device.h"
#include <sys/time.h>
#include "drv_charger_monitor.h"

/*************************************************
*
*                  EXTERN
*
*************************************************/
extern "C" lv_poc_activity_attribute_cb_set lv_poc_activity_func_cb_set;

#define GUIIDTCOM_DEBUG (0)

#ifdef CONFIG_POC_SUPPORT
#define FREERTOS
#define T_TINY_MODE

#define APPTEST_THREAD_PRIORITY (OSI_PRIORITY_NORMAL)
#define APPTEST_STACK_SIZE (8192 * 16)
#define APPTEST_EVENT_QUEUE_SIZE (64)
#define GUIIDTCOM_SELF_INFO_SZIE (1400)
#define GUIIDTCOM_RELEASE_MIC (void *)1
#define GUIIDTCOM_REQUEST_MIC (void *)2
#define GUIIDTCOM_MEMBER_CALL_HALFSINGLE "HALFSINGLE"
#define GUIIDTCOM_MEMBER_CALL_NORMAL "NORMAL"
#define GUIIDTCOM_MEMBER_CALL_MARK "{\"pfCallIn\": \"HALFSINGLE\"}"
#define GUIIDTCOM_GROUP_CALL_MARK "{\"pfCallIn\": \"NORMAL\"}"

//debug log info config                        cool watch搜索项
#define GUIIDTCOM_BUILDGROUP_DEBUG_LOG      1  //[buildgroup]
#define GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG   1  //[grouprefr]
#define GUIIDTCOM_GROUPOPTACK_DEBUG_LOG     1  //[groupoptack]
#define GUIIDTCOM_IDTGROUPLISTDEL_DEBUG_LOG 1  //[idtgroupdel]
#define GUIIDTCOM_MEMBERREFR_DEBUG_LOG      1  //[memberrefr]
#define GUIIDTCOM_IDTSPEAK_DEBUG_LOG        1  //[idtspeak]
#define GUIIDTCOM_IDTSINGLECALL_DEBUG_LOG   1  //[idtsinglecall]
#define GUIIDTCOM_IDTLISTEN_DEBUG_LOG       1  //[idtlisten]
#define GUIIDTCOM_IDTERRORINFO_DEBUG_LOG    1  //[idterrorinfo]
#define GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG    1  //[idtlockgroup]

//windows note
#define GUIIDTCOM_IDTWINDOWS_NOTE    		1

enum{
	USER_OPRATOR_START_SPEAK = 3,
	USER_OPRATOR_SPEAKING  = 4,
	USER_OPRATOR_START_LISTEN = 5,
	USER_OPRATOR_LISTENNING = 6,
};

static void LvGuiIdtCom_delay_close_listen_timer_cb(void *ctx);
static void LvGuiIdtCom_start_speak_voice_timer_cb(void *ctx);
static void prvPocGuiIdtTaskHandleCallFailed(uint32_t id, uint32_t ctx, uint32_t cause_str);

//全部的组
class CAllGroup
{
public:
    CAllGroup()
    {
        Reset();
    }
    ~CAllGroup()
    {
    }

    int Reset()
    {
	    m_Group_Num = 0;
        int i;
        for (i = 0; i < 32; i++)
        {
            m_Group[i].m_ucGName[0] = 0;
            m_Group[i].m_ucGNum[0] = 0;
        }
        return 0;
    }

public:
	unsigned int m_Group_Num;
    CGroup  m_Group[32];//一个用户,最多处于32个组中
};

//IDT使用者
class CIdtUser
{
public:
    CIdtUser()
    {
        Reset();
    }

    ~CIdtUser()
    {
    }

    int Reset()
    {
        m_iCallId   = -1;
        m_iRxCount  = 0;
        m_iTxCount  = 1;
        m_status    = 0;
        m_Group.Reset();
        return 0;
    }

public:
    int m_iCallId;//全系统只有一路呼叫,可以这样简单一些
    int m_iRxCount, m_iTxCount;//统计收发语音包数量
    int m_status; //用户的状态
    CAllGroup m_Group;
};

#if CONFIG_POC_AUDIO_DATA_IDT_BUFF_MAX_SIZE > 3
typedef struct
{
	uint8_t data[320];
	uint32_t length;
} pocAudioData_t;
#endif

typedef struct _PocGuiIIdtComAttr_t
{
public:
	osiThread_t *thread;
	bool        isReady;

	lv_poc_get_group_list_cb_t pocGetGroupListCb;
	lv_poc_get_member_list_cb_t pocGetMemberListCb;
	poc_set_current_group_cb pocSetCurrentGroupCb;
	poc_get_member_status_cb pocGetMemberStatusCb;
	poc_build_group_cb       pocBuildGroupCb;
	poc_set_member_call_status_cb pocMemberCallCb;
	void (*pocLockGroupCb)(lv_poc_group_oprator_type opt);
	void (*pocDeleteGroupcb)(int result_type);
	Msg_GData_s *pPocMemberList;//组成员结构体
	Msg_GData_s *pPocMemberListBuf;//组成员结构体,间隔一段时间更新一次
	CGroup *pLockGroup;
	CGroup LockGroupTemp;
	bool isLockGroupStatus;
	int lockGroupOpt;
	bool isPocMemberListBuf;
	bool isPocGroupListAll;
	Msg_GROUP_MEMBER_s self_info;
	Msg_GROUP_MEMBER_s speaker;
	CGroup speaker_group;
	uint8_t current_group;
	Msg_GROUP_MEMBER_s member_call_obj;
	bool is_member_call;
	int member_call_dir;  //0 tx,  1 rx
	unsigned int mic_ctl;
	int check_listen_count;
	osiTimer_t * delay_close_listen_timer;
	osiTimer_t * get_member_list_timer;
	osiTimer_t * get_group_list_timer;
	osiTimer_t * start_speak_voice_timer;
	osiTimer_t * get_lock_group_status_timer;
	osiTimer_t * check_listen_timer;
	bool delay_close_listen_timer_running;
	bool start_speak_voice_timer_running;
	char self_info_cjson_str[GUIIDTCOM_SELF_INFO_SZIE];
	bool   listen_status;
	bool   speak_status;
	bool   record_fist;
	uint16_t   membercall_count;
	int runcount;
	uint16_t loginstatus_t;
	osiTimer_t * try_login_timer;
	osiTimer_t * auto_login_timer;
	osiTimer_t * monitor_pptkey_timer;
	osiTimer_t * monitor_recorder_timer;
	osiTimer_t * play_tone_timer;
	osiTimer_t * addgroup_user_timer;
	bool onepoweron;
	char build_self_name[16];
#ifndef MUCHGROUP
	int buildgroupnumber;
#endif
	bool   is_makeout_call;
	bool   is_release_call;
	bool   is_justnow_listen;
	int    call_error_case;
	int    current_group_member_dwnum;
} PocGuiIIdtComAttr_t;

CIdtUser m_IdtUser;
static PocGuiIIdtComAttr_t pocIdtAttr = {0};

static void LvGuiIdtCom_delay_close_listen_timer_cb(void *ctx)
{
	pocIdtAttr.delay_close_listen_timer_running = false;
    if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
    {
	    m_IdtUser.m_status = UT_STATUS_ONLINE;
    }
    m_IdtUser.m_iRxCount = 0;
    m_IdtUser.m_iTxCount = 0;
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP, NULL);

	#if GUIIDTCOM_IDTLISTEN_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[idtlisten]%s(%d):delayclose_listen_timer cb", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif
}

static void LvGuiIdtCom_start_play_tone_timer_cb(void *ctx)
{
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND, NULL);
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP, NULL);
	pocIdtAttr.start_speak_voice_timer_running = false;
}

static void LvGuiIdtCom_start_speak_voice_timer_cb(void *ctx)
{
	//goto play start speak tone
	poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Speak, 30, true);
	osiTimerStart(pocIdtAttr.play_tone_timer, 200);//200ms

	#if GUIIDTCOM_IDTSPEAK_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[idtspeak]%s(%d):start speak, timer cb", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif
}

static void LvGuiIdtCom_get_member_list_timer_cb(void *ctx)
{
	if(!pocIdtAttr.isPocMemberListBuf)
	{
		if(lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP, NULL))
		{
			return;
		}
	}
	osiTimerStartRelaxed(pocIdtAttr.get_member_list_timer, 1000, OSI_WAIT_FOREVER);
}

static void LvGuiIdtCom_get_group_list_timer_cb(void *ctx)
{
	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF, NULL))
	{
		if(pocIdtAttr.isReady)
		{
			osiTimerStartRelaxed(pocIdtAttr.get_group_list_timer, 1000, OSI_WAIT_FOREVER);
		}
		return;
	}

	#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG|GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[idtlockgroup][grouprefr]%s(%d):start get lock_group_status and query include_self group", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif
	osiTimerStartRelaxed(pocIdtAttr.get_lock_group_status_timer, 500, OSI_WAIT_FOREVER);

	#if 0
	osiTimerStart(pocIdtAttr.get_group_list_timer, 1000 * 60 * 5, OSI_WAIT_FOREVER);
	#endif
}

static void LvGuiIdtCom_get_lock_group_status_timer_cb(void *ctx)
{
	if(pocIdtAttr.lockGroupOpt > LV_POC_GROUP_OPRATOR_TYPE_NONE)
	{
		osiTimerStartRelaxed(pocIdtAttr.get_lock_group_status_timer, 1000, OSI_WAIT_FOREVER);

		#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG
		char cOutstr[256] = {0};
		cOutstr[0] = '\0';
		sprintf(cOutstr, "[idtlockgroup]%s(%d):get lock group status timer cb -> restart 1s timer", __func__, __LINE__);
		OSI_LOGI(0, cOutstr);
		#endif

		return;
	}

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND, NULL))
	{
		if(pocIdtAttr.isReady)
		{
			osiTimerStartRelaxed(pocIdtAttr.get_lock_group_status_timer, 2000, OSI_WAIT_FOREVER);
		}
		return;
	}

	#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[idtlockgroup]%s(%d):send msg to get lock group status", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif
}

static void LvGuiIdtCom_check_listen_timer_cb(void *ctx)
{
	if(pocIdtAttr.check_listen_count < 1
		|| m_IdtUser.m_status < USER_OPRATOR_START_LISTEN
		|| m_IdtUser.m_status > USER_OPRATOR_LISTENNING)
	{
		if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
		{
			m_IdtUser.m_status = UT_STATUS_ONLINE;
		}

	    if(pocIdtAttr.delay_close_listen_timer_running)
	    {
		    osiTimerStop(pocIdtAttr.delay_close_listen_timer);
		    pocIdtAttr.delay_close_listen_timer_running = false;
	    }
	    osiTimerStart(pocIdtAttr.delay_close_listen_timer, 460);
	    pocIdtAttr.delay_close_listen_timer_running = true;
		pocIdtAttr.check_listen_count = 0;
		osiTimerStop(pocIdtAttr.check_listen_timer);
		return;
	}
	pocIdtAttr.check_listen_count--;
}

static void LvGuiIdtCom_try_login_timer_cb(void *ctx)
{
	if(lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND, NULL))
	{
		osiTimerStop(pocIdtAttr.try_login_timer);
	}
	osiTimerStop(pocIdtAttr.try_login_timer);
}

static void LvGuiIdtCom_auto_login_timer_cb(void *ctx)
{
	if(pocIdtAttr.loginstatus_t == LVPOCLEDIDTCOM_SIGNAL_LOGIN_FAILED)
	{
		if(lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND, NULL))
		{
			OSI_LOGI(0, "[song]autologin ing...[3]\n");
			pocIdtAttr.loginstatus_t = LVPOCLEDIDTCOM_SIGNAL_LOGIN_ING;
		}
	}
}

static void LvGuiIdtCom_ppt_release_timer_cb(void *ctx)
{
	static int makecallcnt = 0;
	bool pttStatus = pocGetPttKeyState()|lv_poc_get_earppt_state();
	if(true == pttStatus && pocIdtAttr.is_makeout_call == true)
	{
		osiTimerStart(pocIdtAttr.monitor_pptkey_timer, 50);
		makecallcnt++;
	}
	else//ppt release
	{
		if(makecallcnt < 10)//press time < 500ms
		{
			lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP, (void *)USER_OPRATOR_SPEAKING);
		}
		makecallcnt = 0;
	}
}

static void LvGuiIdtCom_recorder_timer_cb(void *ctx)
{
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND, NULL);
}

static void LvGuiIdtCom_refresh_group_adduser_timer_cb(void *ctx)
{
#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_IDTGROUPLISTDEL_DEBUG_LOG
	char cOutstr6[128] = {0};
	cOutstr6[0] = '\0';
	sprintf(cOutstr6, "[grouprefr]%s(%d):opt OPT_G_ADDUSER or OPT_G_DELUSER", __func__, __LINE__);
	OSI_LOGI(0, cOutstr6);
#endif

	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF, NULL);
}

static void prvPocGuiIdtTaskHandleLogin(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND:
		{


			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LOGIN_REP:
		{

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_EXIT_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_EXIT_REP:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleSpeak(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_IND:
		{

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_IND:
		{

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleMic(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_MIC_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MIC_REP:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleGroupList(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocIdtAttr.pocGetGroupListCb = (lv_poc_get_group_list_cb_t)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
		{
			pocIdtAttr.pocGetGroupListCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleBuildGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			if(pocIdtAttr.pocBuildGroupCb == NULL)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			if(pocIdtAttr.pocBuildGroupCb == NULL)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			#if GUIIDTCOM_BUILDGROUP_DEBUG_LOG
			char cOutstr[256] = {0};
    		cOutstr[0] = '\0';
    		sprintf(cOutstr, "[buildgroup]%s(%d):register build group cb!", __func__, __LINE__);
    		OSI_LOGI(0, cOutstr);
			#endif

			pocIdtAttr.pocBuildGroupCb = (poc_build_group_cb)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND:
		{
			#if GUIIDTCOM_BUILDGROUP_DEBUG_LOG
			char cOutstr[256] = {0};
    		cOutstr[0] = '\0';
    		sprintf(cOutstr, "[buildgroup]%s(%d):cannel build group cb!", __func__, __LINE__);
    		OSI_LOGI(0, cOutstr);
			#endif

			pocIdtAttr.pocBuildGroupCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleMemberList(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
		{
			if(pocIdtAttr.pocGetMemberListCb == NULL)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
		{
			if(pocIdtAttr.pocGetMemberListCb == NULL)
			{
				break;
			}
			pocIdtAttr.pocGetMemberListCb(1, pocIdtAttr.pPocMemberListBuf->dwNum, pocIdtAttr.pPocMemberListBuf);
			pocIdtAttr.pocGetMemberListCb = NULL;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocIdtAttr.pocGetMemberListCb = (lv_poc_get_member_list_cb_t)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			pocIdtAttr.pocGetMemberListCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleCurrentGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_SET_CURRENT_GROUP_IND:
		{
			if(pocIdtAttr.pocSetCurrentGroupCb == NULL)
			{
				break;
			}

			if(ctx == 0)
			{
				pocIdtAttr.pocSetCurrentGroupCb(0);
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocIdtAttr.pocSetCurrentGroupCb = (poc_set_current_group_cb)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
		{
			pocIdtAttr.pocSetCurrentGroupCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleMemberInfo(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP:
		{
			if(ctx == 0 || !poc_get_lcd_status())
			{
				break;
			}

			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleMemberStatus(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_STATUS_REP:
		{
			if(pocIdtAttr.pocGetMemberStatusCb == NULL)
			{
				break;
			}

			pocIdtAttr.pocGetMemberStatusCb(ctx > 0);
			pocIdtAttr.pocGetMemberStatusCb = NULL;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
		{
			if(ctx == 0)
			{
				break;
			}
			pocIdtAttr.pocGetMemberStatusCb = (poc_get_member_status_cb)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_MEMBER_STATUS_CB_REP:
		{
			pocIdtAttr.pocGetMemberStatusCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandlePlay(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_START_PLAY_IND:
		{
			if(m_IdtUser.m_status < UT_STATUS_ONLINE)
			{
			    m_IdtUser.m_iRxCount = 0;
			    m_IdtUser.m_iTxCount = 0;
				break;
			}
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleRecord(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND:
		{
			if(m_IdtUser.m_status < UT_STATUS_ONLINE || m_IdtUser.m_status < USER_OPRATOR_START_SPEAK || m_IdtUser.m_status > USER_OPRATOR_SPEAKING)
			{
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_REP, GUIIDTCOM_RELEASE_MIC);
				break;
			}
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleMemberCall(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
		{
		    if(ctx == 0)
		    {
			    break;
		    }

			if(pocIdtAttr.listen_status == true)
			{
				/*speak cannot membercall*/
				return;
			}
		    lv_poc_member_call_config_t *member_call_config = (lv_poc_member_call_config_t *)ctx;

		    Msg_GROUP_MEMBER_s *member_call_obj = (Msg_GROUP_MEMBER_s *)member_call_config->members;
		    Msg_GROUP_MEMBER_s *member = member_call_obj;

		    if(member_call_config->func == NULL)
		    {
			    break;
		    }

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
		{
			 //对方同意了单呼通话
		    if(pocIdtAttr.pocMemberCallCb != NULL)
		    {
			    pocIdtAttr.pocMemberCallCb(0, 0);
			    pocIdtAttr.pocMemberCallCb = NULL;
		    }
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
		{
			//对方或者服务器释放或者拒绝了单呼通话
			pocIdtAttr.is_member_call = false;
			pocIdtAttr.member_call_dir = 0;

			lv_poc_activity_func_cb_set.member_call_close();
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleListen(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_LISTEN_START_REP:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP:
		{
			pocIdtAttr.listen_status = false;

			/*恢复run闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_RUN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");

			if((pocIdtAttr.membercall_count > 1 && pocIdtAttr.is_member_call == true) || pocIdtAttr.is_member_call == false)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)"停止聆听", (const uint8_t *)"");
				poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			}

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);

			#if GUIIDTCOM_IDTLISTEN_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[idtlisten]%s(%d):stop listen rep and destory idle,windows", __func__, __LINE__);
			OSI_LOGI(0, cOutstr);
			#endif

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP:
		{
			char speaker_name[100];
			char *speaker_group = (char *)pocIdtAttr.speaker_group.m_ucGName;
			memset(speaker_name, 0, sizeof(char) * 100);
			strcpy(speaker_name, (const char *)pocIdtAttr.speaker.ucName);
			strcat(speaker_name, (const char *)"正在讲话");

			pocIdtAttr.listen_status = true;
			/*开始闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			//member call
			if(pocIdtAttr.is_member_call)
			{
				if(pocIdtAttr.membercall_count > 1)//单呼进入第二次
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)"");
				}
			}
			else
			{
				char speaker_group_name[100];
				strcpy(speaker_group_name, (const char *)pocIdtAttr.speaker_group.m_ucGName);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, speaker_name, speaker_group);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)speaker_group_name);
			}

			#if GUIIDTCOM_IDTLISTEN_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[idtlisten]%s(%d):start listen_speaker rep and display idle,windows", __func__, __LINE__);
			OSI_LOGI(0, cOutstr);
			#endif

			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleGuStatus(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_GU_STATUS_REP:/*组或者用户状态发生变化*/
		{
			if(ctx == 0)
			{
				break;
			}

			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleGroupOperator(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_GROUP_OPERATOR_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleReleaseListenTimer(uint32_t id, uint32_t ctx)
{
	if(pocIdtAttr.delay_close_listen_timer == NULL)
	{
		return;
	}

	osiTimerDelete(pocIdtAttr.delay_close_listen_timer);
	pocIdtAttr.delay_close_listen_timer = NULL;
}

static void prvPocGuiIdtTaskHandleLockGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_REP:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_REP:
		{
			break;
		}

		default:
			break;
	}
}

static void prvPocGuiIdtTaskHandleDeleteGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_REP:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleOther(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_DELAY_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SCREEN_ON_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SCREEN_OFF_IND:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleChargerOpt(uint32_t id, uint32_t ctx)
{
	switch(ctx)//Warning, please read the parameters carefully!!!
	{
		case CHR_CHARGE_START_IND:
		{
			break;
		}

		case CHR_CHARGE_END_IND:
		{
			break;
		}

		case CHR_WARNING_IND:
		{
			break;
		}

		case CHR_SHUTDOWN_IND:
		{
			/*add shutdown opt*/
			osiShutdown(OSI_SHUTDOWN_POWER_OFF);
			break;
		}
	}

}

static
void prvPocGuiIdtTaskHandleCallFailed(uint32_t id, uint32_t ctx, uint32_t cause_str)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_GET_SPEAK_CALL_CASE:
		{
			break;
		}
	}
}


static void prvPocGuiIdtTaskHandleUploadGpsInfo(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUIIDTCOM_SIGNAL_GPS_UPLOADING_IND:
      {
         break;
      }

      default:
         break;
   }
}

static void prvPocGuiIdtTaskHandleRecorderSuspendorResumeOpt(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_RESUME_IND:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void pocGuiIdtComTaskEntry(void *argument)
{

	osiEvent_t event = {0};

    for(int i = 0; i < 1; i++)
    {
	    osiThreadSleepRelaxed(1000, OSI_WAIT_FOREVER);
    }

    pocIdtAttr.isReady = true;

    while(1)
    {
    	if(!osiEventWait(pocIdtAttr.thread, &event))
		{
			if(UT_STATUS_ONLINE == m_IdtUser.m_status)
			{

			}
			continue;
		}

		if(event.id != 100)
		{
			continue;
		}

		switch(event.param1)
		{
			case LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND:
			case LVPOCGUIIDTCOM_SIGNAL_LOGIN_REP:
			case LVPOCGUIIDTCOM_SIGNAL_EXIT_IND:
			case LVPOCGUIIDTCOM_SIGNAL_EXIT_REP:
			{
				prvPocGuiIdtTaskHandleLogin(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP:
			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP:
			{
				prvPocGuiIdtTaskHandleSpeak(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_MIC_IND:
			case LVPOCGUIIDTCOM_SIGNAL_MIC_REP:
			{
				prvPocGuiIdtTaskHandleMic(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_IND:
			case LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_REP:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
			{
				prvPocGuiIdtTaskHandleGroupList(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_REP:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND:
			{
				prvPocGuiIdtTaskHandleBuildGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
			{
				prvPocGuiIdtTaskHandleMemberList(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SET_CURRENT_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
			{
				prvPocGuiIdtTaskHandleCurrentGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_IND:
			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP:
			{
				prvPocGuiIdtTaskHandleMemberInfo(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_STATUS_REP:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_MEMBER_STATUS_CB_REP:
			{
				prvPocGuiIdtTaskHandleMemberStatus(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND:
			case LVPOCGUIIDTCOM_SIGNAL_START_PLAY_IND:
			{
				prvPocGuiIdtTaskHandlePlay(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND:
			case LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND:
			{
				prvPocGuiIdtTaskHandleRecord(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
			case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
			{
				prvPocGuiIdtTaskHandleMemberCall(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_LISTEN_START_REP:
			case LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP:
			case LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP:
			{
				prvPocGuiIdtTaskHandleListen(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GU_STATUS_REP:
			{
				prvPocGuiIdtTaskHandleGuStatus(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GROUP_OPERATOR_REP:
			{
				prvPocGuiIdtTaskHandleGroupOperator(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_RELEASE_LISTEN_TIMER_REP:
			{
				prvPocGuiIdtTaskHandleReleaseListenTimer(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_REP:
			case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_REP:
			{
				prvPocGuiIdtTaskHandleLockGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_REP:
			case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_IND:
    		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_REP:
			{
				prvPocGuiIdtTaskHandleDeleteGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_DELAY_IND:
			case LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP:
			case LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF:
			case LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SCREEN_ON_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SCREEN_OFF_IND:
			{
				prvPocGuiIdtTaskHandleOther(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GET_SPEAK_CALL_CASE://获取对讲原因
			{
				prvPocGuiIdtTaskHandleCallFailed(event.param1, event.param2, event.param3);
				break;
			}


			case LVPOCGUIIDTCOM_SIGNAL_SET_SHUTDOWN_POC:
			{
				bool status;

				status = lv_poc_get_charge_status();
				if(status == false)
				{
					prvPocGuiIdtTaskHandleChargerOpt(event.param1, event.param2);
				}
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GPS_UPLOADING_IND:
            {
            	prvPocGuiIdtTaskHandleUploadGpsInfo(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND:
			case LVPOCGUIIDTCOM_SIGNAL_RESUME_IND:
			{
				prvPocGuiIdtTaskHandleRecorderSuspendorResumeOpt(event.param1, event.param2);
				break;
			}

			default:
				OSI_LOGW(0, "[gic] receive a invalid event\n");
				break;
		}
    }
}

extern "C" void pocGuiIdtComStart(void)
{
    pocIdtAttr.thread = osiThreadCreate(
		"pocGuiIdtCom", pocGuiIdtComTaskEntry, NULL,
		APPTEST_THREAD_PRIORITY, APPTEST_STACK_SIZE,
		APPTEST_EVENT_QUEUE_SIZE);
	pocIdtAttr.delay_close_listen_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_delay_close_listen_timer_cb, NULL);
	pocIdtAttr.start_speak_voice_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_start_speak_voice_timer_cb, NULL);
	pocIdtAttr.get_member_list_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_get_member_list_timer_cb, NULL);
	pocIdtAttr.get_group_list_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_get_group_list_timer_cb, NULL);
	pocIdtAttr.get_lock_group_status_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_get_lock_group_status_timer_cb, NULL);
	pocIdtAttr.check_listen_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_check_listen_timer_cb, NULL);
	pocIdtAttr.try_login_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_try_login_timer_cb, NULL);//注册尝试登录定时器
	pocIdtAttr.auto_login_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_auto_login_timer_cb, NULL);//注册自动登录定时器
	pocIdtAttr.monitor_pptkey_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_ppt_release_timer_cb, NULL);//检查ppt键定时器
	pocIdtAttr.monitor_recorder_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_recorder_timer_cb, NULL);//检查是否有人在录音定时器
	pocIdtAttr.play_tone_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_start_play_tone_timer_cb, NULL);
	pocIdtAttr.addgroup_user_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_refresh_group_adduser_timer_cb, NULL);
}

extern "C" void lvPocGuiIdtCom_Init(void)
{
	memset(&pocIdtAttr, 0, sizeof(PocGuiIIdtComAttr_t));
	pocIdtAttr.membercall_count = 0;
	pocIdtAttr.is_release_call = true;
	pocIdtAttr.pPocMemberList = (Msg_GData_s *)malloc(sizeof(Msg_GData_s));
	pocIdtAttr.pPocMemberListBuf = (Msg_GData_s *)malloc(sizeof(Msg_GData_s));
	pocIdtAttr.pLockGroup = (CGroup *)malloc(sizeof(CGroup));
	if(pocIdtAttr.pLockGroup != NULL)
	{
		memset(pocIdtAttr.pLockGroup, 0, sizeof(CGroup));
	}
	pocGuiIdtComStart();
}

extern "C" bool lvPocGuiIdtCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx)
{
    if (pocIdtAttr.thread == NULL || (signal != LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND && pocIdtAttr.isReady == false))
    {
	    return false;
    }

	static osiEvent_t event = {0};

	uint32_t critical = osiEnterCritical();

	memset(&event, 0, sizeof(osiEvent_t));
	event.id = 100;
	event.param1 = signal;
	event.param2 = (uint32_t)ctx;

	osiExitCritical(critical);

	return osiEventSend(pocIdtAttr.thread, &event);
}

extern "C" void lvPocGuiIdtCom_log(void)
{
	lvPocGuiIdtCom_Init();
}

bool lvPocGuiIdtCom_get_status(void)
{
	return m_IdtUser.m_status;
}

bool lvPocGuiIdtCom_get_listen_status(void)
{
	return pocIdtAttr.listen_status;
}

bool lvPocGuiIdtCom_get_speak_status(void)
{
	return pocIdtAttr.speak_status;
}

extern "C" void *lvPocGuiIdtCom_get_self_info(void)
{
	if(m_IdtUser.m_status < UT_STATUS_ONLINE)
	{
		return NULL;
	}
	return (void *)&pocIdtAttr.self_info;
}

extern "C" void *lvPocGuiIdtCom_get_current_group_info(void)
{
	if(m_IdtUser.m_status < UT_STATUS_ONLINE)
	{
		return NULL;
	}
	return (void *)&m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group];
}

extern "C" void *lvPocGuiIdtCom_get_current_lock_group(void)
{
	if(!pocIdtAttr.isLockGroupStatus || pocIdtAttr.pLockGroup == NULL)
	{
		return NULL;
	}
	return pocIdtAttr.pLockGroup;
}

extern "C" int lvPocGuiIdtCom_get_current_exist_selfgroup(void)
{
	unsigned long i;
	nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

	for (i = 0; i < m_IdtUser.m_Group.m_Group_Num; i++)
	{
		if(0 == strcmp((char *)poc_config->selfbuild_group_num, (char *)m_IdtUser.m_Group.m_Group[i].m_ucGNum))
		{
			break;
		}
	}

	if(i == m_IdtUser.m_Group.m_Group_Num)
	{
		poc_config->is_exist_selfgroup = LVPOCGROUPIDTCOM_SIGNAL_SELF_NO;
	}

	return poc_config->is_exist_selfgroup;
}

extern "C" bool lvPocGuiIdtCase_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx, void * cause_str)
{
    if (pocIdtAttr.thread == NULL || pocIdtAttr.isReady == false)
    {
	    return false;
    }

	static osiEvent_t event = {0};

	uint32_t critical = osiEnterCritical();

	memset(&event, 0, sizeof(osiEvent_t));
	event.id = 100;
	event.param1 = signal;
	event.param2 = (uint32_t)ctx;
	event.param3 = (uint32_t)cause_str;

	osiExitCritical(critical);

	return osiEventSend(pocIdtAttr.thread, &event);
}

#endif

