
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include <stdlib.h>
#include "uart3_gps.h"

static void lv_poc_led_entry(void *param);
static void lv_poc_led_status_all_close(lv_task_t *task);
static void lv_poc_led_status_all_open(lv_task_t *task);
static void lv_poc_red_open_green_close(lv_task_t *task);
static void lv_poc_red_close_green_open(lv_task_t *task);
static void callback_lv_poc_red_close_green_jump(void);
static void callback_lv_poc_green_close_red_jump(void);
static void callback_lv_poc_red_open_green_jump(void);
static void callback_lv_poc_green_open_red_jump(void);
static void callback_lv_poc_green_jump_red_jump(void);
static void Lv_Poc_Led_Status_Callback_Check(lv_task_t *task);
static void Lv_Poc_Led_Status_Task_Handle_Other(uint32_t id, uint32_t ctx1, uint32_t ctx2);

typedef void (*lv_poc_led_jump_status)(void);

typedef struct _PocLedIdtComAttr_t
{
	osiThread_t *thread;
	lv_task_t   *task;
	bool        isReady;
	uint16_t    jumpperiod;
	uint16_t	jumpcount;
	bool        ledstatus;
	uint8_t 	before_status;
	uint8_t 	last_status;
	uint16_t    before_jumpperiod;
	lv_poc_led_jump_status pledjump;
} PocLedIdtComAttr_t;

static PocLedIdtComAttr_t pocLedIdtAttr = {0};

/*
	  name : poc_Status_Led_Task
	  param : none
	  date : 2020-06-02
*/
void poc_Status_Led_Task(void)
{
	memset(&pocLedIdtAttr, 0, sizeof(PocLedIdtComAttr_t));
	pocLedIdtAttr.jumpperiod = 1000;
	pocLedIdtAttr.pledjump = NULL;

	pocLedIdtAttr.thread = osiThreadCreate("status led", lv_poc_led_entry, NULL, OSI_PRIORITY_LOW, 1024, 64);
}

/*
	  name : lv_poc_led_entry
	  param : none
	  date : 2020-06-02
*/
static void lv_poc_led_entry(void *param)
{
	osiEvent_t event = {0};
	pocLedIdtAttr.isReady = true;

	while(1)
	{
		if(osiEventWait(pocLedIdtAttr.thread, &event))
		{
			if(event.id != 200)
			{
				continue;
			}

			if(event.param1 <= LVPOCLEDIDTCOM_SIGNAL_STATUS_START
				|| event.param1 >= LVPOCLEDIDTCOM_SIGNAL_STATUS_END)
			{
				continue;
			}

			if(event.param1 == pocLedIdtAttr.last_status)
			{
				continue;
			}

			if(event.param2)
			{
				pocLedIdtAttr.jumpperiod = (uint16_t)event.param2;//提取周期
				if(pocLedIdtAttr.task != NULL)
				{
					lv_task_set_period(pocLedIdtAttr.task, pocLedIdtAttr.jumpperiod);
				}
			}

			if(event.param3)
				pocLedIdtAttr.jumpcount = (uint16_t)event.param3;//提取次数

			switch(event.param1)
			{
				case LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS:
				{
					pocLedIdtAttr.jumpperiod = 1000;
					if(pocLedIdtAttr.task == NULL)
					{
						pocLedIdtAttr.task = lv_task_create(Lv_Poc_Led_Status_Callback_Check, pocLedIdtAttr.jumpperiod, LV_TASK_PRIO_LOW, NULL);
					}
					lv_poc_refr_task_once(lv_poc_led_status_all_close, LVPOCLISTIDTCOM_LIST_PERIOD_100, LV_TASK_PRIO_LOW);
					break;
				}

				case LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS:
				case LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS:
				{
					pocLedIdtAttr.pledjump = callback_lv_poc_red_close_green_jump;
					pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS;
					break;
				}

				case LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS_STATUS:
				case LVPOCLEDIDTCOM_SIGNAL_RUN_STATUS:
				{
					switch(pocLedIdtAttr.before_status)
					{
						case LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS:
						{
							pocLedIdtAttr.pledjump = callback_lv_poc_red_close_green_jump;
							break;
						}
						case LVPOCLEDIDTCOM_SIGNAL_CHARGING_COMPLETE_STATUS:
						{
							lv_poc_refr_task_once(lv_poc_red_close_green_open, LVPOCLISTIDTCOM_LIST_PERIOD_100, LV_TASK_PRIO_LOW);
							break;
						}
						case LVPOCLEDIDTCOM_SIGNAL_LOW_BATTERY_STATUS:
						{
							pocLedIdtAttr.jumpperiod = pocLedIdtAttr.before_jumpperiod;
							pocLedIdtAttr.pledjump = callback_lv_poc_green_close_red_jump;
							break;
						}
						default:
						{
							pocLedIdtAttr.pledjump = callback_lv_poc_red_close_green_jump;
							break;
						}
					}
					break;
				}

				case LVPOCLEDIDTCOM_SIGNAL_NO_SIM_STATUS:
				case LVPOCLEDIDTCOM_SIGNAL_LOW_BATTERY_STATUS:
				case LVPOCLEDIDTCOM_SIGNAL_NO_NETWORK_STATUS:
				{
					pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_LOW_BATTERY_STATUS;
					pocLedIdtAttr.before_jumpperiod = pocLedIdtAttr.jumpperiod;
					pocLedIdtAttr.pledjump = callback_lv_poc_green_close_red_jump;
					break;
				}

				case LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS:
				{
					pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS;
					pocLedIdtAttr.pledjump = callback_lv_poc_green_close_red_jump;
					break;
				}

				case LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS:
				{
					pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS;
					lv_poc_refr_task_once(lv_poc_red_open_green_close, LVPOCLISTIDTCOM_LIST_PERIOD_100, LV_TASK_PRIO_LOW);
					break;
				}

				case LVPOCLEDIDTCOM_SIGNAL_CHARGING_COMPLETE_STATUS:
				{
					pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_CHARGING_COMPLETE_STATUS;
					lv_poc_refr_task_once(lv_poc_red_close_green_open, LVPOCLISTIDTCOM_LIST_PERIOD_100, LV_TASK_PRIO_LOW);
					break;
				}

				case LVPOCLEDIDTCOM_SIGNAL_CONNECT_NETWORK_STATUS:
				case LVPOCLEDIDTCOM_SIGNAL_MERMEBER_LIST_SUCCESS_STATUS:
				case LVPOCLEDIDTCOM_SIGNAL_GROUP_LIST_SUCCESS_STATUS:
				{
					lv_poc_refr_task_once(lv_poc_red_close_green_open, LVPOCLISTIDTCOM_LIST_PERIOD_100, LV_TASK_PRIO_LOW);
					break;
				}

				case LVPOCLEDIDTCOM_SIGNAL_MERMEBER_LIST_FAIL_STATUS:
				case LVPOCLEDIDTCOM_SIGNAL_GROUP_LIST_FAIL_STATUS:
				{
					lv_poc_refr_task_once(lv_poc_red_open_green_close, LVPOCLISTIDTCOM_LIST_PERIOD_100, LV_TASK_PRIO_LOW);
					break;
				}

				case LVPOCLEDIDTCOM_SIGNAL_FAIL_STATUS:
				{
					return;
				}

				case LVPOCGUIIDTCOM_SIGNAL_GPS_SUSPEND_IND:
				case LVPOCGUIIDTCOM_SIGNAL_GPS_RESUME_IND:
				case LVPOCGUIIDTCOM_SIGNAL_TURN_OFF_SCREEN_IND:
	            case LVPOCGUIIDTCOM_SIGNAL_TURN_ON_SCREEN_IND:
				{
					Lv_Poc_Led_Status_Task_Handle_Other(event.param1, event.param2, event.param3);
					break;
				}

				default:
				{
					lv_poc_refr_task_once(lv_poc_led_status_all_open, LVPOCLISTIDTCOM_LIST_PERIOD_100, LV_TASK_PRIO_LOW);
					pocLedIdtAttr.pledjump = callback_lv_poc_red_open_green_jump;
					pocLedIdtAttr.pledjump = callback_lv_poc_green_close_red_jump;
					pocLedIdtAttr.pledjump = callback_lv_poc_green_open_red_jump;
					pocLedIdtAttr.pledjump = callback_lv_poc_green_jump_red_jump;
					pocLedIdtAttr.isReady = false;
					osiThreadExit();
					break;
				}
			}
			pocLedIdtAttr.last_status = event.param1;
		}
	}
}

/*
	  name : lvPocLedIdtCom_Msg
	  param : none
	  date : 2020-06-02
*/
bool
lvPocLedIdtCom_Msg(LVPOCIDTCOM_Led_SignalType_t signal, LVPOCIDTCOM_Led_Period_t ctx, LVPOCIDTCOM_Led_Jump_Count_t count)
{
	if (pocLedIdtAttr.thread == NULL || pocLedIdtAttr.isReady == false)
	{
		return false;
	}

	osiEvent_t event = {0};
	memset(&event, 0, sizeof(osiEvent_t));
	event.id = 200;
	event.param1 = signal;
	event.param2 = (LVPOCIDTCOM_Led_Period_t)ctx;
	event.param3 = (LVPOCIDTCOM_Led_Jump_Count_t)count;
	return osiEventTrySend(pocLedIdtAttr.thread, &event, 500);
}

/*
	  name : lv_poc_led_status_all_close
	  param : none
	  date : 2020-06-02
*/
static void
lv_poc_led_status_all_close(lv_task_t *task)
{
	pocLedIdtAttr.pledjump = NULL;
	poc_set_green_status(false);
	poc_set_red_status(false);
}

/*
	  name : lv_poc_led_status_all_open
	  param : none
	  date : 2020-06-02
*/
static void
lv_poc_led_status_all_open(lv_task_t *task)
{
	pocLedIdtAttr.pledjump = NULL;
	poc_set_green_status(true);
	poc_set_red_status(true);
}

/*
	  name : lv_poc_red_open_green_close
	  param : none
	  date : 2020-06-02
*/
static void
lv_poc_red_open_green_close(lv_task_t *task)
{
	pocLedIdtAttr.pledjump = NULL;
	poc_set_green_status(false);
	poc_set_red_status(true);
}

/*
	  name : lv_poc_red_close_green_open
	  param : none
	  date : 2020-06-02
*/
static void
lv_poc_red_close_green_open(lv_task_t *task)
{
	pocLedIdtAttr.pledjump = NULL;
	poc_set_green_status(true);
	poc_set_red_status(false);
}

/*
	  name : lv_poc_red_close_green_jump
	  param : none
	  date : 2020-06-02
*/
static void
callback_lv_poc_red_close_green_jump(void)
{
	poc_set_red_status(false);
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_green_status(pocLedIdtAttr.ledstatus);
}

/*
	  name : lv_poc_red_open_green_jump
	  param : none
	  date : 2020-06-02
*/
static void
callback_lv_poc_red_open_green_jump(void)
{
	poc_set_red_status(true);
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_green_status(pocLedIdtAttr.ledstatus);
}

/*
	  name : lv_poc_green_close_red_jump
	  param : none
	  date : 2020-06-02
*/
static void
callback_lv_poc_green_close_red_jump(void)
{
	poc_set_green_status(false);
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_red_status(pocLedIdtAttr.ledstatus);
}

/*
	  name : lv_poc_green_open_red_jump
	  param : none
	  date : 2020-06-02
*/
static void
callback_lv_poc_green_open_red_jump(void)
{
	poc_set_green_status(true);
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_red_status(pocLedIdtAttr.ledstatus);
}

/*
	  name : lv_poc_green_jump_red_jump
  describe : 红绿灯交替跳变
	 param : none
	  date : 2020-06-02
*/
static void
callback_lv_poc_green_jump_red_jump(void)
{
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_red_status(pocLedIdtAttr.ledstatus);
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_green_status(pocLedIdtAttr.ledstatus);
}

/*
	  name : Lv_Poc_Led_Status_Callback_Check
  describe : 循环周期回调消息查询
	 param : none
	  date : 2020-06-02
*/
static void
Lv_Poc_Led_Status_Callback_Check(lv_task_t *task)
{
	if(pocLedIdtAttr.jumpcount == 0 || pocLedIdtAttr.pledjump == NULL)
	{
		return;
	}
	else if(pocLedIdtAttr.jumpcount <= 9)
	{
		pocLedIdtAttr.jumpcount--;
	}

	if(pocLedIdtAttr.pledjump != NULL)
	{
		(pocLedIdtAttr.pledjump)();//回调
	}
}

/*
	  name : Lv_Poc_Led_Status_Task_Handle_Other
  describe : other
	 param : none
	  date : 2020-10-24
*/
static void
Lv_Poc_Led_Status_Task_Handle_Other(uint32_t id, uint32_t ctx1, uint32_t ctx2)
{
	switch(id)
	{
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
		case LVPOCGUIIDTCOM_SIGNAL_GPS_SUSPEND_IND:
		{
			publvPocGpsIdtComSleep();
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_GPS_RESUME_IND:
		{
			publvPocGpsIdtComWake();
			break;
		}
#endif
		case LVPOCGUIIDTCOM_SIGNAL_TURN_OFF_SCREEN_IND:
		{
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
			lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_CLOSE_GPS_REPORT, NULL);
#endif
			lv_poc_notation_del();
			break;
		}

		case LVPOCGUIIDTCOM_SIGNAL_TURN_ON_SCREEN_IND:
		{
#ifdef CONFIG_POC_GUI_GPS_SUPPORT
			if(!pubPocIdtGpsLocationStatus()
				&& pubPocIdtIsHaveExistGps())
			{
				publvPocGpsIdtComWake();
			}
			lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_OPEN_GPS_REPORT, NULL);
#endif
			lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SCREEN_ON_IND, NULL);
			break;
		}
	}

}

#ifdef __cplusplus
}
#endif
