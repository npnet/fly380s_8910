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
#include "guiIdtCom_api.h"
#include "audio_device.h"
#include <sys/time.h>
#include "lv_include/lv_poc_lib.h"
#include "cJSON.h"

extern "C" lv_poc_activity_attribute_cb_set lv_poc_activity_func_cb_set;

#define GUIIDTCOM_DEBUG (0)

//#if 0
#ifdef CONFIG_POC_SUPPORT
#define FREERTOS
#define T_TINY_MODE

#include "IDT.h"

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

enum{
	USER_OPRATOR_START_SPEAK = 3,
	USER_OPRATOR_SPEAKING  = 4,
	USER_OPRATOR_START_LISTEN = 5,
	USER_OPRATOR_LISTENNING = 6,
};


char *GetCauseStr(USHORT usCause);
char *GetOamOptStr(DWORD dwOpt);
char *GetSrvTypeStr(SRV_TYPE_e SrvType);
static void lvPocGuiIdtCom_send_data_callback(uint8_t * data, uint32_t length);
static int LvGuiIdtCom_self_info_json_parse_status(void);
static bool LvGuiIdtCom_self_is_member_call(char * info);
static void LvGuiIdtCom_delay_close_listen_timer_cb(void *ctx);
static void lvPocGuiIdtCom_get_listen_status(LvPocGuiIdtCom_SignalType_t SignalType_t);

//--------------------------------------------------------------------------------
//      TRACE小函数
//  输入:
//      pcFormat:       格式串
//      ...:            参数
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      打印内容不能超过256字节
//--------------------------------------------------------------------------------
extern int IDT_TRACE(const char* pcFormat, ...)
{
    if (NULL == pcFormat)
        return -1;

    char   cBuf[256];
    va_list va;
    va_start(va, pcFormat);
    int iLen = 0;
    iLen = snprintf((char*)&cBuf[iLen], sizeof(cBuf) - iLen - 1, "IDT: ");
    iLen = vsnprintf((char*)&cBuf[iLen], sizeof(cBuf) - iLen - 1, pcFormat, va);
    va_end(va);
    if (iLen <= 0)
        return -1;

    OSI_LOGXE(OSI_LOGPAR_S, 0, "[poc][idt]%s", (char*)cBuf);
    return 0;
}

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
        for (i = 0; i < USER_MAX_GROUP; i++)
        {
            m_Group[i].m_ucGName[0] = 0;
            m_Group[i].m_ucGNum[0] = 0;
        }
        return 0;
    }

public:
	unsigned int m_Group_Num;
    CGroup  m_Group[USER_MAX_GROUP];//一个用户,最多处于32个组中
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
	MEDIAATTR_s attr;
	osiThread_t *thread;
	bool        isReady;
	POCAUDIOPLAYER_HANDLE   player;
	POCAUDIORECORDER_HANDLE recorder;
#if CONFIG_POC_AUDIO_DATA_IDT_BUFF_MAX_SIZE > 3
	pocAudioData_t pocAudioData[CONFIG_POC_AUDIO_DATA_IDT_BUFF_MAX_SIZE];
	uint8_t pocAudioData_read_index;
	uint8_t pocAudioData_write_index;
#endif
	lv_poc_get_group_list_cb_t pocGetGroupListCb;
	lv_poc_get_member_list_cb_t pocGetMemberListCb;
	poc_set_current_group_cb pocSetCurrentGroupCb;
	poc_get_member_status_cb pocGetMemberStatusCb;
	poc_build_group_cb       pocBuildGroupCb;
	poc_set_member_call_status_cb pocMemberCallCb;
	Msg_GData_s *pPocMemberList;//组成员结构体
	Msg_GROUP_MEMBER_s self_info;
	Msg_GROUP_MEMBER_s speaker;
	CGroup speaker_group;
	Msg_GROUP_MEMBER_s member_call_obj;
	bool is_member_call;
	int member_call_dir;  //0 tx,  1 rx
	unsigned int mic_ctl;
	DWORD current_group;
	DWORD query_group;
	osiTimer_t * delay_close_listen_timer;
	bool delay_close_listen_timer_running;
	char self_info_cjson_str[GUIIDTCOM_SELF_INFO_SZIE];
	cJSON * self_info_cjson;
} PocGuiIIdtComAttr_t;

typedef struct
{
	unsigned long dwOptCode;
	unsigned long dwSn;
	unsigned short wRes;
	GData_s pGroup;
} LvPocGuiIdtCom_Group_Operator_t;

typedef struct
{
	DWORD dwSn;
	WORD wRes;
	bool haveUser;
	UData_s pUser;
} LvPocGuiIdtCom_User_Operator_t;

//按键音标志(是否处于对讲状态)
typedef struct Listen_Status
{
    bool   listen_status;
}Lv_Listen_Sta;


CIdtUser m_IdtUser;
static PocGuiIIdtComAttr_t pocIdtAttr = {0};
static Lv_Listen_Sta lv_listen_sta = {0};






int Func_GQueryU(DWORD dwSn, UCHAR *pucGNum)
{
    QUERY_EXT_s ext;
    memset(&ext, 0, sizeof(ext));
    ext.ucGroup = 0;    //组下组不呈现
    ext.ucUser  = 1;    //查询用户
    ext.dwPage  = 0;    //从第0页开始
    ext.dwCount = GROUP_MAX_MEMBER;//每页有1024用户
    ext.ucOrder =0;     //排序方式,0按号码排序,1按名字排序
    IDT_GQueryU(dwSn, pucGNum, &ext);
    return 0;
}


//--------------------------------------------------------------------------------
//      用户状态指示
//  输入:
//      status:         当前用户状态
//      usCause:        原因值
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户状态发生变化
//--------------------------------------------------------------------------------
void callback_IDT_StatusInd(int status, unsigned short usCause)
{
    IDT_TRACE("callback_IDT_StatusInd: status=%d, usCause=%s(%d)", status, GetCauseStr(usCause), usCause);
	static LvPocGuiIdtCom_login_t login_status = {0};
	memset(&login_status, 0, sizeof(LvPocGuiIdtCom_login_t));
	login_status.status = status;
	login_status.cause = usCause;
    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LOGIN_REP, (void *)&login_status);
}

//--------------------------------------------------------------------------------
//      组信息指示
//  输入:
//      pGInfo:         组信息
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户状态发生变化
//--------------------------------------------------------------------------------
void callback_IDT_GInfoInd(USERGINFO_s *pGInfo)
{
    if (NULL == pGInfo)
        return;

    IDT_TRACE("callback_IDT_GInfoInd: FGCount=%d\n", pGInfo->usNum);


    static USERGINFO_s GInfo = {0};
    memcpy(&GInfo, (const void *)pGInfo, sizeof(USERGINFO_s));
    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_REP, (void *)&GInfo);
}

//--------------------------------------------------------------------------------
//      组/用户状态指示
//  输入:
//      pStatus:        状态
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户状态发生变化
//--------------------------------------------------------------------------------
void callback_IDT_GUStatusInd(GU_STATUSGINFO_s *pStatus)
{
    if (NULL == pStatus)
        return;

    IDT_TRACE("callback_IDT_GUStatusInd: usNum=%d", pStatus->usNum);

	static GU_STATUSGINFO_s gu_status = {0};

    memcpy(&gu_status, (const void *)pStatus, sizeof(GU_STATUSGINFO_s));
    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GU_STATUS_REP, (void *)&gu_status);
}

//--------------------------------------------------------------------------------
//      呼出应答
//  输入:
//      pUsrCtx:        用户上下文
//      pcPeerNum:      对方应答的号码,有可能与被叫号码不同
//      pcPeerName:     对方应答的用户名
//      SrvType:        业务类型,实际的业务类型,可能与MakeOut不同
//      pcUserMark:     用户标识
//      pcUserCallRef:  用户使用的呼叫参考号,主叫是自己,被叫是主叫,查看是对端
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      由IDT.dll调用,告诉用户对方应答
//--------------------------------------------------------------------------------
int callback_IDT_CallPeerAnswer(void *pUsrCtx, char *pcPeerNum, char *pcPeerName, SRV_TYPE_e SrvType, char *pcUserMark, char *pcUserCallRef)
{
    IDT_TRACE("callback_IDT_CallPeerAnswer: pUsrCtx=0x%x, pcPeerNum=%s, pcPeerName=%s, SrvType=%s(%d), pcUserMark=%s, pcUserCallRef=%s",
        pUsrCtx, pcPeerNum, pcPeerName, GetSrvTypeStr(SrvType), SrvType, pcUserMark, pcUserCallRef);

	if(pocIdtAttr.is_member_call)
	{
		if(m_IdtUser.m_iCallId != -1)
		{
			IDT_CallMicCtrl(m_IdtUser.m_iCallId, false);
		}
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_REP, (void *)1);
	}

	if(m_IdtUser.m_status == USER_OPRATOR_START_SPEAK)
	{
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_REP, (void *)2);
    }

    return 0;
}

//--------------------------------------------------------------------------------
//      收到呼入
//  输入:
//      ID:             IDT的呼叫ID
//      pcMyNum:        自己号码
//      pcPeerNum:      对方号码
//      pcPeerName:     对方名字
//      SrvType:        业务类型
//      pAttr:          媒体属性
//      pExtInfo:       附加信息
//      pcUserMark:     用户标识
//      pcUserCallRef:  用户使用的呼叫参考号,主叫是自己,被叫是主叫,查看是对端
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      由IDT.dll调用,告诉用户有呼叫进入
//--------------------------------------------------------------------------------
int callback_IDT_CallIn(int ID, char *pcMyNum, char *pcPeerNum, char *pcPeerName, SRV_TYPE_e SrvType, MEDIAATTR_s *pAttr, void *pExtInfo, char *pcUserMark, char *pcUserCallRef)
{
    IDT_TRACE("callback_IDT_CallIn: ID=%d, pcMyNum=%s, pcPeerNum=%s, pcPeerName=%s, SrvType=%s(%d), Attr=ARx(%d):ATx(%d), pcUserMark=%s, pcUserCallRef=%s",
        ID, pcMyNum, pcPeerNum, pcPeerName, GetSrvTypeStr(SrvType), SrvType, pAttr->ucAudioRecv, pAttr->ucAudioSend, pcUserMark, pcUserCallRef);
	if(m_IdtUser.m_iCallId != -1)
	{
		return 0;
	}

    m_IdtUser.m_iCallId = ID;
    m_IdtUser.m_iRxCount = 0;
    m_IdtUser.m_iTxCount = 0;
    memset(&pocIdtAttr.attr, 0, sizeof(MEDIAATTR_s));
    pocIdtAttr.attr.ucAudioRecv = 1;
    bool is_member_call = LvGuiIdtCom_self_is_member_call(pcUserMark);

    switch (SrvType)
    {
	    case SRV_TYPE_BASIC_CALL://单呼
	        {
		        IDT_CallRel(ID, NULL, CAUSE_SRV_NOTSUPPORT);
		        m_IdtUser.m_iCallId = -1;
	        }
	        break;

	    case SRV_TYPE_CONF://组呼
	        {
		        IDT_CallAnswer(m_IdtUser.m_iCallId, &pocIdtAttr.attr, NULL);
		        if(is_member_call)
	            {
			        static Msg_GROUP_MEMBER_s member_call_obj = {0};
			        memset(&member_call_obj, 0, sizeof(Msg_GROUP_MEMBER_s));
			        if(pcPeerName != NULL)
			        {
						strcpy((char *)member_call_obj.ucName, (const char *)pcPeerName);
					}
					else
					{
					member_call_obj.ucName[0] = 0;
					}

			        if(pcPeerNum != NULL)
			        {
						strcpy((char *)member_call_obj.ucNum, (const char *)pcPeerNum);
					}
					else
					{
						member_call_obj.ucNum[0] = 0;
					}
					member_call_obj.ucStatus = UT_STATUS_ONLINE;
		            pocIdtAttr.member_call_dir = 1;
		            lv_poc_activity_func_cb_set.member_call_open((void *)&member_call_obj);
	            }
	            else
	            {
			        if(pcPeerName != NULL)
			        {
						strcpy((char *)pocIdtAttr.speaker_group.m_ucGName, (const char *)pcPeerName);
					}
					else
					{
						pocIdtAttr.speaker_group.m_ucGName[0] = 0;
					}

			        if(pcPeerNum != NULL)
			        {
						strcpy((char *)pocIdtAttr.speaker_group.m_ucGNum, (const char *)pcPeerNum);
					}
					else
					{
						pocIdtAttr.speaker_group.m_ucGNum[0] = 0;
					}
	           }
	        }
	        break;

	    default:
	        IDT_CallRel(ID, NULL, CAUSE_SRV_NOTSUPPORT);
	        m_IdtUser.m_iCallId = -1;
	        break;
    }
    return 0;
}
//--------------------------------------------------------------------------------
//      对方或IDT内部释放呼叫
//  输入:
//      ID:             IDT的呼叫ID,通常不使用这个,但可能启动被叫后,用户还没有应答,就释放了
//      pUsrCtx:        用户上下文
//      uiCause:        释放原因值
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
int callback_IDT_CallRelInd(int ID, void *pUsrCtx, UINT uiCause)
{
    IDT_TRACE("callback_IDT_CallRelInd: ID=%d, pUsrCtx=0x%x, uiCause=%d, m_iCallId=%d", ID, pUsrCtx, uiCause, m_IdtUser.m_iCallId);
    m_IdtUser.m_iCallId = -1;
    m_IdtUser.m_iRxCount = 0;
    m_IdtUser.m_iTxCount = 0;

    int status = m_IdtUser.m_status;
    if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
    {
	    m_IdtUser.m_status = UT_STATUS_ONLINE;
    }

    if(status >= USER_OPRATOR_START_SPEAK && status <= USER_OPRATOR_SPEAKING)
    {
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND, NULL);
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP, (void *)status);
    }

    if(status >= USER_OPRATOR_START_LISTEN && status <= USER_OPRATOR_LISTENNING)
    {
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP, NULL);
    }

    if(pocIdtAttr.is_member_call)
    {
		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_REP, (void *)0);
    }
    return 0;
}
//--------------------------------------------------------------------------------
//      话权指示
//  输入:
//      pUsrCtx:        用户上下文
//      uiInd:          指示值:0话权被释放,1获得话权,与媒体属性相同
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
int callback_IDT_CallMicInd(void *pUsrCtx, UINT uiInd)
{
    IDT_TRACE("callback_IDT_CallMicInd: pUsrCtx=0x%x, uiInd=%d", pUsrCtx, uiInd);
    // 0本端不讲话
    // 1本端讲话
    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_REP, (void *)(uiInd + 1));
    return 0;
}
//--------------------------------------------------------------------------------
//      收到语音数据
//  输入:
//      pUsrCtx:        用户上下文
//      dwStreamId:     StreamId
//      ucCodec:        CODEC
//      pucBuf:         数据
//      iLen:           数据长度
//      dwTsOfs:        时戳空洞
//      dwTsLen:        数据块时戳长度
//      dwTs:           起始时戳
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
int callback_IDT_CallRecvAudioData(void *pUsrCtx, DWORD dwStreamId, UCHAR ucCodec, UCHAR *pucBuf, int iLen, DWORD dwTsOfs, DWORD dwTsLen, DWORD dwTs)
{
    IDT_TRACE("callback_IDT_CallRecvAudioData: pUsrCtx=0x%x, dwStreamId=%d, ucCodec=%d pucBuf=0x%x iLen=%d dwTsOfs=%d dwTsLen=%d dwTs=%d",
	    pUsrCtx, dwStreamId, ucCodec, pucBuf, iLen, dwTsOfs, dwTsLen, dwTs);
	if(m_IdtUser.m_status >= USER_OPRATOR_START_LISTEN || m_IdtUser.m_status <= USER_OPRATOR_LISTENNING)
	{
	    m_IdtUser.m_iRxCount = m_IdtUser.m_iRxCount + 1;

		pocAudioPlayerWriteData(pocIdtAttr.player, (const uint8_t *)pucBuf, iLen);
		if(m_IdtUser.m_iRxCount == 10)
		{
			m_IdtUser.m_status = USER_OPRATOR_LISTENNING;
			lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_START_PLAY_IND, NULL);
		}
	}
    return 0;
}

//--------------------------------------------------------------------------------
//      讲话方提示
//  输入:
//      pUsrCtx:        用户上下文
//      pcNum:          讲话方号码
//      pcName:         讲话方名字
//  返回:
//      0:              成功
//      -1:             失败
//--------------------------------------------------------------------------------
int callback_IDT_CallTalkingIDInd(void *pUsrCtx, char *pcNum, char *pcName)
{
    IDT_TRACE("callback_IDT_CallTalkingIDInd: pUsrCtx=0x%x, pcNum=%s, pcName=%s", pUsrCtx, pcNum, pcName);
    memset(&pocIdtAttr.speaker, 0, sizeof(Msg_GROUP_MEMBER_s));

    if((pcNum != NULL
	    && (strcmp((const char *)pcNum, (const char *)pocIdtAttr.self_info.ucNum) == 0
	    || strlen((const char *)pcNum) < 1))
	    || (pcName != NULL
	    && strlen((const char *)pcName) < 1)
	    || pcNum == NULL
	    || pcName == NULL
	    || m_IdtUser.m_status < UT_STATUS_ONLINE)
    {
	    if(m_IdtUser.m_status >= USER_OPRATOR_START_LISTEN && m_IdtUser.m_status <= USER_OPRATOR_LISTENNING)
	    {
		    if(pocIdtAttr.delay_close_listen_timer != NULL)
		    {
			    if(pocIdtAttr.delay_close_listen_timer_running)
			    {
				    osiTimerStop(pocIdtAttr.delay_close_listen_timer);
				    pocIdtAttr.delay_close_listen_timer_running = false;
			    }
			    osiTimerStart(pocIdtAttr.delay_close_listen_timer, 560);
			    pocIdtAttr.delay_close_listen_timer_running = true;
		    }
		    else
		    {
			    if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
			    {
				    m_IdtUser.m_status = UT_STATUS_ONLINE;
			    }
			    m_IdtUser.m_iRxCount = 0;
			    m_IdtUser.m_iTxCount = 0;
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP, NULL);
		    }
	    }
	    return 0;
    }
    strcpy((char *)pocIdtAttr.speaker.ucNum, (const char *)pcNum);
    strcpy((char *)pocIdtAttr.speaker.ucName, (const char *)pcName);

    pocIdtAttr.speaker.ucStatus = UT_STATUS_ONLINE;
    if(m_IdtUser.m_status < USER_OPRATOR_START_LISTEN || m_IdtUser.m_status > USER_OPRATOR_LISTENNING)
    {
	    if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
	    {
		    m_IdtUser.m_status = USER_OPRATOR_START_LISTEN;
	    }
		m_IdtUser.m_iRxCount = 0;
		m_IdtUser.m_iTxCount = 0;
		poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Listen, true);
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LISTEN_START_REP, NULL);
		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP, NULL);
	}
    return 0;
}

//--------------------------------------------------------------------------------
//      用户操作响应
//  输入:
//      dwOptCode:      操作码      OPT_USER_ADD
//      dwSn:           操作序号
//      wRes:           结果        CAUSE_ZERO
//      pUser:          用户信息
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户操作结果
//--------------------------------------------------------------------------------
void callback_IDT_UOptRsp(DWORD dwOptCode, DWORD dwSn, WORD wRes, UData_s* pUser)
{
    if (NULL == pUser)
    {
        IDT_TRACE("callback_IDT_UOptRsp: dwOptCode=%s(%d), dwSn=%d, wRes=%s(%d)",
            GetOamOptStr(dwOptCode), dwOptCode, dwSn, GetCauseStr(wRes), wRes);
    }
    else
    {
        IDT_TRACE("callback_IDT_UOptRsp: dwOptCode=%s(%d), dwSn=%d, wRes=%s(%d), pUser->ucNum=%s",
            GetOamOptStr(dwOptCode), dwOptCode, dwSn, GetCauseStr(wRes), wRes, pUser->ucNum);
    }

    static LvPocGuiIdtCom_User_Operator_t UOpt = {0};
    memset(&UOpt, 0 , sizeof(LvPocGuiIdtCom_User_Operator_t));
    UOpt.dwSn = dwSn;
    UOpt.wRes = wRes;
    if(pUser != NULL)
    {
	    UOpt.haveUser = true;
	    memcpy(&UOpt.pUser, pUser, sizeof(UData_s));
    }

    if(dwOptCode == OPT_USER_QUERY)
    {
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP, &UOpt);
    }
}

//--------------------------------------------------------------------------------
//      组操作响应
//  输入:
//      dwOptCode:      操作码
//      dwSn:           操作序号
//      wRes:           结果
//      pGroup:         组信息
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户操作结果
//--------------------------------------------------------------------------------
void callback_IDT_GOptRsp(DWORD dwOptCode, DWORD dwSn, WORD wRes,  GData_s *pGroup)
{
    if (NULL == pGroup)
    {
        IDT_TRACE("callback_IDT_GOptRsp: dwOptCode=%s(%d), dwSn=%d, wRes=%s(%d)",
            GetOamOptStr(dwOptCode), dwOptCode, dwSn, GetCauseStr(wRes), wRes);
        return;
    }

    IDT_TRACE("callback_IDT_GOptRsp: dwOptCode=%s(%d), dwSn=%d, wRes=%s(%d), pGroup->ucNum=%s, pGroup->dwNum=%d",
        GetOamOptStr(dwOptCode), dwOptCode, dwSn, GetCauseStr(wRes), wRes, pGroup->ucNum, pGroup->dwNum);

	static LvPocGuiIdtCom_Group_Operator_t grop = {0};

    grop.dwOptCode = dwOptCode;
    grop.dwSn = dwSn;
    grop.wRes = wRes;
    memcpy(&grop.pGroup, (const void *)pGroup, sizeof(GData_s));

    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GROUP_OPERATOR_REP, (void *)&grop);
}

//--------------------------------------------------------------------------------
//      OAM操作提示
//  输入:
//      dwOptCode:      操作码
//                      OPT_USER_ADD        pucUNum,pucUName,ucUAttr有效
//                      OPT_USER_DEL        pucUNum有效
//                      OPT_USER_MODIFY     pucUNum,pucUName,ucUAttr有效
//                      OPT_G_ADD           pucGNum,pucGName有效
//                      OPT_G_DEL           pucGNum有效
//                      OPT_G_MODIFY        pucGNum,pucGName有效
//                      OPT_G_ADDUSER       pucGNum,pucUNum,pucUName,ucUAttr有效
//                      OPT_G_DELUSER       pucGNum,pucUNum有效
//                      OPT_G_MODIFYUSER    pucGNum,pucUNum,pucUName,ucUAttr有效
//      pucGNum:        组号码
//      pucGName:       组名字
//      pucUNum:        用户号码
//      pucUName:       用户名字
//      ucUAttr:        用户属性
//  返回:
//      无
//  注意:
//      由IDT.dll调用,告诉用户组删除/添加用户等操作
//--------------------------------------------------------------------------------
void callback_IDT_OamNotify(DWORD dwOptCode, UCHAR *pucGNum, UCHAR *pucGName, UCHAR *pucUNum, UCHAR *pucUName, UCHAR ucUAttr)
{
    if (NULL != pucGNum)
    {
        IDT_TRACE("callback_IDT_OamNotify: dwOptCode=%s(%d), pucGNum=%s, ucUAttr=%d",
            GetOamOptStr(dwOptCode), dwOptCode, pucGNum, ucUAttr);
    }
    else if (NULL != pucUNum)
    {
        IDT_TRACE("callback_IDT_OamNotify: dwOptCode=%s(%d), pucUNum=%s, ucUAttr=%d",
            GetOamOptStr(dwOptCode), dwOptCode, pucUNum, ucUAttr);
    }
    else
    {
        IDT_TRACE("callback_IDT_OamNotify: dwOptCode=%s(%d), ucUAttr=%d",
            GetOamOptStr(dwOptCode), dwOptCode, ucUAttr);
    }
}

static int LvGuiIdtCom_self_info_json_parse_status(void)
{
	if(m_IdtUser.m_status < UT_STATUS_ONLINE)
	{
		return -1;
	}
	if(0 != IDT_GetStatus(pocIdtAttr.self_info_cjson_str, GUIIDTCOM_SELF_INFO_SZIE))
	{
		memset(pocIdtAttr.self_info_cjson_str, 0, GUIIDTCOM_SELF_INFO_SZIE);
		return -1;
	}

	if(pocIdtAttr.self_info_cjson != NULL)
	{
		cJSON_Delete(pocIdtAttr.self_info_cjson);
		pocIdtAttr.self_info_cjson = NULL;
	}

	pocIdtAttr.self_info_cjson = cJSON_Parse(pocIdtAttr.self_info_cjson_str);
	if(pocIdtAttr.self_info_cjson == NULL)
	{
		return -1;
	}

	char *self_name = cJSON_GetObjectItem(pocIdtAttr.self_info_cjson, "Name")->valuestring;
	char *self_num = cJSON_GetObjectItem(pocIdtAttr.self_info_cjson, "ID")->valuestring;
	int status = cJSON_GetObjectItem(pocIdtAttr.self_info_cjson, "Reg")->valueint;
	strcpy((char *)pocIdtAttr.self_info.ucName, (const char *)self_name);
	strcpy((char *)pocIdtAttr.self_info.ucNum, (const char *)self_num);
	pocIdtAttr.self_info.ucStatus = (uint8_t)status;
	lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, NULL);

	if(pocIdtAttr.self_info_cjson != NULL)
	{
		cJSON_Delete(pocIdtAttr.self_info_cjson);
		pocIdtAttr.self_info_cjson = NULL;
	}
	return 0;
}

static bool LvGuiIdtCom_self_is_member_call(char * info)
{
	bool ret = false;
	if(info == NULL)
	{
		return ret;
	}

	cJSON * user_mark_cjson = cJSON_Parse(info);

	if(user_mark_cjson == NULL)
	{
		return ret;
	}

	do
	{
		cJSON *method_cjson = cJSON_GetObjectItem(user_mark_cjson, "pfCallIn");
		if(method_cjson == NULL)
		{
			break;
		}

		if(method_cjson->valuestring == NULL)
		{
			break;
		}

		if(strcmp((const char *)GUIIDTCOM_MEMBER_CALL_HALFSINGLE, (const char *)method_cjson->valuestring) != 0)
		{
			break;
		}
		ret = true;
	}while(0);
	cJSON_Delete(user_mark_cjson);
	return ret;
}
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
}

//--------------------------------------------------------------------------------
//      用户调试函数
//  输入:
//      pcTxt:          调试字符串
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      FREERTOS调试比较麻烦,加入这个,便于远程调试
//--------------------------------------------------------------------------------
extern int _stricmp(const char *string1, const char *string2);
int callback_IDT_Dbg(char *pcTxt)
{
    if (NULL == pcTxt)
        return -1;
    IDT_TRACE("callback_IDT_Dbg: %s", pcTxt);

    MEDIAATTR_s attr;
    if (0 == _stricmp("query", pcTxt))
    {
        //查询信息
        char cBuf[256];
        IDT_GetStatus(cBuf, sizeof(cBuf));
        IDT_TRACE("IDT_GetStatus: %s", cBuf);
    }
    else if (0 == _stricmp("callout", pcTxt))
    {
        if (-1 != m_IdtUser.m_iCallId)
        {
            IDT_CallRel(m_IdtUser.m_iCallId, NULL, 0);
            osiThreadSleep(500);
            m_IdtUser.m_iCallId = -1;
        }

        //单呼呼出
        memset(&attr, 0, sizeof(attr));
        attr.ucAudioRecv = 1;
        attr.ucAudioSend = 1;
        //对端号码2013,基本呼叫,语音收发,视频没有
        m_IdtUser.m_iCallId = IDT_CallMakeOut((char*)"2013", SRV_TYPE_BASIC_CALL, &attr, NULL, NULL, 1, 0, 1, NULL);
        m_IdtUser.m_iRxCount = 0;
        m_IdtUser.m_iTxCount = 0;
    }
    else if (0 == _stricmp("gcallout", pcTxt))
    {
        if (-1 != m_IdtUser.m_iCallId)
        {
            IDT_CallRel(m_IdtUser.m_iCallId, NULL, 0);
            osiThreadSleep(500);
            m_IdtUser.m_iCallId = -1;
        }

        //组呼呼出
        memset(&attr, 0, sizeof(attr));
        attr.ucAudioSend = 1;
        //对端组号码2090,组呼,语音只发不收,视频没有
        m_IdtUser.m_iCallId = IDT_CallMakeOut((char*)"2090", SRV_TYPE_CONF, &attr, NULL, NULL, 1, 0, 1, NULL);
        m_IdtUser.m_iRxCount = 0;
        m_IdtUser.m_iTxCount = 0;
    }
    else if (0 == _stricmp("answer", pcTxt))
    {
        //单呼应答
        memset(&attr, 0, sizeof(attr));
        attr.ucAudioRecv = 1;
        attr.ucAudioSend = 1;
        IDT_CallAnswer(m_IdtUser.m_iCallId, &attr, NULL);
    }
    else if (0 == _stricmp("rel", pcTxt))
    {
        //释放呼叫
        IDT_CallRel(m_IdtUser.m_iCallId, NULL, CAUSE_ZERO);
        m_IdtUser.m_iCallId = -1;
    }
    else if (0 == _stricmp("ptton", pcTxt))
    {
        //请求话权
        IDT_CallMicCtrl(m_IdtUser.m_iCallId, true);
    }
    else if (0 == _stricmp("pttoff", pcTxt))
    {
        //释放话权
        IDT_CallMicCtrl(m_IdtUser.m_iCallId, false);
    }
    else if (0 == _stricmp("GQuery", pcTxt))
    {
        Func_GQueryU(0, m_IdtUser.m_Group.m_Group[0].m_ucGNum);
    }
    else if (0 == _stricmp("LockG", pcTxt))
    {
        IDT_SetGMemberExtInfo(0, NULL, (UCHAR*)"2090", NULL);
    }
    else if (0 == _stricmp("UnLockG", pcTxt))
    {
        IDT_SetGMemberExtInfo(0, NULL, (UCHAR*)"#", NULL);
    }
	return 0;
}

extern int g_iLog;

void IDT_Entry(void*)
{
    m_IdtUser.Reset();

    // 0关闭日志,1打开日志
    //g_iLog = 0;
    g_iLog = 1;

    static IDT_CALLBACK_s CallBack;
    memset(&CallBack, 0, sizeof(CallBack));
    CallBack.pfStatusInd        = callback_IDT_StatusInd;
    CallBack.pfGInfoInd         = callback_IDT_GInfoInd;
    CallBack.pfGUStatusInd      = callback_IDT_GUStatusInd;

    CallBack.pfCallPeerAnswer   = callback_IDT_CallPeerAnswer;
    CallBack.pfCallIn           = callback_IDT_CallIn;
    CallBack.pfCallRelInd       = callback_IDT_CallRelInd;
    CallBack.pfCallRecvAudioData= callback_IDT_CallRecvAudioData;
    CallBack.pfCallMicInd       = callback_IDT_CallMicInd;
    CallBack.pfCallTalkingIDInd = callback_IDT_CallTalkingIDInd;
    CallBack.pfCallRecvAudioData= callback_IDT_CallRecvAudioData;

    CallBack.pfUOptRsp          = callback_IDT_UOptRsp;
    CallBack.pfGOptRsp          = callback_IDT_GOptRsp;
    CallBack.pfOamNotify        = callback_IDT_OamNotify;

    CallBack.pfDbg              = callback_IDT_Dbg;

    IDT_Start(NULL, 1, (char*)"124.160.11.21", 10000, NULL, 0, (char*)"34042", (char*)"34042", 1, &CallBack, 0, 20000, 0);
}

static void prvPocGuiIdtTaskHandleLogin(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND:
		{
			if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
			{
				break;
			}
			IDT_Entry(NULL);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LOGIN_REP:
		{
			if(pocIdtAttr.player == 0)
			{
				pocIdtAttr.player = pocAudioPlayerCreate(81920);
			}

			if(pocIdtAttr.recorder == 0)
			{
				pocIdtAttr.recorder = pocAudioRecorderCreate(40960, 320, 20, lvPocGuiIdtCom_send_data_callback);
			}

			if(pocIdtAttr.player == 0 || pocIdtAttr.recorder == 0)
			{
				pocIdtAttr.isReady = false;
				osiThreadExit();
			}

			if(ctx == 0)
			{
				break;
			}
			LvPocGuiIdtCom_login_t * login_info = (LvPocGuiIdtCom_login_t *)ctx;

		    if (UT_STATUS_ONLINE > login_info->status)
		    {
				poc_play_voice_one_time(LVPOCAUDIO_Type_No_Login, true);
			    m_IdtUser.m_status = UT_STATUS_OFFLINE;
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "登录失败");
			    pocIdtAttr.isReady = false;
				break;
		    }
		    pocIdtAttr.isReady = true;

	        IDT_StatusSubs((char*)"###", GU_STATUSSUBS_BASIC);
	        if(m_IdtUser.m_status < UT_STATUS_ONLINE)
	        {
				poc_play_voice_one_time(LVPOCAUDIO_Type_Success_Login, true);
	        }
	        m_IdtUser.m_status = UT_STATUS_ONLINE;
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "成功登录");

			LvGuiIdtCom_self_info_json_parse_status();
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, NULL);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_EXIT_IND:
		{
			if(m_IdtUser.m_status < UT_STATUS_ONLINE)
			{
				break;
			}

			IDT_Exit();
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
			if(m_IdtUser.m_status < UT_STATUS_ONLINE)
			{
				break;
			}
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Listen, true);
			m_IdtUser.m_status = USER_OPRATOR_START_SPEAK;

			if(m_IdtUser.m_iCallId == -1)
			{
				memset(&pocIdtAttr.attr, 0, sizeof(MEDIAATTR_s));
				pocIdtAttr.attr.ucAudioSend = 1;
				SRV_TYPE_e srv_type = SRV_TYPE_NONE;
				char *dest_num = NULL;
				char *user_mark = NULL;
				if(pocIdtAttr.is_member_call)
				{
					srv_type = SRV_TYPE_SIMP_CALL;
					dest_num = (char *)pocIdtAttr.member_call_obj.ucNum;
					pocIdtAttr.speaker_group.m_ucGName[0] = 0;
					pocIdtAttr.speaker_group.m_ucGNum[0] = 0;
					user_mark = (char *)GUIIDTCOM_MEMBER_CALL_MARK;
				}
				else
				{
					srv_type = SRV_TYPE_CONF;
					dest_num = (char *)m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGNum;
					strcpy((char *)pocIdtAttr.speaker_group.m_ucGName, (const char *)m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
					strcpy((char *)pocIdtAttr.speaker_group.m_ucGNum, (const char *)m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGNum);
					user_mark = (char *)GUIIDTCOM_GROUP_CALL_MARK;
				}

				m_IdtUser.m_iCallId = IDT_CallMakeOut(dest_num,
					srv_type,
					&pocIdtAttr.attr,
					NULL,
					NULL,
					1,
					0,
					1,
					user_mark);
			}
			else
			{
		        lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_IND, GUIIDTCOM_REQUEST_MIC);
			}
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP:
		{
			//lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "开始对讲", NULL);
			//lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始对讲", NULL);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, "正在讲话", "");
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"正在讲话", (const uint8_t *)"");
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_IND:
		{
			if(m_IdtUser.m_status < UT_STATUS_ONLINE)
			{
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_REP, GUIIDTCOM_RELEASE_MIC);
				break;
			}
	        lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_IND, GUIIDTCOM_RELEASE_MIC);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP:
		{
			if(ctx == USER_OPRATOR_SPEAKING)
			{
				poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, true);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, "停止对讲", NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"停止对讲", NULL);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			}
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
			if(ctx == 0 || m_IdtUser.m_iCallId == -1)
			{
				break;
			}

			unsigned int mic_ctl = (unsigned int)ctx;
			if(mic_ctl <= 1)
			{
		        //释放话权
		        OSI_LOGI(0, "[gic][gicmic] release mic ctl\n");
		        IDT_CallMicCtrl(m_IdtUser.m_iCallId, false);
			}
			else
			{
		        //请求话权
		        OSI_LOGI(0, "[gic][gicmic] request mic ctl\n");
		        IDT_CallMicCtrl(m_IdtUser.m_iCallId, true);
			}
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MIC_REP:
		{
			if(ctx == 0 || m_IdtUser.m_iCallId == -1)
			{
				break;
			}
			unsigned int mic_ctl = (unsigned int)ctx;
			bool pttStatus = pocGetPttKeyState();

			if(mic_ctl > 1 && pocIdtAttr.mic_ctl <= 1 && m_IdtUser.m_status == USER_OPRATOR_START_SPEAK)  //获得话权
			{
				if(pttStatus)
				{
					m_IdtUser.m_iRxCount = 0;
					m_IdtUser.m_iTxCount = 0;
					//lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "获得话权", NULL);

				    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND, NULL);
				    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP, NULL);
				    pocIdtAttr.mic_ctl = mic_ctl;
			    }
			    else
			    {
				    poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Cannot_Speak, false);
				    IDT_CallMicCtrl(m_IdtUser.m_iCallId, false);
				    pocIdtAttr.mic_ctl = 0;
			    }
			}
			else if(mic_ctl <= 1 && pocIdtAttr.mic_ctl > 1) // 释放话权
			{
				int status = m_IdtUser.m_status;

		        if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
		        {
			        m_IdtUser.m_status = UT_STATUS_ONLINE;
		        }
				m_IdtUser.m_iRxCount = 0;
				m_IdtUser.m_iTxCount = 0;

				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND, NULL);

				if(status >= USER_OPRATOR_START_SPEAK && status <= USER_OPRATOR_SPEAKING)
				{
					//lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "释放话权", NULL);
			        lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP, (void *)status);
				}
				pocIdtAttr.mic_ctl = mic_ctl;
			}
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
			if (strlen((const char *)pocIdtAttr.self_info.ucNum) < 1)
			{
				break;
			}

			//IDT_UQueryG(0, pocIdtAttr.self_info.ucNum);
			do
			{
				if(pocIdtAttr.pocGetGroupListCb == NULL)
				{
					break;
				}


				if(m_IdtUser.m_Group.m_Group_Num < 1)
				{
					pocIdtAttr.pocGetGroupListCb(0, 0, NULL);
					break;
				}

				pocIdtAttr.pocGetGroupListCb(1, m_IdtUser.m_Group.m_Group_Num, m_IdtUser.m_Group.m_Group);
			}while(0);
			pocIdtAttr.pocGetGroupListCb = NULL;

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_REP:
		{
			if(ctx == 0)
			{
				break;
			}
			USERGINFO_s *pGInfo = (USERGINFO_s *)ctx;

		    m_IdtUser.m_Group.Reset();
		    m_IdtUser.m_Group.m_Group_Num = pGInfo->usNum;

		    for (int i = 0; i < pGInfo->usNum; i++)
		    {
		        strcpy((char*)m_IdtUser.m_Group.m_Group[i].m_ucGNum, (char*)pGInfo->stGInfo[i].ucNum);
		        strcpy((char*)m_IdtUser.m_Group.m_Group[i].m_ucGName, (char*)pGInfo->stGInfo[i].ucName);
		    }

		    if(pocIdtAttr.current_group >= pGInfo->usNum)
		    {
			    pocIdtAttr.current_group = 0;
		    }

			#if 0
			do
			{
				if(pocIdtAttr.pocGetGroupListCb == NULL)
				{
					break;
				}


				if(m_IdtUser.m_Group.m_Group_Num < 1)
				{
					pocIdtAttr.pocGetGroupListCb(0, 0, NULL);
					break;
				}

				pocIdtAttr.pocGetGroupListCb(1, m_IdtUser.m_Group.m_Group_Num, m_IdtUser.m_Group.m_Group);
			}while(0);
			pocIdtAttr.pocGetGroupListCb = NULL;
			#endif

		    Func_GQueryU(0, pGInfo->stGInfo[pocIdtAttr.current_group].ucNum);
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

			lv_poc_build_new_group_t * new_group = (lv_poc_build_new_group_t *)ctx;
			Msg_GROUP_MEMBER_s * member = NULL;
			GROUP_MEMBER_s * gmember = NULL;
			static GData_s g_data = {0};
			memset(&g_data, 0, sizeof(GData_s));
			g_data.dwNum = new_group->num;

			for(int i = 0; i < new_group->num; i++)
			{
				member = (Msg_GROUP_MEMBER_s *)new_group->members[i];
				gmember = (GROUP_MEMBER_s *)&g_data.member[i];
				gmember->ucType = GROUP_MEMBERTYPE_USER;
				strcpy((char *)gmember->ucName, (const char *)member->ucName);
				strcpy((char *)gmember->ucNum, (const char *)member->ucNum);
			}

			IDT_GAdd(m_IdtUser.m_Group.m_Group_Num, &g_data);
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

			LvPocGuiIdtCom_Group_Operator_t *grop = (LvPocGuiIdtCom_Group_Operator_t *)ctx;

			if(grop->wRes != CAUSE_ZERO)
			{
				pocIdtAttr.pocBuildGroupCb(0);
				pocIdtAttr.pocBuildGroupCb = NULL;
				break;
			}
			pocIdtAttr.pocBuildGroupCb(1);
			pocIdtAttr.pocBuildGroupCb = NULL;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			pocIdtAttr.pocBuildGroupCb = (poc_build_group_cb)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND:
		{
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

			if(ctx == 0)
			{
				pocIdtAttr.query_group = pocIdtAttr.current_group;
			}
			else
			{
				CGroup * group_info = (CGroup *)ctx;
				for(DWORD i = 0; i < m_IdtUser.m_Group.m_Group_Num; i++)
				{
					if(0 == strcmp((const char *)group_info->m_ucGNum, (const char *)m_IdtUser.m_Group.m_Group[i].m_ucGNum))
					{
						pocIdtAttr.query_group = i;
						break;
					}
				}
			}

			if(m_IdtUser.m_Group.m_Group_Num < 1
				||pocIdtAttr.query_group >=  m_IdtUser.m_Group.m_Group_Num
				|| 0 == m_IdtUser.m_Group.m_Group[pocIdtAttr.query_group].m_ucGNum[0])
			{
				pocIdtAttr.pocGetMemberListCb(0, 0, NULL);
				pocIdtAttr.pocGetMemberListCb = NULL;
				pocIdtAttr.query_group = 0;
				break;
			}

	        // 查询组成员
	        Func_GQueryU(pocIdtAttr.query_group, m_IdtUser.m_Group.m_Group[pocIdtAttr.query_group].m_ucGNum);

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
		{
			if(pocIdtAttr.pocGetMemberListCb == NULL)
			{
				break;
			}
			pocIdtAttr.pocGetMemberListCb(1, pocIdtAttr.pPocMemberList->dwNum, pocIdtAttr.pPocMemberList);
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

			CGroup * group_info = (CGroup *)ctx;
			DWORD index = 0;
			for(index = 0; index < m_IdtUser.m_Group.m_Group_Num; index++)
			{
				if(0 == strcmp((const char *)group_info->m_ucGNum, (const char *)m_IdtUser.m_Group.m_Group[index].m_ucGNum)) break;
			}

			if(m_IdtUser.m_Group.m_Group_Num < 1
				|| index >=  m_IdtUser.m_Group.m_Group_Num
				|| 0 == m_IdtUser.m_Group.m_Group[index].m_ucGNum[0])
			{
				pocIdtAttr.pocSetCurrentGroupCb(0);
			}
			else if(index == pocIdtAttr.current_group)
			{
				pocIdtAttr.pocSetCurrentGroupCb(2);
			}
			else
			{
				pocIdtAttr.current_group = index;
				pocIdtAttr.pocSetCurrentGroupCb(1);
			}

			pocIdtAttr.pocSetCurrentGroupCb = NULL;
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
			Msg_GROUP_MEMBER_s * member = (Msg_GROUP_MEMBER_s *)ctx;
			IDT_UQuery(0, member->ucNum);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			LvPocGuiIdtCom_User_Operator_t * UOpt = (LvPocGuiIdtCom_User_Operator_t *)ctx;

			if(pocIdtAttr.pocGetMemberStatusCb != NULL)
			{
				if(UOpt->haveUser != true || UOpt->wRes != CAUSE_ZERO)
				{
					lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_STATUS_REP, NULL);
				}
				else
				{
					lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_STATUS_REP, (void *)(UOpt->pUser.ucStatus > UT_STATUS_OFFLINE));
				}
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
			pocAudioPlayerStop(pocIdtAttr.player);
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
			pocAudioPlayerStart(pocIdtAttr.player);
			m_IdtUser.m_status = USER_OPRATOR_LISTENNING;
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
			pocAudioRecorderStop(pocIdtAttr.recorder);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND:
		{
			if(m_IdtUser.m_status < UT_STATUS_ONLINE)
			{
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_REP, GUIIDTCOM_RELEASE_MIC);
				break;
			}
			pocAudioRecorderStart(pocIdtAttr.recorder);
			m_IdtUser.m_status = USER_OPRATOR_SPEAKING;
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

		    lv_poc_member_call_config_t *member_call_config = (lv_poc_member_call_config_t *)ctx;

		    Msg_GROUP_MEMBER_s *member_call_obj = (Msg_GROUP_MEMBER_s *)member_call_config->members;
		    Msg_GROUP_MEMBER_s *member = member_call_obj;

		    if(member_call_config->func == NULL)
		    {
			    break;
		    }
		    pocIdtAttr.pocMemberCallCb = member_call_config->func;

		    if(member_call_config->enable && member_call_obj == NULL)
		    {
				pocIdtAttr.pocMemberCallCb(false, true);
				pocIdtAttr.pocMemberCallCb = NULL;
				break;
		    }

			do
			{
			    if (member_call_config->enable == true)
			    {
				    if(pocIdtAttr.member_call_dir < 1)
				    {
					    unsigned long k = 0;
						for(k = 0; k < pocIdtAttr.pPocMemberList->dwNum; k++)
						{
							member = &pocIdtAttr.pPocMemberList->member[k];
							if(lv_poc_check_member_equation(member_call_obj->ucName, member->ucName, member_call_obj, member, NULL))
							{
								break;
							}
						}

						if(k >= pocIdtAttr.pPocMemberList->dwNum)
						{
							pocIdtAttr.pocMemberCallCb(false, true);
							pocIdtAttr.pocMemberCallCb = NULL;
							break;
						}
					}

					strcpy((char *)pocIdtAttr.member_call_obj.ucName, (const char *)member->ucName);
					strcpy((char *)pocIdtAttr.member_call_obj.ucNum, (const char *)member->ucNum);
					pocIdtAttr.member_call_obj.ucStatus = UT_STATUS_ONLINE;

					if(pocIdtAttr.member_call_dir < 1)
					{
						if(m_IdtUser.m_iCallId != -1)
						{
							IDT_CallRel(m_IdtUser.m_iCallId, NULL, CAUSE_ZERO);
							osiThreadSleep(500);
							m_IdtUser.m_iCallId = -1;
						}

						memset(&pocIdtAttr.attr, 0, sizeof(MEDIAATTR_s));
						pocIdtAttr.attr.ucAudioSend = 1;
						pocIdtAttr.is_member_call = true;
						char *user_mark = (char *)GUIIDTCOM_MEMBER_CALL_MARK;
						m_IdtUser.m_iCallId = IDT_CallMakeOut((char*)pocIdtAttr.member_call_obj.ucNum,
																	SRV_TYPE_SIMP_CALL,
																	&pocIdtAttr.attr,
																	NULL,
																	NULL,
																	1,
																	0,
																	1,
																	user_mark);
					}
					else
					{
						pocIdtAttr.is_member_call = true;
						pocIdtAttr.pocMemberCallCb(true, true);
						pocIdtAttr.pocMemberCallCb = NULL;
					}
			    }
			    else
			    {
			        bool is_member_call = pocIdtAttr.is_member_call;
					pocIdtAttr.is_member_call = false;
					pocIdtAttr.member_call_dir = 0;
					if(is_member_call && m_IdtUser.m_iCallId != -1)
					{
						IDT_CallRel(m_IdtUser.m_iCallId, NULL, CAUSE_ZERO);
						osiThreadSleep(500);
						m_IdtUser.m_iCallId = -1;
					}
					pocIdtAttr.pocMemberCallCb(false, false);
					pocIdtAttr.pocMemberCallCb = NULL;
			    }
		    }while(0);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_REP:
		{
			if(ctx == 0)   //对方或者服务器释放或者拒绝了单呼通话
			{
				pocIdtAttr.is_member_call = false;
				pocIdtAttr.member_call_dir = 0;
				lv_poc_activity_func_cb_set.member_call_close();
			}
			else if(ctx == 1)  //对方同意了单呼通话
			{
			    if(pocIdtAttr.pocMemberCallCb != NULL)
			    {
				    pocIdtAttr.pocMemberCallCb(true, true);
				    pocIdtAttr.pocMemberCallCb = NULL;
			    }
			}
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
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)"停止聆听", (const uint8_t *)"");
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, true);
			//可以播放按键音
			lvPocGuiIdtCom_get_listen_status(LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP:
		{
			char speaker_name[100];
			char *speaker_group = (char *)pocIdtAttr.speaker_group.m_ucGName;
			memset(speaker_name, 0, sizeof(char) * 100);
			strcpy(speaker_name, (const char *)pocIdtAttr.speaker.ucName);
			strcat(speaker_name, (const char *)"正在讲话");
			//禁止播放按键音
			lvPocGuiIdtCom_get_listen_status(LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP);
			//lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "正在聆听", NULL);
			//lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"正在聆听", NULL);
			if(!pocIdtAttr.is_member_call)
			{
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, speaker_name, speaker_group);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)speaker_group);
			}
			else
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)"");
			}
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
		case LVPOCGUIIDTCOM_SIGNAL_GU_STATUS_REP:
		{
			if(ctx == 0)
			{
				break;
			}
			GU_STATUSGINFO_s *pStatus = (GU_STATUSGINFO_s *)ctx;

			Msg_GROUP_MEMBER_s *member = NULL;
			CGroup *group = NULL;

		    for (int i = 0; i < pStatus->usNum; i++)
		    {
			    if (pStatus->stStatus[i].ucType == 1)
				{
					for(unsigned long k = 0; k < pocIdtAttr.pPocMemberList->dwNum; k++)
					{
						if(0 == strcmp((const char *)pStatus->stStatus[i].ucNum, (const char *)pocIdtAttr.pPocMemberList->member[k].ucNum))
						{
							member = &pocIdtAttr.pPocMemberList->member[k];
							break;
						}
					}

					if(member != NULL)
					{
						lv_poc_activity_func_cb_set.member_list.set_state(NULL, (const char *)member->ucName, (void *)member, pStatus->stStatus[i].Status.ucStatus);
						member = NULL;
					}
				}
				else if (pStatus->stStatus[i].ucType == 2)
				{
					for(unsigned long k = 0; k < m_IdtUser.m_Group.m_Group_Num; k++)
					{
						if(0 == strcmp((const char *)pStatus->stStatus[i].ucNum, (const char *)m_IdtUser.m_Group.m_Group[k].m_ucGNum))
						{
							group = &m_IdtUser.m_Group.m_Group[k];
							break;
						}
					}

					if(group != NULL)
					{
						bool isExist = lv_poc_activity_func_cb_set.group_list.exists(NULL, (const char *)group->m_ucGName, (void *)group);
						if(!pStatus->stStatus[i].Status.ucStatus && isExist)
						{
							lv_poc_activity_func_cb_set.group_list.remove(NULL, (const char *)group->m_ucGName, (void *)group);
							lv_poc_activity_func_cb_set.group_list.refresh(NULL);
						}
						else if (pStatus->stStatus[i].Status.ucStatus && !isExist)
						{
							lv_poc_activity_func_cb_set.group_list.add(NULL, (const char *)group->m_ucGName, (void *)group);
							lv_poc_activity_func_cb_set.group_list.refresh(NULL);
						}
						group = NULL;
					}
				}
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
			LvPocGuiIdtCom_Group_Operator_t *grop = (LvPocGuiIdtCom_Group_Operator_t *)ctx;

			if (OPT_G_QUERYUSER == grop->dwOptCode)
			{
				if (CAUSE_ZERO != grop->wRes)
					return;

				if(grop->dwSn == pocIdtAttr.query_group
					&& pocIdtAttr.pPocMemberList != NULL)
				{
					pocIdtAttr.pPocMemberList->dwNum = grop->pGroup.dwNum;
					for(unsigned long i = 0; i < pocIdtAttr.pPocMemberList->dwNum; i++)
					{
						strcpy((char *)pocIdtAttr.pPocMemberList->member[i].ucName, (char *)grop->pGroup.member[i].ucName);
						strcpy((char *)pocIdtAttr.pPocMemberList->member[i].ucNum, (char *)grop->pGroup.member[i].ucNum);
						pocIdtAttr.pPocMemberList->member[i].ucStatus = grop->pGroup.member[i].ucStatus;//用户状态
					}
					lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_REP, NULL);
				}
			}
			else if (OPT_G_ADD == grop->dwOptCode)
			{
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_REP, grop);
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

static void pocGuiIdtComTaskEntry(void *argument)
{

	osiEvent_t event;

    for(int i = 0; i < 10; i++)
    {
	    osiThreadSleep(1000);
    }

    IDT_Entry(NULL);
    pocIdtAttr.isReady = true;

    while(1)
    {
    	if(!osiEventTryWait(pocIdtAttr.thread , &event, 100))
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
			case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_REP:
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
}

static void lvPocGuiIdtCom_send_data_callback(uint8_t * data, uint32_t length)
{
    if (pocIdtAttr.recorder == 0 || m_IdtUser.m_iCallId == -1 || data == NULL || length < 1)
    {
	    return;
    }
    if(m_IdtUser.m_status != USER_OPRATOR_SPEAKING)
    {
		//lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND, NULL);
	    return;
    }
    IDT_CallSendAuidoData(m_IdtUser.m_iCallId,
						    0,
						    0,
						    data,
						    length,
						    0);
    m_IdtUser.m_iTxCount = m_IdtUser.m_iTxCount + 1;
}

extern "C" void lvPocGuiIdtCom_Init(void)
{
	memset(&pocIdtAttr, 0, sizeof(PocGuiIIdtComAttr_t));
	pocIdtAttr.pPocMemberList = (Msg_GData_s *)malloc(sizeof(Msg_GData_s));
	pocIdtAttr.delay_close_listen_timer = osiTimerCreate(NULL, LvGuiIdtCom_delay_close_listen_timer_cb, NULL);
	pocGuiIdtComStart();
}

extern "C" bool lvPocGuiIdtCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx)
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

	osiExitCritical(critical);

	return osiEventSend(pocIdtAttr.thread, &event);
}

extern "C" void lvPocGuiIdtCom_log(void)
{
	lvPocGuiIdtCom_Init();
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

/*
      name : lvPocGuiIdtCom_get_listen_status
     param :
     			none
  describe : 获取是否在接听状态下
      date : 2020-06-08
*/
static void lvPocGuiIdtCom_get_listen_status(LvPocGuiIdtCom_SignalType_t SignalType_t)
{
	switch(SignalType_t)
	{
		case LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP:
		{
			lv_listen_sta.listen_status = true;
			break;
		}
		case LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP:
		{
			lv_listen_sta.listen_status = false;
			break;
		}
		default:
		break;
	}
}

/*
      name : lvPocGuiIdtCom_listen_status
     param :
     			none
  describe : 返回接听状态
      date : 2020-06-08
*/
extern "C" int lvPocGuiIdtCom_listen_status(void)
{
	return lv_listen_sta.listen_status;
}


#endif

