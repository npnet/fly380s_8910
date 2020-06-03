
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
	bool        isReady;
	uint16_t    jumpperiod;
	bool        ledstatus;
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
	pocLedIdtAttr.thread=osiThreadCreate("status led", poc_Led_Entry, NULL, OSI_PRIORITY_LOW, 1024, 64);
	/*JUMP TASK*/
	osiThreadCreate("jump led task", poc_Led_Jump_Entry, NULL, OSI_PRIORITY_LOW, 1024, 64);
}

/*
	  name : poc_Led_Entry
	  param : none
	  date : 2020-06-02
*/
static void poc_Led_Entry(void *param)
{
	osiEvent_t event;
	lv_poc_led_status_all_close();

	pocLedIdtAttr.isReady = true;
	while(1)
	{
		if(!osiEventTryWait(pocLedIdtAttr.thread , &event, 200))
		{
			continue;
		}
		if(event.id != 200)
		{
			continue;
		}

		if(event.param2)
			pocLedIdtAttr.jumpperiod = (uint16_t)event.param2;//提取周期

		switch(event.param1)
		{
			case LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS://正常状态
				//add you fuc
				pocLedIdtAttr.jumpperiod = 1000;//恢复闪烁线程周期
				lv_poc_led_status_all_close();

				Led_CallBack.pf_poc_led_jump_status = NULL;//注销循环回调

				break;

			case LVPOCLEDIDTCOM_SIGNAL_START_TALK_STATUS://对讲状态
				//add you fuc
				//注册回调
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_red_close_green_jump;

				break;

			case LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS://充电状态
				//add you fuc
				lv_poc_red_close_green_open();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_CONNECT_NETWORK_STATUS://注册上网络
				//add you fuc
				lv_poc_red_close_green_open();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_LOW_BATTERY_STATUS://低电量状态
				//add you fuc
				lv_poc_red_open_green_close();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_MERMEBER_LIST_SUCCESS_STATUS://获取成员列表成功
				//add you fuc
				lv_poc_red_close_green_open();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_MERMEBER_LIST_FAIL_STATUS://获取成员列表失败
				//add you fuc
				lv_poc_red_open_green_close();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_GROUP_LIST_SUCCESS_STATUS://获取群组列表成功
				//add you fuc
				lv_poc_red_close_green_open();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_GROUP_LIST_FAIL_STATUS://获取群组列表失败
				//add you fuc
				lv_poc_red_open_green_close();

				break;

			case LVPOCLEDIDTCOM_SIGNAL_FAIL_STATUS://错误消息
				//add you fuc
				lv_poc_led_status_all_open();//TEST
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_red_open_green_jump;
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_green_close_red_jump;
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_green_open_red_jump;
				Led_CallBack.pf_poc_led_jump_status = callback_lv_poc_green_jump_red_jump;

				pocLedIdtAttr.isReady = false;
				osiThreadExit();
				break;

			default:break;
		}
	}
}

/*
	  name : poc_Led_Entry
	  param : none
	  date : 2020-06-02
*/
static void poc_Led_Jump_Entry(void *param)
{

	while(1)
	{
		if(pocLedIdtAttr.jumpperiod == 0 || Led_CallBack.pf_poc_led_jump_status == NULL)
		{
			osiThreadSleep(100);
			continue;
		}

		//查询回调
		lv_poc_led_status_callback_check(Led_CallBack.pf_poc_led_jump_status);

		osiThreadSleep(pocLedIdtAttr.jumpperiod);
	}
}

/*
	  name : lvPocLedIdtCom_Msg
	  param : none
	  date : 2020-06-02
*/
bool
lvPocLedIdtCom_Msg(LVPOCIDTCOM_Led_SignalType_t signal, LVPOCIDTCOM_Led_Period_t ctx)
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
	poc_set_green_status(false);
	osiThreadSleep(5);//不加延时驱动不成功
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
	poc_set_green_status(true);
	osiThreadSleep(5);//不加延时驱动不成功
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
	poc_set_green_status(false);
	osiThreadSleep(5);//不加延时驱动不成功
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
	poc_set_green_status(true);
	osiThreadSleep(5);//不加延时驱动不成功
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
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_green_status(pocLedIdtAttr.ledstatus);
	osiThreadSleep(5);//不加延时驱动不成功
	poc_set_red_status(false);
}

/*
	  name : lv_poc_red_open_green_jump
	  param : none
	  date : 2020-06-02
*/
static void
callback_lv_poc_red_open_green_jump(osiEvent_t *event)
{
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_green_status(pocLedIdtAttr.ledstatus);
	osiThreadSleep(5);//不加延时驱动不成功
	poc_set_red_status(true);
}

/*
	  name : lv_poc_green_close_red_jump
	  param : none
	  date : 2020-06-02
*/
static void
callback_lv_poc_green_close_red_jump(osiEvent_t *event)
{
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_red_status(pocLedIdtAttr.ledstatus);
	osiThreadSleep(5);//不加延时驱动不成功
	poc_set_green_status(false);
}

/*
	  name : lv_poc_green_open_red_jump
	  param : none
	  date : 2020-06-02
*/
static void
callback_lv_poc_green_open_red_jump(osiEvent_t *event)
{
	pocLedIdtAttr.ledstatus = ! pocLedIdtAttr.ledstatus;
	poc_set_red_status(pocLedIdtAttr.ledstatus);
	osiThreadSleep(5);//不加延时驱动不成功
	poc_set_green_status(true);
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
	osiThreadSleep(5);//不加延时驱动不成功
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
