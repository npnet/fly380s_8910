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
#include "poc_audio_pipe.h"
#include "poc.h"
#include "lv_apps/lv_poc_refr/lv_poc_refr.h"

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
#define POC_AUDIO_MODE_PIPE             1
#define POC_SUPPORT_BND_FUNCTION

enum{
	USER_OPRATOR_START_SPEAK = 4,
	USER_OPRATOR_SPEAKING  = 5,
	USER_OPRATOR_START_LISTEN = 6,
	USER_OPRATOR_LISTENNING  = 7,
};

static void LvGuiBndCom_delay_close_listen_timer_cb(void *ctx);
static void prvPocGuiBndTaskHandleErrorInfo(uint32_t id, uint32_t ctx);
static void lvPocGuiBndCom_send_data_callback(uint8_t * data, uint32_t length);
static int lvPocGuiBndCom_try_get_memberinfo(bnd_gid_t gid, bnd_member_t* dst);
static void lvPocGuiBndCom_Receive_ListUpdate(lv_task_t * task);

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

typedef struct _PocGuiBndPaAttr_t
{
	int  type;
	int  timems;
}PocGuiBndPaAttr_t;

typedef struct _PocGuiBndComAttr_t
{
public:
	osiThread_t *thread;
	bool        isReady;
	int 		pipe;
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

	unsigned int mic_ctl;
	osiTimer_t * delay_close_listen_timer;
	osiTimer_t * get_member_list_timer;
	osiTimer_t * get_group_list_timer;
	osiTimer_t * checkrecoverylisten_timer;

	bool delay_close_listen_timer_running;
	bool   listen_status;
	bool   speak_status;
	bool   record_fist;
	int runcount;
	int ping_time;
	osiTimer_t * try_login_timer;
	osiTimer_t * auto_login_timer;
	bool   is_makeout_call;
	bool   is_release_call;
	bool   set_ping_first;

	//timer
	osiTimer_t * check_ack_timeout_timer;
	osiTimer_t * monitor_recorder_timer;
	osiTimer_t * Bnd_GroupUpdate_timer;
	osiTimer_t * Bnd_MemberUpdate_timer;
	osiTimer_t * ping_timer;
	osiTimer_t * Bnd_workpa_timer;
	osiTimer_t * BrightScreen_timer;
	void *timer_user_data;

	//cb
	lv_poc_get_group_list_cb_t pocGetGroupListCb;
	lv_poc_get_member_list_cb_t pocGetMemberListCb;
	poc_set_current_group_cb pocSetCurrentGroupCb;
	poc_build_tempgrp_cb pocBuildTempGrpCb;
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
	bool is_enter_signal_multi_call;
	bool is_list_update;
	bool is_login_memberlist_first_update;
	bool is_ptt_state;

	//num
	int delay_play_voice;
	unsigned int query_group;
	uint8_t current_group;
	int member_call_dir;  //0 tx,  1 rx
	int call_type;
	int signalcall_gid;
	uint16_t loginstatus_t;
	int single_multi_call_type;
	int info_error_type;
	int  call_briscr_dir;
	int  call_curscr_state;
	int  sema_attr;

	//info
	bnd_group_t BndCurrentGroupInfo;
	bnd_group_t BndTmpGrpInf;
	bnd_member_t BndUserInfo;
	Msg_GROUP_MEMBER_s self_info;
	Msg_GROUP_MEMBER_s member_call_obj;
	PocGuiBndPaAttr_t pa_attr;
	Msg_GData_s BndMsgGMemberBuf;
	bnd_group_t *SignalCallInf;

	//Mutex
	osiMutex_t *lock;
} PocGuiBndComAttr_t;

CBndUser m_BndUser;
static PocGuiBndComAttr_t pocBndAttr = {0};
static lv_poc_tmpgrp_t cit_status = POC_TMPGRP_START;

#ifdef POC_SUPPORT_BND_FUNCTION
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

AIR_OSI_Mutex_t* AIR_OSI_MutexCreate(void)//创建锁
{
	return (AIR_OSI_Mutex_t*)osiMutexCreate();
}

bool AIR_OSI_MutexDelete(AIR_OSI_Mutex_t* mutex)//删除锁
{
	if(mutex == NULL) return false;
	osiMutexDelete((osiMutex_t *)mutex);
	return true;
}

bool AIR_OSI_MutexTryLock(AIR_OSI_Mutex_t* mutex, unsigned int ms_timeout)//上锁
{
	if(mutex == NULL) return false;
	return osiMutexTryLock((osiMutex_t *)mutex, ms_timeout);
}

bool AIR_OSI_MutexUnlock(AIR_OSI_Mutex_t* mutex)//释放锁
{
	if(mutex == NULL) return false;
	osiMutexUnlock((osiMutex_t *)mutex);
	return true;
}

AIR_OSI_Semaphore_t* AIR_OSI_SemaphoreCreate(int a, int b)//创建信号量
{
	return (AIR_OSI_Semaphore_t*)osiSemaphoreCreate(a, b);
}

bool AIR_SemaphoreDelete(AIR_OSI_Semaphore_t* cond)//删除信号量
{
	if(cond == NULL) return false;
	osiSemaphoreDelete((osiSemaphore_t *)cond);
	return true;
}

bool AIR_OSI_SemaphoreRelease(AIR_OSI_Semaphore_t* cond)//释放信号量
{
	if(cond == NULL) return false;
	osiSemaphoreRelease((osiSemaphore_t *)cond);
	return true;
}

bool AIR_OSI_SemaphoreAcquire(AIR_OSI_Semaphore_t* cond)//等待信号量
{
	if(cond == NULL) return false;
	osiSemaphoreAcquire((osiSemaphore_t *)cond);
	return true;
}

bool AIR_OSI_SemaphoreTryAcquire(AIR_OSI_Semaphore_t* cond, unsigned int ms_timeout)//等待信号量到超时
{
	if(cond == NULL) return false;
	return osiSemaphoreTryAcquire((osiSemaphore_t *)cond, ms_timeout);
}

int AIR_FS_Open(char* path, int flags, int a)//打开文件
{
	if(path == NULL)
	{
		OSI_PRINTFI("[bndfs](%s)(%d):null path", __func__, __LINE__);
		return -1;
	}

	int status = vfs_open(path, O_RDWR | O_CREAT, 0);
	OSI_PRINTFI("[bndfs](%s)(%d):path(0x%d), status(%d)", __func__, __LINE__, path, status);
	return status;
}

bool AIR_FS_Close(int file)//关闭文件
{
	int status = vfs_close(file);
	OSI_PRINTFI("[bndfs](%s)(%d):file(0x%d), status(%d)", __func__, __LINE__, file, status);
	return status;
}

int AIR_FS_Read(int file, char* data, int size)//读文件
{
	if(data == NULL)
	{
		OSI_PRINTFI("[bndfs](%s)(%d):null data", __func__, __LINE__);
		return -1;
	}

	int status = vfs_read(file, (void *)data, size);
	OSI_PRINTFI("[bndfs](%s)(%d):file(%d), data(0x%d), size(%d), status(%d)", __func__, __LINE__, file, data, size, status);
	return status;
}

int AIR_FS_Write(int file, char* data, int size)//写文件
{
	if(data == NULL)
	{
		OSI_PRINTFI("[bndfs](%s)(%d):null data", __func__, __LINE__);
		return -1;
	}

	int status = vfs_write(file, (const void *)data, size);
	OSI_PRINTFI("[bndfs](%s)(%d):file(%d), data(0x%d), size(%d), status(%d)", __func__, __LINE__, file, data, size, status);
	return status;
}

bool AIR_FS_Seek(int file, int flag, int type)//操作文件流指针
{
	int status = vfs_lseek(file, flag, type);
	OSI_PRINTFI("[bndfs](%s)(%d):status(%d)", __func__, __LINE__, status);
	return true;
}

int AIR_SOCK_Error(void)//获取网络错误标识
{
	return errno;
}

int AIR_SOCK_Gethostbyname(char* pHostname, AIR_SOCK_IP_ADDR_t* pAddr)//域名解析
{
	if(pHostname == NULL)
	{
		OSI_PRINTFI("[bnd][socket](%s)(%d):get host by name, pHostname NULL", __func__, __LINE__);
		return -1;
	}

	struct sockaddr_in server_addr;
	struct hostent* server_host= gethostbyname(pHostname);

	OSI_PRINTFI("[bnd][socket](%s)(%d):start to get host by name", __func__, __LINE__);//no close, Otherwise it will crash

	memcpy( (void *) &server_addr.sin_addr,
            (void *) server_host->h_addr,
                     server_host->h_length);
	pAddr->addr = server_addr.sin_addr.s_addr;

	OSI_PRINTFI("[bnd][socket](%s)(%d):get host by name, copy data complete", __func__, __LINE__);//no close, Otherwise it will crash

	return 0;
}

char* AIR_SOCK_IPAddr_ntoa(AIR_SOCK_IP_ADDR_t* pIpAddr)//网络地址转字符IP
{
	static char IpAddr[20] = {0};
	__itoa(pIpAddr->addr, (char *)&IpAddr, 10);
	return (char *)IpAddr;
}

uint32_t AIR_SOCK_inet_addr(char * pIp)//字符串IP转int型
{
	return atoi(pIp);
}

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
	AIR_OSI_Thread_t Threadid = (AIR_OSI_Thread_t)osiThreadCreate(name, func, argument,
								 priority, stack_size,
								 event_count);
	return Threadid;
}

bool AIR_OSI_ThreadExit()//标识线程退出，释放资源
{
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
	return (unsigned int)osiEpochSecond();
}

int lib_oem_send_uart(const char* data, int size)
{
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
	nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
	if(poc_config->current_tone_switch == 1)
	{
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SPEAK_TONE_START_IND, NULL);
	}
	else
	{
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SPEAK_TONE_STOP_IND, NULL);
	}
	OSI_PRINTFI("[bnd][record](%s)(%d):rec bnd start record", __func__, __LINE__);
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
	OSI_PRINTFI("[bnd][record](%s)(%d):rec bnd stop record", __func__, __LINE__);
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
	nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
	if(poc_config->current_tone_switch == 0)
	{
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_START_PLAY_IND, NULL);
	}
	OSI_PRINTFI("[bnd][play](%s)(%d):rec bnd start play", __func__, __LINE__);
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
	OSI_PRINTFI("[bnd][play](%s)(%d):rec bnd stop play, no deal", __func__, __LINE__);
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
	int len = 320;
	if(pocBndAttr.player == 0)
	{
		return 0;
	}

#if POC_AUDIO_MODE_PIPE
	pocAudioPipeWriteData((const uint8_t *)data, length);
#else
	len = pocAudioPlayerWriteData(pocBndAttr.player, (const uint8_t *)data, length);
#endif
	m_BndUser.m_iRxCount = m_BndUser.m_iRxCount + 1;

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
			OSI_PRINTFI("[bnd][tone](%s)(%d):bnd start speak tone, no deal", __func__, __LINE__);
			break;
		}

		case 1://stop ptt
		{
			OSI_PRINTFI("[bnd][tone](%s)(%d):bnd stop speak tone", __func__, __LINE__);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_SPEAK, NULL);
			break;
		}

		case 2://error
		{
			//reset
			if(lvPocGuiBndCom_get_listen_status()//抢话权，权限不够
				&& pocBndAttr.is_ptt_state)
			{
				pocBndAttr.is_ptt_state = false;
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无话语权", NULL);
				if(osiTimerIsRunning(pocBndAttr.checkrecoverylisten_timer))
				{
					osiTimerStop(pocBndAttr.checkrecoverylisten_timer);
				}
				osiTimerStart(pocBndAttr.checkrecoverylisten_timer, 1500);
				OSI_PRINTFI("[bnd][tone](%s)(%d):bnd error tone, rob words failed", __func__, __LINE__);
			}
			else
			{
				pocBndAttr.speak_status = false;
				pocBndAttr.listen_status = false;
				OSI_PRINTFI("[bnd][tone](%s)(%d):bnd error tone, timeout", __func__, __LINE__);
			}
			break;
		}

		case 3://start play
		{
			OSI_PRINTFI("[bnd][tone](%s)(%d):bnd start listen tone", __func__, __LINE__);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_START_LISTEN, NULL);
			break;
		}

		case 4://stop play
		{
			OSI_PRINTFI("[bnd][tone](%s)(%d):bnd stop listen tone, no deal", __func__, __LINE__);
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
	OSI_PRINTFI("[bnd][net](%s)(%d):set new apn", __func__, __LINE__);
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
	OSI_PRINTFI("[bnd][net](%s)(%d):open net data", __func__, __LINE__);
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
		//4G
	}
	else if (GprsStatus.nStatus == CFW_NW_STATUS_REGISTRATION_DENIED)
	{
		//NETWORK_TYPE_LIMITED
	}
	else
	{
		return 0;//NO_SERVICE
	}
	OSI_PRINTFI("[bnd][net](%s)(%d):net data normal", __func__, __LINE__);
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
	OSI_PRINTFI("[bnd][net](%s)(%d):net exist", __func__, __LINE__);
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
	OSI_PRINTFI("[bnd][signal](%s)(%d):get rssi", __func__, __LINE__);
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
	OSI_PRINTFI("[bnd][net](%s)(%d):close net data", __func__, __LINE__);
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

int lib_oem_play_open_pcm(void)
{
#if GUIBNDCOM_BND_LOG
	OSI_PRINTFI("[bnd][tts](%s)(%d):play_open_pcm", __func__, __LINE__);
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
	OSI_PRINTFI("[bnd][sim](%s)(%d):exist", __func__, __LINE__);
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
	OSI_PRINTFI("[bnd][meid](%s)(%d):get meid", __func__, __LINE__);
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
#endif

void callback_BND_Audio(AUDIO_STATE state, bnd_uid_t uid, const char* name, int flag)
{
    switch(state)
	{
        case BND_SPEAK_START:
        {
			OSI_PRINTFI("[bnd][speak](%s)(%d):BND_SPEAK_START", __func__, __LINE__);
			m_BndUser.m_iRxCount = 0;
			m_BndUser.m_iTxCount = 0;
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SPEAK_START_REP, NULL);
	        break;
        }

        case BND_SPEAK_STOP:
        {
			OSI_PRINTFI("[bnd][speak](%s)(%d):BND_SPEAK_STOP", __func__, __LINE__);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_REP, NULL);
	        break;
        }

        case BND_LISTEN_START:
        {
			OSI_PRINTFI("[bnd][listen](%s)(%d):BND_LISTEN_START", __func__, __LINE__);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LISTEN_SPEAKER_REP, (void *)name);
	        break;
        }

        case BND_LISTEN_STOP://deal in delay_close_listen_timer_cb
        {
			OSI_PRINTFI("[bnd][listen](%s)(%d):BND_LISTEN_STOP", __func__, __LINE__);
			if(pocBndAttr.delay_close_listen_timer_running)
			{
			   osiTimerStop(pocBndAttr.delay_close_listen_timer);
			   pocBndAttr.delay_close_listen_timer_running = false;
			}
			osiTimerStart(pocBndAttr.delay_close_listen_timer, 560);
			pocBndAttr.delay_close_listen_timer_running = true;
	        break;
        }

		default:
		{
			//OSI_LOGXI(OSI_LOGPAR_SISI, 0, "[cbbnd][audio]state:(%s), uid:(%d), name:(%s), flag:(%d)", str, uid, name, flag);
			break;
		}
    }
}

void callback_BND_SingalCallmember(int ret)
{
    if(ret==1)
	{
		OSI_LOGXI(OSI_LOGPAR_IS, 0, "[cbbnd][singalcall](%d)state:(%s)", __LINE__, "call sucess");
    }
	else
	{
		OSI_LOGXI(OSI_LOGPAR_IS, 0, "[cbbnd][singalcall](%d)state:(%s)", __LINE__, "call failed");
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_ERROR_REP, NULL);
    }
}

void callback_BND_MultiCallmember(int ret)
{
    if(ret==1)
	{
		OSI_LOGXI(OSI_LOGPAR_IS, 0, "[cbbnd][multicall](%d)state:(%s)", __LINE__, "call sucess");
    }
	else
	{
		OSI_LOGXI(OSI_LOGPAR_IS, 0, "[cbbnd][multicall](%d)state:(%s)", __LINE__, "call failed");
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_REP, (void *)ret);
    }
}

void callback_BND_Join_Group(const char* groupname, bnd_gid_t gid)
{
	OSI_PRINTFI("[cbbnd][joingroup](%s)(%d):gid:(%d), gname:(%s)", __func__, __LINE__, gid, groupname);
	bnd_group_t BndMsgCurGInf = {0};

	if(broad_group_getbyid(gid, &BndMsgCurGInf) == 0)
	{
		switch(BndMsgCurGInf.type)
		{
			case GRP_COMMON://组呼
			{
				if(pocBndAttr.is_member_call)//single call
				{
					OSI_PRINTFI("[cbbnd][singlecall](%s)(%d):exit single call ack, join group", __func__, __LINE__);
					lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP, (void *)2);
				}
				else
				{
					OSI_PRINTFI("[cbbnd][groupcall](%s)(%d):group call ack, join group", __func__, __LINE__);
				}
				//cur gro
				pocBndAttr.call_type = GRP_COMMON;
				pocBndAttr.is_have_join_group = true;
				pocBndAttr.BndCurrentGroupInfo.gid = gid;
				memset(pocBndAttr.BndCurrentGroupInfo.name, 0, sizeof(char)*BRD_NAME_LEN);
				strncpy(pocBndAttr.BndCurrentGroupInfo.name, groupname, strlen(groupname));
				//save curgrp id
				nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
				char curren_gid[32] = "";
				__utoa(gid,curren_gid,10);
				strcpy(poc_config->curren_group_id, (char *)curren_gid);
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REP, (void *)gid);
				break;
			}

			case GRP_SINGLECALL://单呼
			{
				if(pocBndAttr.is_member_call)
				{
					//join singel call
					OSI_PRINTFI("[cbbnd][singlecall](%s)(%d):local single call ack", __func__, __LINE__);
					lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP, (void *)gid);
				}
				else
				{
					OSI_PRINTFI("[cbbnd][singlecall](%s)(%d):rec multi call or single call", __func__, __LINE__);
					static bnd_group_t BndSignalInf = {0};
					memset(&BndSignalInf, 0, sizeof(bnd_group_t));

					BndSignalInf.gid = gid;
					strcpy(BndSignalInf.name, groupname);
					lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_SELECT_CALL_MODE, (void *)&BndSignalInf);
				}
				pocBndAttr.call_type = GRP_SINGLECALL;
				break;
			}

			default:
			{
				OSI_PRINTFI("[cbbnd][joingroup](%s)(%d):error", __func__, __LINE__);
				break;
			}
		}
	}
	else
	{
		OSI_PRINTFI("[cbbnd][joingroup](%s)(%d):error", __func__, __LINE__);
	}
	lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, pocBndAttr.self_info.ucName, (char *)pocBndAttr.BndCurrentGroupInfo.name);
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
			pocBndAttr.is_list_update = true;
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
		pocBndAttr.single_multi_call_type == USER_OPRATOR_SIGNAL_CALL ? lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP, (void *)gid) : 0;
	}
	else
	{
        OSI_LOGXI(OSI_LOGPAR_SII, 0, "[cbbnd][memberchange](%s)(%d):gid(%d), m_MemNum(%d)", __FUNCTION__, __LINE__, gid, num);
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
	OSI_PRINTFI("[groupindex](%s)(%d):(%s)", __func__, __LINE__, info);
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_GET_ERROR_CASE, (void *)info);
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
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_LISTEN, NULL);
	OSI_LOGI(0, "[listen](%d)stop player, destory windows", __LINE__);
}

static void LvGuiBndCom_get_member_list_timer_cb(void *ctx)
{
	static int run_Num = 0;
	static int last_info_error_type = 0;
	bnd_member_t BndGData_s[GUIBNDCOM_MEMBERMAX] = {0};
	int m_GNum = 0;

	if(last_info_error_type != pocBndAttr.info_error_type)
	{
		run_Num = 0;
	}
	last_info_error_type = pocBndAttr.info_error_type;

	switch(pocBndAttr.info_error_type)
	{
		case ERROR_TYPE_SINGLE_OR_MULTI_CALL:
		{
			m_GNum = lvPocGuiBndCom_try_get_memberinfo(pocBndAttr.SignalCallInf->gid, BndGData_s);
			m_GNum >= 0 ? lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_REGET_INFO_IND, (void *)m_GNum) : 0;
			break;
		}

		case ERROR_TYPE_MEMBER_REFRESH:
		{
			int gid = atoi((char *)m_BndUser.m_Group.m_Group[pocBndAttr.current_group].m_ucGNum);
			m_GNum = lvPocGuiBndCom_try_get_memberinfo(gid, BndGData_s);
			m_GNum >= 0 ? lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REFRESH_REP, NULL) : 0;
			break;
		}

		case ERROR_TYPE_MEMBER_LIST_GET:
		{
			int gid = (int)pocBndAttr.timer_user_data;
			m_GNum = lvPocGuiBndCom_try_get_memberinfo(gid, BndGData_s);
			m_GNum >= 0 ? lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REP, (void *)gid) : 0;
			break;
		}

		default:
		{
			OSI_PRINTFI("[cbbnd][memberinfo][cb](%s)(%d):error", __func__, __LINE__);
			osiTimerStop(pocBndAttr.get_member_list_timer);
			break;
		}
	}

	if(m_GNum >= 0)
	{
		OSI_PRINTFI("[cbbnd][memberinfo][cb](%s)(%d):reget success, start to refresh", __func__, __LINE__);
		pocBndAttr.info_error_type = ERROR_TYPE_START;
		run_Num = 0;
		osiTimerStop(pocBndAttr.get_member_list_timer);
		return;
	}
	else if(run_Num >= 5)
	{
		switch(pocBndAttr.info_error_type)
		{
			case ERROR_TYPE_MEMBER_LIST_GET:
			{
				if(pocBndAttr.pocGetMemberListCb)
				{
					pocBndAttr.pocGetMemberListCb(0, 0, NULL);
					pocBndAttr.pocGetMemberListCb = NULL;
				}
				break;
			}
		}
		osiTimerStop(pocBndAttr.get_member_list_timer);
		run_Num = 0;
		OSI_PRINTFI("[cbbnd][memberinfo][cb](%s)(%d):reget info(%d)", __func__, __LINE__, m_GNum);
	}
	run_Num++;
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
		nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

		if(broad_group_getbyid(0, &Bnd_CurGInfo) == 0)
		{
			unsigned int m_GNum = broad_get_grouplist(BndCGroup, GUIBNDCOM_GROUPMAX, 0, -1);
			char curren_gid[32] = "";
			__utoa(Bnd_CurGInfo.gid,curren_gid,10);

			if(m_GNum < 0
				|| m_GNum > GUIBNDCOM_GROUPMAX)
			{
				pocBndAttr.current_group = 0;
				strcpy(poc_config->curren_group_id, (char *)curren_gid);
				OSI_PRINTFI("[groupindex](%s)(%d):error", __func__, __LINE__);
			}
			else
			{
				unsigned int  i = 0;
				for(i = 0; i < m_GNum; i++)
				{
					if(0 == strcmp((char *)BndCGroup[i].name, Bnd_CurGInfo.name))
					{
						pocBndAttr.current_group = BndCGroup[i].index;
						pocBndAttr.BndCurrentGroupInfo.gid =BndCGroup[i].gid;
						memset(pocBndAttr.BndCurrentGroupInfo.name, 0, sizeof(char)*BRD_NAME_LEN);
						strcpy(pocBndAttr.BndCurrentGroupInfo.name, Bnd_CurGInfo.name);
						strcpy(poc_config->curren_group_id, curren_gid);
						OSI_PRINTFI("[groupindex](%s)(%d):cur group index(%d)", __func__, __LINE__, pocBndAttr.current_group);
						break;
					}
				}
				if(i == m_GNum)
				{
					pocBndAttr.current_group = 0;
					strcpy(poc_config->curren_group_id, curren_gid);
					OSI_PRINTFI("[groupindex](%s)(%d):no cur group", __func__, __LINE__);
				}
				pocBndAttr.haveingroup = true;
				pocBndAttr.is_login_memberlist_first_update = true;
			}
		}

		//update group buf
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_REFRESH_BUF_REP, NULL);
	}

	OSI_PRINTFI("[group][update](%s)(%d):start request grouprefr, gid(%d), gname(%s)", __func__, __LINE__, pocBndAttr.BndCurrentGroupInfo.gid, pocBndAttr.BndCurrentGroupInfo.name);
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_QUERY_REP, NULL);
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

		lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, pocBndAttr.self_info.ucName, (char *)pocBndAttr.BndCurrentGroupInfo.name);
		OSI_PRINTFI("[member][update](%s)(%d):start request memberrefr, SelfName(%s), SelfId(%s), CurGName(%s)", __func__, __LINE__, pocBndAttr.self_info.ucName, pocBndAttr.self_info.ucNum, pocBndAttr.BndCurrentGroupInfo.name);
	}

	OSI_PRINTFI("[member][update](%s)(%d):start request memberrefr", __func__, __LINE__);
	lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REFRESH_REP, NULL);
}

static void LvGuiBndCom_check_ack_timeout_timer_cb(void *ctx)
{
	if(pocBndAttr.pocGetMemberListCb != NULL
		&& lvPocGuiBndCom_get_status() != USER_OFFLINE)
	{
		pocBndAttr.pocGetMemberListCb(0, 0, NULL);
		pocBndAttr.pocGetMemberListCb = NULL;
	}
    else if(pocBndAttr.pocGetGroupListCb != NULL)
    {
        pocBndAttr.pocGetGroupListCb(0, 0, NULL);
        pocBndAttr.pocGetGroupListCb = NULL;
    }
	else if(pocBndAttr.pocBuildTempGrpCb != NULL)
    {
        pocBndAttr.pocBuildTempGrpCb(0);
        pocBndAttr.pocBuildTempGrpCb = NULL;
    }
	else
	{
		return;
	}

	pocBndAttr.loginstatus_t = LVPOCBNDCOM_SIGNAL_LOGIN_FAILED;
	lv_poc_set_apply_note(POC_APPLY_NOTE_TYPE_NOLOGIN);
	osiTimerIsRunning(pocBndAttr.auto_login_timer) ? osiTimerStop(pocBndAttr.auto_login_timer) : 0;
	osiTimerStart(pocBndAttr.auto_login_timer, 5000);
}

static void LvGuiBndCom_pa_timer_cb(void *ctx)
{
	if(pocBndAttr.pa_attr.type == 1)
	{
		OSI_PRINTFI("[pa][timercb](%s)(%d):open", __func__, __LINE__);
		poc_set_ext_pa_status(true);
	}
	else if(pocBndAttr.pa_attr.type == 2)
	{
		OSI_PRINTFI("[pa][timercb](%s)(%d):close", __func__, __LINE__);
		poc_set_ext_pa_status(false);
	}
	pocBndAttr.pa_attr.type = 0;
}


static void LvGuiBndCom_ping_timer_cb(void *ctx)
{
	static int pingnumber = 0;

	if((pingnumber == pocBndAttr.ping_time) || (pocBndAttr.set_ping_first))
	{
		OSI_LOGI(0, "[ping]set ping");
		broad_send_ping();
		pingnumber = 0;
		pocBndAttr.set_ping_first = false;
	}

	pingnumber++;
	osiTimerStart(pocBndAttr.ping_timer, 1000);
}

static void LvGuiBndCom_Call_Bright_Screen_timer_cb(void *ctx)
{
	OSI_PRINTFI("[CallBriScr][timecb](%s)(%d):cb, briscr_dir(%d), curscr_state(%d)", __func__, __LINE__, pocBndAttr.call_briscr_dir, pocBndAttr.call_curscr_state);
	switch(pocBndAttr.call_briscr_dir)
	{
		case LVPOCBNDCOM_CALL_DIR_TYPE_ENTER:
		{
			poc_UpdateLastActiTime();
			break;
		}

		case LVPOCBNDCOM_CALL_DIR_TYPE_SPEAK:
		case LVPOCBNDCOM_CALL_DIR_TYPE_LISTEN:
		{
			osiTimerStop(pocBndAttr.BrightScreen_timer);

			switch(pocBndAttr.call_curscr_state)
			{
				case LVPOCBNDCOM_CALL_LASTSCR_STATE_OPEN:
				{
					OSI_PRINTFI("[callscr](%s)(%d):scr open, scr on continue", __func__, __LINE__);
					poc_UpdateLastActiTime();//continue last'state
					break;
				}

				case LVPOCBNDCOM_CALL_LASTSCR_STATE_CLOSE:
				{
					OSI_PRINTFI("[callscr](%s)(%d):scr close, scr off", __func__, __LINE__);
					poc_set_lcd_status(false);
					break;
				}
			}
			pocBndAttr.call_briscr_dir = LVPOCBNDCOM_CALL_DIR_TYPE_ENTER;
			pocBndAttr.call_curscr_state = LVPOCBNDCOM_CALL_LASTSCR_STATE_START;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void LvGuiBndCom_Check_Recovery_Listen_timer_cb(void *ctx)
{
	OSI_PRINTFI("[check][listen][timer](%s)(%d):cb", __func__, __LINE__);
	if(lvPocGuiBndCom_get_listen_status())
	{
		lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
		lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_LISTEN_SPEAKER_REP, NULL);
	}
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
			osiMutexLock(pocBndAttr.lock);
#if POC_AUDIO_MODE_PIPE
			pocBndAttr.pipe = pocAudioPipeCreate(8192);//8kb
			pocBndAttr.player = pocAudioPlayerCreate(1024);//1kb
#else
			if(pocBndAttr.player == 0)
			{
				pocBndAttr.player = pocAudioPlayerCreate(8192);//8kb
			}
#endif

			if(pocBndAttr.recorder == 0)
			{
				pocBndAttr.recorder = pocAudioRecorderCreate(40960, 320, 20, lvPocGuiBndCom_send_data_callback);
			}

#if POC_AUDIO_MODE_PIPE
			if(pocBndAttr.pipe == 0
				|| pocBndAttr.recorder == 0
				|| pocBndAttr.player == 0)
			{
				pocBndAttr.isReady = false;
				osiMutexUnlock(pocBndAttr.lock);
				osiThreadExit();
			}
#else
			if(pocBndAttr.player == 0
				|| pocBndAttr.recorder == 0)
			{
				pocBndAttr.isReady = false;
				osiMutexUnlock(pocBndAttr.lock);
				osiThreadExit();
			}
#endif
			if(ctx == 0)
			{
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}
			int *login_info = (int *)ctx;
			int UserState = *login_info;

			if (USER_OFFLINE == UserState
				&& m_BndUser.m_status != USER_OFFLINE)
			{
				pocBndAttr.delay_play_voice++;
				if(pocBndAttr.delay_play_voice >= 30 || pocBndAttr.delay_play_voice == 1)
				{
					pocBndAttr.delay_play_voice = 1;
					poc_play_voice_one_time(LVPOCAUDIO_Type_No_Login, 50, false);
				}
				OSI_PRINTFI("[login](%s)(%d):exit", __func__, __LINE__);
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, true);
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, false);
				m_BndUser.m_status = USER_OFFLINE;
				pocBndAttr.loginstatus_t = LVPOCBNDCOM_SIGNAL_LOGIN_EXIT;
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "登录失败");
				osiTimerStop(pocBndAttr.get_group_list_timer);
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SUSPEND_IND, NULL);
				pocBndAttr.isReady = true;
				pocBndAttr.PoweronUpdateIdleInfo = false;
				pocBndAttr.PoweronCurGInfo = false;
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			if(USER_ONLINE == UserState
				&& m_BndUser.m_status != USER_ONLINE)//登录成功
			{
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, false);
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, true);
				pocBndAttr.loginstatus_t = LVPOCBNDCOM_SIGNAL_LOGIN_SUCCESS;

				if((pub_lv_poc_get_watchdog_status()&0x4) == 0x4)
				{
					int status = pub_lv_poc_get_watchdog_status()&0x3;
					pub_lv_poc_set_watchdog_status(status);
				}
				else
				{
					poc_play_voice_one_time(LVPOCAUDIO_Type_Success_Login, 50, false);
					OSI_PRINTFI("[login](%s)(%d):success", __func__, __LINE__);
				}
				osiTimerStop(pocBndAttr.auto_login_timer);

				pocBndAttr.isReady = true;
				m_BndUser.m_status = USER_ONLINE;
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "成功登录");
				osiTimerStart(pocBndAttr.monitor_recorder_timer, 20000);//20S
				osiTimerStartRelaxed(pocBndAttr.get_group_list_timer, 500, OSI_WAIT_FOREVER);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, NULL);
				OSI_PRINTFI("[login](%s)(%d):login in", __func__, __LINE__);

				nv_poc_setting_msg_t * poc_setting_conf = lv_poc_setting_conf_read();
				if(poc_setting_conf->ping > 0)
					poc_set_ping_time(poc_setting_conf->ping);
			}
			osiMutexUnlock(pocBndAttr.lock);

			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_EXIT_IND:
		{
			if(0 == broad_logout())
			{
				pocBndAttr.isReady = false;
				OSI_PRINTFI("[logout](%s)(%d):logout success", __func__, __LINE__);
			}
			else
			{
				OSI_PRINTFI("[logout](%s)(%d):logout failed", __func__, __LINE__);
			}
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
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"未登录", NULL);
				break;
			}

			if(lv_poc_stop_player_voice() != 2)
			{
				//not allow
				if(!lvPocGuiBndCom_get_listen_status()
					&& !lvPocGuiBndCom_get_speak_status())
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"呼叫错误", NULL);
				}
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			m_BndUser.m_status = USER_OPRATOR_START_SPEAK;
			if(osiTimerIsRunning(pocBndAttr.monitor_recorder_timer))
			{
				osiTimerStop(pocBndAttr.monitor_recorder_timer);
			}
			else
			{
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_RESUME_IND, NULL);
			}

			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER, NULL);
			pocBndAttr.is_ptt_state = true;
			osiMutexUnlock(pocBndAttr.lock);
			broad_speak(1);

			OSI_PRINTFI("[speak](%s)(%d):start apply speak", __func__, __LINE__);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SPEAK_START_REP:
		{
			osiMutexLock(pocBndAttr.lock);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_audio, 2, "开始对讲", NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始对讲", NULL);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS, true);
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
			}
			osiMutexUnlock(pocBndAttr.lock);
			OSI_PRINTFI("[speak](%s)(%d):rec start speak, display windows", __func__, __LINE__);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_IND:
		{
			if(m_BndUser.m_status == USER_OFFLINE)
			{
				break;
			}
			OSI_PRINTFI("[speak](%s)(%d):apply stop speak", __func__, __LINE__);
			pocBndAttr.is_ptt_state = false;
			broad_speak(0);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_REP:
		{
			osiMutexLock(pocBndAttr.lock);
			if(m_BndUser.m_status >= USER_OPRATOR_START_SPEAK
				&& pocBndAttr.is_makeout_call == true)
			{
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS, false);

				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, "停止对讲", NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"停止对讲", NULL);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_speak, 2, NULL, NULL);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
				pocBndAttr.is_makeout_call = false;

				//monitor recorder thread
				osiTimerStart(pocBndAttr.monitor_recorder_timer, 10000);//10S
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT, (void *)LVPOCBNDCOM_CALL_DIR_TYPE_SPEAK);
				OSI_PRINTFI("[speak](%s)(%d):rec stop speak, destory windows", __func__, __LINE__);
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SPEAK_TONE_START_IND:
		{
			osiMutexLock(pocBndAttr.lock);
			poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Speak, 30, true);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SPEAK_TONE_STOP_IND:
		{
			osiMutexLock(pocBndAttr.lock);
			bool pttStatus = pocGetPttKeyState()|lv_poc_get_earppt_state();
			if(pttStatus)
			{
				OSI_PRINTFI("[speak](%s)(%d):tone stop, start to launch record", __func__, __LINE__);
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_STOP_PLAY_IND, NULL);
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_START_RECORD_IND, NULL);
				pocBndAttr.speak_status = true;
			}
			else
			{
				if(pocBndAttr.is_makeout_call == true)
				{
					OSI_PRINTFI("[speak](%s)(%d):ptt release", __func__, __LINE__);
					lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SPEAK_STOP_IND, NULL);
				}
			}
			osiMutexUnlock(pocBndAttr.lock);
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

			osiMutexLock(pocBndAttr.lock);
			if(lvPocGuiBndCom_get_status() == USER_OFFLINE)
			{
				pocBndAttr.pocGetGroupListCb(0, 0, NULL);
				pocBndAttr.pocGetGroupListCb = NULL;
				osiMutexUnlock(pocBndAttr.lock);
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
					OSI_PRINTFI("[grouplist](%s)(%d):error", __func__, __LINE__);
					pocBndAttr.pocGetGroupListCb(0, 0, NULL);
					pocBndAttr.pocGetGroupListCb = NULL;
					osiMutexUnlock(pocBndAttr.lock);
					break;
				}

				for(unsigned int  i = 0; i < m_BndUser.m_Group.m_Group_Num; i++)
				{
					memset(&BndGroupId, 0, 20);
					strcpy((char *)m_BndUser.m_Group.m_Group[i].m_ucGName, (char *)BndCGroup[i].name);
				   	__itoa(BndCGroup[i].gid, (char *)BndGroupId, 10);
					strcpy((char *)m_BndUser.m_Group.m_Group[i].m_ucGNum, (char *)BndGroupId);
					m_BndUser.m_Group.m_Group[i].index = BndCGroup[i].index;

					OSI_PRINTFI("[grouplist](%s)(%d):GName(%s), Gid(%s), index(%d)", __func__, __LINE__, m_BndUser.m_Group.m_Group[i].m_ucGName, m_BndUser.m_Group.m_Group[i].m_ucGNum, m_BndUser.m_Group.m_Group[i].index);
				}

				pocBndAttr.pocGetGroupListCb(1, m_BndUser.m_Group.m_Group_Num, m_BndUser.m_Group.m_Group);
				pocBndAttr.pocGetGroupListCb = NULL;
			}while(0);
			osiMutexUnlock(pocBndAttr.lock);

			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_QUERY_REP:
		{
			osiMutexLock(pocBndAttr.lock);
			if(!lv_poc_get_is_in_grouplist())
			{
				OSI_PRINTFI("[group][refresh](%s)(%d):refr request error, no in list", __func__, __LINE__);
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}
			OSI_PRINTFI("[group][refresh](%s)(%d):rev request grouprefr, launch lv", __func__, __LINE__);
			lv_poc_refr_func_ui(lvPocGuiBndCom_Receive_ListUpdate,
						LVPOCLISTIDTCOM_LIST_PERIOD_30, LV_TASK_PRIO_HIGH, (void *)LVPOCBNDCOM_GROUPUPDATE_REP);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			osiMutexLock(pocBndAttr.lock);
			pocBndAttr.pocGetGroupListCb = (lv_poc_get_group_list_cb_t)ctx;
			osiTimerStart(pocBndAttr.check_ack_timeout_timer, 3000);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
		{
			pocBndAttr.pocGetGroupListCb = NULL;
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_REFRESH_BUF_REP:
		{
			char BndGroupId[20] = {0};
			bnd_group_t BndCGroup[GUIBNDCOM_GROUPMAX] = {0};

			m_BndUser.m_Group.m_Group_Num = 0;
			m_BndUser.m_Group.m_Group_Num = broad_get_grouplist(BndCGroup, GUIBNDCOM_GROUPMAX, 0, -1);

			if(m_BndUser.m_Group.m_Group_Num < 0 || m_BndUser.m_Group.m_Group_Num > GUIBNDCOM_GROUPMAX)
			{
				OSI_PRINTFI("[grouplist](%s)(%d):error", __func__, __LINE__);
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			for(unsigned int  i = 0; i < m_BndUser.m_Group.m_Group_Num; i++)
			{
				memset(&BndGroupId, 0, 20);
				strcpy((char *)m_BndUser.m_Group.m_Group[i].m_ucGName, (char *)BndCGroup[i].name);
			   	__itoa(BndCGroup[i].gid, (char *)BndGroupId, 10);
				strcpy((char *)m_BndUser.m_Group.m_Group[i].m_ucGNum, (char *)BndGroupId);
				m_BndUser.m_Group.m_Group[i].index = BndCGroup[i].index;

				OSI_PRINTFI("[poweron][grouplist][update](%s)(%d):GName(%s), Gid(%s), index(%d)", __func__, __LINE__, m_BndUser.m_Group.m_Group[i].m_ucGName, m_BndUser.m_Group.m_Group[i].m_ucGNum, m_BndUser.m_Group.m_Group[i].index);
			}
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

			osiMutexLock(pocBndAttr.lock);
			if(lvPocGuiBndCom_get_status() == USER_OFFLINE)
			{
				pocBndAttr.pocGetMemberListCb(0, 0, NULL);
				pocBndAttr.pocGetMemberListCb = NULL;
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			bnd_member_t BndGData_s[GUIBNDCOM_MEMBERMAX] = {0};
			char BndMemberId[20] = {0};
			memset(&pocBndAttr.BndMsgGMemberBuf, 0, sizeof(Msg_GData_s));

			if(ctx == 0)//memberlist
			{
				do
				{
					if(pocBndAttr.is_enter_signal_multi_call == true)//multi-call
					{
						nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
						poc_config->tmpgroupid = pocBndAttr.BndTmpGrpInf.gid;
					    lv_poc_setting_conf_write();
						pocBndAttr.BndMsgGMemberBuf.dwNum = lvPocGuiBndCom_try_get_memberinfo(pocBndAttr.BndTmpGrpInf.gid, BndGData_s);
					}
					else
					{
						pocBndAttr.BndMsgGMemberBuf.dwNum = lvPocGuiBndCom_try_get_memberinfo(pocBndAttr.BndCurrentGroupInfo.gid, BndGData_s);
					}

					if(pocBndAttr.BndMsgGMemberBuf.dwNum < 0)
					{
						pocBndAttr.pocGetMemberListCb(0, 0, NULL);
						pocBndAttr.pocGetMemberListCb = NULL;
						pocBndAttr.is_enter_signal_multi_call = false;//multi-call
						OSI_PRINTFI("[memberlist](%s)(%d):error", __func__, __LINE__);
						osiMutexUnlock(pocBndAttr.lock);
						break;
					}

					if(pocBndAttr.is_enter_signal_multi_call == true)//multi-call
					{
						OSI_PRINTFI("[multi-call][activitylist](%s)(%d):open", __func__, __LINE__);
						pocBndAttr.is_enter_signal_multi_call = false;
						lv_poc_activity_func_cb_set.build_tmp_open(pocBndAttr.BndTmpGrpInf.name);
						poc_play_voice_one_time(LVPOCAUDIO_Type_Enter_Temp_Group, 50, false);
					}

					//not allow
					if(!lvPocGuiBndCom_get_listen_status()
						&& !lvPocGuiBndCom_get_speak_status())
					{
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取成员列表...", NULL);
					}

					for(unsigned long  i = 0; i < pocBndAttr.BndMsgGMemberBuf.dwNum; i++)
					{
						memset(BndMemberId, 0, 20);
						strcpy((char *)pocBndAttr.BndMsgGMemberBuf.member[i].ucName, (char *)BndGData_s[i].name);
					   	__itoa(BndGData_s[i].uid, (char *)BndMemberId, 10);
						strcpy((char *)pocBndAttr.BndMsgGMemberBuf.member[i].ucNum, (char *)BndMemberId);
						pocBndAttr.BndMsgGMemberBuf.member[i].ucPrio = BndGData_s[i].prior;
						if(BndGData_s[i].state == 2)//offline
						{
							pocBndAttr.BndMsgGMemberBuf.member[i].ucStatus = 0;
						}
						else
						{
							pocBndAttr.BndMsgGMemberBuf.member[i].ucStatus = 1;
						}

						OSI_PRINTFI("[memberlist](%s)(%d):Uname(%s), Uid(%s), Status(%d)", __func__, __LINE__, pocBndAttr.BndMsgGMemberBuf.member[i].ucName, pocBndAttr.BndMsgGMemberBuf.member[i].ucNum, pocBndAttr.BndMsgGMemberBuf.member[i].ucStatus);
					}
					pocBndAttr.pocGetMemberListCb(1, pocBndAttr.BndMsgGMemberBuf.dwNum, &pocBndAttr.BndMsgGMemberBuf);

					//not allow
					if(!lvPocGuiBndCom_get_listen_status()
						&& !lvPocGuiBndCom_get_speak_status())
					{
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
					}
				}while(0);

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
					OSI_PRINTFI("[groupxmember](%s)(%d):error", __func__, __LINE__);
					pocBndAttr.pocGetMemberListCb(0, 0, NULL);
					pocBndAttr.pocGetMemberListCb = NULL;
					pocBndAttr.query_group = 0;
					osiMutexUnlock(pocBndAttr.lock);
					break;
				}
				//query
				int gid = atoi((char *)group_info->m_ucGNum);

				OSI_PRINTFI("[groupxmember](%s)(%d):gid(%d)", __func__, __LINE__, gid);
				if(pocBndAttr.haveingroup)//in group
				{
					lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REP, (void *)gid);
					OSI_PRINTFI("[groupxmember](%s)(%d):group in", __func__, __LINE__);
				}
				else//other group
				{
					OSI_PRINTFI("[groupxmember](%s)(%d):group out", __func__, __LINE__);
					if(broad_joingroup(gid) != 0)
					{
						OSI_PRINTFI("[groupxmember](%s)(%d):error", __func__, __LINE__);
						pocBndAttr.pocGetMemberListCb(0, 0, NULL);
						pocBndAttr.pocGetMemberListCb = NULL;
					}
					osiTimerStart(pocBndAttr.check_ack_timeout_timer, 3000);
				}
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
		{
			if(lvPocGuiBndCom_get_status() == USER_OFFLINE)
			{
				if(pocBndAttr.pocGetMemberListCb != NULL)
				{
					pocBndAttr.pocGetMemberListCb(0, 0, NULL);
					pocBndAttr.pocGetMemberListCb = NULL;
				}
				OSI_PRINTFI("[memberlist](%s)(%d):user offline", __func__, __LINE__);
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			if(pocBndAttr.pocGetMemberListCb == NULL)
			{
				if(pocBndAttr.is_have_join_group)//member call join group
				{
					OSI_PRINTFI("[memberlist](%s)(%d):join group", __func__, __LINE__);
					pocBndAttr.is_have_join_group = false;
					poc_play_voice_one_time(LVPOCAUDIO_Type_Join_Group, 50, false);
				}
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			if(ctx == 0)
			{
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			static int gid = 0;

			bnd_member_t BndGData_s[GUIBNDCOM_MEMBERMAX] = {0};
			char BndMemberId[20] = {0};
			memset(&pocBndAttr.BndMsgGMemberBuf, 0, sizeof(Msg_GData_s));

			do
			{
				gid = (int)ctx;
				pocBndAttr.BndMsgGMemberBuf.dwNum = lvPocGuiBndCom_try_get_memberinfo(gid, BndGData_s);
				OSI_PRINTFI("[groupxmember](%s)(%d):m_gid(%d), m_Member_Num(%d)", __func__, __LINE__, gid, pocBndAttr.BndMsgGMemberBuf.dwNum);

				if(pocBndAttr.BndMsgGMemberBuf.dwNum < 0 || pocBndAttr.BndMsgGMemberBuf.dwNum > GUIBNDCOM_MEMBERMAX)
				{
					OSI_PRINTFI("[memberlist](%s)(%d):error", __func__, __LINE__);
					pocBndAttr.timer_user_data = (void *)gid;
					pocBndAttr.info_error_type = ERROR_TYPE_MEMBER_LIST_GET;
					osiTimerStartPeriodic(pocBndAttr.get_member_list_timer, 100);
					osiMutexUnlock(pocBndAttr.lock);
					return;
				}

				for(unsigned long  i = 0; i < pocBndAttr.BndMsgGMemberBuf.dwNum; i++)
				{
					memset(BndMemberId, 0, 20);
					strcpy((char *)pocBndAttr.BndMsgGMemberBuf.member[i].ucName, (char *)BndGData_s[i].name);
					__itoa(BndGData_s[i].uid, (char *)BndMemberId, 10);
					strcpy((char *)pocBndAttr.BndMsgGMemberBuf.member[i].ucNum, (char *)BndMemberId);
					pocBndAttr.BndMsgGMemberBuf.member[i].ucPrio = BndGData_s[i].prior;
					if(BndGData_s[i].state == 2)//offline
					{
						pocBndAttr.BndMsgGMemberBuf.member[i].ucStatus = 0;
					}
					else
					{
						pocBndAttr.BndMsgGMemberBuf.member[i].ucStatus = 1;
					}
					OSI_PRINTFI("[groupxmember](%s)(%d):Uname(%s), Uid(%s), Status(%d)", __func__, __LINE__, pocBndAttr.BndMsgGMemberBuf.member[i].ucName, pocBndAttr.BndMsgGMemberBuf.member[i].ucNum, pocBndAttr.BndMsgGMemberBuf.member[i].ucStatus);
				}
				pocBndAttr.pocGetMemberListCb(1, pocBndAttr.BndMsgGMemberBuf.dwNum, &pocBndAttr.BndMsgGMemberBuf);
			}while(0);

			if(!pocBndAttr.haveingroup)//join group
			{
				OSI_PRINTFI("[memberlist](%s)(%d):join group", __func__, __LINE__);
				pocBndAttr.haveingroup = true;
				poc_play_voice_one_time(LVPOCAUDIO_Type_Join_Group, 50, false);
			}

			pocBndAttr.pocGetMemberListCb = NULL;
			//not allow
			if(!lvPocGuiBndCom_get_listen_status()
				&& !lvPocGuiBndCom_get_speak_status())
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			}
			osiMutexUnlock(pocBndAttr.lock);

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

		case LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REFRESH_REP:
		{
			osiMutexLock(pocBndAttr.lock);
			if(!lv_poc_get_is_in_memberlist()
				|| !pocBndAttr.haveingroup)
			{
				OSI_PRINTFI("[member][refresh](%s)(%d):refr request error, no in list", __func__, __LINE__);
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			bool Usesta = false;
			Msg_GData_s BndMsgGMember = {0};
			bnd_member_t BndGData_s[GUIBNDCOM_MEMBERMAX] = {0};
			int gid = atoi((char *)m_BndUser.m_Group.m_Group[pocBndAttr.current_group].m_ucGNum);

			BndMsgGMember.dwNum = lvPocGuiBndCom_try_get_memberinfo(gid, BndGData_s);

			if(BndMsgGMember.dwNum < 0 || BndMsgGMember.dwNum > GUIBNDCOM_MEMBERMAX)
			{
				OSI_PRINTFI("[member][update](%s)(%d):error, launch time to get", __func__, __LINE__);
				pocBndAttr.info_error_type = ERROR_TYPE_MEMBER_REFRESH;
				osiTimerStartPeriodic(pocBndAttr.get_member_list_timer, 100);
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			int  objindex = 0;
			bool is_update = false;

			for(unsigned long i = 0; i < BndMsgGMember.dwNum; i++)//1--online, 2--offline, 3--online in group
			{
				for(unsigned long j = 0; j < BndMsgGMember.dwNum; j++)
				{
					if(0 == strcmp(BndGData_s[i].name, (char *)pocBndAttr.BndMsgGMemberBuf.member[j].ucName))
					{
						if(BndGData_s[i].state == 1
							|| BndGData_s[i].state == 3)//1 and 3 online
						{
							if(pocBndAttr.BndMsgGMemberBuf.member[j].ucStatus == 0)
							{
								Usesta = true;
								objindex = j;
								is_update = true;
								pocBndAttr.BndMsgGMemberBuf.member[j].ucStatus = 1;
								OSI_PRINTFI("[member][refresh](%s)(%d):member update online", __func__, __LINE__);
								break;
							}
						}
						else//2 offline
						{
							if(pocBndAttr.BndMsgGMemberBuf.member[j].ucStatus == 1)
							{
								Usesta = false;
								objindex = j;
								is_update = true;
								pocBndAttr.BndMsgGMemberBuf.member[j].ucStatus = 0;
								OSI_PRINTFI("[member][refresh](%s)(%d):member update offline", __func__, __LINE__);
								break;
							}
						}
					}
				}

				if(is_update)
				{
					break;
				}
			}

			if(!is_update)
			{
				OSI_PRINTFI("[member][update](%s)(%d):no update", __func__, __LINE__);
				if(pocBndAttr.is_login_memberlist_first_update)
				{
					pocBndAttr.is_login_memberlist_first_update = false;
					lv_poc_refr_func_ui(lvPocGuiBndCom_Receive_ListUpdate,
						LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH, (void *)LVPOCBNDCOM_MEMBERUPDATE_REP);
				}

			}
			else
			{
				//update member
				OSI_PRINTFI("[member][update](%s)(%d):start refresh, launch lv", __func__, __LINE__);
				lv_poc_activity_func_cb_set.member_list.set_state(NULL, (const char *)pocBndAttr.BndMsgGMemberBuf.member[objindex].ucName, (void *)&pocBndAttr.BndMsgGMemberBuf.member[objindex], Usesta);
				lv_poc_refr_func_ui(lvPocGuiBndCom_Receive_ListUpdate,
					LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH, (void *)LVPOCBNDCOM_MEMBER_ONOFFLINE_REP);
			}
			osiMutexUnlock(pocBndAttr.lock);

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

			if(ctx == 0
				|| lvPocGuiBndCom_get_status() == USER_OFFLINE)
			{
				pocBndAttr.pocSetCurrentGroupCb(0);
				pocBndAttr.pocSetCurrentGroupCb = NULL;
				OSI_PRINTFI("[sercurgrp](%s)(%d):ctx or user offline", __func__, __LINE__);
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			CGroup * group_info = (CGroup *)ctx;

			//get click group type
			bnd_group_t bnd_group_attr = {0};
			bnd_gid_t gid = atoi((char *)group_info->m_ucGNum);
			int status = broad_group_getbyid(gid, &bnd_group_attr);
			if(status != 0)//error
			{
				pocBndAttr.pocSetCurrentGroupCb(0);
				pocBndAttr.pocSetCurrentGroupCb = NULL;
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			if(bnd_group_attr.type == GRP_COMMON)//set cur group
			{
				lv_poc_activity_func_cb_set.group_member_activity_open((char *)group_info->m_ucGName);

				unsigned int index = 0;
				for(index = 0; index < m_BndUser.m_Group.m_Group_Num; index++)
				{
					if(0 == strcmp((const char *)group_info->m_ucGNum, (const char *)m_BndUser.m_Group.m_Group[index].m_ucGNum)) break;
				}

				if(m_BndUser.m_Group.m_Group_Num < 1
					|| index >=  m_BndUser.m_Group.m_Group_Num)
				{
					OSI_PRINTFI("[setcurgrp](%s)(%d):error, index(%d)", __func__, __LINE__, index);
					pocBndAttr.pocSetCurrentGroupCb(0);
				}
				else if(index == pocBndAttr.current_group)//已在群组
				{
					pocBndAttr.haveingroup = true;
					pocBndAttr.pocSetCurrentGroupCb(2);
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocBndAttr.self_info.ucName, m_BndUser.m_Group.m_Group[pocBndAttr.current_group].m_ucGName);
					OSI_PRINTFI("[setcurgrp](%s)(%d):current group in", __func__, __LINE__, index);
				}
				else//切组成功
				{
					{
						nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
						strcpy(poc_config->curren_group_id, (char *)m_BndUser.m_Group.m_Group[index].m_ucGNum);
						lv_poc_setting_conf_write();
					}//wait for some time
					pocBndAttr.haveingroup = false;
					pocBndAttr.current_group = index;
					pocBndAttr.pocSetCurrentGroupCb(1);
					lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocBndAttr.self_info.ucName, m_BndUser.m_Group.m_Group[pocBndAttr.current_group].m_ucGName);
					//not allow
					if(!lvPocGuiBndCom_get_listen_status()
						&& !lvPocGuiBndCom_get_speak_status())
					{
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取群组成员...", NULL);
					}
					OSI_PRINTFI("[setcurgrp](%s)(%d):current group out", __func__, __LINE__, index);
				}
			}
			else//multi call
			{
				pocBndAttr.haveingroup = false;
				pocBndAttr.pocSetCurrentGroupCb(1);
				lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_normal_info, 2, (char *)pocBndAttr.self_info.ucName, m_BndUser.m_Group.m_Group[pocBndAttr.current_group].m_ucGName);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_SPEAKING, (const uint8_t *)"获取临时组成员...", NULL);
				OSI_PRINTFI("[setcurgrp](%s)(%d):current group out, join tmpgrp", __func__, __LINE__);
			}
			pocBndAttr.pocSetCurrentGroupCb = NULL;
			osiMutexUnlock(pocBndAttr.lock);
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

static void prvPocGuiBndTaskHandleBuildTempGrp(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			if(pocBndAttr.pocBuildTempGrpCb == NULL)
			{
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			if(lvPocGuiBndCom_get_status() == USER_OFFLINE)
			{
				pocBndAttr.pocBuildTempGrpCb(0);
				pocBndAttr.pocBuildTempGrpCb = NULL;
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			char multimemberid[512];
			bnd_uid_t multi_uid[GUIBNDCOM_MEMBERMAX] = {0};
			lv_poc_build_new_tempgrp_t * new_tempgrp = (lv_poc_build_new_tempgrp_t *)ctx;
			Msg_GROUP_MEMBER_s *member_info = NULL;
			memset(multimemberid, 0, sizeof(512));

			for(int i = 0; i < new_tempgrp->num; i++)
			{
				member_info = (Msg_GROUP_MEMBER_s *)new_tempgrp->members[i];
				if(member_info == NULL)
				{
					OSI_PRINTFI("[build][tempgrp](%s)(%d)failed", __func__, __LINE__);
					pocBndAttr.pocBuildTempGrpCb(0);
					pocBndAttr.pocBuildTempGrpCb = NULL;
					osiMutexUnlock(pocBndAttr.lock);
					return;
				}
				multi_uid[i] = atoi((const char *)member_info->ucNum);
				strcat(multimemberid, (const char *)member_info->ucNum);
				strcat(multimemberid, "/");
			}
			OSI_PRINTFI("[build][tempgrp](%s)(%d):multi-id(%s)", __func__, __LINE__, multimemberid);
			//request member multi calls
			broad_callusers(multi_uid, new_tempgrp->num, callback_BND_MultiCallmember);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_REP:
		{
			if(pocBndAttr.pocBuildTempGrpCb == NULL)
			{
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			if(lvPocGuiBndCom_get_status() == USER_OFFLINE)
			{
				pocBndAttr.pocBuildTempGrpCb(0);
				pocBndAttr.pocBuildTempGrpCb = NULL;
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			pocBndAttr.pocBuildTempGrpCb(0);
			pocBndAttr.pocBuildTempGrpCb = NULL;
			OSI_PRINTFI("[build][tempgrp](%s)(%d)request tmpgrp error", __func__, __LINE__);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_GET_MEMBER_REP:
		{
			osiMutexLock(pocBndAttr.lock);
			if(lvPocGuiBndCom_get_status() == USER_OFFLINE)
			{
				if(pocBndAttr.pocBuildTempGrpCb != NULL)
				{
					pocBndAttr.pocBuildTempGrpCb(0);
					pocBndAttr.pocBuildTempGrpCb = NULL;
				}
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			if(pocBndAttr.pocBuildTempGrpCb == NULL)
			{
				OSI_PRINTFI("[call][tempgrp](%s)(%d):rev call_memberinfo", __func__, __LINE__);
				lv_poc_activity_func_cb_set.build_tmp_get_info();
			}
			else
			{
				OSI_PRINTFI("[call][tempgrp](%s)(%d):start request call_memberinfo", __func__, __LINE__);
				pocBndAttr.pocBuildTempGrpCb(1);
				pocBndAttr.pocBuildTempGrpCb = NULL;
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_EXIT_REP:
		{
			osiMutexLock(pocBndAttr.lock);
			pocBndAttr.single_multi_call_type = USER_OPRATOR_START;
			lv_poc_activity_func_cb_set.build_tmp_close(POC_EXITGRP_PASSIVE);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_REGISTER_BIUILD_TEMPGRP_CB_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			pocBndAttr.pocBuildTempGrpCb = (poc_build_tempgrp_cb)ctx;
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_BIUILD_TEMPGRP_CB_IND:
		{
			OSI_PRINTFI("[tempgrp](%s)(%d):cannel build tempgrp cb", __func__, __LINE__);
			pocBndAttr.pocBuildTempGrpCb = NULL;
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

			osiMutexLock(pocBndAttr.lock);
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
			osiMutexUnlock(pocBndAttr.lock);

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

			osiMutexLock(pocBndAttr.lock);
			pocBndAttr.pocGetMemberStatusCb(ctx > 0);
			pocBndAttr.pocGetMemberStatusCb = NULL;
			osiMutexUnlock(pocBndAttr.lock);
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

			osiMutexLock(pocBndAttr.lock);
#if POC_AUDIO_MODE_PIPE
			pocAudioPipeStop();
#else
			pocAudioPlayerStop(pocBndAttr.player);
#endif
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
			osiMutexLock(pocBndAttr.lock);
#if POC_AUDIO_MODE_PIPE
			pocAudioPipeStart();
#else
			pocAudioPlayerStart(pocBndAttr.player);
#endif
			pocBndAttr.listen_status = true;
			m_BndUser.m_status = USER_OPRATOR_LISTENNING;
			osiMutexUnlock(pocBndAttr.lock);
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
			osiMutexLock(pocBndAttr.lock);
			OSI_LOGI(0, "[record](%d):stop record", __LINE__);
			pocAudioRecorderStop(pocBndAttr.recorder);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_START_RECORD_IND:
		{
			if(pocBndAttr.recorder == 0)
			{
				break;
			}
			osiMutexLock(pocBndAttr.lock);
			OSI_LOGI(0, "[record](%d):start record", __LINE__);
			m_BndUser.m_status = USER_OPRATOR_SPEAKING;
			pocAudioRecorderStart(pocBndAttr.recorder);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SEND_RECORD_DATA_IND:
		{
			if(ctx == 0)
			{
				break;
			}
			osiMutexLock(pocBndAttr.lock);
			unsigned char *pRecordData = (unsigned char *)ctx;
			lib_oem_record_cb(pRecordData, 320);
			osiMutexUnlock(pocBndAttr.lock);
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

			osiMutexLock(pocBndAttr.lock);
		    lv_poc_member_call_config_t *member_call_config = (lv_poc_member_call_config_t *)ctx;
		    Msg_GROUP_MEMBER_s *member_call_obj = (Msg_GROUP_MEMBER_s *)member_call_config->members;

		    if(member_call_config->func == NULL)
		    {
				OSI_LOGI(0, "[singlecall](%d)cb func null", __LINE__);
				osiMutexUnlock(pocBndAttr.lock);
			    break;
		    }

		    if(member_call_config->enable && member_call_obj == NULL)
		    {
				OSI_LOGI(0, "[singlecall](%d)error param", __LINE__);
			    lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"错误参数", NULL);
				member_call_config->func(2, 2);
				osiMutexUnlock(pocBndAttr.lock);
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
					broad_calluser(callnumber, callback_BND_SingalCallmember);

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
				OSI_PRINTFI("[singlecall](%s)(%d):local exit single call", __func__, __LINE__);
				bool is_member_call = pocBndAttr.is_member_call;
                pocBndAttr.is_member_call = false;
                pocBndAttr.member_call_dir = 0;
                if(!is_member_call)
                {
					osiMutexUnlock(pocBndAttr.lock);
                    return;
                }

                member_call_config->func(1, 1);
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP, NULL);
			}
			osiMutexUnlock(pocBndAttr.lock);

			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_OK_REP:
		{
			if(ctx == 0)
			{
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			int SigalCallgid = (int)ctx;
			pocBndAttr.signalcall_gid = SigalCallgid;

			 //对方同意了单呼通话
		    if(pocBndAttr.pocMemberCallCb != NULL)
		    {
			    pocBndAttr.pocMemberCallCb(0, 0);
			    pocBndAttr.pocMemberCallCb = NULL;
		    }
			OSI_LOGI(0, "[singlecall](%d)local single call success, enter's gid(%d)", __LINE__, pocBndAttr.signalcall_gid);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_ERROR_REP:
		{
			osiMutexLock(pocBndAttr.lock);
			pocBndAttr.is_member_call = false;
			pocBndAttr.member_call_dir = 0;
			pocBndAttr.signalcall_gid = 0;

			lv_poc_activity_func_cb_set.member_call_close();
			OSI_LOGI(0, "[singlecall](%d)call error", __LINE__);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
		{
			if(ctx == 0
				|| pocBndAttr.is_member_call == false)
			{
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			if(pocBndAttr.single_multi_call_type == USER_OPRATOR_MULTI_CALL)
			{
				pocBndAttr.is_member_call = false;
				OSI_PRINTFI("[cbbnd][tmpgrp](%s)(%d):rec exit multi-call, launch", __func__, __LINE__);
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_EXIT_REP, NULL);
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
				pocBndAttr.single_multi_call_type = USER_OPRATOR_START;

				poc_play_voice_one_time(LVPOCAUDIO_Type_Exit_Member_Call, 30, true);
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"退出单呼", NULL);
				lv_poc_activity_func_cb_set.member_call_close();
				OSI_LOGI(0, "[singlecall](%d)other exit single call", __LINE__);
			}
			else
			{
				OSI_LOGI(0, "[singlecall](%d)error, leave's gid(%d)", __LINE__, SigalCallgid);
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_GET_MEMBER_CALL_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			pocBndAttr.SignalCallInf = (bnd_group_t *)ctx;

			static Msg_GROUP_MEMBER_s member_call_obj = {0};
			memset(&member_call_obj, 0, sizeof(Msg_GROUP_MEMBER_s));
			strcpy((char *)member_call_obj.ucName, (const char *)pocBndAttr.SignalCallInf->name);//临时呼叫的群组名就是用户名
			member_call_obj.ucStatus = USER_ONLINE;
			pocBndAttr.signalcall_gid = pocBndAttr.SignalCallInf->gid;
			pocBndAttr.member_call_dir = 1;
			lv_poc_activity_func_cb_set.member_call_open((void *)&member_call_obj);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_SELECT_CALL_MODE:
		{
			if(ctx == 0)
			{
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			pocBndAttr.SignalCallInf = (bnd_group_t *)ctx;
			bnd_member_t BndGData_s[GUIBNDCOM_MEMBERMAX] = {0};
			int m_GNum = lvPocGuiBndCom_try_get_memberinfo(pocBndAttr.SignalCallInf->gid, BndGData_s);

			if(m_GNum < 0)
			{
				OSI_PRINTFI("[cbbnd][calltype](%s)(%d):get memberinf error, launch time to get", __func__, __LINE__);
				pocBndAttr.info_error_type = ERROR_TYPE_SINGLE_OR_MULTI_CALL;
				osiTimerStartPeriodic(pocBndAttr.get_member_list_timer, 100);
				osiMutexUnlock(pocBndAttr.lock);
				break;
			}

			if(m_GNum < 2)//single call
			{
				OSI_PRINTFI("[cbbnd][calltype](%s)(%d):launch single call", __func__, __LINE__);
				pocBndAttr.single_multi_call_type = USER_OPRATOR_SIGNAL_CALL;
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_GET_MEMBER_CALL_IND, (void *)pocBndAttr.SignalCallInf);
			}
			else//multi call
			{
				OSI_PRINTFI("[cbbnd][calltype](%s)(%d):launch multi call", __func__, __LINE__);
				pocBndAttr.single_multi_call_type = USER_OPRATOR_MULTI_CALL;
				pocBndAttr.is_member_call = true;
				pocBndAttr.is_enter_signal_multi_call = true;

				//tmpgrp info
				pocBndAttr.BndTmpGrpInf.gid = pocBndAttr.SignalCallInf->gid;
				pocBndAttr.BndTmpGrpInf.type = pocBndAttr.SignalCallInf->type;
				pocBndAttr.BndTmpGrpInf.index = pocBndAttr.SignalCallInf->index;
				strcpy(pocBndAttr.BndTmpGrpInf.name, pocBndAttr.SignalCallInf->name);
				//launch
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_GET_MEMBER_REP, NULL);
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_REGET_INFO_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			int m_GNum = (int)ctx;
			if(m_GNum < 2)//single call
			{
				OSI_PRINTFI("[cbbnd][calltype][reget](%s)(%d):launch single call", __func__, __LINE__);
				pocBndAttr.single_multi_call_type = USER_OPRATOR_SIGNAL_CALL;
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_GET_MEMBER_CALL_IND, (void *)pocBndAttr.SignalCallInf);
			}
			else//multi call
			{
				OSI_PRINTFI("[cbbnd][calltype][reget](%s)(%d):launch multi call", __func__, __LINE__);
				pocBndAttr.single_multi_call_type = USER_OPRATOR_MULTI_CALL;
				pocBndAttr.is_member_call = true;
				pocBndAttr.is_enter_signal_multi_call = true;

				//tmpgrp info
				pocBndAttr.BndTmpGrpInf.gid = pocBndAttr.SignalCallInf->gid;
				pocBndAttr.BndTmpGrpInf.type = pocBndAttr.SignalCallInf->type;
				pocBndAttr.BndTmpGrpInf.index = pocBndAttr.SignalCallInf->index;
				strcpy(pocBndAttr.BndTmpGrpInf.name, pocBndAttr.SignalCallInf->name);
				//launch
				lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_GET_MEMBER_REP, NULL);
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_EXIT_SINGLE_JOIN_CURRENT_GROUP:
		{
			osiMutexLock(pocBndAttr.lock);
			int gid = pocBndAttr.BndCurrentGroupInfo.gid;
			if(broad_joingroup(gid) != 0)
			{
				OSI_LOGXI(OSI_LOGPAR_II, 0, "[exitsinglecall](%d)join group error, gid(%d)", __LINE__, gid);
				//not allow
				if(!lvPocGuiBndCom_get_listen_status()
					&& !lvPocGuiBndCom_get_speak_status())
				{
					lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"加入群组失败", NULL);
				}
			}
			osiMutexUnlock(pocBndAttr.lock);
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
			osiMutexLock(pocBndAttr.lock);
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, false);

			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, "停止聆听", "");

			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_LISTENING, (const uint8_t *)"停止聆听", (const uint8_t *)"");
			lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_listen, 2, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			OSI_LOGXI(OSI_LOGPAR_SI, 0, "[listen][cbbnd](%s)(%d):rec stop listen and destory windows", __FUNCTION__, __LINE__);
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT, (void *)LVPOCBNDCOM_CALL_DIR_TYPE_LISTEN);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_LISTEN_SPEAKER_REP:
		{
			static char speaker_name_buf[100] = {0};
			char speaker_name[100];

			osiMutexLock(pocBndAttr.lock);
			if(ctx == 0)
			{
				strcpy(speaker_name, (const char *)speaker_name_buf);
			}
			else
			{
				memset(speaker_name_buf, 0, sizeof(char) * 100);
				memset(speaker_name, 0, sizeof(char) * 100);
				strcpy(speaker_name_buf, (const char *)ctx);
				strcpy(speaker_name, (const char *)speaker_name_buf);
			}

			strcat(speaker_name, (const char *)"正在讲话");
			m_BndUser.m_status = USER_OPRATOR_START_LISTEN;
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, true);

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
			lvPocGuiBndCom_Msg(LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER, NULL);
			osiMutexUnlock(pocBndAttr.lock);
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

			osiMutexLock(pocBndAttr.lock);
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
			osiMutexUnlock(pocBndAttr.lock);

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
			//add shutdown opt
			osiShutdown(OSI_SHUTDOWN_POWER_OFF);
			break;
		}
	}

}

static
void prvPocGuiBndTaskHandleErrorInfo(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_GET_ERROR_CASE:
		{
			if(ctx == 0)
			{
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			char *pErrorInfo = (char *)ctx;

			if(strstr((const char *)pErrorInfo, (const char *)"账号已在其他位置登录"))
			{
				poc_play_voice_one_time(LVPOCAUDIO_Type_This_Account_Already_Logined, 50, true);
				m_BndUser.m_status = USER_OFFLINE;
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, true);
			}
			else if(strstr((const char *)pErrorInfo, (const char *)"单呼失败"))
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"用户忙线中", NULL);
				break;
			}

			//not allow
			if(!lvPocGuiBndCom_get_listen_status()
				&& !lvPocGuiBndCom_get_speak_status())
			{
				lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)pErrorInfo, NULL);
			}

			osiMutexUnlock(pocBndAttr.lock);
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
			osiMutexLock(pocBndAttr.lock);
			if((osiThread_t *)lv_poc_recorder_Thread() != NULL)
			{
				OSI_LOGI(0, "[recorder]start suspend");
				osiThreadSuspend((osiThread_t *)lv_poc_recorder_Thread());
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_RESUME_IND:
		{
			osiMutexLock(pocBndAttr.lock);
			if((osiThread_t *)lv_poc_recorder_Thread() != NULL)
			{
				OSI_LOGI(0, "[recorder]start resume");
				osiThreadResume((osiThread_t *)lv_poc_recorder_Thread());
			}
			osiMutexUnlock(pocBndAttr.lock);
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
	nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_START_SPEAK:
		{
			//no deal
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_SPEAK:
		{
			osiMutexLock(pocBndAttr.lock);
			pocBndAttr.speak_status = false;
			if(poc_config->current_tone_switch == 1)
			{
				poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Speak, 30, true);
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_START_LISTEN:
		{
			osiMutexLock(pocBndAttr.lock);
			if(poc_config->current_tone_switch == 1)
			{
				poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Start_Listen, 30, true);
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_PLAY_TONE_STOP_LISTEN:
		{
			osiMutexLock(pocBndAttr.lock);
			pocBndAttr.listen_status = false;
			if(poc_config->current_tone_switch == 1)
			{
				poc_play_voice_one_time(LVPOCAUDIO_Type_Tone_Stop_Listen, 30, true);
			}
			osiMutexUnlock(pocBndAttr.lock);
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

static void prvPocGuiBndTaskHandleCallBrightScreen(uint32_t id, uint32_t ctx)
{
   switch(id)
   {
      case LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER:
      {
		 osiMutexLock(pocBndAttr.lock);
		 if(!poc_get_lcd_status())
		 {
			 poc_set_lcd_status(true);
			 pocBndAttr.call_curscr_state = LVPOCBNDCOM_CALL_LASTSCR_STATE_CLOSE;
			 OSI_PRINTFI("[callscr](%s)(%d):cur scr close", __func__, __LINE__);
		 }
		 else
		 {
			 pocBndAttr.call_curscr_state = LVPOCBNDCOM_CALL_LASTSCR_STATE_OPEN;
			 OSI_PRINTFI("[callscr](%s)(%d):cur scr open", __func__, __LINE__);
		 }
		 poc_UpdateLastActiTime();
		 osiTimerStartPeriodic(pocBndAttr.BrightScreen_timer, 4000);
		 pocBndAttr.call_briscr_dir = LVPOCBNDCOM_CALL_DIR_TYPE_ENTER;
		 osiMutexUnlock(pocBndAttr.lock);
		 break;
      }

	  case LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT:
	  {
	  	  if(ctx == 0)
	  	  {
			 break;
		  }

		  osiMutexLock(pocBndAttr.lock);
		  switch(ctx)
		  {
			 case LVPOCBNDCOM_CALL_DIR_TYPE_SPEAK:
			 {
				  pocBndAttr.call_briscr_dir = LVPOCBNDCOM_CALL_DIR_TYPE_SPEAK;
				  osiTimerIsRunning(pocBndAttr.BrightScreen_timer) ? \
	     		  osiTimerStop(pocBndAttr.BrightScreen_timer) : 0;
				  osiTimerStart(pocBndAttr.BrightScreen_timer, 4000);
				  OSI_PRINTFI("[callscr](%s)(%d):speak stop", __func__, __LINE__);
				  break;
			 }


			 case LVPOCBNDCOM_CALL_DIR_TYPE_LISTEN:
			 {
				  pocBndAttr.call_briscr_dir = LVPOCBNDCOM_CALL_DIR_TYPE_LISTEN;
				  osiTimerIsRunning(pocBndAttr.BrightScreen_timer) ? \
	     		  osiTimerStop(pocBndAttr.BrightScreen_timer) : 0;
				  osiTimerStart(pocBndAttr.BrightScreen_timer, 4000);
				  OSI_PRINTFI("[callscr](%s)(%d):listen stop", __func__, __LINE__);
				  break;
			 }
		  }
		  osiMutexUnlock(pocBndAttr.lock);
		  break;
	  }

	  case LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_BREAK:
	  {
		  osiMutexLock(pocBndAttr.lock);
	  	  pocBndAttr.call_briscr_dir != LVPOCBNDCOM_CALL_DIR_TYPE_ENTER ? \
	      osiTimerIsRunning(pocBndAttr.BrightScreen_timer) ? \
	      osiTimerStop(pocBndAttr.BrightScreen_timer) : 0 \
	      : 0;
		  OSI_PRINTFI("[callscr](%s)(%d):break", __func__, __LINE__);
		  osiMutexUnlock(pocBndAttr.lock);
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
		case LVPOCGUIBNDCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND:
		{
			osiMutexLock(pocBndAttr.lock);
			OSI_PRINTFI("[timeout](%s)(%d):stop", __func__, __LINE__);
			if(pocBndAttr.check_ack_timeout_timer != NULL)
			{
				osiTimerStop(pocBndAttr.check_ack_timeout_timer);
			}
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_VOICE_PLAY_START_IND:
		{
			osiMutexLock(pocBndAttr.lock);
			OSI_PRINTFI("[poc][audev](%s)(%d):start", __func__, __LINE__);
			lv_poc_set_audevplay_status(true);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		case LVPOCGUIBNDCOM_SIGNAL_VOICE_PLAY_STOP_IND:
		{
			osiMutexLock(pocBndAttr.lock);
			OSI_PRINTFI("[poc][audev](%s)(%d):stop", __func__, __LINE__);
			lv_poc_set_audevplay_status(false);
			osiMutexUnlock(pocBndAttr.lock);
			break;
		}

		default:
		{
			break;
		}
	}
}

static void prvPocGuiBndTaskHandleSetPingTime(uint32_t id, uint32_t ctx)
{
	switch(id)
	{
		case LVPOCGUIBNDCOM_SIGNAL_PING_TIME_IND:
		{
			if(ctx == 0)
			{
				break;
			}

			osiMutexLock(pocBndAttr.lock);
			pocBndAttr.ping_time = (ctx/1000);
			pocBndAttr.set_ping_first = true;

			osiTimerStart(pocBndAttr.ping_timer, ctx);
			osiMutexUnlock(pocBndAttr.lock);
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
	m_BndUser.m_status = USER_OFFLINE;

	//bnd init
	broad_init();
	broad_log(0);//log open or close
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
			case LVPOCGUIBNDCOM_SIGNAL_SPEAK_TONE_START_IND:
			case LVPOCGUIBNDCOM_SIGNAL_SPEAK_TONE_STOP_IND:
			{
				prvPocGuiBndTaskHandleSpeak(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_QUERY_IND:
			case LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_QUERY_REP:
			case LVPOCGUIBNDCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND:
			case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND:
			case LVPOCGUIBNDCOM_SIGNAL_GROUP_LIST_REFRESH_BUF_REP:
			{
				prvPocGuiBndTaskHandleGroupList(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_IND:
			case LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REP:
			case LVPOCGUIBNDCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND:
			case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND:
			case LVPOCGUIBNDCOM_SIGNAL_MEMBER_LIST_QUERY_REFRESH_REP:
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

			case LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_IND:
			case LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_REP:
			case LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_GET_MEMBER_REP:
			case LVPOCGUIBNDCOM_SIGNAL_BIUILD_TEMPGRP_EXIT_REP:
			case LVPOCGUIBNDCOM_SIGNAL_REGISTER_BIUILD_TEMPGRP_CB_IND:
			case LVPOCGUIBNDCOM_SIGNAL_CANCEL_REGISTER_BIUILD_TEMPGRP_CB_IND:
			{
				prvPocGuiBndTaskHandleBuildTempGrp(event.param1, event.param2);
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
			case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_ERROR_REP:
			case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_EXIT_REP:
			case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_GET_MEMBER_CALL_IND:
			case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_SELECT_CALL_MODE:
			case LVPOCGUIBNDCOM_SIGNAL_SINGLE_CALL_STATUS_REGET_INFO_IND:
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

			case LVPOCGUIBNDCOM_SIGNAL_STOP_TIMEOUT_CHECK_ACK_IND:
			case LVPOCGUIBNDCOM_SIGNAL_VOICE_PLAY_START_IND:
			case LVPOCGUIBNDCOM_SIGNAL_VOICE_PLAY_STOP_IND:
			{
				prvPocGuiBndTaskHandleOther(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_GET_ERROR_CASE://获取错误原因
			{
				prvPocGuiBndTaskHandleErrorInfo(event.param1, event.param2);
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

			case LVPOCGUIBNDCOM_SIGNAL_PING_TIME_IND:
			{
				prvPocGuiBndTaskHandleSetPingTime(event.param1, event.param2);
				break;
			}

			case LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_ENTER:
			case LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_EXIT:
			case LVPOCGUIBNDCOM_SIGNAL_CALL_BRIGHT_SCREEN_BREAK:
			{
				prvPocGuiBndTaskHandleCallBrightScreen(event.param1, event.param2);
				break;
			}

			default:
			{
				OSI_PRINTFI("[gic](%s)(%d):receive a invalid event", __func__, __LINE__);
			}
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
	pocBndAttr.get_member_list_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_get_member_list_timer_cb, (void *)pocBndAttr.timer_user_data);
	pocBndAttr.get_group_list_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_get_group_list_timer_cb, NULL);
	pocBndAttr.try_login_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_try_login_timer_cb, NULL);//注册尝试登录定时器
	pocBndAttr.auto_login_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_auto_login_timer_cb, NULL);//注册自动登录定时器
	pocBndAttr.monitor_recorder_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_recorder_timer_cb, NULL);//检查是否有人在录音定时器
	pocBndAttr.Bnd_GroupUpdate_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_Group_update_timer_cb, NULL);//群组刷新定时器
	pocBndAttr.Bnd_MemberUpdate_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_Member_update_timer_cb, NULL);//成员刷新定时器
	pocBndAttr.check_ack_timeout_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_check_ack_timeout_timer_cb, NULL);
	pocBndAttr.ping_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_ping_timer_cb, NULL);
	pocBndAttr.Bnd_workpa_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_pa_timer_cb, NULL);
	pocBndAttr.BrightScreen_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_Call_Bright_Screen_timer_cb, NULL);
	pocBndAttr.checkrecoverylisten_timer = osiTimerCreate(pocBndAttr.thread, LvGuiBndCom_Check_Recovery_Listen_timer_cb, NULL);

	pocBndAttr.lock = osiMutexCreate();
}

static void lvPocGuiBndCom_send_data_callback(uint8_t * data, uint32_t length)
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

static void lvPocGuiBndCom_Receive_ListUpdate(lv_task_t * task)
{
	int type = (int)task->user_data;

	switch(type)
	{
		case LVPOCBNDCOM_MEMBER_ONOFFLINE_REP:
		{
			OSI_PRINTFI("[group][refresh](%s)(%d):rev refresh, lv to online or offline", __func__, __LINE__);
			lv_poc_activity_func_cb_set.member_list.refresh(NULL);
			break;
		}

		case LVPOCBNDCOM_MEMBERUPDATE_REP:
		{
			OSI_PRINTFI("[group][refresh](%s)(%d):rev refresh, lv to memberupdate", __func__, __LINE__);
			lv_poc_activity_func_cb_set.member_list.refresh_with_data(NULL);
			break;
		}

		case LVPOCBNDCOM_GROUPUPDATE_REP:
		{
			OSI_PRINTFI("[group][refresh](%s)(%d):rev refresh, lv to groupupdate", __func__, __LINE__);
			lv_poc_activity_func_cb_set.group_list.refresh_with_data(NULL);
			break;
		}
	}
}

static int lvPocGuiBndCom_try_get_memberinfo(bnd_gid_t gid, bnd_member_t* dst)
{
	int MemberNum = 0;
	int m_GNum = 0;

	MemberNum = broad_get_membercount(gid);
 	m_GNum = broad_get_memberlist(gid, dst, GUIBNDCOM_MEMBERMAX, 0, MemberNum);

	OSI_PRINTFI("[memberinf](%s)(%d):info'get, gid(%d), m_Gnum(%d)", __func__, __LINE__, gid, m_GNum);
	return m_GNum;
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
	pocBndAttr.timer_user_data = (void *)lv_mem_alloc(sizeof(void *));
	pocGuiBndComStart();
}

extern "C" bool lvPocGuiBndCom_Msg(LvPocGuiIdtCom_SignalType_t signal, void * ctx)
{
    if (pocBndAttr.thread == NULL || (signal != LVPOCGUIBNDCOM_SIGNAL_LOGIN_IND && pocBndAttr.isReady == false))
    {
	    return false;
    }

	if(signal < LVPOCGUIBNDCOM_SIGNAL_CIT_ENTER_IND
        && (lvPocGuiBndCom_cit_status(POC_TMPGRP_READ) == POC_CIT_ENTER))
    {
        OSI_PRINTFI("[msg](%s)(%d):error(%d)", __func__, __LINE__, lvPocGuiBndCom_cit_status(POC_TMPGRP_READ));
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

bool lvPocGuiBndCom_SemaphoreAcquire(void)//请求信号量
{
	if(pocBndAttr.sema_attr == true)
	{
		return false;
	}
	pocBndAttr.sema_attr = true;
	return true;
}

bool lvPocGuiBndCom_SemaphoreRelease(void)//释放信号量
{
	if(pocBndAttr.sema_attr == false)
	{
		return false;
	}
	pocBndAttr.sema_attr = false;

	return true;
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

    uint8_t i = 0;
    nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();

    for(i = 0; i < m_BndUser.m_Group.m_Group_Num; i++)
    {
        if(NULL != strstr((char *)m_BndUser.m_Group.m_Group[i].m_ucGNum, poc_config->curren_group_id))
        {
            pocBndAttr.current_group = i;
            strcpy(poc_config->curren_group_id, (char *)m_BndUser.m_Group.m_Group[i].m_ucGNum);
            break;
        }
    }
    lv_poc_setting_conf_write();

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

lv_poc_tmpgrp_t lvPocGuiBndCom_cit_status(lv_poc_tmpgrp_t status)
{
    if(status == POC_TMPGRP_READ)
    {
        return cit_status;
    }
    cit_status = status;
    return POC_TMPGRP_START;
}

#endif

