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
#include "poc_audio_pipe.h"
#include "guiCtelCom_api.h"
#include "audio_device.h"
#include <sys/time.h>
#include "drv_charger_monitor.h"

//c++ --> c
extern "C"{

#include "ctel_oem_api.h"

}

/*************************************************
*
*                  EXTERN
*
*************************************************/
extern "C" lv_poc_activity_attribute_cb_set lv_poc_activity_func_cb_set;

#ifdef CONFIG_POC_SUPPORT
#define FREERTOS
#define T_TINY_MODE

#define APPTEST_THREAD_PRIORITY (OSI_PRIORITY_ABOVE_NORMAL)
#define APPTEST_STACK_SIZE (8192 * 16)
#define APPTEST_EVENT_QUEUE_SIZE (64)
#define GUICTELCOM_SELF_INFO_SZIE (1400)
//group number
#define GUIBNDCOM_GROUPMAX            32
#define USER_ONLINE 	1
#define USER_OFFLINE 	2

//debug log info config                        cool watch搜索项
#define GUICTELCOM_BUILDGROUP_DEBUG_LOG       1  //[buildgroup]
#define GUICTELCOM_GROUPLISTREFR_DEBUG_LOG    1  //[grouprefr]
#define GUICTELCOM_GROUPOPTACK_DEBUG_LOG      1  //[groupoptack]
#define GUICTELCOM_CTELGROUPLISTDEL_DEBUG_LOG 1  //[Ctelgroupdel]
#define GUICTELCOM_MEMBERREFR_DEBUG_LOG       1  //[memberrefr]
#define GUICTELCOM_CTELSPEAK_DEBUG_LOG        1  //[ctelspeak]
#define GUICTELCOM_CTELSINGLECALL_DEBUG_LOG   1  //[ctelsinglecall]
#define GUICTELCOM_CTELLISTEN_DEBUG_LOG       1  //[ctellisten]
#define GUICTELCOM_CTELERRORINFO_DEBUG_LOG    1  //[ctelerrorinfo]
#define GUICTELCOM_CTELLOCKGROUP_DEBUG_LOG    1  //[ctellockgroup]

//windows note
#define GUICTELCOM_CTELWINDOWS_NOTE    		1
#define POC_AUDIO_MODE_PIPE                 1

enum{
	USER_OPRATOR_START_SPEAK = 3,
	USER_OPRATOR_SPEAKING  = 4,
	USER_OPRATOR_START_LISTEN = 5,
	USER_OPRATOR_LISTENNING = 6,
};

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
        for (i = 0; i < GUIBNDCOM_GROUPMAX; i++)
        {
            m_Group[i].m_ucGName[0] = 0;
            m_Group[i].m_ucGNum[0] = 0;
        }
        return 0;
    }

public:
	unsigned int m_Group_Num;
    CGroup  m_Group[GUIBNDCOM_GROUPMAX];//一个用户,最多处于32个组中
};

//CTEL使用者
class CCtelUser
{
public:
    CCtelUser()
    {
        Reset();
    }

    ~CCtelUser()
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

#if CONFIG_POC_AUDIO_DATA_CTEL_BUFF_MAX_SIZE > 3
typedef struct
{
	uint8_t data[320];
	uint32_t length;
} pocAudioData_t;
#endif

//GPS基本数据
typedef struct _GPS_DATA_s
{
    float           longitude;          //经度
    float           latitude;           //纬度
    float           speed;              //速度
    float           direction;          //方向
    //时间
    uint16_t          year;               //年
    uint8_t           month;              //月
    uint8_t           day;                //日
    uint8_t           hour;               //时
    uint8_t           minute;             //分
    uint8_t           second;             //秒
}GPS_DATA_s;

typedef struct _PocGuiCtelComAttr_t
{
public:
	osiThread_t *thread;
	bool        isReady;

	int         pipe;
    POCAUDIOPLAYER_HANDLE    player;
    POCAUDIORECORDER_HANDLE recorder;

	lv_poc_get_group_list_cb_t pocGetGroupListCb;
	lv_poc_get_member_list_cb_t pocGetMemberListCb;
	poc_set_current_group_cb pocSetCurrentGroupCb;
	poc_get_member_status_cb pocGetMemberStatusCb;
	poc_build_group_cb       pocBuildGroupCb;
	poc_set_member_call_status_cb pocMemberCallCb;
	pcm_record_cb send_record_data_cb;

	bool isPocMemberListBuf;
	bool isPocGroupListAll;
	uint32_t query_group;
	int check_listen_count;
	osiTimer_t * delay_close_listen_timer;
	osiTimer_t * get_member_list_timer;
	osiTimer_t * get_group_list_timer;
	osiTimer_t * start_speak_voice_timer;
	osiTimer_t * check_ack_timeout_timer;
	osiTimer_t * check_listen_timer;
	bool delay_close_listen_timer_running;
	bool start_speak_voice_timer_running;
	bool   listen_status;
	bool   is_listen_stop;
	bool   speak_status;
	bool   record_fist;
	int runcount;
	osiTimer_t * try_login_timer;
	osiTimer_t * auto_login_timer;
	osiTimer_t * monitor_pptkey_timer;
	osiTimer_t * monitor_recorder_timer;
	osiTimer_t * BrightScreen_timer;
	bool onepoweron;
	char build_self_name[16];

	bool   is_makeout_call;
	bool   is_release_call;
	bool   is_justnow_listen;
	int    call_error_case;
	int    current_group_member_dwnum;

	//status
	uint16_t loginstatus_t;
	int current_group;
	bool is_member_call;
	bool member_call_dir;
	int call_type;
	bool   invaildlogininf;
	int  call_briscr_dir;
	int  call_curscr_state;

	//group info
	CGroup current_group_info;
	CGroup m_Group[32];
	int m_Group_Num;

	//user info
	Msg_GROUP_MEMBER_s self_info;

	//Mutex
	osiMutex_t *lock;
} PocGuiCtelComAttr_t;

typedef struct
{
	int msgaddr;
} pocCtelMsg_t;

CCtelUser m_CtelUser;
static PocGuiCtelComAttr_t pocCtelAttr = {0};
static lv_poc_cit_status_type isCitStatus = LVPOCCIT_TYPE_READ_STATUS;

/*static*/
static void lvPocGuiCtelCom_send_data_callback(uint8_t * data, uint32_t length);

static void LvGuiCtelCom_get_member_list_timer_cb(void *ctx)
{
	if(!pocCtelAttr.isPocMemberListBuf)
	{
		if(lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP, NULL))
		{
			return;
		}
	}
	osiTimerStartRelaxed(pocCtelAttr.get_member_list_timer, 1000, OSI_WAIT_FOREVER);
}

static void LvGuiCtelCom_get_group_list_timer_cb(void *ctx)
{

}

static void LvGuiCtelCom_check_ack_timeout_timer_cb(void *ctx)
{
	if(pocCtelAttr.pocGetMemberListCb != NULL)
	{
		pocCtelAttr.pocGetMemberListCb(0, 0, NULL);
		pocCtelAttr.pocGetMemberListCb = NULL;
	}
	else if(pocCtelAttr.pocGetGroupListCb != NULL)
	{
		pocCtelAttr.pocGetGroupListCb(0, 0, NULL);
		pocCtelAttr.pocGetGroupListCb = NULL;
	}
}

static void LvGuiCtelCom_check_listen_timer_cb(void *ctx)
{

}

static void LvGuiCtelCom_try_login_timer_cb(void *ctx)
{
	if(lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_LOGIN_IND, NULL))
	{
		osiTimerStop(pocCtelAttr.try_login_timer);
	}
	osiTimerStop(pocCtelAttr.try_login_timer);
}

static void LvGuiCtelCom_auto_login_timer_cb(void *ctx)
{
	if(pocCtelAttr.loginstatus_t == LVPOCCTELCOM_SIGNAL_LOGIN_FAILED)
	{
		if(lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_LOGIN_IND, NULL))
		{
			OSI_LOGI(0, "[song]autologin ing...[3]\n");
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_ING;
		}
	}
}

static void LvGuiCtelCom_ppt_release_timer_cb(void *ctx)
{
	static int makecallcnt = 0;
	bool pttStatus = pocGetPttKeyState()|lv_poc_get_earppt_state();
	if(true == pttStatus && pocCtelAttr.is_makeout_call == true)
	{
		osiTimerStart(pocCtelAttr.monitor_pptkey_timer, 50);
		makecallcnt++;
	}
	else//ppt release
	{
		if(makecallcnt < 10)//press time < 500ms
		{
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_REP, (void *)USER_OPRATOR_SPEAKING);
		}
		makecallcnt = 0;
	}
}

static void LvGuiCtelCom_recorder_timer_cb(void *ctx)
{
	lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SUSPEND_IND, NULL);
}

static void LvGuiCtelCom_Call_Bright_Screen_timer_cb(void *ctx)
{
	OSI_PRINTFI("[CallBriScr][timecb](%s)(%d):cb, briscr_dir(%d), curscr_state(%d)", __func__, __LINE__, pocCtelAttr.call_briscr_dir, pocCtelAttr.call_curscr_state);
	switch(pocCtelAttr.call_briscr_dir)
	{
		case LVPOCBNDCOM_CALL_DIR_TYPE_ENTER:
		{
			poc_UpdateLastActiTime();
			break;
		}

		case LVPOCBNDCOM_CALL_DIR_TYPE_SPEAK:
		case LVPOCBNDCOM_CALL_DIR_TYPE_LISTEN:
		{
			osiTimerStop(pocCtelAttr.BrightScreen_timer);

			switch(pocCtelAttr.call_curscr_state)
			{
				case LVPOCBNDCOM_CALL_LASTSCR_STATE_OPEN:
				{
					OSI_PRINTFI("[callscr](%s)(%d):scr open, scr on continue", __func__, __LINE__);
					poc_UpdateLastActiTime();//continue last'state
					break;
				}

				case LVPOCBNDCOM_CALL_LASTSCR_STATE_CLOSE:
				{
					OSI_PRINTFI("[callscr](%s)(%d):scr close, scr off", __func__, __LINE__);
					poc_set_lcd_status(false);
					break;
				}
			}
			pocCtelAttr.call_briscr_dir = LVPOCBNDCOM_CALL_DIR_TYPE_ENTER;
			pocCtelAttr.call_curscr_state = LVPOCBNDCOM_CALL_LASTSCR_STATE_START;
			break;
		}

		default:
		{
			break;
		}
	}
}

void ui_Ctel_Inform(int result, CALLBACK_DATA *data)
{
	OSI_LOGXI(OSI_LOGPAR_IISI, 0,"[ctelcbmsg]ui_Ctel_Inform: data_addr(%p), data_type(%d), reason(%s), len(%d)", data, data->type, data->reason, data->len);
	switch(data->type)
	{
		case TYPE_NO_SIM://无sim卡
		{
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			m_CtelUser.m_status = USER_OFFLINE;
			break;
		}

		case TYPE_CANT_CONNECT_LTE://驻网失败
		{
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			m_CtelUser.m_status = USER_OFFLINE;
			break;
		}

		case TYPE_DATA_CALL_FAILED://拨号失败
		{
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			m_CtelUser.m_status = USER_OFFLINE;
			break;
		}

		case TYPE_PTT_LOGIN://PPT服务登录成功
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)login success", __LINE__);
			if(pocCtelAttr.loginstatus_t != LVPOCCTELCOM_SIGNAL_LOGIN_SUCCESS)
			{
				pocCtelAttr.invaildlogininf = false;
				static CALLBACK_DATA query_group_info;
				memcpy((void *)&query_group_info, (void *)data, sizeof(CALLBACK_DATA));
				lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_LOGIN_REP, (void *)&query_group_info);
			}
			break;
		}

		case TYPE_PTT_LOGOUT://PPT服务退出登录
		{
			break;
		}

		case TYPE_PTT_LOGIN_USERPWD_ERR://PPT用户名密码错误
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)login failed, password or account error", __LINE__);
            lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, true);
            lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "登录失败");
            pocCtelAttr.isReady = true;
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_FAILED;
			m_CtelUser.m_status = USER_OFFLINE;
			break;
		}

		case TYPE_PTT_CONNECT_TIMEOUT://PPT服务连接超时
		{
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			m_CtelUser.m_status = USER_OFFLINE;
			OSI_LOGE(0,  "[ctelcbmsg](%d)server connect time out", __LINE__);
			break;
		}

		case TYPE_PTT_REQUEST_MIC_SUCC://PPT请求MIC成功
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)request ppt mic success", __LINE__);
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SPEAK_START_REP, NULL);
			break;
		}

		case TYPE_PTT_REQUEST_MIC_FAILED://PPT请求MIC失败
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)request ppt mic failed", __LINE__);
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_REP, NULL);
			break;
		}

		case TYPE_PTT_MIC_LOST://PTT当前请求的MIC权限丢失
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)request ppt mic lose", __LINE__);
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_REP, NULL);
			break;
		}

		case TYPE_PTT_QUERY_GROUP_SUCC://PPT查询组信息成功
		{
			OSI_LOGE(0,  "[ctelcbmsg][group][server](%d)rev group info", __LINE__);
			static CALLBACK_DATA query_group_info;
			memcpy((void *)&query_group_info, (void *)data, sizeof(CALLBACK_DATA));
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_GROUP_LIST_QUERY_REP, (void *)&query_group_info);
			break;
		}

		case TYPE_PTT_QUERY_MEMBER_SUCC://PPT查询组成员成功
		{
			OSI_LOGE(0,  "[ctelcbmsg][groupmember][server](%d)rev groupmember info", __LINE__);
			static CALLBACK_DATA query_member_info;
			memcpy((void *)&query_member_info, (void *)data, sizeof(CALLBACK_DATA));
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_MEMBER_LIST_QUERY_REP, (void *)&query_member_info);
			break;
		}

		case TYPE_PTT_ENTER_GROUP://PPT进入群组
		{
			OSI_LOGE(0,  "[ctelcbmsg][grouprefr](%d)enter group success", __LINE__);
			static CALLBACK_DATA enter_group_info;
			memcpy((void *)&enter_group_info, (void *)data, sizeof(CALLBACK_DATA));
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_JOIN_GROUP_QUERY_GROUPMEMBER_REP, (void *)&enter_group_info);
			break;
		}

		case TYPE_PTT_LEAVE_GROUP://PPT退出群组
		{
			OSI_LOGE(0,  "[ctelcbmsg][grouprefr](%d)exit group success", __LINE__);
			break;
		}

		case TYPE_PTT_RELEASE_MIC://PPT释放MIC权限
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)request ppt release mic", __LINE__);
			static CALLBACK_DATA member_release_mic_info;
			memcpy((void *)&member_release_mic_info, (void *)data, sizeof(CALLBACK_DATA));
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_REP, (void *)&member_release_mic_info);
			break;
		}

		case TYPE_PTT_MEMBER_GET_MIC://成员获得MIC权限
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)member get mic permissions", __LINE__);
			lv_poc_setting_set_current_volume(POC_MMI_VOICE_PLAY, lv_poc_setting_get_current_volume(POC_MMI_VOICE_PLAY), true);
			static CALLBACK_DATA member_get_mic_info;
			memcpy((void *)&member_get_mic_info, (void *)data, sizeof(CALLBACK_DATA));
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_LISTEN_SPEAKER_REP, (void *)&member_get_mic_info);
			break;
		}

		case TYPE_PTT_MEMBER_LOST_MIC://成员失去MIC权限
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)member lose mic permissions", __LINE__);
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_LISTEN_STOP_REP, NULL);
			break;
		}

		case TYPE_PTT_TCP_CLOSE://PPT连接关闭
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)server connect close", __LINE__);
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			m_CtelUser.m_status = USER_OFFLINE;
			break;
		}

		case TYPE_PTT_SERVER_CLOSE://PPT服务停止
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)server connect stop", __LINE__);
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			m_CtelUser.m_status = USER_OFFLINE;
			break;
		}

		case TYPE_PTT_TMP_GROUP_CREAT://PPT创建临时群组，发起临时呼叫
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)crete temporary group", __LINE__);
			static CALLBACK_DATA creat_tmp_group_info;
			memcpy((void *)&creat_tmp_group_info, (void *)data, sizeof(CALLBACK_DATA));
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP, (void *)&creat_tmp_group_info);
			break;
		}

		case TYPE_PTT_TMP_GROUP_JOIN://PPT进入临时群组
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)enter temporary group", __LINE__);
			static CALLBACK_DATA enter_tmp_group_info;
			memcpy((void *)&enter_tmp_group_info, (void *)data, sizeof(CALLBACK_DATA));
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_BE_MEMBER_CALL_REP, (void *)&enter_tmp_group_info);
			break;
		}

		case TYPE_PTT_TMP_GROUP_QUIT://PPT退出临时群组
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)exit temporary group", __LINE__);
			static CALLBACK_DATA quit_tmp_group_info;
			memcpy((void *)&quit_tmp_group_info, (void *)data, sizeof(CALLBACK_DATA));
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP, (void *)&quit_tmp_group_info);
			break;
		}

		case TYPE_RECV_MSG://接收到短信息
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)receiver msg info", __LINE__);
			break;
		}

		case TYPE_CONNECT_SERVER_FAILED://服务器连接失败
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)server connect failed", __LINE__);
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			m_CtelUser.m_status = USER_OFFLINE;
			if(pocCtelAttr.invaildlogininf == false)
			{
				pocCtelAttr.invaildlogininf = true;
				pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_ERROR_MSG, (const uint8_t *)"无效登录信息", NULL);
	            lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "无效登录信息");
			}
			break;
		}

		case TYPE_PTT_INVALID_LOGININFO://无效的登录信息
		{
			if(pocCtelAttr.invaildlogininf == false)
			{
				pocCtelAttr.invaildlogininf = true;
				pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_ERROR_MSG, (const uint8_t *)"无效登录信息", NULL);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "无效登录信息");
			}
			break;
		}

		case TYPE_DNS_SERVER_CHANGED://服务器地址更改
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)dns server changed", __LINE__);
			static CALLBACK_DATA dns_server_info;
			memcpy((void *)&dns_server_info, (void *)data, sizeof(CALLBACK_DATA));
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SERVER_CHANGED_IND, (void *)&dns_server_info);
			break;
		}

		case TYPE_END://未知的原因
		{
			OSI_LOGE(0,  "[ctelcbmsg](%d)unknow case", __LINE__);
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			m_CtelUser.m_status = USER_OFFLINE;
			break;
		}

		default:
		{
			break;
		}

	}
}

int ui_Ctel_GetGprsAttState(void)
{
	return lv_poc_GetGprsAttState();
}

void ui_Ctel_GetSimInfo(char *imsi, unsigned int *lenimsi, char *iccid, unsigned int *leniccid, char *imei, unsigned int *lenimei)
{
	lv_poc_GetSimInfo(imsi, lenimsi, iccid, leniccid, imei, lenimei);
}

int ui_Ctel_Record(ENABLE_TYPE enable, pcm_record_cb cb)
{
	switch(enable)
	{
		case ENABLE_OPEN_START:
		{
			OSI_PRINTFI("(%s)(%d):[record]:start", __func__, __LINE__);
			//pocCtelAttr.send_record_data_cb = cb;
			//lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_START_SPEAK, NULL);
			break;
		}

		case DISABLE_CLOSE_STOP:
		{
			OSI_PRINTFI("(%s)(%d):[record]:stop", __func__, __LINE__);
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_STOP_RECORD_IND, NULL);
			pocCtelAttr.send_record_data_cb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
	return 1;
}

int ui_Ctel_Player(ENABLE_TYPE enable)
{
	switch(enable)
	{
		case ENABLE_OPEN_START:
		{
			OSI_PRINTFI("(%s)(%d):[player]:start", __func__, __LINE__);
			//lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_START_LISTEN, NULL);
			break;
		}

		case DISABLE_CLOSE_STOP:
		{
			OSI_PRINTFI("(%s)(%d):[player]:stop", __func__, __LINE__);
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_STOP_PLAY_IND, NULL);
			break;
		}

		default:
		{
			break;
		}
	}
	return 1;
}

int ui_Ctel_PlayPcm(char *pcm, int len)
{
	if(pocCtelAttr.player == 0)
	{
		return 0;
	}

#if POC_AUDIO_MODE_PIPE
	pocAudioPipeWriteData((const uint8_t *)pcm, (uint32_t)len);
#else
	pocAudioPlayerWriteData(pocCtelAttr.player, (const uint8_t *)pcm, (uint32_t)len);
#endif
	m_CtelUser.m_iRxCount = m_CtelUser.m_iRxCount + 1;

	return 1;
}

int ui_Ctel_Tone(ENABLE_TYPE enable, TONE_TYPE type)
{
	OSI_LOGI(0, "ui_Tone enable %d type %d\n",enable,type);
	if(enable == ENABLE_OPEN_START)
	{
		switch(type)
		{
			case PTT_START:
			{
				OSI_PRINTFI("(%s)(%d):[start][tone]:ctel start speak tone, no deal", __func__, __LINE__);
				//lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_START_SPEAK, NULL);
				//osiThreadSleep(200);//堵住对方线程
				break;
			}

			case PTT_STOP:
			{
				OSI_PRINTFI("(%s)(%d):[start][tone]:ctel stop speak tone", __func__, __LINE__);
				pocCtelAttr.speak_status = false;
				lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_STOP_SPEAK, NULL);
				break;
			}

			case ERROR:
			{
				OSI_PRINTFI("(%s)(%d):[start][tone]:ctel error tone", __func__, __LINE__);
				//reset
				pocCtelAttr.speak_status = false;
				pocCtelAttr.listen_status = false;
				break;
			}

			case PLAY_START:
			{
				OSI_PRINTFI("(%s)(%d):[start][tone]:ctel start listen tone", __func__, __LINE__);
				lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_START_LISTEN, NULL);
				break;
			}

			case PLAY_STOP:
			{
				OSI_PRINTFI("(%s)(%d):[start][tone]:ctel stop listen tone, no deal", __func__, __LINE__);
				pocCtelAttr.listen_status = false;
				lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_STOP_LISTEN, NULL);
				break;
			}

			default:
			{
				break;
			}
		}
	}
	else if(enable == DISABLE_CLOSE_STOP)
	{
		switch(type)
		{
			case PTT_START:
			{
				OSI_PRINTFI("(%s)(%d):[stop][tone]:speak_start tone", __func__, __LINE__);
				break;
			}

			case PTT_STOP:
			{
				OSI_PRINTFI("(%s)(%d):[stop][tone]:speak_stop tone", __func__, __LINE__);
				break;
			}

			case ERROR:
			{
				OSI_PRINTFI("(%s)(%d):[stop][tone]:error", __func__, __LINE__);
				break;
			}

			case PLAY_START:
			{
				OSI_PRINTFI("(%s)(%d):[stop][tone]:listen_start tone", __func__, __LINE__);
				break;
			}

			case PLAY_STOP:
			{
				OSI_PRINTFI("(%s)(%d):[stop][tone]:listen_stop tone", __func__, __LINE__);
				break;
			}

			default:
			{
				break;
			}
		}
	}

	return 1;
}

int ui_Ctel_TTS(ENABLE_TYPE enable, char *unicodestr, int len)
{
	OSI_LOGI(0, "ui_TTS enable %d \n",enable);
	return 1;
}

static void prvPocGuiCtelTaskHandleLogin(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_LOGIN_IND:
		{
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			ctel_stop_pttservice();
			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
			OSI_LOGXI(OSI_LOGPAR_SISI, 0, "[new login]ip_address(%s), ip_port(%d), vice_ip_address(%s), vice_ip_port(%d)", poc_config->ip_address, poc_config->ip_port, poc_config->vice_ip_address, poc_config->vice_ip_port);

			ctel_set_register_info((char*)poc_config->ip_address, poc_config->ip_port, (char*)poc_config->vice_ip_address, poc_config->vice_ip_port);
			ctel_start_pttservice();
			OSI_PRINTFI("(%s)(%d):[login]:start goto login in", __func__, __LINE__);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_LOGIN_REP:
		{
			if(pocCtelAttr.loginstatus_t == LVPOCCTELCOM_SIGNAL_LOGIN_SUCCESS
				|| ctx == 0)
			{
				return;
			}

			osiMutexLock(pocCtelAttr.lock);
			//audio
#if POC_AUDIO_MODE_PIPE
			pocCtelAttr.pipe = pocAudioPipeCreate(8192);//8kb
			pocCtelAttr.player = pocAudioPlayerCreate(1024);//1kb
#else
			if(pocCtelAttr.player == 0)
			{
				pocCtelAttr.player = pocAudioPlayerCreate(8192);//8kb
			}
#endif

			if(pocCtelAttr.recorder == 0)
			{
				pocCtelAttr.recorder = pocAudioRecorderCreate(40960, 320, 20, lvPocGuiCtelCom_send_data_callback);
			}

#if POC_AUDIO_MODE_PIPE
			if(pocCtelAttr.pipe == 0
				|| pocCtelAttr.recorder == 0
				|| pocCtelAttr.player == 0)
			{
				pocCtelAttr.isReady = false;
				osiMutexUnlock(pocCtelAttr.lock);
				osiThreadExit();
			}
#else
			if(pocCtelAttr.player == 0
				|| pocCtelAttr.recorder == 0)
			{
				pocCtelAttr.isReady = false;
				osiMutexUnlock(pocCtelAttr.lock);
				osiThreadExit();
			}
#endif

			CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;
			PTTUSER_INFO *user_info = (PTTUSER_INFO *)ctelcbdata->msgstruct;
			char selfname[45] = {0};
			char userid[45] = {0};

			//user info
			lv_poc_ctel_unicode_to_utf8_convert(user_info->name, (unsigned char *)selfname);
			strcpy((char *)pocCtelAttr.self_info.ucName, selfname);
			__itoa(user_info->uid, (char *)userid , 10);
			strcpy((char *)pocCtelAttr.self_info.ucNum, userid);
			OSI_PRINTFI("(%s)(%d):[login][success]:uid(%d), uname(%s)", __func__, __LINE__, user_info->uid, selfname);

			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

			if(0 == strcmp(poc_config->ip_address, user_info->serverinfo))
			{
				strcpy(poc_config->ip_address, user_info->serverinfo);
				poc_config->ip_port = user_info->port;
			}
			else
			{
				//main -> vice
				strcpy(poc_config->vice_ip_address, poc_config->ip_address);
				poc_config->vice_ip_port = poc_config->ip_port;
				//login ok -> main
				strcpy(poc_config->ip_address, user_info->serverinfo);
				poc_config->ip_port = user_info->port;
			}
			lv_poc_setting_conf_write();

			//login success
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, false);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, true);

			poc_play_voice_one_time(LVPOCAUDIO_Type_Success_Login, 50, true);
			osiTimerStart(pocCtelAttr.monitor_recorder_timer, 20000);//20S

			m_CtelUser.m_status = USER_ONLINE;
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_SUCCESS;
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "成功登录");
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, NULL);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_EXIT_IND:
		{
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SUSPEND_IND, NULL);
			m_CtelUser.m_status = USER_OFFLINE;
			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			ctel_stop_pttservice();
			pocCtelAttr.isReady = false;
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_EXIT_REP:
		{
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, true);
            lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, false);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiCtelTaskHandleSpeak(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_SPEAK_START_IND:
		{
			if(m_CtelUser.m_status == USER_OFFLINE)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"未登录", NULL);
				break;
			}

			if(lv_poc_stop_player_voice() != 2)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"呼叫错误", NULL);
				break;
			}

			osiMutexLock(pocCtelAttr.lock);
			m_CtelUser.m_status = USER_OPRATOR_START_SPEAK;
			if(osiTimerIsRunning(pocCtelAttr.monitor_recorder_timer))
			{
				osiTimerStop(pocCtelAttr.monitor_recorder_timer);
			}
			else
			{
				lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_RESUME_IND, NULL);
			}

			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER, NULL);
			osiMutexUnlock(pocCtelAttr.lock);

			OSI_PRINTFI("(%s)(%d):[ctelspeak]:start speak, request server", __func__, __LINE__);

			ctel_press_ppt_button();//ppt press
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_SPEAK_START_REP:
		{
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
            lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "开始对讲", NULL);
            lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始对讲", NULL);
            //开始闪烁
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS, true);

			if(pocCtelAttr.loginstatus_t == LVPOCCTELCOM_SIGNAL_LOGIN_SUCCESS)
            {
                char speak_name[100] = "";
                strcpy((char *)speak_name, (const char *)"主讲:");
                strcat((char *)speak_name, (const char *)pocCtelAttr.self_info.ucName);
                if(!pocCtelAttr.is_member_call)
                {
                    char group_name[100] = "";
                    strcpy((char *)group_name, (const char *)pocCtelAttr.current_group_info.m_ucGName);
                    lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, speak_name, group_name);
                    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)speak_name, (const uint8_t *)group_name);
                }
                else
                {
                    lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, speak_name, "");
                    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)speak_name, (const uint8_t *)"");
                }
                pocCtelAttr.is_makeout_call = true;
                osiTimerStart(pocCtelAttr.monitor_pptkey_timer, 50);
				OSI_PRINTFI("(%s)(%d):[ctelspeak]:start speak rep to dispaly idleview", __func__, __LINE__);
            }

			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_IND:
		{
			if(pocCtelAttr.listen_status == true)
			{
				return;
			}
			ctel_release_ppt_button();//ppt release
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_REP:
		{
			if(pocCtelAttr.is_makeout_call == true)
			{
	            //恢复run闪烁
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS, false);

	            //poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Speak, 30, true);
	            lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, "停止对讲", NULL);
	            lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"停止对讲", NULL);
	            lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, NULL, NULL);
	            lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				pocCtelAttr.is_makeout_call = false;
				osiTimerStop(pocCtelAttr.monitor_pptkey_timer);

				//monitor recorder thread
				osiTimerStart(pocCtelAttr.monitor_recorder_timer, 10000);//10S
				lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT, (void *)LVPOCBNDCOM_CALL_DIR_TYPE_SPEAK);
				OSI_PRINTFI("(%s)(%d):[ctelspeak]:receive stop_speak_rep", __func__, __LINE__);
			}
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiCtelTaskHandleGroupList(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_GROUP_LIST_QUERY_IND:
		{
            if(pocCtelAttr.pocGetGroupListCb == NULL)
            {
                break;
            }

			if(pocCtelAttr.loginstatus_t != LVPOCCTELCOM_SIGNAL_LOGIN_SUCCESS)
			{
                pocCtelAttr.pocGetGroupListCb(0, 0, NULL);
				pocCtelAttr.pocGetGroupListCb = NULL;
				return;
			}
			//query group
			ctel_query_group();
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取群组列表...", NULL);
			osiTimerStart(pocCtelAttr.check_ack_timeout_timer, 3000);
			OSI_PRINTFI("(%s)(%d):[grouprefr]:start go to query group ind", __func__, __LINE__);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_GROUP_LIST_QUERY_REP:
		{
			if(ctx == 0
				|| pocCtelAttr.pocGetGroupListCb == NULL)
            {
                break;
            }

			osiTimerStop(pocCtelAttr.check_ack_timeout_timer);
			int groupnum = 0;
			char groupid[64] = {0};
			CGroup groupinfo[32] = {0};
			CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;
			if(ctelcbdata == NULL)
			{
				OSI_PRINTFI("(%s)(%d):[ctelcbmsg]:no cb data", __func__, __LINE__);
				if(pocCtelAttr.pocGetGroupListCb != NULL)
				{
					pocCtelAttr.pocGetGroupListCb(0, 0, NULL);
					pocCtelAttr.pocGetGroupListCb = NULL;
				}
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				break;
			}

			groupinfolist *ctelgroupinf = (groupinfolist *)ctelcbdata->msgstruct;
			if(ctelgroupinf == NULL || ctelgroupinf->mtx == NULL)
			{
				OSI_PRINTFI("(%s)(%d):[ctelcbmsg]:no group data", __func__, __LINE__);
				if(pocCtelAttr.pocGetGroupListCb != NULL)
				{
					pocCtelAttr.pocGetGroupListCb(0, 0, NULL);
					pocCtelAttr.pocGetGroupListCb = NULL;
				}
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				break;
			}

			osiMutexLock(ctelgroupinf->mtx);
			linklistgroupinfo *pgroup = ctelgroupinf->head;
			if(pgroup == NULL)
			{
				OSI_PRINTFI("(%s)(%d):[ctelcbmsg]:phead NULL", __func__, __LINE__);
				if(pocCtelAttr.pocGetGroupListCb != NULL)
				{
					pocCtelAttr.pocGetGroupListCb(0, 0, NULL);
					pocCtelAttr.pocGetGroupListCb = NULL;
				}
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				break;
			}
			//get group info
			while(pgroup)
			{
				OSI_PRINTFI("(%s)(%d):[grouprefr]:groupname(%s), gid(%d), pri(%d), groupnum(%d)", __func__, __LINE__, pgroup->data.groupname,
									pgroup->data.groupid, pgroup->data.priority, groupnum);

				lv_poc_ctel_unicode_to_utf8_convert(pgroup->data.groupname, (unsigned char *)groupinfo[groupnum].m_ucGName);

				if(pgroup->data.groupname == NULL
					|| groupinfo[groupnum].m_ucGNum == NULL)
				{
					OSI_PRINTFI("(%s)(%d):[grouprefr]:group info error", __func__, __LINE__);
				}
				__itoa(pgroup->data.groupid, (char *)&groupid , 10);
				strcpy((char *)groupinfo[groupnum].m_ucGNum, groupid);
				groupinfo[groupnum].m_ucPriority = pgroup->data.priority;
				groupinfo[groupnum].m_index = groupnum;
				groupnum++;
				pgroup = pgroup->next;
			}
			osiMutexUnlock(ctelgroupinf->mtx);

			if(groupnum == 0)
			{
				if(pocCtelAttr.pocGetGroupListCb != NULL)
				{
					pocCtelAttr.pocGetGroupListCb(0, 0, NULL);
					pocCtelAttr.pocGetGroupListCb = NULL;
				}
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无有效群组", NULL);
				break;
			}

			OSI_PRINTFI("(%s)(%d):[grouprefr]:query group rep, start goto analysis", __func__, __LINE__);
			pocCtelAttr.m_Group_Num = groupnum;

			for(int i = 0; i < groupnum; i++)
			{
				strcpy((char *)pocCtelAttr.m_Group[i].m_ucGName, (char *)groupinfo[i].m_ucGName);
				strcpy((char *)pocCtelAttr.m_Group[i].m_ucGNum, (char *)groupinfo[i].m_ucGNum);
				pocCtelAttr.m_Group[i].m_ucPriority = groupinfo[i].m_ucPriority;
				pocCtelAttr.m_Group[i].m_index = groupinfo[i].m_index;
			}
			pocCtelAttr.pocGetGroupListCb(1, groupnum, groupinfo);
			pocCtelAttr.pocGetGroupListCb = NULL;
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocCtelAttr.pocGetGroupListCb = (lv_poc_get_group_list_cb_t)ctx;
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
		{
			pocCtelAttr.pocGetGroupListCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiCtelTaskHandleMemberList(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
		{
			if(pocCtelAttr.pocGetMemberListCb == NULL)
            {
                break;
            }

            if(pocCtelAttr.loginstatus_t != LVPOCCTELCOM_SIGNAL_LOGIN_SUCCESS)
            {
                pocCtelAttr.pocGetMemberListCb(0, 0, NULL);
                pocCtelAttr.pocGetMemberListCb = NULL;
                return;
            }

            if(ctx == 0)
            {
				int groupid = atoi((char *)pocCtelAttr.current_group_info.m_ucGNum);

				if(groupid == 0)
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无效组", NULL);
					osiTimerStart(pocCtelAttr.check_ack_timeout_timer, 3000);
					break;
				}
				ctel_query_member(atoi((char *)pocCtelAttr.current_group_info.m_ucGNum));
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取成员列表...", NULL);
				osiTimerStart(pocCtelAttr.check_ack_timeout_timer, 3000);
				OSI_PRINTFI("(%s)(%d):[memberrefr]:query member list, groupid(%d)", __func__, __LINE__, groupid);
			}
			else//no deal, get group member
			{

			}

			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_JOIN_GROUP_QUERY_GROUPMEMBER_REP:
		{
			if(ctx == 0)
            {
                break;
            }

			CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;
			if(ctelcbdata == NULL)
			{
				OSI_PRINTFI("(%s)(%d):[entergroup]:error", __func__, __LINE__);
				break;
			}

			pttgroupinfo *ctelgroupinf = (pttgroupinfo *)ctelcbdata->msgstruct;

			char curgroupname[45] = {0};
			char curgroupid[45] = {0};

			lv_poc_ctel_unicode_to_utf8_convert(ctelgroupinf->name, (unsigned char *)curgroupname);
			//current group info
			__itoa(ctelgroupinf->gid, (char *)curgroupid , 10);
			strcpy((char *)pocCtelAttr.current_group_info.m_ucGName, curgroupname);
			strcpy((char *)pocCtelAttr.current_group_info.m_ucGNum, curgroupid);
			//show
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, pocCtelAttr.self_info.ucName, curgroupname);

			OSI_PRINTFI("(%s)(%d):[entergroup]:groupid(%d), curgroupname(%s), selfname(%s)", __func__, __LINE__, curgroupid, (char *)curgroupname, pocCtelAttr.self_info.ucName);
			if(ctelgroupinf->gid == 0)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无效组", NULL);
				break;
			}

			if(pocCtelAttr.pocGetMemberListCb != NULL)
			{
				OSI_PRINTFI("(%s)(%d):[groupmember]enter group success, to query group member", __func__, __LINE__);
				ctel_query_member(ctelgroupinf->gid);
			}
			else
			{
				OSI_PRINTFI("(%s)(%d):[groupmember]cb NULL, enter group success", __func__, __LINE__);
			}
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_MEMBER_LIST_QUERY_REP://目前最大64个成员-->看情况修改-->lv_poc_lib.h修改
		{
			if(pocCtelAttr.pocGetMemberListCb == NULL)
			{
				break;
			}

			osiTimerStop(pocCtelAttr.check_ack_timeout_timer);
			int membernum = 0;
			char memberid[32] = {0};
			Msg_GData_s memberinfo = {0};
			CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;

			OSI_PRINTFI("(%s)(%d):[groupmember]rec memberlist info", __func__, __LINE__);
			if(ctelcbdata == NULL)
			{
				OSI_LOGE(0,  "[grouprefr][ctelcbmsg]no cb data");
                pocCtelAttr.pocGetMemberListCb(0, 0, NULL);
				pocCtelAttr.pocGetMemberListCb = NULL;
				return;
			}

			userinfolist *ctelgroupmemberinf = (userinfolist *)ctelcbdata->msgstruct;
			if(ctelgroupmemberinf == NULL || ctelgroupmemberinf->mtx == NULL)
			{
				OSI_LOGE(0,  "[grouprefr][ctelcbmsg]no group member data");
                pocCtelAttr.pocGetMemberListCb(0, 0, NULL);
				pocCtelAttr.pocGetMemberListCb = NULL;
				return;
			}

			osiMutexLock(ctelgroupmemberinf->mtx);
			linklistuserinfo *pmember = ctelgroupmemberinf->head;
			if(pmember == NULL)
			{
				OSI_LOGE(0,  "[grouprefr][ctelcbmsg]phead NULL");
                pocCtelAttr.pocGetMemberListCb(0, 0, NULL);
				pocCtelAttr.pocGetMemberListCb = NULL;
				return;
			}
			//get group member info
			while(pmember)
			{
				OSI_PRINTFI("(%s)(%d):[groupmember]:membername(%s), status(%d), uid(%d), number(%d)",  __func__, __LINE__, pmember->data.name, pmember->data.online, pmember->data.uid, membernum);

				lv_poc_ctel_unicode_to_utf8_convert(pmember->data.name, (unsigned char *)memberinfo.member[membernum].ucName);
				memberinfo.member[membernum].ucStatus = pmember->data.online;//1--online,0--offline
				__itoa(pmember->data.uid, (char *)memberid , 10);
				strcpy((char *)memberinfo.member[membernum].ucNum, memberid);
				membernum++;
				pmember = pmember->next;
			}
			osiMutexUnlock(ctelgroupmemberinf->mtx);

			if(membernum == 0)
			{
                pocCtelAttr.pocGetMemberListCb(0, 0, NULL);
				pocCtelAttr.pocGetMemberListCb = NULL;
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无其他有效成员", NULL);
				return;
			}
            //refr member
            memberinfo.dwNum = membernum;
            pocCtelAttr.pocGetMemberListCb(2, membernum, &memberinfo);
			pocCtelAttr.pocGetMemberListCb = NULL;
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			OSI_PRINTFI("(%s)(%d):[groupmember]:member info, number is(%d), go to refr",  __func__, __LINE__, memberinfo.dwNum);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocCtelAttr.pocGetMemberListCb = (lv_poc_get_member_list_cb_t)ctx;
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			pocCtelAttr.pocGetMemberListCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiCtelTaskHandleCurrentGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_SET_CURRENT_GROUP_IND:
		{
			if(ctx == 0)
			{
				OSI_PRINTFI("(%s)(%d)[setcurgrp]:error", __func__, __LINE__);
				break;
			}

			CGroup * group_info = (CGroup *)ctx;

			OSI_PRINTFI("(%s)(%d)[setcurgrp]:oldgroupname(%s), oldgid(%s), oldpri(%d), m_index(%d)", __func__, __LINE__, group_info->m_ucGName,
						group_info->m_ucGNum, group_info->m_ucPriority, group_info->m_index);

			//due to cannot save group info
			if(group_info->m_ucPriority == 0)
			{
				strcpy((char *)group_info->m_ucGName, (char *)pocCtelAttr.m_Group[group_info->m_index].m_ucGName);
				strcpy((char *)group_info->m_ucGNum, (char *)pocCtelAttr.m_Group[group_info->m_index].m_ucGNum);
				group_info->m_ucPriority = pocCtelAttr.m_Group[group_info->m_index].m_ucPriority;
			}

			if(pocCtelAttr.pocSetCurrentGroupCb == NULL)
			{
				break;
			}

			int groupid = atoi((char *)group_info->m_ucGNum);
			int index = 0;

			for(index = 0; index < pocCtelAttr.m_Group_Num; index++)
			{
				if(NULL != strstr((const char *)group_info->m_ucGNum, (const char *)pocCtelAttr.m_Group[index].m_ucGNum)) break;
			}

			if(pocCtelAttr.m_Group_Num < 1
				|| index >=  pocCtelAttr.m_Group_Num
				|| 0 == pocCtelAttr.m_Group[index].m_ucGNum[0])
			{
				OSI_PRINTFI("(%s)(%d)[setcurgrp]:no this group, Group_Index(%d)", __func__, __LINE__, index);
				pocCtelAttr.pocSetCurrentGroupCb(0);
				break;
			}

			//refr view
			lv_task_t *oncetask = lv_task_create(lv_poc_activity_func_cb_set.member_list.group_member_act, 50, LV_TASK_PRIO_HIGHEST, (void *)pocCtelAttr.current_group_info.m_ucGName);
			lv_task_once(oncetask);
			if(index == pocCtelAttr.current_group)//已在群组
			{
				OSI_PRINTFI("(%s)(%d)[setcurgrp]:have in group", __func__, __LINE__);
				pocCtelAttr.pocSetCurrentGroupCb(2);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocCtelAttr.self_info.ucName, pocCtelAttr.current_group_info.m_ucGName);
			}
			else//切组成功
			{
				OSI_PRINTFI("(%s)(%d)[setcurgrp]:switch group success", __func__, __LINE__);
				pocCtelAttr.current_group = index;
				nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
				strcpy((char *)poc_config->old_account_current_group, (const char *)group_info->m_ucGNum);
				lv_poc_setting_conf_write();
				pocCtelAttr.pocSetCurrentGroupCb(1);
				//new curgro
				strcpy((char *)pocCtelAttr.current_group_info.m_ucGName, (char *)group_info->m_ucGName);
				strcpy((char *)pocCtelAttr.current_group_info.m_ucGNum, (char *)group_info->m_ucGNum);
				pocCtelAttr.current_group_info.m_ucPriority = group_info->m_ucPriority;

				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocCtelAttr.self_info.ucName, pocCtelAttr.current_group_info.m_ucGName);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取群组成员...", NULL);
				osiTimerStart(pocCtelAttr.check_ack_timeout_timer, 3000);
			}

			OSI_PRINTFI("(%s)(%d)[setcurgrp]:newcurgroupname(%s), newcurgid(%s), newcurpri(%d)", __func__, __LINE__, pocCtelAttr.current_group_info.m_ucGName,
									pocCtelAttr.current_group_info.m_ucGNum, pocCtelAttr.current_group_info.m_ucPriority);
			pocCtelAttr.pocSetCurrentGroupCb = NULL;

			if(groupid == 0)
			{
				pocCtelAttr.pocSetCurrentGroupCb(0);
				pocCtelAttr.pocSetCurrentGroupCb = NULL;
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无效组", NULL);
				break;
			}
			//enter grp
			OSI_PRINTFI("(%s)(%d)[setcurgrp]:apply enter group", __func__, __LINE__);
			ctel_enter_group(groupid);

			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocCtelAttr.pocSetCurrentGroupCb = (poc_set_current_group_cb)ctx;
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
		{
			pocCtelAttr.pocSetCurrentGroupCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

//#ifdef CONFIG_POC_BUILD_GROUP_SUPPORT
//static void prvPocGuiCtelTaskHandleBuildGroup(uint32_t id, uint32_t ctx)
//{
//	switch(id)
//	{
//		case LVPOCGUICTELCOM_SIGNAL_BIUILD_GROUP_IND:
//		{
//			if(ctx == 0)
//			{
//				break;
//			}
//
//			if(pocCtelAttr.pocBuildGroupCb == NULL)
//			{
//				break;
//			}
//
//			lv_poc_build_new_group_t * new_group = (lv_poc_build_new_group_t *)ctx;
//			int m_GUNum = new_group->num;
//			int callnumber[64] = {0};//目前最大64个成员-->看情况修改
//			Msg_GData_s *member = NULL;
//
//			for(int i = 0; i < m_GUNum; i++)
//			{
//				member = (Msg_GData_s *)new_group->members[i];
//				callnumber[i] = atoi((char *)member->member[i].ucNum);
//				OSI_LOGE(0,  "[ctelcbmsg][buildgroup](%d)member_num(%d), number(%d)", __LINE__, callnumber[i], m_GUNum);
//			}
//
//			OSI_LOGXI(OSI_LOGPAR_IS, 0,"----------line(%d)----------func(%s)----------", __LINE__, __func__);
//			OSI_LOGE(0,  "[ctelcbmsg][buildgroup](%d)start go to build group ind", __LINE__);
//			if(ctel_call_members(callnumber, m_GUNum) != 0)
//			{
//				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)"添加组操作失败", (const uint8_t *)"");
//			}
//			break;
//		}
//
//		case LVPOCGUICTELCOM_SIGNAL_BIUILD_GROUP_REP:
//		{
//			if(ctx == 0)
//			{
//				break;
//			}
//
//			if(pocCtelAttr.pocBuildGroupCb == NULL)
//			{
//				break;
//			}
//
//			//pocCtelAttr.pocBuildGroupCb(1);
//			pocCtelAttr.pocBuildGroupCb = NULL;
//
//			break;
//		}
//
//		case LVPOCGUICTELCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND:
//		{
//			if(ctx == 0)
//			{
//				break;
//			}
//
//			pocCtelAttr.pocBuildGroupCb = (poc_build_group_cb)ctx;
//			break;
//		}
//
//		case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND:
//		{
//			pocCtelAttr.pocBuildGroupCb = NULL;
//			break;
//		}
//
//		default:
//		{
//			break;
//		}
//	}
//}
//#endif

static void prvPocGuiCtelTaskHandleMemberInfo(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
		{
			pocCtelAttr.pocGetMemberStatusCb = (poc_get_member_status_cb)ctx;
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_MEMBER_INFO_IND:
		{
			Msg_GROUP_MEMBER_s *MemberStatus = (Msg_GROUP_MEMBER_s *)ctx;
			OSI_LOGI(0, "[memberstatus]member info enter");
			if(MemberStatus == NULL)
			{
				break;
			}

			if(UT_STATUS_ONLINE == MemberStatus->ucStatus)//online in group
			{
				OSI_LOGI(0, "[memberstatus]member info online");
				pocCtelAttr.pocGetMemberStatusCb(1);
			}
			else//offline
			{
				OSI_LOGI(0, "[memberstatus]member info offline");
				pocCtelAttr.pocGetMemberStatusCb(0);
			}

			break;
		}
	}
}

static void prvPocGuiCtelTaskHandleMemberStatus(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_MEMBER_STATUS_REP:
		{
			if(pocCtelAttr.pocGetMemberStatusCb == NULL)
			{
				break;
			}

			pocCtelAttr.pocGetMemberStatusCb(ctx > 0);
			pocCtelAttr.pocGetMemberStatusCb = NULL;
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
		{
			if(ctx == 0)
			{
				break;
			}
			pocCtelAttr.pocGetMemberStatusCb = (poc_get_member_status_cb)ctx;
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_MEMBER_STATUS_CB_REP:
		{
			pocCtelAttr.pocGetMemberStatusCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandlePlay(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_STOP_PLAY_IND:
		{
			if(pocCtelAttr.player == 0)
			{
				break;
			}

			osiMutexLock(pocCtelAttr.lock);
#if POC_AUDIO_MODE_PIPE
			pocAudioPipeStop();
#else
			pocAudioPlayerStop(pocCtelAttr.player);
#endif
			audevStopPlay();
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_START_PLAY_IND:
		{
			if(m_CtelUser.m_status == USER_OFFLINE
				|| pocCtelAttr.player == 0
				|| pocCtelAttr.is_listen_stop == false)
			{
				m_CtelUser.m_iRxCount = 0;
				m_CtelUser.m_iTxCount = 0;
				break;
			}
			osiMutexLock(pocCtelAttr.lock);
#if POC_AUDIO_MODE_PIPE
			pocAudioPipeStart();
#else
			pocAudioPlayerStart(pocCtelAttr.player);
#endif
			pocCtelAttr.listen_status = true;
			m_CtelUser.m_status = USER_OPRATOR_LISTENNING;
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleRecord(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_STOP_RECORD_IND:
		{
			if(pocCtelAttr.recorder == 0)
			{
				break;
			}
			osiMutexLock(pocCtelAttr.lock);
			OSI_LOGI(0, "[record](%d):stop record", __LINE__);
			pocAudioRecorderStop(pocCtelAttr.recorder);
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_START_RECORD_IND:
		{
			if(pocCtelAttr.recorder == 0)
			{
				break;
			}
			osiMutexLock(pocCtelAttr.lock);
			OSI_LOGI(0, "[record](%d):start record", __LINE__);
			pocCtelAttr.speak_status = true;
			m_CtelUser.m_status = USER_OPRATOR_SPEAKING;
			pocAudioRecorderStart(pocCtelAttr.recorder);
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_SEND_RECORD_DATA_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			if(pocCtelAttr.send_record_data_cb == NULL)
			{
				OSI_PRINTFI("[record](%s)%d):error, cb null", __func__, __LINE__);
				break;
			}

			osiMutexLock(pocCtelAttr.lock);
			unsigned char *pRecordData = (unsigned char *)ctx;
			pocCtelAttr.send_record_data_cb((char *)pRecordData, 320);
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiCtelTaskHandleMemberCall(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
		{
		    lv_poc_member_call_config_t *member_call_config = (lv_poc_member_call_config_t *)ctx;

			Msg_GROUP_MEMBER_s *member_call_obj = (Msg_GROUP_MEMBER_s *)member_call_config->members;

			if(member_call_config->func == NULL)
		    {
				OSI_LOGI(0, "[singlecall](%d)cb func null", __LINE__);
			    break;
		    }

		    if(member_call_config->enable && member_call_obj == NULL)
		    {
				OSI_LOGI(0, "[singlecall](%d)error param", __LINE__);
			    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"错误参数", NULL);
				member_call_config->func(2, 2);
				break;
		    }

			if(member_call_config->enable == true)//request single call
			{
				if(pocCtelAttr.member_call_dir == 0)
				{
					pocCtelAttr.pocMemberCallCb = member_call_config->func;
					pocCtelAttr.is_member_call = true;

					//request member signal call
					unsigned int callnumber[2] = {0};
					callnumber[0] = atoi((char *)member_call_obj->ucNum);
					if(callnumber == 0)
					{
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无效成员", NULL);
						member_call_config->func(2, 2);
						break;
					}
					ctel_call_members(callnumber, 1);

					OSI_LOGI(0, "[singlecall]start request single call");
				}else
				{
					member_call_config->func(0, 0);
					pocCtelAttr.is_member_call = true;

					OSI_LOGI(0, "[singlecall]recive single call");
				}
			}
			else//exit single
			{
				lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP, NULL);
				OSI_LOGI(0, "[singlecall]start local exit single call");
			}

			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
		{
			if(ctx == 0)
			{
				break;
			}

#ifdef SUPPORT_TMP_GROUP
			CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;
			MSGSTRUCT_ARRAY *tempgroup = (MSGSTRUCT_ARRAY *)ctelcbdata->msgstruct;
#endif
			//对方同意了单呼通话
		    if(pocCtelAttr.pocMemberCallCb != NULL)
		    {
			    pocCtelAttr.pocMemberCallCb(0, 0);
			    pocCtelAttr.pocMemberCallCb = NULL;
		    }
			OSI_LOGI(0, "[oemsinglecall][server]receive single call ack");
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
		{
			//CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;
			//int *groupid = (int *)ctelcbdata->msgstruct;

			pocCtelAttr.is_member_call = false;
			pocCtelAttr.member_call_dir = 0;

			poc_play_voice_one_time(LVPOCAUDIO_Type_Exit_Member_Call, 30, true);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"退出单呼", NULL);
			lv_poc_activity_func_cb_set.member_call_close();

			OSI_LOGI(0, "[singlecall][server]exit single call");
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP:
		{
			//request join group
			int groupid = atoi((char *)pocCtelAttr.current_group_info.m_ucGNum);
			if(groupid == 0)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无效组", NULL);
				break;
			}
			ctel_enter_group(groupid);
			OSI_LOGI(0, "[singlecall]local exit single call, join last current group");
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_BE_MEMBER_CALL_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			char tmpgroupname[45] = {0};
			CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;
			TMPGROUP_INFO *tempgroup = (TMPGROUP_INFO *)ctelcbdata->msgstruct;

			if(tempgroup == NULL)
			{
				OSI_LOGI(0, "[singlecall]tempgroup NULL");
				break;
			}

			OSI_LOGXI(OSI_LOGPAR_IIS, 0,"[singlecall][server]rev single call, address(%d), gid(%d), gname(%s)", tempgroup, tempgroup->gid, tempgroup->callingname);
			lv_poc_ctel_unicode_to_utf8_convert(tempgroup->callingname, (unsigned char *)tmpgroupname);

			//join singel call
			static Msg_GROUP_MEMBER_s member_call_obj = {0};
			memset(&member_call_obj, 0, sizeof(Msg_GROUP_MEMBER_s));
			strcpy((char *)member_call_obj.ucName, (const char *)tmpgroupname);//临时呼叫的群组名就是用户名
			member_call_obj.ucStatus = UT_STATUS_ONLINE;
			pocCtelAttr.member_call_dir = 1;
			pocCtelAttr.call_type = LVPOCCTELCOM_CALL_TYPE_SINGLE;
            lv_poc_activity_func_cb_set.member_call_open((void *)&member_call_obj);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiCtelTaskHandleListen(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_LISTEN_SPEAKER_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;
			MEMBER_INFO *memberinf = (MEMBER_INFO *)ctelcbdata->msgstruct;
			char membername[45] = {0};

			if(memberinf == NULL)
			{
				OSI_PRINTFI("[ctellisten](%s)(%d):memeber NULL", __func__, __LINE__);
				break;
			}

			OSI_PRINTFI("[ctellisten](%s)(%d):membername(%s), uid(%d), gid(%d)", __func__, __LINE__, memberinf->name, memberinf->uid, memberinf->gid);
			lv_poc_ctel_unicode_to_utf8_convert(memberinf->name, (unsigned char *)membername);

			char speaker_name[100];
			char *speaker_group = (char *)pocCtelAttr.current_group_info.m_ucGName;
			strcpy(speaker_name, (const char *)membername);
			strcat(speaker_name, (const char *)"正在讲话");

			pocCtelAttr.is_listen_stop = true;
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, true);

			//member call
			if(pocCtelAttr.is_member_call)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)"");
			}
			else
			{
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, speaker_name, speaker_group);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)speaker_group);
			}
			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER, NULL);
			OSI_PRINTFI("(%s)(%d):[ctellisten]:receive start listen", __func__, __LINE__);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_LISTEN_STOP_REP:
		{
			pocCtelAttr.is_listen_stop = false;

			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, false);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");

			if(pocCtelAttr.is_member_call == false)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)"停止聆听", (const uint8_t *)"");
				//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			}

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);

			lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT, (void *)LVPOCBNDCOM_CALL_DIR_TYPE_LISTEN);
			OSI_PRINTFI("[ctellisten](%s)(%d):stop listen rep and destory idle, windows", __func__, __LINE__);

			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiCtelTaskHandleGuStatus(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_GU_STATUS_REP:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiCtelTaskHandleReleaseListenTimer(uint32_t id, uint32_t ctx)
{
	if(pocCtelAttr.delay_close_listen_timer == NULL)
	{
		return;
	}

	osiTimerDelete(pocCtelAttr.delay_close_listen_timer);
	pocCtelAttr.delay_close_listen_timer = NULL;
}

static void prvPocGuiCtelTaskHandleRecordTaskOpt(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_SUSPEND_IND:
		{
			osiMutexLock(pocCtelAttr.lock);
			if((osiThread_t *)lv_poc_recorder_Thread() != NULL)
			{
				OSI_LOGI(0, "[recorder]start suspend");
				osiThreadSuspend((osiThread_t *)lv_poc_recorder_Thread());
			}
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_RESUME_IND:
		{
			osiMutexLock(pocCtelAttr.lock);
			if((osiThread_t *)lv_poc_recorder_Thread() != NULL)
			{
				OSI_LOGI(0, "[recorder]start resume");
				osiThreadResume((osiThread_t *)lv_poc_recorder_Thread());
			}
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiCtelTaskHandlePlayTone(uint32_t id, uint32_t ctx)
{
	osiMutexLock(pocCtelAttr.lock);
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_START_SPEAK:
		{
			OSI_PRINTFI("(%s)(%d):[tone]:launch start_speak tone", __func__, __LINE__);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Speak, 30, true);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_STOP_SPEAK:
		{
			OSI_PRINTFI("(%s)(%d):[tone]:launch stop_speak tone", __func__, __LINE__);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Speak, 30, true);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_START_LISTEN:
		{
			OSI_PRINTFI("(%s)(%d):[tone]:launch start_listen tone", __func__, __LINE__);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Listen, 30, true);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_STOP_LISTEN:
		{
			OSI_PRINTFI("(%s)(%d):[tone]:launch stop_listen tone", __func__, __LINE__);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			break;
		}

		default:
		{
			break;
		}
	}
	osiMutexUnlock(pocCtelAttr.lock);
}

static void prvPocGuiCtelTaskHandleChargerOpt(uint32_t id, uint32_t ctx)
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
			 //add shutdown opt
			 osiShutdown(OSI_SHUTDOWN_POWER_OFF);
			 break;
		 }
	 }
}

static void prvPocGuiCtelTaskHandleUploadGpsInfo(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUICTELCOM_SIGNAL_GPS_UPLOADING_IND:
      {
		 GPS_DATA_s *pGps = (GPS_DATA_s* )ctx;
         if(pGps != NULL
		 	&& pocCtelAttr.loginstatus_t == LVPOCCTELCOM_SIGNAL_LOGIN_SUCCESS)
         {
            bool reportstatus = ctel_report_location_info(pGps->longitude, pGps->latitude);
            if(reportstatus != 0)
            {
               OSI_LOGI(0,"[GPS]Report server failed");
               return;
            }
            OSI_LOGI(0,"[GPS]Report server success");
         }

         break;
      }

      default:
         break;
   }
}

static void prvPocGuiCtelTaskHandleCallBrightScreen(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER:
      {
		 osiMutexLock(pocCtelAttr.lock);
		 if(!poc_get_lcd_status())
		 {
			 poc_set_lcd_status(true);
			 pocCtelAttr.call_curscr_state = LVPOCBNDCOM_CALL_LASTSCR_STATE_CLOSE;
			 OSI_PRINTFI("[callscr](%s)(%d):cur scr close", __func__, __LINE__);
		 }
		 else
		 {
			 pocCtelAttr.call_curscr_state = LVPOCBNDCOM_CALL_LASTSCR_STATE_OPEN;
			 OSI_PRINTFI("[callscr](%s)(%d):cur scr open", __func__, __LINE__);
		 }
		 poc_UpdateLastActiTime();
		 osiTimerStartPeriodic(pocCtelAttr.BrightScreen_timer, 4000);
		 pocCtelAttr.call_briscr_dir = LVPOCBNDCOM_CALL_DIR_TYPE_ENTER;
		 osiMutexUnlock(pocCtelAttr.lock);
		 break;
      }

	  case LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT:
	  {
	  	  if(ctx == 0)
	  	  {
			 break;
		  }

		  osiMutexLock(pocCtelAttr.lock);
		  switch(ctx)
		  {
			 case LVPOCBNDCOM_CALL_DIR_TYPE_SPEAK:
			 {
				  pocCtelAttr.call_briscr_dir = LVPOCBNDCOM_CALL_DIR_TYPE_SPEAK;
				  osiTimerIsRunning(pocCtelAttr.BrightScreen_timer) ? \
	     		  osiTimerStop(pocCtelAttr.BrightScreen_timer) : 0;
				  osiTimerStart(pocCtelAttr.BrightScreen_timer, 4000);
				  OSI_PRINTFI("[callscr](%s)(%d):speak stop", __func__, __LINE__);
				  break;
			 }


			 case LVPOCBNDCOM_CALL_DIR_TYPE_LISTEN:
			 {
				  pocCtelAttr.call_briscr_dir = LVPOCBNDCOM_CALL_DIR_TYPE_LISTEN;
				  osiTimerIsRunning(pocCtelAttr.BrightScreen_timer) ? \
	     		  osiTimerStop(pocCtelAttr.BrightScreen_timer) : 0;
				  osiTimerStart(pocCtelAttr.BrightScreen_timer, 4000);
				  OSI_PRINTFI("[callscr](%s)(%d):listen stop", __func__, __LINE__);
				  break;
			 }
		  }
		  osiMutexUnlock(pocCtelAttr.lock);
		  break;
	  }

	  case LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_BREAK:
	  {
		  osiMutexLock(pocCtelAttr.lock);
	  	  pocCtelAttr.call_briscr_dir != LVPOCBNDCOM_CALL_DIR_TYPE_ENTER ? \
	      osiTimerIsRunning(pocCtelAttr.BrightScreen_timer) ? \
	      osiTimerStop(pocCtelAttr.BrightScreen_timer) : 0 \
	      : 0;
		  OSI_PRINTFI("[callscr](%s)(%d):break", __func__, __LINE__);
		  osiMutexUnlock(pocCtelAttr.lock);
		  break;
	  }

      default:
      {
          break;
      }
   }
}

static void prvPocGuiCtelTaskHandleOther(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUICTELCOM_SIGNAL_DELAY_IND:
		{
			if(ctx < 1)
			{
				break;
			}

			osiThreadSleepRelaxed(ctx, OSI_WAIT_FOREVER);
		}

		case LVPOCGUICTELCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP:
		{
			pocCtelAttr.query_group = pocCtelAttr.current_group;

			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF:
		{
			pocCtelAttr.isPocGroupListAll = false;

			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_SERVER_CHANGED_IND:
		{
			if(ctx == 0)
			{
				break;
			}

#if 0
			char serveripstr[20] = {0};
			int serverport = 0;
			CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;
			char *ctelserverinfo = (char *)ctelcbdata->msgstruct;

			if(ctelserverinfo == NULL)
			{
				OSI_LOGE(0,  "[serverinfo](%d)serverinfo NULL", __LINE__);
				break;
			}

			pocCtelAttr.loginstatus_t = LVPOCCTELCOM_SIGNAL_LOGIN_EXIT;
			OSI_LOGXI(OSI_LOGPAR_SII, 0,"[serverinfo]:serverinfo(%s)", ctelserverinfo);

			ctel_stop_pttservice();
			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
			lv_poc_serverinfo_to_ipport(ctelserverinfo, serveripstr, &serverport);

			if(serverport == 0)
			{
				strcpy(poc_config->ip_address, serveripstr);
			}
			else
			{
				strcpy(poc_config->ip_address, serveripstr);
				poc_config->ip_port = serverport;
			}

			OSI_LOGXI(OSI_LOGPAR_SISS, 0, "[login][new]ip_address(%s), ip_port(%d), account_name(%s), account_passwd(%s)", poc_config->ip_address, poc_config->ip_port, poc_config->account_name, poc_config->account_passwd);
			lv_poc_setting_conf_write();
#else
			CALLBACK_DATA *ctelcbdata = (CALLBACK_DATA *)ctx;
			SERVERCHANGED_INFO *ctelserverinfo = (SERVERCHANGED_INFO *)ctelcbdata->msgstruct;

			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

			strcpy(poc_config->ip_address, ctelserverinfo->newserverinfo);
			poc_config->ip_port = ctelserverinfo->newport;
			strcpy(poc_config->vice_ip_address, ctelserverinfo->oldserverinfo);
			poc_config->vice_ip_port = ctelserverinfo->oldport;
#endif

			ctel_set_register_info((char*)poc_config->ip_address, poc_config->ip_port, (char*)poc_config->vice_ip_address, poc_config->vice_ip_port);
			ctel_start_pttservice();
			OSI_PRINTFI("(%s)(%d):[login]:modify server ip, start goto login in", __func__, __LINE__);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND:
		{
			osiMutexLock(pocCtelAttr.lock);
			OSI_PRINTFI("[timeout](%s)(%d):stop", __func__, __LINE__);
			if(pocCtelAttr.check_ack_timeout_timer != NULL)
			{
				osiTimerStop(pocCtelAttr.check_ack_timeout_timer);
			}
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_VOICE_PLAY_START_IND:
		{
			osiMutexLock(pocCtelAttr.lock);
			OSI_PRINTFI("[poc][audev](%s)(%d):start", __func__, __LINE__);
			lv_poc_set_audevplay_status(true);
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		case LVPOCGUICTELCOM_SIGNAL_VOICE_PLAY_STOP_IND:
		{
			osiMutexLock(pocCtelAttr.lock);
			OSI_PRINTFI("[poc][audev](%s)(%d):stop", __func__, __LINE__);
			lv_poc_set_audevplay_status(false);
			osiMutexUnlock(pocCtelAttr.lock);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void pocGuiCtelComTaskEntry(void *argument)
{
	osiEvent_t event = {0};

    pocCtelAttr.isReady = true;
	m_CtelUser.Reset();

    for(int i = 0; i < 1; i++)
    {
	    osiThreadSleepRelaxed(1000, OSI_WAIT_FOREVER);
    }

	ctel_set_log_level(CTEL_DEBUG); //设置日志的优先级 默认为debug

	CTEL_REGISTER_CB ctelCb;
	ctelCb.callbackInform = ui_Ctel_Inform;
	ctelCb.callbackGetGprsAttState = ui_Ctel_GetGprsAttState;
	ctelCb.callbackGetSimInfo = ui_Ctel_GetSimInfo;
	ctelCb.callbackRecord = ui_Ctel_Record;
	ctelCb.callbackPlayer = ui_Ctel_Player;
	ctelCb.callbackPlayPcm = ui_Ctel_PlayPcm;
	ctelCb.callbackTone = ui_Ctel_Tone;
	ctelCb.callbackTTS = ui_Ctel_TTS;
	ctel_register_callback(&ctelCb); //注册回调

	ctel_set_task_priority(OSI_PRIORITY_NORMAL);//设置任务优先级
	ctel_set_headbeat_time(20);	//设置心跳周期
	//server
	nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
	OSI_LOGXI(OSI_LOGPAR_SISI, 0, "[login]ip_address(%s), ip_port(%d), vice_ip_address(%s), vice_ip_port(%d)", poc_config->ip_address, poc_config->ip_port, poc_config->vice_ip_address, poc_config->vice_ip_port);
	//设置主辅服务器地址
	ctel_set_register_info((char*)poc_config->ip_address, poc_config->ip_port, (char*)poc_config->vice_ip_address, poc_config->vice_ip_port);

	ctel_start_pttservice(); //启动ptt服务

    while(1)
    {
    	if(!osiEventWait(pocCtelAttr.thread, &event))
		{
			continue;
		}

		if(event.id != 100)
		{
			continue;
		}

		switch(event.param1)
		{
			case LVPOCGUICTELCOM_SIGNAL_LOGIN_IND:
			case LVPOCGUICTELCOM_SIGNAL_LOGIN_REP:
			case LVPOCGUICTELCOM_SIGNAL_EXIT_IND:
			case LVPOCGUICTELCOM_SIGNAL_EXIT_REP:
			{
				prvPocGuiCtelTaskHandleLogin(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_SPEAK_START_IND:
			case LVPOCGUICTELCOM_SIGNAL_SPEAK_START_REP:
			case LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_IND:
			case LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_REP:
			{
				prvPocGuiCtelTaskHandleSpeak(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_GROUP_LIST_QUERY_IND:
			case LVPOCGUICTELCOM_SIGNAL_GROUP_LIST_QUERY_REP:
			case LVPOCGUICTELCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
			case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
			{
				prvPocGuiCtelTaskHandleGroupList(event.param1, event.param2);
				break;
			}

#ifdef CONFIG_POC_BUILD_GROUP_SUPPORT
			case LVPOCGUICTELCOM_SIGNAL_BIUILD_GROUP_IND:
			case LVPOCGUICTELCOM_SIGNAL_BIUILD_GROUP_REP:
			case LVPOCGUICTELCOM_SIGNAL_ENTER_TEMPORATY_GROUP_REP:
			case LVPOCGUICTELCOM_SIGNAL_EXIT_TEMPORATY_GROUP_REP:
			case LVPOCGUICTELCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND:
			case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND:
			{
				prvPocGuiCtelTaskHandleBuildGroup(event.param1, event.param2);
				break;
			}
#endif

			case LVPOCGUICTELCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
			case LVPOCGUICTELCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
			case LVPOCGUICTELCOM_SIGNAL_JOIN_GROUP_QUERY_GROUPMEMBER_REP:
			case LVPOCGUICTELCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
			case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
			{
				prvPocGuiCtelTaskHandleMemberList(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_SET_CURRENT_GROUP_IND:
			case LVPOCGUICTELCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
			case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
			{
				prvPocGuiCtelTaskHandleCurrentGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_MEMBER_INFO_IND:
			case LVPOCGUICTELCOM_SIGNAL_MEMBER_INFO_REP:
			{
				prvPocGuiCtelTaskHandleMemberInfo(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_MEMBER_STATUS_REP:
			case LVPOCGUICTELCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
			case LVPOCGUICTELCOM_SIGNAL_CANCEL_REGISTER_MEMBER_STATUS_CB_REP:
			{
				prvPocGuiCtelTaskHandleMemberStatus(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_STOP_PLAY_IND:
			case LVPOCGUICTELCOM_SIGNAL_START_PLAY_IND:
			{
				prvPocGuiBndTaskHandlePlay(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_STOP_RECORD_IND:
			case LVPOCGUICTELCOM_SIGNAL_START_RECORD_IND:
			case LVPOCGUICTELCOM_SIGNAL_SEND_RECORD_DATA_IND:
			{
				prvPocGuiBndTaskHandleRecord(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
			case LVPOCGUICTELCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
			case LVPOCGUICTELCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
			case LVPOCGUICTELCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP:
			case LVPOCGUICTELCOM_SIGNAL_BE_MEMBER_CALL_REP:
			{
				prvPocGuiCtelTaskHandleMemberCall(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_LISTEN_START_REP:
			case LVPOCGUICTELCOM_SIGNAL_LISTEN_STOP_REP:
			case LVPOCGUICTELCOM_SIGNAL_LISTEN_SPEAKER_REP:
			{
				prvPocGuiCtelTaskHandleListen(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_GU_STATUS_REP:
			{
				prvPocGuiCtelTaskHandleGuStatus(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_RELEASE_LISTEN_TIMER_REP:
			{
				prvPocGuiCtelTaskHandleReleaseListenTimer(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_DELAY_IND:
			case LVPOCGUICTELCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP:
			case LVPOCGUICTELCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF:
			case LVPOCGUICTELCOM_SIGNAL_SERVER_CHANGED_IND:
			case LVPOCGUICTELCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND:
			case LVPOCGUICTELCOM_SIGNAL_VOICE_PLAY_START_IND:
			case LVPOCGUICTELCOM_SIGNAL_VOICE_PLAY_STOP_IND:
			{
				prvPocGuiCtelTaskHandleOther(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_SET_SHUTDOWN_POC:
			{
				bool status;

				status = lv_poc_get_charge_status();
				if(status == false)
				{
					prvPocGuiCtelTaskHandleChargerOpt(event.param1, event.param2);
				}
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_GPS_UPLOADING_IND:
            {
            	prvPocGuiCtelTaskHandleUploadGpsInfo(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_SUSPEND_IND:
			case LVPOCGUICTELCOM_SIGNAL_RESUME_IND:
			{
				prvPocGuiCtelTaskHandleRecordTaskOpt(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_START_SPEAK:
			case LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_STOP_SPEAK:
			case LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_START_LISTEN:
			case LVPOCGUICTELCOM_SIGNAL_PLAY_TONE_STOP_LISTEN:
			{
				prvPocGuiCtelTaskHandlePlayTone(event.param1, event.param2);
				break;
			}

			case LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER:
			case LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT:
			case LVPOCGUICTELCOM_SIGNAL_CALL_BRIGHT_SCREEN_BREAK:
			{
				prvPocGuiCtelTaskHandleCallBrightScreen(event.param1, event.param2);
				break;
			}

			default:
				OSI_LOGW(0, "[gic] receive a invalid event\n");
				break;
		}
    }
}

extern "C" void pocGuiCtelComStart(void)
{
    pocCtelAttr.thread = osiThreadCreate(
		"pocGuiCTELCom", pocGuiCtelComTaskEntry, NULL,
		APPTEST_THREAD_PRIORITY, APPTEST_STACK_SIZE,
		APPTEST_EVENT_QUEUE_SIZE);
	pocCtelAttr.get_member_list_timer = osiTimerCreate(pocCtelAttr.thread, LvGuiCtelCom_get_member_list_timer_cb, NULL);
	pocCtelAttr.get_group_list_timer = osiTimerCreate(pocCtelAttr.thread, LvGuiCtelCom_get_group_list_timer_cb, NULL);
	pocCtelAttr.check_ack_timeout_timer = osiTimerCreate(pocCtelAttr.thread, LvGuiCtelCom_check_ack_timeout_timer_cb, NULL);
	pocCtelAttr.check_listen_timer = osiTimerCreate(pocCtelAttr.thread, LvGuiCtelCom_check_listen_timer_cb, NULL);
	pocCtelAttr.try_login_timer = osiTimerCreate(pocCtelAttr.thread, LvGuiCtelCom_try_login_timer_cb, NULL);//注册尝试登录定时器
	pocCtelAttr.auto_login_timer = osiTimerCreate(pocCtelAttr.thread, LvGuiCtelCom_auto_login_timer_cb, NULL);//注册自动登录定时器
	pocCtelAttr.monitor_pptkey_timer = osiTimerCreate(pocCtelAttr.thread, LvGuiCtelCom_ppt_release_timer_cb, NULL);//检查ppt键定时器
	pocCtelAttr.monitor_recorder_timer = osiTimerCreate(pocCtelAttr.thread, LvGuiCtelCom_recorder_timer_cb, NULL);//检查是否有人在录音定时器
	pocCtelAttr.BrightScreen_timer = osiTimerCreate(pocCtelAttr.thread, LvGuiCtelCom_Call_Bright_Screen_timer_cb, NULL);

	pocCtelAttr.lock = osiMutexCreate();
}

static void lvPocGuiCtelCom_send_data_callback(uint8_t * data, uint32_t length)
{
	OSI_LOGI(0, "[bnd][record](%d):start write data to record, data(%d), lenth(%d)", __LINE__, data, length);
    if (pocCtelAttr.recorder == 0 || data == NULL || length < 1)
    {
	    return;
    }

    if(m_CtelUser.m_status != USER_OPRATOR_SPEAKING)
    {
	    return;
    }

	lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SEND_RECORD_DATA_IND, (void *)data);
	m_CtelUser.m_iTxCount = m_CtelUser.m_iTxCount + 1;
}

extern "C" void lvPocGuiCtelCom_Init(void)
{
	memset(&pocCtelAttr, 0, sizeof(PocGuiCtelComAttr_t));
	pocCtelAttr.is_release_call = true;
	pocCtelAttr.send_record_data_cb = NULL;
	pocCtelAttr.current_group = 33;
	pocGuiCtelComStart();
}

extern "C" bool lvPocGuiCtelCom_Msg(LvPocGuiCtelCom_SignalType_t signal, void * ctx)
{
    if (pocCtelAttr.thread == NULL || (signal != LVPOCGUICTELCOM_SIGNAL_LOGIN_IND && pocCtelAttr.isReady == false))
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
#ifdef SUPPORT_CTEL_MSG
	OSI_LOGE(0,  "[ctelcbmsg][ctelmsg](%d)signal(%d), address(%d)", __LINE__, event.param1, event.param2);
#endif
	return osiEventSend(pocCtelAttr.thread, &event);
}

extern "C" void lvPocGuiCtelCom_log(void)
{
	lvPocGuiCtelCom_Init();
}

int lvPocGuiCtelCom_get_status(void)
{
	return pocCtelAttr.loginstatus_t;
}

bool lvPocGuiCtelCom_get_listen_status(void)
{
	return pocCtelAttr.listen_status;
}

bool lvPocGuiCtelCom_get_speak_status(void)
{
	return pocCtelAttr.speak_status;
}

extern "C" void *lvPocGuiCtelCom_get_self_info(void)
{
	if(pocCtelAttr.loginstatus_t != LVPOCCTELCOM_SIGNAL_LOGIN_SUCCESS)
	{
		return NULL;
	}
	return &pocCtelAttr.self_info;
}

extern "C" void *lvPocGuiCtelCom_get_current_group_info(void)
{
	return &pocCtelAttr.current_group_info;
}

extern "C" void *lvPocGuiCtelCom_get_current_lock_group(void)
{
	return NULL;
}

extern "C" int lvPocGuiCtelCom_get_current_exist_selfgroup(void)
{
	nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

	return poc_config->is_exist_selfgroup;
}

bool lvPocGuiCtelCom_modify_current_group_info(CGroup *CurrentGroup)
{
	if(pocCtelAttr.loginstatus_t != LVPOCCTELCOM_SIGNAL_LOGIN_SUCCESS || CurrentGroup == NULL)
	{
	    return false;
	}
	memcpy(&pocCtelAttr.current_group_info, CurrentGroup, sizeof(CGroup));
	lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocCtelAttr.self_info.ucName, pocCtelAttr.current_group_info.m_ucGName);

    return true;
}

/*
      name : lv_poc_persist_ssl_hashKeyConvert
     param : none
    author : wangls
  describe : 字符串->(char)16进制->(wchar)16进制:小端(Little Endian)转化为大端(Big Endian)
      date : 2020-09-14
*/
unsigned int lv_poc_persist_ssl_hashKeyConvert(char *pUserInput, wchar_t *pKeyArray) //98de---de98
{
    if (NULL == pUserInput || NULL == pKeyArray)
    {
            return 0;
    }

    unsigned int uiKeySize = strlen(pUserInput) / 4;
    unsigned int i = 0;
    char cTempor = 0;

    while(i < uiKeySize)
    {
        //1
        if (*pUserInput >= '0' && *pUserInput <= '9')//9
        {
            cTempor = *pUserInput - 48;
        }
        else
        {
            cTempor = 0xa + (*pUserInput - 'a');
        }

        pKeyArray[i] = cTempor;
        pUserInput++;
        //2
        if (*pUserInput >= '0' && *pUserInput <= '9')//8
        {
            cTempor = *pUserInput - 48;
        }
        else
        {
            cTempor = 0xa + (*pUserInput - 'a');
        }

        pKeyArray[i] = (pKeyArray[i] << 4) | cTempor;
        pUserInput++;
        //3
        if (*pUserInput >= '0' && *pUserInput <= '9')//d098
        {
            cTempor = *pUserInput - 48;
        }
        else
        {
            cTempor = 0xa + (*pUserInput - 'a');
        }

        pKeyArray[i] = (cTempor << 12) | pKeyArray[i];//(pKeyArray[i] << 4) | cTempor;
        pUserInput++;
        //4
        if (*pUserInput >= '0' && *pUserInput <= '9')//de98
        {
            cTempor = *pUserInput - 48;
        }
        else
        {
            cTempor = 0xa + (*pUserInput - 'a');
        }

        pKeyArray[i] = (cTempor << 8) | pKeyArray[i];//(pKeyArray[i] << 4) | cTempor;
        pUserInput++;
        i++;
    }

    return uiKeySize;
}

/*****************************************************************************
 * 将一个字符的Unicode(UCS-2和UCS-4)编码转换成UTF-8编码.
 *
 * 参数:
 *    unic     字符的Unicode编码值
 *    pOutput  指向输出的用于存储UTF8编码值的缓冲区的指针
 *    outsize  pOutput缓冲的大小
 *
 * 返回值:
 *    返回转换后的字符的UTF8编码所占的字节数, 如果出错则返回 0 .
 *
 * 注意:
 *     1. UTF8没有字节序问题, 但是Unicode有字节序要求;
 *        字节序分为大端(Big Endian)和小端(Little Endian)两种;
 *        在Intel处理器中采用小端法表示, 在此采用小端法表示. (低地址存低位)
 *     2. 请保证 pOutput 缓冲区有最少有 6 字节的空间大小!
 ****************************************************************************/
int lv_poc_enc_unicode_to_utf8_one(unsigned long unic, unsigned char *pOutput,
        int outSize)
{

    if ( unic <= 0x0000007F )
    {
        // * U-00000000 - U-0000007F:  0xxxxxxx
        *pOutput     = (unic & 0x7F);
        return 1;
    }
    else if ( unic >= 0x00000080 && unic <= 0x000007FF )
    {
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
        *(pOutput+1) = (unic & 0x3F) | 0x80;
        *pOutput     = ((unic >> 6) & 0x1F) | 0xC0;
        return 2;
    }
    else if ( unic >= 0x00000800 && unic <= 0x0000FFFF )
    {
        // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
        *(pOutput+2) = (unic & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >>  6) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 12) & 0x0F) | 0xE0;
        return 3;
    }
    else if ( unic >= 0x00010000 && unic <= 0x001FFFFF )
    {
        // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+3) = (unic & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 12) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 18) & 0x07) | 0xF0;
        return 4;
    }
    else if ( unic >= 0x00200000 && unic <= 0x03FFFFFF )
    {
        // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+4) = (unic & 0x3F) | 0x80;
        *(pOutput+3) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 18) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 24) & 0x03) | 0xF8;
        return 5;
    }
    else if ( unic >= 0x04000000 && unic <= 0x7FFFFFFF )
    {
        // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        *(pOutput+5) = (unic & 0x3F) | 0x80;
        *(pOutput+4) = ((unic >>  6) & 0x3F) | 0x80;
        *(pOutput+3) = ((unic >> 12) & 0x3F) | 0x80;
        *(pOutput+2) = ((unic >> 18) & 0x3F) | 0x80;
        *(pOutput+1) = ((unic >> 24) & 0x3F) | 0x80;
        *pOutput     = ((unic >> 30) & 0x01) | 0xFC;
        return 6;
    }

    return 0;
}

/*
      name : lv_poc_ctel_unicode_to_utf8_convert
     param : none
    author : wangls
  describe : ctel unicode to utf8
      date : 2020-09-14
*/
int lv_poc_ctel_unicode_to_utf8_convert(char *pUserInput, unsigned char *pUserOutput)
{
    unsigned char ctelbufArr[64] = {0};
    int ctellen = 0;
    int alllen = 0;
    int k =0;

    wchar_t ctelbufBrr[64] = {0};

    int ctelsize = lv_poc_persist_ssl_hashKeyConvert(pUserInput, ctelbufBrr);
    for(int i = 0; i < ctelsize; i++)
    {
        ctellen = lv_poc_enc_unicode_to_utf8_one(ctelbufBrr[i], ctelbufArr, sizeof(ctelbufBrr)/sizeof(char));
        if(ctellen)
        {
            for(int j = 0; j < ctellen; j++)
            {
                pUserOutput[k++] = ctelbufArr[j];
            }
            alllen+=ctellen;
        }
    }

    return alllen;
}

/*
      name : lv_poc_serverinfo_to_ipport
     param : none
    author : wangls
  describe : ctel serverinfo to ip and port
      date : 2020-11-25
*/
void lv_poc_serverinfo_to_ipport(char *serverinfo, char *ip, int *port)
{
	char *pServerport = NULL;

	if(NULL != strrchr(serverinfo, ':'))
	{
		int portlen = 0;
		pServerport = strrchr(serverinfo, ':');
		pServerport++;
		portlen = strlen(pServerport);
		*port = atoi(pServerport);
		strncpy(ip, serverinfo, (strlen(serverinfo) - portlen - 1));
	}
	else
	{
		strncpy(ip, serverinfo, 20);
	}
}

extern "C" void lvPocGuiComLogSwitch(bool status)
{
    if(status)
    {
        //g_iLog = 1;
    }
    else
    {
        //g_iLog = 0;
    }
}

extern "C" lv_poc_cit_status_type lvPocGuiComCitStatus(lv_poc_cit_status_type status)
{
    if(status > LVPOCCIT_TYPE_READ_STATUS)
    {
        isCitStatus = status;
    }

    return isCitStatus;
}

#endif

