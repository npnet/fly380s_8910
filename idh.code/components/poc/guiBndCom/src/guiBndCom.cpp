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
//group number
#define GUIBNDCOM_GROUPMAX            32
//member number
#define GUIBNDCOM_MEMBERMAX            128

//debug log info config                        cool watch搜索项
#define GUIBNDCOM_BUILDGROUP_DEBUG_LOG      1  //[buildgroup]
#define GUIBNDCOM_GROUPLISTREFR_DEBUG_LOG   1  //[grouprefr]
#define GUIBNDCOM_GROUPOPTACK_DEBUG_LOG     1  //[groupoptack]
#define GUIBNDCOM_GROUPLISTDEL_DEBUG_LOG    1  //[groupdel]
#define GUIBNDCOM_MEMBERREFR_DEBUG_LOG      1  //[memberrefr]
#define GUIBNDCOM_SPEAK_DEBUG_LOG           1  //[speak]
#define GUIBNDCOM_SINGLECALL_DEBUG_LOG      1  //[singlecall]
#define GUIBNDCOM_LISTEN_DEBUG_LOG          1  //[listen]
#define GUIBNDCOM_ERRORINFO_DEBUG_LOG       1  //[errorinfo]
#define GUIBNDCOM_LOCKGROUP_DEBUG_LOG       1  //[lockgroup]

//windows note
#define GUIBNDCOM_WINDOWS_NOTE    		1
#define GUIBNDCOM_BND_LOG    		    0

enum{
	USER_OPRATOR_START_SPEAK = 4,
	USER_OPRATOR_SPEAKING  = 5,
	USER_OPRATOR_START_LISTEN = 6,
	USER_OPRATOR_LISTENNING  = 7,
	USER_OPRATOR_PPT_RELEASE  = 8,
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
        for (i = 0; i < GUIBNDCOM_GROUPMAX; i++)
        {
            m_Group[i].m_ucGName[0] = 0;
            m_Group[i].m_ucGNum[0] = 0;
			m_Group[i].index = 0;
        }
        return 0;
    }

public:
	unsigned int m_Group_Num;
    CGroup  m_Group[GUIBNDCOM_GROUPMAX];//一个用户,最多处于32个组中
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

//GPS基本数据
typedef struct _BND_GPS_DATA_s
{
    float           longitude;          //经度
    float           latitude;           //纬度
    float           speed;              //速度
    float           direction;          //方向
    //时间
    unsigned short  year;               //年
    unsigned char   month;              //月
    unsigned char   day;                //日
    unsigned char   hour;               //时
    unsigned char   minute;             //分
    unsigned char   second;             //秒
}BND_GPS_DATA_s;

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
	CGroup *pLockGroup;
	CGroup LockGroupTemp;
	bool isLockGroupStatus;
	int lockGroupOpt;
	bool isPocMemberListBuf;
	bool isPocGroupListAll;
	Msg_GROUP_MEMBER_s speaker;
	CGroup speaker_group;

	unsigned int mic_ctl;
	int check_listen_count;
	osiTimer_t * delay_close_listen_timer;
	osiTimer_t * get_member_list_timer;
	osiTimer_t * get_group_list_timer;
	osiTimer_t * start_speak_voice_timer;
	osiTimer_t * check_listen_timer;
	bool delay_close_listen_timer_running;
	bool start_speak_voice_timer_running;
	bool   listen_status;
	bool   speak_status;
	bool   record_fist;
	int runcount;
	osiTimer_t * try_login_timer;
	osiTimer_t * auto_login_timer;
	bool   is_makeout_call;
	bool   is_release_call;

	//timer
	osiTimer_t * check_ack_timeout_timer;
	osiTimer_t * monitor_pptkey_timer;
	osiTimer_t * monitor_recorder_timer;
	osiTimer_t * Bnd_GroupUpdate_timer;
	osiTimer_t * Bnd_MemberUpdate_timer;
	osiTimer_t * play_tone_timer;
	osiTimer_t * delay_stop_play_timer;
	osiTimer_t * delay_stop_tone_timer;

	//cb
	lv_poc_get_group_list_cb_t pocGetGroupListCb;
	lv_poc_get_member_list_cb_t pocGetMemberListCb;
	poc_set_current_group_cb pocSetCurrentGroupCb;
	poc_get_member_status_cb pocGetMemberStatusCb;
	poc_set_member_call_status_cb pocMemberCallCb;
	void (*pocLockGroupCb)(lv_poc_group_oprator_type opt);
	void (*pocDeleteGroupcb)(int result_type);

	//status
	bool invaildlogininf;
	bool PoweronUpdateIdleInfo;
	bool PoweronCurGInfo;
	bool haveingroup;
	bool is_member_call;
	bool is_have_join_group;
	bool onepoweron;

	//num
	int delay_play_voice;
	unsigned int query_group;
	uint8_t current_group;
	int member_call_dir;  //0 tx,  1 rx
	int call_type;
	int signalcall_gid;
	uint16_t loginstatus_t;
	lv_poc_tmpgrp_t cit_status;

	//info
	bnd_group_t BndCurrentGroupInfo;
	bnd_member_t BndUserInfo;
	Msg_GROUP_MEMBER_s self_info;
	Msg_GROUP_MEMBER_s member_call_obj;

	//Mutex
	osiMutex_t *lock;
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
	iLen = snprintf((char*)&cBuf[iLen], sizeof(cBuf) - iLen - 1, "BND: ");
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][mutex](%d):MutexCreate", __LINE__);
#endif
	return (AIR_OSI_Mutex_t*)osiMutexCreate();
}

bool AIR_OSI_MutexDelete(AIR_OSI_Mutex_t* mutex)//删除锁
{
	if(mutex == NULL) return false;
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][mutex](%d):MutexDelete", __LINE__);
#endif
	osiMutexDelete((osiMutex_t *)mutex);
	return true;
}

bool AIR_OSI_MutexTryLock(AIR_OSI_Mutex_t* mutex, unsigned int ms_timeout)//上锁
{
	if(mutex == NULL) return false;
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][mutex](%d):MutexTryLock", __LINE__);
#endif
	return osiMutexTryLock((osiMutex_t *)mutex, ms_timeout);
}

bool AIR_OSI_MutexUnlock(AIR_OSI_Mutex_t* mutex)//释放锁
{
	if(mutex == NULL) return false;
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][mutex](%d):MutexUnlock", __LINE__);
#endif
	osiMutexUnlock((osiMutex_t *)mutex);
	return true;
}

AIR_OSI_Semaphore_t* AIR_OSI_SemaphoreCreate(int a, int b)//创建信号量
{
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreCreate", __LINE__);
#endif
	return (AIR_OSI_Semaphore_t*)osiSemaphoreCreate(a, b);
}

bool AIR_SemaphoreDelete(AIR_OSI_Semaphore_t* cond)//删除信号量
{
	if(cond == NULL) return false;
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreDelete", __LINE__);
#endif
	osiSemaphoreDelete((osiSemaphore_t *)cond);
	return true;
}

bool AIR_OSI_SemaphoreRelease(AIR_OSI_Semaphore_t* cond)//释放信号量
{
	if(cond == NULL) return false;
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreRelease", __LINE__);
#endif
	osiSemaphoreRelease((osiSemaphore_t *)cond);
	return true;
}

bool AIR_OSI_SemaphoreAcquire(AIR_OSI_Semaphore_t* cond)//等待信号量
{
	if(cond == NULL) return false;
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreAcquire", __LINE__);
#endif
	osiSemaphoreAcquire((osiSemaphore_t *)cond);
	return true;
}

bool AIR_OSI_SemaphoreTryAcquire(AIR_OSI_Semaphore_t* cond, unsigned int ms_timeout)//等待信号量到超时
{
	if(cond == NULL) return false;
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][semaphore](%d):SemaphoreTryAcquire", __LINE__);
#endif
	return osiSemaphoreTryAcquire((osiSemaphore_t *)cond, ms_timeout);
}

int AIR_FS_Open(char* path, int flags, int a)//打开文件
{
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][fs](%d):open fs", __LINE__);
#endif
	return vfs_open(path, flags, a);
}

bool AIR_FS_Close(int file)//关闭文件
{
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][fs](%d):close fs", __LINE__);
#endif
	vfs_close(file);
	return true;
}

int AIR_FS_Read(int file, char* data, int size)//读文件
{
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][fs](%d):read fs", __LINE__);
#endif
	return vfs_read(file, (void *)data, size);
}

int AIR_FS_Write(int file, char* data, int size)//写文件
{
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][fs](%d):write fs", __LINE__);
#endif
	return vfs_write(file, (const void *)data, size);
}

bool AIR_FS_Seek(int file, int flag, int type)//操作文件流指针
{
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][fs](%d):seek fs", __LINE__);
#endif
	vfs_lseek(file, flag, type);
	return true;
}

int AIR_SOCK_Error(void)//获取网络错误标识
{
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][socket](%d):get sock error", __LINE__);
#endif

	return errno;
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][system](%d):ThreadCurrent", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][system](%d):ThreadCreate", __LINE__);
#endif
	AIR_OSI_Thread_t Threadid = (AIR_OSI_Thread_t)osiThreadCreate(name, func, argument,
								 priority, stack_size,
								 event_count);
	return Threadid;
}

bool AIR_OSI_ThreadExit()//标识线程退出，释放资源
{
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][system](%d):ThreadExit", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][system](%d):Epoch Second", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][tts](%d):tts play", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][tts](%d):tts stop", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][tts](%d):tts status", __LINE__);
#endif
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
	OSI_LOGI(0, "[bnd][record](%d):rec bnd start record", __LINE__);
	if(pocBndAttr.start_speak_voice_timer != NULL)
	{
		if(pocBndAttr.start_speak_voice_timer_running)
		{
			osiTimerStop(pocBndAttr.start_speak_voice_timer);
			pocBndAttr.start_speak_voice_timer_running = false;
		}
		osiTimerStart(pocBndAttr.start_speak_voice_timer, 160);
		pocBndAttr.start_speak_voice_timer_running = true;
		pocBndAttr.speak_status = true;
	}
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
	OSI_LOGI(0, "[bnd][record](%d):rec bnd stop record", __LINE__);
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_STOP_RECORD_IND, NULL);
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
	OSI_LOGI(0, "[bnd][play](%d):rec bnd start play", __LINE__);
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_START_PLAY_IND, NULL);
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
	osiTimerStart(pocBndAttr.delay_stop_play_timer, 200);
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

	OSI_LOGI(0, "[bnd][play](%d):start write data to play, data(%s), lenth(%d)", __LINE__, *data, length);

	osiMutexTryLock(pocBndAttr.lock, 5);
	int len = pocAudioPlayerWriteData(pocBndAttr.player, (const uint8_t *)data, length);
	osiMutexUnlock(pocBndAttr.lock);

	m_BndUser.m_iRxCount = m_BndUser.m_iRxCount + 1;
	pocBndAttr.check_listen_count++;

	return len;
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
			OSI_LOGI(0, "[bnd][play](%d):bnd start speak tone", __LINE__);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_START_SPEAK, NULL);
			break;
		}

		case 1://stop ptt
		{
			OSI_LOGI(0, "[bnd][play](%d):bnd stop speak tone", __LINE__);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_SPEAK, NULL);
			break;
		}

		case 2://error
		{
			OSI_LOGI(0, "[bnd][play](%d):bnd error tone", __LINE__);
			break;
		}

		case 3://start play
		{
			OSI_LOGI(0, "[bnd][play](%d):bnd start listen tone", __LINE__);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_START_LISTEN, NULL);
			break;
		}

		case 4://stop play
		{
			osiTimerStart(pocBndAttr.delay_stop_tone_timer, 200);
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][net](%d):set new apn", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][net](%d):open net data", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][net](%d):net exist", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][signal](%d):get rssi", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][net](%d):close net data", __LINE__);
#endif
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
#if GUIBNDCOM_BND_LOG
	OSI_LOGI(0, "[bnd][tts](%d):play_open_pcm", __LINE__);
#endif
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
	OSI_LOGI(0, "[bnd][meid](%d):get meid", __LINE__);
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
	if(data == NULL)
	{
		return -1;
	}
	poc_get_device_imei_rep((int8_t *)data);
	OSI_LOGI(0, "[bnd][imei](%d):get imei", __LINE__);
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
	if(data == NULL)
	{
		return -1;
	}
	poc_get_device_imsi_rep((int8_t *)data);
	OSI_LOGI(0, "[bnd][imsi](%d):get imsi", __LINE__);
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
	if(data == NULL)
	{
		return -1;
	}
	poc_get_device_iccid_rep((int8_t *)data);
	OSI_LOGI(0, "[bnd][iccid](%d):get iccid", __LINE__);
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
    switch(state)
	{
        case BND_SPEAK_START:
        {
	        str = "BND_SPEAK_START";
			m_BndUser.m_iRxCount = 0;
			m_BndUser.m_iTxCount = 0;
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SPEAK_START_REP, NULL);
	        break;
        }

        case BND_SPEAK_STOP:
        {
	        str = "BND_SPEAK_STOP";
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_REP, NULL);
	        break;
        }

        case BND_LISTEN_START:
        {
	        str = "BND_LISTEN_START";
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LISTEN_SPEAKER_REP, (void *)name);
	        break;
        }

        case BND_LISTEN_STOP://deal in delay_close_listen_timer_cb
        {
	        str = "bnd listen stop, in delay close listen time cb";
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LISTEN_STOP_REP, NULL);
	        break;
        }

		default:
		{
			break;
		}
    }

	OSI_LOGXI(OSI_LOGPAR_SISI, 0, "[cbbnd][audio]state:(%s), uid:(%d), name:(%s), flag:(%d)", str, uid, name, flag);
}

void callback_BND_Callmember(int ret)
{
    if(ret==1)
	{
		OSI_LOGXI(OSI_LOGPAR_IS, 0, "[cbbnd][membercall](%d)state:(%s)", __LINE__, "call sucess");
    }
	else
	{
		OSI_LOGXI(OSI_LOGPAR_IS, 0, "[cbbnd][membercall](%d)state:(%s)", __LINE__, "call failed");
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP, NULL);
    }
}

void callback_BND_Join_Group(const char* groupname, bnd_gid_t gid)
{
	OSI_LOGXI(OSI_LOGPAR_SIIS, 0, "[cbbnd][joingroup](%s)(%d), gid:(%d), gname:(%s)", __FUNCTION__, __LINE__, gid, groupname);
	bnd_group_t BndMsgCurGInf = {0};

	if(broad_group_getbyid(gid, &BndMsgCurGInf) == 0)
	{
		switch(BndMsgCurGInf.type)
		{
			case GRP_COMMON://组呼
			{
				OSI_LOGXI(OSI_LOGPAR_I, 0, "[cbbnd][joingroup](%d)group call ack", __LINE__);
				if(pocBndAttr.is_member_call)//signal call
				{
					lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP, (void *)2);
				}
				//cur gro
				pocBndAttr.call_type = GRP_COMMON;
				pocBndAttr.is_have_join_group = true;
				pocBndAttr.BndCurrentGroupInfo.gid = gid;
				strncpy(pocBndAttr.BndCurrentGroupInfo.name, groupname, strlen(groupname));
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REP, (void *)gid);
				break;
			}

			case GRP_SINGLECALL://单呼
			{
				if(pocBndAttr.is_member_call)
				{
					//join singel call
					OSI_LOGXI(OSI_LOGPAR_I, 0, "[cbbnd][joingroup](%d)local single call ack", __LINE__);
					lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP, (void *)gid);
				}
				else
				{
					bnd_group_t BndSignalInf = {0};
					BndSignalInf.gid = gid;
					strcpy(BndSignalInf.name, groupname);
					lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_GET_MEMBER_CALL_IND, (void *)&BndSignalInf);
				}
				pocBndAttr.call_type = GRP_SINGLECALL;
				break;
			}

			default:
			{
				OSI_LOGXI(OSI_LOGPAR_SI, 0, "[cbbnd][joingroup](%s)(%d)error", __FUNCTION__, __LINE__);
				break;
			}
		}
	}
	else
	{
		OSI_LOGXI(OSI_LOGPAR_I, 0, "[cbbnd][joingroup](%d)error", __LINE__);
	}
}

void callback_BND_ListUpdate(int flag)
{
    const char* str = NULL;

	switch(flag)
	{
		case 1:
		{
			str = "you need get grouplist -- update";
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_GU_STATUS_REP, (void *)1);
			break;
		}

		case 2:
		{
			str = "you need get memberlist -- update";
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_GU_STATUS_REP, (void *)2);
			break;
		}

		default:
		{
			break;
		}
	}

    OSI_LOGXI(OSI_LOGPAR_SS, 0, "[cbbnd][listupdate](%s), flag(%s)", __FUNCTION__, str);
}

void callback_BND_MemberChange(int flag, bnd_gid_t gid, int num, bnd_uid_t* uids)
{
    if(flag == 1)//leave group
	{
        OSI_LOGXI(OSI_LOGPAR_SIII, 0, "[cbbnd][memberchange][leave group](%s)(%d):gid(%d), m_MemNum(%d)", __FUNCTION__, __LINE__, gid, num);
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP, (void *)gid);
	}
	else
	{
        OSI_LOGXI(OSI_LOGPAR_SII, 0, "[cbbnd][memberchange](%s)(%d):gid(%d)", __FUNCTION__, __LINE__, gid);
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
			OSI_LOGXI(OSI_LOGPAR_SIS, 0, "[cbbnd][login](%s)(%d), state(%s)", __FUNCTION__, __LINE__, str);
			pocBndAttr.invaildlogininf = false;
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LOGIN_REP, (void *)&online);
		}
    }
    else
    {
        str = "USER_OFFLINE";
		if(pocBndAttr.loginstatus_t != LVPOCBNDCOM_SIGNAL_LOGIN_EXIT)
		{
			OSI_LOGXI(OSI_LOGPAR_SIS, 0, "[cbbnd][login](%s)(%d), state(%s)", __FUNCTION__, __LINE__, str);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LOGIN_REP, (void *)&online);
		}
    }
}

void callback_BND_Error(const char* info)
{
	OSI_LOGXI(OSI_LOGPAR_SIS, 0, "[cbbnd][error](%s)(%d):(%s)", __FUNCTION__, __LINE__, info);
}

void callback_BND_Location(unsigned char on, int interval)
{
	OSI_LOGXI(OSI_LOGPAR_SIII, 0, "[cbbnd][location](%s)(%d):gps status(%d), interval(%d)", __FUNCTION__, __LINE__, on, interval);
	if(on == 1)//open gps
	{

	}
	else
	{

	}
}

static void LvGuiBndCom_delay_close_listen_timer_cb(void *ctx)
{
	pocBndAttr.delay_close_listen_timer_running = false;
    if(m_BndUser.m_status != USER_OFFLINE)
    {
	    m_BndUser.m_status = USER_ONLINE;
    }
    m_BndUser.m_iRxCount = 0;
    m_BndUser.m_iTxCount = 0;
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_STOP_PLAY_IND, NULL);
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LISTEN_STOP_REP, NULL);
	OSI_LOGI(0, "[listen](%d)stop player, destory windows", __LINE__);
}

static void LvGuiBndCom_check_listen_timer_cb(void *ctx)//20ms period
{
	if(pocBndAttr.check_listen_count < 1
		|| m_BndUser.m_status != USER_OPRATOR_LISTENNING)
	{
		if(m_BndUser.m_status != USER_OFFLINE)
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
		OSI_LOGI(0, "[listen](%d)check_listen_timer cb, start to stop player", __LINE__);
		return;
	}
	pocBndAttr.check_listen_count--;
}

static void LvGuiBndCom_start_play_tone_timer_cb(void *ctx)
{
	bool pttStatus = pocGetPttKeyState()|lv_poc_get_earppt_state();
	OSI_LOGI(0, "[pttStatus](%d)(%d)", __LINE__, pttStatus);
	if(pttStatus)
	{
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_STOP_PLAY_IND, NULL);
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_START_RECORD_IND, NULL);
	}
	pocBndAttr.start_speak_voice_timer_running = false;
}

static void LvGuiBndCom_start_speak_voice_timer_cb(void *ctx)
{
	poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Speak, 30, true);
	osiTimerStart(pocBndAttr.play_tone_timer, 200);//200ms
}

static void LvGuiBndCom_get_member_list_timer_cb(void *ctx)
{

}

static void LvGuiBndCom_get_group_list_timer_cb(void *ctx)
{

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
			m_BndUser.m_status = USER_OPRATOR_PPT_RELEASE;
//			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_IND, NULL);
		}
		makecallcnt = 0;
	}
}

static void LvGuiBndCom_recorder_timer_cb(void *ctx)
{
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SUSPEND_IND, NULL);
}

static
void LvGuiBndCom_Group_update_timer_cb(void *ctx)
{
	if(pocBndAttr.PoweronCurGInfo == false)//power on first or login success later
	{
		bnd_group_t Bnd_CurGInfo = {0};
		bnd_group_t BndCGroup[GUIBNDCOM_GROUPMAX] = {0};

		pocBndAttr.PoweronCurGInfo = true;

		if(broad_group_getbyid(0, &Bnd_CurGInfo) == 0)
		{
			unsigned int m_GNum = broad_get_grouplist(BndCGroup, GUIBNDCOM_GROUPMAX, 0, -1);

			if(m_GNum < 0
				|| m_GNum > GUIBNDCOM_GROUPMAX)
			{
				OSI_LOGI(0, "[groupindex](%d)error", __LINE__);
			}
			else
			{
				for(unsigned int  i = 0; i < m_GNum; i++)
				{
					if(0 == strcmp((char *)BndCGroup[i].name, Bnd_CurGInfo.name))
					{
						pocBndAttr.current_group = BndCGroup[i].index;
						OSI_LOGI(0, "[groupindex]cur group index(%d)", pocBndAttr.current_group);
						break;
					}
				}
			}
		}
	}

    OSI_LOGXI(OSI_LOGPAR_IIS, 0, "[group](%d)start request grouprefr, gid(%d), gname(%s)", __LINE__, pocBndAttr.BndCurrentGroupInfo.gid, pocBndAttr.BndCurrentGroupInfo.name);

	if(pocBndAttr.self_info.ucName != NULL//show
		&& pocBndAttr.BndCurrentGroupInfo.name != NULL)
	{
		lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, pocBndAttr.self_info.ucName, (char *)pocBndAttr.BndCurrentGroupInfo.name);
	}
}

static
void LvGuiBndCom_Member_update_timer_cb(void *ctx)
{
	if(pocBndAttr.PoweronUpdateIdleInfo == false)
	{
		pocBndAttr.PoweronUpdateIdleInfo = true;
		if(broad_member_getbyid(0, &pocBndAttr.BndUserInfo) == 0)
		{
			char BndMemberId[20] = {0};
			strcpy((char *)pocBndAttr.self_info.ucName, pocBndAttr.BndUserInfo.name);
			memset(BndMemberId, 0, 20);
			__itoa(pocBndAttr.BndUserInfo.uid, (char *)BndMemberId, 10);
			strcpy((char *)pocBndAttr.self_info.ucNum, (char *)BndMemberId);
			if(pocBndAttr.BndUserInfo.state == 2)//offline
			{
				pocBndAttr.self_info.ucStatus = 0;
			}
			else
			{
				pocBndAttr.self_info.ucStatus = 1;
			}
		}

		if(pocBndAttr.self_info.ucName != NULL//show
			&& pocBndAttr.BndCurrentGroupInfo.name != NULL)
		{
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, pocBndAttr.self_info.ucName, (char *)pocBndAttr.BndCurrentGroupInfo.name);
		}
	}
    OSI_LOGXI(OSI_LOGPAR_SSSS, 0, "[member](%s)start request memberrefr, SelfName(%s), SelfId(%s), CurGName(%s)", __FUNCTION__, pocBndAttr.self_info.ucName, pocBndAttr.self_info.ucNum, pocBndAttr.BndCurrentGroupInfo.name);
}

static void LvGuiBndCom_check_ack_timeout_timer_cb(void *ctx)
{
	if(pocBndAttr.pocGetMemberListCb != NULL
		&& lvPocGuiBndCom_get_status() != USER_OFFLINE)
	{
		pocBndAttr.pocGetMemberListCb(0, 0, NULL);
		pocBndAttr.pocGetMemberListCb = NULL;
	}
}

static void LvGuiBndCom_delay_stop_play_timer_cb(void *ctx)
{
	OSI_LOGI(0, "[cbbnd][play](%d):rec bnd stop play", __LINE__);
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_STOP_PLAY_IND, NULL);
}

static void LvGuiBndCom_delay_stop_tone_timer_cb(void *ctx)
{
	OSI_LOGI(0, "[bnd][play](%d):bnd stop listen tone", __LINE__);
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_LISTEN, NULL);
}

static void LvGuiBndCom_Open_PA_timer_cb(void *ctx)
{
	OSI_LOGI(0, "[pa][timecb](%d):open pa", __LINE__);
	poc_set_ext_pa_status(true);
}

static void prvPocGuiBndTaskHandleLogin(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_LOGIN_IND:
		{
			broad_login(callback_BND_Login_State);
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
			int UserState = *login_info;

			if (USER_OFFLINE == UserState
				&& m_BndUser.m_status != USER_OFFLINE)
			{
				if(pocBndAttr.onepoweron == false)
					//&& !lv_poc_watchdog_status())
				{
					pocBndAttr.delay_play_voice++;

					if(pocBndAttr.delay_play_voice >= 30 || pocBndAttr.delay_play_voice == 1)
					{
						pocBndAttr.delay_play_voice = 1;
						poc_play_voice_one_time(LVPOCAUDIO_Type_No_Login, 50, false);
					}
				}

				//lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_1500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
				m_BndUser.m_status = USER_OFFLINE;
				pocBndAttr.loginstatus_t = LVPOCBNDCOM_SIGNAL_LOGIN_EXIT;
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "登录失败");
				osiTimerStop(pocBndAttr.get_group_list_timer);
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SUSPEND_IND, NULL);
				pocBndAttr.isReady = true;
				pocBndAttr.PoweronUpdateIdleInfo = false;
				pocBndAttr.PoweronCurGInfo = false;
			}

			if(USER_ONLINE == UserState
				&& m_BndUser.m_status != USER_ONLINE)//登录成功
			{
				//lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
				pocBndAttr.loginstatus_t = LVPOCBNDCOM_SIGNAL_LOGIN_SUCCESS;

				if(pocBndAttr.onepoweron == false)
					//&& !lv_poc_watchdog_status())
				{
					poc_play_voice_one_time(LVPOCAUDIO_Type_Success_Login, 50, false);
				}
				pocBndAttr.onepoweron = true;
				osiTimerStop(pocBndAttr.auto_login_timer);

				pocBndAttr.isReady = true;
				m_BndUser.m_status = USER_ONLINE;
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "成功登录");
				osiTimerStart(pocBndAttr.monitor_recorder_timer, 20000);//20S
				osiTimerStartRelaxed(pocBndAttr.get_group_list_timer, 500, OSI_WAIT_FOREVER);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, NULL);
			}

			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_EXIT_IND:
		{
			broad_logout();
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
		case LVPOCGUIBNDCOM_SIGNAL_SPEAK_START_IND:
		{
			if(m_BndUser.m_status == USER_OFFLINE)
			{
				break;
			}
			m_BndUser.m_status = USER_OPRATOR_START_SPEAK;
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LAUNCH_NOTE_MSG, (const uint8_t *)"正在申请", (const uint8_t *)"");
			osiTimerStop(pocBndAttr.monitor_recorder_timer);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_RESUME_IND, NULL);
			broad_speak(1);
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[speak](%s)(%d):start apply speak", __FUNCTION__, __LINE__);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SPEAK_START_REP:
		{
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "开始对讲", NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始对讲", NULL);
			/*开始闪烁*/
			//lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
			if(m_BndUser.m_status == USER_OPRATOR_START_SPEAK)
			{
				char speak_name[100] = "";
				strcpy((char *)speak_name, (const char *)"主讲:");
				strcat((char *)speak_name, (const char *)pocBndAttr.self_info.ucName);
				if(!pocBndAttr.is_member_call)
				{
					char group_name[100] = "";
					strcpy((char *)group_name, (const char *)pocBndAttr.BndCurrentGroupInfo.name);
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, speak_name, group_name);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)speak_name, (const uint8_t *)group_name);
				}
				else
				{
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, speak_name, "");
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)speak_name, (const uint8_t *)"");
				}
				pocBndAttr.is_makeout_call = true;
				osiTimerStart(pocBndAttr.monitor_pptkey_timer, 50);
			}
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[speak](%s)(%d):rec start speak, display windows", __FUNCTION__, __LINE__);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_IND:
		{
			broad_speak(0);
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[speak](%s)(%d):apply stop speak", __FUNCTION__, __LINE__);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_REP:
		{
			if((m_BndUser.m_status == USER_OPRATOR_SPEAKING
				&& pocBndAttr.is_makeout_call == true)
				|| m_BndUser.m_status == USER_OPRATOR_PPT_RELEASE)
			{
				/*恢复run闪烁*/
				//lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
				//lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_RUN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, "停止对讲", NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"停止对讲", NULL);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				pocBndAttr.is_makeout_call = false;
				osiTimerStop(pocBndAttr.monitor_pptkey_timer);
				pocBndAttr.speak_status = false;

				//monitor recorder thread
				osiTimerStart(pocBndAttr.monitor_recorder_timer, 10000);//10S
				OSI_LOGXI(OSI_LOGPAR_SI, 0, "[speak](%s)(%d):rec stop speak, destory windows", __FUNCTION__, __LINE__);
			}
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
		case LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_QUERY_IND:
		{
			if(pocBndAttr.pocGetGroupListCb == NULL)
			{
				break;
			}

			if(lvPocGuiBndCom_get_status() == USER_OFFLINE)
			{
				pocBndAttr.pocGetGroupListCb(0, 0, NULL);
				pocBndAttr.pocGetGroupListCb = NULL;
				break;
			}

			do
			{
				char BndGroupId[20] = {0};
				bnd_group_t BndCGroup[GUIBNDCOM_GROUPMAX] = {0};

				m_BndUser.m_Group.m_Group_Num = 0;
				m_BndUser.m_Group.m_Group_Num = broad_get_grouplist(BndCGroup, GUIBNDCOM_GROUPMAX, 0, -1);

				if(m_BndUser.m_Group.m_Group_Num < 0 || m_BndUser.m_Group.m_Group_Num > GUIBNDCOM_GROUPMAX)
				{
					pocBndAttr.pocGetGroupListCb(0, 0, NULL);
					pocBndAttr.pocGetGroupListCb = NULL;
					break;
				}

				for(unsigned int  i = 0; i < m_BndUser.m_Group.m_Group_Num; i++)
				{
					memset(&BndGroupId, 0, 20);
					strcpy((char *)m_BndUser.m_Group.m_Group[i].m_ucGName, (char *)BndCGroup[i].name);
				   	__itoa(BndCGroup[i].gid, (char *)BndGroupId, 10);
					strcpy((char *)m_BndUser.m_Group.m_Group[i].m_ucGNum, (char *)BndGroupId);
					m_BndUser.m_Group.m_Group[i].index = BndCGroup[i].index;

					OSI_LOGXI(OSI_LOGPAR_ISSI, 0, "[group](%d), GName(%s), Gid(%s), index(%d)", __LINE__, m_BndUser.m_Group.m_Group[i].m_ucGName, m_BndUser.m_Group.m_Group[i].m_ucGNum, m_BndUser.m_Group.m_Group[i].index);
				}

				pocBndAttr.pocGetGroupListCb(1, m_BndUser.m_Group.m_Group_Num, m_BndUser.m_Group.m_Group);
				pocBndAttr.pocGetGroupListCb = NULL;
			}while(0);

			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_QUERY_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocBndAttr.pocGetGroupListCb = (lv_poc_get_group_list_cb_t)ctx;
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
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

static void prvPocGuiBndTaskHandleMemberList(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
		{
			if(pocBndAttr.pocGetMemberListCb == NULL)
			{
				break;
			}

			if(lvPocGuiBndCom_get_status() == USER_OFFLINE)
			{
				pocBndAttr.pocGetMemberListCb(0, 0, NULL);
				pocBndAttr.pocGetMemberListCb = NULL;
				break;
			}

			bnd_member_t BndGData_s[GUIBNDCOM_MEMBERMAX] = {0};
			Msg_GData_s BndMsgGMember = {0};
			char BndMemberId[20] = {0};

			if(ctx == 0)//memberlist
			{
				do
				{
					int MemberNum = broad_get_membercount(pocBndAttr.BndCurrentGroupInfo.gid);
					BndMsgGMember.dwNum = broad_get_memberlist(pocBndAttr.BndCurrentGroupInfo.gid, BndGData_s, GUIBNDCOM_MEMBERMAX, 0, MemberNum);

					if(BndMsgGMember.dwNum < 0)
					{
						pocBndAttr.pocGetMemberListCb(0, 0, NULL);
						pocBndAttr.pocGetMemberListCb = NULL;
						break;
					}
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取成员列表...", NULL);
					for(unsigned long  i = 0; i < BndMsgGMember.dwNum; i++)
					{
						memset(BndMemberId, 0, 20);
						strcpy((char *)BndMsgGMember.member[i].ucName, (char *)BndGData_s[i].name);
					   	__itoa(BndGData_s[i].uid, (char *)BndMemberId, 10);
						strcpy((char *)BndMsgGMember.member[i].ucNum, (char *)BndMemberId);
						BndMsgGMember.member[i].ucPrio = BndGData_s[i].prior;
						if(BndGData_s[i].state == 2)//offline
						{
							BndMsgGMember.member[i].ucStatus = 0;
						}
						else
						{
							BndMsgGMember.member[i].ucStatus = 1;
						}

						OSI_LOGXI(OSI_LOGPAR_ISSI, 0, "[member](%d), Uname(%s), Uid(%s), Status(%d)", __LINE__, BndMsgGMember.member[i].ucName, BndMsgGMember.member[i].ucNum, BndMsgGMember.member[i].ucStatus);
					}
					pocBndAttr.pocGetMemberListCb(1, BndMsgGMember.dwNum, &BndMsgGMember);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				}while(0);

				OSI_LOGXI(OSI_LOGPAR_SIII, 0, "[member](%s)(%d), m_gid(%d), m_Member_Num(%d)", __FUNCTION__, __LINE__, pocBndAttr.BndCurrentGroupInfo.gid, BndMsgGMember.dwNum);
				pocBndAttr.pocGetMemberListCb = NULL;
			}
			else//group x member
			{
				CGroup * group_info = (CGroup *)ctx;
				for(unsigned int i = 0; i < m_BndUser.m_Group.m_Group_Num; i++)
				{
					if(0 == strcmp((const char *)group_info->m_ucGNum, (const char *)m_BndUser.m_Group.m_Group[i].m_ucGNum))
					{
						pocBndAttr.query_group = i;
						break;
					}
				}

				if(m_BndUser.m_Group.m_Group_Num < 1
					||pocBndAttr.query_group >=  m_BndUser.m_Group.m_Group_Num)
				{
					pocBndAttr.pocGetMemberListCb(0, 0, NULL);
					pocBndAttr.pocGetMemberListCb = NULL;
					pocBndAttr.query_group = 0;
					break;
				}
				//query
				int gid = atoi((char *)group_info->m_ucGNum);
				OSI_LOGXI(OSI_LOGPAR_SIIS, 0, "[groupxmember](%s)(%d), m_gid(%d), cur_gid(%s)", __FUNCTION__, __LINE__, gid, group_info->m_ucGNum);

				if(pocBndAttr.haveingroup)//in group
				{
					lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REP, (void *)gid);
					OSI_LOGXI(OSI_LOGPAR_I, 0, "[groupxmember](%d)group in", __LINE__);
				}
				else//other group
				{
					OSI_LOGXI(OSI_LOGPAR_I, 0, "[groupxmember](%d)group out", __LINE__);
					if(broad_joingroup(gid) != 0)
					{
						pocBndAttr.pocGetMemberListCb(0, 0, NULL);
						pocBndAttr.pocGetMemberListCb = NULL;
					}
					osiTimerStart(pocBndAttr.check_ack_timeout_timer, 3000);
				}
			}
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
		{
			if(pocBndAttr.pocGetMemberListCb == NULL)
			{
				if(pocBndAttr.is_have_join_group)
				{
					pocBndAttr.is_have_join_group = false;
					poc_play_voice_one_time(LVPOCAUDIO_Type_Join_Group, 50, false);
				}
				break;
			}

			if(ctx == 0)
			{
				break;
			}

			int gid = (int)ctx;
			bnd_member_t BndGData_s[GUIBNDCOM_MEMBERMAX] = {0};
			Msg_GData_s BndMsgGMember = {0};
			char BndMemberId[20] = {0};

			do
			{
				int MemberNum = broad_get_membercount(gid);
				BndMsgGMember.dwNum = broad_get_memberlist(gid, BndGData_s, GUIBNDCOM_MEMBERMAX, 0, MemberNum);
				OSI_LOGXI(OSI_LOGPAR_SIII, 0, "[groupxmember](%s)(%d), m_gid(%d), m_Member_Num(%d)", __FUNCTION__, __LINE__, gid, BndMsgGMember.dwNum);

				if(BndMsgGMember.dwNum < 0 || BndMsgGMember.dwNum > GUIBNDCOM_MEMBERMAX)
				{
					pocBndAttr.pocGetMemberListCb(0, 0, NULL);
					pocBndAttr.pocGetMemberListCb = NULL;
					break;
				}

				for(unsigned long  i = 0; i < BndMsgGMember.dwNum; i++)
				{
					memset(BndMemberId, 0, 20);
					strcpy((char *)BndMsgGMember.member[i].ucName, (char *)BndGData_s[i].name);
					__itoa(BndGData_s[i].uid, (char *)BndMemberId, 10);
					strcpy((char *)BndMsgGMember.member[i].ucNum, (char *)BndMemberId);
					BndMsgGMember.member[i].ucPrio = BndGData_s[i].prior;
					if(BndGData_s[i].state == 2)//offline
					{
						BndMsgGMember.member[i].ucStatus = 0;
					}
					else
					{
						BndMsgGMember.member[i].ucStatus = 1;
					}
					OSI_LOGXI(OSI_LOGPAR_ISSI, 0, "[groupxmember](%d), Uname(%s), Uid(%s), Status(%d)", __LINE__, BndMsgGMember.member[i].ucName, BndMsgGMember.member[i].ucNum, BndMsgGMember.member[i].ucStatus);
				}
				pocBndAttr.pocGetMemberListCb(1, BndMsgGMember.dwNum, &BndMsgGMember);
			}while(0);

			if(!pocBndAttr.haveingroup)
			{
				pocBndAttr.haveingroup = true;
				poc_play_voice_one_time(LVPOCAUDIO_Type_Join_Group, 50, false);
			}

			pocBndAttr.pocGetMemberListCb = NULL;
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);

			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocBndAttr.pocGetMemberListCb = (lv_poc_get_member_list_cb_t)ctx;
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
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
		case LVPOCGUIBNDCOM_SIGNAL_SET_CURRENT_GROUP_IND:
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

			CGroup * group_info = (CGroup *)ctx;
			lv_task_t *oncetask = lv_task_create(lv_poc_activity_func_cb_set.member_list.group_member_act, 50, LV_TASK_PRIO_HIGHEST, (void *)group_info->m_ucGName);
			lv_task_once(oncetask);

			unsigned int index = 0;
			for(index = 0; index < m_BndUser.m_Group.m_Group_Num; index++)
			{
				if(0 == strcmp((const char *)group_info->m_ucGNum, (const char *)m_BndUser.m_Group.m_Group[index].m_ucGNum)) break;
			}

			if(m_BndUser.m_Group.m_Group_Num < 1
				|| index >=  m_BndUser.m_Group.m_Group_Num)
			{
				pocBndAttr.pocSetCurrentGroupCb(0);
			}
			else if(index == pocBndAttr.current_group)//已在群组
			{
				pocBndAttr.haveingroup = true;
				pocBndAttr.pocSetCurrentGroupCb(2);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocBndAttr.self_info.ucName, m_BndUser.m_Group.m_Group[pocBndAttr.current_group].m_ucGName);
				OSI_LOGXI(OSI_LOGPAR_I, 0, "[groupxmember](%d)current group in", __LINE__);
			}
			else//切组成功
			{
				pocBndAttr.haveingroup = false;
				pocBndAttr.current_group = index;
				pocBndAttr.pocSetCurrentGroupCb(1);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocBndAttr.self_info.ucName, m_BndUser.m_Group.m_Group[pocBndAttr.current_group].m_ucGName);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取群组成员...", NULL);
				OSI_LOGXI(OSI_LOGPAR_I, 0, "[groupxmember](%d)current group out", __LINE__);
			}

			pocBndAttr.pocSetCurrentGroupCb = NULL;
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocBndAttr.pocSetCurrentGroupCb = (poc_set_current_group_cb)ctx;
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
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
		case LVPOCGUIBNDCOM_SIGNAL_MEMBER_INFO_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			Msg_GROUP_MEMBER_s *MemberStatus = (Msg_GROUP_MEMBER_s *)ctx;
			if(MemberStatus == NULL)
			{
				break;
			}

			int uid = atoi((char *)MemberStatus->ucNum);
			bnd_member_t BndMemberCallUserInfo = {0};

			if(broad_member_getbyid(uid, &BndMemberCallUserInfo) == 0)
			{
				if(USER_ONLINE == BndMemberCallUserInfo.state)//online in group
				{
					OSI_LOGI(0, "[memberstatus]member info online");
					pocBndAttr.pocGetMemberStatusCb(1);
				}
				else//offline
				{
					OSI_LOGI(0, "[memberstatus]member info offline");
					pocBndAttr.pocGetMemberStatusCb(0);
				}
				pocBndAttr.pocGetMemberStatusCb = NULL;
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
		case LVPOCGUIBNDCOM_SIGNAL_MEMBER_STATUS_REP:
		{
			if(pocBndAttr.pocGetMemberStatusCb == NULL)
			{
				break;
			}

			pocBndAttr.pocGetMemberStatusCb(ctx > 0);
			pocBndAttr.pocGetMemberStatusCb = NULL;
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
		{
			if(ctx == 0)
			{
				break;
			}
			pocBndAttr.pocGetMemberStatusCb = (poc_get_member_status_cb)ctx;
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_MEMBER_STATUS_CB_REP:
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
		case LVPOCGUIBNDCOM_SIGNAL_STOP_PLAY_IND:
		{
			if(pocBndAttr.player == 0)
			{
				break;
			}

			osiMutexTryLock(pocBndAttr.lock, 50);
			pocAudioPlayerStop(pocBndAttr.player);
			audevStopPlay();
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_START_PLAY_IND:
		{
			if(m_BndUser.m_status == USER_OFFLINE || pocBndAttr.player == 0)
			{
				m_BndUser.m_iRxCount = 0;
				m_BndUser.m_iTxCount = 0;
				break;
			}
			m_BndUser.m_status = USER_OPRATOR_LISTENNING;
			pocAudioPlayerStart(pocBndAttr.player);
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
		case LVPOCGUIBNDCOM_SIGNAL_STOP_RECORD_IND:
		{
			if(pocBndAttr.recorder == 0)
			{
				break;
			}
			OSI_LOGI(0, "[record](%d):stop record", __LINE__);
			pocAudioRecorderStop(pocBndAttr.recorder);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_START_RECORD_IND:
		{
			if(pocBndAttr.recorder == 0)
			{
				break;
			}
			OSI_LOGI(0, "[record](%d):start record", __LINE__);
			m_BndUser.m_status = USER_OPRATOR_SPEAKING;
			pocAudioRecorderStart(pocBndAttr.recorder);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SEND_RECORD_DATA_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			unsigned char *pRecordData = (unsigned char *)ctx;
			lib_oem_record_cb(pRecordData, 320);
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
		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
		{
		    if(ctx == 0)
		    {
			    break;
		    }

			if(pocBndAttr.listen_status == true)
			{
				//speak cannot membercall
				return;
			}

		    lv_poc_member_call_config_t *member_call_config = (lv_poc_member_call_config_t *)ctx;
		    Msg_GROUP_MEMBER_s *member_call_obj = (Msg_GROUP_MEMBER_s *)member_call_config->members;

		    if(member_call_config->func == NULL)
		    {
				OSI_LOGI(0, "[singlecall](%d)cb func null", __LINE__);
			    break;
		    }

		    if(member_call_config->enable && member_call_obj == NULL)
		    {
				OSI_LOGI(0, "[singlecall](%d)error param", __LINE__);
			    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"错误参数", NULL);
				member_call_config->func(2, 2);
				break;
		    }

			if(member_call_config->enable == true)//request single call
			{
				if(pocBndAttr.member_call_dir == 0)
				{
					pocBndAttr.pocMemberCallCb = member_call_config->func;
					pocBndAttr.is_member_call = true;

					//request member signal call
					unsigned int callnumber = 0;
					callnumber = atoi((char *)member_call_obj->ucNum);
					broad_calluser(callnumber, callback_BND_Callmember);

					OSI_LOGI(0, "[singlecall](%d)start request single call", __LINE__);
				}else
				{
					member_call_config->func(0, 0);
					pocBndAttr.is_member_call = true;

					OSI_LOGI(0, "[singlecall](%d)recive single call", __LINE__);
				}
			}
			else//exit single
			{
				bool is_member_call = pocBndAttr.is_member_call;
                pocBndAttr.is_member_call = false;
                pocBndAttr.member_call_dir = 0;
                if(!is_member_call)
                {
                    return;
                }

                member_call_config->func(1, 1);
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP, NULL);
				OSI_LOGI(0, "[singlecall](%d)local exit single call", __LINE__);
			}

			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			int SigalCallgid = (int)ctx;
			pocBndAttr.signalcall_gid = SigalCallgid;

			 //对方同意了单呼通话
		    if(pocBndAttr.pocMemberCallCb != NULL)
		    {
			    pocBndAttr.pocMemberCallCb(0, 0);
			    pocBndAttr.pocMemberCallCb = NULL;
		    }
			OSI_LOGI(0, "[singlecall](%d)local single call success, enter's gid(%d)", __LINE__, pocBndAttr.signalcall_gid);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
		{
			if(ctx == 0
				|| pocBndAttr.is_member_call == false)
			{
				break;
			}

			int SigalCallgid = (int)ctx;

			if(pocBndAttr.signalcall_gid == SigalCallgid
				|| SigalCallgid == 2)
			{
				//对方或者服务器释放或者拒绝了单呼通话
				pocBndAttr.is_member_call = false;
				pocBndAttr.member_call_dir = 0;
				pocBndAttr.signalcall_gid = 0;

				poc_play_voice_one_time(LVPOCAUDIO_Type_Exit_Member_Call, 30, true);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"退出单呼", NULL);
				lv_poc_activity_func_cb_set.member_call_close();
				OSI_LOGI(0, "[singlecall](%d)other exit single call", __LINE__);
			}
			else
			{
				OSI_LOGI(0, "[singlecall](%d)error, leave's gid(%d)", __LINE__, SigalCallgid);
			}
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP:
		{
			int gid = pocBndAttr.BndCurrentGroupInfo.gid;
			if(broad_joingroup(gid) != 0)
			{
				OSI_LOGXI(OSI_LOGPAR_II, 0, "[exitsinglecall](%d)join group error, gid(%d)", __LINE__, gid);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"加入群组失败", NULL);
			}
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_GET_MEMBER_CALL_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			bnd_group_t *SignalCallInf = (bnd_group_t *)ctx;

			static Msg_GROUP_MEMBER_s member_call_obj = {0};
			memset(&member_call_obj, 0, sizeof(Msg_GROUP_MEMBER_s));
			strcpy((char *)member_call_obj.ucName, (const char *)SignalCallInf->name);//临时呼叫的群组名就是用户名
			member_call_obj.ucStatus = USER_ONLINE;
			pocBndAttr.signalcall_gid = SignalCallInf->gid;
			pocBndAttr.member_call_dir = 1;
			lv_poc_activity_func_cb_set.member_call_open((void *)&member_call_obj);
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
		case LVPOCGUIBNDCOM_SIGNAL_LISTEN_START_REP:
		{
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_LISTEN_STOP_REP:
		{
			pocBndAttr.listen_status = false;

			//恢复run闪烁
			//lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
			//lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_RUN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_3000 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");

			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)"停止聆听", (const uint8_t *)"");
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[listen][cbbnd](%s)(%d):rec stop listen and destory windows", __FUNCTION__, __LINE__);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_LISTEN_SPEAKER_REP:
		{
			char *BndSpeakername = (char *)ctx;

			char speaker_name[100];
			memset(speaker_name, 0, sizeof(char) * 100);
			strcpy(speaker_name, (const char *)BndSpeakername);
			strcat(speaker_name, (const char *)"正在讲话");

			pocBndAttr.listen_status = true;
			m_BndUser.m_status = USER_OPRATOR_START_LISTEN;
			//lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500 ,LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER);

			//member call
			if(pocBndAttr.is_member_call)
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)"");
			}
			else
			{
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, speaker_name, pocBndAttr.BndCurrentGroupInfo.name);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)speaker_name, (const uint8_t *)pocBndAttr.BndCurrentGroupInfo.name);
			}
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[listen][cbbnd](%s)(%d):rec start listen and display windows", __FUNCTION__, __LINE__);
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
		case LVPOCGUIBNDCOM_SIGNAL_GU_STATUS_REP://组或者用户状态发生变化
		{
			if(ctx == 0)
			{
				break;
			}

			int GuOPT = (int)ctx;

			switch(GuOPT)
			{
				case 1://group update
				{
					if(osiTimerIsRunning(pocBndAttr.Bnd_GroupUpdate_timer))
					{
						osiTimerStop(pocBndAttr.Bnd_GroupUpdate_timer);
					}
					osiTimerStart(pocBndAttr.Bnd_GroupUpdate_timer,1000);
					break;
				}

				case 2://member update
				{
					if(osiTimerIsRunning(pocBndAttr.Bnd_MemberUpdate_timer))
					{
						osiTimerStop(pocBndAttr.Bnd_MemberUpdate_timer);
					}
					osiTimerStart(pocBndAttr.Bnd_MemberUpdate_timer,1000);
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
		case LVPOCGUIBNDCOM_SIGNAL_GET_SPEAK_CALL_CASE:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleUploadGpsInfo(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUIBNDCOM_SIGNAL_GPS_UPLOADING_IND:
      {
	  	 if(ctx == 0)
	  	 {
		 	break;
		 }

		 BND_GPS_DATA_s *GpsData = (BND_GPS_DATA_s *)ctx;

		 double lat = (double)GpsData->latitude;
		 double lon = (double)GpsData->longitude;
		 bnd_time_t Bnd_time = {0};

		 Bnd_time.year = GpsData->year;
		 Bnd_time.month = GpsData->month;
		 Bnd_time.day = GpsData->day;
		 Bnd_time.hour = GpsData->hour;
		 Bnd_time.minute = GpsData->minute;
		 Bnd_time.second = GpsData->second;
		 Bnd_time.millisecond = 0;

		 if(broad_send_gpsinfo(lon, lat, Bnd_time))
		 {
			 OSI_LOGI(0, "[gps](%d)upload error", __LINE__);
		 }
		 OSI_LOGI(0, "[gps](%d)upload success", __LINE__);
         break;
      }

      default:
      {
         break;
      }
   }
}

static void prvPocGuiBndTaskHandleRecordTaskOpt(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_SUSPEND_IND:
		{
			if((osiThread_t *)lv_poc_recorder_Thread() != NULL)
			{
				OSI_LOGI(0, "[recorder]start suspend");
				osiThreadSuspend((osiThread_t *)lv_poc_recorder_Thread());
			}
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_RESUME_IND:
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

static void prvPocGuiBndTaskHandlePlayTone(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_START_SPEAK:
		{
			//no deal
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_SPEAK:
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Speak, 30, true);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_START_LISTEN:
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Listen, 30, true);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_LISTEN:
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleSetVolum(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_VOICE_VOLUM_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			int *volum = (int *)ctx;
			broad_set_vol(BND_VOICE, *volum);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_TONE_VOLUM_IND:
		{
			if(ctx < 0
				|| ctx > 10)
			{
				break;
			}

			int *volum = (int *)ctx;
			broad_set_vol(BND_TONE, *volum);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_TTS_VOLUM_IND:
		{
			if(ctx < 0
				|| ctx > 10)
			{
				break;
			}

			int *volum = (int *)ctx;
			broad_set_vol(BND_TTS, *volum);
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
		case LVPOCGUIBNDCOM_SIGNAL_DELAY_IND:
		{
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SCREEN_ON_IND:
		{
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SCREEN_OFF_IND:
		{
			break;
		}

		default:
		{
			break;
		}
	}
}

static void pocGuiBndComTaskEntry(void *argument)
{
	osiEvent_t event = {0};

    pocBndAttr.isReady = true;
	m_BndUser.Reset();
	//bnd init
	broad_init();
	broad_log(0);
    char ver[64] = {0};
    broad_get_version(ver);
    OSI_LOGXI(OSI_LOGPAR_S, 0, "[bnd]long_ung version:%s\n",ver);
    broad_register_audio_cb(callback_BND_Audio);
    broad_register_join_group_cb(callback_BND_Join_Group);
    broad_register_listupdate_cb(callback_BND_ListUpdate);
    broad_register_member_change_cb(callback_BND_MemberChange);
	broad_register_error_cb(callback_BND_Error);
	broad_register_location_cb(callback_BND_Location);
    osiThreadSleep(1000);
    broad_login(callback_BND_Login_State);
	//config
	broad_set_is_destroy_temp_call(true);

    while(1)
    {
    	if(!osiEventWait(pocBndAttr.thread, &event))
		{
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

			case LVPOCGUIBNDCOM_SIGNAL_SPEAK_START_IND:
			case LVPOCGUIBNDCOM_SIGNAL_SPEAK_START_REP:
			case LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_IND:
			case LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_REP:
			{
				prvPocGuiBndTaskHandleSpeak(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_QUERY_IND:
			case LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_QUERY_REP:
			case LVPOCGUIBNDCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
			case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
			{
				prvPocGuiBndTaskHandleGroupList(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
			case LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
			case LVPOCGUIBNDCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
			case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
			{
				prvPocGuiBndTaskHandleMemberList(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_SET_CURRENT_GROUP_IND:
			case LVPOCGUIBNDCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND:
			case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_SET_CURRENT_GROUP_CB_IND:
			{
				prvPocGuiBndTaskHandleCurrentGroup(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_MEMBER_INFO_IND:
			case LVPOCGUIBNDCOM_SIGNAL_MEMBER_INFO_REP:
			{
				prvPocGuiBndTaskHandleMemberInfo(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_MEMBER_STATUS_REP:
			case LVPOCGUIBNDCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP:
			case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_MEMBER_STATUS_CB_REP:
			{
				prvPocGuiBndTaskHandleMemberStatus(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_STOP_PLAY_IND:
			case LVPOCGUIBNDCOM_SIGNAL_START_PLAY_IND:
			{
				prvPocGuiBndTaskHandlePlay(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_STOP_RECORD_IND:
			case LVPOCGUIBNDCOM_SIGNAL_START_RECORD_IND:
			case LVPOCGUIBNDCOM_SIGNAL_SEND_RECORD_DATA_IND:
			{
				prvPocGuiBndTaskHandleRecord(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_IND:
			case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
			case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
			case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_GET_MEMBER_CALL_IND:
			case LVPOCGUIBNDCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP:
			{
				prvPocGuiBndTaskHandleMemberCall(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_LISTEN_START_REP:
			case LVPOCGUIBNDCOM_SIGNAL_LISTEN_STOP_REP:
			case LVPOCGUIBNDCOM_SIGNAL_LISTEN_SPEAKER_REP:
			{
				prvPocGuiBndTaskHandleListen(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_GU_STATUS_REP:
			{
				prvPocGuiBndTaskHandleGuStatus(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_DELAY_IND:
			case LVPOCGUIBNDCOM_SIGNAL_SCREEN_ON_IND:
			case LVPOCGUIBNDCOM_SIGNAL_SCREEN_OFF_IND:
			{
				prvPocGuiBndTaskHandleOther(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_GET_SPEAK_CALL_CASE://获取对讲原因
			{
				prvPocGuiBndTaskHandleCallFailed(event.param1, event.param2, event.param3);
				break;
			}


			case LVPOCGUIBNDCOM_SIGNAL_SET_SHUTDOWN_POC:
			{
				bool status;

				status = lv_poc_get_charge_status();
				if(status == false)
				{
					prvPocGuiBndTaskHandleChargerOpt(event.param1, event.param2);
				}
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_GPS_UPLOADING_IND:
            {
            	prvPocGuiBndTaskHandleUploadGpsInfo(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_SUSPEND_IND:
			case LVPOCGUIBNDCOM_SIGNAL_RESUME_IND:
			{
				prvPocGuiBndTaskHandleRecordTaskOpt(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_START_SPEAK:
			case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_SPEAK:
			case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_START_LISTEN:
			case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_LISTEN:
			{
				prvPocGuiBndTaskHandlePlayTone(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_VOICE_VOLUM_IND:
			case LVPOCGUIBNDCOM_SIGNAL_TTS_VOLUM_IND:
			case LVPOCGUIBNDCOM_SIGNAL_TONE_VOLUM_IND:
			{
				prvPocGuiBndTaskHandleSetVolum(event.param1, event.param2);
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
	pocBndAttr.check_listen_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_check_listen_timer_cb, NULL);
	pocBndAttr.try_login_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_try_login_timer_cb, NULL);//注册尝试登录定时器
	pocBndAttr.auto_login_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_auto_login_timer_cb, NULL);//注册自动登录定时器
	pocBndAttr.monitor_pptkey_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_ppt_release_timer_cb, NULL);//检查ppt键定时器
	pocBndAttr.monitor_recorder_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_recorder_timer_cb, NULL);//检查是否有人在录音定时器
	pocBndAttr.Bnd_GroupUpdate_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_Group_update_timer_cb, NULL);//群组刷新定时器
	pocBndAttr.Bnd_MemberUpdate_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_Member_update_timer_cb, NULL);//成员刷新定时器
	pocBndAttr.check_ack_timeout_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_check_ack_timeout_timer_cb, NULL);
	pocBndAttr.play_tone_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_start_play_tone_timer_cb, NULL);
	pocBndAttr.delay_stop_play_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_delay_stop_play_timer_cb, NULL);
	pocBndAttr.delay_stop_tone_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_delay_stop_tone_timer_cb, NULL);
	pocBndAttr.lock = osiMutexCreate();
}

static void lvPocGuiIdtCom_send_data_callback(uint8_t * data, uint32_t length)
{
	OSI_LOGI(0, "[bnd][record](%d):start write data to record, data(%d), lenth(%d)", __LINE__, data, length);
    if (pocBndAttr.recorder == 0 || data == NULL || length < 1)
    {
	    return;
    }

    if(m_BndUser.m_status != USER_OPRATOR_SPEAKING)
    {
	    return;
    }

	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SEND_RECORD_DATA_IND, (void *)data);
	m_BndUser.m_iTxCount = m_BndUser.m_iTxCount + 1;
}

extern "C" void lvPocGuiBndCom_Init(void)
{
	memset(&pocBndAttr, 0, sizeof(PocGuiBndComAttr_t));
	pocBndAttr.is_release_call = true;
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

int lvPocGuiBndCom_get_status(void)
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
	if(m_BndUser.m_status == USER_OFFLINE)
	{
		return NULL;
	}
	return (void *)&pocBndAttr.self_info;
}

extern "C" void *lvPocGuiBndCom_get_current_group_info(void)
{
	if(m_BndUser.m_status == USER_OFFLINE)
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
//		if(0 == strcmp((char *)poc_config->selfbuild_group_num, (char *)m_BndUser.m_Group.m_Group[i].gid))
//		{
//			break;
//		}
	}

	if(i == m_BndUser.m_Group.m_Group_Num)
	{
		poc_config->is_exist_selfgroup = LVPOCGROUPIDTCOM_SIGNAL_SELF_NO;
	}

	return poc_config->is_exist_selfgroup;
}

lv_poc_tmpgrp_t lvPocGuiBndCom_cit_status(lv_poc_tmpgrp_t status)
{
    if(status == POC_TMPGRP_READ)
    {
        return pocBndAttr.cit_status;
    }
    pocBndAttr.cit_status = status;
    return POC_TMPGRP_START;
}

#endif

