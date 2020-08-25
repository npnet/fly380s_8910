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

/*define*/
#define APPTEST_THREAD_PRIORITY (OSI_PRIORITY_NORMAL)

/*function*/
static void pocGuiZzdComTaskEntry(void *argument);

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
	return len;
}


osiThread_t *Zzd_Com_thread;

void guiZzdComInit(void)
{
	Zzd_Com_thread = osiThreadCreate(
			"pocGuiIdtCom", pocGuiZzdComTaskEntry, NULL,
			APPTEST_THREAD_PRIORITY, 2048,
			32);

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

    while(1)
    {
    	if(!osiEventTryWait(Zzd_Com_thread, &event, 800))
		{

    	}
    }
}



