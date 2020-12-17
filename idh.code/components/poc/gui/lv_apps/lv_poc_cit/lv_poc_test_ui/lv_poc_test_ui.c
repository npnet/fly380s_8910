#ifdef __cplusplus
extern "C" {
#endif
#include "lv_include/lv_poc.h"
#include "stdlib.h"
#include "gps_nmea.h"
#include "uart3_gps.h"

static lv_obj_t * activity_create(lv_poc_display_t *display);

static void activity_destory(lv_obj_t *obj);

static void * cit_list_create(lv_obj_t * parent, lv_area_t display_area);

static void lv_poc_list_config(lv_obj_t * list, lv_area_t list_area);

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param);

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode);

static lv_obj_t * activity_list;

static lv_poc_win_t * cit_win;

static lv_poc_cit_test_type_t *cit_test_info = NULL;

lv_poc_activity_t * poc_cit_test_ui_activity;

typedef struct
{
    char *content;
    lv_label_long_mode_t content_long_mode;
    lv_label_align_t content_text_align;
    lv_align_t content_align;
    lv_coord_t content_align_x;
    lv_coord_t content_align_y;
} lv_poc_cit_key_label_struct_t;

struct poc_cit_key_obj_t
{
	lv_obj_t *label[24];
	lv_obj_t *keyrow[5];//more 5 row
	lv_style_t style;
};

typedef struct lv_poc_cit_test_t
{
	lv_task_t *refresh_task;
	int screenonoff;
	lv_style_t lcd_bg_style;
	int screenbgswitch;
	bool delayexitcheckkeyflag;
	int delayexitcheckkeyTasknum;
	int checkkeycountdownexitnum;
	int charge_first;
	int rgb_number;
	int gps_run_tim;
	bool delayexitgpsflag;
	int delayexitgpsnum;
	struct poc_cit_key_obj_t keyattr;
}lv_poc_cit_test_t;

static lv_poc_cit_test_t pocCitAttr;

lv_poc_cit_key_label_struct_t lv_poc_cit_key_label_array[16] = {//2(rows)*3(columns)
    {
        "up"     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_CENTER, LV_ALIGN_IN_LEFT_MID, 7, 0,
    },

    {
        "dow"     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_CENTER, LV_ALIGN_IN_LEFT_MID, 7 + 8 + LV_KEY_AVERAGE_WIDTH, 0,
    },

    {
        "v+"      , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_CENTER, LV_ALIGN_IN_LEFT_MID, LV_LCD_WIDTH - LV_KEY_AVERAGE_WIDTH - 11, 0,
    },

    {
        "v-"    , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_CENTER, LV_ALIGN_IN_LEFT_MID, 7, 0,
    },

    {
        "ppt"     , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_CENTER, LV_ALIGN_IN_LEFT_MID, 7 + 8 + LV_KEY_AVERAGE_WIDTH, 0,
    },

    {
        "pow"   , LV_LABEL_LONG_SROLL_CIRC, LV_LABEL_ALIGN_CENTER, LV_ALIGN_IN_LEFT_MID, LV_LCD_WIDTH - LV_KEY_AVERAGE_WIDTH - 11, 0,
    },
};

static lv_obj_t * activity_create(lv_poc_display_t *display)
{
	if(cit_test_info == NULL)
	{
		return NULL;
	}
    cit_win = lv_poc_win_create(display, cit_test_info->name, cit_list_create);
    return (lv_obj_t *)cit_win;
}

static void * cit_list_create(lv_obj_t * parent, lv_area_t display_area)
{
    activity_list = lv_poc_list_create(parent, NULL, display_area, lv_poc_list_config);
    lv_poc_notation_refresh();
    return (void *)activity_list;
}

static void activity_destory(lv_obj_t *obj)
{
	activity_list = NULL;
	poc_cit_test_ui_activity = NULL;
	cit_test_info->id = LV_POC_CIT_OPRATOR_TYPE_START;
	cit_test_info->cit_gps_attr.valid ? cit_test_info->cit_gps_attr.cb(0) : 0;
	cit_test_info->cit_gps_attr.valid = false;
	cit_test_info->cit_calib_attr.valid = false;
	cit_test_info->cit_rtc_attr.valid = false;
	cit_test_info->cit_backlight_attr.valid = false;
	cit_test_info->cit_volum_attr.valid = false;
	cit_test_info->cit_signal_attr.valid = false;
	cit_test_info->cit_charge_attr.valid = false;
	cit_test_info->cit_touch_attr.valid = false;
	cit_test_info->cit_rgb_attr.valid = false;
	cit_test_info->cit_flash_attr.valid = false;
	cit_test_info->cit_key_attr.valid = false;
	cit_test_info->cit_mic_attr.cb ? cit_test_info->cit_mic_attr.cb(false) : 0;
	cit_test_info->cit_headset_attr.cb ? cit_test_info->cit_headset_attr.cb(false) : 0;
	cit_test_info->cit_backlight_attr.cb ? cit_test_info->cit_backlight_attr.cb(1) : 0;
#ifdef CONFIG_POC_GUI_TOUCH_SUPPORT
	cit_test_info->cit_touch_attr.cb(false);
#endif
	cit_test_info = NULL;
}

static void lv_poc_cit_refresh_cb(lv_task_t *task)
{
	if(cit_test_info == NULL)
	{
		return;
	}

	switch(cit_test_info->id)
	{
		case LV_POC_CIT_OPRATOR_TYPE_RTC:
		{
			char rtcinfo[64];
			char rtchourstr[8];
			char rtcminstr[8];
			char rtcsecstr[8];
			lv_poc_time_t time;

			if(cit_test_info->cit_rtc_attr.cb == NULL)
			{
				break;
			}
			cit_test_info->cit_rtc_attr.cb(&time);

			__itoa(time.tm_hour, rtchourstr, 10);
			__itoa(time.tm_min, rtcminstr, 10);
			__itoa(time.tm_sec, rtcsecstr, 10);

			strcpy(rtcinfo, rtchourstr);
			strcat(rtcinfo, ":");
			time.tm_min < 10 ? strcat(rtcinfo, "0") : 0;
			strcat(rtcinfo, rtcminstr);
			strcat(rtcinfo, ":");
			time.tm_sec < 10 ? strcat(rtcinfo, "0") : 0;
			strcat(rtcinfo, rtcsecstr);
			task->user_data != NULL ? lv_label_set_text(task->user_data, rtcinfo) : 0;
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_BACKLIGHT:
		{
			if(cit_test_info->cit_backlight_attr.cb == NULL)
			{
				break;
			}
			pocCitAttr.screenonoff++;
			(pocCitAttr.screenonoff % 2 == 1) ? cit_test_info->cit_backlight_attr.cb(0) : cit_test_info->cit_backlight_attr.cb(1);
			if(pocCitAttr.screenonoff == 8)
			{
				if(pocCitAttr.refresh_task)
				{
					lv_task_del(pocCitAttr.refresh_task);
					pocCitAttr.refresh_task = NULL;
				}
				pocCitAttr.screenonoff = 0;
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_LCD:
		{
			pocCitAttr.screenbgswitch++;
			if(pocCitAttr.screenbgswitch == 1)
			{
				pocCitAttr.lcd_bg_style.body.main_color = LV_COLOR_WHITE;
				pocCitAttr.lcd_bg_style.body.grad_color = LV_COLOR_WHITE;
				task->user_data ? lv_obj_set_style(task->user_data, &pocCitAttr.lcd_bg_style) : 0;
			}
			else if(pocCitAttr.screenbgswitch == 2)
			{
				pocCitAttr.lcd_bg_style.body.main_color = LV_COLOR_RED;
				pocCitAttr.lcd_bg_style.body.grad_color = LV_COLOR_RED;
				task->user_data ? lv_obj_set_style(task->user_data, &pocCitAttr.lcd_bg_style) : 0;
			}
			else if(pocCitAttr.screenbgswitch == 3)
			{
				pocCitAttr.lcd_bg_style.body.main_color = LV_COLOR_GREEN;
				pocCitAttr.lcd_bg_style.body.grad_color = LV_COLOR_GREEN;
				task->user_data ? lv_obj_set_style(task->user_data, &pocCitAttr.lcd_bg_style) : 0;
			}
			else if(pocCitAttr.screenbgswitch == 4)
			{
				pocCitAttr.lcd_bg_style.body.main_color = LV_COLOR_BLUE;
				pocCitAttr.lcd_bg_style.body.grad_color = LV_COLOR_BLUE;
				task->user_data ? lv_obj_set_style(task->user_data, &pocCitAttr.lcd_bg_style) : 0;
			}
			else if(pocCitAttr.screenbgswitch == 5)
			{
				pocCitAttr.lcd_bg_style.body.main_color = LV_COLOR_YELLOW;
				pocCitAttr.lcd_bg_style.body.grad_color = LV_COLOR_YELLOW;
				task->user_data ? lv_obj_set_style(task->user_data, &pocCitAttr.lcd_bg_style) : 0;
			}
			else
			{
				pocCitAttr.screenbgswitch = 0;
				if(pocCitAttr.refresh_task)
				{
					lv_task_del(pocCitAttr.refresh_task);
					pocCitAttr.refresh_task = NULL;
				}
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_VOLUM:
		{
			if(cit_test_info->cit_volum_attr.cb == NULL)
			{
				break;
			}
			cit_test_info->cit_volum_attr.cb(1) ? 0 : cit_test_info->cit_volum_attr.cb(2);
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_SIGNAL:
		{
			char signaldBm[24];
			char signalvalue[8];
			uint8_t signalsign;
			if(cit_test_info->cit_signal_attr.cb == NULL)
			{
				break;
			}
			cit_test_info->cit_signal_attr.cb ? cit_test_info->cit_signal_attr.cb((uint8_t *)&signalsign) : 0;
			__itoa(signalsign, signalvalue, 10);
			strcpy(signaldBm, "-");
			strcat(signaldBm, signalvalue);
			strcat(signaldBm, " DBM");
			task->user_data != NULL ? lv_label_set_text(task->user_data, signaldBm) : 0;
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_KEY:
		{
			pocCitAttr.checkkeycountdownexitnum++;
			if(task->user_data == NULL || cit_test_info == NULL)
			{
				break;
			}
			lv_poc_cit_test_t *pKey = task->user_data;
			pKey->keyattr.style.body.main_color = LV_COLOR_GREEN;
			pKey->keyattr.style.body.grad_color = LV_COLOR_GREEN;

			if(pocCitAttr.delayexitcheckkeyTasknum > 10)//check pass, delay 2s exit
			{
				cit_test_info->cit_key_attr.cb();
				lv_task_del(pocCitAttr.refresh_task);
				pocCitAttr.refresh_task = NULL;
				lv_poc_del_activity(poc_cit_test_ui_activity);
				lvPocCitAutoTestCom_Msg(LV_POC_CIT_AUTO_TEST_TYPE_SUCCESS);
				break;
			}
			//200ms*10*30=1min auto exit
			if(pocCitAttr.checkkeycountdownexitnum >= 300)
			{
				cit_test_info->cit_key_attr.cb();
				lv_task_del(pocCitAttr.refresh_task);
				pocCitAttr.refresh_task = NULL;
				lv_poc_del_activity(poc_cit_test_ui_activity);
				lvPocCitAutoTestCom_Msg(LV_POC_CIT_AUTO_TEST_TYPE_FAILED);
				break;
			}

			if(pocCitAttr.delayexitcheckkeyflag)
			{
				pocCitAttr.delayexitcheckkeyTasknum++;
				break;
			}

			for(int i = 0; i < cit_test_info->cit_key_attr.keynumber; i++)
			{
				if(cit_test_info->cit_key_attr.key_attr[i].lv_poc_cit_key_cb == NULL)
				{
					continue;
				}
				int keyval = cit_test_info->cit_key_attr.key_attr[i].lv_poc_cit_key_cb(false);
				if(keyval == 0)
				{
					continue;
				}

				if(cit_test_info->cit_key_attr.key_attr[i].checkstatus == false)
				{
					lv_label_set_style(pKey->keyattr.label[i], LV_LABEL_STYLE_MAIN, &pKey->keyattr.style);
					lv_label_set_body_draw(pKey->keyattr.label[i],true);
					cit_test_info->cit_key_attr.key_attr[i].checkstatus = true;
					cit_test_info->cit_key_attr.validnumber++;
				}

				if(cit_test_info->cit_key_attr.validnumber == cit_test_info->cit_key_attr.keynumber)
				{
					pocCitAttr.delayexitcheckkeyflag = true;
					cit_test_info->cit_key_attr.cb();
					break;
				}
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_CHARGE:
		{
			if(task->user_data == NULL || cit_test_info->cit_charge_attr.cb == NULL)
			{
				break;
			}
			lv_poc_cit_test_t *pKey = task->user_data;
			battery_values_t battery_t;
			static bool charge_status = false;
			cit_test_info->cit_charge_attr.cb(&battery_t);
			if(pocCitAttr.charge_first == 100)
			{
				pocCitAttr.charge_first = 0;
				if(POC_CHG_DISCONNECTED == battery_t.charging)
					charge_status = true;
				else
					charge_status = false;
			}

			if(POC_CHG_DISCONNECTED == battery_t.charging)
			{
				if(charge_status == true)
				{
					charge_status = false;
					pKey->keyattr.label[0] ? lv_label_set_text(pKey->keyattr.label[0], "状态: 放电中") : 0;
					pKey->keyattr.label[2] ? lv_label_set_text(pKey->keyattr.label[2], "插入: AC和USB\n均未插入") : 0;
				}
			}
			else
			{
				if(charge_status == false)
				{
					charge_status = true;
					pKey->keyattr.label[0] ? lv_label_set_text(pKey->keyattr.label[0], "状态: 充电中") : 0;
					pKey->keyattr.label[2] ? lv_label_set_text(pKey->keyattr.label[2], "插入: USB插入") : 0;
				}
			}

			char totalmvinfo[64];
			char mvstr[16];
			__itoa(battery_t.battery_val_mV, mvstr, 10);
			strcpy(totalmvinfo, "电压: ");
			strcat(totalmvinfo, mvstr);
			strcat(totalmvinfo, "mv");
			pKey->keyattr.label[1] ? lv_label_set_text(pKey->keyattr.label[1], totalmvinfo) : 0;
			break;
		}

#ifdef CONFIG_POC_GUI_GPS_SUPPORT
		case LV_POC_CIT_OPRATOR_TYPE_GPS:
		{
			if(task->user_data == NULL || cit_test_info->cit_gps_attr.cb == NULL)
			{
				break;
			}
			lv_poc_cit_test_t *pKey = task->user_data;
			nmea_msg *gps_info = (nmea_msg *)publvPocGpsIdtComDataInfo();
			if(gps_info == NULL)
			{
				break;
			}

			//svnnum
			char gpssvnum[64];
			char svnumstr[16];
			//gps_info->svnum;//可见的卫星数
			//gps_info->beidou_svnum;//可见的卫星数--beidou
			//gps_info->posslnum;//用于定位的卫星数量
			pocCitAttr.delayexitgpsflag && (gps_info->svnum + gps_info->beidou_svnum) == 0 ?\
			__itoa(9, svnumstr, 10) :\
			__itoa((gps_info->svnum + gps_info->beidou_svnum), svnumstr, 10);
			strcpy(gpssvnum, "卫星数量:");
			strcat(gpssvnum, svnumstr);
			pKey->keyattr.label[0] ? lv_label_set_text(pKey->keyattr.label[0], gpssvnum) : 0;

			//sn
			char gpssn[64];
			char snstr[16];
			int slmsgsn[13] = {0};
			int slmsgsnmax = 0;
			//gps_info->pdop;
			for(int i = 0; i < gps_info->svnum; i++)
			{
				slmsgsn[i] = gps_info->slmsg[i].sn;
				if(i > 0
					&& slmsgsn[i] > slmsgsn[i - 1])
				{
					slmsgsnmax = slmsgsn[i];
				}
				else
				{
					slmsgsnmax = slmsgsn[i - 1];
				}
			}
			pocCitAttr.delayexitgpsflag && slmsgsnmax == 0 ?\
			sprintf(snstr, "%3d", 48) :\
			sprintf(snstr, "%3d", slmsgsnmax);
			strcpy(gpssn, "Top5(sn):");
			strcat(gpssn, snstr);
			pKey->keyattr.label[1] ? lv_label_set_text(pKey->keyattr.label[1], gpssn) : 0;
			//run time
			char gpsruntim[64];
			char runtimstr[16];
			__itoa((pocCitAttr.gps_run_tim++), runtimstr, 10);
			strcpy(gpsruntim, runtimstr);
			strcat(gpsruntim, "秒已耗时");
			pKey->keyattr.label[2] ? lv_label_set_text(pKey->keyattr.label[2], gpsruntim) : 0;

			if((gps_info->gpssta == 1
				|| gps_info->gpssta == 2
				|| gps_info->gpssta == 6)
				&& pocCitAttr.delayexitgpsflag == false)
			{
				pocCitAttr.delayexitgpsflag = true;
				char latlonginfo[64];
				char latstr[8];
				char longstr[8];
				float latf = gps_info->latitude/100000.0;
				float longf = gps_info->longitude/100000.0;
				sprintf(latstr, "%03.1f", latf);
				sprintf(longstr, "%04.1f", longf);
				strcpy(latlonginfo, "定位 ");
				strcat(latlonginfo, latstr);
				strcat(latlonginfo, " ");
				strcat(latlonginfo, longstr);
				pKey->keyattr.label[3] ? lv_label_set_text(pKey->keyattr.label[3], latlonginfo) : 0;
			}

			//delay exit
			if(pocCitAttr.delayexitgpsflag)
			{
				pocCitAttr.delayexitgpsnum++;
			}
			if(pocCitAttr.delayexitgpsnum >= 8)
			{
				pocCitAttr.delayexitgpsflag = false;
				pocCitAttr.delayexitgpsnum = 0;
				pocCitAttr.gps_run_tim = 0;
				lv_task_del(pocCitAttr.refresh_task);
				pocCitAttr.refresh_task = NULL;
				cit_test_info->cit_gps_attr.valid ? cit_test_info->cit_gps_attr.cb(0) : 0;
				cit_test_info->cit_gps_attr.valid = false;
				lv_poc_del_activity(poc_cit_test_ui_activity);
				lvPocCitAutoTestCom_Msg(LV_POC_CIT_AUTO_TEST_TYPE_SUCCESS);
			}
			break;
		}
#endif

		case LV_POC_CIT_OPRATOR_TYPE_SIM:
		{
			if(task->user_data == NULL)
			{
				break;
			}
			lv_poc_cit_test_t *pKey = task->user_data;

			if(!poc_check_sim_prsent(POC_SIM_1))
			{
				pKey->keyattr.label[1] ? lv_label_set_text(pKey->keyattr.label[1], "Sim State:no sim") : 0;
				pKey->keyattr.label[2] ? lv_label_set_text(pKey->keyattr.label[2], "Sim Country:\ncan't get") : 0;
				pKey->keyattr.label[3] ? lv_label_set_text(pKey->keyattr.label[3], "Sim Operator:\ncan't get") : 0;
				pKey->keyattr.label[4] ? lv_label_set_text(pKey->keyattr.label[4], "Sim Operator Na\nme:can't get") : 0;
				pKey->keyattr.label[5] ? lv_label_set_text(pKey->keyattr.label[5], "Sim Serial Num\nber:can't get") : 0;
				pKey->keyattr.label[6] ? lv_label_set_text(pKey->keyattr.label[6], "Subscriber Id:\ncan't get") : 0;
				pKey->keyattr.label[7] ? lv_label_set_text(pKey->keyattr.label[7], "Device Id:can't\nget") : 0;
				pKey->keyattr.label[8] ? lv_label_set_text(pKey->keyattr.label[8], "Line 1 Number:\nPhone Type:GSM") : 0;
				pKey->keyattr.label[9] ? lv_label_set_text(pKey->keyattr.label[9], "Data State:dis\nconnected") : 0;
				pKey->keyattr.label[10] ? lv_label_set_text(pKey->keyattr.label[10], "Data Activity:\nnone") : 0;
				pKey->keyattr.label[11] ? lv_label_set_text(pKey->keyattr.label[11], "Network County:\ncn") : 0;
				pKey->keyattr.label[12] ? lv_label_set_text(pKey->keyattr.label[12], "Network Opera\ntor:46000") : 0;
				pKey->keyattr.label[13] ? lv_label_set_text(pKey->keyattr.label[13], "Network Type:\ncan'tget") : 0;
			}
			else
			{
				char iccidstr[24] = {0};
				char imsistr[24] = {0};

				pKey->keyattr.label[1] ? lv_label_set_text(pKey->keyattr.label[1], "Sim State:fine") : 0;
				pKey->keyattr.label[8] ? lv_label_set_text(pKey->keyattr.label[8], "Line 1 Number:\nPhone Type:GSM") : 0;
				//Country
				char country[64] = {0};
				char countrystr[16] = {0};
				lv_poc_get_mobile_card_operator(countrystr, false);
				strcpy(country, "Sim Country:\n");
				strcat(country, countrystr);
				pKey->keyattr.label[2] ? lv_label_set_text(pKey->keyattr.label[2], country) : 0;
				//sim opt
				char simopt[64] = {0};
				char optstr[8] = {0};
				poc_get_device_imsi_rep((int8_t *)imsistr);
				strncpy(optstr, imsistr, 5);
				strcpy(simopt, "Sim Operator:\n");
				strcat(simopt, optstr);
				pKey->keyattr.label[3] ? lv_label_set_text(pKey->keyattr.label[3], simopt) : 0;
				//sim opt name
				char simoptabbr[64] = {0};
				char optabbrstr[16] = {0};
				lv_poc_get_mobile_card_operator(optabbrstr, true);
				strcpy(simoptabbr, "Sim Operator Na\nme:");
				strcat(simoptabbr, optabbrstr);
				pKey->keyattr.label[4] ? lv_label_set_text(pKey->keyattr.label[4], simoptabbr) : 0;
				//iccid
				char simiccidnum[64] = {0};
				poc_get_device_iccid_rep((int8_t *)iccidstr);
				strcpy(simiccidnum, "Sim Serial Number:\n");
				strcat(simiccidnum, iccidstr);
				pKey->keyattr.label[5] ? lv_label_set_text(pKey->keyattr.label[5], simiccidnum) : 0;
				//imsi
				char simimsinum[64] = {0};
				strcpy(simimsinum, "Subscriber Id:\n");
				strcat(simimsinum, imsistr);
				pKey->keyattr.label[6] ? lv_label_set_text(pKey->keyattr.label[6], simimsinum) : 0;
				//need add, waiting
				pKey->keyattr.label[7] ? lv_label_set_text(pKey->keyattr.label[7], "Device Id:can't\nget") : 0;
				//net connect state
				if(!poc_get_network_register_status(POC_SIM_1))
				{
					pKey->keyattr.label[9] ? lv_label_set_text(pKey->keyattr.label[9], "Data State:dis\nconnected") : 0;
				}
				else
				{
					pKey->keyattr.label[9] ? lv_label_set_text(pKey->keyattr.label[9], "Data State:\nconnected") : 0;
				}
				//data acti
				pKey->keyattr.label[10] ? lv_label_set_text(pKey->keyattr.label[10], "Data Activity:\nnone") : 0;
				//net country
				char netcountry[64] = {0};
				strcpy(netcountry, "Network Country:\n");
				strcat(netcountry, countrystr);
				pKey->keyattr.label[11] ? lv_label_set_text(pKey->keyattr.label[11], netcountry) : 0;
				//net operator
				char simnetopt[64] = {0};
				strcpy(simnetopt, "Network Opera\ntor:");
				strcat(simnetopt, optstr);
				pKey->keyattr.label[12] ? lv_label_set_text(pKey->keyattr.label[12], simnetopt) : 0;
				//net type
				char simnettype[64] = {0};
				strcpy(simnettype, "Network Type:\n");
				POC_MMI_MODEM_PLMN_RAT rat;
				int8_t	operat;
				poc_get_operator_network_type_req(POC_SIM_1,&operat,&rat);
				switch(rat)
				{
					case  MMI_MODEM_PLMN_RAT_GSM:
						strcat(simnettype, "GSM");
						break;
					case MMI_MODEM_PLMN_RAT_UMTS:
						strcat(simnettype, "UTRAM");
						break;
					case MMI_MODEM_PLMN_RAT_LTE:
						strcat(simnettype, "LTE");
						break;
					case MMI_MODEM_PLMN_RAT_NO_SERVICE:
						strcat(simnettype, "无服务");
						break;
					default:
						strcat(simnettype, "UNKNOW");
						break;
				}
				pKey->keyattr.label[13] ? lv_label_set_text(pKey->keyattr.label[13], simnettype) : 0;
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_RGB:
		{
			if(cit_test_info->cit_rgb_attr.cb == NULL)
			{
				break;
			}
			pocCitAttr.rgb_number++;
			switch(pocCitAttr.rgb_number)
			{
				case 1:
				{
					cit_test_info->cit_rgb_attr.cb(0);
					break;
				}

				case 2:
				{
					cit_test_info->cit_rgb_attr.cb(1);
					break;
				}

				case 3:
				{
					cit_test_info->cit_rgb_attr.cb(2);
					break;
				}

				case 4:
				{
					cit_test_info->cit_rgb_attr.cb(3);
					if(pocCitAttr.refresh_task)
					{
						lv_task_del(pocCitAttr.refresh_task);
						pocCitAttr.refresh_task = NULL;
					}
					break;
				}
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_FLASH:
		{
			if(task->user_data == NULL || cit_test_info->cit_flash_attr.cb == NULL)
			{
				break;
			}
			lv_poc_cit_test_t *pKey = task->user_data;

			char availmem[64];
			char availmemstr[20];
			sprintf(availmemstr,"%3.2f",cit_test_info->cit_flash_attr.cb(1));//ftoc
			strcpy(availmem, "已用内存:");
			strcat(availmem, availmemstr);
			strcat(availmem, " MB");
			pKey->keyattr.label[1] ? lv_label_set_text(pKey->keyattr.label[1], availmem) : 0;

			char unavailmem[64];
			char unavailmemstr[20];
			sprintf(unavailmemstr,"%3.2f",cit_test_info->cit_flash_attr.cb(0));//ftoc
			strcpy(unavailmem, "可用内存:");
			strcat(unavailmem, unavailmemstr);
			strcat(unavailmem, " MB");
			pKey->keyattr.label[2] ? lv_label_set_text(pKey->keyattr.label[2], unavailmem) : 0;
			break;
		}

		default:
		{
			break;
		}
	}
}

static void lv_poc_list_config(lv_obj_t * list, lv_area_t list_area)
{
	if(cit_test_info == NULL)
	{
		return;
	}

	switch(cit_test_info->id)
	{
		case LV_POC_CIT_OPRATOR_TYPE_CALIBATE:
		{
			if(cit_test_info->cit_calib_attr.cb == NULL)
			{
				break;
			}
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_style_t * style_label;

			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

			if(cit_test_info->cit_calib_attr.cb() == 0
				&& cit_test_info->cit_calib_attr.valid)
			{
				lv_label_set_text(label, "已校准");
			}
			else
			{
				char calibParam[16];
				char nocalib[64];
				__itoa(lv_poc_get_calib_status(), calibParam, 10);
				strcpy(nocalib, "未校准:");
				strcat(nocalib, calibParam);
				strcat(nocalib, "项");
				lv_label_set_text(label, nocalib);
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_RTC:
		{
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_style_t * style_label;

			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_CENTER);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
			lv_label_set_text(label, "系统时间");
			//rtc
			lv_obj_t *label2 = lv_label_create(list, NULL);
			lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label2, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label2, LV_LABEL_ALIGN_CENTER);
			lv_obj_set_width(label2, lv_obj_get_width(list));
			lv_obj_align(label2, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 1000, LV_TASK_PRIO_MID, (void *)label2);
				lv_task_ready(pocCitAttr.refresh_task);
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_BACKLIGHT:
		{
			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 500, LV_TASK_PRIO_HIGHEST, NULL);
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_LCD:
		{
			memset(&pocCitAttr.lcd_bg_style, 0, sizeof(lv_style_t));
			lv_style_copy(&pocCitAttr.lcd_bg_style, &lv_style_pretty_color);
			pocCitAttr.lcd_bg_style.body.opa = 255;
			pocCitAttr.lcd_bg_style.body.radius = 0;
			pocCitAttr.lcd_bg_style.body.border.part = LV_BORDER_NONE;

			lv_obj_t *lcd_bg_obj = lv_obj_create(list, NULL);
			lv_obj_set_size(lcd_bg_obj, LV_HOR_RES, LV_VER_RES);
			lv_obj_set_pos(lcd_bg_obj, 0, 0);

			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 500, LV_TASK_PRIO_HIGH, (void *)lcd_bg_obj);
				lv_task_ready(pocCitAttr.refresh_task);
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_VOLUM:
		{
			if(cit_test_info->cit_volum_attr.cb == NULL)
			{
				break;
			}
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_style_t * style_label;

			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_CENTER);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_set_height(label, lv_obj_get_width(list)/4);
			lv_obj_align(label, list, LV_ALIGN_CENTER, 0, 0);
			lv_label_set_text(label, "请通过音量键\n调节音量.");

			if(pocCitAttr.refresh_task == NULL)
			{
				cit_test_info->cit_volum_attr.cb(0);
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 1000, LV_TASK_PRIO_HIGH, (void *)NULL);
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_MIC:
		{
			if(cit_test_info->cit_mic_attr.cb == NULL)
			{
				break;
			}
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_style_t * style_label;

			//speak
			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_set_height(label, lv_obj_get_width(list)/4);
			lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
			lv_label_set_text(label, "1.请按下PPT开始\n讲话录音.");
			//listen
			lv_obj_t *label2 = lv_label_create(list, NULL);
			lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label2, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label2, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label2, lv_obj_get_width(list));
			lv_obj_set_height(label2, lv_obj_get_width(list)/4);
			lv_obj_align(label2, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
			lv_label_set_text(label2, "2.松开PPT键,播放\n录音内容.");
			cit_test_info->cit_mic_attr.cb(true);;//loopback
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_SIGNAL:
		{
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_style_t * style_label;
			//signal
			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_CENTER);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 200, LV_TASK_PRIO_MID, (void *)label);
				lv_task_ready(pocCitAttr.refresh_task);
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_KEY:
		{
			if(cit_test_info->cit_key_attr.keynumber == 0)
			{
				break;
			}
			memset(&pocCitAttr.keyattr.style, 0, sizeof(lv_style_t));
			poc_setting_conf = lv_poc_setting_conf_read();
			lv_style_copy(&pocCitAttr.keyattr.style, &lv_style_pretty_color);
			pocCitAttr.keyattr.style.text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			pocCitAttr.keyattr.style.text.color = LV_COLOR_BLACK;
			pocCitAttr.keyattr.style.text.opa = 255;
			pocCitAttr.keyattr.style.body.main_color = LV_COLOR_RED;
			pocCitAttr.keyattr.style.body.grad_color = LV_COLOR_RED;
			pocCitAttr.keyattr.style.body.opa = 255;
			pocCitAttr.keyattr.style.body.radius = 0;

			lv_style_t *labelstyle = (lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;

			int rows = 0;
			if((cit_test_info->cit_key_attr.keynumber % LV_LCD_COLUMN) != 0)
			{
				rows = cit_test_info->cit_key_attr.keynumber / LV_LCD_COLUMN + 1;
			}
			else
			{
				rows = cit_test_info->cit_key_attr.keynumber / LV_LCD_COLUMN;
			}

			for(int i = 0; i < rows; i++)
			{
				pocCitAttr.keyattr.keyrow[i] = lv_obj_create(list, NULL);
				lv_label_set_style(pocCitAttr.keyattr.keyrow[i], LV_LABEL_STYLE_MAIN, labelstyle);
				lv_obj_set_width(pocCitAttr.keyattr.keyrow[i], LV_LCD_WIDTH);
				lv_obj_set_height(pocCitAttr.keyattr.keyrow[i], (LV_LCD_HEIGHT - LV_POC_STATUS_BAR_VER_RES*3)/rows);
			}

			for(int i = 0; i < cit_test_info->cit_key_attr.keynumber; i++)
			{
				lv_obj_t *label;
				if(i <= (1*LV_LCD_COLUMN - 1))//more LV_LCD_COLUMN*4 key
				{
					label = lv_label_create(pocCitAttr.keyattr.keyrow[0], NULL);
					lv_obj_align(label, pocCitAttr.keyattr.keyrow[0], lv_poc_cit_key_label_array[i].content_align, lv_poc_cit_key_label_array[i].content_align_x, lv_poc_cit_key_label_array[i].content_align_y);
				}
				else if(i > (1*LV_LCD_COLUMN - 1) && i <= (2*LV_LCD_COLUMN - 1))
				{
					label = lv_label_create(pocCitAttr.keyattr.keyrow[1], NULL);
					lv_obj_align(label, pocCitAttr.keyattr.keyrow[1], lv_poc_cit_key_label_array[i].content_align, lv_poc_cit_key_label_array[i].content_align_x, lv_poc_cit_key_label_array[i].content_align_y);
				}
				else if(i > (2*LV_LCD_COLUMN - 1) && i <= (3*LV_LCD_COLUMN - 1))
				{
					label = lv_label_create(pocCitAttr.keyattr.keyrow[2], NULL);
					lv_obj_align(label, pocCitAttr.keyattr.keyrow[2], lv_poc_cit_key_label_array[i].content_align, lv_poc_cit_key_label_array[i].content_align_x, lv_poc_cit_key_label_array[i].content_align_y);
				}
				else if(i > (3*LV_LCD_COLUMN - 1) && i <= (4*LV_LCD_COLUMN - 1))
				{
					label = lv_label_create(pocCitAttr.keyattr.keyrow[3], NULL);
					lv_obj_align(label, pocCitAttr.keyattr.keyrow[3], lv_poc_cit_key_label_array[i].content_align, lv_poc_cit_key_label_array[i].content_align_x, lv_poc_cit_key_label_array[i].content_align_y);
				}
				lv_obj_set_width(label, cit_test_info->cit_key_attr.key_attr[i].keywidth);
				lv_obj_set_height(label, cit_test_info->cit_key_attr.key_attr[i].keyheight);
				lv_label_set_long_mode(label, lv_poc_cit_key_label_array[i].content_long_mode);
				lv_label_set_align(label, lv_poc_cit_key_label_array[i].content_text_align);
				lv_label_set_text(label, lv_poc_cit_key_label_array[i].content);
				lv_label_set_style(label, LV_LABEL_STYLE_MAIN, &pocCitAttr.keyattr.style);
				lv_label_set_body_draw(label,true);
				pocCitAttr.keyattr.label[i] = label;
			}

			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 200, LV_TASK_PRIO_HIGH, (void *)&pocCitAttr);
			}

			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_CHARGE:
		{
			lv_style_t * style_label;
			//charge status
			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label, "");
			pocCitAttr.keyattr.label[0] = label;
			//mv
			lv_obj_t *label2 = lv_label_create(list, NULL);
			lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label2, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label2, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label2, lv_obj_get_width(list));
			lv_obj_align(label2, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label2, "");
			pocCitAttr.keyattr.label[1] = label2;
			//connect
			lv_obj_t *label3 = lv_label_create(list, NULL);
			lv_label_set_style(label3, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label3, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label3, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label3, lv_obj_get_width(list));
			lv_obj_set_height(label3, lv_obj_get_width(list)/4);
			lv_obj_align(label3, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label3, "");
			pocCitAttr.keyattr.label[2] = label3;

			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 500, LV_TASK_PRIO_MID, (void *)&pocCitAttr);
				lv_task_ready(pocCitAttr.refresh_task);
			}
			break;
		}

#ifdef CONFIG_POC_GUI_GPS_SUPPORT
		case LV_POC_CIT_OPRATOR_TYPE_GPS:
		{
			if(cit_test_info->cit_gps_attr.cb == NULL)
			{
				break;
			}
			lv_style_t * style_label;
			//卫星数量
			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label, "卫星数量:");
			pocCitAttr.keyattr.label[0] = label;
			//mv
			lv_obj_t *label2 = lv_label_create(list, NULL);
			lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label2, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label2, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label2, lv_obj_get_width(list));
			lv_obj_align(label2, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label2, "Top5:");
			pocCitAttr.keyattr.label[1] = label2;
			//run time
			lv_obj_t *label3 = lv_label_create(list, NULL);
			lv_label_set_style(label3, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label3, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label3, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label3, lv_obj_get_width(list));
			lv_obj_align(label3, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label3, "秒已消耗");
			pocCitAttr.keyattr.label[2] = label3;
			//location lat long
			lv_obj_t *label4 = lv_label_create(list, NULL);
			lv_label_set_style(label4, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label4, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label4, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label4, lv_obj_get_width(list));
			lv_obj_align(label4, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label4, "");
			pocCitAttr.keyattr.label[3] = label4;

			//check GPS task status
			cit_test_info->cit_gps_attr.valid ? cit_test_info->cit_gps_attr.cb(1) : 0;

			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 1000, LV_TASK_PRIO_MID, (void *)&pocCitAttr);
				lv_task_ready(pocCitAttr.refresh_task);
			}
			break;
		}
#endif

		case LV_POC_CIT_OPRATOR_TYPE_SIM:
		{
			lv_style_t * style_label;
			//label1
			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label, "Sim1 测试结果:");
			pocCitAttr.keyattr.label[0] = label;
			//label2
			lv_obj_t *label2 = lv_label_create(list, NULL);
			lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label2, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label2, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label2, lv_obj_get_width(list));
			lv_obj_align(label2, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label2, "Sim State:");
			pocCitAttr.keyattr.label[1] = label2;
			//label3
			lv_obj_t *label3 = lv_label_create(list, NULL);
			lv_label_set_style(label3, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label3, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label3, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label3, lv_obj_get_width(list));
			lv_obj_set_height(label3, lv_obj_get_width(list)/4);
			lv_obj_align(label3, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label3, "Sim Country:");
			pocCitAttr.keyattr.label[2] = label3;
			//label4
			lv_obj_t *label4 = lv_label_create(list, NULL);
			lv_label_set_style(label4, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label4, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label4, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label4, lv_obj_get_width(list));
			lv_obj_set_height(label4, lv_obj_get_width(list)/4);
			lv_obj_align(label4, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label4, "Sim Operator:");
			pocCitAttr.keyattr.label[3] = label4;
			//label5
			lv_obj_t *label5 = lv_label_create(list, NULL);
			lv_label_set_style(label5, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label5, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label5, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label5, lv_obj_get_width(list));
			lv_obj_set_height(label5, lv_obj_get_width(list)/4);
			lv_obj_align(label5, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label5, "Sim Operator Name:");
			pocCitAttr.keyattr.label[4] = label5;
			//label6
			lv_obj_t *label6 = lv_label_create(list, NULL);
			lv_label_set_style(label6, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label6, LV_LABEL_LONG_SROLL);
			lv_label_set_align(label6, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label6, lv_obj_get_width(list));
			lv_obj_set_height(label6, lv_obj_get_width(list)/4);
			lv_obj_align(label6, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label6, "Sim Serial Number:");
			pocCitAttr.keyattr.label[5] = label6;
			//label7
			lv_obj_t *label7 = lv_label_create(list, NULL);
			lv_label_set_style(label7, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label7, LV_LABEL_LONG_SROLL);
			lv_label_set_align(label7, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label7, lv_obj_get_width(list));
			lv_obj_set_height(label7, lv_obj_get_width(list)/4);
			lv_obj_align(label7, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label7, "Subscriber Id:");
			pocCitAttr.keyattr.label[6] = label7;
			//label8
			lv_obj_t *label8 = lv_label_create(list, NULL);
			lv_label_set_style(label8, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label8, LV_LABEL_LONG_SROLL);
			lv_label_set_align(label8, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label8, lv_obj_get_width(list));
			lv_obj_set_height(label8, lv_obj_get_width(list)/4);
			lv_obj_align(label8, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label8, "Device Id:");
			pocCitAttr.keyattr.label[7] = label8;
			//label9
			lv_obj_t *label9 = lv_label_create(list, NULL);
			lv_label_set_style(label9, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label9, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label9, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label9, lv_obj_get_width(list));
			lv_obj_set_height(label9, lv_obj_get_width(list)/4);
			lv_obj_align(label9, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label9, "Line 1 Number:\nPhone Type:GSM");
			pocCitAttr.keyattr.label[8] = label9;
			//label10
			lv_obj_t *label10 = lv_label_create(list, NULL);
			lv_label_set_style(label10, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label10, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label10, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label10, lv_obj_get_width(list));
			lv_obj_set_height(label10, lv_obj_get_width(list)/4);
			lv_obj_align(label10, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label10, "Data State:");
			pocCitAttr.keyattr.label[9] = label10;
			//label11
			lv_obj_t *label11 = lv_label_create(list, NULL);
			lv_label_set_style(label11, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label11, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label11, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label11, lv_obj_get_width(list));
			lv_obj_set_height(label11, lv_obj_get_width(list)/4);
			lv_obj_align(label11, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label11, "Data Activity:none");
			pocCitAttr.keyattr.label[10] = label11;
			//label12
			lv_obj_t *label12 = lv_label_create(list, NULL);
			lv_label_set_style(label12, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label12, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label12, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label12, lv_obj_get_width(list));
			lv_obj_set_height(label12, lv_obj_get_width(list)/4);
			lv_obj_align(label12, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label12, "Network County:cn");
			pocCitAttr.keyattr.label[11] = label12;
			//label13
			lv_obj_t *label13 = lv_label_create(list, NULL);
			lv_label_set_style(label13, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label13, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label13, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label13, lv_obj_get_width(list));
			lv_obj_set_height(label13, lv_obj_get_width(list)/4);
			lv_obj_align(label13, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label13, "Network Operator:46000");
			pocCitAttr.keyattr.label[12] = label13;
			//label14
			lv_obj_t *label14 = lv_label_create(list, NULL);
			lv_label_set_style(label14, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label14, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label14, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label14, lv_obj_get_width(list));
			lv_obj_set_height(label14, lv_obj_get_width(list)/4);
			lv_obj_align(label14, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label14, "Network Type:lte");
			pocCitAttr.keyattr.label[13] = label14;

			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 20000, LV_TASK_PRIO_MID, (void *)&pocCitAttr);
				lv_task_ready(pocCitAttr.refresh_task);
			}
			break;
		}
#ifdef CONFIG_POC_GUI_TOUCH_SUPPORT
		case LV_POC_CIT_OPRATOR_TYPE_TOUCH:
		{
			if(cit_test_info->cit_touch_attr.cb == NULL)
			{
				break;
			}
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_style_t * style_label;

			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_set_height(label, lv_obj_get_width(list)/4);
			lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
			lv_label_set_text(label, "请观察手电筒\n是否开启.");
			cit_test_info->cit_touch_attr.cb(true);
			break;
		}
#endif
		case LV_POC_CIT_OPRATOR_TYPE_RGB:
		{
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_style_t * style_label;

			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_set_height(label, lv_obj_get_width(list)/4);
			lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
			lv_label_set_text(label, "三色灯是否\n全部有效?");
			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 3000, LV_TASK_PRIO_MID, (void *)NULL);
				lv_task_ready(pocCitAttr.refresh_task);
			}
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_HEADSET:
		{
			if(cit_test_info->cit_headset_attr.cb == NULL)
			{
				break;
			}
			lv_style_t * style_label;
			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label, "1.请插入耳机.");
			pocCitAttr.keyattr.label[0] = label;
			//pre
			lv_obj_t *label2 = lv_label_create(list, NULL);
			lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label2, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label2, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label2, lv_obj_get_width(list));
			lv_obj_set_height(label2, lv_obj_get_width(list)/4);
			lv_obj_align(label2, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label2, "2.请按下耳机上的\nPPT开始讲话录音.");
			pocCitAttr.keyattr.label[1] = label2;
			//rel
			lv_obj_t *label3 = lv_label_create(list, NULL);
			lv_label_set_style(label3, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label3, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label3, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label3, lv_obj_get_width(list));
			lv_obj_set_height(label3, lv_obj_get_width(list)/4);
			lv_obj_align(label3, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label3, "3.松开耳机上的PPT\n键，播放录音内容.");
			pocCitAttr.keyattr.label[2] = label3;
			cit_test_info->cit_headset_attr.cb ? cit_test_info->cit_headset_attr.cb(true) : 0;//loopback
			break;
		}

		case LV_POC_CIT_OPRATOR_TYPE_FLASH:
		{
			lv_style_t * style_label;
			//total memory
			poc_setting_conf = lv_poc_setting_conf_read();
			style_label = ( lv_style_t * )poc_setting_conf->theme.current_theme->style_fota_label;//no use dead(style_cit_label)
			style_label->text.font = (lv_font_t *)poc_setting_conf->font.cit_label_current_font;
			lv_obj_t *label = lv_label_create(list, NULL);
			lv_label_set_style(label, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label, lv_obj_get_width(list));
			lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label, "总内存:8 MB");
			pocCitAttr.keyattr.label[0] = label;
			//used memory
			lv_obj_t *label2 = lv_label_create(list, NULL);
			lv_label_set_style(label2, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label2, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label2, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label2, lv_obj_get_width(list));
			lv_obj_align(label2, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label2, "");
			pocCitAttr.keyattr.label[1] = label2;
			//unused memory
			lv_obj_t *label3 = lv_label_create(list, NULL);
			lv_label_set_style(label3, LV_LABEL_STYLE_MAIN, style_label);
			lv_label_set_long_mode(label3, LV_LABEL_LONG_SROLL_CIRC);
			lv_label_set_align(label3, LV_LABEL_ALIGN_LEFT);
			lv_obj_set_width(label3, lv_obj_get_width(list));
			lv_obj_set_height(label3, lv_obj_get_width(list)/4);
			lv_obj_align(label3, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
			lv_label_set_text(label3, "");
			pocCitAttr.keyattr.label[2] = label3;

			if(pocCitAttr.refresh_task == NULL)
			{
				pocCitAttr.refresh_task = lv_task_create(lv_poc_cit_refresh_cb, 2000, LV_TASK_PRIO_MID, (void *)&pocCitAttr);
				lv_task_ready(pocCitAttr.refresh_task);
			}
			break;
		}

		default:
		{
			break;
		}
	}

}

static lv_res_t signal_func(struct _lv_obj_t * obj, lv_signal_t sign, void * param)
{
	switch(sign)
	{
		case LV_SIGNAL_CONTROL:
		{
			if(param == NULL) return LV_RES_OK;
			unsigned int c = *(unsigned int *)param;
			switch(c)
			{
				case LV_GROUP_KEY_ENTER:
				{
					if(pocCitAttr.refresh_task != NULL)
					{
						lv_task_del(pocCitAttr.refresh_task);
						pocCitAttr.refresh_task = NULL;
					}
					lv_poc_del_activity(poc_cit_test_ui_activity);
					lvPocCitAutoTestCom_Msg(LV_POC_CIT_AUTO_TEST_TYPE_SUCCESS);
					break;
				}
				case LV_KEY_ESC:
				{
					if(pocCitAttr.refresh_task != NULL)
					{
						lv_task_del(pocCitAttr.refresh_task);
						pocCitAttr.refresh_task = NULL;
					}
					lv_poc_del_activity(poc_cit_test_ui_activity);
					lvPocCitAutoTestCom_Msg(LV_POC_CIT_AUTO_TEST_TYPE_FAILED);
					break;
				}

				case LV_GROUP_KEY_DOWN:

				case LV_GROUP_KEY_UP:
				{
					lv_signal_send(activity_list, LV_SIGNAL_CONTROL, param);
					break;
				}

				case LV_GROUP_KEY_GP:
				{

					break;
				}

				case LV_GROUP_KEY_MB:
				{

					break;
				}

				case LV_GROUP_KEY_VOL_DOWN:
				{
					break;
				}

				case LV_GROUP_KEY_VOL_UP:
				{
					break;
				}

				case LV_GROUP_KEY_POC:
				{
					break;
				}
			}
			break;
		}

		case LV_SIGNAL_FOCUS:
		{
			break;
		}

		case LV_SIGNAL_DEFOCUS:
		{
			break;
		}

		default:
		{
			break;
		}
	}
	return LV_RES_OK;
}

static bool design_func(struct _lv_obj_t * obj, const lv_area_t * mask_p, lv_design_mode_t mode)
{
	return true;
}


void lv_poc_test_ui_open(void *info)
{
    lv_poc_activity_ext_t activity_ext = {ACT_ID_POC_CIT_TEST_UI,
											activity_create,
											activity_destory};
	lv_poc_control_text_t control_text ={
		.left_text   = "通过",
		.middle_text = "",
		.right_text  = "失败",
		};
	if(poc_cit_test_ui_activity != NULL)
	{
		return;
	}

	memset(&pocCitAttr, 0, sizeof(lv_poc_cit_test_t));
	pocCitAttr.charge_first = 100;
	cit_test_info = info;
    poc_cit_test_ui_activity = lv_poc_create_activity(&activity_ext, true, true, &control_text);
    lv_poc_activity_set_signal_cb(poc_cit_test_ui_activity, signal_func);
    lv_poc_activity_set_design_cb(poc_cit_test_ui_activity, design_func);
}

#ifdef __cplusplus
}
#endif

