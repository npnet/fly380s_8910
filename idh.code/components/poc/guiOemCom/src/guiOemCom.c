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

//debug log info config                        cool watch搜索项
#define GUIIDTCOM_OEMACK_DEBUG_LOG          1  //[oemack]
#define GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG   1  //[grouprefr]
#define GUIIDTCOM_GROUPOPTACK_DEBUG_LOG     1  //[groupoptack]
#define GUIIDTCOM_MEMBERREFR_DEBUG_LOG      1  //[memberrefr]
#define GUIIDTCOM_OEMSPEAK_DEBUG_LOG        1  //[oemspeak]
#define GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG   1  //[oemsinglecall]
#define GUIIDTCOM_OEMLISTEN_DEBUG_LOG       1  //[oemlisten]
#define GUIIDTCOM_OEMERRORINFO_DEBUG_LOG    1  //[oemerrorinfo]
#define GUIIDTCOM_OEMLOCKGROUP_DEBUG_LOG    1  //[oemlockgroup]
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
	osiThread_t *thread;
	osiThread_t *oemapthread;
	//queue
	osiMessageQueue_t *xQueue;
	osiTimer_t *Oem_MsgQueue_timer;
	int   sOemMessageID;
	int   rOemMessageID;
	int   DealMessageQueuePeroid;

	//Info
	OemCGroup OemCurrentGroupInfo;
	OemCGroup OemCurrentGroupInfoBuf;//缓存Info
	OemCGroup OemMonitorGroupInfo;
	PocGuiOemUserAttr_t  OemUserInfo;
	PocGuiOemUserAttr_t  OemSpeakerInfo;

	PocGuiOemGroupAttr_t OemAllGroupInfo;
	PocGuiOemUserAttr_t  OemAllUserInfo;

	//cb
	lv_poc_get_member_list_cb_t pocGetMemberListCb;
	lv_poc_get_group_list_cb_t pocGetGroupListCb;
	poc_set_current_group_cb pocSetCurrentGroupCb;
	poc_get_member_status_cb pocGetMemberStatusCb;
	poc_set_member_call_status_cb pocMemberCallCb;

	bool 		isReady;
	int 		pocnetstatus;

	char 		pocIpAccoutaPassword[128];
	size_t      pocIpAccoutaPasswordLen;

	//status
	bool is_member_call;
	bool member_call_dir; 
	int call_type; 
	bool is_group_info;
	bool is_member_info;
	int  m_status;
	int  loginstatus_t;
	int  groupstatus_t;
	bool is_speak_status;
}PocGuiOemComAttr_t;

enum{
	USER_OPRATOR_START_SPEAK = 3,
	USER_OPRATOR_SPEAKING  = 4,
	USER_OPRATOR_START_LISTEN = 5,
	USER_OPRATOR_LISTENNING = 6,
};

/*******************STATIC PARAM********************/
static PocGuiOemComAttr_t pocOemAttr = {0};
static LinkQueue OemQueue = {0};
//group
static lv_poc_oem_group_list *prv_group_list = NULL;
static oem_list_element_t *prv_group_list_cur = NULL;
//member
static lv_poc_oem_member_list *prv_member_list = NULL;
static oem_list_element_t *prv_member_list_online_cur = NULL;
static oem_list_element_t *prv_member_list_offline_cur = NULL;
/*******************STATIC FUNC********************/
static void pocGuiOemComTaskEntry(void *argument);
static void pocGuiOemApTaskEntry(void *argument);
static bool OemEnQueue(LinkQueue *Queue, PocGuiOemApSendAttr_t e);
static bool OemDeQueue(LinkQueue *Queue, PocGuiOemApSendAttr_t *e);
static OemCGroup prv_lv_poc_join_current_group_infomation(char * Information);
static PocGuiOemUserAttr_t prv_lv_poc_power_on_get_self_infomation(char * Information);
static PocGuiOemAtBufferAttr_t prv_lv_poc_oem_get_global_atcmd_info(void *info, int index, void *table);
static void prv_lv_poc_oem_get_group_list_info(void *information);
static void prv_lv_poc_oem_get_group_member_info(void *information);

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

void OEM_TTS_Stop()
{

}

//OEM CB
int OEM_SendUart(char *uf,int len)
{
	PocGuiOemApSendAttr_t apopt = {0};

	strcpy(apopt.oembuf, uf);
	apopt.oemlen = len;

	OemEnQueue(&OemQueue, apopt);
	#if 0
	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]OEM_SendUart ack %s", apopt->oembuf);
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
		return false;

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

void lvPocGuiOemCom_Request_Login(void)
{
	#if GUIIDTCOM_OEMLOGIN_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[oemlogin]%s(%d):start login in", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif

	OEMPOC_AT_Recv(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN, strlen(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN));
}

void lvPocGuiOemCom_Request_Logout(void)
{
	#if GUIIDTCOM_OEMLOGIN_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[oemlogin]%s(%d):start login out", __func__, __LINE__);
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
	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]open poc %s",OEM_OPT_CODE);
}

void lvPocGuiOemCom_Request_add_ListenGroup(char *id_group)
{
	char groupoptcode[64] = {0};

	strcpy(groupoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ADDLISTENGROUP);
	strcat(groupoptcode, id_group);

	OEMPOC_AT_Recv(groupoptcode, strlen(groupoptcode));
}

void lvPocGuiOemCom_Request_cannel_ListenGroup(char *id_group)
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

void lvPocGuiOemCom_Request_Set_Volum(char *volum)
{
	char volumoptcode[64] = {0};

	strcpy(volumoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETVOLUM);
	strcat(volumoptcode, volum);

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

void lvPocGuiOemCom_Request_AllMemeberInfo(void)
{
	OEMPOC_AT_Recv((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ALL_MEMBER_INDEX, strlen((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ALL_MEMBER_INDEX));
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
				OSI_LOGI(0, "[song]set poc success");
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SETPOC_REP, NULL);
				break;
			}

			switch(pocOemAttr.loginstatus_t)
			{
				case LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS:
				{
					//login exit--8200应答
					if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_NOLOGIN_ACK))
					{
						pocOemAttr.loginstatus_t = LVPOCOEMCOM_SIGNAL_LOGIN_EXIT;
						break;
					}
					//listen start--8B0001应答
					else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTLISTEN_ACK))
					{
						pocOemAttr.m_status = USER_OPRATOR_LISTENNING;
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_LISTEN_SPEAKER_REP, NULL);
						break;
					}
					//listen stop--8B0000应答
					else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPLISTEN_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_LISTEN_STOP_REP, NULL);
						break;
					}
					//speak start--0B应答
					else if(NULL != strstr(apopt->oembuf, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTSPEAK_ACK))
					{
						pocOemAttr.m_status = USER_OPRATOR_SPEAKING;
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_REP, NULL);
						break;
					}
					//speak stop--0C应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPSPEAK_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_REP, NULL);
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
					//all group info(所有群组信息)--0D应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUPLISTINFO_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_GROUPLIST_DATA_REP, apopt);
						break;
					}
					//index 1 group's member number(成员个数)--0E应答
					else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUP_MEMBER_INDEX_ACK))
					{
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_INFO_REP, apopt);
						break;
					}
					//index 1 group's member info(成员信息)--0E/13应答
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
						//查询账号是否正确
						if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ACCOUT))
						{
							lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_AP_POC_START, NULL);
							OSI_LOGI(0, "[song]accout correct");
						}
						else//需重新设置账号
						{

						}
						break;
					}
					//login success
					else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN_SUCCESS_ACK))
					{
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
			//转义StrHex
			lv_poc_oemdata_strtostrhex(ufs, (char *)ctx, strlen((char *)ctx));

			//组合
			strcpy(pocOemAttr.pocIpAccoutaPassword, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETPARAM);
			strcat(pocOemAttr.pocIpAccoutaPassword, ufs);
			strcat(pocOemAttr.pocIpAccoutaPassword, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_END);
			pocOemAttr.pocIpAccoutaPasswordLen = strlen(pocOemAttr.pocIpAccoutaPassword);
			//SET POC
			OEMPOC_AT_Recv(pocOemAttr.pocIpAccoutaPassword, pocOemAttr.pocIpAccoutaPasswordLen);
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]oem set param %s", (char *)pocOemAttr.pocIpAccoutaPassword);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SETPOC_REP:
		{
			if(pocOemAttr.loginstatus_t != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
			{
				lvPocGuiOemCom_Request_Login();
			}
			//save accout

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
			//User Info
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
			pocOemAttr.m_status = USER_OPRATOR_START_SPEAK;
			lvPocGuiOemCom_Request_Start_Speak();
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_REP:
		{
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "开始对讲", NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始对讲", NULL);
			/*开始闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
			if(pocOemAttr.m_status == USER_OPRATOR_SPEAKING)
			{
				char speak_name[100] = "";
				strcat((char *)speak_name, (const char *)pocOemAttr.OemSpeakerInfo.OemUserName);
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
			/*恢复run闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_RUN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Speak, 30, true);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, "停止对讲", NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"停止对讲", NULL);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);

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
			pocOemAttr.is_speak_status = false;
			/*恢复run闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_RUN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");

			if(pocOemAttr.is_member_call == false)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)"停止聆听", (const uint8_t *)"");
				//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			}

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_LISTEN_SPEAKER_REP:
		{
			char speaker_name[100];
			char *speaker_group = (char *)pocOemAttr.OemCurrentGroupInfo.m_ucGName;
			memset(speaker_name, 0, sizeof(char) * 100);
			strcpy(speaker_name, (const char *)pocOemAttr.OemSpeakerInfo.OemUserName);
			strcat(speaker_name, (const char *)"正在讲话");
			pocOemAttr.is_speak_status = true;

			/*开始闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
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
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP, NULL);
						break;
					}

					case LVPOCOEMCOM_CALL_TYPE_GROUP:
					{
					
						break;
					}
				}

				#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG|GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG
				char cOutstr[256] = {0};
				cOutstr[0] = '\0';
				sprintf(cOutstr, "[grouprefr][oemsinglecall]%s(%d):[86][ack]exit group", __func__, __LINE__);
				OSI_LOGI(0, cOutstr);
				#endif
			}
			else
			{
				if(0 == strcmp((char *)pocOemAttr.OemCurrentGroupInfo.m_ucGTemporary, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_TEMPORARYGROUP))
				{//join singel call		
					static OemCGroupMember member_call_obj = {0};
					memset(&member_call_obj, 0, sizeof(OemCGroupMember));

					strcpy((char *)member_call_obj.m_ucUName, (const char *)pocOemAttr.OemCurrentGroupInfo.m_ucGName);//临时呼叫的群组名就是用户名
					strcpy((char *)member_call_obj.m_ucUStatus, (const char *)OEM_GROUP_MEMBER_ONLINE_IN);

					pocOemAttr.member_call_dir = 1;
					pocOemAttr.call_type = LVPOCOEMCOM_CALL_TYPE_SINGLE;
					pocOemAttr.groupstatus_t = LVPOCOEMCOM_SIGNAL_GROUP_JOIN;
		            lv_poc_activity_func_cb_set.member_call_open((void *)&member_call_obj);

					#if GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG
					OSI_LOGI(0, "[grouprefr][oemsinglecall]start single call");
					#endif
					
				}
				else if(0 == strcmp((char *)pocOemAttr.OemCurrentGroupInfo.m_ucGTemporary, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FIXEDGROUP))
				{//join group call
					pocOemAttr.groupstatus_t = LVPOCOEMCOM_SIGNAL_GROUP_JOIN;
					poc_play_voice_one_time(LVPOCAUDIO_Type_Join_Group, 50, false);
						
					pocOemAttr.call_type = LVPOCOEMCOM_CALL_TYPE_GROUP;
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocOemAttr.OemUserInfo.OemUserName, pocOemAttr.OemCurrentGroupInfo.m_ucGName);

					#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
					char cOutstr[256] = {0};
					cOutstr[0] = '\0';
					sprintf(cOutstr, "[grouprefr]%s(%d):selete join group", __func__, __LINE__);
					OSI_LOGI(0, cOutstr);
					#endif
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
    int Gnum = 0;
	
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_IND:
		{
			if(pocOemAttr.pocGetGroupListCb == NULL)
			{
				break;
			}
			lvPocGuiOemCom_Request_AllGroupInfo();

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
			Gnum = lv_poc_oemdata_strtodec(Grouplist.buf[3].oembuf, sizeof(Grouplist.buf[3].oembuf)/sizeof(char));
			if(0 == strcmp(Grouplist.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FAILED))
			{
				pocOemAttr.pocGetGroupListCb(1, 0, NULL);
				pocOemAttr.pocGetGroupListCb = NULL;
				break;
			}
			//refr group
			pocOemAttr.pocGetGroupListCb(2, Gnum, pub_lv_poc_get_group_list_all_info());
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_UPDATE:
		{
		
			break;
		}
		
	    case LVPOCGUIOEMCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
	    {
			pocOemAttr.pocGetGroupListCb = (lv_poc_get_group_list_cb_t)ctx;
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

			#if GUIIDTCOM_GROUPOPTACK_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[grouproptack]%s(%d):join group", __func__, __LINE__);
			OSI_LOGI(0, cOutstr);
			#endif
			
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
			lvPocGuiOemCom_Request_AllMemeberInfo();
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
			pocOemAttr.OemAllUserInfo.OemUserNumber = lv_poc_oemdata_strtodec(Userlist.buf[3].oembuf, sizeof(Userlist.buf[3].oembuf)/sizeof(char));
			#if 0
			OSI_LOGI(0, "[song]member index %d", pocOemAttr.OemAllUserInfo.OemUserNumber);
			#endif
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
			if(ctx == 0)
			{
				//get current group's memberlist info
				OemCGroup *CurrentGUInfo = (OemCGroup *)lvPocGuiOemCom_get_current_group_info();

				if(CurrentGUInfo->m_ucGID != NULL)
				{
					//OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]current group id get member info %s", CurrentGUInfo->m_ucGID);
					lvPocGuiOemCom_Request_Groupx_MemeberInfo((char *)CurrentGUInfo->m_ucGID);
				}
				break;
			}
			else
			{
				//grouplist x group get member
				OemCGroup *xgroup_info = (OemCGroup *)ctx;
				if(xgroup_info->m_ucGID != NULL)
				{
					//OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]group id get member info %s", xgroup_info->m_ucGID);
					lvPocGuiOemCom_Request_Groupx_MemeberInfo((char *)xgroup_info->m_ucGID);
				}
			}

			break;
		}
		case LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;

			PocGuiOemAtBufferAttr_t Mmemberlist;
			PocGuiOemAtTableAttr_t table[4] = {
				{5,2},//应答号--13
				{7,2},//结果码
				{9,4},//操作ID--忽略
				{13,4},//组成员个数
			};

			Mmemberlist = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);
			int GUNum = lv_poc_oemdata_strtodec(Mmemberlist.buf[3].oembuf, sizeof(Mmemberlist.buf[3].oembuf)/sizeof(char));
			if(0 == strcmp(Mmemberlist.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_FAILED))
			{
				pocOemAttr.pocGetMemberListCb(1, 0, NULL);
				pocOemAttr.pocGetMemberListCb = NULL;
				break;
			}
			//refr member
			pocOemAttr.pocGetMemberListCb(2, GUNum, pub_lv_poc_get_member_list_info());

			#if 0//debug Linklist
			oem_list_element_t *p_element = NULL;
			p_element = prv_member_list->online_list;
			while(p_element)
			{
				OemCGroupMember *prv_user_info = (OemCGroupMember *)p_element->information;
				OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]online member %s", prv_user_info->m_ucUName);
				p_element = p_element->next;
			}

			p_element = prv_member_list->offline_list;
			while(p_element)
			{
				OemCGroupMember *prv_user_info = (OemCGroupMember *)p_element->information;
				OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]offline member %s", prv_user_info->m_ucUName);
				p_element = p_element->next;
			}
			#endif

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_UPDATE:
		{	
			//update current group member info
			lvPocGuiOemCom_Request_Groupx_MemeberInfo((char *)pocOemAttr.OemCurrentGroupInfo.m_ucGID);
			break;
		}
		
		case LVPOCGUIOEMCOM_SIGNAL_MEMBERLIST_DATA_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;
			//Nodelink
			prv_lv_poc_oem_get_group_member_info(apopt->oembuf);

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			pocOemAttr.pocGetMemberListCb = (lv_poc_get_member_list_cb_t)ctx;
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

			#if GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG
			OSI_LOGI(0, "[oemsinglecall]exit single call");
			#endif
				
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP:
		{
			#if GUIIDTCOM_OEMSINGLECALL_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[oemsinglecall]%s(%d):exit single call, join new groupID[%s]", __func__, __LINE__, pocOemAttr.OemCurrentGroupInfoBuf.m_ucGID);
			OSI_LOGI(0, cOutstr);
			#endif
				
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
		case LVPOCGUIOEMCOM_SIGNAL_MEMBER_GROUP_INFO_UPDATE:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx;
			
			PocGuiOemAtBufferAttr_t GUInfo;
			PocGuiOemAtTableAttr_t table[2] = {
				{5,2},//应答号--85
				{7,2},//结果码
			};

			GUInfo = prv_lv_poc_oem_get_global_atcmd_info(apopt->oembuf, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

			if(0 == strcmp(GUInfo.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GROUPINFOUPDATE))//组需更新
			{
				//lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_GROUP_LIST_QUERY_UPDATE, NULL);

				#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
				char cOutstr[256] = {0};
				cOutstr[0] = '\0';
				sprintf(cOutstr, "[grouprefr]%s(%d):[85][ack]group request refr", __func__, __LINE__);
				OSI_LOGI(0, cOutstr);
				#endif
				
			}
			else if(0 == strcmp(GUInfo.buf[1].oembuf, LVPOCPOCOEMCOM_SIGNAL_OPTCODE_MEMEBERINFOUPDATE))//成员需更新
			{
				//lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_MEMBER_LIST_QUERY_UPDATE, NULL);

				#if GUIIDTCOM_GROUPLISTREFR_DEBUG_LOG
				char cOutstr[256] = {0};
				cOutstr[0] = '\0';
				sprintf(cOutstr, "[grouprefr]%s(%d):[85][ack]member request refr", __func__, __LINE__);
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
void LvGuiOemCom_AutoLogin_timer_cb(void *ctx)
{

}

void PocGuiOemComStart(void)
{
	pocOemAttr.thread = osiThreadCreate(
			"pocGuiOemCom", pocGuiOemComTaskEntry, NULL,
			APPTEST_THREAD_PRIORITY, APPTEST_STACK_SIZE,
			APPTEST_EVENT_QUEUE_SIZE);
	pocOemAttr.oemapthread = osiThreadCreate(
			"pocGuiOemAp", pocGuiOemApTaskEntry, NULL,
			APPTEST_THREAD_PRIORITY, OEMAP_STACK_SIZE,
			OEMAP_EVENT_QUEUE_SIZE);

	pocOemAttr.Oem_MsgQueue_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_AutoLogin_timer_cb, NULL);/*误触碰定时器*/
}

void lvPocGuiOemCom_Init(void)
{
	memset(&pocOemAttr, 0, sizeof(PocGuiOemComAttr_t));
	//申请队列
	//参数 1:队列深度
	//参数 2:队列项内容大小
	pocOemAttr.xQueue = osiMessageQueueCreate(10, sizeof(struct _PocGuiOemComAttr_t*));

	bool status = OemInitQueue(&OemQueue);
	if(!status){
		OemDestroyQueue(&OemQueue);
	}
	//power on status
	pocOemAttr.loginstatus_t = LVPOCOEMCOM_SIGNAL_LOGIN_EXIT;
	pocOemAttr.groupstatus_t = LVPOCOEMCOM_SIGNAL_GROUP_EXIT;

	PocGuiOemComStart();
}

static
void pocGuiOemComTaskEntry(void *argument)
{
	osiEvent_t event = {0};
	//Oem init
	OEM_PocInit();

    for(int i = 0; i < 1; i++)
    {
	    osiThreadSleep(1000);
    }

	/*have connect network*/
	pocOemAttr.pocnetstatus = true;
	OEMNetworkStatusChange(pocOemAttr.pocnetstatus);

	//set volum
	lvPocGuiOemCom_Request_Set_Volum("64");//0-FFFF

    while(1)
    {
    	if(!osiEventTryWait(pocOemAttr.thread, &event, 200))
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

	PocGuiOemApSendAttr_t *apopt = NULL;
	PocGuiOemApSendAttr_t oemopt = {0};
	bool OemStatus = false;

    while(1)
    {
    	if(!osiMessageQueueTryGet(pocOemAttr.xQueue, (void *)&apopt, 200))
		{
			OemStatus = OemDeQueue(&OemQueue, &oemopt);
			if(OemStatus)
			{
				if(NULL != oemopt.oembuf)
				{
					#if 0
					OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]linkqueue %s", oemopt.oembuf);
					#endif
					lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_AP_POC_REP, (void *)&oemopt);
				}
			}
			continue;
    	}
    }
}

bool lvPocGuiOemCom_MessageQueue(osiMessageQueue_t *mq, const void *msg)
{
	if(pocOemAttr.oemapthread == NULL)
	{
		return false;
	}

	return osiMessageQueueTryPut(mq, (void *)msg, 200);
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
	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]OemGroupID %s", GInfo.m_ucGID);
	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]OemGroupName %s", GInfo.m_ucGName);
	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]OemGroupGTemporary %s", GInfo.m_ucGTemporary);
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

	grouplistbuf = prv_lv_poc_oem_get_global_atcmd_info(information, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

	strcpy((char *)prv_group_info.m_ucGMonitor, grouplistbuf.buf[1].oembuf);
	prv_group_info.m_ucGIndex = lv_poc_oemdata_strtodec(grouplistbuf.buf[3].oembuf, sizeof(grouplistbuf.buf[3].oembuf)/sizeof(char));
	strcpy((char *)prv_group_info.m_ucGID, grouplistbuf.buf[4].oembuf);
	prv_group_info.m_ucGMemberNum = lv_poc_oemdata_strtodec(grouplistbuf.buf[5].oembuf, sizeof(grouplistbuf.buf[5].oembuf)/sizeof(char));
	lv_poc_oem_unicode_to_utf8_convert(grouplistbuf.buf[6].oembuf, prv_group_info.m_ucGName);

	if(!prv_group_list)
	{
		prv_group_list = (lv_poc_oem_group_list *)lv_mem_alloc(sizeof(lv_poc_oem_group_list));

		if(prv_group_list == NULL)
		{
			return;
		}
		prv_group_list->group_list = NULL;
		prv_group_list->group_number = 0;
	}
	//Nodelist
	oem_list_element_t *p_element = NULL;
	p_element = (oem_list_element_t *)lv_mem_alloc(sizeof(oem_list_element_t));
	p_element->information = (OemCGroup *)lv_mem_alloc(sizeof(OemCGroup));

	if(p_element == NULL || p_element->information == NULL)
	{
		p_element = prv_group_list->group_list;
		while(p_element)
		{
			prv_group_list_cur = p_element;
			p_element = p_element->next;
			lv_mem_free(prv_group_list_cur);
		}
		return;
	}

	if((NULL != p_element) && (NULL != p_element->information))
	{
		p_element->next = NULL;
		p_element->list_item = NULL;
		memcpy(p_element->information, &prv_group_info, sizeof(OemCGroup));
		strcpy(p_element->name, (char *)prv_group_info.m_ucGName);
		prv_group_list->group_number = prv_group_info.m_ucGIndex + 1;

		if(prv_group_list->group_list != NULL)
		{
			prv_group_list_cur->next = p_element;
			prv_group_list_cur = prv_group_list_cur->next;
		}
		else
		{
			prv_group_list->group_list = p_element;
			prv_group_list_cur = p_element;
		}
	}
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
	PocGuiOemAtBufferAttr_t groupxmemberlistbuf;
	OemCGroupMember prv_user_info = {0};

	groupxmemberlistbuf = prv_lv_poc_oem_get_global_atcmd_info(information, sizeof(table)/sizeof(PocGuiOemAtTableAttr_t), table);

	strcpy((char *)prv_user_info.m_ucUStatus, groupxmemberlistbuf.buf[1].oembuf);
	prv_user_info.m_ucUIndex = lv_poc_oemdata_strtodec(groupxmemberlistbuf.buf[3].oembuf, sizeof(groupxmemberlistbuf.buf[3].oembuf)/sizeof(char));
	strcpy((char *)prv_user_info.m_ucUID, groupxmemberlistbuf.buf[4].oembuf);
	lv_poc_oem_unicode_to_utf8_convert(groupxmemberlistbuf.buf[5].oembuf, prv_user_info.m_ucUName);

	//generate Linklist head node
	if(prv_member_list == NULL)
	{
		prv_member_list = (lv_poc_oem_member_list *)lv_mem_alloc(sizeof(lv_poc_oem_member_list));
		if(prv_member_list == NULL)
		{
			return;
		}

		prv_member_list->offline_number = 0;
		prv_member_list->online_number = 0;
		prv_member_list->hide_offline = false;
		prv_member_list->online_list = NULL;
		prv_member_list->offline_list = NULL;
	}

	oem_list_element_t *p_element = NULL;
	p_element = (oem_list_element_t *)lv_mem_alloc(sizeof(oem_list_element_t));
	p_element->information = (OemCGroupMember *)lv_mem_alloc(sizeof(OemCGroupMember));
	if(p_element == NULL || p_element->information == NULL)
	{
		p_element = prv_member_list->online_list;
		while(p_element)
		{
			prv_member_list_online_cur = p_element;
			p_element = p_element->next;
			lv_mem_free(prv_member_list_online_cur);
		}

		p_element = prv_member_list->offline_list;
		while(p_element)
		{
			prv_member_list_offline_cur = p_element;
			p_element = p_element->next;
			lv_mem_free(prv_member_list_offline_cur);
		}

		return;
	}

	p_element->next = NULL;
	p_element->list_item = NULL;
	strcpy(p_element->name, (char *)prv_user_info.m_ucUName);
	memcpy((void *)p_element->information, (void *)&prv_user_info, sizeof(OemCGroupMember));

	if(0 == strcmp(groupxmemberlistbuf.buf[1].oembuf, "03"))
	{
		//online
		if(prv_member_list->online_list != NULL){
			prv_member_list_online_cur->next = p_element;
			prv_member_list_online_cur = prv_member_list_online_cur->next;
		}
		else{
			prv_member_list->online_list = p_element;
			prv_member_list_online_cur = p_element;
		}
		prv_member_list->online_number++;
	}else{
		//offline
		if(prv_member_list->offline_list != NULL){
			prv_member_list_offline_cur->next = p_element;
			prv_member_list_offline_cur = prv_member_list_offline_cur->next;
		}else{
			prv_member_list->offline_list = p_element;
			prv_member_list_offline_cur = p_element;
		}
		prv_member_list->offline_number++;
	}

}

lv_poc_oem_group_list *pub_lv_poc_get_group_list_all_info(void)
{
	if(prv_group_list == NULL)
	{
		return NULL;
	}
	return prv_group_list;
}

bool pub_lv_poc_free_group_list(void)
{
	oem_list_element_t *p_group_list;

	if(prv_group_list != NULL)
	{
		p_group_list = prv_group_list->group_list;
		while(p_group_list)
		{
			prv_group_list_cur = p_group_list;
			p_group_list = p_group_list->next;
			lv_mem_free(prv_group_list_cur);
		}

		lv_mem_free(prv_group_list);
		OSI_LOGI(0, "[song]free oem group_list");
	}

	prv_group_list = NULL;
	prv_group_list_cur = NULL;

	return true;
}

lv_poc_oem_member_list *pub_lv_poc_get_member_list_info(void)
{
	if(prv_member_list == NULL)
	{
		return NULL;
	}
	return prv_member_list;
}

bool pub_lv_poc_free_member_list(void)
{
	oem_list_element_t *p_member_list;

	if(prv_member_list != NULL)
	{
		p_member_list = prv_member_list->online_list;
		while(p_member_list)
		{
			prv_member_list_online_cur = p_member_list;
			p_member_list = p_member_list->next;
			lv_mem_free(prv_member_list_online_cur);
		}

		p_member_list = prv_member_list->offline_list;
		while(p_member_list)
		{
			prv_member_list_offline_cur = p_member_list;
			p_member_list = p_member_list->next;
			lv_mem_free(prv_member_list_offline_cur);
		}

		lv_mem_free(prv_member_list);
		OSI_LOGI(0, "[song]free oem member_list");
	}

	prv_member_list = NULL;
	prv_member_list_online_cur = NULL;
	prv_member_list_offline_cur = NULL;

	return true;
}

bool lvPocGuiOemCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx)
{
	if(pocOemAttr.thread == NULL)
	{
		return false;
	}

	static osiEvent_t event = {0};

	uint32_t critical = osiEnterCritical();

	memset(&event, 0, sizeof(osiEvent_t));
	event.id = 200;
	event.param1 = signal;
	event.param2 = (uint32_t)ctx;

	osiExitCritical(critical);

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
  describe : 字符串转十进制
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
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]grouplistinfo %s", oematbuf.buf[i].oembuf);
			#endif
			break;
		}
		oematbuf.buf[i].oemlen = table_t[i].datalen;
		strncpy(oematbuf.buf[i].oembuf, info + table_t[i].pindex, oematbuf.buf[i].oemlen);
		#if 0
		OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]grouplistinfo %s", oematbuf.buf[i].oembuf);
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

void *lvPocGuiOemCom_get_monitor_group(void)
{
	if(pocOemAttr.loginstatus_t != LVPOCOEMCOM_SIGNAL_LOGIN_SUCCESS)
	{
		return NULL;
	}

	return &pocOemAttr.OemMonitorGroupInfo;
}

bool lvPocGuiIdtCom_get_listen_status(void)
{
	return pocOemAttr.is_speak_status;
}

#if 1/*确信*/
bool lvPocGuiIdtCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx)
{
	return true;
}

void lvPocGuiIdtCom_log(void)
{
	return ;
}

bool lvPocGuiIdtCom_get_status(void)
{
	return true;
}

void *lvPocGuiIdtCom_get_current_lock_group(void)
{
	return NULL;
}

bool lvPocGuiIdtCase_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx, void * cause_str)
{
	return true;
}

#endif


