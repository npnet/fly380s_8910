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
#include "uart3_gps.h"
#include "drv_charger_monitor.h"
#include <sys/time.h>
#include "lv_gui_main.h"
#include "cJSON.h"
#include "poc_audio_pipe.h"

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
#define GUIIDTCOM_IDTWINDOWS_NOTE    		0
#define GUIIDTCOM_LOCK_FUNC                 1
#define POC_AUDIO_MODE_PIPE                 1

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
static void LvGuiIdtCom_start_speak_voice_timer_cb(void *ctx);
static void prvPocGuiIdtTaskHandleCallFailed(uint32_t id, uint32_t ctx, uint32_t cause_str);

/*************************************************************************************/
#define     CPTM_MM_REG         0x10        // 注册请求
#define     CPTM_MM_OFFLINE     0x11        // 离线扫描
#define     CPTM_MM_PERIOD      0x12        // 周期注册
#define     CPTM_MM_MODIFY      0x13        // 修改用户属性
#define     CPTM_MM_NAT         0x14        // NAT

#define     CPTM_CC_SETUPACK    0x20        // 发送SETUP,等待SETUP_ACK
#define     CPTM_CC_CONN        0x21        // 发送SETUP,等待CONN
#define     CPTM_CC_ANSWER      0x22        // 发送CallIn给用户,等用户应答
#define     CPTM_CC_CONNACK     0x23        // 发送CONN给对端,等待CONNACK
#define     CPTM_CC_HB          0x24        // 通话后的心跳定时器
#define     CPTM_CC_PRESS       0x25        // 按键定时器

char* LvPocGetIdtTmStr(WORD wTm)
{
    static char cBuf[32];

    switch (wTm)
    {
    case CPTM_MM_REG:
        return (char*)"CPTM_MM_REG";
    case CPTM_MM_OFFLINE:
        return (char*)"CPTM_MM_OFFLINE";
    case CPTM_MM_PERIOD:
        return (char*)"CPTM_MM_PERIOD";
    case CPTM_MM_MODIFY:
        return (char*)"CPTM_MM_MODIFY";
    case CPTM_MM_NAT:
        return (char*)"CPTM_MM_NAT";
    case CPTM_CC_SETUPACK:
        return (char*)"CPTM_CC_SETUPACK";
    case CPTM_CC_CONN:
        return (char*)"CPTM_CC_CONN";
    case CPTM_CC_ANSWER:
        return (char*)"CPTM_CC_ANSWER";
    case CPTM_CC_CONNACK:
        return (char*)"CPTM_CC_CONNACK";
    case CPTM_CC_HB:
        return (char*)"CPTM_CC_HB";
    case CPTM_CC_PRESS:
        return (char*)"CPTM_CC_PRESS";
    default:
        snprintf(cBuf, sizeof(cBuf), "Tm=%d", wTm);
        return cBuf;
    }
}

enum
{
    CCTM_HB_CSA_P               = 1,
    CCTM_HB_CSA_MG              = 2,
    CCTM_HB_BCSM                = 3,

    //CSA消息
    CCTM_WAIT_MGBINGRSP         = 4,
    CCTM_WAIT_MGIVRRSP          = 5,
    CCTM_WAIT_MGCONNRSP         = 6,
    CCTM_WAIT_MGDISCRSP         = 7,
    CCTM_WAIT_MMACCRSP          = 8,
    CCTM_WAIT_PSETUPACK         = 9,
    CCTM_WAIT_PCONNACK          = 10,
    CCTM_WAIT_MY_MGMODMYRSP     = 11,
    CCTM_WAIT_MY_PMODRSP        = 12,
    CCTM_WAIT_MY_MGMODPEERRSP   = 13,
    CCTM_WAIT_P_MGMODRSP        = 14,

    //BCSM消息
    //消息定时器
    CCTM_WAIT_SETUPACK          = 15,
    CCTM_WAIT_CONNACK           = 16,
    //收号定时器
    CCTM_RECVNUM_START          = 17,
    CCTM_RECVNUM_INTER          = 18,
    CCTM_RECVNUM_TOTAL          = 19,
    //O端
    CCTM_O_PIC_AuthorizeOriginationAttempt = 20,        //等待MM的接入响应,CSA
    CCTM_O_PIC_CollectInfomation= 21,
    CCTM_O_PIC_AnalyzeInfomation= 22,
    CCTM_O_PIC_SelectRoute      = 23,                   //等待MM的路由响应 BCSM
    CCTM_O_PIC_AuthorizeCallSetup= 24,
    CCTM_O_PIC_SendCall         = 25,
    CCTM_O_PIC_Alerting         = 26,
    CCTM_O_PIC_Active           = 27,
    CCTM_O_PIC_Suspended        = 28,
    CCTM_O_PIC_Exception        = 29,
    //T端
    CCTM_T_PIC_AuthorizeTerminationAttempt = 30,
    CCTM_T_PIC_SelectFacility   = 31,
    CCTM_T_PIC_PresentCall      = 32,
    CCTM_T_PIC_Alerting         = 33,
    CCTM_T_PIC_Active           = 34,
    CCTM_T_PIC_Suspended        = 35,
    CCTM_T_PIC_Exception        = 36,

    CCTM_RE_CALL                = 37,
    CCTM_CS_MICGET              = 38,
    CCTM_CS_MICREL              = 39,

    CCTM_RECV_NUM               = 40,
    CCTM_RECV_NUM_TOTAL         = 41,

    CCTM_SCAN_DEV               = 42,                   //扫描设备
    CCTM_SCAN_TER               = 43,                   //扫描终端

    CCTM_WAIT_SETUP             = 44,


    MSG_TM_MAX,
};

char* LvPocGetMcTmStr(WORD wTm)
{
    static char cBuf[64];

    switch (wTm)
    {
    case CCTM_HB_CSA_P:
        return (char*)"CCTM_HB_CSA_P";
    case CCTM_HB_CSA_MG:
        return (char*)"CCTM_HB_CSA_MG";
    case CCTM_HB_BCSM:
        return (char*)"CCTM_HB_BCSM";

    //CSA消息
    case CCTM_WAIT_MGBINGRSP:
        return (char*)"CCTM_WAIT_MGBINGRSP";
    case CCTM_WAIT_MGIVRRSP:
        return (char*)"CCTM_WAIT_MGIVRRSP";
    case CCTM_WAIT_MGCONNRSP:
        return (char*)"CCTM_WAIT_MGCONNRSP";
    case CCTM_WAIT_MGDISCRSP:
        return (char*)"CCTM_WAIT_MGDISCRSP";
    case CCTM_WAIT_MMACCRSP:
        return (char*)"CCTM_WAIT_MMACCRSP";
    case CCTM_WAIT_PSETUPACK:
        return (char*)"CCTM_WAIT_PSETUPACK";
    case CCTM_WAIT_PCONNACK:
        return (char*)"CCTM_WAIT_PCONNACK";
    case CCTM_WAIT_MY_MGMODMYRSP:
        return (char*)"CCTM_WAIT_MY_MGMODMYRSP";
    case CCTM_WAIT_MY_PMODRSP:
        return (char*)"CCTM_WAIT_MY_PMODRSP";
    case CCTM_WAIT_MY_MGMODPEERRSP:
        return (char*)"CCTM_WAIT_MY_MGMODPEERRSP";
    case CCTM_WAIT_P_MGMODRSP:
        return (char*)"CCTM_WAIT_P_MGMODRSP";

    //BCSM消息
    //消息定时器
    case CCTM_WAIT_SETUPACK:
        return (char*)"CCTM_WAIT_SETUPACK";
    case CCTM_WAIT_CONNACK:
        return (char*)"CCTM_WAIT_CONNACK";
    //收号定时器
    case CCTM_RECVNUM_START:
        return (char*)"CCTM_RECVNUM_START";
    case CCTM_RECVNUM_INTER:
        return (char*)"CCTM_RECVNUM_INTER";
    case CCTM_RECVNUM_TOTAL:
        return (char*)"CCTM_RECVNUM_TOTAL";
    //O端
    case CCTM_O_PIC_AuthorizeOriginationAttempt:
        return (char*)"CCTM_O_PIC_AuthorizeOriginationAttempt";
    case CCTM_O_PIC_CollectInfomation:
        return (char*)"CCTM_O_PIC_CollectInfomation";
    case CCTM_O_PIC_AnalyzeInfomation:
        return (char*)"CCTM_O_PIC_AnalyzeInfomation";
    case CCTM_O_PIC_SelectRoute:
        return (char*)"CCTM_O_PIC_SelectRoute";
    case CCTM_O_PIC_AuthorizeCallSetup:
        return (char*)"CCTM_O_PIC_AuthorizeCallSetup";
    case CCTM_O_PIC_SendCall:
        return (char*)"CCTM_O_PIC_SendCall";
    case CCTM_O_PIC_Alerting:
        return (char*)"CCTM_O_PIC_Alerting";
    case CCTM_O_PIC_Active:
        return (char*)"CCTM_O_PIC_Active";
    case CCTM_O_PIC_Suspended:
        return (char*)"CCTM_O_PIC_Suspended";
    case CCTM_O_PIC_Exception:
        return (char*)"CCTM_O_PIC_Exception";
    //T端
    case CCTM_T_PIC_AuthorizeTerminationAttempt:
        return (char*)"CCTM_T_PIC_AuthorizeTerminationAttempt";
    case CCTM_T_PIC_SelectFacility:
        return (char*)"CCTM_T_PIC_SelectFacility";
    case CCTM_T_PIC_PresentCall:
        return (char*)"CCTM_T_PIC_PresentCall";
    case CCTM_T_PIC_Alerting:
        return (char*)"CCTM_T_PIC_Alerting";
    case CCTM_T_PIC_Active:
        return (char*)"CCTM_T_PIC_Active";
    case CCTM_T_PIC_Suspended:
        return (char*)"CCTM_T_PIC_Suspended";
    case CCTM_T_PIC_Exception:
        return (char*)"CCTM_T_PIC_Exception";

    case CCTM_RE_CALL:
        return (char*)"CCTM_RE_CALL";
    case CCTM_CS_MICGET:
        return (char*)"CCTM_CS_MICGET";
    case CCTM_CS_MICREL:
        return (char*)"CCTM_CS_MICREL";

    case CCTM_RECV_NUM:
        return (char*)"CCTM_RECV_NUM";
    case CCTM_RECV_NUM_TOTAL:
        return (char*)"CCTM_RECV_NUM_TOTAL";

    case CCTM_SCAN_DEV:
        return (char*)"CCTM_SCAN_DEV";
    case CCTM_SCAN_TER:
        return (char*)"CCTM_SCAN_TER";

    case CCTM_WAIT_SETUP:
        return (char*)"CCTM_WAIT_SETUP";

    default:
        snprintf(cBuf, sizeof(cBuf), "Tm=%d", wTm);
        return cBuf;
    }
}

#define     CPTM_SCAN        0x01   //TCP扫描定时器
#define     CPTM_HB          0x02   //状态机与主控状态机的心跳定时器
#define     CPTM_NUM         0x03   //发送号码定时器

char* LvPocGetMgTmStr(WORD wTm)
{
    static char cBuf[32];

    switch (wTm)
    {
    case CPTM_SCAN:
        return (char*)"CPTM_SCAN";
    case CPTM_HB:
        return (char*)"CPTM_HB";
    case CPTM_NUM:
        return (char*)"CPTM_NUM";
    default:
        snprintf(cBuf, sizeof(cBuf), "Tm=%d", wTm);
        return cBuf;
    }
}

char *LvPocGetCauseStr(USHORT usCause)
{
    static char cBuf[32];

	if (14 == (usCause & 0xff))//定时器超时
	{
		WORD ucSrc = (usCause & 0xc000);
//		UCHAR ucH = (usCause & 0x3f00) >> 8;
		switch (ucSrc)
		{
		case 0x0000://IDT定时器超时
//			snprintf(cBuf, sizeof(cBuf), "定时器超时:IDT-%s", LvPocGetIdtTmStr(ucH));
//			break;
			return (char*)"申请话语权失败";
		case 0x4000://MC定时器超时
//			snprintf(cBuf, sizeof(cBuf), "定时器超时:MC-%s", LvPocGetMcTmStr(ucH));
//			break;
			return (char*)"申请话语权失败";
		case 0x8000://MG定时器超时
//			snprintf(cBuf, sizeof(cBuf), "定时器超时:MG-%s", LvPocGetMgTmStr(ucH));
//			break;
			return (char*)"申请话语权失败";
		default:
//			snprintf(cBuf, sizeof(cBuf), "定时器超时:%d-%d", ucSrc, ucH);
//			break;
			return (char*)"申请话语权失败";
		}
		return cBuf;
	}

    switch (usCause)
    {
    case CAUSE_ZERO:
        return (char*)"错误0";
    case CAUSE_UNASSIGNED_NUMBER:
        return (char*)"未分配号码";
    case CAUSE_NO_ROUTE_TO_DEST:
        return (char*)"无目的路由";
    case CAUSE_USER_BUSY:
        return (char*)"用户忙";
    case CAUSE_ALERT_NO_ANSWER:
        return (char*)"用户无应答(人不应答)";
    case CAUSE_CALL_REJECTED:
        return (char*)"呼叫被拒绝";
    case CAUSE_ROUTING_ERROR:
        return (char*)"路由错误";
    case CAUSE_FACILITY_REJECTED:
        return (char*)"设备拒绝";
    case CAUSE_NORMAL_UNSPECIFIED:
        return (char*)"通常,未指定";
    case CAUSE_TEMPORARY_FAILURE:
        return (char*)"临时错误";
    case CAUSE_RESOURCE_UNAVAIL:
        return (char*)"资源不可用";
    case CAUSE_INVALID_CALL_REF:
        return (char*)"不正确的呼叫参考号";
    case CAUSE_MANDATORY_IE_MISSING:
        return (char*)"必选信息单元丢失";
    case CAUSE_TIMER_EXPIRY:
        return (char*)"定时器超时";
    case CAUSE_CALL_REJ_BY_USER:
        return (char*)"被用户拒绝";
    case CAUSE_CALLEE_STOP:
        return (char*)"被叫停止";
    case CAUSE_USER_NO_EXIST:
        return (char*)"用户不存在";
    case CAUSE_MS_UNACCESSAVLE:
        return (char*)"不可接入";
    case CAUSE_MS_POWEROFF:
        return (char*)"用户关机";
    case CAUSE_FORCE_RELEASE:
        return (char*)"强制拆线";
    case CAUSE_HO_RELEASE:
        return (char*)"切换拆线";
    case CAUSE_CALL_CONFLICT:
        return (char*)"呼叫冲突";
    case CAUSE_TEMP_UNAVAIL:
        return (char*)"暂时无法接通";
    case CAUSE_AUTH_ERROR:
        return (char*)"鉴权错误";
    case CAUSE_NEED_AUTH:
        return (char*)"需要鉴权";
    case CAUSE_SDP_SEL:
        return (char*)"SDP选择错误";
    case CAUSE_MS_ERROR:
        return (char*)"媒体资源错误";
    case CAUSE_INNER_ERROR:
        return (char*)"内部错误";
    case CAUSE_PRIO_ERROR:
        return (char*)"优先级不够";
    case CAUSE_SRV_CONFLICT:
        return (char*)"业务冲突";
    case CAUSE_NOTREL_RECALL:
        return (char*)"由于业务要求,不释放,启动重呼定时器";
    case CAUSE_NO_CALL:
        return (char*)"呼叫不存在";
    case CAUSE_ERROR_IPADDR:
        return (char*)"错误IP地址过来的业务请求";
    case CAUSE_DUP_REG:
        return (char*)"重复注册";
    case CAUSE_MG_OFFLINE:
        return (char*)"MG离线";
    case CAUSE_DS_REQ_QUITCALL:
        return (char*)"调度员要求退出呼叫";
    case CAUSE_DB_ERROR:
        return (char*)"数据库操作错误";
    case CAUSE_TOOMANY_USER:
        return (char*)"用户数太多";
    case CAUSE_SAME_USERNUM:
        return (char*)"相同的用户号码";
    case CAUSE_SAME_USERIPADDR:
        return (char*)"相同的固定IP地址";
    case CAUSE_PARAM_ERROR:
        return (char*)"参数错误";
    case CAUSE_SAME_GNUM:
        return (char*)"相同的组号码";
    case CAUSE_TOOMANY_GROUP:
        return (char*)"太多的组";
    case CAUSE_NO_GROUP:
        return (char*)"没有这个组";
    case CAUSE_SAME_USERNAME:
        return (char*)"相同的用户名字";
    case CAUSE_OAM_OPT_ERROR:
        return (char*)"OAM操作错误";
    case CAUSE_INVALID_NUM_FORMAT:
        return (char*)"不正确的地址格式";
    case CAUSE_INVALID_DNSIP:
        return (char*)"DNS或IP地址错误";
    case CAUSE_SRV_NOTSUPPORT:
        return (char*)"不支持的业务";
    case CAUSE_MEDIA_NOTDATA:
        return (char*)"没有媒体数据";
    case CAUSE_RECALL:
        return (char*)"重新呼叫";
    case CAUSE_LINK_DISC:
        return (char*)"断链";
    case CAUSE_ORG_RIGHT:
        return (char*)"组织越权";
    case CAUSE_SAME_ORGNUM:
        return (char*)"相同的组织号码";
    case CAUSE_SAME_ORGNAME:
        return (char*)"相同的组织名字";
    case CAUSE_UNASSIGNED_ORG:
        return (char*)"未分配的组织号码";
    case CAUSE_INOTHER_ORG:
        return (char*)"在其他组织中";
    case CAUSE_HAVE_GCALL:
        return (char*)"已经有组呼存在";
    case CAUSE_HAVE_CONF:
        return (char*)"已经有会议存在";
    case CAUSE_SEG_FORMAT:
        return (char*)"错误的号段格式";
    case CAUSE_USEG_CONFLICT:
        return (char*)"用户号码段冲突";
    case CAUSE_GSEG_CONFLICT:
        return (char*)"组号码段冲突";
    case CAUSE_NOTIN_SEG:
        return (char*)"不在号段内";
    case CAUSE_USER_IN_TOOMANY_GROUP:
        return (char*)"用户所在组太多";
    case CAUSE_DSSEG_CONFLICT:
        return (char*)"调度台号段冲突";
    case CAUSE_OUTNETWORK_NUM:
        return (char*)"外网用户";
    case CAUSE_CFU:
        return (char*)"无条件呼叫前转";
    case CAUSE_CFB:
        return (char*)"遇忙呼叫前转";
    case CAUSE_CFNRc:
        return (char*)"不可及呼叫前转";
    case CAUSE_CFNRy:
        return (char*)"无应答呼叫前转";
    case CAUSE_MAX_FWDTIME:
        return (char*)"最大前转次数";
    case CAUSE_OAM_RIGHT:
        return (char*)"OAM操作无权限";
    case CAUSE_DGT_ERROR:
        return (char*)"号码错误";
    case CAUSE_RESOURCE_NOTENOUGH:
        return (char*)"资源不足";
    case CAUSE_ORG_EXPIRE:
        return (char*)"组织到期";
    case CAUSE_USER_EXPIRE:
        return (char*)"用户到期";
    case CAUSE_SAME_ROUTENAME:
        return (char*)"相同的路由名字";
    case CAUSE_UNASSIGNED_ROUTE:
        return (char*)"未分配的路由";
    case CAUSE_OAM_FWD:
        return (char*)"OAM消息前转";
    case CAUSE_UNCFG_MAINNUM:
        return (char*)"未配置主号码";
    case CAUSE_G_NOUSER:
        return (char*)"组中没有用户";
    case CAUSE_U_LOCK_G:
        return (char*)"用户锁定在其他组";
    case CAUSE_U_OFFLINE_G:
        return (char*)"组中没有在线用户";
    default:
        snprintf(cBuf, sizeof(cBuf), "CAUSE=%d", usCause);
        return cBuf;
    }
}
/******************************************************************************************/

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

    OSI_LOGXE(OSI_LOGPAR_S, 0, "%s", (char*)cBuf);
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
	osiMutex_t  *mutex;
	bool        isReady;
	int 		pipe;
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
	Msg_GROUP_MEMBER_s member_call_obj;
	bool is_member_call;
	int member_call_dir;  //0 tx,  1 rx
	unsigned int mic_ctl;
	DWORD current_group;
	DWORD query_group;
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
	cJSON * self_info_cjson;
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
	osiTimer_t * optGUadd_timer;
	osiTimer_t * openpa_timer;
	osiTimer_t * closepa_timer;
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
	osiTimer_t * callidle_timer;
#endif
	osiTimer_t * check_ack_timeout_timer;
	bool onepoweron;
	char build_self_name[16];
#ifndef MUCHGROUP
	int buildgroupnumber;
#endif
	bool   is_makeout_call;
	bool   is_release_call;
	bool   is_justnow_listen;
	bool   is_check_listen;
	int    call_error_case;
	int    current_group_member_dwnum;
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

CIdtUser m_IdtUser;
static PocGuiIIdtComAttr_t pocIdtAttr = {0};

int Func_GQueryU(DWORD dwSn, UCHAR *pucGNum)
{
    QUERY_EXT_s ext;
    memset(&ext, 0, sizeof(ext));
    if(pucGNum == NULL)
    {
	    ext.ucAll = 1;  //查询所有的用户
	    pucGNum = (UCHAR *)"0";
    }

	ext.ucGroup = 0;	//组下组不呈现
	ext.ucUser	= 1;	//查询用户
    ext.dwPage  = 0;    //从第0页开始
    ext.dwCount = GROUP_MAX_MEMBER;//每页有1024用户
    ext.ucOrder =0;     //排序方式,0按号码排序,1按名字排序
    IDT_GQueryU(dwSn, pucGNum, &ext);
    return 0;
}

int Func_GQueryGAll(DWORD dwSn)
{
    QUERY_EXT_s ext;
    memset(&ext, 0, sizeof(ext));

	ext.ucAll = 1;  //查询所有的用户
	ext.ucGroup = 1;	//组下组呈现
	ext.ucUser	= 0;	//查询用户
    ext.dwPage  = 0;    //从第0页开始
    ext.dwCount = GROUP_MAX_MEMBER;//每页有1024用户
    ext.ucOrder =0;     //排序方式,0按号码排序,1按名字排序
    IDT_GQueryU(dwSn, (UCHAR *)"0", &ext);
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
	/*tell me group info ind*/
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

	GU_STATUSGINFO_s *gu_status = (GU_STATUSGINFO_s *)malloc(sizeof(GU_STATUSGINFO_s));

	if(gu_status != NULL)
	{
	    memcpy(gu_status, (const void *)pStatus, sizeof(GU_STATUSGINFO_s));
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GU_STATUS_REP, (void *)gu_status);
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

	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][call peer answer]%s(%d):[server]receive call_ack", __func__, __LINE__);

	pocIdtAttr.is_release_call = false;//obtain call
	pocIdtAttr.is_justnow_listen = false;

	if(pocIdtAttr.is_member_call)
	{
		if(m_IdtUser.m_iCallId != -1)
		{
			IDT_CallMicCtrl(m_IdtUser.m_iCallId, false);
			OSI_LOGI(0, "[poc][call peer answer]m_iCallId != -1");
		}
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP, NULL);
		OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][call peer answer]%s(%d):[server]receive single_call_ack --> call_status_ok_rep send msg", __func__, __LINE__);
	}

	if(m_IdtUser.m_status == USER_OPRATOR_START_SPEAK)
	{
		OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][call peer answer]%s(%d):[server]receive start_speak_ack --> mic_rep send msg", __func__, __LINE__);
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
    bool is_member_call = LvGuiIdtCom_self_is_member_call(pcUserMark);
	if(m_IdtUser.m_iCallId != -1)
	{
		if(SrvType == SRV_TYPE_CONF && is_member_call)
		{
			if(IDT_CallRel(m_IdtUser.m_iCallId, NULL, CAUSE_ZERO) == -1)
			{
				IDT_CallRel(ID, NULL, CAUSE_CALL_CONFLICT);
				OSI_LOGI(0, "[poc][call in]call conflict");
				return 0;
			}
			m_IdtUser.m_iCallId = -1;
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][call in]%s(%d):m_iCallId != -1 and have conf to release single call", __func__, __LINE__);
		}
		else
		{
			IDT_CallRel(ID, NULL, CAUSE_HAVE_CONF);
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][call in]%s(%d):m_iCallId != -1 and have conf to release group call", __func__, __LINE__);
			return 0;
		}
	}

    m_IdtUser.m_iCallId = ID;
    m_IdtUser.m_iRxCount = 0;
    m_IdtUser.m_iTxCount = 0;
    memset(&pocIdtAttr.attr, 0, sizeof(MEDIAATTR_s));
    pocIdtAttr.attr.ucAudioRecv = 1;

    switch (SrvType)
    {
	    case SRV_TYPE_BASIC_CALL://单呼
	        {
		        IDT_CallRel(ID, NULL, CAUSE_SRV_NOTSUPPORT);
		        m_IdtUser.m_iCallId = -1;
				OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][call in]%s(%d):call failed", __func__, __LINE__);
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
					pocIdtAttr.is_member_call = true;
		            lv_poc_activity_func_cb_set.member_call_open((void *)&member_call_obj);
					pocIdtAttr.membercall_count = 0;//复位单呼数据计数
					lv_poc_set_first_membercall(true);
					lv_poc_set_play_tone_status(false);
					m_IdtUser.m_status = USER_OPRATOR_START_LISTEN;
					OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][call in]%s(%d):rec single call", __func__, __LINE__);
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

					OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][call in]%s(%d):rec group call", __func__, __LINE__);
					//write flag
					pocIdtAttr.is_justnow_listen = true;
					pocIdtAttr.is_release_call = false;//obtain call
					m_IdtUser.m_status = USER_OPRATOR_START_LISTEN;
			   		lv_poc_set_play_tone_status(false);
	           }
	        }
	        break;

	    default:
	        IDT_CallRel(ID, NULL, CAUSE_SRV_NOTSUPPORT);
	        m_IdtUser.m_iCallId = -1;

			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][call in]%s(%d):not support srv", __func__, __LINE__);

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

	//reset call cnt
	m_IdtUser.m_iCallId = -1;
    m_IdtUser.m_iRxCount = 0;
    m_IdtUser.m_iTxCount = 0;
	pocIdtAttr.is_release_call = true;//release call

	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][rel ind]%s(%d):release call and reset m_iCallId = -1", __func__, __LINE__);

	int status = m_IdtUser.m_status;
    if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
    {
	    m_IdtUser.m_status = UT_STATUS_ONLINE;
    }

    if(status >= USER_OPRATOR_START_SPEAK && status <= USER_OPRATOR_SPEAKING)
    {
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND, NULL);
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP, (void *)status);

		/*发送呼叫原因*/
		lvPocGuiIdtCase_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_SPEAK_CALL_CASE, (void *)uiCause, LvPocGetCauseStr(uiCause));
    }

    if((status >= USER_OPRATOR_START_LISTEN && status <= USER_OPRATOR_LISTENNING)
		|| (status == UT_STATUS_ONLINE && pocIdtAttr.is_check_listen == true))
    {
		OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][rel ind]%s(%d):listen release call", __func__, __LINE__);
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP, NULL);
    }

    if(pocIdtAttr.is_member_call)
    {
		pocIdtAttr.membercall_count = 0;
		lv_poc_set_first_membercall(true);
		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP, (void *)uiCause);
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
    IDT_TRACE("callback_IDT_CallMicInd: pUsrCtx=0x%x", pUsrCtx);
	// 0本端不讲话
    // 1本端讲话
    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_REP, (void *)(uiInd + 1));
	OSI_LOGXI(OSI_LOGPAR_SIS, 0, "[poc][call mic ind]%s(%d):[server]call instruction=(%s)", __func__, __LINE__, (uiInd + 1) == 1?"release the words right":"obtain the words right");
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
    IDT_TRACE("callback_IDT_CallRecvAudioData: pUsrCtx=0x%x, dwStreamId=%d, ucCodec=%d pucBuf=0x%x iLen=%d dwTsOfs=%d dwTsLen=%d dwTs=%d RxCount=%d",
	    pUsrCtx, dwStreamId, ucCodec, pucBuf, iLen, dwTsOfs, dwTsLen, dwTs, m_IdtUser.m_iRxCount);
	if(m_IdtUser.m_status >= USER_OPRATOR_START_LISTEN
		&& m_IdtUser.m_status <= USER_OPRATOR_LISTENNING)
	{
	    m_IdtUser.m_iRxCount = m_IdtUser.m_iRxCount + 1;

#if POC_AUDIO_MODE_PIPE
		pocAudioPipeWriteData((const uint8_t *)pucBuf, iLen);
#else
		pocAudioPlayerWriteData(pocIdtAttr.player, (const uint8_t *)pucBuf, iLen);
#endif
		pocIdtAttr.check_listen_count++;

		if(m_IdtUser.m_iRxCount >= 10
			&& lv_poc_get_play_tone_status())
		{
			lv_poc_set_play_tone_status(false);
			pocIdtAttr.membercall_count++;//记录单呼数据帧
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
    IDT_TRACE("callback_IDT_CallTalkingIDInd: pUsrCtx=0x%x, pcNum=%s, pcName=%s, curstatus=%d", pUsrCtx, pcNum, pcName, m_IdtUser.m_status);
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
		OSI_LOGI(0, "[poc][TalkingIDInd](%d):pcNum or pcName NULL, m_status(%d)", __LINE__, m_IdtUser.m_status);
	    if(m_IdtUser.m_status >= USER_OPRATOR_START_LISTEN
			&& m_IdtUser.m_status <= USER_OPRATOR_LISTENNING
			&& m_IdtUser.m_iRxCount > 5)
	    {
		    if(pocIdtAttr.delay_close_listen_timer != NULL)
		    {
			    if(pocIdtAttr.delay_close_listen_timer_running)
			    {
				    osiTimerStop(pocIdtAttr.delay_close_listen_timer);
				    pocIdtAttr.delay_close_listen_timer_running = false;
			    }
			    pocIdtAttr.check_listen_count = 0;
			    osiTimerStop(pocIdtAttr.check_listen_timer);
			    osiTimerStart(pocIdtAttr.delay_close_listen_timer, 560);
			    pocIdtAttr.delay_close_listen_timer_running = true;
				OSI_LOGI(0, "[poc][TalkingIDInd](%d):delay_close_listen_timer 560ms", __LINE__);
		    }
		    else
		    {
				OSI_LOGI(0, "[poc][TalkingIDInd](%d):delay_close_listen_timer NULL", __LINE__);
			    osiTimerStop(pocIdtAttr.check_listen_timer);
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

		if(!lv_poc_get_first_membercall())
		{
			pocIdtAttr.membercall_count++;//记录单呼数据帧
			lv_poc_set_first_membercall(true);
		}
		else
		{
			lv_poc_set_first_membercall(false);
		}
	    return 0;
    }
	OSI_LOGI(0, "[poc][TalkingIDInd](%d):Normal, status(%d)", __LINE__, m_IdtUser.m_status);
    strcpy((char *)pocIdtAttr.speaker.ucNum, (const char *)pcNum);
    strcpy((char *)pocIdtAttr.speaker.ucName, (const char *)pcName);

    pocIdtAttr.speaker.ucStatus = UT_STATUS_ONLINE;
    if((m_IdtUser.m_status >= USER_OPRATOR_START_LISTEN
		&& m_IdtUser.m_status <= USER_OPRATOR_LISTENNING)
		|| m_IdtUser.m_status == UT_STATUS_ONLINE)
    {
		OSI_LOGI(0, "[poc][TalkingIDInd](%d):status(%d), start listen", __LINE__, m_IdtUser.m_status);
	    if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
	    {
		    m_IdtUser.m_status = USER_OPRATOR_START_LISTEN;
	    }
		m_IdtUser.m_iRxCount = 0;
		m_IdtUser.m_iTxCount = 0;
	    if(pocIdtAttr.delay_close_listen_timer_running)
	    {
			OSI_LOGI(0, "[poc][TalkingIDInd](%d):close delay listen timer", __LINE__);
		    osiTimerStop(pocIdtAttr.delay_close_listen_timer);
		    pocIdtAttr.delay_close_listen_timer_running = false;
	    }
		osiTimerStop(pocIdtAttr.check_listen_timer);
		pocIdtAttr.check_listen_count = 5*20;//容许5帧的网络延时
		osiTimerStartPeriodic(pocIdtAttr.check_listen_timer, 20);
		if(!(pocIdtAttr.is_member_call
			&& pocIdtAttr.membercall_count == 0))
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Listen, 30, true);
		}
		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP, NULL);
		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SET_SCREEN_STATUS_IND, NULL);

		pocIdtAttr.membercall_count++;//记录单呼数据帧
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
		//gps monitor
		osiTimerStop(pocIdtAttr.callidle_timer);
		lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_STOP_GPS_LOCATION, NULL);
#endif
		OSI_LOGI(0, "[poc][TalkingIDInd](%d):start listen to send msg, UI display", __LINE__);
	}
	else
	{
		OSI_LOGI(0, "[poc][TalkingIDInd](%d):status(%d), error", __LINE__, m_IdtUser.m_status);
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
    UOpt.dwSn = dwSn;/*操作序号*/
    UOpt.wRes = wRes;/*结果*/
    if(pUser != NULL)
    {
	    UOpt.haveUser = true;
	    memcpy(&UOpt.pUser, pUser, sizeof(UData_s));
    }
	else
	{
		UOpt.haveUser = false;
	}

    if(dwOptCode == OPT_USER_QUERY)//查询用户
    {
	    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP, &UOpt);
		OSI_PRINTFI("[poc][UOptRsp][user query]%s(%d):member info ack", __func__, __LINE__);
    }
    else if(dwOptCode == OPT_GMEMBER_EXTINFO)
    {
	    unsigned int result = (unsigned int)wRes;
	    if(pocIdtAttr.lockGroupOpt == LV_POC_GROUP_OPRATOR_TYPE_LOCK)/*锁组*/
	    {
		    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_REP, (void *)result);
			OSI_PRINTFI("[poc][UOptRsp][lock group]%s(%d):lock group success ack", __func__, __LINE__);
	    }
	    else if(pocIdtAttr.lockGroupOpt == LV_POC_GROUP_OPRATOR_TYPE_UNLOCK)/*解锁*/
	    {
		    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_REP, (void *)result);
			OSI_PRINTFI("[poc][UOptRsp][lock group]%s(%d):unlock group success ack", __func__, __LINE__);
	    }
		else if(pocIdtAttr.lockGroupOpt == LV_POC_GROUP_OPRATOR_TYPE_UNLOCK_BE_DELETED_GROUP)
		{
			lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_REP, NULL);
			OSI_PRINTFI("[poc][UOptRsp][lock group]%s(%d):be deleted group, unlock group success ack", __func__, __LINE__);
		}
	    else
	    {
		    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND, NULL);
			OSI_PRINTFI("[poc][UOptRsp][lock group]%s(%d):get invalid lock group msg, check lock group status from server", __func__, __LINE__);
	    }
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

    IDT_TRACE("callback_IDT_GOptRsp: dwOptCode=%s(%d), dwSn=%d, wRes=%s(%d), pGroup->ucNum=%s, pGroup->dwNum=%d, pGroup->ucName=%s",
        GetOamOptStr(dwOptCode), dwOptCode, dwSn, GetCauseStr(wRes), wRes, pGroup->ucNum, pGroup->dwNum, pGroup->ucName);

	static LvPocGuiIdtCom_Group_Operator_t grop = {0};

    grop.dwOptCode = dwOptCode;
    grop.dwSn = dwSn;
    grop.wRes = wRes;
    memcpy(&grop.pGroup, (const void *)pGroup, sizeof(GData_s));
	//组操作响应
    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GROUP_OPERATOR_REP, (void *)&grop);
	OSI_PRINTFI("[poc][GOptRsp]%s(%d):GOptRsp_G_OPT=(%ld),GOptRsp_GNUM=(%s)", __func__, __LINE__, dwOptCode, pGroup->ucNum);
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

		static LvPocGuiIdtCom_Group_Operator_t gropoptcode_t = {0};
	    gropoptcode_t.dwOptCode = dwOptCode;/*操作码*/
		strcpy((char *)gropoptcode_t.pGroup.ucNum, (char *)pucGNum);/*组号码*/
		strcpy((char *)gropoptcode_t.pGroup.ucName, (char *)pucGName);/*组名字*/

		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GROUP_OPERATOR_REP, (void *)&gropoptcode_t);
	}
    else if (NULL != pucUNum)
    {
        IDT_TRACE("callback_IDT_OamNotify: dwOptCode=%s(%d), pucUNum=%s, ucUAttr=%d",
            GetOamOptStr(dwOptCode), dwOptCode, pucUNum, ucUAttr);
    }
	OSI_PRINTFI("[poc][OamNotify]%s(%d):OamNotify_G_OPT=(%ld),OamNotify_GNUM=(%s)", __func__, __LINE__, dwOptCode, pucGNum);
}

//--------------------------------------------------------------------------------
//      组成员扩展信息指示
//  输入:
//      pucGNum:        组号码,如果为空,表示是订阅所有用户返回的
//      pInfo:          扩展信息
//  返回:
//      0:              成功
//      -1:             失败
//  注意:
//      由IDT.dll调用,告诉用户扩展信息指示
//--------------------------------------------------------------------------------
int callback_IDT_GMemberExtInfoInd(UCHAR *pucGNum, GMEMBER_EXTINFO_s *pInfo)
{
	IDT_TRACE("callback_IDT_GMemberExtInfoInd: pucGNum->wNum=%d", pucGNum, pInfo->wNum);
    return 0;
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
	OSI_PRINTFI("[poc][time cb][listen]%s(%d):delay close display and stop play", __func__, __LINE__);
}

static void LvGuiIdtCom_start_speak_voice_timer_cb(void *ctx)
{
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND, NULL);
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP, NULL);
	pocIdtAttr.start_speak_voice_timer_running = false;
	OSI_PRINTFI("[poc][time cb][speak]%s(%d):stop play and start record", __func__, __LINE__);
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
	OSI_PRINTFI("[poc][time cb][member]%s(%d):get member list", __func__, __LINE__);
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
	OSI_PRINTFI("[poc][time cb][group]%s(%d):get group list of include's self", __func__, __LINE__);
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
		OSI_PRINTFI("[poc][time cb][lock group]%s(%d):get lock group status timer cb -> restart 1s timer", __func__, __LINE__);
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
	OSI_PRINTFI("[poc][time cb][lock group]%s(%d):send msg to get lock group status", __func__, __LINE__);
}

static void LvGuiIdtCom_check_listen_timer_cb(void *ctx)
{
	if(pocIdtAttr.check_listen_count < 1)
	{
		if(m_IdtUser.m_status > UT_STATUS_OFFLINE)
		{
			m_IdtUser.m_status = UT_STATUS_ONLINE;
		}
		OSI_PRINTFI("[poc][time cb][check listen]%s(%d):no data packet, launch 460ms time", __func__, __LINE__);
	    if(pocIdtAttr.delay_close_listen_timer_running)
	    {
		    osiTimerStop(pocIdtAttr.delay_close_listen_timer);
		    pocIdtAttr.delay_close_listen_timer_running = false;
	    }
		if((!lv_poc_get_first_membercall())
			|| (pocIdtAttr.membercall_count > 1))
		{
		    osiTimerStart(pocIdtAttr.delay_close_listen_timer, 460);
		    pocIdtAttr.delay_close_listen_timer_running = true;
		}
		pocIdtAttr.check_listen_count = 0;
		pocIdtAttr.is_check_listen = true;
		osiTimerStop(pocIdtAttr.check_listen_timer);
		return;
	}
	pocIdtAttr.check_listen_count--;
	OSI_PRINTFI("[poc][time cb][check listen]%s(%d):m_status(%d), listencount(%d)", __func__, __LINE__, m_IdtUser.m_status, pocIdtAttr.check_listen_count);
}

static void LvGuiIdtCom_try_login_timer_cb(void *ctx)
{
	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND, NULL))
	{
		osiTimerStart(pocIdtAttr.try_login_timer, 2000);
	}
	OSI_PRINTFI("[poc][time cb][login]%s(%d):try to relogin", __func__, __LINE__);
}

static void LvGuiIdtCom_auto_login_timer_cb(void *ctx)
{
	if(pocIdtAttr.loginstatus_t != LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS)
	{
		if(lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND, NULL))
		{
			OSI_LOGI(0, "[poc][time cb][login]autologin ing...");
			pocIdtAttr.loginstatus_t = LVPOCLEDIDTCOM_SIGNAL_LOGIN_ING;
		}
	}
	OSI_PRINTFI("[poc][time cb][login]%s(%d):autologin", __func__, __LINE__);
}

static void LvGuiIdtCom_ppt_release_timer_cb(void *ctx)
{
	static int makecallcnt = 0;
	bool pttStatus = pocGetPttKeyState() || lv_poc_get_earppt_state();
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
		OSI_PRINTFI("[poc][time cb][ptt]%s(%d):release", __func__, __LINE__);
	}
}

static void LvGuiIdtCom_recorder_timer_cb(void *ctx)
{
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND, NULL);
}

static void LvGuiIdtCom_optGUAdd_timer_cb(void *ctx)
{
	OSI_LOGI(0, "[grouprefr][timecb](%d):opt user of add or del", __LINE__);
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF, NULL);
}

static void LvGuiIdtCom_open_pa_timer_cb(void *ctx)
{
	OSI_LOGI(0, "[pa][timecb](%d):open pa", __LINE__);
	poc_set_ext_pa_status(true);
}

static void LvGuiIdtCom_close_pa_timer_cb(void *ctx)
{
	OSI_LOGI(0, "[pa][timecb][audio](%d):close pa", __LINE__);
	poc_set_ext_pa_status(false);
}
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
static void LvGuiIdtCom_call_idle_timer_cb(void *ctx)
{
	lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_START_GPS_LOCATION, NULL);
}
#endif

static void LvGuiIdtCom_check_ack_timeout_timer_cb(void *ctx)
{
    if(pocIdtAttr.pocGetMemberListCb != NULL)
    {
        pocIdtAttr.pocGetMemberListCb(0, 0, NULL);
        pocIdtAttr.pocGetMemberListCb = NULL;
    }
    else if(pocIdtAttr.pocGetGroupListCb != NULL)
    {
        pocIdtAttr.pocGetGroupListCb(0, 0, NULL);
        pocIdtAttr.pocGetGroupListCb = NULL;
    }
	else if(pocIdtAttr.pocBuildGroupCb != NULL)
	{
		pocIdtAttr.pocBuildGroupCb(0);
		pocIdtAttr.pocBuildGroupCb = NULL;
	}
	pocIdtAttr.loginstatus_t = LVPOCLEDIDTCOM_SIGNAL_LOGIN_FAILED;
	m_IdtUser.m_status = UT_STATUS_OFFLINE;
	m_IdtUser.m_Group.m_Group_Num = 0;
	pocIdtAttr.pPocMemberList->dwNum = 0;
	lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_NOLOGIN);
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
	return 0;

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
    CallBack.pfGMemberExtInfoInd= callback_IDT_GMemberExtInfoInd;

    CallBack.pfDbg              = callback_IDT_Dbg;

    nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

    if(strcmp((const char *)poc_config->old_account_name, (const char *)poc_config->account_name) != 0)
    {
	    strcpy((char *)poc_config->old_account_name, (const char *)poc_config->account_name);
	    poc_config->old_account_current_group[0] = 0;
    }

	#if 1
    IDT_Start(NULL, 1, (char*)poc_config->ip_address, poc_config->ip_port, NULL, 0, (char*)poc_config->account_name, (char*)poc_config->account_passwd, 1, &CallBack, 0, 20000, 0);
    //IDT_Start(NULL, 1, (char*)"106.74.78.213:10000", 10000, NULL, 0, (char*)"81000102", (char*)"81000102", 1, &CallBack, 0, 20000, 0);
	#else
	IDT_Start(NULL, 1, (char*)"124.160.11.22:10200", 10000, NULL, 0, (char*)"89860620180041632069", (char*)"2014", 1, &CallBack, 0, 20000, 0);
	#endif
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
			static int play_voice_delay = 0;

#if POC_AUDIO_MODE_PIPE
			pocIdtAttr.pipe = pocAudioPipeCreate(8192);//8kb
			pocIdtAttr.player = pocAudioPlayerCreate(1024);//1kb
#else
			if(pocIdtAttr.player == 0)
			{
				pocIdtAttr.player = pocAudioPlayerCreate(8192);//8kb
			}
#endif
			if(pocIdtAttr.recorder == 0)
			{												//40k
				pocIdtAttr.recorder = pocAudioRecorderCreate(40960, 320, 20, lvPocGuiIdtCom_send_data_callback);
			}
#if POC_AUDIO_MODE_PIPE
			if(pocIdtAttr.pipe == 0
				|| pocIdtAttr.recorder == 0
				|| pocIdtAttr.player == 0)
			{
				pocIdtAttr.isReady = false;
				osiThreadExit();
			}
#else
			if(pocIdtAttr.player == 0 || pocIdtAttr.recorder == 0)
			{
				pocIdtAttr.isReady = false;
				osiThreadExit();
			}
#endif
			if(ctx == 0)
			{
				break;
			}
			LvPocGuiIdtCom_login_t * login_info = (LvPocGuiIdtCom_login_t *)ctx;

		    if (UT_STATUS_ONLINE > login_info->status)
		    {
				if(pocIdtAttr.loginstatus_t == LVPOCLEDIDTCOM_SIGNAL_LOGIN_FAILED)
				{
					break;
				}

				if(login_info->cause == 33)
				{//账号在别处被登录
					poc_play_voice_one_time(LVPOCAUDIO_Type_This_Account_Already_Logined, 50, true);
  					pocIdtAttr.loginstatus_t = LVPOCLEDIDTCOM_SIGNAL_LOGIN_FAILED;
				}
				else
				{//当前未登录
					if(pocIdtAttr.onepoweron == false
						&& !lv_poc_watchdog_status())
					{
						play_voice_delay++;
						if(play_voice_delay >= 30 || play_voice_delay == 1)//2min
						{
							poc_play_voice_one_time(LVPOCAUDIO_Type_No_Login, 50, true);
							play_voice_delay = 1;
						}
					}
					pocIdtAttr.loginstatus_t = LVPOCLEDIDTCOM_SIGNAL_LOGIN_FAILED;

					//开启自动登陆功能
					osiTimerStart(pocIdtAttr.auto_login_timer, 20000);
				}
				lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_NOLOGIN);
				m_IdtUser.m_status = UT_STATUS_OFFLINE;
				m_IdtUser.m_Group.m_Group_Num = 0;
				pocIdtAttr.pPocMemberList->dwNum = 0;
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "登录失败");
				osiTimerStop(pocIdtAttr.get_group_list_timer);
				osiTimerStop(pocIdtAttr.get_lock_group_status_timer);
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND, NULL);
				#if 0
			    pocIdtAttr.isReady = false;
				#else//can auto login
				pocIdtAttr.isReady = true;
				#endif
				break;
		    }
		    pocIdtAttr.isReady = true;

	        IDT_StatusSubs((char*)"###", GU_STATUSSUBS_BASIC);
	        if(m_IdtUser.m_status < UT_STATUS_ONLINE)//登录成功
	        {
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
				pocIdtAttr.loginstatus_t = LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS;

				if(pocIdtAttr.onepoweron == false
					&& !lv_poc_watchdog_status())
				{
					poc_play_voice_one_time(LVPOCAUDIO_Type_Success_Login, 50, true);
				}
				pocIdtAttr.onepoweron = true;
				osiTimerStop(pocIdtAttr.auto_login_timer);
			}
	        m_IdtUser.m_status = UT_STATUS_ONLINE;
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "成功登录");
			osiTimerStart(pocIdtAttr.monitor_recorder_timer, 20000);//20S

			LvGuiIdtCom_self_info_json_parse_status();
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, NULL);
			osiTimerStartRelaxed(pocIdtAttr.get_group_list_timer, 500, OSI_WAIT_FOREVER);
			lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_LOGINSUCCESS);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_EXIT_IND:
		{
			if(m_IdtUser.m_status < UT_STATUS_ONLINE)
			{
				break;
			}
			pocIdtAttr.loginstatus_t = LVPOCLEDIDTCOM_SIGNAL_LOGIN_EXIT;
			IDT_Exit();
            pocIdtAttr.isReady = false;//关机后禁止发消息
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

			if(pocIdtAttr.loginstatus_t != LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS
				|| lv_poc_get_apply_note() == POC_APPLY_NOTE_TYPE_NOLOGIN)
			{
				pocIdtAttr.loginstatus_t = LVPOCLEDIDTCOM_SIGNAL_LOGIN_ING;
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "尝试登录中...");
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"未登录", (const uint8_t *)"尝试登录...");
				osiTimerIsRunning(pocIdtAttr.try_login_timer) ? 0 : osiTimerStart(pocIdtAttr.try_login_timer, 2000);
				break;
			}
			else if(pocIdtAttr.loginstatus_t == LVPOCLEDIDTCOM_SIGNAL_LOGIN_ING)
			{
				break;
			}

			if(pocIdtAttr.is_release_call)//call no exist
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"正在申请", (const uint8_t *)"");
			}

			if(pocIdtAttr.listen_status == true)
			{
				return;
			}

			osiTimerStop(pocIdtAttr.monitor_recorder_timer);
			lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_RESUME_IND, NULL);
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
					strcpy((char *)pocIdtAttr.speaker_group.m_ucGName, (const char *)m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
					strcpy((char *)pocIdtAttr.speaker_group.m_ucGNum, (const char *)m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGNum);
					dest_num = (char *)pocIdtAttr.speaker_group.m_ucGNum;
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
			}//group call, speak group number and current group number is different
			else if(!pocIdtAttr.is_member_call && strcmp((const char *)pocIdtAttr.speaker_group.m_ucGNum, (const char *)m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGNum) != 0)
			{
				//note call failed case
				if(pocIdtAttr.is_release_call == false && pocIdtAttr.is_justnow_listen == true)
				{
					pocIdtAttr.call_error_case = LV_POC_CALL_ERROR_GROUP_IS_BUSY;
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_ERROR_MSG, (const uint8_t *)"组呼失败,其他组呼叫线路存在", (const uint8_t *)"");
					return;
				}

				if(0 == IDT_CallRel(m_IdtUser.m_iCallId, NULL, CAUSE_ZERO))//若要改为立马释放呼叫的话加入该原因值：CAUSE_CALL_REJ_BY_USER
				{
					m_IdtUser.m_iCallId = -1;
				}

				char *dest_num = NULL;
				char *user_mark = NULL;
				memset(&pocIdtAttr.attr, 0, sizeof(MEDIAATTR_s));
				pocIdtAttr.attr.ucAudioSend = 1;
				SRV_TYPE_e srv_type = SRV_TYPE_NONE;
				srv_type = SRV_TYPE_CONF;
				strcpy((char *)pocIdtAttr.speaker_group.m_ucGName, (const char *)m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
				strcpy((char *)pocIdtAttr.speaker_group.m_ucGNum, (const char *)m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGNum);
				dest_num = (char *)pocIdtAttr.speaker_group.m_ucGNum;
				user_mark = (char *)GUIIDTCOM_GROUP_CALL_MARK;

				m_IdtUser.m_iCallId = IDT_CallMakeOut(dest_num,//对方号码
					srv_type,//会议
					&pocIdtAttr.attr,//媒体属性
					NULL,
					NULL,
					1,//直接呼出
					0,//会议结束后不删除组
					1,//自动话权
					user_mark);//用户标识字符串
			}
			else
			{
		        lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_IND, GUIIDTCOM_REQUEST_MIC);
			}
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP:
		{
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "开始对讲", NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始对讲", NULL);

			if(m_IdtUser.m_status == USER_OPRATOR_SPEAKING)
			{
				/*开始闪烁*/
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

				char speak_name[100] = "";
				strcpy((char *)speak_name, (const char *)"主讲:");
				strcat((char *)speak_name, (const char *)pocIdtAttr.self_info.ucName);
				if(!pocIdtAttr.is_member_call)
				{
					char group_name[100] = "";
					strcpy((char *)group_name, (const char *)pocIdtAttr.speaker_group.m_ucGName);
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, speak_name, group_name);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)speak_name, (const uint8_t *)group_name);
				}
				else
				{
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, speak_name, "");
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)speak_name, (const uint8_t *)"");
				}
				pocIdtAttr.is_makeout_call = true;
				osiTimerStart(pocIdtAttr.monitor_pptkey_timer, 50);

				OSI_PRINTFI("[poc][idtspeak]%s(%d):start speak rep to dispaly idleview", __func__, __LINE__);
			}
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

			OSI_PRINTFI("[poc][idtspeak]%s(%d):stop speak ind, to mic ind and release mic", __func__, __LINE__);

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP:
		{
			if((ctx == USER_OPRATOR_SPEAKING
				&& pocIdtAttr.is_makeout_call == true)
				|| (ctx == USER_OPRATOR_START_SPEAK
      				&& pocIdtAttr.is_makeout_call == true))
			{
				/*恢复run闪烁*/
			    lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

				poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Speak, 30, true);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, "停止对讲", NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"停止对讲", NULL);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				pocIdtAttr.is_makeout_call = false;
				osiTimerStop(pocIdtAttr.monitor_pptkey_timer);
				pocIdtAttr.speak_status = false;

				OSI_PRINTFI("[poc][idtspeak]%s(%d):receive stop_speak_rep", __func__, __LINE__);
				//monitor recorder thread
				osiTimerStart(pocIdtAttr.monitor_recorder_timer, 10000);//10S
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
				//monitor idle, goto location
				osiTimerStart(pocIdtAttr.callidle_timer, 10000);//10S
#endif
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
		        IDT_CallMicCtrl(m_IdtUser.m_iCallId, false);

				#if GUIIDTCOM_IDTSPEAK_DEBUG_LOG
				char cOutstr[256] = {0};
				cOutstr[0] = '\0';
				sprintf(cOutstr, "[idtspeak]%s(%d):release mic ind to start", __func__, __LINE__);
				OSI_LOGI(0, cOutstr);
				#endif
			}
			else
			{
		        //请求话权
		        IDT_CallMicCtrl(m_IdtUser.m_iCallId, true);

				#if GUIIDTCOM_IDTSPEAK_DEBUG_LOG
				char cOutstr[256] = {0};
				cOutstr[0] = '\0';
				sprintf(cOutstr, "[idtspeak]%s(%d):request mic ind to start", __func__, __LINE__);
				OSI_LOGI(0, cOutstr);
				#endif
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
			bool pttStatus = pocGetPttKeyState() || lv_poc_get_earppt_state();

			if(mic_ctl > 1 && pocIdtAttr.mic_ctl <= 1 && m_IdtUser.m_status == USER_OPRATOR_START_SPEAK)  //获得话权
			{
				if(pttStatus)
				{
					m_IdtUser.m_iRxCount = 0;
					m_IdtUser.m_iTxCount = 0;

				    if(pocIdtAttr.start_speak_voice_timer != NULL)
				    {
					    if(pocIdtAttr.start_speak_voice_timer_running)
					    {
						    osiTimerStop(pocIdtAttr.start_speak_voice_timer);
						    pocIdtAttr.start_speak_voice_timer_running = false;
					    }
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
						//gps monitor
						osiTimerStop(pocIdtAttr.callidle_timer);
						osiMutexLock(pocIdtAttr.mutex);
						lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_STOP_GPS_LOCATION, NULL);
						osiMutexUnlock(pocIdtAttr.mutex);
#endif
						poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Speak, 30, true);
						osiTimerStart(pocIdtAttr.start_speak_voice_timer, 200);//
					    pocIdtAttr.start_speak_voice_timer_running = true;
						pocIdtAttr.speak_status = true;
				    }

				    pocIdtAttr.mic_ctl = mic_ctl;
			    }
			    else
			    {
				    poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Cannot_Speak, 30, false);
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
			        lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP, (void *)status);
				}
				pocIdtAttr.mic_ctl = mic_ctl;
			}
			else
			{
				#if GUIIDTCOM_IDTSPEAK_DEBUG_LOG
				char cOutstr[256] = {0};
				cOutstr[0] = '\0';
				sprintf(cOutstr, "[idtspeak]%s(%d):mic rep --> not obtain call and not release call", __func__, __LINE__);
				OSI_LOGI(0, cOutstr);
				#endif
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
				OSI_LOGI(0, "[group](%d):error", __LINE__);
				pocIdtAttr.pocGetGroupListCb(0, 0, NULL);
				break;
			}

			do
			{
				if(pocIdtAttr.pocGetGroupListCb == NULL)
				{
					break;
				}

				if(m_IdtUser.m_Group.m_Group_Num < 1
					|| pocIdtAttr.loginstatus_t == LVPOCLEDIDTCOM_SIGNAL_LOGIN_FAILED)
				{
					OSI_LOGI(0, "[group](%d):error", __LINE__);
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
			bool checked_current = false;

			if(!pocIdtAttr.isPocGroupListAll)
			{
				break;
			}
			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
			pocIdtAttr.isPocGroupListAll = false;

		    m_IdtUser.m_Group.Reset();
		    m_IdtUser.m_Group.m_Group_Num = pGInfo->usNum;

		    for (int i = 0; i < pGInfo->usNum; i++)
		    {
		        strcpy((char*)m_IdtUser.m_Group.m_Group[i].m_ucGNum, (char*)pGInfo->stGInfo[i].ucNum);
		        strcpy((char*)m_IdtUser.m_Group.m_Group[i].m_ucGName, (char*)pGInfo->stGInfo[i].ucName);
		        m_IdtUser.m_Group.m_Group[i].m_ucPriority = pGInfo->stGInfo[i].ucPrio;
		        if(!checked_current)
		        {
			        if(strlen((const char *)poc_config->old_account_current_group) > 0)
			        {
				        if(strcmp((const char *)poc_config->old_account_current_group, (const char *)pGInfo->stGInfo[i].ucNum) == 0)
				        {
					        pocIdtAttr.current_group = i;
					        lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
					        checked_current = true;
				        }
			        }
			        else
			        {
				        strcpy((char *)poc_config->old_account_current_group, (const char *)pGInfo->stGInfo[i].ucNum);
						lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
				        lv_poc_setting_conf_write();
				        pocIdtAttr.current_group = i;
				        checked_current = true;
			        }
		        }
		    }

		    if(!checked_current)
		    {
			    pocIdtAttr.current_group = 0;
		        strcpy((char *)poc_config->old_account_current_group, (const char *)pGInfo->stGInfo[pocIdtAttr.current_group].ucNum);
		        lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
		        lv_poc_setting_conf_write();
		        checked_current = true;
		    }

		    lv_poc_activity_func_cb_set.group_list.refresh_with_data(NULL);

		    if(!pocIdtAttr.isPocMemberListBuf)
		    {
				IDT_TRACE("isPocMemberListBuf false query group \n");
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP, NULL);
			}
			else
			{
				IDT_TRACE("isPocMemberListBuf true query group \n");
				osiTimerStartRelaxed(pocIdtAttr.get_member_list_timer, 500, OSI_WAIT_FOREVER);
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
			osiTimerStart(pocIdtAttr.check_ack_timeout_timer, 3000);
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
	static GData_s g_data = {0};
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

			if(pocIdtAttr.loginstatus_t == LVPOCLEDIDTCOM_SIGNAL_LOGIN_FAILED)
			{
				pocIdtAttr.pocBuildGroupCb(0);
				pocIdtAttr.pocBuildGroupCb = NULL;
				break;
			}

			lv_poc_build_new_group_t * new_group = (lv_poc_build_new_group_t *)ctx;
			Msg_GROUP_MEMBER_s * member = NULL;
			GROUP_MEMBER_s * gmember = NULL;

			memset(&g_data, 0, sizeof(GData_s));
			g_data.dwNum = new_group->num;
			g_data.ucPriority = m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucPriority;

			for(int i = 0; i < new_group->num; i++)
			{
				if(new_group->members[i] == NULL)
				{
					pocIdtAttr.pocBuildGroupCb(0);
					pocIdtAttr.pocBuildGroupCb = NULL;
					return;
				}
				member = (Msg_GROUP_MEMBER_s *)new_group->members[i];
				gmember = (GROUP_MEMBER_s *)&g_data.member[i];
				if(member == NULL
					|| gmember == NULL)
				{
					pocIdtAttr.pocBuildGroupCb(0);
					pocIdtAttr.pocBuildGroupCb = NULL;
					return;
				}
				gmember->ucType = GROUP_MEMBERTYPE_USER;
				strcpy((char *)gmember->ucName, (const char *)member->ucName);
				strcpy((char *)gmember->ucNum, (const char *)member->ucNum);
				strcpy((char *)gmember->ucAGNum, (const char *)member->ucAGNum);
				gmember->ucPrio = member->ucPrio;
				gmember->ucUTType = member->ucUTType;
				gmember->ucAttr = member->ucAttr;
				gmember->ucChanNum = member->ucChanNum;
				gmember->ucStatus = member->ucStatus;
				gmember->ucFGCount = member->ucFGCount;
			}

			//群组名字
			lv_poc_member_info_t self_info = lv_poc_get_self_info();
			char *self_name = lv_poc_get_member_name(self_info);
			strcpy((char *)g_data.ucName, (const char *)self_name);
			strcat((char *)g_data.ucName, (const char *)"(自建)");

			if(IDT_GAdd(0, &g_data) != 0)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)"添加组操作失败", (const uint8_t *)"");
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

			LvPocGuiIdtCom_Group_Operator_t *grop = (LvPocGuiIdtCom_Group_Operator_t *)ctx;

			if(grop->wRes != CAUSE_ZERO)
			{
				pocIdtAttr.pocBuildGroupCb(0);
				pocIdtAttr.pocBuildGroupCb = NULL;
				break;
			}

			for(unsigned long i = 0; i < g_data.dwNum; i++)
			{
				if(0 != IDT_GAddU(LV_POC_IDT_DWSN_QUERY_GROUPADDUSER, grop->pGroup.ucNum, &g_data.member[i]))
				{
					OSI_PRINTFI("[buildgroup](%s)(%d):build group add member failed", __func__, __LINE__);
				}
			}
			if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF, NULL))
			{
				osiTimerStartRelaxed(pocIdtAttr.get_group_list_timer, 1000, OSI_WAIT_FOREVER);
				return;
			}
			OSI_PRINTFI("[buildgroup](%s)(%d):rep server build group msg, group_num[%s]", __func__, __LINE__, grop->pGroup.ucNum);
			pocIdtAttr.pocBuildGroupCb(1);
			pocIdtAttr.pocBuildGroupCb = NULL;

			//save self_build group
			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
			poc_config->is_exist_selfgroup = LVPOCGROUPIDTCOM_SIGNAL_SELF_EXIST;
			strcpy((char *)poc_config->selfbuild_group_num, (char *)grop->pGroup.ucNum);
		    lv_poc_setting_conf_write();

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			pocIdtAttr.pocBuildGroupCb = (poc_build_group_cb)ctx;
			osiTimerStart(pocIdtAttr.check_ack_timeout_timer, 3000);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND:
		{
			OSI_PRINTFI("[buildgroup](%s)(%d):cannel build group cb", __func__, __LINE__);
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
				if(pocIdtAttr.pPocMemberList->dwNum < 1
					|| pocIdtAttr.loginstatus_t == LVPOCLEDIDTCOM_SIGNAL_LOGIN_FAILED)
				{
					pocIdtAttr.pocGetMemberListCb(0, 0, NULL);
				}
				else
				{
					pocIdtAttr.pocGetMemberListCb(1, pocIdtAttr.pPocMemberList->dwNum, pocIdtAttr.pPocMemberList);
				}
				pocIdtAttr.pocGetMemberListCb = NULL;
				break;
			}
			else
			{
				OSI_PRINTFI("[memberlist](%s)(%d):query memberlist ind", __func__, __LINE__);
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
	        pocIdtAttr.isPocMemberListBuf = true;
	        Func_GQueryU(pocIdtAttr.query_group, m_IdtUser.m_Group.m_Group[pocIdtAttr.query_group].m_ucGNum);
			OSI_PRINTFI("[memberlist](%s)(%d):query group member to send msg", __func__, __LINE__);
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
			osiTimerStart(pocIdtAttr.check_ack_timeout_timer, 3000);
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
			lv_task_t *oncetask = lv_task_create(lv_poc_activity_func_cb_set.member_list.group_member_act, 50, LV_TASK_PRIO_HIGHEST, (void *)group_info->m_ucGName);
			lv_task_once(oncetask);

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
			else if(index == pocIdtAttr.current_group)//已在群组
			{
				pocIdtAttr.pocSetCurrentGroupCb(2);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
			}
			else//切组成功
			{
				pocIdtAttr.current_group = index;
				nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
		        strcpy((char *)poc_config->old_account_current_group, (const char *)m_IdtUser.m_Group.m_Group[index].m_ucGNum);
		        lv_poc_setting_conf_write();
				pocIdtAttr.pocSetCurrentGroupCb(1);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
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
			OSI_PRINTFI("[idtlockgroup](%s)(%d):query user info", __func__, __LINE__);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP:
		{
			if(ctx == 0 || !poc_get_lcd_status())
			{
				break;
			}
			LvPocGuiIdtCom_User_Operator_t * UOpt = (LvPocGuiIdtCom_User_Operator_t *)ctx;

			Msg_GData_s *pPocMemberList = pocIdtAttr.pPocMemberList;
			for(unsigned int i = 0; i < pPocMemberList->dwNum; i++)
			{
				if(strcmp((const char *)pPocMemberList->member[i].ucNum, (const char *)UOpt->pUser.ucNum) == 0)
				{
					strcpy((char *)pPocMemberList->member[i].ucName, (char *)UOpt->pUser.ucName);
					strcpy((char *)pPocMemberList->member[i].ucNum, (char *)UOpt->pUser.ucNum);
					pPocMemberList->member[i].ucPrio = UOpt->pUser.ucPriority;
					pPocMemberList->member[i].ucAttr = UOpt->pUser.ucAttr;
					pPocMemberList->member[i].ucChanNum = UOpt->pUser.ucChanNum;
					pPocMemberList->member[i].ucStatus = UOpt->pUser.ucStatus;//用户状态
					break;
				}
			}

			pPocMemberList = pocIdtAttr.pPocMemberListBuf;
			for(unsigned int i = 0; i < pPocMemberList->dwNum; i++)
			{
				if(strcmp((const char *)pPocMemberList->member[i].ucNum, (const char *)UOpt->pUser.ucNum) == 0)
				{
					strcpy((char *)pPocMemberList->member[i].ucName, (char *)UOpt->pUser.ucName);
					strcpy((char *)pPocMemberList->member[i].ucNum, (char *)UOpt->pUser.ucNum);
					pPocMemberList->member[i].ucPrio = UOpt->pUser.ucPriority;
					pPocMemberList->member[i].ucAttr = UOpt->pUser.ucAttr;
					pPocMemberList->member[i].ucChanNum = UOpt->pUser.ucChanNum;
					pPocMemberList->member[i].ucStatus = UOpt->pUser.ucStatus;//用户状态
					break;
				}
			}

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

			if(UOpt->haveUser == true && strcmp((const char *)UOpt->pUser.ucNum, (const char *)pocIdtAttr.self_info.ucNum) == 0)
			{
				cJSON * workinfo_cjson = cJSON_Parse((const char *)UOpt->pUser.ucWorkInfo);
				do
				{
					if(workinfo_cjson == NULL || m_IdtUser.m_Group.m_Group_Num < 1)
					{
						static int check_self_workinfo_count = 0;
						if(check_self_workinfo_count < 5)
						{
							osiTimerStop(pocIdtAttr.get_lock_group_status_timer);
							osiTimerStartRelaxed(pocIdtAttr.get_lock_group_status_timer, 2000, OSI_WAIT_FOREVER);
							check_self_workinfo_count++;
							OSI_PRINTFI("[idtlockgroup](%s)(%d):don't parse lock group cjson string, try again in 2s", __func__, __LINE__);
						}
						else
						{
							osiTimerStop(pocIdtAttr.get_lock_group_status_timer);
							osiTimerStartRelaxed(pocIdtAttr.get_lock_group_status_timer, 1000 * 60 * 1, OSI_WAIT_FOREVER);
							check_self_workinfo_count = 0;
							OSI_PRINTFI("[idtlockgroup](%s)(%d):don't parse lock group cjson string, try again in 1min", __func__, __LINE__);
						}
						break;
					}

					char *lock_group_num = cJSON_GetObjectItem(workinfo_cjson, "LG")->valuestring;

					if(lock_group_num != NULL)
					{
						OSI_PRINTFI("[idtlockgroup](%s)(%d):current lock group info:[%s]", __func__, __LINE__, strcmp(lock_group_num, "#") == 0?"no lock group":"exist lock group");
					}

					if(lock_group_num == NULL || strcmp((const char *)lock_group_num, (const char *)"#") == 0)
					{
						pocIdtAttr.isLockGroupStatus = false;//未锁组
						OSI_PRINTFI("[idtlockgroup](%s)(%d):lock_group_num == NULL, current no lock group", __func__, __LINE__);
					}
					else
					{
						bool checked_lock_group = false;
						strcpy((char *)pocIdtAttr.pLockGroup->m_ucGNum, (const char *)lock_group_num);
						for(unsigned int i = 0; i < m_IdtUser.m_Group.m_Group_Num; i++)
						{
							if(strcmp((const char *)lock_group_num, (const char *)m_IdtUser.m_Group.m_Group[i].m_ucGNum) == 0)
							{
								strcpy((char *)pocIdtAttr.pLockGroup->m_ucGName, (const char *)m_IdtUser.m_Group.m_Group[i].m_ucGName);
								pocIdtAttr.pLockGroup->m_ucPriority = m_IdtUser.m_Group.m_Group[i].m_ucPriority;
								pocIdtAttr.current_group = i;
								checked_lock_group = true;
								break;
							}
						}

						if(!checked_lock_group)
						{
							IDT_SetGMemberExtInfo(0, NULL, (UCHAR *)"#", NULL);
							pocIdtAttr.current_group = 0;
							pocIdtAttr.isLockGroupStatus = false;

							lv_poc_activity_func_cb_set.group_list.lock_group(NULL, LV_POC_GROUP_OPRATOR_TYPE_UNLOCK);
							OSI_PRINTFI("[idtlockgroup](%s)(%d):don't find group[%s], unlock group", __func__, __LINE__, lock_group_num);
						}
						else
						{
							pocIdtAttr.isLockGroupStatus = true;
							lv_poc_activity_func_cb_set.group_list.lock_group(NULL, LV_POC_GROUP_OPRATOR_TYPE_LOCK);

							nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
							strcpy((char *)poc_config->old_account_current_group, lock_group_num);
					        lv_poc_setting_conf_write();
							lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
							OSI_PRINTFI("[idtlockgroup](%s)(%d):find group[%s], lock group", __func__, __LINE__, lock_group_num);
						}
					}

					cJSON_Delete(workinfo_cjson);
				}while(0);
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
#if POC_AUDIO_MODE_PIPE
			pocAudioPipeStop();
#else
			pocAudioPlayerStop(pocIdtAttr.player);
#endif
			audevStopPlay();
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
#if POC_AUDIO_MODE_PIPE
			pocAudioPipeStart();
#else
			pocAudioPlayerStart(pocIdtAttr.player);
#endif
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
			if(m_IdtUser.m_status < UT_STATUS_ONLINE || m_IdtUser.m_status < USER_OPRATOR_START_SPEAK || m_IdtUser.m_status > USER_OPRATOR_SPEAKING)
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

		    if(member_call_config->enable && member_call_obj == NULL)
		    {
			    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"错误参数", NULL);
				member_call_config->func(2, 2);
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
						    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"请稍后再试", NULL);
							member_call_config->func(2, 2);
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

						if(m_IdtUser.m_iCallId != -1)
						{
						    pocIdtAttr.pocMemberCallCb = member_call_config->func;
					    }
					    else
					    {
						    member_call_config->func(1, 1);
					    }
					}
					else
					{
						pocIdtAttr.is_member_call = true;
						member_call_config->func(0, 0);
					}
			    }
			    else
			    {
			        bool is_member_call = pocIdtAttr.is_member_call;
					pocIdtAttr.is_member_call = false;
					pocIdtAttr.member_call_dir = 0;
					if(is_member_call && m_IdtUser.m_iCallId != -1 && 0 == IDT_CallRel(m_IdtUser.m_iCallId, NULL, CAUSE_ZERO))
					{
						m_IdtUser.m_iCallId = -1;
					}

					member_call_config->func(1, 1);
			    }
		    }while(0);
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

			OSI_LOGI(0, "poc_check_member_call close LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_REP");

			if(ctx != CAUSE_ZERO)
			{
				//单呼错误提示
				lvPocGuiIdtCase_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_SPEAK_CALL_CASE, (void *)ctx, LvPocGetCauseStr(ctx));
			}
			else
			{
				poc_play_voice_one_time(LVPOCAUDIO_Type_Exit_Member_Call, 30, true);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"退出单呼", NULL);
			}
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
			pocIdtAttr.is_check_listen = false;

			/*恢复run闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");

			if((pocIdtAttr.membercall_count > 1 && pocIdtAttr.is_member_call == true) || pocIdtAttr.is_member_call == false)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)"停止聆听", (const uint8_t *)"");
				poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			}

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
			//monitor idle, goto location
			osiTimerStart(pocIdtAttr.callidle_timer, 10000);//10S
#endif
			OSI_PRINTFI("[poc][idtlisten]%s(%d):stop listen rep and destory idle,windows", __func__, __LINE__);

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
			//开始闪烁
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

			OSI_PRINTFI("[poc][idtlisten]%s(%d):start listen_speaker rep and display idle,windows", __func__, __LINE__);

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
			GU_STATUSGINFO_s *pStatus = (GU_STATUSGINFO_s *)ctx;

			Msg_GROUP_MEMBER_s *member = NULL;
			CGroup *group = NULL;
			bool isUpdateGroup = false;
			bool isUpdateMemberList = false;
		    for (int i = 0; i < pStatus->usNum; i++)
		    {
			    if (pStatus->stStatus[i].ucType == 1)
				{
					member = NULL;
					for(unsigned long k = 0; k < pocIdtAttr.pPocMemberList->dwNum; k++)
					{
						if(0 == strcmp((const char *)pStatus->stStatus[i].ucNum, (const char *)pocIdtAttr.pPocMemberList->member[k].ucNum))
						{
							member = &pocIdtAttr.pPocMemberList->member[k];
							if(member->ucStatus == pStatus->stStatus[i].Status.ucStatus)
							{
								member = NULL;/*状态相同不处理*/
							}
							else
							{
								if(pStatus->stStatus[i].Status.ucStatus == UT_STATUS_ONLINE)
								{
									#if GUIIDTCOM_MEMBERREFR_DEBUG_LOG
									char cOutstr[256] = {0};
						    		cOutstr[0] = '\0';
						    		sprintf(cOutstr, "[memberrefr]%s(%d):member online", __func__, __LINE__);
						    		OSI_LOGI(0, cOutstr);
									#endif
									member->ucStatus = UT_STATUS_ONLINE;
								}
								else if(pStatus->stStatus[i].Status.ucStatus == UT_STATUS_OFFLINE)
								{
									#if GUIIDTCOM_MEMBERREFR_DEBUG_LOG
									char cOutstr[256] = {0};
						    		cOutstr[0] = '\0';
						    		sprintf(cOutstr, "[memberrefr]%s(%d):member offline", __func__, __LINE__);
						    		OSI_LOGI(0, cOutstr);
									#endif
									member->ucStatus = UT_STATUS_OFFLINE;
								}
								else
								{
									member = NULL;/*状态相同不处理*/
								}
							}
							break;
						}
					}

					if(member != NULL)
					{
						#if GUIIDTCOM_MEMBERREFR_DEBUG_LOG
						char cOutstr[256] = {0};
			    		cOutstr[0] = '\0';
			    		sprintf(cOutstr, "[memberrefr]%s(%d):to set member", __func__, __LINE__);
			    		OSI_LOGI(0, cOutstr);
						#endif
						isUpdateMemberList = true;
						lv_poc_activity_func_cb_set.member_list.set_state(NULL, (const char *)member->ucName, (void *)member, member->ucStatus);
					}
				}
				else if (pStatus->stStatus[i].ucType == 2)
				{
					isUpdateGroup = true;
					group = NULL;
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
						if(isExist)/*组存在*/
						{
							#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_BUILDGROUP_DEBUG_LOG
							char cOutstr[256] = {0};
				    		cOutstr[0] = '\0';
				    		sprintf(cOutstr, "[buildgroup][grouprefr]%s(%d):group exist", __func__, __LINE__);
				    		OSI_LOGI(0, cOutstr);
							#endif
							isUpdateGroup = false;
							isUpdateMemberList = false;
						}
						else if (!isExist)/*组不存在*/
						{
							#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_BUILDGROUP_DEBUG_LOG
							char cOutstr[256] = {0};
				    		cOutstr[0] = '\0';
				    		sprintf(cOutstr, "[buildgroup][grouprefr]%s(%d):group isn't exist", __func__, __LINE__);
				    		OSI_LOGI(0, cOutstr);
							#endif
							isUpdateGroup = true;
							isUpdateMemberList = true;
						}
					}
					else
					{
						#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_BUILDGROUP_DEBUG_LOG
						char cOutstr[256] = {0};
			    		cOutstr[0] = '\0';
			    		sprintf(cOutstr, "[buildgroup][grouprefr]%s(%d):group NULL", __func__, __LINE__);
			    		OSI_LOGI(0, cOutstr);
						#endif
					}
				}
		    }
		    free(pStatus);

		    if(isUpdateGroup)
		    {
				#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
				char cOutstr[256] = {0};
	    		cOutstr[0] = '\0';
	    		sprintf(cOutstr, "[buildgroup][grouprefr]%s(%d):group have update", __func__, __LINE__);
	    		OSI_LOGI(0, cOutstr);
				#endif
			    osiTimerStop(pocIdtAttr.get_group_list_timer);
			    osiTimerStartRelaxed(pocIdtAttr.get_group_list_timer, 100, OSI_WAIT_FOREVER);
		    }

		    if(isUpdateMemberList)
		    {
				#if GUIIDTCOM_MEMBERREFR_DEBUG_LOG
				char cOutstr[256] = {0};
	    		cOutstr[0] = '\0';
	    		sprintf(cOutstr, "[buildgroup][memberrefr]%s(%d):member have update", __func__, __LINE__);
	    		OSI_LOGI(0, cOutstr);
				#endif
			    lv_poc_activity_func_cb_set.member_list.refresh(NULL);
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

			if (OPT_G_QUERYUSER == grop->dwOptCode)//查询组成员
			{
				if (CAUSE_ZERO != grop->wRes || grop->pGroup.ucNum == 0)/*error optcode:grop->pGroup.ucNum = 0*/
				{
					return;
				}

				if(pocIdtAttr.pPocMemberList != NULL
					&& pocIdtAttr.pPocMemberListBuf != NULL)
				{
					Msg_GData_s *pPocMemberList = NULL;
					if(pocIdtAttr.isPocMemberListBuf)
					{
						pPocMemberList = pocIdtAttr.pPocMemberListBuf;
					}
					else
					{
						pPocMemberList = pocIdtAttr.pPocMemberList;//上电填充成员列表
					}

					pPocMemberList->dwNum = grop->pGroup.dwNum;
					if(grop->pGroup.ucNum != 0)//抛弃群组号码为0的成员个数
					{
						pocIdtAttr.current_group_member_dwnum = pPocMemberList->dwNum;/*组用户个数*/
					}

					for(unsigned long i = 0; i < pPocMemberList->dwNum; i++)
					{
						strcpy((char *)pPocMemberList->member[i].ucName, (char *)grop->pGroup.member[i].ucName);
						strcpy((char *)pPocMemberList->member[i].ucNum, (char *)grop->pGroup.member[i].ucNum);
						strcpy((char *)pPocMemberList->member[i].ucAGNum, (char *)grop->pGroup.member[i].ucAGNum);
						pPocMemberList->member[i].ucPrio = grop->pGroup.member[i].ucPrio;
						pPocMemberList->member[i].ucUTType = grop->pGroup.member[i].ucUTType;
						pPocMemberList->member[i].ucAttr = grop->pGroup.member[i].ucAttr;
						pPocMemberList->member[i].ucChanNum = grop->pGroup.member[i].ucChanNum;
						pPocMemberList->member[i].ucFGCount = grop->pGroup.member[i].ucFGCount;
						pPocMemberList->member[i].ucStatus = grop->pGroup.member[i].ucStatus;//用户状态
					}

					if(pocIdtAttr.isPocMemberListBuf)
					{
						lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_REP, NULL);
						pocIdtAttr.isPocMemberListBuf = false;

						#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
						char cOutstr[128] = {0};
			    		cOutstr[0] = '\0';
			    		sprintf(cOutstr, "[grouprefr]%s(%d):query G_USER", __func__, __LINE__);
			    		OSI_LOGI(0, cOutstr);
						#endif
					}
					lv_poc_activity_func_cb_set.member_list.refresh_with_data(NULL);

					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
					char cOutstr1[128] = {0};
		    		cOutstr1[0] = '\0';
		    		sprintf(cOutstr1, "[grouprefr][server]%s(%d):refresh group member", __func__, __LINE__);
		    		OSI_LOGI(0, cOutstr1);
					#endif
				}
			}
			else if (OPT_G_ADD == grop->dwOptCode)
			{
				#if GUIIDTCOM_BUILDGROUP_DEBUG_LOG
				OSI_LOGI(0, "[grouprefr][buildgroup]OPT_G_ADD opt");
				#endif
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_REP, grop);
			}
			else if (OPT_G_MODIFY == grop->dwOptCode)//获取组的更新信息
			{
				bool checked_current = false;

				if(m_IdtUser.m_Group.m_Group_Num == 0) return;

				nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

				for(unsigned long i = 0; i < m_IdtUser.m_Group.m_Group_Num; i++)
				{
					if(strcmp((char *)m_IdtUser.m_Group.m_Group[i].m_ucGNum, (char *)grop->pGroup.ucNum) == 0)//寻找到修改的组号码
					{
						//替换所属组号码的组名称
						strcpy((char*)m_IdtUser.m_Group.m_Group[i].m_ucGName, (char*)grop->pGroup.ucName);
						//查找是否是当前群组
						if(!checked_current)
						{
							if(strlen((const char *)poc_config->old_account_current_group) > 0)
							{
								if(strcmp((const char *)poc_config->old_account_current_group, (const char *)grop->pGroup.member[1].ucNum) == 0)
								{
									pocIdtAttr.current_group = 1;
									lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
									checked_current = true;
								}
							}
						}
						break;//退出查找
					}
				}

				#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
				char cOutstr[128] = {0};
				cOutstr[0] = '\0';
				sprintf(cOutstr, "[grouprefr]%s(%d):opt G_MODIFY, refresh grouplist", __func__, __LINE__);
				OSI_LOGI(0, cOutstr);
				#endif

				lv_poc_activity_func_cb_set.group_list.refresh_with_data(NULL);
			}
			else if (OPT_U_QUERYGROUP == grop->dwOptCode)//用户归属组
			{
				#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
				char cOutstr[128] = {0};
	    		cOutstr[0] = '\0';
	    		sprintf(cOutstr, "[grouprefr]%s(%d):opt query user_group", __func__, __LINE__);
	    		OSI_LOGI(0, cOutstr);
				#endif

			    m_IdtUser.m_Group.Reset();
			    m_IdtUser.m_Group.m_Group_Num = grop->pGroup.dwNum;//有几个组
				pocIdtAttr.current_group_member_dwnum = grop->pGroup.dwNum;//组用户个数
			    bool checked_current = false;
				nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

				for (unsigned long i = 0; i < grop->pGroup.dwNum; i++)
			    {
			        strcpy((char*)m_IdtUser.m_Group.m_Group[i].m_ucGNum, (char*)grop->pGroup.member[i].ucNum);
			        strcpy((char*)m_IdtUser.m_Group.m_Group[i].m_ucGName, (char*)grop->pGroup.member[i].ucName);
			        m_IdtUser.m_Group.m_Group[i].m_ucPriority = grop->pGroup.member[i].ucPrio;

			        if(!checked_current)
			        {
				        if(strlen((const char *)poc_config->old_account_current_group) > 0)
				        {
					        if(strcmp((const char *)poc_config->old_account_current_group, (const char *)grop->pGroup.member[i].ucNum) == 0)
					        {
						        pocIdtAttr.current_group = i;
						        lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
						        checked_current = true;
					        }
				        }
				        else
				        {
					        strcpy((char *)poc_config->old_account_current_group, (const char *)grop->pGroup.member[i].ucNum);
							lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
					        lv_poc_setting_conf_write();
					        pocIdtAttr.current_group = i;
					        checked_current = true;
				        }
			        }
			    }

			    if(!checked_current)
			    {
				    pocIdtAttr.current_group = 0;
			        strcpy((char *)poc_config->old_account_current_group, (const char *)grop->pGroup.member[pocIdtAttr.current_group].ucNum);
			        lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, m_IdtUser.m_Group.m_Group[pocIdtAttr.current_group].m_ucGName);
			        lv_poc_setting_conf_write();
			        checked_current = true;

					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
					char cOutstr1[128] = {0};
		    		cOutstr1[0] = '\0';
		    		sprintf(cOutstr1, "[grouprefr]%s(%d):query no current group, 0 index is current group", __func__, __LINE__);
		    		OSI_LOGI(0, cOutstr1);
					#endif
			    }

				bool isRefreshGroupList = true;
			    if(pocIdtAttr.pocDeleteGroupcb != NULL)
			    {
				    pocIdtAttr.pocDeleteGroupcb(0);
				    pocIdtAttr.pocDeleteGroupcb = NULL;
				    isRefreshGroupList = false;
				    lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_REP, NULL);

					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_IDTGROUPLISTDEL_DEBUG_LOG
					char cOutstr2[128] = {0};
		    		cOutstr2[0] = '\0';
		    		sprintf(cOutstr2, "[grouprefr][idtgroupdel]%s(%d):delete group_cb, send msg (delete_group_rep)", __func__, __LINE__);
		    		OSI_LOGI(0, cOutstr2);
					#endif
			    }
				if(isRefreshGroupList)
				{
					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_IDTGROUPLISTDEL_DEBUG_LOG
					char cOutstr3[128] = {0};
		    		cOutstr3[0] = '\0';
		    		sprintf(cOutstr3, "[grouprefr][idtgroupdel]%s(%d):refresh grouplist", __func__, __LINE__);
		    		OSI_LOGI(0, cOutstr3);
					#endif

				    lv_poc_activity_func_cb_set.group_list.refresh_with_data(NULL);
			    }

			    if(!pocIdtAttr.isPocMemberListBuf)
			    {
					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
					char cOutstr4[128] = {0};
		    		cOutstr4[0] = '\0';
		    		sprintf(cOutstr4, "[grouprefr]%s(%d):get memberlist cur_group", __func__, __LINE__);
		    		OSI_LOGI(0, cOutstr4);
					#endif

					lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP, NULL);
				}
				else
				{
					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
					char cOutstr5[128] = {0};
		    		cOutstr5[0] = '\0';
		    		sprintf(cOutstr5, "[grouprefr]%s(%d):start get_memberlist_timer", __func__, __LINE__);
		    		OSI_LOGI(0, cOutstr5);
					#endif

					osiTimerStartRelaxed(pocIdtAttr.get_member_list_timer, 1000, OSI_WAIT_FOREVER);
				}
			}
			else if (OPT_G_DEL == grop->dwOptCode)//删除组
			{
				#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_IDTGROUPLISTDEL_DEBUG_LOG
				char cOutstr6[128] = {0};
	    		cOutstr6[0] = '\0';
	    		sprintf(cOutstr6, "[grouprefr][idtgroupdel]%s(%d):opt G_DEL", __func__, __LINE__);
	    		OSI_LOGI(0, cOutstr6);
				#endif

				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_REP, grop);
			}
			else if(OPT_G_ADDUSER == grop->dwOptCode
					|| OPT_G_DELUSER == grop->dwOptCode)
			{
				if(pocIdtAttr.optGUadd_timer != NULL)
				{
					if(true == osiTimerIsRunning(pocIdtAttr.optGUadd_timer))
					{
						osiTimerStop(pocIdtAttr.optGUadd_timer);
					}
					osiTimerStart(pocIdtAttr.optGUadd_timer, 1000);
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

static void prvPocGuiIdtTaskHandleReleaseListenTimer(uint32_t id, uint32_t ctx)
{
	if(pocIdtAttr.delay_close_listen_timer == NULL)
	{
		return;
	}

	osiTimerDelete(pocIdtAttr.delay_close_listen_timer);
	pocIdtAttr.delay_close_listen_timer = NULL;
}

#if GUIIDTCOM_LOCK_FUNC
static void prvPocGuiIdtTaskHandleLockGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_IND:
		{
			if(ctx == 0 || pocIdtAttr.pLockGroup == NULL) return;
			LvPocGuiIdtCom_lock_group_t *msg = (LvPocGuiIdtCom_lock_group_t *)ctx;
			if(msg->opt != LV_POC_GROUP_OPRATOR_TYPE_LOCK || msg->group_info == NULL)
			{
				break;
			}

			if(pocIdtAttr.lockGroupOpt > LV_POC_GROUP_OPRATOR_TYPE_NONE && pocIdtAttr.lockGroupOpt != LV_POC_GROUP_OPRATOR_TYPE_LOCK_FAILED)
			{
				break;
			}

			pocIdtAttr.lockGroupOpt = LV_POC_GROUP_OPRATOR_TYPE_LOCK;
			pocIdtAttr.pocLockGroupCb = msg->cb;
			CGroup * group_info = (CGroup *)msg->group_info;
			pocIdtAttr.LockGroupTemp.m_ucPriority = group_info->m_ucPriority;
			strcpy((char *)pocIdtAttr.LockGroupTemp.m_ucGName, (const char *)group_info->m_ucGName);
			strcpy((char *)pocIdtAttr.LockGroupTemp.m_ucGNum, (const char *)group_info->m_ucGNum);
			IDT_SetGMemberExtInfo(0, NULL, (UCHAR *)group_info->m_ucGNum, NULL);

			#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[idtlockgroup]%s(%d):lock group ind, send msg to server", __func__, __LINE__);
			OSI_LOGI(0, cOutstr);
			#endif

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_REP:
		{
			if(pocIdtAttr.lockGroupOpt != LV_POC_GROUP_OPRATOR_TYPE_LOCK)
			{
				break;
			}

			lv_poc_group_oprator_type opt = LV_POC_GROUP_OPRATOR_TYPE_LOCK_FAILED;
			if(ctx == CAUSE_ZERO)/*锁组操作是否成功*/
			{
				CGroup * p_group = NULL;
				unsigned long i = 0;
				/*校验本地组号码是否存在*/
				for (i = 0; i < m_IdtUser.m_Group.m_Group_Num; i++)
				{
					p_group = (CGroup *)&m_IdtUser.m_Group.m_Group[i];/*服务器获取*/
					if(strcmp((const char *)pocIdtAttr.LockGroupTemp.m_ucGNum, (const char *)p_group->m_ucGNum) == 0)
					{
						pocIdtAttr.current_group = i;
						strcpy((char *)pocIdtAttr.pLockGroup->m_ucGNum, (const char *)p_group->m_ucGNum);
						strcpy((char *)pocIdtAttr.pLockGroup->m_ucGName, (const char *)p_group->m_ucGName);
						pocIdtAttr.pLockGroup->m_ucPriority = p_group->m_ucPriority;
						lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocIdtAttr.self_info.ucName, p_group->m_ucGName);
						break;
					}
				}

				if(i < m_IdtUser.m_Group.m_Group_Num)
				{
					pocIdtAttr.isLockGroupStatus = true;
					opt = LV_POC_GROUP_OPRATOR_TYPE_LOCK_OK;
					if(pocIdtAttr.pocLockGroupCb != NULL)
					{
						#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG
						char cOutstr3[128] = {0};
						cOutstr3[0] = '\0';
						sprintf(cOutstr3, "[idtlockgroup]%s(%d):lock success->start cb", __func__, __LINE__);
						OSI_LOGI(0, cOutstr3);
						#endif

						pocIdtAttr.pocLockGroupCb(opt);
					}

					pocIdtAttr.pocLockGroupCb = NULL;
					pocIdtAttr.lockGroupOpt = LV_POC_GROUP_OPRATOR_TYPE_NONE;
				}
				else
				{
					pocIdtAttr.lockGroupOpt = opt;
					pocIdtAttr.pocLockGroupCb = NULL;
					lv_poc_set_lock_group(LV_POC_GROUP_OPRATOR_TYPE_UNLOCK, NULL, NULL);

					#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG
					char cOutstr1[128] = {0};
					cOutstr1[0] = '\0';
					sprintf(cOutstr1, "[idtlockgroup]%s(%d):lock failed->no group", __func__, __LINE__);
					OSI_LOGI(0, cOutstr1);
					#endif
				}
			}
			else/*锁组失败*/
			{
				pocIdtAttr.lockGroupOpt = opt;
				pocIdtAttr.pocLockGroupCb = NULL;
				lv_poc_set_lock_group(LV_POC_GROUP_OPRATOR_TYPE_UNLOCK, NULL, NULL);

				#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG
				char cOutstr2[128] = {0};
				cOutstr2[0] = '\0';
				sprintf(cOutstr2, "[idtlockgroup]%s(%d):lock failed,opt code no correct", __func__, __LINE__);
				OSI_LOGI(0, cOutstr2);
				#endif
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_IND:
		{
			if(ctx == 0 || pocIdtAttr.pLockGroup == NULL) return;
			LvPocGuiIdtCom_lock_group_t *msg = (LvPocGuiIdtCom_lock_group_t *)ctx;
			if(msg->opt != LV_POC_GROUP_OPRATOR_TYPE_UNLOCK)
			{
				break;
			}

			if(pocIdtAttr.lockGroupOpt > LV_POC_GROUP_OPRATOR_TYPE_NONE && pocIdtAttr.lockGroupOpt != LV_POC_GROUP_OPRATOR_TYPE_UNLOCK_FAILED)
			{
				break;
			}

			pocIdtAttr.lockGroupOpt = LV_POC_GROUP_OPRATOR_TYPE_UNLOCK;
			pocIdtAttr.pocLockGroupCb = msg->cb;
			IDT_SetGMemberExtInfo(0, NULL, (UCHAR *)"#", NULL);//不锁定组

			#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG
			char cOutstr[128] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[idtlockgroup]%s(%d):unlock group ind, send msg to server", __func__, __LINE__);
			OSI_LOGI(0, cOutstr);
			#endif

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_REP:
		{
			if(pocIdtAttr.lockGroupOpt == LV_POC_GROUP_OPRATOR_TYPE_NONE)
			{
				break;
			}

			lv_poc_group_oprator_type opt = LV_POC_GROUP_OPRATOR_TYPE_NONE;

			if(ctx == CAUSE_ZERO)
			{
				opt = LV_POC_GROUP_OPRATOR_TYPE_UNLOCK_OK;
				pocIdtAttr.isLockGroupStatus = false;

				if(pocIdtAttr.pocLockGroupCb != NULL)
				{
					pocIdtAttr.pocLockGroupCb(opt);
					pocIdtAttr.pocLockGroupCb = NULL;

					#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG
					char cOutstr1[128] = {0};
					cOutstr1[0] = '\0';
					sprintf(cOutstr1, "[idtlockgroup]%s(%d):unlock success->start cb", __func__, __LINE__);
					OSI_LOGI(0, cOutstr1);
					#endif
				}

				pocIdtAttr.lockGroupOpt = LV_POC_GROUP_OPRATOR_TYPE_NONE;
				pocIdtAttr.pocLockGroupCb = NULL;
			}
			else
			{
				opt = LV_POC_GROUP_OPRATOR_TYPE_UNLOCK_FAILED;
				pocIdtAttr.pocLockGroupCb = NULL;
				pocIdtAttr.lockGroupOpt = opt;

				#if GUIIDTCOM_IDTLOCKGROUP_DEBUG_LOG
				char cOutstr1[128] = {0};
				cOutstr1[0] = '\0';
				sprintf(cOutstr1, "[idtlockgroup]%s(%d):unlock failed", __func__, __LINE__);
				OSI_LOGI(0, cOutstr1);
				#endif
			}

			break;
		}

		default:
			break;
	}
}
#endif

static void prvPocGuiIdtTaskHandleDeleteGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			LvPocGuiIdtCom_delete_group_t *del_group = (LvPocGuiIdtCom_delete_group_t *)ctx;
			if(del_group->cb == NULL || del_group->group_info == NULL)
			{
				if(del_group->cb != NULL)
				{
					del_group->cb(1);
				}
				break;
			}

			unsigned int i = 0;
			CGroup *group_info = (CGroup *)del_group->group_info;
			CGroup *current_group_info = (CGroup *)lvPocGuiIdtCom_get_current_group_info();

			//remain one group
			if(m_IdtUser.m_Group.m_Group_Num < 2 || GROUP_EQUATION(group_info->m_ucGName, current_group_info->m_ucGName, group_info, current_group_info, NULL))
			{
				del_group->cb(3);
				break;
			}

			for(i = 0; i < m_IdtUser.m_Group.m_Group_Num; i++)
			{
				if(strcmp((const char *)m_IdtUser.m_Group.m_Group[i].m_ucGNum,(const char *)group_info->m_ucGNum) == 0)
				{
					break;//不能删除自己当前组
				}
			}

			if(i >= m_IdtUser.m_Group.m_Group_Num)//未找到该组
			{
				del_group->cb(4);
				break;
			}

			pocIdtAttr.pocDeleteGroupcb = del_group->cb;
			if(IDT_GDel(LV_POC_IDT_DWSN_QUERY_DELETEGROUP, (UCHAR *)group_info->m_ucGNum) == -1)
			{
				pocIdtAttr.pocDeleteGroupcb(5);
				pocIdtAttr.pocDeleteGroupcb = NULL;
			}

			OSI_PRINTFI("[grouprefr][idtgroupdel]%s(%d):del group ind", __func__, __LINE__);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_REP:
		{
			if(ctx == 0)
			{
				break;
			}
			LvPocGuiIdtCom_Group_Operator_t *grop = (LvPocGuiIdtCom_Group_Operator_t *)ctx;

			if(grop->wRes != LV_POC_IDT_DWSN_QUERY_DELETEGROUP)
			{
				if(pocIdtAttr.pocDeleteGroupcb != NULL)//local poc delete group updater it
				{
					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_IDTGROUPLISTDEL_DEBUG_LOG
					char cOutstr[128] = {0};
		    		cOutstr[0] = '\0';
		    		sprintf(cOutstr, "[grouprefr][idtgroupdel]%s(%d):start del group cb, groupnum[%s]", __func__, __LINE__, grop->pGroup.ucNum);
		    		OSI_LOGI(0, cOutstr);
					#endif

					pocIdtAttr.pocDeleteGroupcb(grop->wRes);//0 success
					pocIdtAttr.pocDeleteGroupcb = NULL;
					//self del lock isn't exist
					if(0 == strcmp((char *)pocIdtAttr.pLockGroup->m_ucGNum, (char *)grop->pGroup.ucNum))//delete lock group
					{
						lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_IND, NULL);
					}
				}
				else//other poc delete group updater it
				{
					lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF, NULL);

					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
					char cOutstr2[256] = {0};
		    		cOutstr2[0] = '\0';
		    		sprintf(cOutstr2, "[idtgroupdel]%s(%d):lock group[%s], del group[%s]", __func__, __LINE__, pocIdtAttr.pLockGroup->m_ucGNum, grop->pGroup.ucNum);
		    		OSI_LOGI(0, cOutstr2);
					#endif

					if(0 == strcmp((char *)pocIdtAttr.pLockGroup->m_ucGNum, (char *)grop->pGroup.ucNum))//delete lock group
					{
						lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_IND, NULL);
					}
				}
				//check self_build group
				nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
				if(LVPOCGROUPIDTCOM_SIGNAL_SELF_NO != poc_config->is_exist_selfgroup
					&& (NULL != poc_config->selfbuild_group_num)
					&& (0 == strcmp((char *)poc_config->selfbuild_group_num, (char *)grop->pGroup.ucNum)))
				{
					poc_config->is_exist_selfgroup = LVPOCGROUPIDTCOM_SIGNAL_SELF_NO;
					strcpy((char *)poc_config->selfbuild_group_num, (char *)"");
			    	lv_poc_setting_conf_write();

					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
					char cOutstr2[256] = {0};
		    		cOutstr2[0] = '\0';
		    		sprintf(cOutstr2, "[idtgroupdel]%s(%d):del self_build group[%s]", __func__, __LINE__, grop->pGroup.ucNum);
		    		OSI_LOGI(0, cOutstr2);
					#endif
				}

				break;
			}

			#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_IDTGROUPLISTDEL_DEBUG_LOG
			char cOutstr[128] = {0};
    		cOutstr[0] = '\0';
    		sprintf(cOutstr, "[grouprefr][idtgroupdel]%s(%d):other del opt", __func__, __LINE__);
    		OSI_LOGI(0, cOutstr);
			#endif

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_IND:
		{
			#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_IDTGROUPLISTDEL_DEBUG_LOG
			char cOutstr[128] = {0};
    		cOutstr[0] = '\0';
    		sprintf(cOutstr, "[grouprefr][idtgroupdel]%s(%d):start unlock be deleted group", __func__, __LINE__);
    		OSI_LOGI(0, cOutstr);
			#endif

			pocIdtAttr.lockGroupOpt = LV_POC_GROUP_OPRATOR_TYPE_UNLOCK_BE_DELETED_GROUP;
			IDT_SetGMemberExtInfo(0, NULL, (UCHAR *)"#", NULL);//解锁被删除的组
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_REP:
		{
			pocIdtAttr.lockGroupOpt = LV_POC_GROUP_OPRATOR_TYPE_NONE;
			pocIdtAttr.isLockGroupStatus = false;
			pocIdtAttr.pocDeleteGroupcb = NULL;

			strcpy((char *)pocIdtAttr.pLockGroup->m_ucGNum, (const char *)"");
			strcpy((char *)pocIdtAttr.pLockGroup->m_ucGName, (const char *)"");
			pocIdtAttr.pLockGroup->m_ucPriority = 0;

			#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_IDTGROUPLISTDEL_DEBUG_LOG
			char cOutstr[128] = {0};
    		cOutstr[0] = '\0';
    		sprintf(cOutstr, "[grouprefr][idtgroupdel]%s(%d):be deleted group, have unlocked success", __func__, __LINE__);
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

static void prvPocGuiIdtTaskHandleOther(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_DELAY_IND:
		{
			if(ctx < 1)
			{
				break;
			}

			osiThreadSleepRelaxed(ctx, OSI_WAIT_FOREVER);
		}

		case LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP:
		{
			pocIdtAttr.query_group = pocIdtAttr.current_group;
			Func_GQueryU(pocIdtAttr.query_group, NULL);
			OSI_PRINTFI("[grouprefr](%s)(%d):query current group", __func__, __LINE__);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF:
		{
			pocIdtAttr.isPocGroupListAll = false;
			IDT_UQueryG(0, pocIdtAttr.self_info.ucNum);
			OSI_PRINTFI("[grouprefr](%s)(%d):query include_self_group", __func__, __LINE__);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND:
		{
			if(ctx == 0)
			{
				OSI_PRINTFI("[idtlockgroup](%s)(%d):get_lock_group_status, start to query user info ind", __func__, __LINE__);
				if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_IND, &pocIdtAttr.self_info))
				{
					osiTimerStop(pocIdtAttr.get_lock_group_status_timer);
					osiTimerStartRelaxed(pocIdtAttr.get_lock_group_status_timer, 1000, OSI_WAIT_FOREVER);
				}
			}
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SCREEN_ON_IND:
		{
			lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND, NULL);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SCREEN_OFF_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LOOPBACK_RECORDER_IND:
		{
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始录音", (const uint8_t *)"");
			lv_poc_start_recordwriter();
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LOOPBACK_PLAYER_IND:
		{
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始播放", (const uint8_t *)"");
			lv_poc_start_playfile();
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_TEST_VLOUM_PLAY_IND:
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Test_Volum, 50, false);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SET_SCREEN_STATUS_IND:
		{
			if(!poc_get_lcd_status())
			{
				lvGuiScreenOn();
				lvGuiUpdateLastActivityTime();
			}
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND:
		{
			lvPocGuiIdtCom_stop_check_ack();
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
			//add shutdown opt
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
			switch(ctx)
			{
				case CAUSE_ZERO://错误0
				{

					switch(pocIdtAttr.call_error_case)
					{
						case LV_POC_CALL_ERROR_GROUP_IS_BUSY:
						{
							lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
							lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)"呼叫释放", (const uint8_t *)"");
							pocIdtAttr.call_error_case = LV_POC_CALL_ERROR_NONE;
							break;
						}

						default:
						{
							break;
						}
					}

					break;
				}

				case CAUSE_U_OFFLINE_G://组中没有在线成员
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)cause_str, (const uint8_t *)"");

					break;
				}
				case CAUSE_U_LOCK_G://用户锁定在其他组
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)cause_str, (const uint8_t *)"");

					break;
				}
				case CAUSE_G_NOUSER://组中没有用户
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)cause_str, (const uint8_t *)"");

					break;
				}
				case CAUSE_MS_POWEROFF://用户关机
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)cause_str, (const uint8_t *)"");

					break;
				}
				case CAUSE_RESOURCE_UNAVAIL://资源不可用---用户忙线中
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)"用户忙线中", (const uint8_t *)"");

					break;
				}
				case CAUSE_TIMER_EXPIRY://定时器超时
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)"呼叫超时", (const uint8_t *)"");

					break;
				}
				case 18702://用户关机
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)"用户关机", (const uint8_t *)"");

					break;
				}
				case CAUSE_CALL_CONFLICT://呼叫冲突
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)"呼叫冲突", (const uint8_t *)"");

					break;
				}

				case 8462://上次呼叫存在--->释放呼叫
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)"呼叫释放", (const uint8_t *)"");

					break;
				}

				default:
				{

#if GUIIDTCOM_IDTWINDOWS_NOTE
					//此处可以显示所有异常状态,release版本需要关闭
					#if GUIIDTCOM_IDTERRORINFO_DEBUG_LOG
					char cOutstr[256] = {0};
					cOutstr[0] = '\0';
					sprintf(cOutstr, "[idterrorinfo]%s(%d):call error (%d)(%s)", __func__, __LINE__, (int)id, (uint8_t *)cause_str);
					OSI_LOGI(0, cOutstr);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG,(const uint8_t *)cause_str, (const uint8_t *)"");
					#endif
#endif
					break;
				}

			}

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
         GPS_DATA_s *pGps = (GPS_DATA_s* )ctx;
         if(pGps != NULL
		 	&& pocIdtAttr.loginstatus_t == LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS)
         {
            bool reportstatus = IDT_GpsReport(pGps, NULL);
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

static void prvPocGuiIdtTaskHandleRecorderSuspendorResumeOpt(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND:
		{
			if((osiThread_t *)lv_poc_recorder_Thread() != NULL)
			{
				OSI_LOGI(0, "[recorder]start suspend");
				osiThreadSuspend((osiThread_t *)lv_poc_recorder_Thread());
			}
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_RESUME_IND:
		{
			if((osiThread_t *)lv_poc_recorder_Thread() != NULL)
			{
				OSI_LOGI(0, "[recorder]start resume");
				osiThreadResume((osiThread_t *)lv_poc_recorder_Thread());
			}
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleHeadsetInsert(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUIIDTCOM_SIGNAL_HEADSET_INSERT:
      {
         lv_poc_set_headset_status(true);

		 if(!lvPocGuiIdtCom_get_listen_status()
		 	&& !lvPocGuiIdtCom_get_speak_status())
		 {
         	lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
         	lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"耳机插入", (const uint8_t *)"");
		 }
		 break;
      }
      case LVPOCGUIIDTCOM_SIGNAL_HEADSET_PULL_OUT:
      {
         lv_poc_set_headset_status(false);

		 if(!lvPocGuiIdtCom_get_listen_status()
		 	&& !lvPocGuiIdtCom_get_speak_status())
		 {
         	lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
         	lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"耳机拔出", (const uint8_t *)"");
		 }
		 break;
      }
      default:
         break;
   }
}

static void prvPocGuiIdtTaskHandleDelayOpenPA(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_DELAY_OPEN_PA_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			osiTimerStart(pocIdtAttr.openpa_timer, ctx);
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_DELAY_CLOSE_PA_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			osiTimerStart(pocIdtAttr.closepa_timer, ctx);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandlePingNet(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
	   case LVPOCGUIIDTCOM_SIGNAL_PING_SUCCESS_REP:
	   {
		  lv_poc_activity_func_cb_set.status_led(LVPOCGUIIDTCOM_SIGNAL_PING_SUCCESS_IND, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_200 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
		  break;
	   }

	   case LVPOCGUIIDTCOM_SIGNAL_PING_FAILED_REP:
	   {
		  lv_poc_activity_func_cb_set.status_led(LVPOCGUIIDTCOM_SIGNAL_PING_FAILED_IND, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_200 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_4);
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

    IDT_Entry(NULL);
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
#if GUIIDTCOM_LOCK_FUNC
			case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_REP:
			case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_REP:
			{
				prvPocGuiIdtTaskHandleLockGroup(event.param1, event.param2);
				break;
			}
#endif
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
			case LVPOCGUIIDTCOM_SIGNAL_LOOPBACK_RECORDER_IND:
			case LVPOCGUIIDTCOM_SIGNAL_LOOPBACK_PLAYER_IND:
			case LVPOCGUIIDTCOM_SIGNAL_TEST_VLOUM_PLAY_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SET_SCREEN_STATUS_IND:
			case LVPOCGUIIDTCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND:
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

			case LVPOCGUIIDTCOM_SIGNAL_HEADSET_INSERT:
            case LVPOCGUIIDTCOM_SIGNAL_HEADSET_PULL_OUT:
            {
            	prvPocGuiIdtTaskHandleHeadsetInsert(event.param1, event.param2);
				break;
			}

            case LVPOCGUIIDTCOM_SIGNAL_DELAY_OPEN_PA_IND:
			case LVPOCGUIIDTCOM_SIGNAL_DELAY_CLOSE_PA_IND:
            {
                prvPocGuiIdtTaskHandleDelayOpenPA(event.param1, event.param2);
                break;
         	}

			case LVPOCGUIIDTCOM_SIGNAL_PING_SUCCESS_REP:
			case LVPOCGUIIDTCOM_SIGNAL_PING_FAILED_REP:
			{
				prvPocGuiIdtTaskHandlePingNet(event.param1, event.param2);
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
	pocIdtAttr.mutex = osiMutexCreate();
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
	pocIdtAttr.optGUadd_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_optGUAdd_timer_cb, NULL);
	pocIdtAttr.openpa_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_open_pa_timer_cb, NULL);
	pocIdtAttr.closepa_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_close_pa_timer_cb, NULL);
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
	pocIdtAttr.callidle_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_call_idle_timer_cb, NULL);
#endif
	pocIdtAttr.check_ack_timeout_timer = osiTimerCreate(pocIdtAttr.thread, LvGuiIdtCom_check_ack_timeout_timer_cb, NULL);
}

static void lvPocGuiIdtCom_send_data_callback(uint8_t * data, uint32_t length)
{
    if (pocIdtAttr.recorder == 0 || m_IdtUser.m_iCallId == -1 || data == NULL || length < 1)
    {
	    return;
    }
    if(m_IdtUser.m_status != USER_OPRATOR_SPEAKING)
    {
	    return;
    }
    IDT_CallSendAuidoData(m_IdtUser.m_iCallId,
						    0,
						    0,
						    data,
						    length,
						    0);
    m_IdtUser.m_iTxCount = m_IdtUser.m_iTxCount + 1;
	OSI_LOGI(0, "[record][senddata](%d):data(%d), len(%d)", __LINE__, data, length);
}

extern "C" void lvPocGuiIdtCom_Init(void)
{
	memset(&pocIdtAttr, 0, sizeof(PocGuiIIdtComAttr_t));
	pocIdtAttr.membercall_count = 0;
	lv_poc_set_first_membercall(true);
	pocIdtAttr.is_release_call = true;
	pocIdtAttr.pPocMemberList = (Msg_GData_s *)malloc(sizeof(Msg_GData_s));
	pocIdtAttr.pPocMemberListBuf = (Msg_GData_s *)malloc(sizeof(Msg_GData_s));
	pocIdtAttr.pLockGroup = (CGroup *)malloc(sizeof(CGroup));
	if(pocIdtAttr.pLockGroup != NULL)
	{
		memset(pocIdtAttr.pLockGroup, 0, sizeof(CGroup));
	}
	lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_NOLOGIN);
	m_IdtUser.m_status = 0;
	pocGuiIdtComStart();
}

extern "C" void lvPocGuiIdtCom_stop_check_ack(void)
{
	if(pocIdtAttr.check_ack_timeout_timer != NULL)
	{
		osiTimerStop(pocIdtAttr.check_ack_timeout_timer);
	}
}

extern "C" bool lvPocGuiIdtCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx)
{
    if (pocIdtAttr.thread == NULL || (signal != LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND && pocIdtAttr.isReady == false))
    {
	    return false;
    }

    if(signal == LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND)
    {
	    IDT_Entry(NULL);
	    return true;
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

extern "C" void lvPocGuiComLogSwitch(bool status)
{
	if(status)
	{
		g_iLog = 1;
	}
	else
	{
		g_iLog = 0;
	}
}

#endif

