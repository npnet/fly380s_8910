﻿
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include <stdlib.h>
#include "uart3_gps.h"
#include "poc_audio_player.h"
#include "poc_audio_recorder.h"

/*************************************************
*
*                  EXTERN
*
*************************************************/
extern int lv_poc_cit_get_run_status(void);

/*************************************************
*
*                  STATIC
*
*************************************************/
static void lv_poc_led_entry(void *param);
static void lv_poc_led_register(void);
static void lv_poc_led_status_all_close(void);
static void lv_poc_led_status_all_open(void);
static void lv_poc_red_open_green_close(void);
static void lv_poc_red_close_green_open(void);
static void callback_lv_poc_red_close_green_jump(void);
static void callback_lv_poc_green_close_red_jump(void);
static void callback_lv_poc_red_open_green_jump(void);
static void callback_lv_poc_green_open_red_jump(void);
static void callback_lv_poc_green_jump_red_jump(void);
static void lv_poc_led_status_callback_check(lv_task_t *task);
static void lv_poc_led_status_task_handle_other(uint32_t id);

typedef void (*poc_led_cb)(void);

typedef struct _LedAttr_t
{
	int  priority;
	int  id;
	bool valid;
	int  openms;//100的倍数
	int  closems;//100的倍数
	int  count;
	poc_led_cb cb;
	char 		*str;
}LedAttr_t;

typedef struct _PocLedComAttr_t
{
    osiThread_t *thread;
    lv_task_t   *task;
    bool        isReady;
    bool        ledstatus;
	bool        ishavemsg;
	bool        isledbright;
	uint8_t 	count;
	uint8_t 	openindex;
	uint8_t 	closeindex;
} PocLedComAttr_t;

static char *ledstr[LVPOCLEDIDTCOM_SIGNAL_STATUS_END] =
{
	"REGISTER_STATUS",
	"POWEROFF_STATUS",
	"SCAN_NETWORK_STATUS",
	"IDLE_STATUS",
	"SETTING_STATUS",
	"CHARGING_STATUS",
	"CHARGING_COMPLETE_STATUS",
	"LOW_BATTERY_STATUS",
	"START_TALK_STATUS",
	"START_LISTEN_STATUS",
	"NO_SIM_STATUS",
	"NO_LOGIN_STATUS",
	"MAX",

	"GPS_SUSPEND_IND",
	"GPS_RESUME_IND",
	"TURN_OFF_SCREEN_IND",
	"TURN_ON_SCREEN_IND",
	"PING_SUCCESS_IND",
	"PING_FAILED_IND",
    "HEADSET_INSERT",
	"HEADSET_PULL_OUT",

	"LOOPBACK_RECORDER_IND",
    "LOOPBACK_PLAYER_IND",
    "TEST_VLOUM_PLAY_IND",
};

//led registry
static LedAttr_t ledattr[LVPOCGUIIDTCOM_SIGNAL_MAX] = {

	{0, LVPOCLEDIDTCOM_SIGNAL_REGISTER_STATUS, false, 0, 0, LVPOCLEDIDTCOM_SIGNAL_JUMP_1, lv_poc_led_register, "register"},
	//deal pri
	{1, LVPOCLEDIDTCOM_SIGNAL_NO_SIM_STATUS, false, 500, 500, LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER, callback_lv_poc_green_close_red_jump, "no sim"},
	{2, LVPOCLEDIDTCOM_SIGNAL_SCAN_NETWORK_STATUS, false, 500, 500, LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER, callback_lv_poc_red_close_green_jump, "scan network"},
	{3, LVPOCLEDIDTCOM_SIGNAL_NO_NETWORK_STATUS, false, 500, 800, LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER, callback_lv_poc_green_close_red_jump, "no network"},
	{4, LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS, false, 1500, 1500, LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER, callback_lv_poc_green_close_red_jump, "no login"},
	{5, LVPOCLEDIDTCOM_SIGNAL_SETTING_STATUS, false, 0, 0, LVPOCLEDIDTCOM_SIGNAL_JUMP_1, lv_poc_red_close_green_open, "setting"},
	{6, LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS, false, 500, 500, LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER, callback_lv_poc_red_close_green_jump, "listen"},
	{7, LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS, false, 500, 500, LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER, callback_lv_poc_red_close_green_jump, "speak"},
	{8, LVPOCLEDIDTCOM_SIGNAL_CHARGING_COMPLETE_STATUS, false, 0, 0, LVPOCLEDIDTCOM_SIGNAL_JUMP_1, lv_poc_red_close_green_open, "charge complete"},
	{9, LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS, false, 0, 0, LVPOCLEDIDTCOM_SIGNAL_JUMP_1, lv_poc_red_open_green_close, "charging"},
	{10, LVPOCLEDIDTCOM_SIGNAL_LOW_BATTERY_STATUS, false, 500, 500, LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER, callback_lv_poc_green_close_red_jump, "low battery"},
	{11, LVPOCLEDIDTCOM_SIGNAL_IDLE_STATUS, false, 500, 3000, LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER, callback_lv_poc_red_close_green_jump, "idle"},
};

static PocLedComAttr_t pocLedAttr = {0};

/*
      name : poc_status_led_task
      param : none
      date : 2021-01-04
*/
void poc_status_led_task(void)
{
    memset(&pocLedAttr, 0, sizeof(PocLedComAttr_t));
    pocLedAttr.thread = osiThreadCreate("status led", lv_poc_led_entry, NULL, OSI_PRIORITY_NORMAL, 1024, 64);
}

/*
      name : lv_poc_led_entry
      param : none
      date : 2021-01-04
*/
static void lv_poc_led_entry(void *param)
{
    osiEvent_t event = {0};
    pocLedAttr.isReady = true;

    while(1)
    {
        if(osiEventWait(pocLedAttr.thread, &event))
        {
			OSI_PRINTFI("[LED](%s)(%d):rec msg, status(%s)", __func__, __LINE__, event.param1 > 0 ? ledstr[event.param1 - 1] : "error");
            if(event.id != 200)
            {
                continue;
            }

            if(event.param1 <= LVPOCLEDIDTCOM_SIGNAL_STATUS_START
                || event.param1 >= LVPOCLEDIDTCOM_SIGNAL_STATUS_END)
            {
                continue;
            }

			if(event.param1 == LVPOCLEDIDTCOM_SIGNAL_REGISTER_STATUS)
			{
				lv_poc_led_register();
				continue;
			}

			if(event.param1 < LVPOCGUIIDTCOM_SIGNAL_MAX)
			{
				int i;
				for(i = 1; i < LVPOCGUIIDTCOM_SIGNAL_MAX; i++)//scan register or cannel register
				{
					if(ledattr[i].id == event.param1)
					{
						OSI_PRINTFI("[LED](%s)(%d):reset task, status(%s), name(%s)", __func__, __LINE__, event.param2 ? "register" : "cannel register", ledattr[i].str);
						if(event.param2)
						{
							pocLedAttr.task ? lv_task_reset(pocLedAttr.task) : 0;
							lv_poc_led_status_all_close();
							pocLedAttr.count = ledattr[i].count;//valid
							pocLedAttr.ishavemsg = true;
							pocLedAttr.ledstatus = false;
						}
						ledattr[i].valid = event.param2;//register or cannel register
						break;
					}
				}

				if(event.param2)//register
				{
					continue;
				}
				//cancel register, scan valid count
				for(i = 1; i < LVPOCGUIIDTCOM_SIGNAL_MAX; i++)
				{
					if(ledattr[i].valid)
					{
						pocLedAttr.task ? lv_task_reset(pocLedAttr.task) : 0;
						lv_poc_led_status_all_close();
						pocLedAttr.count = ledattr[i].count;//valid
						pocLedAttr.ishavemsg = true;
						pocLedAttr.ledstatus = false;
						break;
					}
				}
				continue;
			}

            switch(event.param1)
            {
                case LVPOCGUIIDTCOM_SIGNAL_GPS_SUSPEND_IND:
                case LVPOCGUIIDTCOM_SIGNAL_GPS_RESUME_IND:
                case LVPOCGUIIDTCOM_SIGNAL_TURN_OFF_SCREEN_IND:
                case LVPOCGUIIDTCOM_SIGNAL_TURN_ON_SCREEN_IND:
				case LVPOCGUICOM_SIGNAL_LOOPBACK_RECORDER_IND:
				case LVPOCGUICOM_SIGNAL_LOOPBACK_PLAYER_IND:
				case LVPOCGUICOM_SIGNAL_TEST_VLOUM_PLAY_IND:
				case LVPOCGUIOEMCOM_SIGNAL_HEADSET_INSERT:
				case LVPOCGUIOEMCOM_SIGNAL_HEADSET_PULL_OUT:
                {
                    lv_poc_led_status_task_handle_other(event.param1);
                    break;
                }

                default:
                {
					lv_poc_led_status_all_open();
					lv_poc_red_open_green_close();
					lv_poc_red_close_green_open();
					callback_lv_poc_red_close_green_jump();
					callback_lv_poc_green_close_red_jump();
					callback_lv_poc_red_open_green_jump();
					callback_lv_poc_green_open_red_jump();
					callback_lv_poc_green_jump_red_jump();
					lv_poc_led_status_all_close();
                    pocLedAttr.isReady = false;
                    osiThreadExit();
                    break;
                }
            }
        }
    }
}

/*
      name : lv_poc_led_register
      param : none
      date : 2021-01-04
*/
static void
lv_poc_led_register(void)
{
    if(pocLedAttr.task == NULL)
    {
        pocLedAttr.task = lv_task_create(lv_poc_led_status_callback_check, 100, LV_TASK_PRIO_MID, NULL);
    }
    lv_poc_led_status_all_close();
}

/*
      name : lv_poc_led_status_all_close
      param : none
      date : 2021-01-04
*/
static void
lv_poc_led_status_all_close(void)
{
	poc_set_green_status(false);
	poc_set_red_status(false);
}

/*
      name : lv_poc_led_status_all_open
      param : none
      date : 2021-01-04
*/
static void
lv_poc_led_status_all_open(void)
{
	poc_set_green_status(true);
	poc_set_red_status(true);
}

/*
      name : lv_poc_red_open_green_close
      param : none
      date : 2021-01-04
*/
static void
lv_poc_red_open_green_close(void)
{
	poc_set_green_status(false);
	poc_set_red_status(true);
}

/*
      name : lv_poc_red_close_green_open
      param : none
      date : 2021-01-04
*/
static void
lv_poc_red_close_green_open(void)
{
	poc_set_green_status(true);
	poc_set_red_status(false);
}

/*
      name : lv_poc_red_close_green_jump
      param : none
      date : 2021-01-04
*/
static void
callback_lv_poc_red_close_green_jump(void)
{
	poc_set_red_status(false);
    pocLedAttr.ledstatus = ! pocLedAttr.ledstatus;
	poc_set_green_status(pocLedAttr.ledstatus);
}

/*
      name : lv_poc_red_open_green_jump
      param : none
      date : 2021-01-04
*/
static void
callback_lv_poc_red_open_green_jump(void)
{
	poc_set_red_status(true);
    pocLedAttr.ledstatus = ! pocLedAttr.ledstatus;
	poc_set_green_status(pocLedAttr.ledstatus);
}

/*
      name : lv_poc_green_close_red_jump
      param : none
      date : 2021-01-04
*/
static void
callback_lv_poc_green_close_red_jump(void)
{
	poc_set_green_status(false);
    pocLedAttr.ledstatus = ! pocLedAttr.ledstatus;
	poc_set_red_status(pocLedAttr.ledstatus);
}

/*
      name : lv_poc_green_open_red_jump
      param : none
      date : 2021-01-04
*/
static void
callback_lv_poc_green_open_red_jump(void)
{
	poc_set_green_status(true);
    pocLedAttr.ledstatus = ! pocLedAttr.ledstatus;
	poc_set_red_status(pocLedAttr.ledstatus);
}

/*
      name : lv_poc_green_jump_red_jump
  describe : 红绿灯交替跳变
     param : none
      date : 2021-01-04
*/
static void
callback_lv_poc_green_jump_red_jump(void)
{
    pocLedAttr.ledstatus = ! pocLedAttr.ledstatus;
	poc_set_red_status(pocLedAttr.ledstatus);
	poc_set_green_status(pocLedAttr.ledstatus);
}

/*
      name : lv_poc_led_status_callback_check
     param : none
      date : 2021-01-04
*/
static void
lv_poc_led_status_callback_check(lv_task_t *task)
{
    if(lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_RGB)
    {
        return;
    }

	if(pocLedAttr.count == 0)
	{
		//OSI_PRINTFI("[LED](%s)(%d):count is 0", __func__, __LINE__);
		return;
	}

	for(int i = 0; i < LVPOCGUIIDTCOM_SIGNAL_MAX; i++)
	{
		if(ledattr[i].valid == true)
		{
			if(pocLedAttr.ishavemsg)
			{
				OSI_PRINTFI("[LED][cb](%s)(%d):current run state(%s)", __func__, __LINE__, ledattr[i].str);
				pocLedAttr.ishavemsg = false;
				pocLedAttr.openindex = 0;
				pocLedAttr.closeindex = 0;
				pocLedAttr.isledbright = true;
			}

			if(ledattr[i].openms == 0
				|| ledattr[i].closems == 0)//once need cancel
			{
				pocLedAttr.count--;
				ledattr[i].cb ? ledattr[i].cb() : 0;
				return;
			}

			if(pocLedAttr.isledbright)
			{
				pocLedAttr.openindex++;
				if(ledattr[i].closems/100 <= pocLedAttr.openindex)
				{
					pocLedAttr.isledbright = false;
					pocLedAttr.count < LVPOCLEDIDTCOM_SIGNAL_JUMP_FOREVER ? (pocLedAttr.count--) : 0;
					pocLedAttr.openindex = 0;
					ledattr[i].cb ? ledattr[i].cb() : 0;
				}
			}
			else
			{
				pocLedAttr.closeindex++;
				if(ledattr[i].openms/100 <= pocLedAttr.closeindex)
				{
					pocLedAttr.isledbright = true;
					pocLedAttr.closeindex = 0;
					ledattr[i].cb ? ledattr[i].cb() : 0;
				}
			}
			break;
		}
	}
}

/*
      name : lv_poc_led_status_task_handle_other
  describe : other
     param : none
      date : 2020-10-24
*/
static void
lv_poc_led_status_task_handle_other(uint32_t id)
{
    switch(id)
    {
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
        case LVPOCGUIIDTCOM_SIGNAL_GPS_SUSPEND_IND:
        {
            lv_poc_set_auto_deepsleep(true);
            publvPocGpsIdtComSleep();
            break;
        }

        case LVPOCGUIIDTCOM_SIGNAL_GPS_RESUME_IND:
        {
            lv_poc_set_auto_deepsleep(false);
            publvPocGpsIdtComWake();
            break;
        }
#endif
        case LVPOCGUIIDTCOM_SIGNAL_TURN_OFF_SCREEN_IND:
        {
            break;
        }

        case LVPOCGUIIDTCOM_SIGNAL_TURN_ON_SCREEN_IND:
        {
            break;
        }

		case LVPOCGUICOM_SIGNAL_LOOPBACK_RECORDER_IND:
		{
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始录音", (const uint8_t *)"");
			lv_poc_start_recordwriter();
			break;
		}

		case LVPOCGUICOM_SIGNAL_LOOPBACK_PLAYER_IND:
		{
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"开始播放", (const uint8_t *)"");
			lv_poc_start_playfile();
			break;
		}

		case LVPOCGUICOM_SIGNAL_TEST_VLOUM_PLAY_IND:
		{
			poc_play_voice_one_time(LVPOCAUDIO_Type_Test_Volum, 50, false);
			break;
		}

		case LVPOCGUIOEMCOM_SIGNAL_HEADSET_INSERT:
	    {
		   if(!lvPocGuiIdtCom_get_listen_status()
			  && !lvPocGuiIdtCom_get_speak_status())
		   {
			  lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			  lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"耳机插入", (const uint8_t *)"");
		   }
		   break;
	    }

	    case LVPOCGUIOEMCOM_SIGNAL_HEADSET_PULL_OUT:
	    {
		   if(!lvPocGuiIdtCom_get_listen_status()
			  && !lvPocGuiIdtCom_get_speak_status())
		   {
			  lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_DESTORY, NULL, NULL);
			  lv_poc_activity_func_cb_set.window_note(LV_POC_NOTATION_NORMAL_MSG, (const uint8_t *)"耳机拔出", (const uint8_t *)"");
		   }
		   break;
		}
    }
}

/*
	  name : lvPocLedCom_Msg
	  param : none
	  date : 2021-01-04
*/
bool
lvPocLedCom_Msg(LVPOCIDTCOM_Led_SignalType_t signal, bool valid)
{
	if (pocLedAttr.thread == NULL || pocLedAttr.isReady == false)
	{
		return false;
	}

	if(lv_poc_cit_get_run_status() == LV_POC_CIT_OPRATOR_TYPE_RGB)
	{
		OSI_LOGI(0, "[LED][EVENT][CIT]test rgb");
		return false;
	}

	if(signal == LVPOCLEDIDTCOM_SIGNAL_POWEROFF_STATUS)
	{
		pocLedAttr.task ? lv_task_del(pocLedAttr.task) : 0;
		lv_poc_red_open_green_close();
		return true;
	}

	osiEvent_t event = {0};
	memset(&event, 0, sizeof(osiEvent_t));
	event.id = 200;
	event.param1 = signal;
	event.param2 = valid;

	return osiEventSend(pocLedAttr.thread, &event);
}

#ifdef __cplusplus
}
#endif
