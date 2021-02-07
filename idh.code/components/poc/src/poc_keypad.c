﻿/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
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

// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "poc_keypad.h"
#ifdef CONFIG_POC_KEYPAD_SUPPORT

#include "osi_log.h"
#include "osi_api.h"
#include "osi_pipe.h"
#include "ml.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc_lib.h"
#include "guiCtelCom_api.h"
#include "lv_include/lv_poc.h"
#include "lv_gui_main.h"
#include "poc_audio_recorder.h"
#include "drv_keypad.h"

#define POC_RECORD_OR_SPEAK_CALL 1//1-正常对讲，0-自录自播
#define POC_RECORDER_PLAY_MODE 1//1-play 0-poc

static lv_indev_state_t preKeyState = 0xff;
static uint32_t   preKey      = 0xff;
static lv_indev_state_t prvPttKeyState = 0xff;

//extern
extern lv_poc_activity_t * poc_member_call_activity;
extern int lv_poc_cit_get_run_status(void);

static lv_indev_state_t prvPowerKeyState = 0xff;
static bool isReadyPowerOff = false;
static bool *isReadyCtnKeyState = NULL;
static osiTimer_t * prvPowerTimer = NULL;
static osiTimer_t * prvPttKeyStateTimer = NULL;
static bool pttKeyStatus = false;
static bool prvPttKeyStateTimerRunning = false;
static void poc_power_on_charge_set_lcd_status(uint8_t lcdstatus);

static void prvPowerKeyCb(void *ctx)
{
	isReadyPowerOff = true;
	if(!lv_poc_charge_poweron_status())//正常开机
	{
		OSI_LOGI(0, "[poc][ctnkey](%d):state(%d)", __LINE__, *isReadyCtnKeyState);
		if(*isReadyCtnKeyState == true)
		{
			return;
		}

		lv_poc_refr_task_once(lv_poc_shutdown_note_activity_open,
			LVPOCLISTIDTCOM_LIST_PERIOD_10, LV_TASK_PRIO_HIGH);
	}
	else//充电开机
	{
		osiSetBootCause(OSI_BOOTCAUSE_PWRKEY);/*as reboot*/
		osiShutdown(OSI_SHUTDOWN_RESET);//重启设备
	}
}

static void prvPttKeyStateCb(void *ctx)
{
	if(drvKeypadState(5)) // ptt key's id is 5
	{
		OSI_PRINTFI("[poc][voice][%s](%d)Ptt key is press", __func__, __LINE__);
		osiTimerStart(prvPttKeyStateTimer, 2000);
	}
	else
	{
		OSI_PRINTFI("[poc][voice][%s](%d)Ptt key is release", __func__, __LINE__);
		if(prvPttKeyStateTimerRunning)
		{
			osiTimerStop(prvPttKeyStateTimer);
			prvPttKeyStateTimerRunning = false;
		}
		lv_poc_get_loopback_recordplay_status() ? lvPocLedCom_Msg(LVPOCGUICOM_SIGNAL_LOOPBACK_PLAYER_IND, false) : lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_IND, NULL);
		poc_set_ptt_key_status(false);
		prvPttKeyState = false;
	}
}

bool pocKeypadHandle(uint32_t id, lv_indev_state_t state, void *p)
{
	bool ret = false;
	isReadyCtnKeyState = (bool *)p;
	*isReadyCtnKeyState ? (osiTimerIsRunning(prvPowerTimer) ? osiTimerStop(prvPowerTimer) : 0) : 0;

	if(id == LV_GROUP_KEY_POC) //poc
	{
		if((prvPttKeyState != state)
			&& (!lvPocGuiCtelCom_get_listen_status()))
		{
			if(state == LV_INDEV_STATE_PR)
			{
				if(prvPttKeyStateTimer == NULL)
				{
					prvPttKeyStateTimer = osiTimerCreate(NULL, prvPttKeyStateCb, NULL);
				}

				if(!prvPttKeyStateTimerRunning)
				{
					osiTimerStart(prvPttKeyStateTimer, 2000);
					prvPttKeyStateTimerRunning = true;

					poc_set_ptt_key_status(true);
					prvPttKeyState = true;
				}

				switch(lv_poc_get_apply_note())
				{
					case POC_APPLY_NOTE_TYPE_NONETWORK:
					{
						lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"无网络连接", (const uint8_t *)"");
						break;
					}

					case POC_APPLY_NOTE_TYPE_NOLOGIN:
					case POC_APPLY_NOTE_TYPE_LOGINSUCCESS:
					{
						lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_KEY ? lv_poc_type_key_poc_cb(true) : (lv_poc_get_loopback_recordplay_status() ? lvPocLedCom_Msg(LVPOCGUICOM_SIGNAL_LOOPBACK_RECORDER_IND, false) : lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SPEAK_START_IND, NULL));
						break;
					}
				}
            }
            else
            {
				lv_poc_get_loopback_recordplay_status() ? lvPocLedCom_Msg(LVPOCGUICOM_SIGNAL_LOOPBACK_PLAYER_IND, false) : lvPocGuiCtelCom_Msg(LVPOCGUICTELCOM_SIGNAL_SPEAK_STOP_IND, NULL);

				if(prvPttKeyStateTimerRunning)
				{
					osiTimerStop(prvPttKeyStateTimer);
					prvPttKeyStateTimerRunning = false;
				}

				poc_set_ptt_key_status(false);
				prvPttKeyState = false;
			}
		}

		ret = false;
	}
	else if(id == 0xf0)//power key
	{
		if(prvPowerTimer == NULL)
		{
			prvPowerTimer = osiTimerCreate(NULL, prvPowerKeyCb, NULL);
		}

        if(prvPowerKeyState != state)
        {
            if(state == LV_INDEV_STATE_PR)
            {
				if(lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_KEY)
				{
					lv_poc_type_key_power_cb(true);
					return false;
				}

                if(prvPowerTimer != NULL)
                {
					osiTimerStart(prvPowerTimer, LONGPRESS_SHUTDOWN_TIME);
                    isReadyPowerOff = false;
                }
            }
            else
            {
                if(prvPowerTimer == NULL)
				{
					return false;
                }
	            if (!isReadyPowerOff)
	            {
					osiTimerStop(prvPowerTimer);
					if(lv_poc_charge_poweron_status())
					{
						poc_power_on_charge_set_lcd_status(!poc_get_lcd_status());
					}
					else
					{
						poc_set_lcd_status(!poc_get_lcd_status());
					}
	            }
			}
		}
		prvPowerKeyState = state;
		ret = true;
	}
	preKey = id;
	preKeyState = state;
	return ret;
}

bool pocGetPttKeyState(void)
{
	return prvPttKeyState == KEY_STATE_PRESS ? true : false;
}

static
void poc_power_on_charge_set_lcd_status(uint8_t lcdstatus)
{

	if(lcdstatus != 0)
	{
		lvGuiChargeScreenOn();
	}
	else
	{
		lvGuiScreenOff();
	}

}

void poc_set_ptt_key_status(bool keyStatus)
{
	pttKeyStatus = keyStatus;
}

bool poc_get_ptt_key_status(void)
{
	return pttKeyStatus;
}

#endif
