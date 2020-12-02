#ifndef _INCLUDE_GUIOemCOM_API_H_
#define _INCLUDE_GUIOemCOM_API_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../gui/lv_include/lv_poc_type.h"
#include "at_engine.h"

OSI_EXTERN_C_BEGIN
;
typedef enum
{
    LVPOCGUIIDTCOM_SIGNAL_START = (1 << 8) - 1,

    LVPOCGUIOEMCOM_SIGNAL_AP_POC_START,

    LVPOCGUIOEMCOM_SIGNAL_AP_POC_IND,
    LVPOCGUIOEMCOM_SIGNAL_AP_POC_REP,

    LVPOCGUIOEMCOM_SIGNAL_OPENPOC_IND,
    LVPOCGUIOEMCOM_SIGNAL_OPENPOC_REP,

    LVPOCGUIOEMCOM_SIGNAL_SETPOC_IND,
    LVPOCGUIOEMCOM_SIGNAL_SETPOC_REP,

    LVPOCGUIOEMCOM_SIGNAL_LOGIN_IND,
    LVPOCGUIOEMCOM_SIGNAL_LOGIN_REP,

    LVPOCGUIOEMCOM_SIGNAL_EXIT_IND,
    LVPOCGUIOEMCOM_SIGNAL_EXIT_REP,

    LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_IND,
    LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_REP,

    LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_IND,
    LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_REP,

    LVPOCGUIOEMCOM_SIGNAL_LISTEN_START_REP,
    LVPOCGUIOEMCOM_SIGNAL_LISTEN_STOP_REP,
    LVPOCGUIOEMCOM_SIGNAL_LISTEN_SPEAKER_REP,

    LVPOCGUIOEMCOM_SIGNAL_JOIN_GROUP_REP,
    LVPOCGUIOEMCOM_SIGNAL_GROUPLIST_DATA_REP,

    LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_INFO_IN,
    LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_INFO_REP,
    LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_DATA_REP,

    LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_IND,
    LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_REP,
    LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_UPDATE,
    LVPOCGUIOEMCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND,
    LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND,

    LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_IND,
    LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_REP,
    LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_UPDATE,
    LVPOCGUIOEMCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND,
    LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND,

    LVPOCGUIOEMCOM_SIGNAL_SET_CURRENT_GROUP_IND,
    LVPOCGUIOEMCOM_SIGNAL_SET_CURRENT_GROUP_REP,
    LVPOCGUIOEMCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND,
    LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND,

    LVPOCGUIOEMCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP,
    LVPOCGUIOEMCOM_SIGNAL_MEMBER_INFO_IND,

    LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_IND,
    LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP,
    LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP,

    LVPOCGUIOEMCOM_SIGNAL_MEMBER_GROUP_INFO_UPDATE,

    LVPOCGUIOEMCOM_SIGNAL_CALL_TALKING_ID_IND,

    LVPOCGUIOEMCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP,

    LVPOCGUIOEMCOM_SIGNAL_GET_MONITOR_GROUP_STATUS_IND,

    LVPOCGUIOEMCOM_SIGNAL_MONITOR_GROUP_IND,
    LVPOCGUIOEMCOM_SIGNAL_MONITOR_GROUP_REP,

    LVPOCGUIOEMCOM_SIGNAL_UNMONITOR_GROUP_IND,
    LVPOCGUIOEMCOM_SIGNAL_UNMONITOR_GROUP_REP,

    LVPOCGUIOEMCOM_SIGNAL_POWERON_CHECK_MONITOR_GROUP_REP,

    LVPOCGUIOEMCOM_SIGNAL_GPS_UPLOADING_IND,

    LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_REP_RECORD_IND,

    LVPOCGUIOEMCOM_SIGNAL_SCREEN_ON_IND,
    LVPOCGUIOEMCOM_SIGNAL_SCREEN_OFF_IND,

	LVPOCGUIOEMCOM_SIGNAL_STOP_PLAYER_VOICE,
    LVPOCGUIOEMCOM_SIGNAL_SET_STOP_PLAYER_TTS_VOICE,
    LVPOCGUIOEMCOM_SIGNAL_SET_START_PLAYER_TTS_VOICE,

	LVPOCGUIOEMCOM_SIGNAL_LOOPBACK_RECORDER_IND,
    LVPOCGUIOEMCOM_SIGNAL_LOOPBACK_PLAYER_IND,
    LVPOCGUIOEMCOM_SIGNAL_TEST_VLOUM_PLAY_IND,
	LVPOCGUIOEMCOM_SIGNAL_SET_SCREEN_STATUS_IND,
	LVPOCGUIOEMCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND,

} LvPocGuiOemCom_SignalType_t;

typedef enum{/*登陆状态*/

    LVPOCOEMCOM_SIGNAL_LOGIN_START = 0,

    LVPOCOEMCOM_SIGNAL_LOGIN_FAILED = 1 ,
    LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS = 2 ,
    LVPOCOEMCOM_SIGNAL_LOGIN_EXIT = 3 ,

    LVPOCOEMCOM_SIGNAL_LOGIN_END    ,
}LVPOCIDTCOM_LOGIN_STATUS_T;

typedef enum{/*群组*/

    LVPOCOEMCOM_SIGNAL_GROUP_START = 0,

    LVPOCOEMCOM_SIGNAL_GROUP_NONE = 1 ,
    LVPOCOEMCOM_SIGNAL_GROUP_JOIN = 2 ,
    LVPOCOEMCOM_SIGNAL_GROUP_EXIT = 3 ,

    LVPOCOEMCOM_SIGNAL_GROUP_END    ,
}LVPOCOEMCOM_GROUP_STATUS_T;

typedef enum{

    LVPOCOEMCOM_CALL_TYPE_START = 0,

    LVPOCOEMCOM_CALL_TYPE_SINGLE = 1 ,
    LVPOCOEMCOM_CALL_TYPE_GROUP = 2 ,

    LVPOCOEMCOM_CALL_TYPE_END    ,
}LVPOCOEMCOM_CALL_TYPE_T;

typedef enum{

    LVPOCOEMCOM_TYPE_REPPONSE_SPEED_START = 0,

    LVPOCOEMCOM_TYPE_REPPONSE_SPEED_MEMBERLIST    = 50 ,
    LVPOCOEMCOM_TYPE_REPPONSE_SPEED_GROUPLIST     = 50 ,
    LVPOCOEMCOM_TYPE_REPPONSE_SPEED_MONITORGROUP  = 50 ,
    LVPOCOEMCOM_TYPE_REPPONSE_SPEED_NORMAL        = 100 ,
    LVPOCOEMCOM_TYPE_REPPONSE_SPEED_SLEEP         = 5000 ,

    LVPOCOEMCOM_TYPE_REPPONSE_SPEED_END    ,
}LVPOCOEMCOM_REPPONSE_SPEED_TYPE_T;


/*******************************************OPT REQUEST CODE***********************************************************/
//OPT SEND CODE
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_OPENPOC                "005200"//请求打开POC
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETPARAM               "015300"//请求设置POC
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GETPARAM               "020000"//请求获取POC
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN                  "030000"//请求登入
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGOUT                 "040000"//请求登出
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTSPEAK             "0B0000"//请求开始讲话
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPSPEAK              "0C0000"//请求停止讲话

//MEMBER REQUEST
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_MEMBERCALL                  "0A0000"//请求发起单呼
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LAUNCH_MEMBERVOICECALL      "420000"//请求发起语音通话
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_RECEIVE_MEMBERVOICECALL     "430001"//请求接听语音通话
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_CANNEL_MEMBERVOICECALL      "430000"//请求挂断语音通话

//GROUP REQUEST
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ADDLISTENGROUP              "072200"//请求监听组
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_CANNELLISTENGROUP           "082200"//请求取消监听组
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_REQUESTJOINGROUP            "092200"//请求加入群组
#define LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_INDEX                    "0D0000"//请求组的个数
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ALL_MEMBER_INDEX            "0E000000000001"//请求所有成员信息
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_MEMBER_INDEX          "130000"//请求某个组的成员信息
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GET_GROUP_MEMBER_ONLINE     "3B0000"//请求组成员在线人数


#define LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETVOLUM               "250000"  //请求设置音量
#define LVPOCPOCOEMCOM_SIGNAL_OPTCODE_NOTETONE               "7C0000"  //请求开关提示音
#define LVPOCPOCOEMCOM_SIGNAL_OPTCODE_OPEN_LOG               "7E000001"//开关LOG

#define LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ACCOUT                 "66747468"//ftth---账号
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_END                 "00"        //结尾符
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FAILED              "ff"        //操作失败
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_EXIT_GROUP          "ffffffff"//离开群组

/****************************************OPT ACK CODE*************************************************/
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_POCSTARTED_ACK           "POC:ff00"     //POC已启动应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_OPENPOC_ACK              "POC:00005200" //打开POC应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETPARAM_ACK             "POC:01005300" //设置POC参数应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GETPARAM_ACK             "POC:02000000" //获取POC参数应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN_ACK                "POC:030000"   //登入成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGOUT_ACK               "POC:040000"   //登出成功应答

#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN_STATUS_ACK        "POC:82" //登录应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_NOLOGIN_ACK             "00" //未登录
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGINING_ACK            "01" //正在登录
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN_CANCELLATION_ACK  "03" //登出成功
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN_SUCCESS_ACK       "POC:8202" //离线时登录成功

#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ADDLISTENGROUP_ACK      "POC:07" //请求监听组成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_CANNELLISTENGROUP_ACK   "POC:08" //取消监听组成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_REQUESTJOINGROUP_ACK    "POC:09" //请求加入群组成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTSPEAK_ACK          "0b000000" //开始讲话成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPSPEAK_ACK           "0c000000" //停止讲话成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTLISTEN_ACK         "8b0001"   //开始接听成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPLISTEN_ACK          "8b0000"   //停止接听成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETVOLUM_ACK            "250000"   //设置音量成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_NETWORK_CONNECTED_ACK   "970001"   //POC已连接网络应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_NETWORK_NOT_CONNECT_ACK "970000"   //POC未连接网络应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_UPDATEDATAI_ACK         "POC:85"   //更新用户群组信息
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_JNIO_ACK          "POC:86"   //加入群组
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SPEAKERINFO_ACK         "POC:83"   //讲话用户信息
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_NOTETONE_ACK            "POC:7C"   //关闭提示音

//MEMBER ACK
#define LVPOCPOCOEMCOM_SIGNAL_OPTCODE_MEMBERCALL_ACK             "POC:0a"//单呼用户成功应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_CALL_FAILED_ACK         "POC:8400"//提示--"呼叫失败"应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_UPDATEDATAUSERNAME_ACK  "POC:8700"//用户名字更新
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_VOICECALLSTART_ACK      "POC:a301"//来电
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_VOICECALLEND_ACK        "POC:a300"//结束通话
//0E0000->请求组中的成员个数---应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_MEMBERLISTINFO_ACK      "POC:81"  //返回群组成员信息

//GROUP ACK
#define LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_INDEX_ACK                    "POC:0d"//请求组的个数成功
#define LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_MEMBER_INDEX_ACK             "POC:0e"//请求组中的成员个数成功
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_QUERY_X_GROUPMEMBERINFO_ACK     "POC:13"//某个群组的成员
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUPINFOUPDATE                 "00"        //组更新
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_MEMEBERINFOUPDATE               "01"        //成员更新
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FIXEDGROUP                      "00"        //固定群组
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_TEMPORARYGROUP                  "01"        //临时群组

//0D0000->请求组的个数---应答
#define    LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUPLISTINFO_ACK       "POC:80"//返回群组信息

/********************************************************************************************************/
#define OEM_GROUP_MEMBER_OFFLINE     "01"//离线
#define OEM_GROUP_MEMBER_ONLINE_OUT  "02"//在线--组外
#define OEM_GROUP_MEMBER_ONLINE_IN   "03"//在线--组内

#define OEM_GROUP_MONITOR    "01"//此组被监听组
#define OEM_GROUP_UNMONITOR  "00"//此组未监听组

#define OEM_OTHER_TERMINAL_LOGIN "7651835bc87eef7a7b76555f21000000"//其他终端登录
#define OEM_NO_NETWORK           "e06570656e63517fdc7ee14ff75321000000"//无网络信号

/********************************************************************************************************/
//SET POC FUNC
//A:TTS_FUNC B:NOTIFY_FUNC C:OFFLINEPLAY_FUNC
//1:open 0:close
#define OEM_FUNC_OPEN  "01"
#define OEM_FUNC_CLOSE "00"
#define OEM_DTOS(x)    #x
#define LVPOCPOCOEMCOM_SIGNAL_OPENPOC(A,B,C,D)  A##0##B##0##C##0##D

/**********************EXTERN**********************/

typedef struct _PocGuiOemUserAttr_t
{
    char  OemUserName[64];
    char  OemUserID[64];
    char  OemUserStatus[64];
    int   OemUserNumber;
}PocGuiOemUserAttr_t;

typedef struct _PocGuiOemGroupAttr_t
{
    char  OemGroupName[64];
    char  OemGroupID[32];
    int   OemGroupNumber;
}PocGuiOemGroupAttr_t;

typedef struct
{
    int opt;
    void *group_info;
    void (*cb)(lv_poc_group_oprator_type opt);
} LvPocGuiIdtCom_monitor_group_t;

extern atCmdEngine_t *ap_Oem_engine;

void lvPocGuiOemCom_Init(void);

bool lvPocGuiOemCom_Msg(LvPocGuiOemCom_SignalType_t signal, void * ctx);

bool lvPocGuiOemCom_CriRe_Msg(LvPocGuiOemCom_SignalType_t signal, void * ctx);

bool lvPocGuiOemCom_MessageQueue(osiMessageQueue_t *mq, const void *msg);

int OEM_SendUart(char *uf,int len);

void OEM_TTS_Stop();

int OEM_TTS_Spk(char* atxt);

void lv_poc_oemdata_strtostrhex(char *pszDest, char *pbSrc, int nLen);

unsigned int lv_poc_persist_ssl_hashKeyConvert(char *pUserInput, wchar_t *pKeyArray);

void lv_poc_unicode_to_gb2312_convert(char *pUserInput, char *pUserOutput);

int unicode_to_utf(unsigned long unicode, unsigned char *utf);

int lv_poc_oem_unicode_to_utf8_convert(char *pUserInput, unsigned char *pUserOutput);

uint64_t lv_poc_oemdata_strtodec(char *data,uint32_t len);

uint64_t lv_poc_oemdata_hexstrtodec(char *s, uint32_t len);

void *lvPocGuiOemCom_get_oem_self_info(void);

void *lvPocGuiOemCom_get_current_group_info(void);

bool lvPocGuiOemCom_Request_Groupx_MemeberInfo(char *GroupID);

bool lvPocGuiOemCom_Request_Join_Groupx(char *GroupID);

bool lvPocGuiOemCom_modify_current_group_info(OemCGroup *CurrentGroup);

bool lvPocGuiOemCom_Request_Member_Call(char *UserID);

bool lvPocGuiOemCom_get_speak_status(void);

bool lvPocGuiOemCom_get_listen_status(void);

int lvPocGuiOemCom_get_login_status(void);

bool lvPocGuiOemCom_get_obtainning_state(void);

bool lvPocGuiOemCom_get_system_status(void);

void lvPocGuiOemCom_set_obtainning_state(bool status);

void lvPocGuiOemCom_set_response_speed(LVPOCOEMCOM_REPPONSE_SPEED_TYPE_T response_speed);

void lvPocGuiOemCom_stop_check_ack(void);

OSI_EXTERN_C_END

#endif
