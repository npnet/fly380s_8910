#include "poc_config.h"
#include "ats_config.h"
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
#include "lv_include/lv_poc_type.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

static nv_poc_setting_msg_t poc_setting_conf_local = {0};
static nv_poc_theme_msg_node_t theme_white = {0};
#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
static nv_poc_theme_msg_node_t theme_black = {0};
#endif

#define POC_SETTING_CONFIG_FILENAME CONFIG_FS_AP_NVM_DIR "/poc_setting_config.nv"
#define POC_SETTING_CONFIG_FILESIZE (sizeof(nv_poc_setting_msg_t))
#define POC_SETTING_CONFIG_BUFFER (&poc_setting_conf_local)
static bool nv_poc_setting_config_is_writed = false;
static drvGpio_t * poc_torch_gpio = NULL;
static drvGpio_t * poc_keypad_led_gpio = NULL;
static drvGpio_t * poc_ext_pa_gpio = NULL;
static drvGpio_t * poc_port_Gpio = NULL;
drvGpioConfig_t* configport = NULL;



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
	system_time.tm_mon = system_time.tm_mon + 1;/*优化月份 + 1 0-11*/
	if(system_time.tm_hour > 23)
	{
		system_time.tm_hour = system_time.tm_hour % 24;
		system_time.tm_mday = system_time.tm_mday + 1;
		switch(system_time.tm_mon)//月份0-11
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

static osiThread_t * prv_play_btn_voice_one_time_thread = NULL;
static auPlayer_t * prv_play_btn_voice_one_time_player = NULL;
static osiThread_t * prv_play_voice_one_time_thread = NULL;
static auPlayer_t * prv_play_voice_one_time_player = NULL;
extern lv_poc_audio_dsc_t lv_poc_audio_msg;
extern lv_poc_audio_dsc_t lv_poc_audio_start_machine;
extern lv_poc_audio_dsc_t lv_poc_audio_no_connected;
extern lv_poc_audio_dsc_t lv_poc_audio_no_login;
extern lv_poc_audio_dsc_t lv_poc_audio_fail_to_update_group;
extern lv_poc_audio_dsc_t lv_poc_audio_fail_to_update_member;
extern lv_poc_audio_dsc_t lv_poc_audio_success_to_build_group;
extern lv_poc_audio_dsc_t lv_poc_audio_low_battery;
extern lv_poc_audio_dsc_t lv_poc_audio_success_member_call;
extern lv_poc_audio_dsc_t lv_poc_audio_join_group;
extern lv_poc_audio_dsc_t lv_poc_audio_offline_member;
extern lv_poc_audio_dsc_t lv_poc_audio_insert_sim_card;
extern lv_poc_audio_dsc_t lv_poc_audio_success_to_login;
extern lv_poc_audio_dsc_t lv_poc_audio_tone_cannot_speak;
extern lv_poc_audio_dsc_t lv_poc_audio_tone_lost_mic;
extern lv_poc_audio_dsc_t lv_poc_audio_tone_note;
extern lv_poc_audio_dsc_t lv_poc_audio_tone_start_listen;
extern lv_poc_audio_dsc_t lv_poc_audio_tone_start_speak;
extern lv_poc_audio_dsc_t lv_poc_audio_tone_stop_listen;
extern lv_poc_audio_dsc_t lv_poc_audio_tone_stop_sepak;
static lv_poc_audio_dsc_t *prv_lv_poc_audio_array[] = {
	NULL,
	&lv_poc_audio_start_machine,
	&lv_poc_audio_fail_to_update_group,
	&lv_poc_audio_fail_to_update_member,
	&lv_poc_audio_insert_sim_card,
	&lv_poc_audio_join_group,
	&lv_poc_audio_low_battery,
	&lv_poc_audio_no_login,
	&lv_poc_audio_offline_member,
	&lv_poc_audio_success_member_call,
	&lv_poc_audio_success_to_build_group,
	&lv_poc_audio_success_to_login,
	&lv_poc_audio_no_connected,
	&lv_poc_audio_tone_cannot_speak,
	&lv_poc_audio_tone_lost_mic,
	&lv_poc_audio_tone_note,
	&lv_poc_audio_tone_start_listen,
	&lv_poc_audio_tone_start_speak,
	&lv_poc_audio_tone_stop_listen,
	&lv_poc_audio_tone_stop_sepak,
};

static void prv_play_btn_voice_one_time_thread_callback(void * ctx)
{
	do
	{
		if(NULL == prv_play_btn_voice_one_time_player)
		{
			prv_play_btn_voice_one_time_player = auPlayerCreate();
			if(NULL == prv_play_btn_voice_one_time_player)
			{
				break;
			}
		}
		auPlayerStop(prv_play_btn_voice_one_time_player);
	    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
	    auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
		auPlayerStartMem(prv_play_btn_voice_one_time_player, AUSTREAM_FORMAT_PCM, params, lv_poc_audio_msg.data, lv_poc_audio_msg.data_size);
		osiThreadSleep(140);
		auPlayerStop(prv_play_btn_voice_one_time_player);
	}while(0);
	prv_play_btn_voice_one_time_thread = NULL;
	osiThreadExit();
}

static void prv_play_voice_one_time_thread_callback(void * ctx)
{
    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
   auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
	bool isPlayVoice = false;
	osiEvent_t event = {0};
	LVPOCAUDIO_Type_e voice_queue[10] = {0};
	int voice_queue_reader = 0;
	int voice_queue_writer = 0;
	LVPOCAUDIO_Type_e voice_type = 0;
	auStreamFormat_t voice_formate = AUSTREAM_FORMAT_UNKNOWN;

	while(1)
{
		if(osiEventTryWait(prv_play_voice_one_time_thread, &event, 20))
		{
			if(event.id != 101)
			{
				continue;
			}

			voice_type = event.param1;

			if(event.param2 > 0)
			{
				if(isPlayVoice)
				{
					auPlayerStop(prv_play_voice_one_time_player);
					isPlayVoice = false;
				}
			}
			else if(isPlayVoice)
			{
				voice_queue[voice_queue_writer] = voice_type;
				voice_queue_writer = (voice_queue_writer + 1) % 10;
				continue;
			}
		}
		else
		{
			if(isPlayVoice)
			{
				if(auPlayerWaitFinish(prv_play_voice_one_time_player, 20))
				{
					auPlayerStop(prv_play_voice_one_time_player);
					isPlayVoice = false;
				}
				else
				{
					continue;
				}
			}

			if(voice_queue_reader == voice_queue_writer)
			{
				continue;
			}
			voice_type = voice_queue[voice_queue_reader];
			voice_queue_reader = (voice_queue_reader + 1) % 10;
		}

		if(voice_type <= LVPOCAUDIO_Type_Start_Index || voice_type >= LVPOCAUDIO_Type_End_Index)
		{
			continue;
		}

		switch(voice_type)
		{
			case LVPOCAUDIO_Type_Start_Machine:
			case LVPOCAUDIO_Type_Fail_Update_Group:
			case LVPOCAUDIO_Type_Fail_Update_Member:
			case LVPOCAUDIO_Type_Insert_SIM_Card:
			case LVPOCAUDIO_Type_Join_Group:
			case LVPOCAUDIO_Type_Low_Battery:
			case LVPOCAUDIO_Type_No_Login:
			case LVPOCAUDIO_Type_Offline_Member:
			case LVPOCAUDIO_Type_Success_Member_Call:
			case LVPOCAUDIO_Type_Success_Build_Group:
			case LVPOCAUDIO_Type_Success_Login:
			case LVPOCAUDIO_Type_No_Connected:
				voice_formate = AUSTREAM_FORMAT_MP3;
				break;
			case LVPOCAUDIO_Type_Tone_Cannot_Speak:
			case LVPOCAUDIO_Type_Tone_Lost_Mic:
			case LVPOCAUDIO_Type_Tone_Note:
			case LVPOCAUDIO_Type_Tone_Start_Listen:
			case LVPOCAUDIO_Type_Tone_Start_Speak:
			case LVPOCAUDIO_Type_Tone_Stop_Listen:
			case LVPOCAUDIO_Type_Tone_Stop_Speak:
				voice_formate = AUSTREAM_FORMAT_WAVPCM;
				break;

			default:
				voice_formate = AUSTREAM_FORMAT_WAVPCM;
				break;
		}

		if(prv_lv_poc_audio_array[voice_type] != NULL)
		{
			auPlayerStartMem(prv_play_voice_one_time_player, voice_formate, params, prv_lv_poc_audio_array[voice_type]->data, prv_lv_poc_audio_array[voice_type]->data_size);
			isPlayVoice = true;
		}
	}

	osiThreadExit();
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
	if(!quiet && (!lvPocGuiIdtCom_listen_status()))
	{
		if(prv_play_btn_voice_one_time_thread != NULL)
		{
			return;
		}prv_play_btn_voice_one_time_thread = osiThreadCreate("play_btn_voice", prv_play_btn_voice_one_time_thread_callback, NULL, OSI_PRIORITY_LOW, 1024*3, 64);
	}
}

/*
      name : poc_play_voice_one_time
     param :
     			[voice_type] type
      date : 2020-06-11
*/
void
poc_play_voice_one_time(IN       LVPOCAUDIO_Type_e voice_type, IN bool isBreak)
{
	if(NULL != prv_play_btn_voice_one_time_player)
	{
		auPlayerStop(prv_play_btn_voice_one_time_player);
	}

	if(NULL == prv_play_voice_one_time_player)
	{
		prv_play_voice_one_time_player = auPlayerCreate();
		if(NULL == prv_play_voice_one_time_player)
		{
			return;
	}
	}

	if(prv_play_voice_one_time_thread == NULL)
	{
		prv_play_voice_one_time_thread = osiThreadCreate("play_voice", prv_play_voice_one_time_thread_callback, NULL, OSI_PRIORITY_LOW, 1024*3, 64);
		if(prv_play_voice_one_time_thread == NULL)
	{
			return;
		}
	}

	osiEvent_t event = {0};
	event.id = 101;
	event.param1 = voice_type;
	event.param2 = (int)isBreak;
	osiEventSend(prv_play_voice_one_time_thread, &event);
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

	if(!poc_check_sim_prsent(POC_SIM_1))
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_Insert_SIM_Card, false);
		return false;
	}

	if (CFW_CfgGetNwStatus(&status, nSim) != 0 ||
		CFW_NwGetStatus(&nStatusInfo, nSim) != 0)
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_No_Connected, false);
		return false;
	}

	if(nStatusInfo.nStatus == 0 || nStatusInfo.nStatus == 2 || nStatusInfo.nStatus == 4)
	{
		poc_play_voice_one_time(LVPOCAUDIO_Type_No_Connected, false);
		return false;
	}
	return true;
}

/*
      name : poc_net_work_config
      param : config network to log idt server
      date : 2020-05-11
*/
void
poc_net_work_config(IN POC_SIM_ID sim)
{
	POC_SIM_ID nsim = POC_SIM_1;
	CFW_NwSetFTA(0, nsim);
	CFW_NwSetAutoAttachFlag(1, nsim);
}



static void
prv_poc_mmi_poc_setting_config_const(OUT nv_poc_setting_msg_t * poc_setting)
{
	poc_setting->font.list_btn_big_font = (uint32_t)LV_POC_FONT_MSYH(6500, 18);
	poc_setting->font.list_btn_small_font = (uint32_t)LV_POC_FONT_MSYH(6500, 15);
	poc_setting->font.about_label_big_font = (uint32_t)LV_POC_FONT_MSYH(6500, 18);
	poc_setting->font.about_label_small_font = (uint32_t)LV_POC_FONT_MSYH(6500, 15);
	poc_setting->font.win_title_font = (uint32_t)LV_POC_FONT_MSYH(6500, 18);
	poc_setting->font.activity_control_font = (uint32_t)LV_POC_FONT_MSYH(6500, 15);
	poc_setting->font.status_bar_time_font = (uint32_t)LV_POC_FONT_MSYH(6500, 14);
	//poc_setting->font.status_bar_signal_type_font = (uint32_t)LV_POC_FONT_MSYH(3500, 14);
	poc_setting->font.idle_big_clock_font = (uint32_t)LV_POC_FONT_MSYH(2500, 45);//主界面时间time
	poc_setting->font.idle_date_label_font = (uint32_t)LV_POC_FONT_MSYH(2500, 18);//主界面日期label
	poc_setting->font.idle_page2_msg_font = (uint32_t)LV_POC_FONT_MSYH(6500, 15);
	poc_setting->font.idle_popwindows_msg_font = (uint32_t)LV_POC_FONT_MSYH(6500, 18);
	poc_setting->font.idle_lockgroupwindows_msg_font = (uint32_t)LV_POC_FONT_MSYH(6500, 14);
	poc_setting->font.idle_shutdownwindows_msg_font = (uint32_t)LV_POC_FONT_MSYH(6500, 16);

	poc_setting->theme.white = &theme_white;
	poc_setting->theme.white->style_base = (uint32_t)&theme_white_style_base;
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


#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
	poc_setting->theme.black = &theme_black;
	poc_setting->theme.black->style_base = (uint32_t)&theme_black_style_base;
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
#endif
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
#ifdef CONFIG_POC_TTS_SUPPORT
	poc_setting->voice_broadcast_switch = 0;
#endif
#ifdef CONFIG_POC_GUI_KEYPAD_LIGHT_SUPPORT
	poc_setting->keypad_led_switch = 0;
#endif
	poc_setting->GPS_switch = 0;
	poc_setting->electric_torch_switch = 0;
	poc_setting->screen_brightness = 4;
	poc_setting->screen_bright_time = 2;
	//poc_setting->current_theme = 0;
	poc_setting->main_SIM = 0;
#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
	poc_setting->net_type = 0;
#endif
	poc_setting->font.big_font_switch = 1;
	poc_setting->font.list_page_colum_count = 3;
	poc_setting->font.list_btn_current_font = poc_setting->font.list_btn_big_font;
	poc_setting->font.about_label_current_font = poc_setting->font.about_label_big_font;
	poc_setting->volume = 5;
	poc_setting->language = 0;
#ifdef CONFIG_AT_MY_ACCOUNT_SUPPORT
	strcpy(poc_setting->account_name, "00000");
	strcpy(poc_setting->account_passwd, "00000");
	strcpy(poc_setting->ip_address, "124.160.11.21");
	poc_setting->ip_port = 10000;
#endif
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
#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
	if(poc_setting->theme.type == 0)
	{
		poc_setting->theme.current_theme = poc_setting->theme.white;
	}
	else if(poc_setting->theme.type == 1)
	{
		poc_setting->theme.current_theme = poc_setting->theme.black;
	}
#else
	poc_setting->theme.type = 0;
	poc_setting->theme.current_theme = poc_setting->theme.white;
#endif

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
    else
    {
	    memcpy(imei, (const void *)nImei, nImeiLen);
	}
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
	char * name = NULL;
	name = lv_poc_get_member_name(lv_poc_get_self_info());
	if(name == NULL)
	{
		return "";
	}
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
#ifdef CONFIG_POC_TTS_SUPPORT
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
#else
	return true;
#endif
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
	drvGpioConfig_t * config = NULL;
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
	free(config);
}

#ifdef CONFIG_POC_GUI_KEYPAD_LIGHT_SUPPORT
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
	drvGpioConfig_t * config = NULL;
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
	free(config);
}
#endif

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
	drvGpioConfig_t * config = NULL;
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
	free(config);
}

/*
      name : poc_port_init
      param : port is the IO that needs to be set
      date : 2020-05-08
*/
drvGpioConfig_t *
poc_port_init(void)
{
	if(poc_port_Gpio != NULL) return false;
	static drvGpioConfig_t * config = NULL;
	if(config == NULL)
	{
		config = (drvGpioConfig_t *)calloc(1, sizeof(drvGpioConfig_t));
		if(config == NULL)
		{
			return false;
		}
		memset(config, 0, sizeof(drvGpioConfig_t));
		config->mode = DRV_GPIO_OUTPUT;
		config->debounce = true;
		config->out_level = false;
	}

	return config;
}

/*
      name : poc_set_port_status
      param : open true is open port
      date : 2020-05-08
*/
bool
poc_set_port_status(uint32_t port, drvGpioConfig_t *config,bool open)
{
	poc_port_Gpio = drvGpioOpen(port, config, NULL, NULL);
	drvGpioWrite(poc_port_Gpio , open);
	return open;
}

/*
      name : poc_set_red_status
      param : if the status is true,open green led,but...
      date : 2020-05-09
*/
bool
poc_set_red_status(bool ledstatus)
{
#if 1
    if(configport==NULL)
    {
		configport=poc_port_init();
    }
	poc_set_port_status(poc_red_led,configport,ledstatus);
#endif
	return ledstatus;
}

/*
      name : poc_set_green_status
      param : if the status is true,open green led,but...
      date : 2020-05-09
*/
bool
poc_set_green_status(bool ledstatus)
{
#if 1
	//reg
	hwp_gpio1->gpio_oen_val&=~(1<<poc_green_led);//set gpio direction
	hwp_gpio1->gpio_oen_set_out|=(1<<poc_green_led);//set gpio output
    if(ledstatus)
	hwp_gpio1->gpio_set_reg|=(1<<poc_green_led);//open status
	else
	hwp_gpio1->gpio_clr_reg|=(1<<poc_green_led);//close status
#endif

#if 0
	REG_IOMUX_PAD_GPIO_13_CFG_REG_T  *gpio_13_t = NULL;
	//config
	gpio_13_t->b.pad_gpio_13_sel=0;//gpio13
	gpio_13_t->b.pad_gpio_13_pull_dn=1;//pull down
	gpio_13_t->b.pad_gpio_13_oen_reg=0;//set output mode
	gpio_13_t->b.pad_gpio_13_out_reg=ledstatus;//open or close

#endif

	return ledstatus;
}

static lv_poc_group_list_t * prv_group_list = NULL;
static get_group_list_cb   prv_group_list_cb = NULL;

static void
prv_lv_poc_get_group_list_cb(int msg_type, uint32_t num, CGroup *group)
{
	lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND, NULL);
	if(msg_type == 0
		|| num == 0
		|| group == NULL
		|| prv_group_list == NULL
		|| prv_group_list_cb == NULL)
	{
		if(prv_group_list_cb != NULL)
		{
			prv_group_list_cb(0);
			prv_group_list_cb = NULL;
		}
		prv_group_list = NULL;
		return;
	}

	prv_group_list->group_number = num;
	list_element_t * p_element = NULL;
	list_element_t * p_cur = NULL;
	for(int i = 0; i < num; i++)
	{
		p_element = (list_element_t *)lv_mem_alloc(sizeof(list_element_t));
		if(p_element == NULL)
		{
			p_element = prv_group_list->group_list;
			while(p_element)
			{
				p_cur = p_element;
				p_element = p_element->next;
				lv_mem_free(p_cur);
			}
			prv_group_list_cb(0);
			prv_group_list_cb = NULL;
			return;
		}
		p_element->next = NULL;
		p_element->list_item = NULL;
		p_element->information = (void *)(&group[i]);
		strcpy(p_element->name, (const char *)group[i].m_ucGName);
		if(prv_group_list->group_list != NULL)
		{
			p_cur->next = p_element;
			p_cur = p_cur->next;
		}
		else
		{
			prv_group_list->group_list = p_element;
			p_cur = p_element;
		}
		p_element = NULL;
	}
	prv_group_list_cb(1);
	prv_group_list_cb = NULL;
	prv_group_list = NULL;
}


/*
	  name : lv_poc_get_group_list
	  param :member_list{@group information} func{@callback GUI}
	  date : 2020-05-14
*/
bool
lv_poc_get_group_list(lv_poc_group_list_t * group_list, get_group_list_cb func)
{
	if(group_list == NULL || func == NULL)
	{
		return false;
	}
	prv_group_list = group_list;
	prv_group_list_cb = func;

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_GROUP_LIST_CB_IND, prv_lv_poc_get_group_list_cb))
	{
		prv_group_list_cb = NULL;
		prv_group_list = NULL;
		return false;
	}

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GROUP_LIST_QUERY_IND, NULL))
	{
		if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_GROUP_LIST_CB_IND, NULL))
		{
			OSI_LOGE(0, "cancel register callback[get group list cb] failed!");
		}
		prv_group_list_cb = NULL;
		prv_group_list = NULL;
		return false;
	}

	return true;
}

/*
	  name : lv_poc_check_group_equation
	  param :
	  date : 2020-05-19
*/
bool
lv_poc_check_group_equation(void * A, void *B, void *C, void *D, void *E)
{
	CGroup *info1 = (CGroup *)C;
	CGroup *info2 = (CGroup *)D;
	bool ret1 = false, ret2 = false;

	if(info1 != NULL && info2 != NULL)
	{
		if(info1 == info2)
		{
			return true;
		}
		ret1 = (0 == strcmp((const char *)info1->m_ucGName, (const char *)info2->m_ucGName));
		ret2 = (0 == strcmp((const char *)info1->m_ucGNum, (const char *)info2->m_ucGNum));
	}

	return (ret1 && ret2);
}

static lv_poc_member_list_t * prv_member_list = NULL;
static get_member_list_cb prv_member_list_cb = NULL;
static int prv_member_list_type  = 0;

static void
prv_lv_poc_get_member_list_cb(int msg_type, unsigned long num, Msg_GData_s *pGroup)
{
	bool is_self_wrote = false;
	lv_poc_member_info_t self_info = lv_poc_get_self_info();
	char *self_name = lv_poc_get_member_name(self_info);

	if(prv_member_list == NULL
		|| prv_member_list_cb == NULL
		|| (prv_member_list_type < 1 || prv_member_list_type > 3))
	{
		if(prv_member_list_cb != NULL)
		{
			prv_member_list_cb(0);
			prv_member_list_cb = NULL;
		}
		prv_member_list = NULL;
		prv_member_list_cb = NULL;
		return;
	}

	if(msg_type == 0 || num < 1)
	{
		prv_member_list_cb(0);
		prv_member_list_cb = NULL;
		prv_member_list = NULL;
		prv_member_list_cb = NULL;
		return;
	}

	list_element_t * p_element = NULL;
	list_element_t * p_cur = NULL;

	for(int i = 0; i < num; i++)
	{
		p_element = (list_element_t *)lv_mem_alloc(sizeof(list_element_t));

		if(p_element == NULL)
		{
			p_element = prv_member_list->online_list;
			while(p_element)
			{
				p_cur = p_element;
				p_element = p_element->next;
				lv_mem_free(p_cur);
			}

			p_element = prv_member_list->offline_list;
			while(p_element)
			{
				p_cur = p_element;
				p_element = p_element->next;
				lv_mem_free(p_cur);
			}
			prv_member_list_cb(0);
			return;
		}
		p_element->next = NULL;
		p_element->list_item = NULL;
		p_element->information = (void *)(&pGroup->member[i]);
		strcpy(p_element->name, (const char *)(pGroup->member[i].ucName));
		if(!is_self_wrote && MEMBER_EQUATION(self_name, pGroup->member[i].ucName, self_info, &pGroup->member[i], NULL))
		{
			is_self_wrote = true;
			strcat(p_element->name, (const char *)"[我]");
		}

		if(pGroup->member[i].ucStatus == 1
			&& (prv_member_list_type == 2
			|| prv_member_list_type == 1))//在线
		{
			prv_member_list->online_number++;//计算在线人数
			if(prv_member_list->online_list != NULL)
			{
				p_cur->next = p_element;
				p_cur = p_cur->next;
			}
			else
			{
				prv_member_list->online_list = p_element;
				p_cur = p_element;
			}
		}

		if(pGroup->member[i].ucStatus == 0
			&& (prv_member_list_type == 3
			|| prv_member_list_type == 1))//离线
		{
			prv_member_list->offline_number++;//计算离线人数
			if(prv_member_list->offline_list != NULL)
			{
				p_cur->next = p_element;
				p_cur = p_cur->next;
			}
			else
			{
				prv_member_list->offline_list = p_element;
				p_cur = p_element;
			}
		}
		p_element = NULL;
	}
	prv_member_list_cb(1);
	prv_member_list_cb = NULL;
	prv_member_list = NULL;
	prv_member_list_type  = 0;
}


/*
	  name : lv_poc_get_member_list
	  param :member_list{@member information} type{@status } func{@callback GUI}
	  date : 2020-05-12
*/
bool
lv_poc_get_member_list(lv_poc_group_info_t group_info, lv_poc_member_list_t * member_list, int type, get_member_list_cb func)
{
	if(member_list == NULL
		|| (type < 1 || type > 3)
		|| func == NULL)
	{
		return false;
	}

	prv_member_list = member_list;
	prv_member_list_type = type;
	prv_member_list_cb = func;

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_REGISTER_GET_MEMBER_LIST_CB_IND, prv_lv_poc_get_member_list_cb))
	{
		prv_member_list = NULL;
		prv_member_list_type = 0;
		prv_member_list_cb = NULL;

		return false;
	}

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_LIST_QUERY_IND, group_info))
	{
		if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_GET_MEMBER_LIST_CB_IND, NULL))
		{
			OSI_LOGE(0, "cancel register callback[get member list cb] failed!");
		}
		prv_member_list = NULL;
		prv_member_list_type = 0;
		prv_member_list_cb = NULL;
		return false;
	}

	return true;
}

/*
	  name : lv_poc_check_member_equation
	  param :
	  date : 2020-05-19
*/
bool
lv_poc_check_member_equation(void * A, void *B, void *C, void *D, void *E)
{
	Msg_GROUP_MEMBER_s *info1 = (Msg_GROUP_MEMBER_s *)C;
	Msg_GROUP_MEMBER_s *info2 = (Msg_GROUP_MEMBER_s *)D;
	bool ret1 = false, ret2 = false;

	if(info1 != NULL && info2 != NULL)
	{
		if(info1 == info2)
		{
			return true;
		}
		ret1 = (0 == strcmp((const char *)info1->ucName, (const char *)info2->ucName));
		ret2 = (0 == strcmp((const char *)info1->ucNum, (const char *)info2->ucNum));
	}

	return (ret1 && ret2);
}

/*
	  name : lv_poc_build_new_group
	  param :
	  date : 2020-05-19
*/
bool
lv_poc_build_new_group(lv_poc_member_info_t *members, int32_t num, poc_build_group_cb func)
{
	if(members == NULL || num < 1 || func == NULL)
	{
		return false;
	}

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_REGISTER_BIUILD_GROUP_CB_IND, func))
	{
		return false;
	}
	static lv_poc_build_new_group_t group_member = {0};
	memset(&group_member, 0, sizeof(lv_poc_build_new_group_t));
	group_member.members = members;
	group_member.num = num;

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_BIUILD_GROUP_IND, &group_member))
	{
		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_CANCEL_REGISTER_BIUILD_GROUP_CB_IND, func);
		return false;
	}
	return true;
}

/*
	  name : lv_poc_get_self_info
	  param :
	  date : 2020-05-20
*/
lv_poc_member_info_t
lv_poc_get_self_info(void)
{
	return (lv_poc_member_info_t)lvPocGuiIdtCom_get_self_info();
}

/*
	  name : lv_poc_get_current_group
	  param :
	  date : 2020-05-22
*/
lv_poc_group_info_t
lv_poc_get_current_group(void)
{
	return (lv_poc_group_info_t)lvPocGuiIdtCom_get_current_group_info();
}

/*
	  name : lv_poc_set_current_group
	  param :
	  date : 2020-05-22
*/
bool
lv_poc_set_current_group(lv_poc_group_info_t group, poc_set_current_group_cb func)
{
	if(group == NULL || func == NULL)
	{
		return false;
	}

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_REGISTER_SET_CURRENT_GROUP_CB_IND, func))
	{
		return false;
	}

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SET_CURRENT_GROUP_IND, group))
	{
		return false;
	}
	return true;
}

/*
	  name : lv_poc_get_group_name
	  param :
	  date : 2020-05-22
*/
char *
lv_poc_get_group_name(lv_poc_group_info_t group)
{
	if(group == NULL)
	{
		return NULL;
	}

	CGroup * group_info = (CGroup *)group;
	return (char *)group_info->m_ucGName;
}

/*
	  name : lv_poc_get_member_name
	  param :
	  date : 2020-05-20
*/
char *
lv_poc_get_member_name(lv_poc_member_info_t members)
{
	if(members == NULL)
	{
		return NULL;
	}

	Msg_GROUP_MEMBER_s * member_info = (Msg_GROUP_MEMBER_s *)members;
	return (char *)member_info->ucName;
}

/*
	  name : lv_poc_get_member_status
	  param :
	  date : 2020-05-21
*/
bool
lv_poc_get_member_status(lv_poc_member_info_t members, poc_get_member_status_cb func)
{
	if(members == NULL || func == NULL)
	{
		return false;
	}

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_REGISTER_MEMBER_STATUS_CB_REP, func))
	{
		return false;
	}

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_MEMBER_INFO_IND, members))
	{
		return false;
	}
	return true;;
}

/*
	  name : lv_poc_get_member_status
	  param :
	  date : 2020-05-21
*/
bool
lv_poc_set_member_call_status(lv_poc_member_info_t member, bool enable, poc_set_member_call_status_cb func)
{
	static lv_poc_member_call_config_t member_call_config = {0};

	if(NULL == func)
	{
		return false;
	}

	memset(&member_call_config, 0, sizeof(lv_poc_member_call_config_t));
	member_call_config.members = member,
	member_call_config.enable = enable;
	member_call_config.func = func;

	if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SINGLE_CALL_STATUS_IND, &member_call_config))
	{
		return false;
	}
	return true;;
}


