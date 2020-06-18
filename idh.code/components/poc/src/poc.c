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

static void pocIdtStartHandleTask(void * ctx)
{
	poc_net_work_config(POC_SIM_1);
	lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0);
	poc_play_voice_one_time(LVPOCAUDIO_Type_Start_Machine, true);
	osiThreadSleep(5000);
	while(!poc_get_network_register_status(POC_SIM_1))
	{
		OSI_LOGI(0, "[poc][idt] checking network\n");
		osiThreadSleep(5000);
	}
	lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0);
	lv_poc_activity_func_cb_set.idle_note(lv_poc_idle_page2_warnning_info, 1, "正在登录...");
	lvPocGuiIdtCom_log();
	/*网络校时*/
	lv_poc_sntp_Update_Time();
	osiThreadExit();
}

#ifdef CONFIG_POC_GUI_START_ANIMATION_SUPPORT
static void pocStartAnimation(void *ctx)
{
	lvGuiRequestSceenOn(3);
	lv_obj_t *poc_power_on_backgroup_image = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_auto_size(poc_power_on_backgroup_image, false);
	lv_obj_set_size(poc_power_on_backgroup_image, 160, 128);
	extern lv_img_dsc_t img_poweron_poc_logo_unicom;
	lv_img_set_src(poc_power_on_backgroup_image, &img_poweron_poc_logo_unicom);

	osiThreadSleep(4000);
	osiThreadCreate("pocIdtStart", pocIdtStartHandleTask, NULL, OSI_PRIORITY_NORMAL, 1024, 64);
	osiThreadSleep(2800);
	lv_poc_create_idle();
	osiThreadSleep(200);
	lv_obj_del(poc_power_on_backgroup_image);
	lvGuiUpdateLastActivityTime();
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
	lv_obj_set_size(bg, 160, 128);
	lv_obj_t * label = lv_label_create(bg, NULL);
	lv_label_set_text(label, "Flyscale");
	lv_obj_align(label, bg, LV_ALIGN_CENTER, 0, 0);
}
#endif

void pocStart(void *ctx)
{
    OSI_LOGI(0, "lvgl poc start");
    poc_Status_Led_Task();
    lv_poc_activity_func_cb_set.status_led(LVPOCLEDIDTCOM_SIGNAL_NORMAL_STATUS, LVPOCLEDIDTCOM_BREATH_LAMP_PERIOD_0);
    drvLcdInitV2();

    drvLcd_t *lcd = drvLcdGetByname(DRV_NAME_LCD1);
    drvLcdOpenV2(lcd);
    drvLcdFill(lcd, 0, NULL, true);
    drvLcdSetBackLightEnable(lcd, true);
	lvGuiInit(pocLvglStart);

	osiThreadExit();
}


#endif
