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
#include "uart3_gps.h"
#include "abup_fota.h"

static bool lv_poc_watchdog_power_on_mode = false;
static lv_obj_t *poc_power_on_backgroup_sprd_image = NULL;
static lv_obj_t *poc_power_on_backgroup_image = NULL;

static
void prv_lv_poc_network_config_task(lv_task_t * task)
{
	poc_net_work_config(POC_SIM_1);
}

static
void prv_lv_poc_power_on_picture(lv_task_t * task)
{
	lv_poc_create_idle();
}

static
void prv_lv_poc_power_on_sprd_image(lv_task_t * task)
{
	poc_power_on_backgroup_sprd_image = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_auto_size(poc_power_on_backgroup_sprd_image, false);
	lv_obj_set_size(poc_power_on_backgroup_sprd_image, 160, 128);
	lv_img_set_src(poc_power_on_backgroup_sprd_image, &img_poweron_poc_logo_sprd);
}

static
void prv_lv_poc_power_on_backgroup_image(lv_task_t * task)
{
	poc_power_on_backgroup_image = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_auto_size(poc_power_on_backgroup_image, false);
	lv_obj_set_size(poc_power_on_backgroup_image, 160, 128);
	extern lv_img_dsc_t img_poweron_poc_logo_unicom;
	lv_img_set_src(poc_power_on_backgroup_image, &img_poweron_poc_logo_unicom);
}

bool pub_lv_poc_get_watchdog_status(void)
{
	return lv_poc_watchdog_power_on_mode;
}

void pub_lv_poc_set_watchdog_status(bool status)
{
	lv_poc_watchdog_power_on_mode = status;
}

static void pocIdtStartHandleTask(void * ctx)
{
    lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0, LVPOCLEDIDTCOM_SIGNAL_JUMP_1);
	if(!pub_lv_poc_get_watchdog_status())
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Start_Machine, 50, true);
		osiThreadSleepRelaxed(5000, OSI_WAIT_FOREVER);
	}

	lv_poc_set_power_on_status(true);
	while(!poc_get_network_register_status(POC_SIM_1))
	{
		OSI_LOGI(0, "[poc][idt] checking network\n");
		osiThreadSleepRelaxed(5000*6, OSI_WAIT_FOREVER);//30s
	}
	lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "正在登录...");

	if(!pub_lv_poc_get_watchdog_status())
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Now_Loginning, 50, true);
	}
	osiThreadSleepRelaxed(2000, OSI_WAIT_FOREVER);

#ifdef CONFIG_POC_FOTA_POWER_ON_SUPPORT
	abup_check_update_result();
#endif

#ifdef CONFIG_POC_SUPPORT
	lvPocGuiIdtCom_log();
#endif

	osiThreadExit();
}

#ifdef CONFIG_POC_GUI_START_ANIMATION_SUPPORT
static void pocStartAnimation(void *ctx)
{
	lvGuiRequestSceenOn(3);
	lv_poc_refr_task_once(prv_lv_poc_network_config_task, 50, LV_TASK_PRIO_HIGH);
	if(!pub_lv_poc_get_watchdog_status())
	{
		lv_poc_refr_task_once(prv_lv_poc_power_on_sprd_image, 50, LV_TASK_PRIO_HIGH);
		osiThreadSleepRelaxed(2000, OSI_WAIT_FOREVER);

		drvLcd_t *lcd = drvLcdGetByname(DRV_NAME_LCD1);
		drvLcdSetBackLightEnable(lcd, true);
		poc_set_lcd_blacklight(RG_RGB_BACKLIGHT_LEVEL_3);
		osiThreadSleepRelaxed(3000, OSI_WAIT_FOREVER);

		if(poc_power_on_backgroup_sprd_image != NULL)
		{
			lv_obj_del(poc_power_on_backgroup_sprd_image);
		}
		lv_poc_refr_task_once(prv_lv_poc_power_on_backgroup_image, 50, LV_TASK_PRIO_HIGH);
		lv_poc_setting_init();
		osiThreadSleepRelaxed(4000, OSI_WAIT_FOREVER);
		osiThreadCreate("pocIdtStart", pocIdtStartHandleTask, NULL, OSI_PRIORITY_NORMAL, 1024, 64);
		osiThreadSleepRelaxed(2800, OSI_WAIT_FOREVER);
 		lv_poc_refr_task_once(prv_lv_poc_power_on_picture, LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH);
		osiThreadSleepRelaxed(200, OSI_WAIT_FOREVER);

		if(poc_power_on_backgroup_image != NULL)
		{
			lv_obj_del(poc_power_on_backgroup_image);
		}
	}
	else//watchdog
	{
		lv_poc_setting_init();
		osiThreadSleepRelaxed(3000, OSI_WAIT_FOREVER);
		osiThreadCreate("pocIdtStart", pocIdtStartHandleTask, NULL, OSI_PRIORITY_NORMAL, 1024, 64);
		osiThreadSleepRelaxed(2000, OSI_WAIT_FOREVER);
		lv_poc_refr_task_once(prv_lv_poc_power_on_picture,
			LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH);
	}
	lvGuiUpdateLastActivityTime();
	lv_poc_sntp_Update_Time();

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
	poc_config_Lcd_power_vol();
	drvLcdInitV2();

	drvLcd_t *lcd = drvLcdGetByname(DRV_NAME_LCD1);
    drvLcdOpenV2(lcd);
    drvLcdFill(lcd, 0, NULL, true);
	drvLcdSetBackLightEnable(lcd, false);

	/*LED Init*/
    halPmuSwitchPower(HAL_POWER_REDLED, true, false);
    halPmuSwitchPower(HAL_POWER_GREENLED, true, false);
	halPmuSwitchPower(HAL_POWER_TOUCHLED, true, false);

    /*close led*/
    poc_set_red_blacklight(false);
    poc_set_green_blacklight(false);

#ifdef CONFIG_POC_GUI_KEYPAD_LIGHT_SUPPORT
    poc_set_keypad_led_status(false);
#endif


#ifdef CONFIG_POC_GUI_GPS_SUPPORT
	publvPocGpsIdtComInit();
#endif

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
		pub_lv_poc_set_watchdog_status(true);
		OSI_LOGI(0, "[song]poc boot mode is wdg power on");
		lvGuiInit(pocLvglStart);
	}
	else//设备重启或正常开机
	{
		pub_lv_poc_set_watchdog_status(false);
		lvGuiInit(pocLvglStart);
	}

	osiThreadExit();
}

#endif
