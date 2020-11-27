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

#ifndef _INCLUDE_GUIIDTCOM_H_
#define _INCLUDE_GUIIDTCOM_H_

#if 0
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "poc_audio_player.h"
#include "poc_audio_recorder.h"

#define FREERTOS
#define T_TINY_MODE

//组信息
class CGroup
{
public:
    CGroup();
    ~CGroup();
    int Reset();

public:
    UCHAR   m_ucGNum[32];     //组号码
    UCHAR   m_ucGName[64];    //组名字
    UCHAR   m_ucPriority;
};

//全部的组
class CAllGroup
{
public:
    CAllGroup();
    ~CAllGroup();

    int Reset();

public:
    CGroup  m_Group[USER_MAX_GROUP];//一个用户,最多处于32个组中
};




//IDT使用者
class CIdtUser
{
public:
    CIdtUser();

    ~CIdtUser();

    int Reset();

public:
    int m_iCallId;//全系统只有一路呼叫,可以这样简单一些
    int m_iRxCount, m_iTxCount;//统计收发语音包数量
    CAllGroup m_Group;
};

class LvPoc
{
public:

	IDT_CALLBACK_s CallBack;
	osiThread_t * thread;
	osiCallback_t thread_cb;
	POCAUDIOPLAYER_HANDLE player;
	POCAUDIORECORDER_HANDLE recorder;
	uint32_t lvPoc_id;
	osiEvent_t event_queue;
	uint32_t event_wait_timeout;
	static uint32_t lvPoc_num;
	bool isInit;
	bool isRunning;
	bool status;
	unsigned short status_cause;
	char * status_cause_str;
	uint8_t device_status;

	CIdtUser *m_IdtUser;

public:
	LvPoc();

	~LvPoc();

	bool init(bool log_enable, uint32_t playBufSize, uint32_t recorderBufSize);  //初始化

	bool Logging(void);

	bool Exit(void);

	bool log(void);

	void log(bool enable);

	bool startspeak(void);

	bool stopspeak(void);

	bool ReciveMsg(uint32_t signal, void * ctx);
};
#endif

#endif
