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

/**********************EXTERN**********************/
atCmdEngine_t *ap_Oem_engine = NULL;

/**********************STATIC**********************/
typedef struct _PocGuiOemApSendAttr_t
{
	char *oembuf;
	int   oemlen;
}PocGuiOemApSendAttr_t;

static PocGuiOemApSendAttr_t pOemCome;//存储消息队列

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

	bool 		isReady;
	int 		pocnetstatus;

	char 		pocIpAccoutaPassword[128];
	size_t      pocIpAccoutaPasswordLen;

	bool is_member_call;
	int  m_status;
	bool is_pocconfig;

}PocGuiOemComAttr_t;

static PocGuiOemComAttr_t pocOemAttr = {0};

enum{
	USER_OPRATOR_START_SPEAK = 3,
	USER_OPRATOR_SPEAKING  = 4,
	USER_OPRATOR_START_LISTEN = 5,
	USER_OPRATOR_LISTENNING = 6,
};

/*******************STATIC FUNC********************/
static void pocGuiOemComTaskEntry(void *argument);
static void pocGuiOemApTaskEntry(void *argument);

#ifndef ENABLEPOCOemINTERFACE
/*-----------Oem platform-------------*/

/* 1:TTS正在播放  0:TTS播放完毕 */
extern void OEM_TTS_Status_CB(int type);

/*过滤串口数据  buf为 AT+POC= 等号后面的数据*/
extern void OEMPOC_AT_Recv(char* buf ,int len);

/* 实现开机自动联网，并上报联网状态 */
/* 1 NET_CONNECTED 0 NET_DISCONNECTED */
extern void OEMNetworkStatusChange(int result);

/*链接库并调用初始化接口*/
extern void OEM_PocInit();

/*-----------本地端实现接口-----------*/

/*TTS接口 unicode*/
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
	PocGuiOemApSendAttr_t *apopt = &pOemCome;

	apopt->oembuf = uf;
	apopt->oemlen = len;

	if(!lvPocGuiOemCom_MessageQueue(pocOemAttr.xQueue, (void *)&apopt))
	{
		if(lvPocGuiOemCom_MessageQueue(pocOemAttr.xQueue, (void *)&apopt))
		{
			return len;
		}
		return 0;
	}

	OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]OEM_SendUart ack %s", apopt->oembuf);

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
/*------------------END----------------*/
#endif

void lvPocGuiOemCom_Login(void)
{
	OEMPOC_AT_Recv(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN, strlen(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN));
}

void lvPocGuiOemCom_Logout(void)
{
	OEMPOC_AT_Recv(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGOUT, strlen(LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGOUT));
}

void lvPocGuiOemCom_OpenPOC(char *tts_status, char *notify_status, char* offlineplay_status)
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

void lvPocGuiOemCom_add_ListenGroup(char *id_group)
{
	char groupoptcode[64] = {0};

	strcpy(groupoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ADDLISTENGROUP);
	strcat(groupoptcode, id_group);

	OEMPOC_AT_Recv(groupoptcode, strlen(groupoptcode));
}

void lvPocGuiOemCom_cannel_ListenGroup(char *id_group)
{
	char groupoptcode[64] = {0};

	strcpy(groupoptcode, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_CANNELLISTENGROUP);
	strcat(groupoptcode, id_group);

	OEMPOC_AT_Recv(groupoptcode, strlen(groupoptcode));
}

void lvPocGuiOemCom_Start_Speak(void)
{
	OEMPOC_AT_Recv((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTSPEAK, strlen((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTSPEAK));
}

void lvPocGuiOemCom_Stop_Speak(void)
{
	OEMPOC_AT_Recv((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPSPEAK, strlen((char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPSPEAK));
}

void lvPocGuiOemCom_Set_Volum(char *volum)
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

static
void prvPocGuiOemTaskHandleMsgCB(uint32_t id, uint32_t ctx1, uint32_t ctx2)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_OPENPOC_IND:
		{
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_AP_POC_REP:
		{
			PocGuiOemApSendAttr_t *apopt = (PocGuiOemApSendAttr_t *)ctx1;

			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]oem msg code %s", apopt->oembuf);
			//OPEN POC
			if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_OPENPOC_ACK))
			{
				OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]open poc success");
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_OPENPOC_REP, (void*)apopt->oembuf);
				break;
			}
			//SET POC
			else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_SETPARAM_ACK))
			{
				OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]set poc success");
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SETPOC_REP, (void*)apopt->oembuf);
				break;
			}
			//GET POC
			else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_GETPARAM_ACK))
			{
				//查询账号是否正确
				if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_ACCOUT))
				{
					lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_AP_POC_START, NULL);
					//验证账号
					OSI_LOGI(0, "[song]accout ok");
				}
				else//需重新设置账号
				{

				}
				break;
			}
			//LOGINNING
			else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGINING_ACK))
			{
				break;
			}
			//LOGIN SUCCESS
			else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_LOGIN_SUCCESS_ACK))
			{
				OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]login success");
				break;
			}
			//软件已启动
			else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_POCSTARTED_ACK))
			{
				//请求参数
				lvPocGuiOemCom_Request_PocParam();
				break;
			}
			//listen
			else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTLISTEN_ACK))
			{
				pocOemAttr.m_status = USER_OPRATOR_LISTENNING;
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_LISTEN_SPEAKER_REP, NULL);
				break;
			}
			else if(NULL != strstr(apopt->oembuf,LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPLISTEN_ACK))
			{
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_LISTEN_STOP_REP, NULL);
				break;
			}
			//speak
			else if(NULL != strstr(apopt->oembuf, (char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STARTSPEAK_ACK))
			{
				pocOemAttr.m_status = USER_OPRATOR_SPEAKING;
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_REP, NULL);
				break;
			}
			else if(NULL != strstr(apopt->oembuf,(const char *)LVPOCPOCOEMCOM_SIGNAL_OPTCODE_STOPSPEAK_ACK))
			{
				lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_REP, NULL);
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

static
void prvPocGuiOemTaskHandleSetPOC(uint32_t id, uint32_t ctx1, uint32_t ctx2)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_SETPOC_IND:
		{
			char ufs[128] = {0};

			memset(&pocOemAttr.pocIpAccoutaPassword, 0, sizeof(pocOemAttr.pocIpAccoutaPassword));
			//转义StrHex
			Data_StrToStrHex(ufs, (char *)ctx1, strlen((char *)ctx1));

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
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]poc param %s", (char *)ctx1);
			break;
		}

		default:
		{
			break;
		}
	}

}

static
void prvPocGuiOemTaskHandleOpenPOC(uint32_t id, uint32_t ctx1, uint32_t ctx2)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_OPENPOC_IND:
		{

			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_OPENPOC_REP:
		{
			lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_LOGIN_IND, (void*)ctx1);
			break;
		}

		default:
		{
			break;
		}
	}

}

static
void prvPocGuiOemTaskHandleLogin(uint32_t id, uint32_t ctx1, uint32_t ctx2)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_LOGIN_IND:
		{
			lvPocGuiOemCom_Login();
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_LOGIN_REP:
		{
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_EXIT_IND:
		{
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
void prvPocGuiOemTaskHandleSpeak(uint32_t id, uint32_t ctx1, uint32_t ctx2)
{
	switch(id)
	{
		case LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_IND:
		{
			pocOemAttr.m_status = USER_OPRATOR_START_SPEAK;
			lvPocGuiOemCom_Start_Speak();
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
				strcpy((char *)speak_name, (const char *)"主讲:");
				strcat((char *)speak_name, (const char *)"");//pocIdtAttr.self_info.ucName);
				if(!pocOemAttr.is_member_call)
				{
					char group_name[100] = "";
					strcpy((char *)group_name, (const char *)"飞图同辉测试1组");//pocIdtAttr.speaker_group.m_ucGName
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
			lvPocGuiOemCom_Stop_Speak();
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
void prvPocGuiOemTaskHandleListen(uint32_t id, uint32_t ctx1, uint32_t ctx2)
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
			/*恢复run闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_RUN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");

			if(pocOemAttr.is_member_call == false)//(pocOemAttr.membercall_count > 1 && pocOemAttr.is_member_call == true) ||
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
			char *speaker_group = (char *)"飞图同辉测试1组";//pocIdtAttr.speaker_group.m_ucGName;
			memset(speaker_name, 0, sizeof(char) * 100);
			strcpy(speaker_name, (const char *)"");//pocIdtAttr.speaker.ucName);
			strcat(speaker_name, (const char *)"正在讲话");

			/*开始闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
			#if 0
			if(pocOemAttr.membercall_count > 1 && pocOemAttr.is_member_call)/*单呼进入第二次*/
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)"");
			}
			#endif

			if(!pocOemAttr.is_member_call)
			{
				char speaker_group_name[100];
				strcpy(speaker_group_name, (const char *)"");//pocIdtAttr.speaker_group.m_ucGName);
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
void LvGuiOemCom_OemSendMsgQueue_timer_cb(void *ctx)
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

	pocOemAttr.Oem_MsgQueue_timer = osiTimerCreate(pocOemAttr.thread, LvGuiOemCom_OemSendMsgQueue_timer_cb, NULL);/*误触碰定时器*/
}

void lvPocGuiOemCom_Init(void)
{
	memset(&pocOemAttr, 0, sizeof(PocGuiOemComAttr_t));
	//申请队列
	//参数 1:队列深度
	//参数 2:队列项内容大小
	pocOemAttr.xQueue = osiMessageQueueCreate(10, sizeof(struct _PocGuiOemComAttr_t*));

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
	lvPocGuiOemCom_Set_Volum("64");//0-FFFF

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
			{
				prvPocGuiOemTaskHandleOpenPOC(event.param1, event.param2, event.param3);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_AP_POC_IND:
			{
				char *ap_poc_param = (char *)event.param2;

				OEMPOC_AT_Recv(ap_poc_param, strlen(ap_poc_param));
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_AP_POC_REP:
			{
				prvPocGuiOemTaskHandleMsgCB(event.param1, event.param2, event.param3);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_LOGIN_IND:
			case LVPOCGUIOEMCOM_SIGNAL_LOGIN_REP:
			case LVPOCGUIOEMCOM_SIGNAL_EXIT_IND:
			case LVPOCGUIOEMCOM_SIGNAL_EXIT_REP:
			{
				prvPocGuiOemTaskHandleLogin(event.param1, event.param2, event.param3);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_SETPOC_IND:
			case LVPOCGUIOEMCOM_SIGNAL_SETPOC_REP:
			{
				prvPocGuiOemTaskHandleSetPOC(event.param1, event.param2, event.param3);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_IND:
			case LVPOCGUIOEMCOM_SIGNAL_SPEAK_START_REP:
			case LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_IND:
			case LVPOCGUIOEMCOM_SIGNAL_SPEAK_STOP_REP:
			{
				prvPocGuiOemTaskHandleSpeak(event.param1, event.param2, event.param3);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_LISTEN_START_REP:
    		case LVPOCGUIOEMCOM_SIGNAL_LISTEN_STOP_REP:
    		case LVPOCGUIOEMCOM_SIGNAL_LISTEN_SPEAKER_REP:
			{
				prvPocGuiOemTaskHandleListen(event.param1, event.param2, event.param3);
				break;
			}

			case LVPOCGUIOEMCOM_SIGNAL_AP_POC_START:
			{
				//open poc
				lvPocGuiOemCom_OpenPOC(OEM_FUNC_CLOSE, OEM_FUNC_OPEN, OEM_FUNC_OPEN);
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

    while(1)
    {
    	if(!osiMessageQueueTryGet(pocOemAttr.xQueue, (void *)&apopt, 200))
		{
			continue;
    	}

		if(NULL != apopt->oembuf)
		{
			//send data
			lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_AP_POC_REP, (void *)apopt);
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

void Data_StrToStrHex(char *pszDest, char *pbSrc, int nLen)
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

void *lvPocGuiIdtCom_get_self_info(void)
{
	return NULL;
}

void *lvPocGuiIdtCom_get_current_group_info(void)
{
	return NULL;
}

bool lvPocGuiIdtCom_get_listen_status(void)
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


