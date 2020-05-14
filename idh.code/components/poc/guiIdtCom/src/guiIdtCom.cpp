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

#define GUIIDTCOM_DEBUG (0)

//#if 0
#ifdef CONFIG_POC_SUPPORT
#define FREERTOS
#define T_TINY_MODE

#include "IDT.h"

#define APPTEST_THREAD_PRIORITY (OSI_PRIORITY_NORMAL)
#define APPTEST_STACK_SIZE (8192 * 4)
#define APPTEST_EVENT_QUEUE_SIZE (64)

static GData_s *pGroup=NULL;//成员列表结构体
static uint32_t pGroup_member_number;//成员数量

char *GetCauseStr(USHORT usCause);
char *GetOamOptStr(DWORD dwOpt);
char *GetSrvTypeStr(SRV_TYPE_e SrvType);
static void lvPocGuiIdtCom_send_data_callback(uint8_t * data, uint32_t length);


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

//组信息
class CGroup
{
public:
    CGroup()
    {
        Reset();
    }
    ~CGroup()
    {
    }
    int Reset()
    {
        m_ucGNum[0]     = 0;
        m_ucGName[0]    = 0;
        return 0;
    }

public:
    UCHAR   m_ucGNum[32];     //组号码
    UCHAR   m_ucGName[64];    //组名字
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
        int i;
        for (i = 0; i < USER_MAX_GROUP; i++)
        {
            m_Group[i].Reset();
        }
        return 0;
    }

public:
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
} PocGuiIIdtComAttr_t;
CIdtUser m_IdtUser;
static PocGuiIIdtComAttr_t pocIdtAttr = {0};






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
    if (UT_STATUS_ONLINE == status)
    {
        IDT_StatusSubs((char*)"###", GU_STATUSSUBS_BASIC);
        m_IdtUser.m_status = 0;
    }
    else
    {
	    m_IdtUser.m_status = 0xff;
    }

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

    m_IdtUser.m_Group.Reset();
    for (int i = 0; i < pGInfo->usNum; i++)
    {
        IDT_TRACE("%s\n", pGInfo->stGInfo[i].ucNum);
        strcpy((char*)m_IdtUser.m_Group.m_Group[i].m_ucGNum, (char*)pGInfo->stGInfo[i].ucNum);
        strcpy((char*)m_IdtUser.m_Group.m_Group[i].m_ucGName, (char*)pGInfo->stGInfo[i].ucName);
    }

    // 启动查询所有组成员
    Func_GQueryU(0, pGInfo->stGInfo[0].ucNum);
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
    for (int i = 0; i < pStatus->usNum; i++)
    {
        IDT_TRACE("%d:%s--%d", pStatus->stStatus[i].ucType, pStatus->stStatus[i].ucNum, pStatus->stStatus[i].Status.ucStatus);
    }
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
	if(m_IdtUser.m_status == 3 || m_IdtUser.m_status == 4)
	{
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
	}
    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND, NULL);
    //进入通话界面
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
    m_IdtUser.m_iCallId = ID;
    m_IdtUser.m_iRxCount = 0;
    m_IdtUser.m_iTxCount = 0;
    OSI_LOGI(0, "[gic] start group\n");

    switch (SrvType)
    {
    case SRV_TYPE_BASIC_CALL://单呼
        //振铃
        break;

    case SRV_TYPE_CONF://组呼
        //直接接通
        {
	        OSI_LOGI(0, "[gic] start group call\n");
            memset(&pocIdtAttr.attr, 0, sizeof(MEDIAATTR_s));
            pocIdtAttr.attr.ucAudioRecv = 1;
            IDT_CallAnswer(m_IdtUser.m_iCallId, &pocIdtAttr.attr, NULL);
            m_IdtUser.m_status = 3;
#if CONFIG_POC_AUDIO_DATA_IDT_BUFF_MAX_SIZE > 3
            pocIdtAttr.pocAudioData_read_index = 0;
            pocIdtAttr.pocAudioData_write_index = 0;
#endif
            lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
        }
        break;

    default:
        IDT_CallRel(ID, NULL, CAUSE_SRV_NOTSUPPORT);
        m_IdtUser.m_iCallId = -1;
        m_IdtUser.m_iRxCount = 0;
        m_IdtUser.m_iTxCount = 0;
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
    m_IdtUser.m_status = 0;

#if CONFIG_POC_AUDIO_DATA_IDT_BUFF_MAX_SIZE > 3
    pocIdtAttr.pocAudioData_read_index = 0;
    pocIdtAttr.pocAudioData_write_index = 0;
#endif

    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND, NULL);
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
	if(m_IdtUser.m_status == 4 || m_IdtUser.m_status == 3)
	{
	    m_IdtUser.m_iRxCount = m_IdtUser.m_iRxCount + 1;
	    struct timeval tm;
	    extern int gettimeofday(struct timeval *tv, void *tz);
	    gettimeofday(&tm, NULL);
	    OSI_LOGI(0, "[gic]callback_IDT_CallRecvAudioData: m_iRxCount=%d, time=%d", m_IdtUser.m_iRxCount, tm.tv_sec*1000 + tm.tv_usec/1000);
		OSI_LOGI(0, "[gic] write data player\n");
#if CONFIG_POC_AUDIO_DATA_IDT_BUFF_MAX_SIZE < 4
		pocAudioPlayerWriteData(pocIdtAttr.player, (const uint8_t *)pucBuf, iLen);
#else
		memcpy((void *)(pocIdtAttr.pocAudioData[pocIdtAttr.pocAudioData_write_index].data), (const void *)pucBuf, iLen);
		pocIdtAttr.pocAudioData[pocIdtAttr.pocAudioData_write_index].length = iLen;
		pocIdtAttr.pocAudioData_write_index = (pocIdtAttr.pocAudioData_write_index + 1) % CONFIG_POC_AUDIO_DATA_IDT_BUFF_MAX_SIZE;
		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_WRITE_DATA_IND, NULL);
#endif
		if(m_IdtUser.m_iRxCount == 10)
		{
			lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND, NULL);
			lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_START_PLAY_IND, NULL);
			lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LISTEN_START_REP, NULL);
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
    //显示讲话人号码/名字,UTF-8编码
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
    if (OPT_G_QUERYUSER == dwOptCode)
    {
        if (CAUSE_ZERO != wRes)
            return;

        int i;
        for (i = 0; i < (int)pGroup->dwNum; i++)
        {
            IDT_TRACE("    %s(%s): %d", pGroup->member[i].ucNum, pGroup->member[i].ucName, pGroup->member[i].ucStatus);
        }

        dwSn++;
        if (dwSn >= USER_MAX_GROUP)
            return;

        if (0 == m_IdtUser.m_Group.m_Group[dwSn].m_ucGNum[0])
            return;
        // 持续查询剩下的组成员
        Func_GQueryU(dwSn, m_IdtUser.m_Group.m_Group[dwSn].m_ucGNum);

		/*添加查询组成员是否完成 
		  通过消息发送触发调用
		  日期：5.13	
		*/
		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP,NULL);//发送消息已经完成获取成员列表
    }
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
    else if (0 == _stricmp("gquery", pcTxt))
    {
        Func_GQueryU(0, m_IdtUser.m_Group.m_Group[0].m_ucGNum);
    }

    return 0;
}

extern int g_iLog;

void IDT_Entry(void*)
{
    m_IdtUser.Reset();

    // 0关闭日志,1打开日志
    //g_iLog = 0;
    g_iLog = 0;

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

    IDT_Start(NULL, 1, (char*)"124.160.11.21", 10000, NULL, 0, (char*)"34012", (char*)"34012", 1, &CallBack, 0, 20000, 0);
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

    for(; ; )
    {
    	if(!osiEventTryWait(pocIdtAttr.thread , &event, 100))
		{			
			continue;
		}

		if(event.id != 100)
		{
			continue;
		}

		switch(event.param1)
		{
			case LVPOCGUIIDTCOM_SIGNAL_WRITE_DATA_IND:
			{
#if CONFIG_POC_AUDIO_DATA_IDT_BUFF_MAX_SIZE > 3
				if(!(m_IdtUser.m_status == 3 || m_IdtUser.m_status == 4)) break;
				pocAudioPlayerWriteData(pocIdtAttr.player, (const uint8_t *)pocIdtAttr.pocAudioData[pocIdtAttr.pocAudioData_read_index].data, pocIdtAttr.pocAudioData[pocIdtAttr.pocAudioData_read_index].length);
				pocIdtAttr.pocAudioData_read_index = (pocIdtAttr.pocAudioData_read_index + 1) % CONFIG_POC_AUDIO_DATA_IDT_BUFF_MAX_SIZE;
				if(!(m_IdtUser.m_status == 3 || m_IdtUser.m_status == 4))
				{
					pocIdtAttr.pocAudioData_read_index = 0;
					pocIdtAttr.pocAudioData_write_index = 0;
				}
#endif
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND:
			{
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_LOGIN_REP:
			{
				OSI_LOGI(0, "[gic] LOGIN_REP 1\n");
				if(pocIdtAttr.player == 0)
				{
					pocIdtAttr.player = pocAudioPlayerCreate(320000);
				}
				OSI_LOGI(0, "[gic] LOGIN_REP 2 player <- %d\n", pocIdtAttr.player);

				if(pocIdtAttr.recorder == 0)
				{
					pocIdtAttr.recorder = pocAudioRecorderCreate(48000, 320, 20, lvPocGuiIdtCom_send_data_callback);
				}
				OSI_LOGI(0, "[gic] LOGIN_REP 3 recorder  <- %d\n", pocIdtAttr.recorder);

				if(pocIdtAttr.player == 0 || pocIdtAttr.recorder == 0)
				{
					pocIdtAttr.isReady = false;
					osiThreadExit();
				}
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_IND:
			{
				OSI_LOGI(0, "[gic] start speak\n");
				memset(&pocIdtAttr.attr, 0, sizeof(MEDIAATTR_s));
				pocIdtAttr.attr.ucAudioSend = 1;
				m_IdtUser.m_iCallId = IDT_CallMakeOut((char*)m_IdtUser.m_Group.m_Group[0].m_ucGNum,
															SRV_TYPE_CONF,
															&pocIdtAttr.attr,
															NULL,
															NULL,
															1,
															0,
															1,
															NULL);
				m_IdtUser.m_status = 1;
				m_IdtUser.m_iRxCount = 0;
				m_IdtUser.m_iTxCount = 0;
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP:
			{
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_IND:
			{
				OSI_LOGI(0, "[gic] stop speak\n");
				IDT_CallRel(m_IdtUser.m_iCallId, NULL, CAUSE_ZERO);
		        m_IdtUser.m_iCallId = -1;
		        lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND, NULL);
		        m_IdtUser.m_status = 0;
				m_IdtUser.m_iRxCount = 0;
				m_IdtUser.m_iTxCount = 0;
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP:
			{
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_LISTEN_START_REP:
			{
				OSI_LOGI(0, "[gic] start listen\n");
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP:
			{
				OSI_LOGI(0, "[gic] stop listen\n");
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND:
			{
				pocAudioPlayerStop(pocIdtAttr.player);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_START_PLAY_IND:
			{
				audevStopPlay();
				audevStopRecord();
				OSI_LOGI(0, "[gic][play][ctr] 2");
				pocAudioPlayerStart(pocIdtAttr.player);
				m_IdtUser.m_status = 4;
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND:
			{
				pocAudioRecorderStop(pocIdtAttr.recorder);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND:
			{
				audevStopPlay();
				audevStopRecord();
				pocAudioRecorderStart(pocIdtAttr.recorder);
				m_IdtUser.m_status = 2;
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_IND://发送获取成员列表
			{
				OSI_LOGI(0, "[lml] start obtain member\n");
				if(event.param2 == false)
				{
					break;
				}
				
				Msg_GData_s * data = (Msg_GData_s *)event.param2;
				//复制数据
				data->dwNum=pGroup->dwNum;
				for(pGroup_member_number=0;pGroup_member_number<data->dwNum;pGroup_member_number++)
				{
					strcpy((char *)data->member[pGroup_member_number].ucName,(char *)pGroup->member[pGroup_member_number].ucName);
					strcpy((char *)data->member[pGroup_member_number].ucNum,(char *)pGroup->member[pGroup_member_number].ucNum);
					data->member[pGroup_member_number].ucStatus=pGroup->member[pGroup_member_number].ucStatus;//用户状态
				}	
			
				break;
			}
			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP://成功获取成员列表
			{
				OSI_LOGI(0, "[lml] successfully obtained the member list\n");
				lv_poc_get_member_list_from_msg(1);
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
    OSI_LOGE(0, "----------------------------------------------------");
    pocIdtAttr.thread = osiThreadCreate(
		"pocGuiIdtCom", pocGuiIdtComTaskEntry, NULL,
		APPTEST_THREAD_PRIORITY, APPTEST_STACK_SIZE,
		APPTEST_EVENT_QUEUE_SIZE);
}

static void lvPocGuiIdtCom_send_data_callback(uint8_t * data, uint32_t length)
{
	OSI_LOGI(0, "[gic] checkout send data conditions\n");
	OSI_LOGI(0, "[gic] data <- 0x%p  length <- %d \n", data, length);
    if (pocIdtAttr.recorder == 0 || m_IdtUser.m_iCallId == -1 || data == NULL || length < 1)
    {
	    return;
    }
    if(m_IdtUser.m_status != 2)
    {
		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND, NULL);
	    return;
    }
	OSI_LOGI(0, "[gic] send data to server\n");
	#if 1
    IDT_CallSendAuidoData(m_IdtUser.m_iCallId,
						    0,
						    0,
						    data,
						    length,
						    0);
	#endif
    m_IdtUser.m_iTxCount = m_IdtUser.m_iTxCount + 1;
}

extern "C" void lvPocGuiIdtCom_Init(void)
{
	memset(&pocIdtAttr, 0, sizeof(PocGuiIIdtComAttr_t));
	pocGuiIdtComStart();
}

extern "C" bool lvPocGuiIdtCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx)
{
    if (pocIdtAttr.thread == NULL || pocIdtAttr.isReady == false)
    {
	    return false;
    }

	osiEvent_t event = {0};
	memset(&event, 0, sizeof(osiEvent_t));
	event.id = 100;
	event.param1 = signal;
	event.param2 = (uint32_t)ctx;
	return osiEventSend(pocIdtAttr.thread, &event);
}

extern "C" void lvPocGuiIdtCom_log(void)
{
	lvPocGuiIdtCom_Init();
}

#endif

