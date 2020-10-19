
#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include <stdlib.h>

static void poc_Led_Entry(void *param);
static void poc_Led_Jump_Entry(void *param);
static void lv_poc_led_status_all_close(void);
static void lv_poc_led_status_all_open(void);
static void lv_poc_red_open_green_close(void);
static void lv_poc_red_close_green_open(void);
static void callback_lv_poc_red_close_green_jump(osiEvent_t *event);
static void callback_lv_poc_green_close_red_jump(osiEvent_t *event);
static void callback_lv_poc_red_open_green_jump(osiEvent_t *event);
static void callback_lv_poc_green_open_red_jump(osiEvent_t *event);
static void callback_lv_poc_green_jump_red_jump(osiEvent_t *event);

static void lv_poc_led_status_callback(void *param);

static void lv_poc_led_status_callback_check(void *param);


typedef void (*lv_poc_led_jump_status)(osiEvent_t *event);
//回调函数
typedef struct _LED_CALLBACK_s
{
    lv_poc_led_jump_status		pf_poc_led_jump_status;//循环回调
}LV_POC_LED_CALLBACK;

typedef struct _PocLedIdtComAttr_t
{
	osiThread_t *thread;
	osiThread_t *jumpThread;
	bool        isReady;
	uint16_t    jumpperiod;
	uint16_t	jumpcount;
	bool        ledstatus;
	uint8_t 	before_status;/*记录上次运行状态*/
	uint16_t    before_jumpperiod;/*记录上次的运行周期*/
} PocLedIdtComAttr_t;

static PocLedIdtComAttr_t pocLedIdtAttr = {0};
static LV_POC_LED_CALLBACK Led_CallBack = {0};


/*
	  name : poc_Status_Led_Task
	  param : none
	  date : 2020-06-02
*/
void poc_Status_Led_Task(void)
{
	memset(&pocLedIdtAttr, 0, sizeof(PocLedIdtComAttr_t));

	pocLedIdtAttr.jumpperiod = 1000;//默认周期1s

	lv_poc_led_status_callback(NULL);//注销回调

	/*LED MSG TASK*/
	pocLedIdtAttr.thread = osiThreadCreate("status led", poc_Led_Entry, NULL, OSI_PRIORITY_LOW, 1024, 64);
	/*JUMP TASK*/
	pocLedIdtAttr.jumpThread = osiThreadCreate("jump led task", poc_Led_Jump_Entry, NULL, OSI_PRIORITY_LOW, 1024, 64);
}

/*
	  name : poc_Led_Entry
	  param : none
	  date : 2020-06-02
*/
static void poc_Led_Entry(void *param)
{
	osiEvent_t event;
	bool isValid = true;
	lv_poc_led_status_all_close();
	pocLedIdtAttr.isReady = true;
	while(1)
	{
		if(!osiEventTryWait(pocLedIdtAttr.thread , &event, 450))
		{
			continue;
		}
		if(event.id != 200)
		{
			continue;
		}

		if(event.param2)
			pocLedIdtAttr.jumpperiod = (uint16_t)event.param2;//提取周期
		if(event.param3)
			pocLedIdtAttr.jumpcount = (uint16_t)event.param3;//提取次数

		switch(event.param1)
		{
			case LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS:

				pocLedIdtAttr.jumpperiod = 1000;/*恢复闪烁线程周期*/
				lv_poc_led_status_all_close();
				Led_CallBack.pf_poc_led_jump_status = NULL;

				break;

			case LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS:
			case LVPOCLEDIDTCOM_SIGNAL_START_LISTEN_STATUS:

				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_red_close_green_jump;

				break;

			case LVPOCLEDIDTCOM_SIGNAL_LOGIN_SUCCESS_STATUS:
			case LVPOCLEDIDTCOM_SIGNAL_RUN_STATUS:
			{
				switch(pocLedIdtAttr.before_status)
				{
					case LVPOCLEDIDTCOM_SIGNAL_DISCHARGING_STATUS:
					{
						Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_red_close_green_jump;
						break;
					}
					case LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS:
					{
						lv_poc_red_open_green_close();
						break;
					}
					case LVPOCLEDIDTCOM_SIGNAL_CHARGING_COMPLETE_STATUS:
					{
						lv_poc_red_close_green_open();
						break;
					}
					case LVPOCLEDIDTCOM_SIGNAL_LOW_BATTERY_STATUS:
					{
						pocLedIdtAttr.jumpperiod = pocLedIdtAttr.before_jumpperiod;
						Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_green_close_red_jump;
						break;
					}
					default:
					{
						Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_red_close_green_jump;
						break;
					}
				}

				break;
			}

			case LVPOCLEDIDTCOM_SIGNAL_NO_SIM_STATUS:
			case LVPOCLEDIDTCOM_SIGNAL_LOW_BATTERY_STATUS:
			case LVPOCLEDIDTCOM_SIGNAL_NO_NETWORK_STATUS:

				pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_LOW_BATTERY_STATUS;
				pocLedIdtAttr.before_jumpperiod = pocLedIdtAttr.jumpperiod;
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_green_close_red_jump;

				break;

			case LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS:

				pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_NO_LOGIN_STATUS;
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_green_close_red_jump;
				break;

			case LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS:

				pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS;
				lv_poc_red_open_green_close();

				break;
			case LVPOCLEDIDTCOM_SIGNAL_DISCHARGING_STATUS:

				pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_DISCHARGING_STATUS;
				lv_poc_led_status_all_close();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_CHARGING_COMPLETE_STATUS:

				pocLedIdtAttr.before_status = LVPOCLEDIDTCOM_SIGNAL_CHARGING_COMPLETE_STATUS;
				lv_poc_red_close_green_open();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_CONNECT_NETWORK_STATUS:
			case LVPOCLEDIDTCOM_SIGNAL_MERMEBER_LIST_SUCCESS_STATUS:
			case LVPOCLEDIDTCOM_SIGNAL_GROUP_LIST_SUCCESS_STATUS:

				lv_poc_red_close_green_open();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_MERMEBER_LIST_FAIL_STATUS:
			case LVPOCLEDIDTCOM_SIGNAL_GROUP_LIST_FAIL_STATUS:

				lv_poc_red_open_green_close();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_FAIL_STATUS:

				lv_poc_led_status_all_open();
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_red_open_green_jump;
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_green_close_red_jump;
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_green_open_red_jump;
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_green_jump_red_jump;

				pocLedIdtAttr.isReady = false;
				osiThreadExit();
				return;

			default:
				isValid = false;
				break;
		}

		if (isValid)
		{
			osiEvent_t event_temp = {0};
			event_temp.param1 = pocLedIdtAttr.jumpperiod;
			osiEventTrySend(pocLedIdtAttr.jumpThread, &event_temp, 100);
		}

		isValid = true;
	}
}

/*
	  name : poc_Led_Entry
	  param : none
	  date : 2020-06-02
*/
static void poc_Led_Jump_Entry(void *param)
{
	osiEvent_t event = {0};

	while(1)
	{
		if(osiEventTryWait(pocLedIdtAttr.jumpThread , &event, pocLedIdtAttr.jumpperiod))
		{

		}
		else//查询回调
		{
			if(pocLedIdtAttr.jumpcount != 0 && pocLedIdtAttr.jumpperiod != 0)
			{
				if(pocLedIdtAttr.jumpcount <= 9) pocLedIdtAttr.jumpcount--;
				lv_poc_led_status_callback_check(Led_CallBack.pf_poc_led_jump_status);
			}
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
	return osiEventSend(pocLedIdtAttr.thread, &event);
}

/*
	  name : lv_poc_led_status_all_close
	  param : none
	  date : 2020-06-02
*/
static void
lv_poc_led_status_all_close(void)
{
	Led_CallBack.pf_poc_led_jump_status = NULL;
	poc_set_green_status(false);
	poc_set_red_status(false);
}

/*
	  name : lv_poc_led_status_all_open
	  param : none
	  date : 2020-06-02
*/
static void
lv_poc_led_status_all_open(void)
{
	Led_CallBack.pf_poc_led_jump_status = NULL;
	poc_set_green_status(true);
	poc_set_red_status(true);
}

/*
	  name : lv_poc_red_open_green_close
	  param : none
	  date : 2020-06-02
*/
static void
lv_poc_red_open_green_close(void)
{
	Led_CallBack.pf_poc_led_jump_status = NULL;
	poc_set_green_status(false);
	poc_set_red_status(true);
}

/*
	  name : lv_poc_red_close_green_open
	  param : none
	  date : 2020-06-02
*/
static void
lv_poc_red_close_green_open(void)
{
	Led_CallBack.pf_poc_led_jump_status = NULL;
	poc_set_green_status(true);
	poc_set_red_status(false);
}

/*
	  name : lv_poc_red_close_green_jump
	  param : none
	  date : 2020-06-02
*/
static void
callback_lv_poc_red_close_green_jump(osiEvent_t *event)
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
callback_lv_poc_red_open_green_jump(osiEvent_t *event)
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
callback_lv_poc_green_close_red_jump(osiEvent_t *event)
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
callback_lv_poc_green_open_red_jump(osiEvent_t *event)
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
callback_lv_poc_green_jump_red_jump(osiEvent_t *event)
{
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_red_status(pocLedIdtAttr.ledstatus);
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_green_status(pocLedIdtAttr.ledstatus);
}

/*
	  name : lv_poc_led_status_callback
  describe : 回调函数
	 param : none
	  date : 2020-06-02
*/
static void
lv_poc_led_status_callback(void *param)
{
    memset(&Led_CallBack, 0, sizeof(Led_CallBack));

    Led_CallBack.pf_poc_led_jump_status  = param;
}

/*
	  name : lv_poc_led_status_callback_check
  describe : 循环周期回调消息查询
	 param : none
	  date : 2020-06-02
*/
static void
lv_poc_led_status_callback_check(void *param)
{

	if(param != NULL)
	((lv_poc_led_jump_status)param)(NULL);//回调
}

#ifdef __cplusplus
}
#endif
