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
#include "tts_player.h"
#include <wchar.h>
#include <locale.h>

#include "at_engine.h"

/*define*/
//#define POC_AP_OEM_AT_OPT //使用AT调试OEM
#define APPTEST_THREAD_PRIORITY (OSI_PRIORITY_NORMAL)
#define APPTEST_STACK_SIZE (8192 * 16)
#define APPTEST_EVENT_QUEUE_SIZE (64)

#define OEMAP_STACK_SIZE (1024 * 16)
#define OEMAP_EVENT_QUEUE_SIZE (64)

//MsgQueueTime
#define POC_OEM_MESSAGE_TIME (500)
#define GUIIDTCOM_OEM_SELF_MSG              0  //[oem self msg]

//debug log info config                        cool watch搜索项
#define GUIIDTCOM_OEMACK_DEBUG_LOG          1  //[oemack]
#define GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG   1  //[grouprefr]
#define GUIIDTCOM_GROUPOPTACK_DEBUG_LOG     1  //[groupoptack]
#define GUIIDTCOM_MEMBERREFR_DEBUG_LOG      1  //[memberrefr]
#define GUIIDTCOM_OEMSPEAK_DEBUG_LOG        1  //[oemspeak]
#define GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG   1  //[oemsinglecall]
#define GUIIDTCOM_OEMLISTEN_DEBUG_LOG       1  //[oemlisten]
#define GUIIDTCOM_OEMERRORINFO_DEBUG_LOG    1  //[oemerrorinfo]
#define GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG 1  //[oemmonitorgroup]
#define GUIIDTCOM_OEMLOGIN_DEBUG_LOG        1  //[oemlogin]

/**********************EXTERN**********************/
atCmdEngine_t *ap_Oem_engine = NULL;

/**********************STATIC**********************/
typedef struct _PocGuiOemApSendAttr_t
{
	char  oembuf[64];
	int   oemlen;
}PocGuiOemApSendAttr_t;

typedef struct _PocGuiOemAtBufferAttr_t
{
	int   index;//参数个数
	PocGuiOemApSendAttr_t buf[16];//AT内容
}PocGuiOemAtBufferAttr_t;

typedef struct _PocGuiOemAtTableAttr_t
{
	int   pindex;//偏移指数
	int   datalen;//获取长度
}PocGuiOemAtTableAttr_t;

//Queue
typedef struct QNode{

	PocGuiOemApSendAttr_t data;
	struct QNode *next;
}QNode, *QueuePtr;

typedef struct{
	QueuePtr p_Head;
	QueuePtr p_rear;
}LinkQueue;

typedef struct _PocGuiOemComAttr_t
{
	bool is_ready;
	osiThread_t *thread;
	osiThread_t *oemapthread;
	//queue
	osiMessageQueue_t *xQueue;
	int   sOemMessageID;
	int   rOemMessageID;
	int   DealMessageQueuePeroid;

	//timer
	osiTimer_t *Oem_AutoLogin_timer;
	osiTimer_t *Oem_GroupUpdate_timer;
	osiTimer_t *Oem_MemberUpdate_timer;
	osiTimer_t *Oem_PoweronRequestMonitorGroup_timer;
	osiTimer_t *Oem_Nologin_timer;
	osiTimer_t *Oem_Check_ack_timeout_timer;
	osiTimer_t *Oem_Openpa_timer;
	osiTimer_t *Oem_BrightScreen_timer;

	//Info
	OemCGroup OemCurrentGroupInfo;
	OemCGroup OemCurrentGroupInfoBuf;//缓存Info
	OemCGroup pGroupMonitorInfo;
	PocGuiOemUserAttr_t  OemUserInfo;
	//cache group member info
	Msg_GData_s Msg_GroupMemberData_s;
	uint8_t OemGroupMemberInfoBufnumber;//member number
	uint8_t OemGroupMemberInfoBufIndex;//
	bool is_OemGroupMemberInfoBuf_update;

	//cache group info
	Msg_CGroup_s GroupData_s;
	uint8_t OemGroupInfoBufnumber;//group number
	bool is_OemGroupInfoBuf_update;

	//note info
	PocGuiOemUserAttr_t  OemSpeakerInfo;
	PocGuiOemGroupAttr_t OemAllGroupInfo;
	PocGuiOemUserAttr_t  OemAllUserInfo;

	//cb
	lv_poc_get_member_list_cb_t pocGetMemberListCb;
	lv_poc_get_group_list_cb_t pocGetGroupListCb;
	poc_build_tempgrp_cb       pocBuildTempGrpCb;
	poc_set_current_group_cb pocSetCurrentGroupCb;
	poc_get_member_status_cb pocGetMemberStatusCb;
	poc_set_member_call_status_cb pocMemberCallCb;
	lv_poc_set_monitor_cb_t pocSetMonitorCb;

	//oem ack
	int         oem_response_time;
	int         signal_multi_call_type;
	bool 		isReady;
	int 		pocnetstatus;
	char 		pocIpAccoutaPassword[128];
	size_t      pocIpAccoutaPasswordLen;
	char 		oem_account[24];

	//status
	bool is_member_call;
	bool member_call_dir;
	int call_type;
	bool is_group_info;
	bool is_member_info;
	int  m_status;
	int  loginstatus_t;
	int  groupstatus_t;
	bool is_listen_status;
	bool is_speak_status;
	bool is_member_update;
	bool is_update_data;
	bool is_oem_system_status;
	bool is_oem_nologin_status;
	bool is_enter_signal_multi_call;
	int  cit_status;
	int  call_dir_type;
	int  call_briscr_dir;
	int  call_curscr_state;

	//other
	int  oem_monitor_group_index;

}PocGuiOemComAttr_t;

enum{
	USER_OPRATOR_START_SPEAK = 3,
	USER_OPRATOR_SPEAKING  = 4,
	USER_OPRATOR_START_LISTEN = 5,
	USER_OPRATOR_LISTENNING = 6,
};

enum{
	USER_OPRATOR_SIGNAL_CALL = 1,//单呼
	USER_OPRATOR_MULTI_CALL  = 2,//多呼
};

/*******************STATIC PARAM********************/
static PocGuiOemComAttr_t pocOemAttr = {0};
#if GUIIDTCOM_OEM_SELF_MSG
static LinkQueue OemQueue = {0};
#endif
/*******************STATIC FUNC********************/
static void pocGuiOemComTaskEntry(void *argument);
static void pocGuiOemApTaskEntry(void *argument);
#if GUIIDTCOM_OEM_SELF_MSG
static bool OemEnQueue(LinkQueue *Queue, PocGuiOemApSendAttr_t e);
static bool OemDeQueue(LinkQueue *Queue, PocGuiOemApSendAttr_t *e);
#endif
static OemCGroup prv_lv_poc_join_current_group_infomation(char * Information);
static PocGuiOemUserAttr_t prv_lv_poc_power_on_get_self_infomation(char * Information);
static PocGuiOemAtBufferAttr_t prv_lv_poc_oem_get_global_atcmd_info(void *info, int index, void *table);
static void prv_lv_poc_oem_get_group_list_info(void *information);
static void prv_lv_poc_oem_get_group_member_info(void *information);
static void prv_lv_poc_oem_update_member_status(void *information);
static void lv_poc_extract_account(char *pocIpAccoutaPassword,char *account);



// 1:TTS正在播放  0:TTS播放完毕
extern void OEM_TTS_Status_CB(int type);

//过滤串口数据  buf为 AT+POC= 等号后面的数据
extern void OEMPOC_AT_Recv(char* buf ,int len);

// 实现开机自动联网，并上报联网状态 1 NET_CONNECTED 0 NET_DISCONNECTED
extern void OEMNetworkStatusChange(int result);

//链接库并调用初始化接口
extern void OEM_PocInit();

//TTS接口 unicode
int OEM_TTS_Spk(char* atxt)
{
	return true;
}

/*enum {
 ETTS_ENCODE_ASCII = 1,
 ETTS_ENCODE_GBK,
 ETTS_ENCODE_UTF16BE,
 ETTS_ENCODE_UTF16LE,
 ETTS_ENCODE_UTF8 };
 ------------------
 TTS接口 encode
*/
int OEM_TTS_SpkEx(char* atxt, unsigned encode)
{
	return true;
}

void OEM_TTS_Stop()
{
	ttsStop();
}

//OEM CB
int OEM_SendUart(char *uf,int len)
{
#if GUIIDTCOM_OEM_SELF_MSG
	PocGuiOemApSendAttr_t apopt = {0};

	strcpy(apopt.oembuf, uf);
	apopt.oemlen = len;

	OemEnQueue(&OemQueue, apopt);
	OSI_LOGXI(OSI_LOGPAR_S, 0, "[oemserver]OEM_SendUart ack %s", apopt.oembuf);
#else
	PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)malloc(sizeof(PocGuiOemApSendAttr_t));
	if(apopt == NULL)
	{
		free(apopt);
		apopt = NULL;

	}
	memset(apopt,0,sizeof(PocGuiOemApSendAttr_t));
   	strcpy(apopt->oembuf, uf);
   	apopt->oemlen = len;


    if(!lvPocGuiOemCom_MessageQueue(pocOemAttr.xQueue, (void *)&apopt))//send point's address, also is param's address, *(&apopt) = malloc(sizeof(PocGuiOemApSendAttr_t))
    {
		if(!lvPocGuiOemCom_MessageQueue(pocOemAttr.xQueue, (void *)&apopt))
    	{
   	  		return 0;
   	 	}
    }
#endif
	return len;
}

//获取模块固件版本号:成功返回0，失败-1
int OEM_GetModeVersion(char *version)
{
	if(version == NULL)
	{
		return -1;
	}

	return 0;
}

#if GUIIDTCOM_OEM_SELF_MSG
/*****************************************POC*******************************************/

bool OemInitQueue(LinkQueue *Queue)
{
	Queue->p_Head = Queue->p_rear = (QueuePtr)malloc(sizeof(QNode));

	if(!Queue)
		return false;
	Queue->p_Head->next = NULL;

	return true;
}

bool OemDestroyQueue(LinkQueue *Queue)
{
	while(Queue->p_Head){
		Queue->p_rear = Queue->p_Head->next;
		free(Queue->p_Head);
		Queue->p_Head = Queue->p_rear;
	}

	return true;
}

static
bool OemEnQueue(LinkQueue *Queue, PocGuiOemApSendAttr_t e)
{
	QueuePtr p;

	p = (QueuePtr)malloc(sizeof(QNode));
	if(!p)
	{
		OSI_LOGI(0, "[song]oem malloc failed");
		return false;
	}
	p->data = e;
	p->next = NULL;
	Queue->p_rear->next = p;
	Queue->p_rear = p;

	return true;
}

static
bool OemDeQueue(LinkQueue *Queue, PocGuiOemApSendAttr_t *e)
{
	QueuePtr p;

	if(Queue->p_Head == Queue->p_rear)
		return false;

	p = Queue->p_Head->next;
	*e = p->data;
	Queue->p_Head->next = p->next;
	if(Queue->p_rear == p)
		Queue->p_rear = Queue->p_Head;
	free(p);

	return true;
}
#endif

void lvPocGuiOemCom_Request_Login(void)
{
	#if GUIIDTCOM_OEMLOGIN_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[oemack][login]%s(%d):start login in", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif

	OEMPOC_AT_Recv(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN, strlen(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN));
}

void lvPocGuiOemCom_Request_Logout(void)
{
	#if GUIIDTCOM_OEMLOGIN_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[oemack][login]%s(%d):start login out", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif
	OEMPOC_AT_Recv(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGOUT, strlen(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGOUT));
}

void lvPocGuiOemCom_Request_OpenPOC(char *tts_status, char *notify_status, char* offlineplay_status)
{
	char OEM_OPT_CODE[64] = {0};
	strcpy(OEM_OPT_CODE, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_OPENPOC);
	strcat(OEM_OPT_CODE, (char *)tts_status);
	strcat(OEM_OPT_CODE, (char *)notify_status);
	strcat(OEM_OPT_CODE, (char *)offlineplay_status);
	strcat(OEM_OPT_CODE, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_END);

	OEMPOC_AT_Recv((char *)OEM_OPT_CODE, strlen((char *)OEM_OPT_CODE));
	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[oemack][login]open poc %s",OEM_OPT_CODE);
}

void lvPocGuiOemCom_Request_add_MonitorGroup(char *id_group)
{
	char groupoptcode[64] = {0};

	strcpy(groupoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ADDLISTENGROUP);
	strcat(groupoptcode, id_group);

	OEMPOC_AT_Recv(groupoptcode, strlen(groupoptcode));
}

void lvPocGuiOemCom_Request_cannel_MonitorGroup(char *id_group)
{
	char groupoptcode[64] = {0};

	strcpy(groupoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_CANNELLISTENGROUP);
	strcat(groupoptcode, id_group);

	OEMPOC_AT_Recv(groupoptcode, strlen(groupoptcode));
}

void lvPocGuiOemCom_Request_Start_Speak(void)
{
	OEMPOC_AT_Recv((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTSPEAK, strlen((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTSPEAK));
}

void lvPocGuiOemCom_Request_Stop_Speak(void)
{
	OEMPOC_AT_Recv((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPSPEAK, strlen((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPSPEAK));
}

void lvPocGuiOemCom_Request_Set_Tone_Volum(char *volum)//0-100
{
	char volumoptcode[64] = {0};

	strcpy(volumoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETVOLUM);
	strcat(volumoptcode, volum);
	strcat(volumoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_END);//end eof

	OEMPOC_AT_Recv(volumoptcode, strlen(volumoptcode));
}

void lvPocGuiOemCom_Request_PocParam(void)
{
	OEMPOC_AT_Recv((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GETPARAM, strlen((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GETPARAM));
}

void lvPocGuiOemCom_Request_AllGroupInfo(void)
{
	OEMPOC_AT_Recv((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_INDEX, strlen((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_INDEX));
}

void lvPocGuiOemCom_Request_IndexGrpMemeberInfo(char *IndexGrpID)
{
	char pIdxGrp[64] = {0};
	if(IndexGrpID == NULL)
	{
		return;
	}
	strcpy(pIdxGrp, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ALL_MEMBER_INDEX);
	strcat(pIdxGrp, IndexGrpID);

	OEMPOC_AT_Recv((char *)pIdxGrp, strlen((char *)pIdxGrp));
}

bool lvPocGuiOemCom_Request_Groupx_MemeberInfo(char *GroupID)
{
	char pGroup[64] = {0};
	if(GroupID == NULL)
	{
		return false;
	}
	strcpy(pGroup, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_MEMBER_INDEX);
	strcat(pGroup, GroupID);

	OEMPOC_AT_Recv((char *)pGroup, strlen((char *)pGroup));
	return true;
}

bool lvPocGuiOemCom_Request_Join_Groupx(char *GroupID)
{
	char pGroup[64] = {0};
	if(GroupID == NULL)
	{
		return false;
	}
	strcpy(pGroup, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_REQUESTJOINGROUP);
	strcat(pGroup, GroupID);

	OEMPOC_AT_Recv((char *)pGroup, strlen((char *)pGroup));
	return true;
}

bool lvPocGuiOemCom_Request_Member_Call(char *UserID)
{
	char pUser[64] = {0};
	if(UserID == NULL)
	{
		return false;
	}
	strcpy(pUser, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_MEMBERCALL);
	strcat(pUser, UserID);

	OEMPOC_AT_Recv((char *)pUser, strlen((char *)pUser));
	return true;
}

void lvPocGuiOemCom_Request_Set_Tone(char *tonestatus)
{
	char toneoptcode[64] = {0};

	strcpy(toneoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_NOTETONE);
	strcat(toneoptcode, tonestatus);

	OEMPOC_AT_Recv(toneoptcode, strlen(toneoptcode));
}

void lvPocGuiOemCom_Request_Set_Log(char *switchstatus)
{
	char logoptcode[64] = {0};

	strcpy(logoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_OPEN_LOG);
	strcat(logoptcode, switchstatus);

	OEMPOC_AT_Recv(logoptcode, strlen(logoptcode));
}

void lvPocGuiOemCom_Request_Set_AudioQuility(char *quility)
{
	char quilityoptcode[64] = {0};

	strcpy(quilityoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_AUDIOQUILITY);
	strcat(quilityoptcode, quility);

	OEMPOC_AT_Recv(quilityoptcode, strlen(quilityoptcode));
}

void lvPocGuiOemCom_Request_Set_HeartBeat(char *heartBeat)
{
	char heartBeatoptcode[64] = {0};

	strcpy(heartBeatoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_HEARTBEAT);
	strcat(heartBeatoptcode, heartBeat);

	OEMPOC_AT_Recv(heartBeatoptcode, strlen(heartBeatoptcode));
}

static
void prvPocGuiOemTaskHandleMsgCB(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_AP_POC_IND:
		{
			char *ap_poc_param = (char *)ctx;

			OEMPOC_AT_Recv(ap_poc_param, strlen(ap_poc_param));
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_AP_POC_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			#if GUIIDTCOM_OEMACK_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[oemack]%s(%d):linkqueue[%s]", __func__, __LINE__, apopt->oembuf);
			OSI_LOGI(0, cOutstr);
			#endif

			//set poc
			if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETPARAM_ACK))
			{
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SETPOC_REP, NULL);
				break;
			}

			switch(pocOemAttr.loginstatus_t)
			{
				case LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS:
				{
					//login status--82应答
					if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN_STATUS_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_EXIT_REP, apopt);
						break;
					}
					//speak start--960000应答
					else if(NULL != strstr(apopt->oembuf, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTSPEAK_ACK))
					{
						pocOemAttr.m_status = USER_OPRATOR_SPEAKING;
						lvPocGuiOemCom_CriRe_Msg(LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_REP, NULL);
						break;
					}
					//speak stop--960001应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPSPEAK_ACK))
					{
						lvPocGuiOemCom_CriRe_Msg(LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_REP, NULL);
						break;
					}
					//call failed--960002应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_CALLOUT_FAILED_ACK))
					{
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"呼叫失败", NULL);
						break;
					}
					//listen start--960003应答
					else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTLISTEN_ACK))
					{
						pocOemAttr.m_status = USER_OPRATOR_LISTENNING;
						pocOemAttr.call_dir_type = LVPOCOEMCOM_CALL_DIR_TYPE_REP;
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_STOP_PLAYER_VOICE, NULL);
						break;
					}
					//listen stop--960004应答
					else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPLISTEN_ACK))
					{
						lvPocGuiOemCom_CriRe_Msg(LVPOCGUIOEMCOM_SIGNAL_LISTEN_STOP_REP, NULL);
						break;
					}
					//join group info--notify ack:86
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_JNIO_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_JOIN_GROUP_REP, apopt);
						break;
					}
					//group number(群组个数)--0D应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_INDEX_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_REP, apopt);
						break;
					}
					//all group info(所有群组信息)--80应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUPLISTINFO_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_GROUPLIST_DATA_REP, apopt);
						break;
					}
					//index 1 group's member number(成员个数)--0E应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_MEMBER_INDEX_ACK))
					{
						if(pocOemAttr.is_enter_signal_multi_call)
						{
							lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_REP, apopt);
						}
						else
						{
							lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_INFO_REP, apopt);
						}
						break;
					}
					//index 1 group's member info(成员信息)--81应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_MEMBERLISTINFO_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_DATA_REP, apopt);
						break;
					}
					//query x group's member(某个群组的成员)--13应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_QUERY_X_GROUPMEMBERINFO_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_REP, apopt);
						break;
					}
					//grouplist join other group ack(加入群组)--09应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_REQUESTJOINGROUP_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SET_CURRENT_GROUP_REP, apopt);
						break;
					}
					//single call(单呼)--0a应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_MEMBERCALL_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP, apopt);
						break;
					}
					//update user or group info--notify ack:85
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_UPDATEDATAI_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_MEMBER_GROUP_INFO_UPDATE, apopt);
						break;
					}
					//speaker info--notify ack:83
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SPEAKERINFO_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_CALL_TALKING_ID_IND, apopt);
						break;
					}
					//request monitor group--ack:07
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ADDLISTENGROUP_ACK))
					{
						if(lvPocGuiOemCom_get_system_status() == false)
						{
							lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_POWERON_CHECK_MONITOR_GROUP_REP, NULL);
						}
						else
						{
							lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_MONITOR_GROUP_REP, apopt);
						}
						break;
					}
					//cannel monitor group--ack:08
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_CANNELLISTENGROUP_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_UNMONITOR_GROUP_REP, apopt);
						break;
					}
					break;
				}
				case LVPOCOEMCOM_SIGNAL_LOGIN_EXIT:
				case LVPOCOEMCOM_SIGNAL_LOGIN_FAILED:
				{
					//app start
					if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_POCSTARTED_ACK))
					{
						lvPocGuiOemCom_Request_PocParam();
						break;
					}
					//open poc
					else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_OPENPOC_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_OPENPOC_REP, (void*)apopt->oembuf);
						break;
					}
					//get poc
					else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GETPARAM_ACK))
					{
						if(lvPocGuiOemCom_get_login_status() == LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
						{
							break;
						}

						nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
						//查询账号是否正确
						if(NULL != strstr(apopt->oembuf,poc_config->oem_account))
						{
							lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_AP_POC_START, NULL);
							OSI_LOGI(0, "[oemack][login]accout correct, start to login");
						}
						else//需重新设置账号
						{
							if(pocOemAttr.is_oem_nologin_status == false)
							{
								pocOemAttr.is_oem_nologin_status = true;
								osiTimerStart(pocOemAttr.Oem_Nologin_timer, 6000);
							}
							OSI_PRINTFI("[oemack][login]accout error, please to set");
						}
						osiTimerIsRunning(pocOemAttr.Oem_AutoLogin_timer) ? 0 : osiTimerStart(pocOemAttr.Oem_AutoLogin_timer, 5000);
						OSI_PRINTFI("[oemack][login](%s)(%d):accout ori(%s), nv(%s)", __func__, __LINE__, apopt->oembuf, poc_config->oem_account);
						break;
					}
					//login success
					else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN_SUCCESS_ACK))
					{
						if(lvPocGuiOemCom_get_login_status() == LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
						{
							break;
						}

						osiTimerIsRunning(pocOemAttr.Oem_AutoLogin_timer) ? osiTimerStop(pocOemAttr.Oem_AutoLogin_timer) : 0;
						pocOemAttr.loginstatus_t = LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS;
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_LOGIN_REP, apopt);
						break;
					}

					break;
				}

				default:
				{
					break;
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

static
void prvPocGuiOemTaskHandleSetPOC(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_SETPOC_IND:
		{
			char ufs[128] = {0};
			memset(&pocOemAttr.pocIpAccoutaPassword, 0, sizeof(pocOemAttr.pocIpAccoutaPassword));
			//read account
			memset(&pocOemAttr.oem_account, 0, sizeof(pocOemAttr.oem_account));
			lv_poc_extract_account((char *)pocOemAttr.pocIpAccoutaPassword,(char *)pocOemAttr.oem_account);
			//转义StrHex
			lv_poc_oemdata_strtostrhex(ufs, (char *)ctx, strlen((char *)ctx));

			//组合
			strcpy(pocOemAttr.pocIpAccoutaPassword, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETPARAM);
			strcat(pocOemAttr.pocIpAccoutaPassword, ufs);
			strcat(pocOemAttr.pocIpAccoutaPassword, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_END);
			pocOemAttr.pocIpAccoutaPasswordLen = strlen(pocOemAttr.pocIpAccoutaPassword);
			//SET POC
			OEMPOC_AT_Recv(pocOemAttr.pocIpAccoutaPassword, pocOemAttr.pocIpAccoutaPasswordLen);
		 	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[oemack][set][ind]oem set param %s", (char *)pocOemAttr.pocIpAccoutaPassword);

			//save account
			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
			strcpy(poc_config->oemipaccoutapassword, pocOemAttr.pocIpAccoutaPassword);
			strcpy(poc_config->oem_account,pocOemAttr.oem_account);
			poc_config->oemipaccoutapasswordlen = pocOemAttr.pocIpAccoutaPasswordLen;
			lv_poc_setting_conf_write();
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SETPOC_REP:
		{
			OSI_LOGI(0, "[oemack][set][server]oem set param success");
			if(pocOemAttr.loginstatus_t != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
			{//otherwhere have to login
				//lvPocGuiOemCom_Request_Login();
			}
			break;
		}

		default:
		{
			break;
		}
	}

}

static
void prvPocGuiOemTaskHandleOpenPOC(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_OPENPOC_IND:
		{

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_OPENPOC_REP:
		{
			lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_LOGIN_IND, (void*)ctx);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_AP_POC_START:
		{
			lvPocGuiOemCom_Request_OpenPOC(OEM_FUNC_CLOSE, OEM_FUNC_OPEN, OEM_FUNC_OPEN);
			break;
		}

		default:
		{
			break;
		}
	}

}

static
void prvPocGuiOemTaskHandleLogin(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_LOGIN_IND:
		{
			lvPocGuiOemCom_Request_Login();
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_LOGIN_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Success_Login, 50, false);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "成功登录");
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, NULL);
			//user info
			pocOemAttr.OemUserInfo = prv_lv_poc_power_on_get_self_infomation(apopt->oembuf);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_EXIT_IND:
		{
			lvPocGuiOemCom_Request_Logout();
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_EXIT_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;
			PocGuiOemAtBufferAttr_t OfflineInfo;
			PocGuiOemAtTableAttr_t table[4] = {
				{5,2},//应答号--82
				{7,2},//登录状态
				{9,4},//操作ID--忽略
				{13,0},//离线原因
			};

			OfflineInfo = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

			if(0 == strcmp(OfflineInfo.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_NOLOGIN_ACK))
			{//no login
				if(NULL != strstr(OfflineInfo.buf[3].oembuf, OEM_OTHER_TERMINAL_LOGIN))
				{
					poc_play_voice_one_time(LVPOCAUDIO_Type_This_Account_Already_Logined, 50, false);
				}
				else if(NULL != strstr(OfflineInfo.buf[3].oembuf, OEM_NO_NETWORK))
				{//auto login
					osiTimerIsRunning(pocOemAttr.Oem_AutoLogin_timer) ? 0 : osiTimerStart(pocOemAttr.Oem_AutoLogin_timer, 5000);
				}
				else
				{//auto login
					osiTimerIsRunning(pocOemAttr.Oem_AutoLogin_timer) ? 0 : osiTimerStart(pocOemAttr.Oem_AutoLogin_timer, 5000);
				}
				pocOemAttr.loginstatus_t = LVPOCOEMCOM_SIGNAL_LOGIN_FAILED;
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_1500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "登录失败");
				pocOemAttr.is_oem_system_status = false;
			}
			else if(0 == strcmp(OfflineInfo.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGINING_ACK))
			{//loginning

			}
			else if(0 == strcmp(OfflineInfo.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN_CANCELLATION_ACK))
			{//login out success
				pocOemAttr.loginstatus_t = LVPOCOEMCOM_SIGNAL_GROUP_EXIT;
			}
			break;
		}

		default:
		{
			break;
		}
	}
}

static
void prvPocGuiOemTaskHandleSpeak(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_IND:
		{
			if(pocOemAttr.loginstatus_t != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
            {
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"未登录", NULL);
            }
			else
			{
				pocOemAttr.call_dir_type = LVPOCOEMCOM_CALL_DIR_TYPE_IND;
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_STOP_PLAYER_VOICE, NULL);
			}
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_REP:
		{
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "开始对讲", NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始对讲", NULL);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
			if(pocOemAttr.m_status == USER_OPRATOR_SPEAKING)
			{
				char speak_name[100] = "";
				strcpy((char *)speak_name, (const char *)pocOemAttr.OemUserInfo.OemUserName);
				if(!pocOemAttr.is_member_call)
				{
					char group_name[100] = "";
					strcpy((char *)group_name, (const char *)pocOemAttr.OemCurrentGroupInfo.m_ucGName);
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, speak_name, group_name);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)speak_name, (const uint8_t *)group_name);
				}
				else
				{
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, speak_name, "");
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)speak_name, (const uint8_t *)"");
				}
			}

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_IND:
		{
			lvPocGuiOemCom_Request_Stop_Speak();
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_REP:
		{
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Speak, 30, true);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, "停止对讲", NULL);
			//lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"停止对讲", NULL);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lvPocGuiOemCom_CriRe_Msg(LVPOCGUIOEMCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT, (void *)LVPOCOEMCOM_CALL_DIR_TYPE_SPEAK);
			break;
		}

		default:
		{
			break;
		}
	}
}

static
void prvPocGuiOemTaskHandleListen(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_LISTEN_START_REP:
		{
			pocOemAttr.m_status = USER_OPRATOR_START_LISTEN;
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_LISTEN_STOP_REP:
		{
			pocOemAttr.is_listen_status = false;
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");
			//lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)"停止聆听", (const uint8_t *)"");
			//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lvPocGuiOemCom_CriRe_Msg(LVPOCGUIOEMCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT, (void *)LVPOCOEMCOM_CALL_DIR_TYPE_LISTEN);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_LISTEN_SPEAKER_REP:
		{
			char speaker_name[100];
			char *speaker_group = (char *)pocOemAttr.OemCurrentGroupInfo.m_ucGName;
			memset(speaker_name, 0, sizeof(char) * 100);
			strcpy(speaker_name, (const char *)pocOemAttr.OemSpeakerInfo.OemUserName);
			strcat(speaker_name, (const char *)"正在讲话");
			pocOemAttr.is_listen_status = true;
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
			//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			if(pocOemAttr.is_member_call)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)"");
			}

			if(!pocOemAttr.is_member_call)
			{
				char speaker_group_name[100];
				strcpy(speaker_group_name, (const char *)speaker_group);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, speaker_name, speaker_group);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)speaker_group_name);
			}

			break;
		}

		default:
		{
			break;
		}
	}
}

static
void prvPocGuiOemTaskHandleGroupUpdateOpt(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_JOIN_GROUP_REP://86
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			memcpy(&pocOemAttr.OemCurrentGroupInfoBuf, &pocOemAttr.OemCurrentGroupInfo, sizeof(OemCGroup));
			pocOemAttr.OemCurrentGroupInfo = prv_lv_poc_join_current_group_infomation(apopt->oembuf);

			if(0 == strcmp((char *)pocOemAttr.OemCurrentGroupInfo.m_ucGID,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_EXIT_GROUP))//exit group
			{
				pocOemAttr.groupstatus_t = LVPOCOEMCOM_SIGNAL_GROUP_EXIT;

				switch(pocOemAttr.call_type)
				{
					case LVPOCOEMCOM_CALL_TYPE_SINGLE:
					{
						if(pocOemAttr.signal_multi_call_type == USER_OPRATOR_SIGNAL_CALL)
						{
							OSI_PRINTFI("[build][tempgrp](%s)(%d):signal-call, exit group", __func__, __LINE__);
							lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP, NULL);
						}
						else
						{
							OSI_PRINTFI("[build][tempgrp](%s)(%d):multi-call, exit group", __func__, __LINE__);
							lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_EXIT_REP, NULL);
						}
						break;
					}

					case LVPOCOEMCOM_CALL_TYPE_GROUP:
					{
						OSI_PRINTFI("[build][tempgrp](%s)(%d):group-call, exit group", __func__, __LINE__);
						break;
					}
				}
			}
			else
			{
				if(0 == strcmp((char *)pocOemAttr.OemCurrentGroupInfo.m_ucGTemporary, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_TEMPORARYGROUP))
				{
					//join singel call or multi call
					lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_REP, NULL);
					OSI_PRINTFI("[call][tempgrp](%s)(%d):start single call or multi call", __func__, __LINE__);
				}
				else if(0 == strcmp((char *)pocOemAttr.OemCurrentGroupInfo.m_ucGTemporary, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FIXEDGROUP))
				{
					//join group call
					pocOemAttr.groupstatus_t = LVPOCOEMCOM_SIGNAL_GROUP_JOIN;
					poc_play_voice_one_time(LVPOCAUDIO_Type_Join_Group, 50, false);

					pocOemAttr.call_type = LVPOCOEMCOM_CALL_TYPE_GROUP;
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocOemAttr.OemUserInfo.OemUserName, pocOemAttr.OemCurrentGroupInfo.m_ucGName);

					//power on ready work
					if(!lvPocGuiOemCom_get_system_status())
					{
						osiTimerStart(pocOemAttr.Oem_PoweronRequestMonitorGroup_timer, 500);
					}
					OSI_PRINTFI("[groupcall](%s)(%d):selete join group", __func__, __LINE__);
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

static
void prvPocGuiOemTaskHandleGroupList(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_IND:
		{
			if(pocOemAttr.pocGetGroupListCb == NULL)
			{
				break;
			}

			if(pocOemAttr.loginstatus_t != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
			{
				pocOemAttr.pocGetGroupListCb(1, 0, NULL);
				pocOemAttr.pocGetGroupListCb = NULL;
				return;
			}

			if(pocOemAttr.loginstatus_t == LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS
				&& (lvPocGuiOemCom_get_system_status() == false))//monitor group opt not ready
			{
				pocOemAttr.pocGetGroupListCb(1, 0, NULL);
				pocOemAttr.pocGetGroupListCb = NULL;
				return;
			}

			lvPocGuiOemCom_Request_AllGroupInfo();
			lvPocGuiOemCom_set_obtainning_state(true);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取群组列表...", NULL);
			break;
		}
	    case LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_REP:
	    {
			if(ctx == 0)
			{
				break;
			}

			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;
			PocGuiOemAtBufferAttr_t Grouplist;
			PocGuiOemAtTableAttr_t table[4] = {
				{5,2},//应答号--0D
				{7,2},//结果码
				{9,4},//操作ID--忽略
				{13,4},//群组个数
			};

			Grouplist = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);
			pocOemAttr.GroupData_s.m_Group_Num = lv_poc_oemdata_hexstrtodec(Grouplist.buf[3].oembuf, sizeof(Grouplist.buf[3].oembuf)/sizeof(char));
			pocOemAttr.OemGroupInfoBufnumber = 0;

			if(0 == strcmp(Grouplist.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FAILED))
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);

				pocOemAttr.pocGetGroupListCb(1, 0, NULL);
				pocOemAttr.pocGetGroupListCb = NULL;
				lvPocGuiOemCom_set_obtainning_state(false);
				return;
			}

			if(pocOemAttr.is_enter_signal_multi_call)
			{
				OSI_PRINTFI("[build][tempgrp](%s)(%d)rec group Number, goto query member", __func__, __LINE__);
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_GET_MEMBER_REP, NULL);
			}
			else
			{
				//refr group
				pocOemAttr.pocGetGroupListCb(2, pocOemAttr.GroupData_s.m_Group_Num, &pocOemAttr.GroupData_s);
				lvPocGuiOemCom_set_obtainning_state(false);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
#if GUIIDTCOM_OEM_SELF_MSG
				lvPocGuiOemCom_set_response_speed(LVPOCOEMCOM_TYPE_REPPONSE_SPEED_NORMAL);
#endif
			}
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_UPDATE:
		{

			break;
		}

	    case LVPOCGUIOEMCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
	    {
			pocOemAttr.pocGetGroupListCb = (lv_poc_get_group_list_cb_t)ctx;
			osiTimerStart(pocOemAttr.Oem_Check_ack_timeout_timer, 3000);
			break;
		}


	    case LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
	    {
			pocOemAttr.pocGetGroupListCb = NULL;
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_GROUPLIST_DATA_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			prv_lv_poc_oem_get_group_list_info(apopt->oembuf);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiIdtTaskHandleBuildTempGrp(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			if(pocOemAttr.pocBuildTempGrpCb == NULL)
			{
				break;
			}

			if(pocOemAttr.loginstatus_t == LVPOCOEMCOM_SIGNAL_LOGIN_FAILED)
			{
				pocOemAttr.pocBuildTempGrpCb(0);
				pocOemAttr.pocBuildTempGrpCb = NULL;
				break;
			}

			char multimemberid[128];
			memset(multimemberid, 0, sizeof(128));
			lv_poc_build_new_tempgrp_t * new_tempgrp = (lv_poc_build_new_tempgrp_t *)ctx;
			OemCGroupMember *member_info = NULL;

			for(int i = 0; i < new_tempgrp->num; i++)
			{
				member_info = (OemCGroupMember *)new_tempgrp->members[i];
				if(member_info == NULL)
				{
					OSI_PRINTFI("[build][tempgrp](%s)(%d)failed", __func__, __LINE__);
					pocOemAttr.pocBuildTempGrpCb(0);
					pocOemAttr.pocBuildTempGrpCb = NULL;
					return;
				}
				strcat(multimemberid, (char *)member_info->m_ucUID);
			}
			OSI_PRINTFI("[build][tempgrp](%s)(%d):multi-id(%s)", __func__, __LINE__, multimemberid);
			//request member multi calls
			lvPocGuiOemCom_Request_Member_Call((char *)multimemberid);
			lvPocGuiOemCom_set_obtainning_state(true);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_REP:
		{
			lvPocGuiOemCom_Request_AllGroupInfo();
			pocOemAttr.is_enter_signal_multi_call = true;
			OSI_PRINTFI("[build][tempgrp](%s)(%d)query all group", __func__, __LINE__);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_GET_MEMBER_REP:
		{
			if(pocOemAttr.pocBuildTempGrpCb == NULL)
			{
				OSI_PRINTFI("[call][tempgrp](%s)(%d):rev call_memberinfo", __func__, __LINE__);
				extern void lv_poc_tmpgrp_multi_call_open(void);
				pocOemAttr.is_enter_signal_multi_call ? lv_poc_tmpgrp_multi_call_open() : 0;
			}
			else
			{
				OSI_PRINTFI("[call][tempgrp](%s)(%d):start request call_memberinfo", __func__, __LINE__);
				pocOemAttr.pocBuildTempGrpCb(1);
				pocOemAttr.pocBuildTempGrpCb = NULL;
			}
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_EXIT_REP:
		{
			lv_poc_memberlist_activity_close(POC_EXITGRP_PASSIVE);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_REGISTER_BIUILD_TEMPGRP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocOemAttr.pocBuildTempGrpCb = (poc_build_tempgrp_cb)ctx;
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_BIUILD_TEMPGRP_CB_IND:
		{
			OSI_PRINTFI("[tempgrp](%s)(%d):cannel build tempgrp cb", __func__, __LINE__);
			pocOemAttr.pocBuildTempGrpCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static
void prvPocGuiOemTaskHandleSetCurrentGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_SET_CURRENT_GROUP_IND:
		{
			if(pocOemAttr.pocSetCurrentGroupCb == NULL)
			{
				break;
			}
			OemCGroup *SetCGroup = (OemCGroup *)ctx;
			//request join group
			lvPocGuiOemCom_Request_Join_Groupx((char *)SetCGroup->m_ucGID);
			lvPocGuiOemCom_set_obtainning_state(true);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取群组成员...", NULL);

			#if GUIIDTCOM_GROUPOPTACK_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[grouproptack]%s(%d):join group", __func__, __LINE__);
			OSI_LOGI(0, cOutstr);
			#endif

			lv_task_t *oncetask = lv_task_create(lv_poc_activity_func_cb_set.member_list.group_member_act, 50, LV_TASK_PRIO_HIGHEST, (void *)SetCGroup->m_ucGName);
            lv_task_once(oncetask);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SET_CURRENT_GROUP_REP:
		{
			if(pocOemAttr.pocSetCurrentGroupCb == NULL)
			{
				return;
			}

			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;
			PocGuiOemAtBufferAttr_t CurrentGroup;
			PocGuiOemAtTableAttr_t table[3] = {
				{5,2},//应答号--09
				{7,2},//结果码
				{9,4},//操作ID--忽略
			};

			CurrentGroup = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

			if(0 == strcmp(CurrentGroup.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FAILED))//same group
			{
				pocOemAttr.pocSetCurrentGroupCb(2);
				break;
			}else
			{//set success
				pocOemAttr.pocSetCurrentGroupCb(1);
			}

			#if GUIIDTCOM_GROUPOPTACK_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[grouproptack]%s(%d):join group ack to refr", __func__, __LINE__);
			OSI_LOGI(0, cOutstr);
			#endif

			break;
		}

    	case LVPOCGUIOEMCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
    	{
			pocOemAttr.pocSetCurrentGroupCb = (poc_set_current_group_cb)ctx;
			break;
		}

    	case LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
    	{
			pocOemAttr.pocSetCurrentGroupCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static
void prvPocGuiOemTaskHandlePoweronMemberOpt(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_INFO_IN:
		{
			//上电第一次获取索引为1的群组的所有成员
			lvPocGuiOemCom_Request_IndexGrpMemeberInfo("00000001");
			pocOemAttr.is_member_info = true;
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_INFO_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			PocGuiOemAtBufferAttr_t Userlist;
			PocGuiOemAtTableAttr_t table[4] = {
				{5,2},//应答号--0E
				{7,2},//结果码
				{9,4},//操作ID--忽略
				{13,4},//用户个数
			};

			Userlist = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);
			pocOemAttr.OemAllUserInfo.OemUserNumber = lv_poc_oemdata_hexstrtodec(Userlist.buf[3].oembuf, sizeof(Userlist.buf[3].oembuf)/sizeof(char));
			break;
		}

		default:
		{
			break;
		}
	}

}

static
void prvPocGuiOemTaskHandleMemberList(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
		{
			if(pocOemAttr.pocGetMemberListCb == NULL)
			{
				break;
			}

			if(pocOemAttr.loginstatus_t != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
			{
				pocOemAttr.pocGetMemberListCb(1, 0, NULL);
				pocOemAttr.pocGetMemberListCb = NULL;
				return;
			}

			if(pocOemAttr.loginstatus_t == LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS
				&& (lvPocGuiOemCom_get_system_status() == false))//monitor group opt not ready
			{
				pocOemAttr.pocGetMemberListCb(1, 0, NULL);
				pocOemAttr.pocGetMemberListCb = NULL;
				return;
			}

			if(ctx == 0)
			{
				OSI_PRINTFI("[tempgrp](%s)(%d):ctx'0 get member", __func__, __LINE__);
				lvPocGuiOemCom_set_obtainning_state(true);
				if(pocOemAttr.is_enter_signal_multi_call)//signal call or multi call
				{
					char GrpIndex[32];
					char GrpIdxStr[8];
					int GrpNum = pocOemAttr.GroupData_s.m_Group_Num - 1;

					__itoa(GrpNum, GrpIdxStr, 10);
					GrpNum > 9 ? strcpy(GrpIndex, "000000") : strcpy(GrpIndex, "0000000");
					strcat(GrpIndex, GrpIdxStr);

					GrpNum ? lvPocGuiOemCom_Request_IndexGrpMemeberInfo(GrpIndex) : 0;
					OSI_PRINTFI("[build][tempgrp](%s)(%d)request index tmpgrp Info, GrpIdx(%s)", __func__, __LINE__, GrpIndex);
				}
				else//memberlist
				{
					//get current group's memberlist info
					OemCGroup *CurrentGUInfo = (OemCGroup *)lvPocGuiOemCom_get_current_group_info();

					if(CurrentGUInfo->m_ucGID != NULL)
					{
						lvPocGuiOemCom_Request_Groupx_MemeberInfo((char *)CurrentGUInfo->m_ucGID);
						lvPocGuiOemCom_set_obtainning_state(true);
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取成员列表...", NULL);
					}
				}
				break;
			}
			else
			{
				//grouplist x group get member
				OemCGroup *xgroup_info = (OemCGroup *)ctx;
				if(xgroup_info->m_ucGID != NULL)
				{
					lvPocGuiOemCom_Request_Groupx_MemeberInfo((char *)xgroup_info->m_ucGID);
				}
				OSI_PRINTFI("[tempgrp](%s)(%d):grouplist x group get member", __func__, __LINE__);
			}

			break;
		}
		case LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			PocGuiOemAtBufferAttr_t Mmemberlist;
			PocGuiOemAtTableAttr_t table[4] = {
				{5,2},//应答号--13 or 0E
				{7,2},//结果码
				{9,4},//操作ID--忽略
				{13,4},//组成员个数
			};

			Mmemberlist = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);
			pocOemAttr.Msg_GroupMemberData_s.dwNum = lv_poc_oemdata_hexstrtodec(Mmemberlist.buf[3].oembuf, sizeof(Mmemberlist.buf[3].oembuf)/sizeof(char));
			pocOemAttr.OemGroupMemberInfoBufnumber = 0;

			if(0 == strcmp(Mmemberlist.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FAILED))
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				if(pocOemAttr.is_member_update == false)
				{
					pocOemAttr.pocGetMemberListCb ? pocOemAttr.pocGetMemberListCb(1, 0, NULL) : 0;
					pocOemAttr.pocGetMemberListCb = NULL;
				}else
				{
					pocOemAttr.is_member_update = false;
				}
				lvPocGuiOemCom_set_obtainning_state(false);
				return;
			}

			if(pocOemAttr.is_enter_signal_multi_call)//signal call or multi call
			{
				pocOemAttr.is_enter_signal_multi_call = false;
				if(pocOemAttr.Msg_GroupMemberData_s.dwNum > 1)//multi call(no self)
				{
					//refr member
					OSI_PRINTFI("[multi-call](%s)(%d)multi call, start refr list, member'number(%d)", __func__, __LINE__, pocOemAttr.Msg_GroupMemberData_s.dwNum);
					lv_task_t *oncetask = lv_task_create(lv_poc_activity_func_cb_set.member_list.group_member_act, 50, LV_TASK_PRIO_HIGHEST, (void *)pocOemAttr.OemCurrentGroupInfo.m_ucGName);
					lv_task_once(oncetask);
					lv_task_ready(oncetask);
					poc_play_voice_one_time(LVPOCAUDIO_Type_Enter_Temp_Group, 50, false);
					pocOemAttr.signal_multi_call_type = USER_OPRATOR_MULTI_CALL;
					pocOemAttr.call_type = LVPOCOEMCOM_CALL_TYPE_SINGLE;
					pocOemAttr.pocGetMemberListCb(2, pocOemAttr.Msg_GroupMemberData_s.dwNum, &pocOemAttr.Msg_GroupMemberData_s);
					lvPocGuiOemCom_set_obtainning_state(false);
				}
				else//signal call
				{
					pocOemAttr.signal_multi_call_type = USER_OPRATOR_SIGNAL_CALL;
					pocOemAttr.call_type = LVPOCOEMCOM_CALL_TYPE_SINGLE;

					if(pocOemAttr.is_member_call)
					{
						OSI_PRINTFI("[signalcall](%s)(%d):have in signal call, break", __func__, __LINE__);
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND, NULL);
						break;
					}

					static OemCGroupMember member_call_obj = {0};
					memset(&member_call_obj, 0, sizeof(OemCGroupMember));

					OSI_PRINTFI("[signalcall](%s)(%d):signal call, start open view, member'number(%d)", __func__, __LINE__, pocOemAttr.Msg_GroupMemberData_s.dwNum);
					strcpy((char *)member_call_obj.m_ucUName, (const char *)pocOemAttr.OemCurrentGroupInfo.m_ucGName);//临时呼叫的群组名就是用户名
					strcpy((char *)member_call_obj.m_ucUStatus, (const char *)OEM_GROUP_MEMBER_ONLINE_IN);

					pocOemAttr.member_call_dir = 1;
					pocOemAttr.groupstatus_t = LVPOCOEMCOM_SIGNAL_GROUP_JOIN;
					lv_poc_activity_func_cb_set.member_call_open((void *)&member_call_obj);
				}
				break;
			}

			if(pocOemAttr.is_member_update == false)
			{
				//refr member
				pocOemAttr.pocGetMemberListCb(2, pocOemAttr.Msg_GroupMemberData_s.dwNum, &pocOemAttr.Msg_GroupMemberData_s);
				lvPocGuiOemCom_set_obtainning_state(false);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			}else
			{
				//update online or offline member
				bool is_member_status = false;
				pocOemAttr.is_member_update = false;

				if(0 == strcmp((char *)pocOemAttr.Msg_GroupMemberData_s.member[pocOemAttr.OemGroupMemberInfoBufIndex].m_ucUStatus, OEM_GROUP_MEMBER_OFFLINE))
				{
					is_member_status = false;
				}else
				{
					is_member_status = true;
				}
				lv_poc_activity_func_cb_set.member_list.set_state(NULL, (const char *)&pocOemAttr.Msg_GroupMemberData_s.member[pocOemAttr.OemGroupMemberInfoBufIndex].m_ucUName, (void *)&pocOemAttr.Msg_GroupMemberData_s.member[pocOemAttr.OemGroupMemberInfoBufIndex], is_member_status);

				if(pocOemAttr.is_OemGroupMemberInfoBuf_update == true)
				{
					OSI_LOGI(0, "[grouprefr]group member update");
					lv_poc_activity_func_cb_set.member_list.refresh(NULL);
				}
				pocOemAttr.is_OemGroupMemberInfoBuf_update = false;
			}

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_UPDATE:
		{
			//update current group member info
			lvPocGuiOemCom_Request_Groupx_MemeberInfo((char *)pocOemAttr.OemCurrentGroupInfo.m_ucGID);
			pocOemAttr.is_member_update = true;
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_DATA_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			if(pocOemAttr.is_member_update == false)
			{
				//Nodelink
				prv_lv_poc_oem_get_group_member_info(apopt->oembuf);
			}else
			{
				//check online or offline member
				prv_lv_poc_oem_update_member_status(apopt->oembuf);
			}

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			pocOemAttr.pocGetMemberListCb = (lv_poc_get_member_list_cb_t)ctx;
			osiTimerStart(pocOemAttr.Oem_Check_ack_timeout_timer, 3000);
			break;
		}
		case LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			pocOemAttr.pocGetMemberListCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}

}

static
void prvPocGuiOemTaskHandleMemberStatus(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
		{
			pocOemAttr.pocGetMemberStatusCb = (poc_get_member_status_cb)ctx;
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_MEMBER_INFO_IND:
		{
			OemCGroupMember *MemberStatus = (OemCGroupMember *)ctx;
			OSI_LOGI(0, "[song]member info enter");
			if(MemberStatus == NULL)
			{
				break;
			}

			if(0 == strcmp((char *)MemberStatus->m_ucUStatus, OEM_GROUP_MEMBER_ONLINE_IN))//online in group
			{
				OSI_LOGI(0, "[song]member info online");
				pocOemAttr.pocGetMemberStatusCb(1);
			}
			else if(0 == strcmp((char *)MemberStatus->m_ucUStatus, OEM_GROUP_MEMBER_ONLINE_OUT))//online out group
			{
				pocOemAttr.pocGetMemberStatusCb(2);
			}
			else//offline
			{
				OSI_LOGI(0, "[song]member info offline");
				pocOemAttr.pocGetMemberStatusCb(0);
			}

			break;
		}

		default:
		{
			break;
		}
	}
}

static
void prvPocGuiOemTaskHandleMemberSignalCall(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
		{
		    lv_poc_member_call_config_t *member_call_config = (lv_poc_member_call_config_t *)ctx;

			OemCGroupMember *member_call_obj = (OemCGroupMember *)member_call_config->members;

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

			if(member_call_config->enable == true)//request single call
			{
				if(pocOemAttr.member_call_dir == 0)
				{
					pocOemAttr.pocMemberCallCb = member_call_config->func;
					pocOemAttr.is_member_call = true;

					//request member signal call
					lvPocGuiOemCom_Request_Member_Call((char *)member_call_obj->m_ucUID);

					#if GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG
					OSI_LOGI(0, "[oemsinglecall]start request single call");
					#endif
				}else
				{
					member_call_config->func(0, 0);
					pocOemAttr.is_member_call = true;

					#if GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG
					OSI_LOGI(0, "[oemsinglecall]recive single call");
					#endif
				}
			}
			else//exit single
			{
				bool is_member_call = pocOemAttr.is_member_call;
				pocOemAttr.is_member_call = false;
				pocOemAttr.member_call_dir = 0;
				if(!is_member_call)
				{
					return;
				}

				member_call_config->func(1, 1);
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP, NULL);

				#if GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG
				OSI_LOGI(0, "[oemsinglecall]local exit single call");
				#endif
			}

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
		{
			//对方同意了单呼通话
		    if(pocOemAttr.pocMemberCallCb != NULL)
		    {
			    pocOemAttr.pocMemberCallCb(0, 0);
			    pocOemAttr.pocMemberCallCb = NULL;
		    }

			#if GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG
			OSI_LOGI(0, "[oemsinglecall]receive single call ack");
			#endif

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
		{
			pocOemAttr.is_member_call = false;
			pocOemAttr.member_call_dir = 0;

			poc_play_voice_one_time(LVPOCAUDIO_Type_Exit_Member_Call, 30, true);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"退出单呼", NULL);

			lv_poc_activity_func_cb_set.member_call_close();

			OSI_PRINTFI("[oemsinglecall](%s)(%d):exit single call", __func__, __LINE__);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP:
		{
			OSI_PRINTFI("[oemsinglecall](%s)(%d):exit single call, join new groupID[%s]", __func__, __LINE__, pocOemAttr.OemCurrentGroupInfoBuf.m_ucGID);
			//request join group
			lvPocGuiOemCom_Request_Join_Groupx((char *)pocOemAttr.OemCurrentGroupInfoBuf.m_ucGID);
			break;
		}

		default:
		{
			break;
		}
	}
}

static
void prvPocGuiOemTaskHandleMemberorGroupUpdateOpt(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_MEMBER_GROUP_INFO_UPDATE://85
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			PocGuiOemAtBufferAttr_t GUInfo;
			PocGuiOemAtTableAttr_t table[2] = {
				{5,2},//应答号--85
				{7,0},//结果码
			};

			GUInfo = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

			if(NULL != strstr(GUInfo.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUPINFOUPDATE))//组需更新
			{
				if(osiTimerIsRunning(pocOemAttr.Oem_GroupUpdate_timer))
				{
					osiTimerStop(pocOemAttr.Oem_GroupUpdate_timer);
				}
				osiTimerStart(pocOemAttr.Oem_GroupUpdate_timer,1000);

				#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
				OSI_LOGI(0, "[grouprefr](%d):receive grouprefr request", __LINE__);
				#endif
			}
			else if(NULL != strstr(GUInfo.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_MEMEBERINFOUPDATE))//成员需更新
			{
				if(osiTimerIsRunning(pocOemAttr.Oem_MemberUpdate_timer))
				{
					osiTimerStop(pocOemAttr.Oem_MemberUpdate_timer);
				}
				osiTimerStart(pocOemAttr.Oem_MemberUpdate_timer,1000);

				#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
				OSI_LOGI(0, "[grouprefr](%d):receive memberrefr request", __LINE__);
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

static
void prvPocGuiOemTaskHandleCallTalkingIDInd(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_CALL_TALKING_ID_IND:
		{

			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			PocGuiOemAtBufferAttr_t TalkingIDInfo;
			PocGuiOemAtTableAttr_t table[4] = {
				{5,2},//应答号--83
				{7,2},//讲话状态
				{9,8},//用户id
				{17,0},//用户名字(unicode码) *尾部填0为取到'\0'
			};

			TalkingIDInfo = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);
			//copy info
			strcpy(pocOemAttr.OemSpeakerInfo.OemUserStatus, TalkingIDInfo.buf[1].oembuf);
			strcpy(pocOemAttr.OemSpeakerInfo.OemUserID, TalkingIDInfo.buf[2].oembuf);
			lv_poc_oem_unicode_to_utf8_convert(TalkingIDInfo.buf[3].oembuf, (unsigned char*)pocOemAttr.OemSpeakerInfo.OemUserName);

			break;
		}

		default:
		{
			break;
		}
	}
}

static
void prvPocGuiOemTaskHandleMonitorOpt(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_MONITOR_GROUP_IND:
		{
			LvPocGuiIdtCom_monitor_group_t *pGroup = (LvPocGuiIdtCom_monitor_group_t *)ctx;

			#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
			OSI_LOGI(0, "[oemmonitorgroup](%d):enter request monitor", __LINE__);
			#endif
			if(pGroup->opt != LV_POC_GROUP_OPRATOR_TYPE_MONITOR
				|| pGroup->group_info == NULL
				|| pGroup->cb == NULL)
			{
				#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
				OSI_LOGI(0, "[oemmonitorgroup](%d):request monitor param error", __LINE__);
				#endif
				return;
			}

			OemCGroup *pGroupInfo = (OemCGroup *)pGroup->group_info;
			memcpy((void*)&pocOemAttr.pGroupMonitorInfo, (void *)pGroupInfo, sizeof(OemCGroup));
			if(NULL != strstr((const char *)pGroupInfo->m_ucGMonitor, OEM_GROUP_UNMONITOR))
			{
				#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
				OSI_LOGI(0, "[oemmonitorgroup](%d):start request monitor", __LINE__);
				#endif
				pocOemAttr.pocSetMonitorCb = pGroup->cb;
				lvPocGuiOemCom_Request_add_MonitorGroup((char *)&pGroupInfo->m_ucGID);
			}

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_MONITOR_GROUP_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			PocGuiOemAtBufferAttr_t MonitorGroupInfo;
			PocGuiOemAtTableAttr_t table[3] = {
				{5,2},//应答号--07
				{7,2},//结果码
				{9,0},//操作id
			};

			MonitorGroupInfo = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);
			if(0 == strcmp(MonitorGroupInfo.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FAILED))
			{
				pocOemAttr.pocSetMonitorCb(LV_POC_GROUP_OPRATOR_TYPE_MONITOR_FAILED);
				pocOemAttr.pocSetMonitorCb = NULL;
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"监听失败", (const uint8_t *)"");
				return;
			}

			#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
			OSI_LOGI(0, "[oemmonitorgroup](%d):(server)request monitor ack", __LINE__);
		    #endif
			//nv save
			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
			for(int i = 0; i < 5; i++)
			{
				if(NULL != strstr((char *)poc_config->nv_monitor_group[i].m_ucGID, "nomonitor"))
				{
					strcpy((char *)poc_config->nv_monitor_group[i].m_ucGID, (char *)pocOemAttr.pGroupMonitorInfo.m_ucGID);
					lv_poc_setting_conf_write();

					#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
					OSI_LOGXI(OSI_LOGPAR_ISI, 0, "[oemmonitorgroup](%d):request monitor to save nv,id(%s)(%d)", __LINE__, poc_config->nv_monitor_group[i].m_ucGID, i);
					#endif
					break;
				}
			}
			memset(&pocOemAttr.pGroupMonitorInfo, 0, sizeof(OemCGroup));
			pocOemAttr.pocSetMonitorCb(LV_POC_GROUP_OPRATOR_TYPE_MONITOR_OK);
			pocOemAttr.pocSetMonitorCb = NULL;
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_UNMONITOR_GROUP_IND:
		{
			LvPocGuiIdtCom_monitor_group_t *pGroup = (LvPocGuiIdtCom_monitor_group_t *)ctx;

			#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
			OSI_LOGI(0, "[oemmonitorgroup](%d):enter cannel monitor", __LINE__);
			#endif

			if(pGroup->opt != LV_POC_GROUP_OPRATOR_TYPE_UNMONITOR
				|| pGroup->group_info == NULL
				|| pGroup->cb == NULL)
			{
				#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
				OSI_LOGI(0, "[oemmonitorgroup](%d):cannel monitor param error", __LINE__);
				#endif
				return;
			}

			OemCGroup *pGroupInfo = (OemCGroup *)pGroup->group_info;
			memcpy((void *)&pocOemAttr.pGroupMonitorInfo, (void *)pGroupInfo, sizeof(OemCGroup));
			if(NULL != strstr((const char *)pGroupInfo->m_ucGMonitor, OEM_GROUP_MONITOR))
			{
				#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
				OSI_LOGI(0, "[oemmonitorgroup](%d):start cannel monitor", __LINE__);
				#endif
				pocOemAttr.pocSetMonitorCb = pGroup->cb;
				lvPocGuiOemCom_Request_cannel_MonitorGroup((char *)&pGroupInfo->m_ucGID);
			}

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_UNMONITOR_GROUP_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			PocGuiOemAtBufferAttr_t CannelMonitorGroupInfo;
			PocGuiOemAtTableAttr_t table[3] = {
				{5,2},//应答号--08
				{7,2},//结果码
				{9,0},//操作id
			};

			CannelMonitorGroupInfo = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);
			if(0 == strcmp(CannelMonitorGroupInfo.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FAILED))
			{
				pocOemAttr.pocSetMonitorCb(LV_POC_GROUP_OPRATOR_TYPE_UNMONITOR_FAILED);
				pocOemAttr.pocSetMonitorCb = NULL;
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"取消监听失败", (const uint8_t *)"");
				return;
			}

			#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
			OSI_LOGI(0, "[oemmonitorgroup](%d):(server)cannel monitor ack", __LINE__);
		    #endif
			//nv save
			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
			for(int i = 0; i < 5; i++)
			{
				if(0 == strcmp((char *)poc_config->nv_monitor_group[i].m_ucGID, (char *)pocOemAttr.pGroupMonitorInfo.m_ucGID))
				{
					strcpy((char *)poc_config->nv_monitor_group[i].m_ucGID, "nomonitor");
					lv_poc_setting_conf_write();

					#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
					OSI_LOGXI(OSI_LOGPAR_ISI, 0, "[oemmonitorgroup](%d):cannel monitor to save nv, id(%s)(%d)", __LINE__, poc_config->nv_monitor_group[i].m_ucGID, i);
					#endif
					break;
				}
			}
			memset(&pocOemAttr.pGroupMonitorInfo, 0, sizeof(OemCGroup));
			pocOemAttr.pocSetMonitorCb(LV_POC_GROUP_OPRATOR_TYPE_UNMONITOR_OK);
			pocOemAttr.pocSetMonitorCb = NULL;
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_POWERON_CHECK_MONITOR_GROUP_REP:
		{
			//Check to see if there is a listening group
			nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
			for(int i = pocOemAttr.oem_monitor_group_index; i < 5; i++)
			{
				if(NULL == strstr((char *)poc_config->nv_monitor_group[i].m_ucGID, "nomonitor"))
				{
					lvPocGuiOemCom_Request_add_MonitorGroup((char *)&poc_config->nv_monitor_group[i].m_ucGID);
					pocOemAttr.oem_monitor_group_index++;

					#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
					OSI_LOGXI(OSI_LOGPAR_ISI, 0, "[oemmonitorgroup](%d):power on monitor group, id(%s)(%d)", __LINE__, poc_config->nv_monitor_group[i].m_ucGID, i);
					#endif
					return;
				}
				else
				{
					pocOemAttr.oem_monitor_group_index++;

					#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
					OSI_LOGI(0, "[oemmonitorgroup]no monitor group");
					#endif
				}
			}
			//oem system status
			pocOemAttr.is_oem_system_status = true;
			pocOemAttr.oem_monitor_group_index = 0;
#if GUIIDTCOM_OEM_SELF_MSG
			lvPocGuiOemCom_set_response_speed(LVPOCOEMCOM_TYPE_REPPONSE_SPEED_NORMAL);
#endif
			#if GUIIDTCOM_OEMMONITORGROUP_DEBUG_LOG
			OSI_LOGI(0, "[oemmonitorgroup](%d):check monitor group success", __LINE__);
			#endif
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiOemTaskHandleHeadsetInsert(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUIOEMCOM_SIGNAL_HEADSET_INSERT:
      {
         lv_poc_set_headset_status(true);

		 if(!lvPocGuiOemCom_get_listen_status()
		 	&& !lvPocGuiOemCom_get_speak_status())
		 {
         	lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
         	lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"耳机插入", (const uint8_t *)"");
		 }
		 break;
      }

      case LVPOCGUIOEMCOM_SIGNAL_HEADSET_PULL_OUT:
      {
         lv_poc_set_headset_status(false);

		 if(!lvPocGuiOemCom_get_listen_status()
		 	&& !lvPocGuiOemCom_get_speak_status())
		 {
         	lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
         	lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"耳机拔出", (const uint8_t *)"");
		 }
		 break;
      }

      default:
      {
         break;
      }
   }
}

static void prvPocGuiOemTaskHandleDelayOpenPA(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUIOEMCOM_SIGNAL_DELAY_OPEN_PA_IND:
      {
	  	 if(ctx == 0
		 	|| !lvPocGuiOemCom_get_listen_status())
	  	 {
	     	break;
		 }
		 osiTimerStart(pocOemAttr.Oem_Openpa_timer, ctx);
		 break;
      }

      default:
      {
         break;
      }
   }
}

static void prvPocGuiOemTaskHandleCallBrightScreen(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUIOEMCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER:
      {
		 if(!poc_get_lcd_status())
		 {
			 poc_set_lcd_status(true);
			 pocOemAttr.call_curscr_state = LVPOCOEMCOM_CALL_LASTSCR_STATE_CLOSE;
			 OSI_PRINTFI("[callscr](%s)(%d):cur scr close", __func__, __LINE__);
		 }
		 else
		 {
			 pocOemAttr.call_curscr_state = LVPOCOEMCOM_CALL_LASTSCR_STATE_OPEN;
			 OSI_PRINTFI("[callscr](%s)(%d):cur scr open", __func__, __LINE__);
		 }
		 poc_UpdateLastActiTime();
		 osiTimerStartPeriodic(pocOemAttr.Oem_BrightScreen_timer, 4000);
		 pocOemAttr.call_briscr_dir = LVPOCOEMCOM_CALL_DIR_TYPE_ENTER;
		 break;
      }

	  case LVPOCGUIOEMCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT:
	  {
	  	 if(ctx == 0)
	  	 {
			break;
		 }

		 switch(ctx)
		 {
			 case LVPOCOEMCOM_CALL_DIR_TYPE_SPEAK:
			 {
				 pocOemAttr.call_briscr_dir = LVPOCOEMCOM_CALL_DIR_TYPE_SPEAK;
				 osiTimerIsRunning(pocOemAttr.Oem_BrightScreen_timer) ? \
	     		 osiTimerStop(pocOemAttr.Oem_BrightScreen_timer) : 0;
				 osiTimerStart(pocOemAttr.Oem_BrightScreen_timer, 4000);
				 OSI_PRINTFI("[callscr](%s)(%d):speak stop", __func__, __LINE__);
				 break;
			 }


			 case LVPOCOEMCOM_CALL_DIR_TYPE_LISTEN:
			 {
				 pocOemAttr.call_briscr_dir = LVPOCOEMCOM_CALL_DIR_TYPE_LISTEN;
				 osiTimerIsRunning(pocOemAttr.Oem_BrightScreen_timer) ? \
	     		 osiTimerStop(pocOemAttr.Oem_BrightScreen_timer) : 0;
				 osiTimerStart(pocOemAttr.Oem_BrightScreen_timer, 4000);
				 OSI_PRINTFI("[callscr](%s)(%d):listen stop", __func__, __LINE__);
				 break;
			 }
		 }
		 break;
	  }

	  case LVPOCGUIOEMCOM_SIGNAL_CALL_BRIGHT_SCREEN_BREAK:
	  {
	  	 pocOemAttr.call_briscr_dir != LVPOCOEMCOM_CALL_DIR_TYPE_ENTER ? \
	     osiTimerIsRunning(pocOemAttr.Oem_BrightScreen_timer) ? \
	     osiTimerStop(pocOemAttr.Oem_BrightScreen_timer) : 0 \
	     : 0;
		 OSI_PRINTFI("[callscr](%s)(%d):break", __func__, __LINE__);
		 break;
	  }

      default:
      {
         break;
      }
   }
}

static void prvPocGuiOemTaskHandleOther(uint32_t id, uint32_t ctx)
{
    switch(id)
    {
        case LVPOCGUIOEMCOM_SIGNAL_STOP_PLAYER_VOICE:
        {
			if(lv_poc_stop_player_voice())
			{
				OEM_TTS_Status_CB(false);//release
				lvPocGuiOemCom_CriRe_Msg(LVPOCGUIOEMCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER, NULL);
				switch(pocOemAttr.call_dir_type)
				{
					case LVPOCOEMCOM_CALL_DIR_TYPE_IND:
					{
						lvPocGuiOemCom_Request_Start_Speak();
						break;
					}

					case LVPOCOEMCOM_CALL_DIR_TYPE_REP:
					{
						lvPocGuiOemCom_CriRe_Msg(LVPOCGUIOEMCOM_SIGNAL_LISTEN_SPEAKER_REP, NULL);
						break;
					}
				}
			}
			else
			{
				switch(pocOemAttr.call_dir_type)
				{
					case LVPOCOEMCOM_CALL_DIR_TYPE_IND:
					{
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"对讲异常", NULL);
						break;
					}

					case LVPOCOEMCOM_CALL_DIR_TYPE_REP:
					{
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"接听异常", NULL);
						break;
					}
				}
			}
        	break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SET_STOP_PLAYER_TTS_VOICE:
		{
			if(lvPocGuiOemCom_get_login_status() == LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
			{
				OEM_TTS_Status_CB(false);
			}
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SET_START_PLAYER_TTS_VOICE:
		{
			if(lvPocGuiOemCom_get_login_status() == LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
			{
				OEM_TTS_Status_CB(true);
			}
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_LOOPBACK_RECORDER_IND:
		{
			lv_poc_start_recordwriter();
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_LOOPBACK_PLAYER_IND:
		{
			lv_poc_start_playfile();
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_TEST_VLOUM_PLAY_IND:
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Test_Volum, 50, false);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND:
		{
			if(lvPocGuiOemCom_get_login_status() == LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
			{
				OSI_PRINTFI("[timeout](%s)(%d):stop", __func__, __LINE__);
				lvPocGuiOemCom_stop_check_ack();
			}
			break;
		}

		default:
		{
			break;
		}
    }
}

static
void LvGuiOemCom_AutoLogin_timer_cb(void *ctx)
{
	nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
	OEMPOC_AT_Recv(poc_config->oemipaccoutapassword, poc_config->oemipaccoutapasswordlen);
	lvPocGuiOemCom_Request_PocParam();
}

static
void LvGuiOemCom_Group_update_timer_cb(void *ctx)
{
	OSI_LOGI(0, "[grouprefr](%d):start request grouprefr", __LINE__);
}

static
void LvGuiOemCom_Member_update_timer_cb(void *ctx)
{
	extern bool lv_poc_is_memberlist_activity(void);
	if(!lv_poc_is_memberlist_activity())
	{
		return;
	}
	lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_UPDATE, NULL);
	OSI_LOGI(0, "[grouprefr](%d):start request memberrefr", __LINE__);
}

static
void LvGuiOemCom_poweron_request_monitor_group_timer_cb(void *ctx)
{
	//start check monitor group
	lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_POWERON_CHECK_MONITOR_GROUP_REP, NULL);
}

static
void LvGuiOemCom_nologin_timer_cb(void *ctx)
{
	if(lvPocGuiOemCom_get_login_status() != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_No_Login, 50, false);
		lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_1500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
	}
}

static void LvGuiOemCom_check_ack_timeout_timer_cb(void *ctx)
{
	OSI_PRINTFI("[timeout](%s)(%d):timeout no get info", __func__, __LINE__);
    if(pocOemAttr.pocGetMemberListCb != NULL)
    {
        pocOemAttr.pocGetMemberListCb(0, 0, NULL);
        pocOemAttr.pocGetMemberListCb = NULL;
    }
    else if(pocOemAttr.pocGetGroupListCb != NULL)
    {
        pocOemAttr.pocGetGroupListCb(0, 0, NULL);
        pocOemAttr.pocGetGroupListCb = NULL;
    }
	else if(pocOemAttr.pocBuildTempGrpCb != NULL)
    {
        pocOemAttr.pocBuildTempGrpCb(0);
        pocOemAttr.pocBuildTempGrpCb = NULL;
    }
	pocOemAttr.loginstatus_t = LVPOCOEMCOM_SIGNAL_LOGIN_FAILED;
	lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_NOLOGIN);
	//auto login
	osiTimerIsRunning(pocOemAttr.Oem_AutoLogin_timer) ? 0 : osiTimerStart(pocOemAttr.Oem_AutoLogin_timer, 5000);
}

static void LvGuiOemCom_open_pa_timer_cb(void *ctx)
{
	OSI_LOGI(0, "[pa][timecb](%d):open pa", __LINE__);
	poc_set_ext_pa_status(true);
}

static void LvGuiOemCom_Call_Bright_Screen_timer_cb(void *ctx)
{
	OSI_PRINTFI("[CallBriScr][timecb](%s)(%d):cb, briscr_dir(%d), curscr_state(%d)", __func__, __LINE__, pocOemAttr.call_briscr_dir, pocOemAttr.call_curscr_state);
	switch(pocOemAttr.call_briscr_dir)
	{
		case LVPOCOEMCOM_CALL_DIR_TYPE_ENTER:
		{
			poc_UpdateLastActiTime();
			break;
		}

		case LVPOCOEMCOM_CALL_DIR_TYPE_SPEAK:
		case LVPOCOEMCOM_CALL_DIR_TYPE_LISTEN:
		{
			osiTimerStop(pocOemAttr.Oem_BrightScreen_timer);

			switch(pocOemAttr.call_curscr_state)
			{
				case LVPOCOEMCOM_CALL_LASTSCR_STATE_OPEN:
				{
					OSI_PRINTFI("[callscr](%s)(%d):scr open, scr on continue", __func__, __LINE__);
					poc_UpdateLastActiTime();//continue last'state
					break;
				}

				case LVPOCOEMCOM_CALL_LASTSCR_STATE_CLOSE:
				{
					OSI_PRINTFI("[callscr](%s)(%d):scr close, scr off", __func__, __LINE__);
					poc_set_lcd_status(false);
					break;
				}
			}
			pocOemAttr.call_briscr_dir = LVPOCOEMCOM_CALL_DIR_TYPE_ENTER;
			pocOemAttr.call_curscr_state = LVPOCOEMCOM_CALL_LASTSCR_STATE_START;
			break;
		}

		default:
		{
			break;
		}
	}
}

void PocGuiOemComStart(void)
{
	pocOemAttr.thread = osiThreadCreate(
			"pocGuiOemCom", pocGuiOemComTaskEntry, NULL,
			APPTEST_THREAD_PRIORITY, APPTEST_STACK_SIZE,
			APPTEST_EVENT_QUEUE_SIZE);
	pocOemAttr.oemapthread = osiThreadCreate(
			"pocGuiOemAp", pocGuiOemApTaskEntry, NULL,
			OSI_PRIORITY_NORMAL, OEMAP_STACK_SIZE,
			OEMAP_EVENT_QUEUE_SIZE);

	pocOemAttr.Oem_AutoLogin_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_AutoLogin_timer_cb, NULL);//误触碰定时器
	pocOemAttr.Oem_GroupUpdate_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_Group_update_timer_cb, NULL);//群组刷新定时器
	pocOemAttr.Oem_MemberUpdate_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_Member_update_timer_cb, NULL);//成员刷新定时器
	pocOemAttr.Oem_PoweronRequestMonitorGroup_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_poweron_request_monitor_group_timer_cb, NULL);//上电检查监听群组定时器
	pocOemAttr.Oem_Nologin_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_nologin_timer_cb, NULL);//未登录定时器
	pocOemAttr.Oem_Check_ack_timeout_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_check_ack_timeout_timer_cb, NULL);
	pocOemAttr.Oem_Openpa_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_open_pa_timer_cb, NULL);
	pocOemAttr.Oem_BrightScreen_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_Call_Bright_Screen_timer_cb, NULL);
}

void lvPocGuiOemCom_Init(void)
{
	memset(&pocOemAttr, 0, sizeof(PocGuiOemComAttr_t));
	//申请队列
	//参数 1:队列深度
	//参数 2:队列项内容大小
	pocOemAttr.xQueue = osiMessageQueueCreate(32, sizeof(struct _PocGuiOemComAttr_t*));

#if GUIIDTCOM_OEM_SELF_MSG
	bool status = OemInitQueue(&OemQueue);
	if(!status){
		OemDestroyQueue(&OemQueue);
	}
#endif
	//power on status
	pocOemAttr.loginstatus_t = LVPOCOEMCOM_SIGNAL_LOGIN_FAILED;
	pocOemAttr.groupstatus_t = LVPOCOEMCOM_SIGNAL_GROUP_EXIT;

	PocGuiOemComStart();
}

static
void pocGuiOemComTaskEntry(void *argument)
{
	osiEvent_t event = {0};
	pocOemAttr.isReady = true;
	//Oem init
	OEM_PocInit();
    for(int i = 0; i < 1; i++)
    {
	    osiThreadSleep(1000);
    }

	//have connect network
	pocOemAttr.pocnetstatus = true;
	OEMNetworkStatusChange(pocOemAttr.pocnetstatus);

	lvPocGuiOemCom_Request_Set_Tone(OEM_FUNC_OPEN);
	lvPocGuiOemCom_Request_Set_Tone_Volum("32");//set volum:0-64(hex)
	lvPocGuiOemCom_Request_Set_Log(OEM_FUNC_CLOSE);
	lvPocGuiOemCom_Request_Set_AudioQuility("02");//8k
	lvPocGuiOemCom_Request_Set_HeartBeat("1E");//30s
    while(1)
    {
    	if(!osiEventWait(pocOemAttr.thread, &event))
		{
			continue;
    	}

		if(event.id != 200)
		{
			continue;
		}

		switch(event.param1)
		{
			case LVPOCGUIOEMCOM_SIGNAL_OPENPOC_IND:
			case LVPOCGUIOEMCOM_SIGNAL_OPENPOC_REP:
			case LVPOCGUIOEMCOM_SIGNAL_AP_POC_START:
			{
				prvPocGuiOemTaskHandleOpenPOC(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_AP_POC_IND:
			case LVPOCGUIOEMCOM_SIGNAL_AP_POC_REP:
			{
				prvPocGuiOemTaskHandleMsgCB(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_LOGIN_IND:
			case LVPOCGUIOEMCOM_SIGNAL_LOGIN_REP:
			case LVPOCGUIOEMCOM_SIGNAL_EXIT_IND:
			case LVPOCGUIOEMCOM_SIGNAL_EXIT_REP:
			{
				prvPocGuiOemTaskHandleLogin(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_SETPOC_IND:
			case LVPOCGUIOEMCOM_SIGNAL_SETPOC_REP:
			{
				prvPocGuiOemTaskHandleSetPOC(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_IND:
			case LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_REP:
			case LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_IND:
			case LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_REP:
			{
				prvPocGuiOemTaskHandleSpeak(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_LISTEN_START_REP:
    		case LVPOCGUIOEMCOM_SIGNAL_LISTEN_STOP_REP:
    		case LVPOCGUIOEMCOM_SIGNAL_LISTEN_SPEAKER_REP:
			{
				prvPocGuiOemTaskHandleListen(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_JOIN_GROUP_REP:
			{
				prvPocGuiOemTaskHandleGroupUpdateOpt(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_INFO_IN:
			case LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_INFO_REP:
			{
				prvPocGuiOemTaskHandlePoweronMemberOpt(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
			case LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
			case LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_UPDATE:
			case LVPOCGUIOEMCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
			case LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
			case LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_DATA_REP:
			{
				prvPocGuiOemTaskHandleMemberList(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_IND:
			case LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_REP:
			case LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_UPDATE:
			case LVPOCGUIOEMCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
			case LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
			case LVPOCGUIOEMCOM_SIGNAL_GROUPLIST_DATA_REP:
			{
				prvPocGuiOemTaskHandleGroupList(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_IND:
			case LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_REP:
			case LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_GET_MEMBER_REP:
			case LVPOCGUIOEMCOM_SIGNAL_BIUILD_TEMPGRP_EXIT_REP:
			case LVPOCGUIOEMCOM_SIGNAL_REGISTER_BIUILD_TEMPGRP_CB_IND:
			case LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_BIUILD_TEMPGRP_CB_IND:
			{
				prvPocGuiIdtTaskHandleBuildTempGrp(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_SET_CURRENT_GROUP_IND:
			case LVPOCGUIOEMCOM_SIGNAL_SET_CURRENT_GROUP_REP:
    		case LVPOCGUIOEMCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
    		case LVPOCGUIOEMCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
    		{
				prvPocGuiOemTaskHandleSetCurrentGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
			case LVPOCGUIOEMCOM_SIGNAL_MEMBER_INFO_IND:
			{
				prvPocGuiOemTaskHandleMemberStatus(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
			case LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
			case LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
			case LVPOCGUIOEMCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP:
			{
				prvPocGuiOemTaskHandleMemberSignalCall(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_MEMBER_GROUP_INFO_UPDATE:
			{
				prvPocGuiOemTaskHandleMemberorGroupUpdateOpt(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_CALL_TALKING_ID_IND:
			{
				prvPocGuiOemTaskHandleCallTalkingIDInd(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_MONITOR_GROUP_IND:
    		case LVPOCGUIOEMCOM_SIGNAL_MONITOR_GROUP_REP:
   			case LVPOCGUIOEMCOM_SIGNAL_UNMONITOR_GROUP_IND:
    		case LVPOCGUIOEMCOM_SIGNAL_UNMONITOR_GROUP_REP:
			case LVPOCGUIOEMCOM_SIGNAL_POWERON_CHECK_MONITOR_GROUP_REP:
   			{
   				prvPocGuiOemTaskHandleMonitorOpt(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_HEADSET_INSERT:
			case LVPOCGUIOEMCOM_SIGNAL_HEADSET_PULL_OUT:
			{
				prvPocGuiOemTaskHandleHeadsetInsert(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_DELAY_OPEN_PA_IND:
	        {
	            prvPocGuiOemTaskHandleDelayOpenPA(event.param1, event.param2);
	            break;
	     	}

			case LVPOCGUIOEMCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER:
			case LVPOCGUIOEMCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT:
			case LVPOCGUIOEMCOM_SIGNAL_CALL_BRIGHT_SCREEN_BREAK:
			{
				prvPocGuiOemTaskHandleCallBrightScreen(event.param1, event.param2);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_STOP_PLAYER_VOICE:
			case LVPOCGUIOEMCOM_SIGNAL_SET_STOP_PLAYER_TTS_VOICE:
			case LVPOCGUIOEMCOM_SIGNAL_SET_START_PLAYER_TTS_VOICE:
			case LVPOCGUIOEMCOM_SIGNAL_LOOPBACK_RECORDER_IND:
			case LVPOCGUIOEMCOM_SIGNAL_LOOPBACK_PLAYER_IND:
			case LVPOCGUIOEMCOM_SIGNAL_TEST_VLOUM_PLAY_IND:
			case LVPOCGUIOEMCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND:
   			{
   				prvPocGuiOemTaskHandleOther(event.param1, event.param2);
				break;
			}

			default:
			{
				break;
			}
		}
    }
}

static
void pocGuiOemApTaskEntry(void *argument)
{
#if GUIIDTCOM_OEM_SELF_MSG
	PocGuiOemApSendAttr_t oemopt = {0};
	lvPocGuiOemCom_set_response_speed(LVPOCOEMCOM_TYPE_REPPONSE_SPEED_NORMAL);
#else
    PocGuiOemApSendAttr_t *apopt = NULL;
#endif
    while(1)
    {
#if GUIIDTCOM_OEM_SELF_MSG
		osiThreadSleepRelaxed(pocOemAttr.oem_response_time, 60000);//60s

		if(!OemDeQueue(&OemQueue, &oemopt))
		{
			continue;
		}

		if(NULL != oemopt.oembuf)
		{
			lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_AP_POC_REP, (void *)&oemopt);
		}
#else
		if(!osiMessageQueueGet(pocOemAttr.xQueue, (void *)&apopt))//*(&apopt) is param's address
        {
            continue;
        }

        if(NULL != apopt->oembuf)
        {
            //send data
            if(!lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_AP_POC_REP, (void *)apopt))
            {

			}
			else
			{
				if(apopt == NULL)
				{
					free(apopt);
					apopt = NULL;
				}
			}
        }

#endif
    }
}

bool lvPocGuiOemCom_MessageQueue(osiMessageQueue_t *mq, const void *msg)
{
	if(pocOemAttr.oemapthread == NULL)
	{
		OSI_LOGI(0, "[msg]Thread NULL");
		return false;
	}

	return osiMessageQueuePut(mq, (void *)msg);
}

static
OemCGroup prv_lv_poc_join_current_group_infomation(char * Information)
{
	OemCGroup GInfo;

	PocGuiOemAtTableAttr_t table[4] = {
		{5,2},//组应答号--86
		{7,2},//临时组标志--(01-临时组) (00-固定组)
		{9,8},//群组id
		{17,0},//群组名字(unicode码) *尾部填0为取到'\0'
	};
	PocGuiOemAtBufferAttr_t joingroupbuf;

	joingroupbuf = prv_lv_poc_oem_get_global_atcmd_info(Information, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

	strcpy((char *)GInfo.m_ucGTemporary, joingroupbuf.buf[1].oembuf);
	strcpy((char *)GInfo.m_ucGID, joingroupbuf.buf[2].oembuf);
	lv_poc_oem_unicode_to_utf8_convert(joingroupbuf.buf[3].oembuf, GInfo.m_ucGName);

	#if 0
	OSI_LOGXI(OSI_LOGPAR_S, 0, "[grpinfo]OemGroupID %s", GInfo.m_ucGID);
	OSI_LOGXI(OSI_LOGPAR_S, 0, "[grpinfo]OemGroupName %s", GInfo.m_ucGName);
	OSI_LOGXI(OSI_LOGPAR_S, 0, "[grpinfo]OemGroupGTemporary %s", GInfo.m_ucGTemporary);
	#endif

	return GInfo;
}

static
PocGuiOemUserAttr_t prv_lv_poc_power_on_get_self_infomation(char * Information)
{
	PocGuiOemUserAttr_t UInfo;

	PocGuiOemAtTableAttr_t table[4] = {
		{5,2},//登录答号--82
		{7,2},//用户状态
		{9,8},//用户id
		{17,0},//用户名字(unicode码) *尾部填0为取到'\0'
	};
	PocGuiOemAtBufferAttr_t joingroupbuf;

	joingroupbuf = prv_lv_poc_oem_get_global_atcmd_info(Information, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

	strcpy((char *)UInfo.OemUserStatus, joingroupbuf.buf[1].oembuf);
	strcpy((char *)UInfo.OemUserID, joingroupbuf.buf[2].oembuf);
	lv_poc_oem_unicode_to_utf8_convert(joingroupbuf.buf[3].oembuf, (unsigned char*)UInfo.OemUserName);

	#if 0
	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]OemUserID %s", UInfo.OemUserID);
	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]OemUserName %s", UInfo.OemUserName);
	#endif

	return UInfo;
}

static
void prv_lv_poc_oem_get_group_list_info(void *information)
{
	PocGuiOemAtTableAttr_t table[7] = {
		{5,2},//组应答号--80
		{7,2},//此组是否被监听--(00-否) (01-是)
		{9,4},//操作ID--忽略
		{13,4},//组索引--第几个组
		{17,8},//组id
		{25,4},//组成员个数
		{29,0},//组名字(unicode码) *尾部填0为取到'\0'
	};
	PocGuiOemAtBufferAttr_t grouplistbuf;
	OemCGroup prv_group_info = {0};
	memset(&grouplistbuf, 0, sizeof(PocGuiOemAtBufferAttr_t));
	memset(&prv_group_info, 0, sizeof(OemCGroup));

	grouplistbuf = prv_lv_poc_oem_get_global_atcmd_info(information, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

	strcpy((char *)prv_group_info.m_ucGMonitor, grouplistbuf.buf[1].oembuf);
	prv_group_info.m_ucGIndex = lv_poc_oemdata_hexstrtodec(grouplistbuf.buf[3].oembuf, sizeof(grouplistbuf.buf[3].oembuf)/sizeof(char));
	strcpy((char *)prv_group_info.m_ucGID, grouplistbuf.buf[4].oembuf);
	prv_group_info.m_ucGMemberNum = lv_poc_oemdata_hexstrtodec(grouplistbuf.buf[5].oembuf, sizeof(grouplistbuf.buf[5].oembuf)/sizeof(char));
	lv_poc_oem_unicode_to_utf8_convert(grouplistbuf.buf[6].oembuf, prv_group_info.m_ucGName);

	memcpy((void *)&pocOemAttr.GroupData_s.m_Group[pocOemAttr.OemGroupInfoBufnumber], (void *)&prv_group_info, sizeof(OemCGroup));
	pocOemAttr.OemGroupInfoBufnumber++;
}

static
void prv_lv_poc_oem_get_group_member_info(void *information)
{
	PocGuiOemAtTableAttr_t table[6] = {
		{5,2},//组应答号--81
		{7,2},//成员状态--(01-离线) (02-在线~不在此群组) (03-在线~在此群组中)
		{9,4},//操作ID--忽略
		{13,4},//用户索引--第几个用户
		{17,8},//用户id
		{25,0},//用户名字(unicode码) *尾部填0为取到'\0'
	};
	PocGuiOemAtBufferAttr_t groupxmemberlistbuf = {0};
	OemCGroupMember prv_user_info = {0};
	memset(&groupxmemberlistbuf, 0, sizeof(PocGuiOemAtBufferAttr_t));
	memset(&prv_user_info, 0, sizeof(OemCGroupMember));

	groupxmemberlistbuf = prv_lv_poc_oem_get_global_atcmd_info(information, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

	strcpy((char *)prv_user_info.m_ucUStatus, groupxmemberlistbuf.buf[1].oembuf);
	prv_user_info.m_ucUIndex = lv_poc_oemdata_hexstrtodec(groupxmemberlistbuf.buf[3].oembuf, sizeof(groupxmemberlistbuf.buf[3].oembuf)/sizeof(char));
	strcpy((char *)prv_user_info.m_ucUID, groupxmemberlistbuf.buf[4].oembuf);
	lv_poc_oem_unicode_to_utf8_convert(groupxmemberlistbuf.buf[5].oembuf, prv_user_info.m_ucUName);

	memcpy((void *)&pocOemAttr.Msg_GroupMemberData_s.member[pocOemAttr.OemGroupMemberInfoBufnumber], (void *)&prv_user_info, sizeof(OemCGroupMember));
	pocOemAttr.OemGroupMemberInfoBufnumber++;
}

static
void prv_lv_poc_oem_update_member_status(void *information)
{
	PocGuiOemAtTableAttr_t table[6] = {
		{5,2},//组应答号--81
		{7,2},//成员状态--(01-离线) (02-在线~不在此群组) (03-在线~在此群组中)
		{9,4},//操作ID--忽略
		{13,4},//用户索引--第几个用户
		{17,8},//用户id
		{25,0},//用户名字(unicode码) *尾部填0为取到'\0'
	};
	PocGuiOemAtBufferAttr_t groupxmemberstatus = {0};
	OemCGroupMember prv_userstatus_info = {0};

	memset(&groupxmemberstatus, 0, sizeof(PocGuiOemAtBufferAttr_t));
	memset(&prv_userstatus_info, 0, sizeof(OemCGroupMember));

	groupxmemberstatus = prv_lv_poc_oem_get_global_atcmd_info(information, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

	strcpy((char *)prv_userstatus_info.m_ucUStatus, groupxmemberstatus.buf[1].oembuf);
	prv_userstatus_info.m_ucUIndex = lv_poc_oemdata_hexstrtodec(groupxmemberstatus.buf[3].oembuf, sizeof(groupxmemberstatus.buf[3].oembuf)/sizeof(char));
	strcpy((char *)prv_userstatus_info.m_ucUID, groupxmemberstatus.buf[4].oembuf);
	lv_poc_oem_unicode_to_utf8_convert(groupxmemberstatus.buf[5].oembuf, prv_userstatus_info.m_ucUName);

	for(int i = 0; i < pocOemAttr.Msg_GroupMemberData_s.dwNum ; i++)
	{
		if((0 == strcmp((char *)prv_userstatus_info.m_ucUID, (char *)pocOemAttr.Msg_GroupMemberData_s.member[i].m_ucUID))
			&& (0 != strcmp((char *)prv_userstatus_info.m_ucUStatus, (char *)pocOemAttr.Msg_GroupMemberData_s.member[i].m_ucUStatus)))
		{//id same && status different
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[grouprefr]member status ack[%s]", prv_userstatus_info.m_ucUStatus);

			memcpy(&pocOemAttr.Msg_GroupMemberData_s.member[i], &prv_userstatus_info, sizeof(OemCGroupMember));
			pocOemAttr.OemGroupMemberInfoBufIndex = i;
			pocOemAttr.is_OemGroupMemberInfoBuf_update = true;
			break;
		}
	}
}

bool lvPocGuiOemCom_Msg(LvPocGuiOemCom_SignalType_t signal, void * ctx)
{
	if(pocOemAttr.thread == NULL
		|| !pocOemAttr.isReady)
	{
		OSI_LOGI(0, "[Event]Thread NULL");
		return false;
	}

	if(signal < LVPOCGUIOEMCOM_SIGNAL_CIT_ENTER_IND
		&& (lvPocGuiOemCom_cit_status(POC_TMPGRP_READ) == POC_CIT_ENTER))
	{
		return false;
	}

	osiEvent_t event = {0};

	event.id = 200;
	event.param1 = signal;
	event.param2 = (uint32_t)ctx;

	return osiEventSend(pocOemAttr.thread, &event);
}

bool lvPocGuiOemCom_CriRe_Msg(LvPocGuiOemCom_SignalType_t signal, void * ctx)
{
	if(pocOemAttr.thread == NULL)
	{
		OSI_LOGI(0, "[Event]Thread NULL");
		return false;
	}

	if(signal < LVPOCGUIOEMCOM_SIGNAL_CIT_ENTER_IND
		&& (lvPocGuiOemCom_cit_status(POC_TMPGRP_READ) == POC_CIT_ENTER))
	{
		return false;
	}

	osiEvent_t event = {0};
	uint32_t osiECtc = osiEnterCritical();

	event.id = 200;
	event.param1 = signal;
	event.param2 = (uint32_t)ctx;

	osiExitCritical(osiECtc);

	return osiEventSend(pocOemAttr.thread, &event);
}

/*
	  name : lv_poc_oemdata_strtostrhex
	 param : none
	author : wangls
  describe : 字符串->16进制字符串
	  date : 2020-09-14
*/
void lv_poc_oemdata_strtostrhex(char *pszDest, char *pbSrc, int nLen)
{
	char ddl, ddh;
	uint32_t i;
	for (i = 0; i < nLen; i++)
	{
		ddh = 48 + pbSrc[i] / 16;
		ddl = 48 + pbSrc[i] % 16;
		if (ddh > 57) ddh = ddh + 7;
		if (ddl > 57) ddl = ddl + 7;
		pszDest[i * 2] = ddh;
		pszDest[i * 2 + 1] = ddl;
	}
	pszDest[nLen * 2] = '\0';
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
	int i = 0;
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
	  name : lv_poc_unicode_to_gb2312_convert
	 param : none
	author : wangls
  describe : unicode to gb2312
	  date : 2020-09-14
*/
void lv_poc_unicode_to_gb2312_convert(char *pUserInput, char *pUserOutput)
{
	wchar_t oembufBrr[64] = {0};
	wchar_t oembufArr[64] = {0};
	wchar_t wstr[64];
	char str[64];

	#if 0
	wchar_t oemwstr[] = {0x98de,0x56fe,0x540c,0x8f89,0x6d4b,0x8bd5,0x0031,0x7ec4};//    ori:{0x52B3,0x788c,0};
	char oemstr[64];
	wcstombs(oemstr, oemwstr, sizeof(oemstr)/sizeof(char));
	int oemsize = lv_poc_persist_ssl_hashKeyConvert(pUserInput, oembufBrr);
	printf("[song]oemstr%s \n", oemstr);
	#endif

	int oemsize = lv_poc_persist_ssl_hashKeyConvert(pUserInput, oembufBrr);
	for(int i = 0; i < oemsize; i++)
	{
		oembufArr[i] = (oembufBrr[i])|(oembufBrr[i]&0x0f);
	}
	wcscpy(wstr, (wchar_t *)oembufArr);
	setlocale(LC_ALL,"");
	wcstombs(str, wstr, sizeof(str)/sizeof(char));

	strcpy(pUserOutput, str);//copy
}

/*
	  name : lv_poc_oem_unicode_to_utf8_convert
	 param : none
	author : wangls
  describe : oem unicode to utf8
	  date : 2020-09-14
*/
int lv_poc_oem_unicode_to_utf8_convert(char *pUserInput, unsigned char *pUserOutput)
{
	unsigned char oembufArr[64] = {0};
	int oemlen = 0;
	int alllen = 0;
	int k =0;

	wchar_t oembufBrr[64] = {0};

	int oemsize = lv_poc_persist_ssl_hashKeyConvert(pUserInput, oembufBrr);
	for(int i = 0; i < oemsize; i++)
	{
		oemlen = lv_poc_enc_unicode_to_utf8_one(oembufBrr[i], oembufArr, sizeof(oembufBrr)/sizeof(char));
		if(oemlen)
		{
			for(int j = 0; j < oemlen; j++)
			{
				pUserOutput[k++] = oembufArr[j];
			}
			alllen+=oemlen;
		}
	}

	return alllen;
}

/*
	  name : lv_poc_oemdata_strtodec
	 param : none
	author : wangls
  describe : 十进制字符串转十进制
	  date : 2020-09-14
*/
uint64_t lv_poc_oemdata_strtodec(char *data,uint32_t len)
{
    uint32_t i;
    unsigned long long n = 0;
    for (i = 0; i<len; ++i)
    {
        if(data[i] >= '0' && data[i] <= '9')
        {
            n = 10 * n + (data[i] - '0');
        }
    }
     return n;
}

/*
	  name : lv_poc_oemdata_hexstrtodec
	 param : none
	author : wangls
  describe : 十六进制字符串转十进制
	  date : 2020-10-19
*/
uint64_t lv_poc_oemdata_hexstrtodec(char *s, uint32_t len)
{
	uint64_t temp = 0;

	temp  = strtol(s, NULL, 16);

	return temp;
}
static void lv_poc_extract_account(char *pocIpAccoutaPassword,char *account)
{
	if(pocIpAccoutaPassword == NULL || account == NULL)
		return;
	char *temp1 = NULL;
	char *temp2 = NULL;
	char *id = NULL;
	int len = 0;
	temp1 = strchr(pocIpAccoutaPassword, ';');
	temp1 ? temp1++ : 0;
	temp1 ? temp2 = strchr(temp1, ';') : 0;
	id = strchr(pocIpAccoutaPassword, 'd');
	id ? id++ : 0;
	id ? id++ : 0;
	(id && temp2) ? (len = strlen(id) - strlen(temp2)) : 0;
	account&&id&&len ? (strncpy(account, id, len)) : 0;
}

/*
	  name : lv_poc_oem_get_global_atcmd_info
	 param : info-AT内容
	 		 index-参数个数
	 		 table-拆分信息表{参数1:指针偏移量 参数2:截取数据长度}
	author : wangls
  describe : 拆分整条AT指令
	  date : 2020-09-16
*/
PocGuiOemAtBufferAttr_t prv_lv_poc_oem_get_global_atcmd_info(void *info, int index, void *table)
{
	PocGuiOemAtBufferAttr_t oematbuf;
	PocGuiOemAtTableAttr_t *table_t = (PocGuiOemAtTableAttr_t *)table;
	memset(&oematbuf, 0, sizeof(PocGuiOemAtBufferAttr_t));
	int buflen = strlen(info);
	oematbuf.index = index;

	for(int i = 0; i < index; i++)
	{
		if(i == (index - 1))
		{
			oematbuf.buf[i].oemlen = buflen - table_t[i].pindex;
			strncpy(oematbuf.buf[i].oembuf, info + table_t[i].pindex, oematbuf.buf[i].oemlen);
			#if 0
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[atcmd]grouplistinfo %s", oematbuf.buf[i].oembuf);
			#endif
			break;
		}
		oematbuf.buf[i].oemlen = table_t[i].datalen;
		strncpy(oematbuf.buf[i].oembuf, info + table_t[i].pindex, oematbuf.buf[i].oemlen);
		#if 0
		if(i == 5 && oematbuf.buf[5].oembuf != NULL)
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[atcmd]grouplistinfo %s", oematbuf.buf[i].oembuf);
		#endif
	}

	return oematbuf;
}

void *lvPocGuiOemCom_get_oem_self_info(void)
{
	if(pocOemAttr.loginstatus_t != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
	{
		return NULL;
	}

	return &pocOemAttr.OemUserInfo;
}

void *lvPocGuiOemCom_get_current_group_info(void)
{
	if(pocOemAttr.loginstatus_t != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
	{
		return NULL;
	}

	return &pocOemAttr.OemCurrentGroupInfo;
}

bool lvPocGuiOemCom_modify_current_group_info(OemCGroup *CurrentGroup)
{
	if(pocOemAttr.loginstatus_t != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS || CurrentGroup == NULL)
	{
		return false;
	}
	memcpy(&pocOemAttr.OemCurrentGroupInfo, CurrentGroup, sizeof(OemCGroup));
	lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocOemAttr.OemUserInfo.OemUserName, pocOemAttr.OemCurrentGroupInfo.m_ucGName);

	return true;
}

bool lvPocGuiOemCom_get_speak_status(void)
{
	return pocOemAttr.is_speak_status;
}

bool lvPocGuiOemCom_get_listen_status(void)
{
	return pocOemAttr.is_listen_status;
}

bool lvPocGuiOemCom_get_obtainning_state(void)
{
	return pocOemAttr.is_update_data;
}

int lvPocGuiOemCom_get_login_status(void)
{
	return pocOemAttr.loginstatus_t;
}

bool lvPocGuiOemCom_get_system_status(void)
{
	return pocOemAttr.is_oem_system_status;
}

void lvPocGuiOemCom_set_obtainning_state(bool status)
{
	pocOemAttr.is_update_data = status;
}

void lvPocGuiOemCom_stop_check_ack(void)
{
	if(pocOemAttr.Oem_Check_ack_timeout_timer != NULL)
	{
		osiTimerStop(pocOemAttr.Oem_Check_ack_timeout_timer);
	}
}

lv_poc_tmpgrp_t lvPocGuiOemCom_cit_status(lv_poc_tmpgrp_t status)
{
	if(status == POC_TMPGRP_READ)
	{
		return pocOemAttr.cit_status;
	}
	pocOemAttr.cit_status = status;
	return false;
}

#if GUIIDTCOM_OEM_SELF_MSG
void lvPocGuiOemCom_set_response_speed(LVPOCOEMCOM_REPPONSE_SPEED_TYPE_T response_speed)
{
	pocOemAttr.oem_response_time = response_speed;
}
#endif

