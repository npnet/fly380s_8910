#include "poc_config.h"
#include "lv_include/lv_poc_lib.h"
#include "lv_objx/lv_poc_obj/lv_poc_font_resource.h"
#include "lv_objx/lv_poc_obj/lv_poc_obj.h"
#include "lv_gui_main.h"
#include "osi_api.h"
#include "time.h"
#include "vfs.h"
#include "drv_charger.h"
#include "cfw.h"
#include "drv_lcd_v2.h"
#include "audio_player.h"
#include "audio_device.h"
#include "nvm.h"
#include "tts_player.h"
#include "ml.h"
#include <stdlib.h>
#include "hal_iomux.h"
#include "drv_gpio.h"



static nv_poc_setting_msg_t poc_setting_conf_local = {0};
static nv_poc_theme_msg_node_t theme_white = {0};
static nv_poc_theme_msg_node_t theme_black = {0};

#define POC_SETTING_CONFIG_FILENAME CONFIG_FS_AP_NVM_DIR "/poc_setting_config.nv"
#define POC_SETTING_CONFIG_FILESIZE (sizeof(nv_poc_setting_msg_t))
#define POC_SETTING_CONFIG_BUFFER (&poc_setting_conf_local)
static bool nv_poc_setting_config_is_writed = false;
static drvGpio_t * poc_torch_gpio = NULL;
static drvGpio_t * poc_keypad_led_gpio = NULL;
static drvGpio_t * poc_ext_pa_gpio = NULL;

/*
      name : lv_poc_get_keypad_dev
    return : get keypad indev
      date : 2020-04-22
*/
OUT lv_indev_t *
lv_poc_get_keypad_dev(void)
{
	return lvGuiGetKeyPad();
}

/*
      name : lv_poc_setting_conf_init
    return : bool
      date : 2020-03-30
*/
OUT bool
lv_poc_setting_conf_init(void)
{
	static bool isInit = false;
	if(isInit) return true;
	isInit = true;
	char * path = POC_SETTING_CONFIG_FILENAME;
	int32_t size = POC_SETTING_CONFIG_FILESIZE;
	int fd = -1;
	int file_size = -1;
	file_size = vfs_file_size(path);
	if(file_size != size)
	{
		if(file_size >= 0)
		{
			vfs_unlink(path);
		}
	    fd = vfs_creat(path, O_CREAT);
	    if(fd < 0)
	    {
		    goto lv_poc_setting_nvm_failed;
	    }
	    vfs_close(fd);

	    memset(&poc_setting_conf_local, 0, size);
	    poc_mmi_poc_setting_config(POC_SETTING_CONFIG_BUFFER);
	}
	else
	{
		file_size = vfs_file_read(path, POC_SETTING_CONFIG_BUFFER, POC_SETTING_CONFIG_FILESIZE);
		if(file_size != size)
	    {
		    goto lv_poc_setting_nvm_failed;
	    }
		poc_mmi_poc_setting_config_restart(POC_SETTING_CONFIG_BUFFER);
	}

    if(vfs_sfile_init(path) < 0)
    {
	    goto lv_poc_setting_nvm_failed;
    }

    file_size = vfs_sfile_write(path, POC_SETTING_CONFIG_BUFFER, POC_SETTING_CONFIG_FILESIZE);
	if(file_size != size)
    {
	    goto lv_poc_setting_nvm_failed;
    }
	return true;

lv_poc_setting_nvm_failed:
	poc_mmi_poc_setting_config(POC_SETTING_CONFIG_BUFFER);
	return false;
}

/*
      name : lv_poc_setting_conf_read
    return : point a buff
      date : 2020-03-30
*/
OUT nv_poc_setting_msg_t *
lv_poc_setting_conf_read(void)
{
	if(nv_poc_setting_config_is_writed)
	{
		nv_poc_setting_config_is_writed = false;
		vfs_sfile_read(POC_SETTING_CONFIG_FILENAME, POC_SETTING_CONFIG_BUFFER, POC_SETTING_CONFIG_FILESIZE);
	}

    return POC_SETTING_CONFIG_BUFFER;
}

/*
      name : lv_poc_setting_conf_write
    return : length of buff
      date : 2020-03-30
*/
OUT int32_t
lv_poc_setting_conf_write(void)
{
	int file_size = -1;
	file_size = vfs_sfile_write(POC_SETTING_CONFIG_FILENAME, POC_SETTING_CONFIG_BUFFER, POC_SETTING_CONFIG_FILESIZE);
	nv_poc_setting_config_is_writed = true;
    return file_size;
}

/*
      name : lv_poc_setting_get_current_volume
     param : POC_MMI_VOICE_TYPE_E
    return : current volum
      date : 2020-03-30
*/
OUT uint8_t
lv_poc_setting_get_current_volume(IN POC_MMI_VOICE_TYPE_E type)
{
	nv_poc_setting_msg_t * config = lv_poc_setting_conf_read();
	if(type == POC_MMI_VOICE_MSG)
	{
		return config->volume;
	}
	else if(type == POC_MMI_VOICE_PLAY)
	{
		return config->volume;
	}
	return 0xff;
}

/*
      name : lv_poc_setting_set_current_volume
     param :
     			[type]  POC_MMI_VOICE_TYPE_E
     			[volum] config volum
     			[play]  need paly msg audio
      date : 2020-03-30
*/
void
lv_poc_setting_set_current_volume(IN POC_MMI_VOICE_TYPE_E type, IN uint8_t volume, IN bool play)
{
	uint8_t poc_volum = volume * 10;
	if(poc_volum > 100)
	{
		poc_volum = 100;
	}
	else if(poc_volum < 1)
	{
		poc_volum = 0;
	}
	nv_poc_setting_msg_t * config = lv_poc_setting_conf_read();
	if(type == POC_MMI_VOICE_MSG)
	{
		config->volume = poc_volum / 10;
		audevSetVoiceVolume(poc_volum);
	}
	else if(type == POC_MMI_VOICE_PLAY)
	{
		config->volume = poc_volum / 10;
		audevSetPlayVolume(poc_volum);
	}
	lv_poc_setting_conf_write();

	if(play && !ttsIsPlaying())
	{
		poc_play_btn_voice_one_time(config->volume, play);
	}
}

/*
      name : lv_poc_get_time
    return : get local time
      date : 2020-03-30
*/
void
lv_poc_get_time(OUT lv_poc_time_t * time)
{
	if(time == NULL)
	{
		return;
	}
    struct tm system_time;
    time_t lt = osiEpochSecond();
    gmtime_r(&lt, &system_time);
    system_time.tm_hour = system_time.tm_hour + 8;
	if(system_time.tm_hour > 23)
	{
		system_time.tm_hour = system_time.tm_hour % 24;
		system_time.tm_mday = system_time.tm_mday + 1;
		switch(system_time.tm_mon)
		{
			case 1:
			case 3:
			case 5:
			case 7:
			case 8:
			case 10:
				if(system_time.tm_mday > 31)
				{
					system_time.tm_mday = system_time.tm_mday % 32 + 1;
					system_time.tm_mon = system_time.tm_mon + 1;
				}
				break;

			case 4:
			case 6:
			case 9:
			case 11:
				if(system_time.tm_mday > 30)
				{
					system_time.tm_mday = system_time.tm_mday % 31 + 1;
					system_time.tm_mon = system_time.tm_mon + 1;
				}
				break;

			case 2:
				if(system_time.tm_year % 400 == 0 || (system_time.tm_year % 100 != 0 && system_time.tm_year % 4 == 0))
				{
					if(system_time.tm_mday > 29)
					{
						system_time.tm_mday = system_time.tm_mday % 30 + 1;
						system_time.tm_mon = system_time.tm_mon + 1;
					}
				}
				else
				{
					if(system_time.tm_mday > 28)
					{
						system_time.tm_mday = system_time.tm_mday % 29 + 1;
						system_time.tm_mon = system_time.tm_mon + 1;
					}
				}

				break;
			case 12:
				if(system_time.tm_mday > 31)
				{
					system_time.tm_mday = system_time.tm_mday % 32 + 1;
					system_time.tm_mon = system_time.tm_mon % 13 + 1;
					system_time.tm_year = system_time.tm_year + 1;
				}
				break;
			default:
				break;
		}
	}

	time->tm_year = system_time.tm_year;
	time->tm_mon = system_time.tm_mon;
	time->tm_mday = system_time.tm_mday;
	time->tm_hour = system_time.tm_hour;
	time->tm_min = system_time.tm_min;
	time->tm_sec = system_time.tm_sec;
	time->tm_wday = system_time.tm_wday;
}

/*
      name : poc_set_lcd_bright_time
     param : config lcd bright time
             [0]5秒 [1]15秒 [2]30秒 [3]1分钟 [4]2分钟 [5]5分钟 [6]10分钟 [7]30分钟     [default 2]
      date : 2020-03-30
*/
void
poc_set_lcd_bright_time(IN uint8_t timeout)
{
	unsigned bright_time[8] = {5000, 15000, 30000, 60000, 120000, 300000, 600000, 1800000};
	uint8_t index = 0;
	if(timeout > 7)
	{
		index = 7;
	}
	else if(timeout < 1)
	{
		index = 0;
	}
	else
	{
		index = timeout;
	}
	lvGuiSetInactiveTimeout(bright_time[index]);
}

/*
      name : poc_set_lcd_status
     param : config lcd state
      date : 2020-03-30
*/
void
poc_set_lcd_status(IN int8_t wakeup)
{
	if(wakeup != 0)
	{
		lvGuiScreenOn();
	}
	else
	{
		lvGuiScreenOff();
	}
}

/*
      name : poc_get_lcd_status
    return : get lcd state
      date : 2020-03-30
*/
OUT bool
poc_get_lcd_status(void)
{
	return lvGuiGetScreenStatus();
}

/*
      name : poc_set_lcd_blacklight
    return : set lcd blacklight
      date : 2020-03-30
*/
void
poc_set_lcd_blacklight(IN int8_t blacklight)
{
	drvLcdSetBackLightNess(NULL, blacklight);
}

/*
      name : poc_play_btn_voice_one_time
     param :
     			[volum] volum when playing
     			[quiet] quiet mode
      date : 2020-03-30
*/
void
poc_play_btn_voice_one_time(IN int8_t volum, IN bool quiet)
{
	static auPlayer_t * player = NULL;
	if(!quiet)
	{
		extern lv_poc_audio_dsc_t lv_poc_audio_msg;
		if(NULL == player)
		{
			player = auPlayerCreate();
			if(NULL == player)
			{
				return;
			}
		}
		auPlayerStop(player);
	    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
	    auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
		auPlayerStartMem(player, AUSTREAM_FORMAT_PCM, params, lv_poc_audio_msg.data, lv_poc_audio_msg.data_size);
	}
}

/*
      name : poc_battery_get_status
     param : point a battery buff
      date : 2020-03-30
*/
void
poc_battery_get_status(OUT battery_values_t *values)
{
	int8_t status[16];
	memset(status, 0, sizeof(int8_t) * 16);
	drvChargerGetStatus(status);

	values->battery_status = status[0];

	values->battery_value = status[2];

	if(status[1] == 0 || status[1] == 3)
	{
		values->charging = POC_CHG_DISCONNECTED;
	}
	else if(status[1] == 1 || status[1] == 2)
	{
		values->charging = POC_CHG_CONNECTED;
	}
	else
	{
		values->charging = POC_CHG_DISCONNECTED;
	}

	uint32_t * temp = (uint32_t *)(&(status[4]));
	values->charge_cur_mA = *temp;
	temp = (uint32_t *)(&(status[8]));
	values->battery_val_mV = *temp;
	temp = (uint32_t *)(&(status[12]));
	values->battery_temp = *temp;
}

/*
      name : poc_check_sim_prsent
     param : check SIM
    return :
    			[true]  SIM is present
    			[false] SIM isn't present
      date : 2020-03-30
*/
OUT bool
poc_check_sim_prsent(IN POC_SIM_ID sim)
{
	CFW_SIM_STATUS status = CFW_GetSimStatus(POC_SIM_1);

    if (status != 0x04)
    {
        if (status == CFW_SIM_ABSENT)
            return false;
        else if (status == CFW_SIM_NORMAL)
            return true;
        else if (status == CFW_SIM_TEST)
            return true;
        else if (status == CFW_SIM_ABNORMAL)
            return true;
    }
    return false;
}

/*
      name : poc_get_signal_bar_strenth
     param : SIM_ID
    return : POC_MMI_MODEM_SIGNAL_BAR
      date : 2020-03-30
*/
OUT POC_MMI_MODEM_SIGNAL_BAR
poc_get_signal_bar_strenth(POC_SIM_ID sim)
{
	uint8_t nSignalLevel = 99;
	uint8_t nBitError = 99;
	uint32_t ret = 0;
	uint8_t rat = 0;
	uint8_t nSim = POC_SIM_1;
	static POC_MMI_MODEM_SIGNAL_BAR _signal_bar = MMI_MODEM_SIGNAL_BAR_0;

	CFW_COMM_MODE nFM = CFW_DISABLE_COMM;
	ret = CFW_GetComm(&nFM, nSim);
	if (nFM != CFW_DISABLE_COMM)
	{
		rat = CFW_NWGetStackRat(nSim);
		if (rat == 4)
		{
			CFW_NW_QUAL_INFO iQualReport;

			ret = CFW_NwGetLteSignalQuality(&nSignalLevel, &nBitError, nSim);
			if (ret != 0)
			{
				goto LV_POC_GET_SIGNAL_BAR_ENDLE;
			}
			ret = CFW_NwGetQualReport(&iQualReport, nSim);
			if (ret != 0)
			{
				goto LV_POC_GET_SIGNAL_BAR_ENDLE;
			}
			if (iQualReport.nRssidBm < -113)
			{
				nSignalLevel = 0;
			}
			else if ((iQualReport.nRssidBm >= -113) && (iQualReport.nRssidBm <= -51))
			{
				nSignalLevel = (uint8_t)((iQualReport.nRssidBm + 113) / 2);
			}
			else
			{
				nSignalLevel = 31;
			}
		}
		else
		{
			ret = CFW_NwGetSignalQuality(&nSignalLevel, &nBitError, nSim);
			if (nSignalLevel > 113)
			{
				nSignalLevel = 0;
			}
			else if ((nSignalLevel <= 113) && (nSignalLevel >= 51))
			{
				nSignalLevel = (uint8_t)(31 - (nSignalLevel - 51) / 2);
			}
			else
			{
				nSignalLevel = 31;
			}
		}
		if (ret != 0)
			goto LV_POC_GET_SIGNAL_BAR_ENDLE;
	}

	if(nSignalLevel == 99 || nSignalLevel < 5)
	{
		_signal_bar = MMI_MODEM_SIGNAL_BAR_0;
	}
	else if(nSignalLevel >= 25)
	{
		_signal_bar = MMI_MODEM_SIGNAL_BAR_5;
	}
	else if(nSignalLevel >= 21)
	{
		_signal_bar = MMI_MODEM_SIGNAL_BAR_4;
	}
	else if(nSignalLevel >= 17)
	{
		_signal_bar = MMI_MODEM_SIGNAL_BAR_3;
	}
	else if(nSignalLevel >= 13)
	{
		_signal_bar = MMI_MODEM_SIGNAL_BAR_2;
	}
	else
	{
		_signal_bar = MMI_MODEM_SIGNAL_BAR_1;
	}

LV_POC_GET_SIGNAL_BAR_ENDLE:
	return _signal_bar;
}

/*
      name : get Operator(short name)
     param :
     			[sim] POC_SIM_ID
         		[operator]
         		[rat] network type
  return  void
*/
void
poc_get_operator_req(IN POC_SIM_ID sim, OUT int8_t * operat, OUT POC_MMI_MODEM_PLMN_RAT * rat)
{
	uint32_t ret = 0;
	uint8_t _rat = 0;
	uint8_t nSim = POC_SIM_1;
	static POC_MMI_MODEM_PLMN_RAT _signal_type = MMI_MODEM_PLMN_RAT_UNKNOW;
	if(operat == NULL || rat == NULL) return;

	CFW_COMM_MODE nFM = CFW_DISABLE_COMM;
	ret = CFW_GetComm(&nFM, nSim);
	if (nFM != CFW_DISABLE_COMM)
	{
		if (ret != 0)
		{
			strcpy((char *)operat, "UN");
			_signal_type = MMI_MODEM_PLMN_RAT_UNKNOW;
			goto LV_POC_GET_SIGNAL_TYPR_ENDLE;
		}
		_rat = CFW_NWGetStackRat(nSim);
		if (_rat == 4)
		{
			strcpy((char *)operat, "LTE");
			_signal_type = MMI_MODEM_PLMN_RAT_LTE;
		}
		else
		{
			strcpy((char *)operat, "GSM");
			_signal_type = MMI_MODEM_PLMN_RAT_GSM;
		}
	}

LV_POC_GET_SIGNAL_TYPR_ENDLE:
	*rat = _signal_type;
}

/*
      name : get status of ps attach
     param :
     			[sim] POC_SIM_ID
      date : 2020-05-06
*/
bool
poc_get_network_ps_attach_status(IN POC_SIM_ID sim)
{
    uint32_t ret;
    uint8_t uState = 0;
    uint8_t nSim = POC_SIM_1;
    ret = CFW_GetGprsAttState(&uState, nSim);
    if (ret != 0)
        return false;
    return uState? true:false;
}

/*
      name : get status of register network
     param :
     			[sim] POC_SIM_ID
      date : 2020-05-06
*/
bool
poc_get_network_register_status(IN POC_SIM_ID sim)
{
	CFW_NW_STATUS_INFO nStatusInfo;
    uint8_t nSim = POC_SIM_1;
	uint8_t status;

	if (CFW_CfgGetNwStatus(&status, nSim) != 0 ||
		CFW_NwGetStatus(&nStatusInfo, nSim) != 0)
	{
		return false;
	}

	if(nStatusInfo.nStatus == 0 || nStatusInfo.nStatus == 2 || nStatusInfo.nStatus == 4)
	{
		return false;
	}
	return true;
}



static void
prv_poc_mmi_poc_setting_config_const(OUT nv_poc_setting_msg_t * poc_setting)
{
#if 0
	poc_setting->font.list_btn_big_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.list_btn_small_font = (uint32_t)&lv_poc_font_noto_sans_cjk_16;
	poc_setting->font.about_label_big_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	poc_setting->font.about_label_small_font = (uint32_t)&lv_poc_font_noto_sans_cjk_14;
	poc_setting->font.win_title_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	poc_setting->font.activity_control_font = (uint32_t)&lv_poc_font_noto_sans_cjk_16;
	poc_setting->font.status_bar_time_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	//poc_setting->font.status_bar_signal_type_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.idle_big_clock_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.idle_date_label_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.idle_page2_msg_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
#elif 1
	poc_setting->font.list_btn_big_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	poc_setting->font.list_btn_small_font = (uint32_t)&lv_poc_font_noto_sans_cjk_16;
	poc_setting->font.about_label_big_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	poc_setting->font.about_label_small_font = (uint32_t)&lv_poc_font_noto_sans_cjk_14;
	poc_setting->font.win_title_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	poc_setting->font.activity_control_font = (uint32_t)&lv_poc_font_noto_sans_cjk_16;
	poc_setting->font.status_bar_time_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	//poc_setting->font.status_bar_signal_type_font = (uint32_t)&lv_poc_font_noto_sans_cjk_20;
	poc_setting->font.idle_big_clock_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	poc_setting->font.idle_date_label_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
	poc_setting->font.idle_page2_msg_font = (uint32_t)&lv_poc_font_noto_sans_cjk_18;
#endif

	poc_setting->theme.white = &theme_white;
	poc_setting->theme.white->style_list_scroll = (uint32_t)&theme_white_style_list_scroll;
	poc_setting->theme.white->style_list_page = (uint32_t)&theme_white_style_list_page;
	poc_setting->theme.white->style_list_btn_rel = (uint32_t)&theme_white_style_list_btn_rel;
	poc_setting->theme.white->style_list_btn_pr = (uint32_t)&theme_white_style_list_btn_pr;
	poc_setting->theme.white->style_list_btn_ina = (uint32_t)&theme_white_style_list_btn_ina;
	poc_setting->theme.white->style_idle_big_clock = (uint32_t)&theme_white_style_idle_big_clock;
	poc_setting->theme.white->style_idle_date_label = (uint32_t)&theme_white_style_idle_date_label;
	poc_setting->theme.white->style_idle_msg_label = (uint32_t)&theme_white_style_idle_msg_label;
	poc_setting->theme.white->style_about_label = (uint32_t)&theme_white_style_about_label;
	poc_setting->theme.white->style_cb = (uint32_t)&theme_white_style_rb;
	poc_setting->theme.white->style_rb = (uint32_t)&theme_white_style_cb;
	poc_setting->theme.white->style_switch_bg = (uint32_t)&theme_white_style_switch_bg;
	poc_setting->theme.white->style_switch_indic = (uint32_t)&theme_white_style_switch_indic;
	poc_setting->theme.white->style_switch_knob_off = (uint32_t)&theme_white_style_switch_knob_off;
	poc_setting->theme.white->style_switch_knob_on = (uint32_t)&theme_white_style_switch_knob_on;
	poc_setting->theme.white->style_status_bar = (uint32_t)&theme_white_style_status_bar;
	poc_setting->theme.white->style_status_bar_time = (uint32_t)&theme_white_style_status_bar_time;
	poc_setting->theme.white->style_win_header = (uint32_t)&theme_white_style_win_header;
	poc_setting->theme.white->style_control = (uint32_t)&theme_white_style_control;


	poc_setting->theme.black = &theme_black;
	poc_setting->theme.black->style_list_scroll = (uint32_t)&theme_black_style_list_scroll;
	poc_setting->theme.black->style_list_page = (uint32_t)&theme_black_style_list_page;
	poc_setting->theme.black->style_list_btn_rel = (uint32_t)&theme_black_style_list_btn_rel;
	poc_setting->theme.black->style_list_btn_pr = (uint32_t)&theme_black_style_list_btn_pr;
	poc_setting->theme.black->style_list_btn_ina = (uint32_t)&theme_black_style_list_btn_ina;
	poc_setting->theme.black->style_idle_big_clock = (uint32_t)&theme_black_style_idle_big_clock;
	poc_setting->theme.black->style_idle_date_label = (uint32_t)&theme_black_style_idle_date_label;
	poc_setting->theme.black->style_idle_msg_label = (uint32_t)&theme_black_style_idle_msg_label;
	poc_setting->theme.black->style_about_label = (uint32_t)&theme_black_style_about_label;
	poc_setting->theme.black->style_cb = (uint32_t)&theme_black_style_rb;
	poc_setting->theme.black->style_rb = (uint32_t)&theme_black_style_cb;
	poc_setting->theme.black->style_switch_bg = (uint32_t)&theme_black_style_switch_bg;
	poc_setting->theme.black->style_switch_indic = (uint32_t)&theme_black_style_switch_indic;
	poc_setting->theme.black->style_switch_knob_off = (uint32_t)&theme_black_style_switch_knob_off;
	poc_setting->theme.black->style_switch_knob_on = (uint32_t)&theme_black_style_switch_knob_on;
	poc_setting->theme.black->style_status_bar = (uint32_t)&theme_black_style_status_bar;
	poc_setting->theme.black->style_status_bar_time = (uint32_t)&theme_black_style_status_bar_time;
	poc_setting->theme.black->style_win_header = (uint32_t)&theme_black_style_win_header;
	poc_setting->theme.black->style_control = (uint32_t)&theme_black_style_control;
}

/*
      name : poc_mmi_poc_setting_config
     param : [poc_setting] IN param
      date : 2020-03-30
*/
void
poc_mmi_poc_setting_config(OUT nv_poc_setting_msg_t * poc_setting)
{
	prv_poc_mmi_poc_setting_config_const(poc_setting);
	poc_setting->theme.type = 0;
	poc_setting->theme.current_theme = poc_setting->theme.white;
	poc_setting->read_and_write_check = 0x3f;
	poc_setting->btn_voice_switch = 0;
	poc_setting->voice_broadcast_switch = 0;
	poc_setting->keypad_led_switch = 0;
	poc_setting->GPS_switch = 0;
	poc_setting->electric_torch_switch = 0;
	poc_setting->screen_brightness = 4;
	poc_setting->screen_bright_time = 2;
	//poc_setting->current_theme = 0;
	poc_setting->main_SIM = 0;
	poc_setting->net_type = 0;
	poc_setting->font.big_font_switch = 1;
	poc_setting->font.list_page_colum_count = 3;
	poc_setting->font.list_btn_current_font = poc_setting->font.list_btn_big_font;
	poc_setting->font.about_label_current_font = poc_setting->font.about_label_big_font;
	poc_setting->volume = 5;
	poc_setting->language = 0;
}

/*
      name : poc_mmi_poc_setting_config_restart
     param : [poc_setting] IN param
      date : 2020-03-30
*/
void
poc_mmi_poc_setting_config_restart(OUT nv_poc_setting_msg_t * poc_setting)
{
	prv_poc_mmi_poc_setting_config_const(poc_setting);

#if 1
	if(poc_setting->theme.type == 0)
	{
		poc_setting->theme.current_theme = poc_setting->theme.white;
	}
	else if(poc_setting->theme.type == 1)
	{
		poc_setting->theme.current_theme = poc_setting->theme.black;
	}

	if(poc_setting->font.big_font_switch == 0)
	{
		poc_setting->font.list_page_colum_count = 4;
		poc_setting->font.list_btn_current_font = poc_setting->font.list_btn_small_font;
		poc_setting->font.about_label_current_font = poc_setting->font.about_label_small_font;
	}
	else if(poc_setting->font.big_font_switch == 1)
	{
		poc_setting->font.list_page_colum_count = 3;
		poc_setting->font.list_btn_current_font = poc_setting->font.list_btn_big_font;
		poc_setting->font.about_label_current_font = poc_setting->font.about_label_big_font;
	}
#endif
}

/*
      name : poc_sys_delay
     param : time in Ms
      date : 2020-03-30
*/
void
poc_sys_delay(IN uint32_t ms)
{
	//SDL_Delay(ms);
}

/*
      name : poc_get_device_imei_rep
     param : get device imei
      date : 2020-03-30
*/
void
poc_get_device_imei_rep(OUT int8_t * imei)
{
    uint8_t nSim = POC_SIM_1;

    uint8_t nImei[16] = {};
    int nImeiLen = nvmReadImei(nSim, (nvmImei_t *)nImei);
    if (nImeiLen == -1)
    {
	    memset(nImei, 0, 16);
    }
	strcpy((char *)imei, (const char *)nImei);
}

/*
      name : poc_get_device_iccid_rep
     param : get device iccid
      date : 2020-03-30
*/
void
poc_get_device_iccid_rep(int8_t * iccid)
{
	uint8_t nSim = POC_SIM_1;
    uint8_t ICCID[21] = {0};
    uint8_t *pICCID = CFW_GetICCID(nSim);
    if (pICCID != NULL)
    {
	    extern uint16_t atConvertICCID(uint8_t *pInput, uint16_t nInputLen, uint8_t *pOutput);
        atConvertICCID(pICCID, 10, ICCID);
    }
    else
    {
		memset(ICCID, 0, 21);
    }
    OSI_LOGI(0, "[poc][ext_lib][trace] %s %s", ICCID, pICCID);
    strcpy((char *)iccid, (const char *)ICCID);
}

/*
      name : poc_get_device_account_rep
     param : get device account
      date : 2020-04-29
*/
char *
poc_get_device_account_rep(POC_SIM_ID nSim)
{
	static char * name = NULL;
	if(name == NULL)
	{
		name = (char *)malloc(sizeof(char) * 64);
		if(name == NULL)
		{
			return NULL;
		}
	}
	strcpy(name, "飞图测试1");
	return name;
}

/*
      name : poc_broadcast_play_rep
     param : play text in some method
      date : 2020-04-29
*/
bool
poc_broadcast_play_rep(uint8_t * text, uint32_t length, uint8_t play, bool force)
{
	if(!play)
	{
		return false;
	}

	if(length < 1 || text == NULL)
	{
		return false;
	}

	if(ttsIsPlaying())
	{
		if(!force)
		{
			return false;
		}
		ttsStop();
	}
	char * test_text = "1234567890";
	return ttsPlayText(test_text, strlen(test_text), ML_UTF8);
}

/*
      name : poc_set_torch_status
     param : open  true is open torch
      date : 2020-04-30
*/
bool
poc_set_torch_status(bool open)
{
	poc_torch_init();
	drvGpioWrite(poc_torch_gpio, open);
	return open;
}

/*
      name : poc_set_torch_status
     param : open  true is open torch
      date : 2020-04-30
*/
bool
poc_get_torch_status(void)
{
	poc_torch_init();
	return drvGpioRead(poc_torch_gpio);
}

/*
      name : poc_torch_init
     param : none
      date : 2020-04-30
*/
void
poc_torch_init(void)
{
	if(poc_torch_gpio != NULL) return;
	static drvGpioConfig_t * config = NULL;
	if(config == NULL)
	{
		config = (drvGpioConfig_t *)calloc(1, sizeof(drvGpioConfig_t));
		if(config == NULL)
		{
			return;
		}
		memset(config, 0, sizeof(drvGpioConfig_t));
		config->mode = DRV_GPIO_OUTPUT;
		config->debounce = true;
		config->out_level = false;
	}
	poc_torch_gpio = drvGpioOpen(2, config, NULL, NULL);
}

/*
      name : poc_set_keypad_led_status
     param : open  true is open keypad led
      date : 2020-04-30
*/
bool
poc_set_keypad_led_status(bool open)
{
	poc_keypad_led_init();
	drvGpioWrite(poc_keypad_led_gpio, open);
	return open;
}

/*
      name : poc_get_keypad_led_status
     param : none
      date : 2020-04-30
*/
bool
poc_get_keypad_led_status(void)
{
	poc_keypad_led_init();
	return drvGpioRead(poc_keypad_led_gpio);
}

/*
      name : poc_keypad_led_init
     param : none
      date : 2020-04-30
*/
void
poc_keypad_led_init(void)
{
	if(poc_keypad_led_gpio != NULL) return;
	static drvGpioConfig_t * config = NULL;
	nv_poc_setting_msg_t * poc_setting = lv_poc_setting_conf_read();
	if(config == NULL)
	{
		config = (drvGpioConfig_t *)calloc(1, sizeof(drvGpioConfig_t));
		if(config == NULL)
		{
			return;
		}
		memset(config, 0, sizeof(drvGpioConfig_t));
		config->mode = DRV_GPIO_OUTPUT;
		config->debounce = true;
		config->out_level = poc_setting->keypad_led_switch;
	}
	poc_keypad_led_gpio = drvGpioOpen(15, config, NULL, NULL);
}

/*
      name : poc_set_PA_status
     param : open  true is open external PA
      date : 2020-04-30
*/
bool
poc_set_ext_pa_status(bool open)
{
	#define POC_EXT_PA_DELAY_US 2
	poc_ext_pa_init();
	if(poc_ext_pa_gpio == NULL) return false;
	if(open)
	{
		//one
		drvGpioWrite(poc_ext_pa_gpio, true);
		osiThreadSleepUS(POC_EXT_PA_DELAY_US);
		drvGpioWrite(poc_ext_pa_gpio, false);
		osiThreadSleepUS(POC_EXT_PA_DELAY_US);

		//two
		drvGpioWrite(poc_ext_pa_gpio, true);
		osiThreadSleepUS(POC_EXT_PA_DELAY_US);
		drvGpioWrite(poc_ext_pa_gpio, false);
		osiThreadSleepUS(POC_EXT_PA_DELAY_US);

		//three
		drvGpioWrite(poc_ext_pa_gpio, true);
		osiThreadSleepUS(POC_EXT_PA_DELAY_US);
		drvGpioWrite(poc_ext_pa_gpio, false);
		osiThreadSleepUS(POC_EXT_PA_DELAY_US);

		//four
		drvGpioWrite(poc_ext_pa_gpio, true);
	}
	else
	{
		drvGpioWrite(poc_ext_pa_gpio, false);
	}
	OSI_LOGI(0, "[poc][audio][PA] audio_device line <- %d - %d\n", __LINE__, open);
	return open;
}

/*
      name : poc_get_ext_pa_status
     param : none
      date : 2020-04-30
*/
bool
poc_get_ext_pa_status(void)
{
	poc_ext_pa_init();
	if(poc_ext_pa_gpio == NULL) return false;
	return drvGpioRead(poc_keypad_led_gpio);
}

/*
      name : poc_ext_pa_init
     param : none
      date : 2020-04-30
*/
void
poc_ext_pa_init(void)
{
	if(poc_ext_pa_gpio != NULL) return;
	static drvGpioConfig_t * config = NULL;
	if(config == NULL)
	{
		config = (drvGpioConfig_t *)calloc(1, sizeof(drvGpioConfig_t));
		if(config == NULL)
		{
			return;
		}
		memset(config, 0, sizeof(drvGpioConfig_t));
		config->mode = DRV_GPIO_OUTPUT;
		config->debounce = true;
		config->out_level = false;
	}
	poc_ext_pa_gpio = drvGpioOpen(9, config, NULL, NULL);
}
