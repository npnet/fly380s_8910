#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"
#include <stdlib.h>
#include "lv_gui_main.h"
#include "hal_chip.h"
#include "drv_lcd_v2.h"
#include "drv_names.h"

/*掉电多长时间后关机*/
#define POC_CHARGE_POWER_DOWN_TIME 1/*2000*/

static void lv_poc_shutdown_charge_power_on_logo(void);
static void lv_poc_shutdown_charge_Animation_Task(void *ctx);
static void lv_poc_charge_poweron_battery_refresh(lv_task_t *task);
static void lv_poc_charge_power_outages_shutdown_task(lv_task_t * task);
static void lv_poc_show_sprd_image(lv_task_t *task);

static lv_style_t lv_poc_shutdown_charge_style = {0};
static lv_obj_t *lv_poc_shutdown_charge_obj = NULL;//背景框
static lv_obj_t *poc_charge_battery_image = NULL;
static bool poc_charge_poweron_status = false;
static lv_obj_t *poc_power_on_backgroup_sprd_image = NULL;


/*充电开机电池图标*/
const lv_img_dsc_t *charge_battery_img_dispaly[8] = { &indeterminate0
														,&indeterminate1
														,&indeterminate2
														,&indeterminate3
														,&indeterminate4
														,&indeterminate5
														,&indeterminate6};


/*
	  name : pocLvgl_ShutdownCharge_Start
	 param : none
  describe : 创建关机充电任务
	  date : 2020-06-24
*/
void pocLvgl_ShutdownCharge_Start(void)
{
	osiThreadCreate("osiShutdownChargeAnimation", lv_poc_shutdown_charge_Animation_Task, NULL, OSI_PRIORITY_NORMAL, 1024*3, 32);
}

/*
	  name : lv_poc_shutdown_charge_Animation
	 param : none
  describe : 创建关机充电动画任务
	  date : 2020-06-24
*/
static
void lv_poc_shutdown_charge_Animation_Task(void *ctx)
{
    lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_REGISTER_STATUS, true);
	lv_poc_shutdown_charge_power_on_logo();
	poc_charge_poweron_status = true;//为充电开机
	lv_poc_set_power_on_status(true);//设备就绪
	//lv刷新图标
	lv_task_create(lv_poc_charge_poweron_battery_refresh, LVPOCLISTIDTCOM_LIST_PERIOD_800
	, LV_TASK_PRIO_MID, NULL);
	lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS, true);
	while(1)
	{
		osiThreadSleepRelaxed(1000, OSI_WAIT_FOREVER);
	}
}

/*
	  name : lv_poc_shutdown_charge_power_on_logo
	 param : none
  describe : 创建充电开机背景
	  date : 2020-06-24
*/
static
void lv_poc_shutdown_charge_power_on_background(void)
{
	//背景框
	poc_setting_conf = lv_poc_setting_conf_read();//获取字体
	memset(&lv_poc_shutdown_charge_style, 0, sizeof(lv_style_t));
	lv_style_copy(&lv_poc_shutdown_charge_style, &lv_style_pretty_color);
	lv_poc_shutdown_charge_style.body.main_color = LV_COLOR_BLACK;//LV_COLOR_BLACK;
	lv_poc_shutdown_charge_style.body.grad_color = LV_COLOR_BLACK;
	lv_poc_shutdown_charge_style.body.opa = 255;
	lv_poc_shutdown_charge_style.body.radius = 0;
	lv_poc_shutdown_charge_style.body.border.part = LV_BORDER_NONE;//去除边缘

	lv_poc_shutdown_charge_obj = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_style(lv_poc_shutdown_charge_obj, &lv_poc_shutdown_charge_style);
	lv_obj_set_size(lv_poc_shutdown_charge_obj, LV_HOR_RES, LV_VER_RES);
	lv_obj_set_pos(lv_poc_shutdown_charge_obj, 0, 0);

	//电池图片
	poc_charge_battery_image = lv_img_create(lv_poc_shutdown_charge_obj, NULL);
	lv_img_set_auto_size(poc_charge_battery_image, false);
	lv_obj_set_size(poc_charge_battery_image, 97, 59);
	lv_img_set_src(poc_charge_battery_image, &indeterminate0);
	lv_obj_align(poc_charge_battery_image, lv_poc_shutdown_charge_obj, LV_ALIGN_CENTER, 0, 0);
}

/*
	  name : lv_poc_shutdown_charge_power_on_logo
	 param : none
  describe : 创建充电开机动画
	  date : 2020-06-24
*/
static
void lv_poc_shutdown_charge_power_on_logo(void)
{
	lvGuiRequestSceenOn(3);//开屏

	//魔方图片
	lv_poc_refr_task_once(lv_poc_show_sprd_image, LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_MID);
	osiThreadSleep(2000);

	drvLcd_t *lcd = drvLcdGetByname(DRV_NAME_LCD1);
	drvLcdSetBackLightEnable(lcd, true);
	poc_set_lcd_blacklight(RG_RGB_BACKLIGHT_LEVEL_3);
	//熄屏时间
	lvGuiSetInactiveTimeout(8000);
	osiThreadSleep(3000);
	lv_obj_del(poc_power_on_backgroup_sprd_image);
	//背景块
	lv_poc_refr_task_once(lv_poc_shutdown_charge_power_on_background, LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_MID);

	lvGuiReleaseScreenOn(3);//熄屏

}

/*
	  name : lv_poc_power_on_charge_get_battery_img
	 param : none
  describe : 获取充电开机电池图片
	  date : 2020-06-24
*/
static
lv_img_dsc_t *lv_poc_power_on_charge_get_battery_img(void)
{
	static bool charge_complete = true;
	const lv_img_dsc_t * battery_img = NULL;
	static uint8_t battery_img_cur = 100;
	battery_values_t battery_t;
	poc_battery_get_status(&battery_t);

	if(battery_img_cur == 100)
	{
		battery_img_cur = lv_poc_get_battery_cnt(&battery_t);
		if(battery_img_cur < 7)
		{
			lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS, true);
		}
	}

    if(!battery_t.battery_status)
    {
        battery_img = &no_battery_charging_shutdown;
        return (lv_img_dsc_t *)battery_img;
    }
	if(battery_t.charging == POC_CHG_DISCONNECTED)//设备未充电
	{
		lv_poc_charge_power_outages_shutdown_task(NULL);
		!charge_complete ? (charge_complete = true) : 0;
	}
	else//设备在充电
	{
		if(battery_t.battery_value >= 100)
		{
			if(charge_complete == true)
			{
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_CHARGING_STATUS, false);
				lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_CHARGING_COMPLETE_STATUS, true);
				charge_complete = false;
			}
			battery_img = charge_battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=6;
		}
		else if(battery_t.battery_value >= 85)
		{
			battery_img = charge_battery_img_dispaly[battery_img_cur];

			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=5;
		}
		else if(battery_t.battery_value >= 68)
		{
			battery_img = charge_battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=4;
		}
		else if(battery_t.battery_value >= 51)
		{
			battery_img = charge_battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=3;
		}
		else if(battery_t.battery_value >= 34)
		{
			battery_img = charge_battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=2;
		}
		else if(battery_t.battery_value >= 17)
		{
			battery_img = charge_battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=1;
		}
		else if(battery_t.battery_value >= 0)
		{
			battery_img = charge_battery_img_dispaly[battery_img_cur];
			battery_img_cur++;
			if(battery_img_cur>6)
			battery_img_cur=0;
		}
	}
	return (lv_img_dsc_t *)battery_img;
}

/*
	  name : lv_poc_charge_poweron_battery_refresh
	 param : none
  describe : 实时刷新充电开机电池图片
	  date : 2020-06-24
*/
static
void lv_poc_charge_poweron_battery_refresh(lv_task_t *task)
{
	static lv_img_dsc_t * pre_battery_img = NULL;
	lv_img_dsc_t * cur_battery_img = lv_poc_power_on_charge_get_battery_img();
	if(cur_battery_img != pre_battery_img && cur_battery_img != NULL)
	{
		pre_battery_img = cur_battery_img;
		lv_img_set_src(poc_charge_battery_image, cur_battery_img);
	}
}

/*
	  name : lv_poc_charge_poweron_status
	 param : none
  describe : 是否为充电开机
	  date : 2020-06-24
*/
bool lv_poc_charge_poweron_status(void)
{
	return poc_charge_poweron_status;
}

/*
	  name : lv_poc_charge_power_outages_shutdown_task
	 param : none
  describe : 设备断电关机任务
	  date : 2020-06-23
*/
static
void lv_poc_charge_power_outages_shutdown_task(lv_task_t * task)
{
	osiShutdown(OSI_SHUTDOWN_POWER_OFF);//关机
}

/*
	  name : lv_poc_show_sprd_image
	 param : none
  describe : 播放魔方图片
	  date : 2020-07-28
*/
static
void lv_poc_show_sprd_image(lv_task_t *task)
{
	poc_power_on_backgroup_sprd_image = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_auto_size(poc_power_on_backgroup_sprd_image, false);
	lv_obj_set_size(poc_power_on_backgroup_sprd_image, 132, 132);
	lv_img_set_src(poc_power_on_backgroup_sprd_image, &img_poweron_poc_logo_sprd);
}

#ifdef __cplusplus
}
#endif
