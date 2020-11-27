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
#include "guiBndCom_api.h"
#include "audio_device.h"
#include <sys/time.h>
#include "drv_charger_monitor.h"
//system
#include "vfs.h"
#include "cfw.h"
#include "netdb.h"
#include "sockets.h"
#include "tts_player.h"
//bnd
#include "broad.h"
#include "type.h"
#include "oem_lib_api.h"

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

#define APPTEST_THREAD_PRIORITY (OSI_PRIORITY_NORMAL)
#define APPTEST_STACK_SIZE (8192 * 16)
#define APPTEST_EVENT_QUEUE_SIZE (64)
#define GUIIDTCOM_SELF_INFO_SZIE (1400)
#define GUIIDTCOM_RELEASE_MIC (void *)1
#define GUIIDTCOM_REQUEST_MIC (void *)2

//debug log info config                        cool watch搜索项
#define GUIBNDCOM_BUILDGROUP_DEBUG_LOG      1  //[buildgroup]
#define GUIBNDCOM_GROUPLISTREFR_DEBUG_LOG   1  //[grouprefr]
#define GUIBNDCOM_GROUPOPTACK_DEBUG_LOG     1  //[groupoptack]
#define GUIBNDCOM_GROUPLISTDEL_DEBUG_LOG 1  //[idtgroupdel]
#define GUIBNDCOM_MEMBERREFR_DEBUG_LOG      1  //[memberrefr]
#define GUIBNDCOM_SPEAK_DEBUG_LOG        1  //[idtspeak]
#define GUIBNDCOM_SINGLECALL_DEBUG_LOG   1  //[idtsinglecall]
#define GUIBNDCOM_LISTEN_DEBUG_LOG       1  //[idtlisten]
#define GUIBNDCOM_ERRORINFO_DEBUG_LOG    1  //[idterrorinfo]
#define GUIBNDCOM_LOCKGROUP_DEBUG_LOG    1  //[idtlockgroup]

//windows note
#define GUIBNDCOM_WINDOWS_NOTE    		1

enum{
	USER_OPRATOR_START_SPEAK = 3,
	USER_OPRATOR_SPEAKING  = 4,
	USER_OPRATOR_START_LISTEN = 5,
	USER_OPRATOR_LISTENNING = 6,
};

static void LvGuiBndCom_delay_close_listen_timer_cb(void *ctx);
static void LvGuiBndCom_start_speak_voice_timer_cb(void *ctx);
static void prvPocGuiBndTaskHandleCallFailed(uint32_t id, uint32_t ctx, uint32_t cause_str);
static void lvPocGuiIdtCom_send_data_callback(uint8_t * data, uint32_t length);

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
        for (i = 0; i < 32; i++)
        {
            m_Group[i].m_ucGName[0] = 0;
            m_Group[i].m_ucGNum[0] = 0;
        }
        return 0;
    }

public:
	unsigned int m_Group_Num;
    CGroup  m_Group[32];//一个用户,最多处于32个组中
};

//BND使用者
class CBndUser
{
public:
    CBndUser()
    {
        Reset();
    }

    ~CBndUser()
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

typedef struct _PocGuiBndComAttr_t
{
public:
	osiThread_t *thread;
	bool        isReady;
	POCAUDIOPLAYER_HANDLE	player;
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
	uint8_t current_group;
	Msg_GROUP_MEMBER_s member_call_obj;
	bool is_member_call;
	int member_call_dir;  //0 tx,  1 rx
	unsigned int mic_ctl;
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
	osiTimer_t * addgroup_user_timer;
	bool onepoweron;
	char build_self_name[16];
	bool   is_makeout_call;
	bool   is_release_call;
	bool   is_justnow_listen;
	int    call_error_case;
	int    current_group_member_dwnum;

	//status
	bool invaildlogininf;

	int delay_play_voice;

} PocGuiBndComAttr_t;

CBndUser m_BndUser;
static PocGuiBndComAttr_t pocBndAttr = {0};

bool AIR_Log_Debug(const char *format, ...)//日志打印
{
	if (NULL == format)
		return -1;

	char   cBuf[256];
	va_list va;
	va_start(va, format);
	int iLen = 0;
	iLen = snprintf((char*)&cBuf[iLen], sizeof(cBuf) - iLen - 1, "IDT: ");
	iLen = vsnprintf((char*)&cBuf[iLen], sizeof(cBuf) - iLen - 1, format, va);
	va_end(va);
	if (iLen <= 0)
		return -1;

	OSI_LOGXE(OSI_LOGPAR_S, 0, "%s", (char*)cBuf);

	return true;
}

/* 锁和信号量相关接口*/
AIR_OSI_Mutex_t* AIR_OSI_MutexCreate(void)//创建锁
{
	OSI_LOGI(0, "[bnd][mutex](%d):MutexCreate", __LINE__);
	return (AIR_OSI_Mutex_t*)osiMutexCreate();
}

bool AIR_OSI_MutexDelete(AIR_OSI_Mutex_t* mutex)//删除锁
{
	OSI_LOGI(0, "[bnd][mutex](%d):MutexDelete", __LINE__);
	osiMutexDelete((osiMutex_t *)mutex);
	return true;
}

bool AIR_OSI_MutexTryLock(AIR_OSI_Mutex_t* mutex, unsigned int ms_timeout)//上锁
{
	OSI_LOGI(0, "[bnd][mutex](%d):MutexTryLock", __LINE__);
	return osiMutexTryLock((osiMutex_t *)mutex, ms_timeout);
}

bool AIR_OSI_MutexUnlock(AIR_OSI_Mutex_t* mutex)//释放锁
{
	OSI_LOGI(0, "[bnd][mutex](%d):MutexUnlock", __LINE__);
	osiMutexUnlock((osiMutex_t *)mutex);
	return true;
}

AIR_OSI_Semaphore_t* AIR_OSI_SemaphoreCreate(int a, int b)//创建信号量
{
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreCreate", __LINE__);
	return (AIR_OSI_Semaphore_t*)osiSemaphoreCreate(a, b);
}

bool AIR_SemaphoreDelete(AIR_OSI_Semaphore_t* cond)//删除信号量
{
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreDelete", __LINE__);
	osiSemaphoreDelete((osiSemaphore_t *)cond);
	return true;
}

bool AIR_OSI_SemaphoreRelease(AIR_OSI_Semaphore_t* cond)//释放信号量
{
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreRelease", __LINE__);
	osiSemaphoreRelease((osiSemaphore_t *)cond);
	return true;
}

bool AIR_OSI_SemaphoreAcquire(AIR_OSI_Semaphore_t* cond)//等待信号量
{
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreAcquire", __LINE__);
	osiSemaphoreAcquire((osiSemaphore_t *)cond);
	return true;
}

bool AIR_OSI_SemaphoreTryAcquire(AIR_OSI_Semaphore_t* cond, unsigned int ms_timeout)//等待信号量到超时
{
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreTryAcquire", __LINE__);
	return osiSemaphoreTryAcquire((osiSemaphore_t *)cond, ms_timeout);
}

int AIR_FS_Open(char* path, int flags, int a)//打开文件
{
	OSI_LOGI(0, "[bnd][fs](%d):open fs", __LINE__);
	return vfs_open(path, flags, a);
}

bool AIR_FS_Close(int file)//关闭文件
{
	OSI_LOGI(0, "[bnd][fs](%d):close fs", __LINE__);
	vfs_close(file);
	return true;
}

int AIR_FS_Read(int file, char* data, int size)//读文件
{
	OSI_LOGI(0, "[bnd][fs](%d):read fs", __LINE__);
	return vfs_read(file, (void *)data, size);
}

int AIR_FS_Write(int file, char* data, int size)//写文件
{
	OSI_LOGI(0, "[bnd][fs](%d):write fs", __LINE__);
	return vfs_write(file, (const void *)data, size);
}

bool AIR_FS_Seek(int file, int flag, int type)//操作文件流指针
{
	OSI_LOGI(0, "[bnd][fs](%d):seek fs", __LINE__);
	vfs_lseek(file, flag, type);
	return true;
}

int AIR_SOCK_Error(void)//获取网络错误标识
{
	OSI_LOGI(0, "[bnd][socket](%d):get sock error", __LINE__);

	return errno;

	//uint8_t status;
	//return CFW_CfgGetNwStatus(&status, CFW_SIM_0);
}

int AIR_SOCK_Gethostbyname(char* pHostname, AIR_SOCK_IP_ADDR_t* pAddr)//域名解析
{
	if(pHostname == NULL)
	{
		OSI_LOGI(0, "[bnd][socket](%d):get host by name, pHostname NULL", __LINE__);
		return -1;
	}

	struct sockaddr_in server_addr;
	struct hostent* server_host= gethostbyname(pHostname);

	OSI_LOGI(0, "[bnd][socket](%d):get host by name, start", __LINE__);
	memcpy( (void *) &server_addr.sin_addr,
            (void *) server_host->h_addr,
                     server_host->h_length);
	pAddr->addr = server_addr.sin_addr.s_addr;
	OSI_LOGI(0, "[bnd][socket](%d):get host by name, end", __LINE__);

	return 0;
}

char* AIR_SOCK_IPAddr_ntoa(AIR_SOCK_IP_ADDR_t* pIpAddr)//网络地址转字符IP
{
	static char IpAddr[20] = {0};
	OSI_LOGI(0, "[bnd][socket](%d):itoa-->sock ipaddr ntoa", __LINE__);
	__itoa(pIpAddr->addr, (char *)&IpAddr, 10);
	return (char *)IpAddr;
}

uint32_t AIR_SOCK_inet_addr(char * pIp)//字符串IP转int型
{
	OSI_LOGI(0, "[bnd][socket](%d):atoi-->sock inet addr", __LINE__);
	return atoi(pIp);
}

/*管道相关*/
AIR_OSI_Pipe_t* AIR_OSI_PipeCreate()//创建管道
{
	return (AIR_OSI_Pipe_t*)osiPipeCreate(320);
}

bool AIR_OSI_PipeDelete(AIR_OSI_Pipe_t* pipe)//删除管道
{
	osiPipeDelete((osiPipe_t *)pipe);
	return true;
}

int AIR_OSI_PipeWrite(AIR_OSI_Pipe_t* pipe,void*buf,int nbyte)//向管道写数据
{
	return osiPipeWrite((osiPipe_t *)pipe, (const void *)buf, (unsigned)nbyte);
}

int AIR_OSI_PipeRead(AIR_OSI_Pipe_t* pipe,void*buf,int nbyte)//从管道读数据
{
	return osiPipeRead((osiPipe_t *)pipe, buf, (unsigned)nbyte);
}

bool AIR_OSI_ThreadSleep(int ms)//延时函数
{
	osiThreadSleep(ms);
	return true;
}

AIR_OSI_Thread_t AIR_OSI_ThreadCurrent()//获取当前线程标识
{
	OSI_LOGI(0, "[bnd][system](%d):ThreadCurrent", __LINE__);
	AIR_OSI_Thread_t Threadid = (AIR_OSI_Thread_t)osiThreadCurrent();
	return Threadid;
}

AIR_OSI_Thread_t AIR_OSI_ThreadCreate(const char *name,      //创建线程
                                     AIR_OSI_Callback_t func,
                                     void *argument,
                                     uint32_t        priority,
                                     uint32_t        stack_size,
                                     uint32_t        event_count)
{
	OSI_LOGI(0, "[bnd][system](%d):ThreadCreate", __LINE__);
	AIR_OSI_Thread_t Threadid = (AIR_OSI_Thread_t)osiThreadCreate(name, func, argument,
								 priority, stack_size,
								 event_count);
	return Threadid;
}

bool AIR_OSI_ThreadExit()//标识线程退出，释放资源
{
	OSI_LOGI(0, "[bnd][system](%d):ThreadExit", __LINE__);
	osiThreadExit();
	return true;
}

bool AIR_OSI_ThreadSuspend(AIR_OSI_Thread_t tid)//暂停线程，暂未使用可空实现
{
	return true;
}

bool AIR_OSI_ThreadResume(AIR_OSI_Thread_t tid)//恢复线程，暂未使用可空实现
{
	return true;
}

unsigned int AIR_OSI_UpTime()//获取开机后的ms
{
	return (unsigned int)osiUpTime();
}

unsigned int AIR_OSI_EpochSecond()//获取时间戳
{
	OSI_LOGI(0, "[bnd][system](%d):Epoch Second", __LINE__);
	return (unsigned int)osiEpochSecond();
}

int lib_oem_send_uart(const char* data, int size)
{
	OSI_LOGI(0, "[bnd][uart](%d):send", __LINE__);
	return strlen(data);
}

/*tts functions*/
/*
* Function: lib_oem_tts_play
*
* Description:
* push the tts data
*
* Rarameter:
* text:unicode data
*
* Return:
* none
*/

int lib_oem_tts_play(char *text, int len)
{
	OSI_LOGI(0, "[bnd][tts](%d):tts play", __LINE__);
	ttsPlayText(text, len, ML_UTF8);
	return 0;
}

/*
* Function: lib_oem_tts_stop
*
* Description:
* stop the tts
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_tts_stop(void)
{
	OSI_LOGI(0, "[bnd][tts](%d):tts stop", __LINE__);
	ttsStop();
	return 0;
}

/*
* Function: lib_oem_tts_status
*
* Description:
* get the tts status
*
* Rarameter:
* none
*
* Return:
* 0-not work
* 1-working
*/

int lib_oem_tts_status(void)
{
	OSI_LOGI(0, "[bnd][tts](%d):tts status", __LINE__);
	if(!ttsIsPlaying())
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/*
* Function: lib_oem_start_record
*
* Description:
* start record for ptt or open device for record
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_start_record(void)
{
	if(pocBndAttr.recorder == 0)
	{
		return -1;
	}
	OSI_LOGI(0, "[bnd][record](%d):start record", __LINE__);
	pocAudioRecorderStart(pocBndAttr.recorder);
	m_BndUser.m_status = USER_OPRATOR_SPEAKING;
	return 0;
}

/*
* Function: lib_oem_stop_record
*
* Description:
* stop record or close device for record
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_stop_record(void)
{
	if(pocBndAttr.recorder == 0)
	{
		return -1;
	}
	OSI_LOGI(0, "[bnd][record](%d):stop record", __LINE__);
	pocAudioRecorderStop(pocBndAttr.recorder);
	return 0;
}

/*
* Function: lib_oem_start_play
*
* Description:
* start play pcm, or open device for play
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_start_play(void)
{
	if(m_BndUser.m_status < USER_ONLINE || pocBndAttr.player == 0)
	{
	    m_BndUser.m_iRxCount = 0;
	    m_BndUser.m_iTxCount = 0;
		return 1;
	}
	OSI_LOGI(0, "[bnd][play](%d):start play", __LINE__);
	pocAudioPlayerStart(pocBndAttr.player);
	m_BndUser.m_status = USER_OPRATOR_LISTENNING;
	return 0;
}

/*
* Function: lib_oem_stop_play
*
* Description:
* stop play pcm, or close device for play
*
* Rarameter:
* none
*
* Return:
* none
*/

int lib_oem_stop_play(void)
{
	if(pocBndAttr.player == 0)
	{
		return -1;
	}
	OSI_LOGI(0, "[bnd][play](%d):stop play", __LINE__);
	pocAudioPlayerStop(pocBndAttr.player);
	audevStopPlay();
	return 0;
}

/*
* Function: lib_oem_play
*
* Description:
* push  pcm data
*
* Rarameter:
* data:pcm data
* length: the length of data
*
* Return:
* receive data length
*/

int lib_oem_play(const char* data, int length)
{
	if(pocBndAttr.player == 0)
	{
		return 0;
	}
	OSI_LOGI(0, "[bnd][play](%d):start write data to play, lenth(%d)", __LINE__, length);
	return pocAudioPlayerWriteData(pocBndAttr.player, (const uint8_t *)data, length);
}

/*
* Function: lib_oem_play_tone
*
* Description:
* Play tone
*
* Rarameter:
* type:
* [0]--start ptt
* [1]--stop ptt
* [2]--error
* [3]--start play
* [4]--stop play
*
* Return:
* none
*/
int lib_oem_play_tone(int type)
{
	switch(type)
	{
		case 0://start ptt
		{
			OSI_LOGI(0, "[bnd][play](%d):start speak", __LINE__);
			//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Speak, 30, true);
			break;
		}

		case 1://stop ptt
		{
			OSI_LOGI(0, "[bnd][play](%d):stop speak", __LINE__);
			//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Speak, 30, true);
			break;
		}

		case 2://error
		{
			OSI_LOGI(0, "[bnd][play](%d):error", __LINE__);
			break;
		}

		case 3://start play
		{
			OSI_LOGI(0, "[bnd][play](%d):start listen", __LINE__);
			//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Listen, 30, true);
			break;
		}

		case 4://stop play
		{
			OSI_LOGI(0, "[bnd][play](%d):stop listen", __LINE__);
			//poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			break;
		}
	}
	return 0;
}

/* network functions*/

/*
* Function: lib_oem_socket_set_apn
*
* Description:
* set the new apn
*
* Rarameter:
* apn
*
* Return:
* none
*/

void lib_oem_socket_set_apn(char *apn)
{
	OSI_LOGI(0, "[bnd][net](%d):set new apn", __LINE__);
	strcpy(apn, "UNICON");
}

/*
* Function: lib_oem_socket_net_open
*
* Description:
* Open the network data
*
* Rarameter:
* none
*
* Return:
* 0-FAIL
* 1-SUCCESS
*/

int  lib_oem_socket_net_open(void)
{
	if(0 != CFW_SetComm(CFW_ENABLE_COMM, 0, 0, CFW_SIM_0))
	{
		return 0;
	}
	OSI_LOGI(0, "[bnd][net](%d):open net data", __LINE__);
	return 1;
}

/*
* Function: lib_oem_socket_get_net_status
*
* Description:
* Get network data status
*
* Parameter:
* none
*
* Return:
* 0-FAIL
* 1-SUCCESS
*/

int lib_oem_socket_get_net_status(void)
{
	uint32_t uErrCode = 0;
	CFW_NW_STATUS_INFO GprsStatus;

	uErrCode = CFW_GprsGetstatus(&GprsStatus, CFW_SIM_0);
	if (uErrCode != 0)
    {
        return 0;
    }
	if (GprsStatus.nStatus == CFW_NW_STATUS_REGISTERED_HOME || GprsStatus.nStatus == CFW_NW_STATUS_REGISTERED_ROAMING)
	{
		//TB_NETWORK_TYPE_4G;
	}
	else if (GprsStatus.nStatus == CFW_NW_STATUS_REGISTRATION_DENIED)
	{
		//TB_NETWORK_TYPE_LIMITED;
	}
	else
	{
		return 0;//TB_NETWORK_TYPE_NO_SERVICE;
	}
	OSI_LOGI(0, "[bnd][net](%d):net data normal", __LINE__);
	return 1;
}

/*
* Function: lib_oem_socket_get_network
*
* Description:
* Get network status
*
* Rarameter:
* none
*
* Return:
* -1-FAIL
* 0-
* 1-SUCCESS
*/

int lib_oem_socket_get_network(void)
{
	CFW_NW_STATUS_INFO nStatusInfo;
	if(CFW_NwGetStatus(&nStatusInfo, CFW_SIM_0) != 0)
	{
		return -1;
	}

	if(nStatusInfo.nStatus == 0
		|| nStatusInfo.nStatus == 2
		|| nStatusInfo.nStatus == 4)
	{
		return -1;
	}
	OSI_LOGI(0, "[bnd][net](%d):net exist", __LINE__);
	return 1;
}

/*
* Function: lib_oem_get_rssi
*
* Description:
* Get network signal strength
*
* Rarameter:
* none
*
* Return:
* rssi
*
*/

int lib_oem_get_rssi(void)
{
	OSI_LOGI(0, "[bnd][signal](%d):get rssi", __LINE__);
	return poc_get_signal_dBm(NULL);
}

/*
* Function: lib_oem_net_close
*
* Description:
* Close the network
*
* Rarameter:
* none
*
* Return:
* 0-SUCCESS
* 1-FAILURE
*
*/

int lib_oem_net_close(void)
{
	if(0 != CFW_SetComm(CFW_DISABLE_COMM, 0, 0, CFW_SIM_0))
	{
		return 1;
	}
	OSI_LOGI(0, "[bnd][net](%d):close net data", __LINE__);
	return 0;
}

/*
* Function: lib_oem_play_open_pcm
*
* Description:
* Initialize pcm for TTS
*
* Input:
* none
*
* Return:
* 0-FAILURE
* 1-SUCCESS
*
*/

int lib_oem_play_open_pcm()
{
	OSI_LOGI(0, "[bnd][tts](%d):play_open_pcm", __LINE__);
	return 1;
}

/*sim*/

/*
* Function: lib_oem_is_sim_present
*
* Description:
* Check the status of the SIM card
*
* Input:
* none
*
* Return:
* 0-SIMcard is absent
* 1-SIM card is present
*
*/
int lib_oem_is_sim_present(void)
{
	if(poc_check_sim_prsent(POC_SIM_1))
	{
		return 1;
	}
	OSI_LOGI(0, "[bnd][sim](%d):exist", __LINE__);
	return 0;
}

/*========================================================================================
FUNCTION:
lib_oem_get_meid

DESCRIPTION:
get meid

PARAMETERS:
data:meid

RETURN:
[0]SUCCESS
[-1]FAILURE

=========================================================================================*/

int lib_oem_get_meid(char* data)
{
	OSI_LOGI(0, "[bnd][meid](%d):finish get", __LINE__);
	return 0;
}

/*========================================================================================
FUNCTION:
lib_oem_get_imei

DESCRIPTION:
get imei

PARAMETERS:
data:imei

RETURN:
[0]SUCCESS
[-1]FAILURE

=========================================================================================*/

int lib_oem_get_imei(char* data)
{
	OSI_LOGI(0, "[bnd][imei](%d):start get", __LINE__);
	if(data == NULL)
	{
		return -1;
	}
	poc_get_device_imei_rep((int8_t *)data);
	OSI_LOGI(0, "[bnd][imei](%d):finish get", __LINE__);
	return 0;
}

/*========================================================================================
FUNCTION:
lib_oem_get_imsi

DESCRIPTION:
get imsi

PARAMETERS:
data:imsi

RETURN:
[0]SUCCESS
[-1]FAILURE

=========================================================================================*/

int lib_oem_get_imsi(char* data)
{
	OSI_LOGI(0, "[bnd][imsi](%d):start get", __LINE__);
	if(data == NULL)
	{
		return -1;
	}
	poc_get_device_imsi_rep((int8_t *)data);
	OSI_LOGI(0, "[bnd][imsi](%d):finish get", __LINE__);
	return 0;
}

/*========================================================================================
FUNCTION:
lib_oem_get_iccid

DESCRIPTION:
get iccid

PARAMETERS:
data:iccid

RETURN:
[0]SUCCESS
[-1]FAILURE

=========================================================================================*/

int lib_oem_get_iccid(char* data)
{
	OSI_LOGI(0, "[bnd][iccid](%d):start get", __LINE__);
	if(data == NULL)
	{
		return -1;
	}
	poc_get_device_iccid_rep((int8_t *)data);
	OSI_LOGI(0, "[bnd][iccid](%d):finish get", __LINE__);
	return 0;
}


/*========================================================================================
FUNCTION:
lib_oem_get_system_version

DESCRIPTION:
get system version

RETURN:
version
=========================================================================================*/
char* lib_oem_get_system_version(void)
{
	return 0;
}

/*========================================================================================
FUNCTION:
lib_oem_get_other_version

DESCRIPTION:
get lua version

RETURN:
version
=========================================================================================*/
char* lib_oem_get_other_version(void)
{
	return 0;
}

/**远程升级初始化
*@return 0:  表示成功
*   <0：  表示失败
**/
int lib_ota_init(void)
{
	return 0;
}

/**远程升级下载
*@param  data:    下载固件包数据
*@param  len:    下载固件包长度
*@param  total:    固件包总大小
*@return 0:  表示成功
*   <0：  表示失败
**/
int lib_ota_process(char* data, unsigned int len, unsigned int total)
{
	return 0;
}

/**远程升级结束且校验
*@return 0:  表示成功, 成功后模块自动重启升级
*   <0：  表示失败
**/
int lib_ota_done(void)
{
	return 0;
}

/*******************************************************************************************************/
void callback_BND_Audio(AUDIO_STATE state, bnd_uid_t uid, const char* name, int flag)
{
    const char* str = NULL;
	OSI_LOGXI(OSI_LOGPAR_S, 0, "[cbaudio](%s)(%d)", __FUNCTION__, __LINE__);
    switch(state)
	{
        case BND_SPEAK_START:
        {
	        str = "BND_SPEAK_START";
	        break;
        }

        case BND_SPEAK_STOP:
        {
	        str = "BND_SPEAK_STOP";
	        break;
        }

        case BND_LISTEN_START:
        {
	        str = "BND_LISTEN_START";
	        break;
        }

        case BND_LISTEN_STOP:
        {
	        str = "BND_LISTEN_STOP";
	        break;
        }

		default:
		{
			break;
		}
    }

	OSI_LOGXI(OSI_LOGPAR_ISI, 0, "[cbaudio]state:(%s), uid:(%d), name:(%s), flag:(%d)", str, uid, name, flag);
}

void callback_BND_Join_Group(const char* groupname, bnd_gid_t gid)
{
    OSI_LOGXI(OSI_LOGPAR_SIS, 0, "[cbjoingroup](%s)(%d), gid:(%d), gname:(%s)", __FUNCTION__, __LINE__, gid, groupname);
}

void callback_BND_ListUpdate(int flag)
{
    const char* str = NULL;

	switch(flag)
	{
		case 1:
		{
			str = "you need get grouplist -- update";
			break;
		}

		case 2:
		{
			str = "you need get memberlist -- update";
			break;
		}

		default:
		{
			break;
		}
	}

    OSI_LOGXI(OSI_LOGPAR_SS, 0, "[cblistupdate](%s), flag(%s)", __FUNCTION__, str);
}

void callback_BND_MemberChange(int flag, bnd_gid_t gid, int num, bnd_uid_t* uids)
{
    if(flag==1)
	{
        OSI_LOGXI(OSI_LOGPAR_SI, 0, "[cbmemberchange]long_ung %s member leve group num:%d\n", __FUNCTION__, num);
    }
}

void callback_BND_Login_State(int online)
{
    const char* str;
    if(online == USER_ONLINE)
    {
        str = "USER_ONLINE";
		if(pocBndAttr.loginstatus_t != LVPOCBNDCOM_SIGNAL_LOGIN_SUCCESS)
		{
			pocBndAttr.invaildlogininf = false;
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LOGIN_REP, (void *)&online);
		}
    }
    else
    {
        str = "USER_OFFLINE";
		pocBndAttr.loginstatus_t = LVPOCBNDCOM_SIGNAL_LOGIN_EXIT;
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LOGIN_REP, (void *)&online);
    }
    OSI_LOGXI(OSI_LOGPAR_SS, 0, "[cblogin]long_ung %s, %s\n", __FUNCTION__, str);
}

static void LvGuiBndCom_delay_close_listen_timer_cb(void *ctx)
{
	pocBndAttr.delay_close_listen_timer_running = false;
    if(m_BndUser.m_status > USER_OFFLINE)
    {
	    m_BndUser.m_status = USER_ONLINE;
    }
    m_BndUser.m_iRxCount = 0;
    m_BndUser.m_iTxCount = 0;
	lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
	lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP, NULL);

	#if GUIBNDCOM_LISTEN_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[idtlisten]%s(%d):delayclose_listen_timer cb", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif
}

static void LvGuiBndCom_start_play_tone_timer_cb(void *ctx)
{
	lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND, NULL);
	lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND, NULL);
	lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP, NULL);
	pocBndAttr.start_speak_voice_timer_running = false;
}

static void LvGuiBndCom_start_speak_voice_timer_cb(void *ctx)
{
	//goto play start speak tone
	poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Speak, 30, true);
	osiTimerStart(pocBndAttr.play_tone_timer, 200);//200ms

	#if GUIBNDCOM_SPEAK_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[idtspeak]%s(%d):start speak, timer cb", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif
}

static void LvGuiBndCom_get_member_list_timer_cb(void *ctx)
{
	if(!pocBndAttr.isPocMemberListBuf)
	{
		if(lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP, NULL))
		{
			return;
		}
	}
	osiTimerStartRelaxed(pocBndAttr.get_member_list_timer, 1000, OSI_WAIT_FOREVER);
}

static void LvGuiBndCom_get_group_list_timer_cb(void *ctx)
{
	if(!lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF, NULL))
	{
		if(pocBndAttr.isReady)
		{
			osiTimerStartRelaxed(pocBndAttr.get_group_list_timer, 1000, OSI_WAIT_FOREVER);
		}
		return;
	}

	#if GUIBNDCOM_LOCKGROUP_DEBUG_LOG|GUIBNDCOM_GROUPLISTREFR_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[idtlockgroup][grouprefr]%s(%d):start get lock_group_status and query include_self group", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif
	osiTimerStartRelaxed(pocBndAttr.get_lock_group_status_timer, 500, OSI_WAIT_FOREVER);

	#if 0
	osiTimerStart(pocBndAttr.get_group_list_timer, 1000 * 60 * 5, OSI_WAIT_FOREVER);
	#endif
}

static void LvGuiBndCom_get_lock_group_status_timer_cb(void *ctx)
{
	if(pocBndAttr.lockGroupOpt > LV_POC_GROUP_OPRATOR_TYPE_NONE)
	{
		osiTimerStartRelaxed(pocBndAttr.get_lock_group_status_timer, 1000, OSI_WAIT_FOREVER);

		#if GUIBNDCOM_LOCKGROUP_DEBUG_LOG
		char cOutstr[256] = {0};
		cOutstr[0] = '\0';
		sprintf(cOutstr, "[idtlockgroup]%s(%d):get lock group status timer cb -> restart 1s timer", __func__, __LINE__);
		OSI_LOGI(0, cOutstr);
		#endif

		return;
	}

	if(!lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND, NULL))
	{
		if(pocBndAttr.isReady)
		{
			osiTimerStartRelaxed(pocBndAttr.get_lock_group_status_timer, 2000, OSI_WAIT_FOREVER);
		}
		return;
	}

	#if GUIBNDCOM_LOCKGROUP_DEBUG_LOG
	char cOutstr[256] = {0};
	cOutstr[0] = '\0';
	sprintf(cOutstr, "[idtlockgroup]%s(%d):send msg to get lock group status", __func__, __LINE__);
	OSI_LOGI(0, cOutstr);
	#endif
}

static void LvGuiBndCom_check_listen_timer_cb(void *ctx)
{
	if(pocBndAttr.check_listen_count < 1
		|| m_BndUser.m_status < USER_OPRATOR_START_LISTEN
		|| m_BndUser.m_status > USER_OPRATOR_LISTENNING)
	{
		if(m_BndUser.m_status > USER_OFFLINE)
		{
			m_BndUser.m_status = USER_ONLINE;
		}

	    if(pocBndAttr.delay_close_listen_timer_running)
	    {
		    osiTimerStop(pocBndAttr.delay_close_listen_timer);
		    pocBndAttr.delay_close_listen_timer_running = false;
	    }
	    osiTimerStart(pocBndAttr.delay_close_listen_timer, 460);
	    pocBndAttr.delay_close_listen_timer_running = true;
		pocBndAttr.check_listen_count = 0;
		osiTimerStop(pocBndAttr.check_listen_timer);
		return;
	}
	pocBndAttr.check_listen_count--;
}

static void LvGuiBndCom_try_login_timer_cb(void *ctx)
{
	if(lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LOGIN_IND, NULL))
	{
		osiTimerStop(pocBndAttr.try_login_timer);
	}
	osiTimerStop(pocBndAttr.try_login_timer);
}

static void LvGuiBndCom_auto_login_timer_cb(void *ctx)
{
	if(pocBndAttr.loginstatus_t == LVPOCBNDCOM_SIGNAL_LOGIN_FAILED)
	{
		if(lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LOGIN_IND, NULL))
		{
			OSI_LOGI(0, "[song]autologin ing...[3]\n");
			pocBndAttr.loginstatus_t = LVPOCBNDCOM_SIGNAL_LOGIN_ING;
		}
	}
}

static void LvGuiBndCom_ppt_release_timer_cb(void *ctx)
{
	static int makecallcnt = 0;
	bool pttStatus = pocGetPttKeyState()|lv_poc_get_earppt_state();
	if(true == pttStatus && pocBndAttr.is_makeout_call == true)
	{
		osiTimerStart(pocBndAttr.monitor_pptkey_timer, 50);
		makecallcnt++;
	}
	else//ppt release
	{
		if(makecallcnt < 10)//press time < 500ms
		{
			lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP, (void *)USER_OPRATOR_SPEAKING);
		}
		makecallcnt = 0;
	}
}

static void LvGuiBndCom_recorder_timer_cb(void *ctx)
{
	lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND, NULL);
}

static void LvGuiBndCom_refresh_group_adduser_timer_cb(void *ctx)
{
#if GUIBNDCOM_GROUPLISTREFR_DEBUG_LOG|GUIBNDCOM_GROUPLISTDEL_DEBUG_LOG
	char cOutstr6[128] = {0};
	cOutstr6[0] = '\0';
	sprintf(cOutstr6, "[grouprefr]%s(%d):opt OPT_G_ADDUSER or OPT_G_DELUSER", __func__, __LINE__);
	OSI_LOGI(0, cOutstr6);
#endif

	lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF, NULL);
}

static void prvPocGuiBndTaskHandleLogin(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_LOGIN_IND:
		{


			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_LOGIN_REP:
		{
			if(pocBndAttr.player == 0)
			{
				pocBndAttr.player = pocAudioPlayerCreate(81920);
			}

			if(pocBndAttr.recorder == 0)
			{
				pocBndAttr.recorder = pocAudioRecorderCreate(40960, 320, 20, lvPocGuiIdtCom_send_data_callback);
			}

			if(pocBndAttr.player == 0 || pocBndAttr.recorder == 0)
			{
				pocBndAttr.isReady = false;
				osiThreadExit();
			}

			if(ctx == 0)
			{
				break;
			}
			int *login_info = (int *)ctx;
			m_BndUser.m_status = *login_info;

			if (USER_OFFLINE == m_BndUser.m_status)
			{
				if(pocBndAttr.onepoweron == false
					&& !lv_poc_watchdog_status())
				{
					pocBndAttr.delay_play_voice++;

					if(pocBndAttr.delay_play_voice >= 30 || pocBndAttr.delay_play_voice == 1)
					{
						pocBndAttr.delay_play_voice = 1;
						poc_play_voice_one_time(LVPOCAUDIO_Type_No_Login, 50, true);
					}
				}

				//开启自动登陆功能
				osiTimerStart(pocBndAttr.auto_login_timer, 20000);

				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_1500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
				m_BndUser.m_status = USER_OFFLINE;
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "登录失败");
				osiTimerStop(pocBndAttr.get_group_list_timer);
				osiTimerStop(pocBndAttr.get_lock_group_status_timer);
				lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND, NULL);
				pocBndAttr.isReady = true;
				break;
			}
			pocBndAttr.isReady = true;

			if(m_BndUser.m_status < USER_ONLINE)//登录成功
			{
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
				pocBndAttr.loginstatus_t = LVPOCBNDCOM_SIGNAL_LOGIN_SUCCESS;

				if(pocBndAttr.onepoweron == false
					&& !lv_poc_watchdog_status())
				{
					poc_play_voice_one_time(LVPOCAUDIO_Type_Success_Login, 50, true);
				}
				pocBndAttr.onepoweron = true;
				osiTimerStop(pocBndAttr.auto_login_timer);
			}
			m_BndUser.m_status = USER_ONLINE;
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "成功登录");
			osiTimerStart(pocBndAttr.monitor_recorder_timer, 20000);//20S

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, NULL);
			osiTimerStartRelaxed(pocBndAttr.get_group_list_timer, 500, OSI_WAIT_FOREVER);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_EXIT_IND:
		{
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_EXIT_REP:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleSpeak(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_IND:
		{

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_IND:
		{

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleMic(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_MIC_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MIC_REP:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleGroupList(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocBndAttr.pocGetGroupListCb = (lv_poc_get_group_list_cb_t)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
		{
			pocBndAttr.pocGetGroupListCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleBuildGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			if(pocBndAttr.pocBuildGroupCb == NULL)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			if(pocBndAttr.pocBuildGroupCb == NULL)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			#if GUIBNDCOM_BUILDGROUP_DEBUG_LOG
			char cOutstr[256] = {0};
    		cOutstr[0] = '\0';
    		sprintf(cOutstr, "[buildgroup]%s(%d):register build group cb!", __func__, __LINE__);
    		OSI_LOGI(0, cOutstr);
			#endif

			pocBndAttr.pocBuildGroupCb = (poc_build_group_cb)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND:
		{
			#if GUIBNDCOM_BUILDGROUP_DEBUG_LOG
			char cOutstr[256] = {0};
    		cOutstr[0] = '\0';
    		sprintf(cOutstr, "[buildgroup]%s(%d):cannel build group cb!", __func__, __LINE__);
    		OSI_LOGI(0, cOutstr);
			#endif

			pocBndAttr.pocBuildGroupCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleMemberList(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
		{
			if(pocBndAttr.pocGetMemberListCb == NULL)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
		{
			if(pocBndAttr.pocGetMemberListCb == NULL)
			{
				break;
			}
			pocBndAttr.pocGetMemberListCb(1, pocBndAttr.pPocMemberListBuf->dwNum, pocBndAttr.pPocMemberListBuf);
			pocBndAttr.pocGetMemberListCb = NULL;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocBndAttr.pocGetMemberListCb = (lv_poc_get_member_list_cb_t)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			pocBndAttr.pocGetMemberListCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleCurrentGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_SET_CURRENT_GROUP_IND:
		{
			if(pocBndAttr.pocSetCurrentGroupCb == NULL)
			{
				break;
			}

			if(ctx == 0)
			{
				pocBndAttr.pocSetCurrentGroupCb(0);
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocBndAttr.pocSetCurrentGroupCb = (poc_set_current_group_cb)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
		{
			pocBndAttr.pocSetCurrentGroupCb = NULL;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleMemberInfo(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP:
		{
			if(ctx == 0 || !poc_get_lcd_status())
			{
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

static void prvPocGuiBndTaskHandleMemberStatus(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_MEMBER_STATUS_REP:
		{
			if(pocBndAttr.pocGetMemberStatusCb == NULL)
			{
				break;
			}

			pocBndAttr.pocGetMemberStatusCb(ctx > 0);
			pocBndAttr.pocGetMemberStatusCb = NULL;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
		{
			if(ctx == 0)
			{
				break;
			}
			pocBndAttr.pocGetMemberStatusCb = (poc_get_member_status_cb)ctx;
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_MEMBER_STATUS_CB_REP:
		{
			pocBndAttr.pocGetMemberStatusCb = NULL;
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
		case LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_START_PLAY_IND:
		{
			if(m_BndUser.m_status < USER_ONLINE)
			{
			    m_BndUser.m_iRxCount = 0;
			    m_BndUser.m_iTxCount = 0;
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

static void prvPocGuiBndTaskHandleRecord(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND:
		{
			if(m_BndUser.m_status < USER_ONLINE || m_BndUser.m_status < USER_OPRATOR_START_SPEAK || m_BndUser.m_status > USER_OPRATOR_SPEAKING)
			{
				lvPocGuiBndCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MIC_REP, GUIIDTCOM_RELEASE_MIC);
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

static void prvPocGuiBndTaskHandleMemberCall(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
		{
		    if(ctx == 0)
		    {
			    break;
		    }

			if(pocBndAttr.listen_status == true)
			{
				/*speak cannot membercall*/
				return;
			}
		    lv_poc_member_call_config_t *member_call_config = (lv_poc_member_call_config_t *)ctx;

//		    Msg_GROUP_MEMBER_s *member_call_obj = (Msg_GROUP_MEMBER_s *)member_call_config->members;
//		    Msg_GROUP_MEMBER_s *member = member_call_obj;

		    if(member_call_config->func == NULL)
		    {
			    break;
		    }

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
		{
			 //对方同意了单呼通话
		    if(pocBndAttr.pocMemberCallCb != NULL)
		    {
			    pocBndAttr.pocMemberCallCb(0, 0);
			    pocBndAttr.pocMemberCallCb = NULL;
		    }
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
		{
			//对方或者服务器释放或者拒绝了单呼通话
			pocBndAttr.is_member_call = false;
			pocBndAttr.member_call_dir = 0;

			lv_poc_activity_func_cb_set.member_call_close();
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleListen(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_LISTEN_START_REP:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP:
		{
			pocBndAttr.listen_status = false;

			/*恢复run闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_RUN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");

			if((pocBndAttr.membercall_count > 1 && pocBndAttr.is_member_call == true) || pocBndAttr.is_member_call == false)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)"停止聆听", (const uint8_t *)"");
				poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			}

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);

			#if GUIBNDCOM_LISTEN_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[idtlisten]%s(%d):stop listen rep and destory idle,windows", __func__, __LINE__);
			OSI_LOGI(0, cOutstr);
			#endif

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP:
		{
			char speaker_name[100];
			char *speaker_group = (char *)pocBndAttr.speaker_group.m_ucGName;
			memset(speaker_name, 0, sizeof(char) * 100);
			strcpy(speaker_name, (const char *)pocBndAttr.speaker.ucName);
			strcat(speaker_name, (const char *)"正在讲话");

			pocBndAttr.listen_status = true;
			/*开始闪烁*/
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			//member call
			if(pocBndAttr.is_member_call)
			{
				if(pocBndAttr.membercall_count > 1)//单呼进入第二次
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)"");
				}
			}
			else
			{
				char speaker_group_name[100];
				strcpy(speaker_group_name, (const char *)pocBndAttr.speaker_group.m_ucGName);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, speaker_name, speaker_group);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)speaker_group_name);
			}

			#if GUIBNDCOM_LISTEN_DEBUG_LOG
			char cOutstr[256] = {0};
			cOutstr[0] = '\0';
			sprintf(cOutstr, "[idtlisten]%s(%d):start listen_speaker rep and display idle,windows", __func__, __LINE__);
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

static void prvPocGuiBndTaskHandleGuStatus(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_GU_STATUS_REP:/*组或者用户状态发生变化*/
		{
			if(ctx == 0)
			{
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

static void prvPocGuiBndTaskHandleGroupOperator(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_GROUP_OPERATOR_REP:
		{
			if(ctx == 0)
			{
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

static void prvPocGuiBndTaskHandleReleaseListenTimer(uint32_t id, uint32_t ctx)
{
	if(pocBndAttr.delay_close_listen_timer == NULL)
	{
		return;
	}

	osiTimerDelete(pocBndAttr.delay_close_listen_timer);
	pocBndAttr.delay_close_listen_timer = NULL;
}

static void prvPocGuiBndTaskHandleLockGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_REP:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_REP:
		{
			break;
		}

		default:
			break;
	}
}

static void prvPocGuiBndTaskHandleDeleteGroup(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_REP:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleOther(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_DELAY_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SCREEN_ON_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_SCREEN_OFF_IND:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleChargerOpt(uint32_t id, uint32_t ctx)
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
			/*add shutdown opt*/
			osiShutdown(OSI_SHUTDOWN_POWER_OFF);
			break;
		}
	}

}

static
void prvPocGuiBndTaskHandleCallFailed(uint32_t id, uint32_t ctx, uint32_t cause_str)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_GET_SPEAK_CALL_CASE:
		{
			break;
		}
	}
}


static void prvPocGuiBndTaskHandleUploadGpsInfo(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUIIDTCOM_SIGNAL_GPS_UPLOADING_IND:
      {
         break;
      }

      default:
         break;
   }
}

static void prvPocGuiBndTaskHandleRecorderSuspendorResumeOpt(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND:
		{
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_RESUME_IND:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void get_memberlist(int begin, int count)
{
    bnd_group_t grp;
    bnd_member_t mm[16];
    memset(&grp,0, sizeof(bnd_group_t));
    broad_group_getbyid(0, &grp);
    OSI_LOGXI(OSI_LOGPAR_IS, 0, "long_ung gid:%d, gname:%s\n",grp.gid, grp.name);
    if(grp.gid){
        int size;
        size = broad_get_membercount(grp.gid);
        OSI_LOGI(0, "long_ung member num:%d\n",size);
        memset(mm, 0, sizeof(bnd_member_t)*16);
        size = broad_get_memberlist(grp.gid, mm, 16, begin, count);
        if(size){
            int i;
            for(i=0;i<size;i++){
                OSI_LOGXI(OSI_LOGPAR_IIS, 0, "long_ung [%d]uid:%d,name:%s\n",mm[i].index,mm[i].uid, mm[i].name);
            }
        }
        OSI_LOGI(0, "long_ung member-size:%d\n", size);
    }
}

static void get_grouplist(int begin, int count)
{
    int size;
    bnd_group_t groups[16];
    memset(groups,0,sizeof(bnd_group_t)*16);
    size = broad_get_grouplist(groups, 16, begin, count);
    if(size){
        int i;
        for(i=0;i<size;i++){
            OSI_LOGXI(OSI_LOGPAR_IIS, 0, "long_ung [%d]gid:%d,name:%s\n",groups[i].index,groups[i].gid, groups[i].name);
        }
    }
    OSI_LOGI(0, "long_ung group-count:%d\n", size);
}

static void pocGuiBndComTaskEntry(void *argument)
{
	osiEvent_t event = {0};

    pocBndAttr.isReady = true;
	m_BndUser.Reset();
	//bnd init
	broad_init();
	broad_log(1);
    char ver[64] = {0};
    broad_get_version(ver);
    OSI_LOGXI(OSI_LOGPAR_S, 0, "[bnd]long_ung version:%s\n",ver);
    broad_register_audio_cb(callback_BND_Audio);
    broad_register_join_group_cb(callback_BND_Join_Group);
    broad_register_listupdate_cb(callback_BND_ListUpdate);
    broad_register_member_change_cb(callback_BND_MemberChange);
    osiThreadSleep(1000);
    broad_login(callback_BND_Login_State);

    while(1)
    {
    	if(!osiEventWait(pocBndAttr.thread, &event))
		{
			if(USER_ONLINE == m_BndUser.m_status)
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
			case LVPOCGUIBNDCOM_SIGNAL_LOGIN_IND:
			case LVPOCGUIBNDCOM_SIGNAL_LOGIN_REP:
			case LVPOCGUIBNDCOM_SIGNAL_EXIT_IND:
			case LVPOCGUIBNDCOM_SIGNAL_EXIT_REP:
			{
				prvPocGuiBndTaskHandleLogin(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_REP:
			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_REP:
			{
				prvPocGuiBndTaskHandleSpeak(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_MIC_IND:
			case LVPOCGUIIDTCOM_SIGNAL_MIC_REP:
			{
				prvPocGuiBndTaskHandleMic(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_IND:
			case LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_REP:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
			{
				prvPocGuiBndTaskHandleGroupList(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_REP:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND:
			{
				prvPocGuiBndTaskHandleBuildGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
			{
				prvPocGuiBndTaskHandleMemberList(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SET_CURRENT_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
			{
				prvPocGuiBndTaskHandleCurrentGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_IND:
			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_REP:
			{
				prvPocGuiBndTaskHandleMemberInfo(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_MEMBER_STATUS_REP:
			case LVPOCGUIIDTCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
			case LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_MEMBER_STATUS_CB_REP:
			{
				prvPocGuiBndTaskHandleMemberStatus(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_STOP_PLAY_IND:
			case LVPOCGUIIDTCOM_SIGNAL_START_PLAY_IND:
			{
				prvPocGuiBndTaskHandlePlay(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_STOP_RECORD_IND:
			case LVPOCGUIIDTCOM_SIGNAL_START_RECORD_IND:
			{
				prvPocGuiBndTaskHandleRecord(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
			case LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
			{
				prvPocGuiBndTaskHandleMemberCall(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_LISTEN_START_REP:
			case LVPOCGUIIDTCOM_SIGNAL_LISTEN_STOP_REP:
			case LVPOCGUIIDTCOM_SIGNAL_LISTEN_SPEAKER_REP:
			{
				prvPocGuiBndTaskHandleListen(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GU_STATUS_REP:
			{
				prvPocGuiBndTaskHandleGuStatus(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GROUP_OPERATOR_REP:
			{
				prvPocGuiBndTaskHandleGroupOperator(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_RELEASE_LISTEN_TIMER_REP:
			{
				prvPocGuiBndTaskHandleReleaseListenTimer(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_LOCK_GROUP_REP:
			case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_GROUP_REP:
			{
				prvPocGuiBndTaskHandleLockGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_IND:
			case LVPOCGUIIDTCOM_SIGNAL_DELETE_GROUP_REP:
			case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_IND:
    		case LVPOCGUIIDTCOM_SIGNAL_UNLOCK_BE_DELETED_LOCK_GROUP_REP:
			{
				prvPocGuiBndTaskHandleDeleteGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_DELAY_IND:
			case LVPOCGUIIDTCOM_SIGNAL_GET_MEMBER_LIST_CUR_GROUP:
			case LVPOCGUIIDTCOM_SIGNAL_GET_GROUP_LIST_INCLUDE_SELF:
			case LVPOCGUIIDTCOM_SIGNAL_GET_LOCK_GROUP_STATUS_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SCREEN_ON_IND:
			case LVPOCGUIIDTCOM_SIGNAL_SCREEN_OFF_IND:
			{
				prvPocGuiBndTaskHandleOther(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GET_SPEAK_CALL_CASE://获取对讲原因
			{
				prvPocGuiBndTaskHandleCallFailed(event.param1, event.param2, event.param3);
				break;
			}


			case LVPOCGUIIDTCOM_SIGNAL_SET_SHUTDOWN_POC:
			{
				bool status;

				status = lv_poc_get_charge_status();
				if(status == false)
				{
					prvPocGuiBndTaskHandleChargerOpt(event.param1, event.param2);
				}
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_GPS_UPLOADING_IND:
            {
            	prvPocGuiBndTaskHandleUploadGpsInfo(event.param1, event.param2);
				break;
			}

			case LVPOCGUIIDTCOM_SIGNAL_SUSPEND_IND:
			case LVPOCGUIIDTCOM_SIGNAL_RESUME_IND:
			{
				prvPocGuiBndTaskHandleRecorderSuspendorResumeOpt(event.param1, event.param2);
				break;
			}

			default:
				OSI_LOGW(0, "[gic] receive a invalid event\n");
				break;
		}
    }
}

extern "C" void pocGuiBndComStart(void)
{
    pocBndAttr.thread = osiThreadCreate(
		"pocGuiBndCom", pocGuiBndComTaskEntry, NULL,
		APPTEST_THREAD_PRIORITY, APPTEST_STACK_SIZE,
		APPTEST_EVENT_QUEUE_SIZE);
	pocBndAttr.delay_close_listen_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_delay_close_listen_timer_cb, NULL);
	pocBndAttr.start_speak_voice_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_start_speak_voice_timer_cb, NULL);
	pocBndAttr.get_member_list_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_get_member_list_timer_cb, NULL);
	pocBndAttr.get_group_list_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_get_group_list_timer_cb, NULL);
	pocBndAttr.get_lock_group_status_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_get_lock_group_status_timer_cb, NULL);
	pocBndAttr.check_listen_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_check_listen_timer_cb, NULL);
	pocBndAttr.try_login_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_try_login_timer_cb, NULL);//注册尝试登录定时器
	pocBndAttr.auto_login_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_auto_login_timer_cb, NULL);//注册自动登录定时器
	pocBndAttr.monitor_pptkey_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_ppt_release_timer_cb, NULL);//检查ppt键定时器
	pocBndAttr.monitor_recorder_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_recorder_timer_cb, NULL);//检查是否有人在录音定时器
	pocBndAttr.play_tone_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_start_play_tone_timer_cb, NULL);
	pocBndAttr.addgroup_user_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_refresh_group_adduser_timer_cb, NULL);
}

static void lvPocGuiIdtCom_send_data_callback(uint8_t * data, uint32_t length)
{
    if (pocBndAttr.recorder == 0 || m_BndUser.m_iCallId == -1 || data == NULL || length < 1)
    {
	    return;
    }
    if(m_BndUser.m_status != USER_OPRATOR_SPEAKING)
    {
	    return;
    }
//    IDT_CallSendAuidoData(m_BndUser.m_iCallId,
//						    0,
//						    0,
//						    data,
//						    length,
//						    0);
    m_BndUser.m_iTxCount = m_BndUser.m_iTxCount + 1;
}

extern "C" void lvPocGuiBndCom_Init(void)
{
	memset(&pocBndAttr, 0, sizeof(PocGuiBndComAttr_t));
	pocBndAttr.membercall_count = 0;
	pocBndAttr.is_release_call = true;
	pocBndAttr.pPocMemberList = (Msg_GData_s *)malloc(sizeof(Msg_GData_s));
	pocBndAttr.pPocMemberListBuf = (Msg_GData_s *)malloc(sizeof(Msg_GData_s));
	pocBndAttr.pLockGroup = (CGroup *)malloc(sizeof(CGroup));
	if(pocBndAttr.pLockGroup != NULL)
	{
		memset(pocBndAttr.pLockGroup, 0, sizeof(CGroup));
	}
	pocGuiBndComStart();
}

extern "C" bool lvPocGuiBndCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx)
{
    if (pocBndAttr.thread == NULL || (signal != LVPOCGUIBNDCOM_SIGNAL_LOGIN_IND && pocBndAttr.isReady == false))
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

	return osiEventSend(pocBndAttr.thread, &event);
}

extern "C" void lvPocGuiBndCom_log(void)
{
	lvPocGuiBndCom_Init();
}

bool lvPocGuiBndCom_get_status(void)
{
	return m_BndUser.m_status;
}

bool lvPocGuiBndCom_get_listen_status(void)
{
	return pocBndAttr.listen_status;
}

bool lvPocGuiBndCom_get_speak_status(void)
{
	return pocBndAttr.speak_status;
}

extern "C" void *lvPocGuiBndCom_get_self_info(void)
{
	if(m_BndUser.m_status < USER_ONLINE)
	{
		return NULL;
	}
	return (void *)&pocBndAttr.self_info;
}

extern "C" void *lvPocGuiBndCom_get_current_group_info(void)
{
	if(m_BndUser.m_status < USER_ONLINE)
	{
		return NULL;
	}
	return (void *)&m_BndUser.m_Group.m_Group[pocBndAttr.current_group];
}

extern "C" void *lvPocGuiBndCom_get_current_lock_group(void)
{
	if(!pocBndAttr.isLockGroupStatus || pocBndAttr.pLockGroup == NULL)
	{
		return NULL;
	}
	return pocBndAttr.pLockGroup;
}

extern "C" int lvPocGuiBndCom_get_current_exist_selfgroup(void)
{
	unsigned long i;
	nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

	for (i = 0; i < m_BndUser.m_Group.m_Group_Num; i++)
	{
		if(0 == strcmp((char *)poc_config->selfbuild_group_num, (char *)m_BndUser.m_Group.m_Group[i].m_ucGNum))
		{
			break;
		}
	}

	if(i == m_BndUser.m_Group.m_Group_Num)
	{
		poc_config->is_exist_selfgroup = LVPOCGROUPIDTCOM_SIGNAL_SELF_NO;
	}

	return poc_config->is_exist_selfgroup;
}

#endif

