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

// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "poc.h"
#ifdef CONFIG_POC_SUPPORT

#include "osi_log.h"
#include "osi_api.h"
#include "osi_pipe.h"
#include "ml.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lvgl.h"
#include "lv_gui_main.h"
#include "drv_lcd_v2.h"
#include "drv_names.h"
#include "lv_include/lv_poc.h"
#include "app_test.h"
#include "guiIdtCom_api.h"
#include "hal_chip.h"
#include "tts_player.h"

static void lv_poc_network_config_task(lv_task_t * task);
static void lv_poc_power_on_picture(lv_task_t * task);
bool lv_poc_watchdog_power_on_mode = false;

static void pocIdtStartHandleTask(void * ctx)
{
	lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0, LVPOCLEDIDTCOM_SIGNAL_JUMP_1);

	if(lv_poc_watchdog_power_on_mode == false)
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Start_Machine, 50, true);
		osiThreadSleep(5000);
	}
	lv_poc_set_power_on_status(true);//设备准备就绪

	lv_poc_set_power_on_status(true);/*设备就绪*/

	while(!poc_get_network_register_status(POC_SIM_1))
	{
		OSI_LOGI(0, "[poc][idt] checking network\n");
		osiThreadSleep(5000*6);/*30s*/
	}
	lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "正在登录...");

	if(lv_poc_watchdog_power_on_mode == false)
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Now_Loginning, 50, true);
	}
	#if 0
	pocAudioPlayerSound();/*new player test*/
	#endif
	osiThreadSleep(2000);
	lvPocGuiIdtCom_log();

	osiThreadExit();
}

#ifdef CONFIG_POC_GUI_START_ANIMATION_SUPPORT
static void pocStartAnimation(void *ctx)
{
	lvGuiRequestSceenOn(3);
	//首先启动配网任务，解决第一次下载程序代码登陆不上问题
	lv_task_t * task = lv_task_create(lv_poc_network_config_task,
		50, LV_TASK_PRIO_HIGH, NULL);
	lv_task_once(task);

	if(lv_poc_watchdog_power_on_mode == false)
	{
		//开机图片
		lv_obj_t *poc_power_on_backgroup_image = lv_img_create(lv_scr_act(), NULL);
		lv_img_set_auto_size(poc_power_on_backgroup_image, false);
		lv_obj_set_size(poc_power_on_backgroup_image, 132, 132);
		lv_img_set_src(poc_power_on_backgroup_image, &poc_power_on_t2y);

		/*打开屏幕，除去花屏问题*/
		drvLcd_t *lcd = drvLcdGetByname(DRV_NAME_LCD1);
		drvLcdSetBackLightEnable(lcd, true);
		osiThreadSleep(2000);

		poc_set_lcd_blacklight(RG_RGB_BACKLIGHT_LEVEL_3);
		osiThreadSleep(3000);
		lv_poc_setting_init();/*开机配置*/
		osiThreadSleep(4000);
		osiThreadCreate("pocIdtStart", pocIdtStartHandleTask, NULL, OSI_PRIORITY_NORMAL, 1024, 64);
		osiThreadSleep(2800);
		lv_poc_refr_task_once(lv_poc_power_on_picture,
			LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH);
		osiThreadSleep(200);
		lv_obj_del(poc_power_on_backgroup_image);
	}
	else/*watchdog*/
	{
		lv_poc_setting_init();/*开机配置*/
		osiThreadSleep(3000);
		osiThreadCreate("pocIdtStart", pocIdtStartHandleTask, NULL, OSI_PRIORITY_NORMAL, 1024, 64);
		osiThreadSleep(2000);
		lv_poc_refr_task_once(lv_poc_power_on_picture,
			LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH);
		drvLcd_t *lcd = drvLcdGetByname(DRV_NAME_LCD1);
		drvLcdSetBackLightEnable(lcd, true);
	}
	lvGuiUpdateLastActivityTime();
	/*网络校时*/
	lv_poc_sntp_Update_Time();
	lv_poc_get_record_mic_gain();
	osiThreadSleep(500);
	lv_poc_set_record_mic_gain(MUSICRECORD, Handfree, POC_MIC_ANA_GAIN_LEVEL_7, POC_MIC_ADC_GAIN_LEVEL_15);/*set record mic gain*/

	lvGuiReleaseScreenOn(3);
	osiThreadExit();
}
#endif

#ifdef CONFIG_POC_GUI_SUPPORT
static void pocLvglStart(void)
{
#ifdef CONFIG_POC_GUI_START_ANIMATION_SUPPORT
	osiThreadCreate("oicStartWithAnimation", pocStartAnimation, NULL, OSI_PRIORITY_NORMAL, 1024 * 15, 64);
#else
	lv_poc_create_idle();

	osiThreadCreate("pocIdtStart", pocIdtStartHandleTask, NULL, OSI_PRIORITY_NORMAL, 1024, 64);
#endif
}
#else
static void pocLvglStart(void)
{
	lv_obj_t * bg = lv_obj_create(lv_scr_act(), NULL);
	lv_obj_set_size(bg, 132, 132);
	lv_obj_t * label = lv_label_create(bg, NULL);
	lv_label_set_text(label, "Flyscale");
	lv_obj_align(label, bg, LV_ALIGN_CENTER, 0, 0);
}
#endif

void pocStart(void *ctx)
{
    OSI_LOGI(0, "[song]lvgl poc start");

    poc_Status_Led_Task();
    lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0, LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
    drvLcdInitV2();

	drvLcd_t *lcd = drvLcdGetByname(DRV_NAME_LCD1);
    drvLcdOpenV2(lcd);
    drvLcdFill(lcd, 0, NULL, true);
    drvLcdSetBackLightEnable(lcd, false);

	//获取开机方式
	uint32_t boot_causes = osiGetBootCauses();
	OSI_LOGI(0, "[song]poc boot mode is = %d", boot_causes);
	if(boot_causes == OSI_BOOTCAUSE_CHARGE
		|| boot_causes == (OSI_BOOTCAUSE_CHARGE|OSI_BOOTCAUSE_PSM_WAKEUP))
		//设备为充电启动||设备充电启动并且从PSM唤醒启动
	{
		OSI_LOGI(0, "[song]poc boot mode is charge power on");
		lvGuiInit(pocLvgl_ShutdownCharge_Start);
	}//看门狗重启
	else if(boot_causes == OSI_BOOTCAUSE_WDG
		||  boot_causes == (OSI_BOOTCAUSE_CHARGE|OSI_BOOTCAUSE_WDG))
	{
		lv_poc_watchdog_power_on_mode = true;
		OSI_LOGI(0, "[song]poc boot mode is wdg power on");
		lvGuiInit(pocLvglStart);
	}
	else//设备重启或正常开机
	{
		lv_poc_watchdog_power_on_mode = false;
		lvGuiInit(pocLvglStart);
	}

	osiThreadExit();
}

/*
	  name : lv_poc_network_config_task
	 param : none
	author : wangls
  describe : 设备配网任务
	  date : 2020-06-30
*/
static
void lv_poc_network_config_task(lv_task_t * task)
{
	poc_net_work_config(POC_SIM_1);
}

/*
	  name : lv_poc_power_on_picture
	 param : none
	author : wangls
  describe : 开机桌面
	  date : 2020-07-03
*/
static
void lv_poc_power_on_picture(lv_task_t * task)
{
	lv_poc_create_idle();
	lv_poc_check_volum_task(LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_500);
}

#endif
