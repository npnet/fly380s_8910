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
#define APPTEST_THREAD_PRIORITY (OSI_PRIORITY_NORMAL)
#define APPTEST_STACK_SIZE (8192 * 16)
#define APPTEST_EVENT_QUEUE_SIZE (64)

/**********************EXTERN**********************/
atCmdEngine_t *ap_zzd_engine = NULL;

/**********************STATIC**********************/
typedef struct _PocGuiZzdComAttr_t
{
	osiThread_t *thread;
	bool 		isReady;
	int 		pocnetstatus;
}PocGuiZzdComAttr_t;

static PocGuiZzdComAttr_t pocZzdAttr = {0};

/*******************STATIC FUNC********************/
static void pocGuiZzdComTaskEntry(void *argument);

#ifndef ENABLEPOCZZDINTERFACE
/*-----------zzd platform-------------*/

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

/*TTS接口*/
/*unicode*/
int OEM_TTS_Spk(char* atxt)
{
	return true;
}

void OEM_TTS_Stop()
{

}

/** Uart */
int OEM_SendUart(char *uf,int len)//返回成功发送的值
{
	if(ap_zzd_engine == NULL)
	{
		OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]OEM NULL");
		return 0;
	}
	lvPocGuiZzdCom_Msg(LVPOCGUIZZDCOM_SIGNAL_AP_POC_REP, (void*)uf, (void*)len);

	return len;
}
/*------------------END----------------*/
#endif

void PocGuiZzdComStart(void)
{
	pocZzdAttr.thread = osiThreadCreate(
			"pocGuiZzdCom", pocGuiZzdComTaskEntry, NULL,
			APPTEST_THREAD_PRIORITY, APPTEST_STACK_SIZE,
			APPTEST_EVENT_QUEUE_SIZE);

}

void lvPocGuiZzdCom_Init(void)
{
	memset(&pocZzdAttr, 0, sizeof(PocGuiZzdComAttr_t));

	PocGuiZzdComStart();
}

static
void pocGuiZzdComTaskEntry(void *argument)
{

	osiEvent_t event = {0};
	/*zzd init*/
	OEM_PocInit();

    for(int i = 0; i < 1; i++)
    {
	    osiThreadSleep(1000);
    }

	/*have connect network*/
	pocZzdAttr.pocnetstatus = true;
	OEMNetworkStatusChange(pocZzdAttr.pocnetstatus);

    while(1)
    {
    	if(!osiEventTryWait(pocZzdAttr.thread, &event, 200))
		{
			continue;
    	}

		if(event.id != 200)
		{
			continue;
		}

		switch(event.param1)
		{
			case LVPOCGUIZZDCOM_SIGNAL_AP_POC_IND:
			{
				char *ap_poc_param = (char *)event.param2;

				OEMPOC_AT_Recv(ap_poc_param, strlen(ap_poc_param));/*转发数据至zzd*/
				break;
			}
			case LVPOCGUIZZDCOM_SIGNAL_AP_POC_REP:
			{
				char *uf = (char *)event.param2;
				size_t len = (size_t)event.param3;

				atCmdWrite(ap_zzd_engine, (const void *)uf, len);
				OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]OEM send is %s", uf);
				break;
			}
			default:
			{
				break;
			}
		}
    }
}

bool lvPocGuiZzdCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx1, void * ctx2)
{
	if(pocZzdAttr.thread == NULL)
	{
		return false;
	}

	static osiEvent_t event = {0};

	uint32_t critical = osiEnterCritical();

	memset(&event, 0, sizeof(osiEvent_t));
	event.id = 200;
	event.param1 = signal;
	event.param2 = (uint32_t)ctx1;
	event.param3 = (uint32_t)ctx2;

	osiExitCritical(critical);

	return osiEventSend(pocZzdAttr.thread, &event);
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


