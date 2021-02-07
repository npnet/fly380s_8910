/***********************************************************************************/
#include "stdlib.h"
#include "string.h"
#include "poc_config.h"
#include <stdio.h>
#include "osi_log.h"
#include "osi_api.h"
#include "hal_iomux.h"
#include "gps_nmea.h"
#include "hal_chip.h"
#include "drv_uart.h"
#include "drv_i2c.h"
#include "lv_include/lv_poc_lib.h"
#include "guiIdtCom_api.h"
#include "uart3_gps.h"
#include "lv_apps/lv_poc_led/lv_poc_led.h"
#include "lv_apps/lv_poc_refr/lv_poc_refr.h"

#ifdef CONFIG_POC_GUI_GPS_SUPPORT

#define GPS_RX_BUF_SIZE (1024)
#define GPS_TX_BUF_SIZE (1024)

#define GPS_GET_INFO_TASK_CB_FREQ         (1*1000/10)                                 //0.1s----获取刷新GPS信息
#define GPS_NO_LOCATION_CHECK_SCAN_FREQ   (5*1000)                                    //5s----未定位到时连续获取GPS周期
#define GPS_NO_LOCATION_OUTAGE_FREQ       (24)                                        //120 = 5*24s--未定位到时连续去定位超时时长
#define GPS_NO_LOCATION_STOP_WORK_FREQ    (2*60*1000)                                 //2min--未定位到超时后关闭定位时长
#define GPS_LOCATION_CHECK_REPORT_FREQ    (5*60*1000 - GPS_DEEP_SLEEP_RESPONSE_TIMR)  //5min--定位后循环上传服务器周期
#define GPS_DEEP_SLEEP_RESPONSE_TIMR      (5*1000)                                    //5s----唤醒系统缓冲时间
#define GPS_RUN_TIMER_CB_FREQ             (1*1000)                                    //1s----闲步时长
#define GPS_COUNTDOWN_LOCATION_CB_FREQ    (1*1000)                                    //1s----倒计时定位周期

#define GPS_RUN_GET_DATA_NUMBER_FREQ      10                                          //至少读1s--最少有1次数据更新
#define GPS_RUN_WRITE_DATA_WAIT_PERIOD    100000                                      //100ms------回调后循环获取并解析GPS信息

//#define SUPPORT_GPS_LOG
/***********************************************************************************/
//GPS基本数据
typedef struct _IDT_GPS_DATA_s
{
    float           longitude;          //经度
    float           latitude;           //纬度
    float           speed;              //速度
    float           direction;          //方向
    //时间
    unsigned short  year;               //年
    unsigned char   month;              //月
    unsigned char   day;                //日
    unsigned char   hour;               //时
    unsigned char   minute;             //分
    unsigned char   second;             //秒
}IDT_GPS_DATA_s;
static IDT_GPS_DATA_s gps_t;

typedef struct _PocGpsIdtComAttr_t
{
	osiThread_t *thread;
	osiTimer_t * GpsInfoTimer;
	osiTimer_t * GpsSuspendTimer;
    osiTimer_t * RestartGpsTimer;
    osiTimer_t * SleepGpsTimer;
	lv_task_t * countdownlocationtask;
	lv_task_t * GpsGetInfo;
	bool        isReady;
	bool        ishavegps;
	bool        timeout_nolocation_status;
	bool        gps_location_status;
	bool        gps_error_data_status;
	bool        task_status;
	int         scannumber;
	int         getgpsdatanumber;
	int         countdownlocation;
	int         thislocationtime;
	int         errornumber;
	int 		poweronlocationnumber;
} PocGpsIdtComAttr_t;

static PocGpsIdtComAttr_t pocGpsIdtAttr = {0};

//I2C
static nmea_msg poc_idt_gps;
static nmea_msg poc_gps_monitor;
static uint8_t gps_data[GPS_RX_BUF_SIZE]={0};
static drvI2cMaster_t *i2c_p = NULL;
static uint8_t salve_i2c_addr_w = 0x60 >> 1;
uint8_t salve_i2c_addr_r = 0x61 >> 1;

//开启RMC
static uint8_t grmc[12] = {0x23,0x3E,0x03,0x51,0x04,0x00,0x03,0x04,0x01,0x01,0x61,0x82};
static uint8_t ggga[12] = {0x23,0x3E,0x03,0x51,0x04,0x00,0x04,0x04,0x01,0x01,0x62,0x86};
static uint8_t ggsa[12] = {0x23,0x3E,0x03,0x51,0x04,0x00,0x05,0x04,0x01,0x01,0x63,0x8A};
static uint8_t ggsv[12] = {0x23,0x3E,0x03,0x51,0x04,0x00,0x06,0x04,0x01,0x01,0x64,0x8E};

extern bool lv_poc_show_gps_location_status_img(bool status);
static void prvlvPocGpsIdtComHandleGpsTimercb(void *ctx);
static void prvlvPocGpsIdtComHandleSuspendTimercb(void *ctx);
static void prvlvPocGpsIdtComHandleRestartTimercb(void *ctx);
static void prvlvPocGpsIdtComHandleSleepTimercb(void *ctx);
static void prvlvPocGpsIdtComOpenGpsPm(void);
static void prvlvPocGpsIdtComCloseGpsPm(void);
static void prvlvPocGpsIdtComReconfig(void);
static void prvlvPocGpsIdtComCreateCountDownTask(void);
static void prvlvPocGpsIdtComDelCountDownTask(void);
static void prvlvPocGpsIdtComHandleLocationCountDowncb(lv_task_t * task);
static void prvlvPocGpsIdtComAnalyzeData(lv_task_t *task);
static void prvlvPocGpsIdtComErrorData(void);

#ifdef SUPPORT_GPS_REG
static
void prvGPSWriteOneReg(uint8_t addr, uint8_t data)
{
    drvI2cSlave_t idAddress = {salve_i2c_addr_w, addr, 0, false};
    if (i2c_p != NULL)
    {
        drvI2cWrite(i2c_p, &idAddress, &data, 1);
    }
    else
    {
        OSI_LOGE(0, "i2c is not open");
    }
}
#endif

static
bool prvGPSReadReg(uint8_t addr, uint8_t *data, uint32_t len)
{
    drvI2cSlave_t idAddress = {salve_i2c_addr_w, addr, 0, false};
	if (i2c_p != NULL)
    {
        return drvI2cRead(i2c_p, &idAddress, data, len);
    }
    OSI_LOGE(0, "i2c is not open");
	return false;
}


static
bool prvGPSWriteRegList(uint8_t *data, uint16_t len)
{
    drvI2cSlave_t wirte_data = {salve_i2c_addr_w, 0, 0, false};
    if (drvI2cWrite(i2c_p, &wirte_data, data, len))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static
bool prvlvPocGpsI2cOpen(uint32_t name, drvI2cBps_t bps)
{
    if (name == 0 || i2c_p != NULL)
    {
        return false;
    }
    i2c_p = drvI2cMasterAcquire(name, bps);
    if (i2c_p == NULL)
    {
        OSI_LOGE(0, "[GPS]i2c open fail");
        return false;
    }

    hwp_iomux->pad_gpio_14_cfg_reg |= IOMUX_PAD_GPIO_14_SEL_V_FUN_I2C_M2_SCL_SEL;
    hwp_iomux->pad_gpio_15_cfg_reg |= IOMUX_PAD_GPIO_15_SEL_V_FUN_I2C_M2_SDA_SEL;

    return true;
}

static
bool prvlvPocGpsI2cClose(uint32_t name, drvI2cBps_t bps)
{
    if (i2c_p != NULL)
        drvI2cMasterRelease(i2c_p);
    i2c_p = NULL;
    hwp_iomux->pad_gpio_14_cfg_reg = IOMUX_PAD_GPIO_14_SEL_V_FUN_GPIO_14_SEL;
    hwp_iomux->pad_gpio_15_cfg_reg = IOMUX_PAD_GPIO_15_SEL_V_FUN_GPIO_15_SEL;

	return true;
}

static
void prvlvPocGpsI2cLowOut(void)
{
	hwp_iomux->pad_gpio_14_cfg_reg = IOMUX_PAD_GPIO_14_SEL_V_FUN_GPIO_14_SEL;
	hwp_iomux->pad_gpio_15_cfg_reg = IOMUX_PAD_GPIO_15_SEL_V_FUN_GPIO_15_SEL;

	poc_set_iic_status(false);
}

static
void prvlvPocGpsIdtComGetInfo(lv_task_t *task)
{
	pocGpsIdtAttr.getgpsdatanumber++;
	memset(&gps_data, 0, sizeof(GPS_RX_BUF_SIZE));
	if(pubPocIdtGpsLocationStatus())
	{
		memset(&poc_idt_gps, 0, sizeof(nmea_msg));
		memset(&poc_gps_monitor, 0, sizeof(nmea_msg));
	}

	if(!prvGPSReadReg(0x82, gps_data, GPS_RX_BUF_SIZE))
	{
		OSI_LOGE(0, "[GPS]0x82 read fail, may gps no power");
		goto gpsinfoerror;
	}
	else
	{
#ifdef SUPPORT_GPS_LOG
		OSI_PRINTFI("[GPS][EVENT](%s)", gps_data);
#endif
		lv_task_set_period(pocGpsIdtAttr.GpsGetInfo, 2000);
		lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_CHECK_DATA_IND, NULL);
	}

	return;

gpsinfoerror:
	OSI_LOGI(0,"[GPS][EVENT][ERROR]:(%d)get info error", __LINE__);
	prvlvPocGpsIdtComErrorData();
}

static
void prvlvPocGpsIdtComCheckData(void)
{
	int gpsvalidnumber = 0;

	for(int j = 0; j < GPS_RX_BUF_SIZE; j++)//数据有效性检查
	{
		(gps_data[j] != 0xFF) ? (gpsvalidnumber++) : 0;
	}

	if(gpsvalidnumber > 80)//看情况修改---有效数据个数(正常定位到360字节左右)
	{
#ifdef SUPPORT_GPS_LOG
		OSI_LOGI(0, "[GPS]valid data number(%d)", gpsvalidnumber);
#endif
		goto gpscheckcorrect;
	}
	else
	{
#ifdef SUPPORT_GPS_LOG
		OSI_LOGE(0, "[GPS]invalid data(%d)", pocGpsIdtAttr.getgpsdatanumber);
#endif
		memset(&gps_data, 0, sizeof(GPS_RX_BUF_SIZE));
		gpsvalidnumber = 0;
	}

	if(pocGpsIdtAttr.getgpsdatanumber >= (GPS_RUN_GET_DATA_NUMBER_FREQ - 2))
	{
#ifdef SUPPORT_GPS_LOG
		OSI_LOGE(0, "[GPS]end invalid data");
#endif
		goto gpscheckerror;
	}
	lv_task_set_period(pocGpsIdtAttr.GpsGetInfo, 100);
	return;

gpscheckcorrect:
	pocGpsIdtAttr.getgpsdatanumber = 0;
	lv_poc_refr_task_once(prvlvPocGpsIdtComAnalyzeData, LVPOCLISTIDTCOM_LIST_PERIOD_50, LV_TASK_PRIO_HIGH);
	if(pocGpsIdtAttr.GpsGetInfo != NULL)
	{
		lv_task_del(pocGpsIdtAttr.GpsGetInfo);
		pocGpsIdtAttr.GpsGetInfo = NULL;
	}
	return;

gpscheckerror:
	OSI_LOGI(0,"[GPS][EVENT][ERROR]:(%d)check data error", __LINE__);
	prvlvPocGpsIdtComErrorData();
}

static
void prvlvPocGpsIdtComAnalyzeData(lv_task_t *task)
{
	float longitude = 0;
	float latitude = 0;

	//Analyze gps data
	GPS_Analysis(&poc_idt_gps,(uint8_t*)gps_data);
	memcpy((void *)&poc_gps_monitor, (void *)&poc_idt_gps, sizeof(nmea_msg));
	if(poc_idt_gps.longitude != 0
		&& poc_idt_gps.latitude != 0)
	{
		char longitudestr[32] = {0};
		char latitudestr[32] = {0};

		__itoa(poc_idt_gps.longitude, (char *)&longitudestr, 10);//10 --- 十进制
		longitude = atoff(longitudestr) / 100000.0;
		__itoa(poc_idt_gps.latitude, (char *)&latitudestr, 10);//10 --- 十进制
		latitude = atoff(latitudestr) / 100000.0;

		if('S' == poc_idt_gps.nshemi)
		{
			longitude = -longitude;
		}
		if('W' == poc_idt_gps.ewhemi)
		{
			latitude = -latitude;
		}
	}
#ifdef SUPPORT_GPS_LOG
	OSI_LOGI(0,"[GPS][EVENT]: gps.nshemi(%c)", poc_idt_gps.nshemi);//latitude
	OSI_LOGI(0,"[GPS][EVENT]: gps.ewhemi(%c)", poc_idt_gps.ewhemi);//longitude
	OSI_LOGI(0,"[GPS][EVENT]: gps.gpssta(%d)", poc_idt_gps.gpssta);//0--no locate
	OSI_LOGI(0,"[GPS][EVENT]: gps.fixmode(%d)", poc_idt_gps.fixmode);//0--no locate
	OSI_LOGI(0,"[GPS][EVENT]: UTC Date:%04d/%02d/%02d", poc_idt_gps.utc.year,poc_idt_gps.utc.month,poc_idt_gps.utc.date);
	OSI_LOGI(0,"[GPS][EVENT]: UTC Time:%02d:%02d:%02d", poc_idt_gps.utc.hour + 8,poc_idt_gps.utc.min,poc_idt_gps.utc.sec);
	OSI_LOGI(0,"[GPS][EVENT]: speed:%d\r\n",poc_idt_gps.speed);
	OSI_LOGXI(OSI_LOGPAR_IFF, 0,"[GPS][EVENT]:((up)%c%f(ori)%d)", poc_idt_gps.nshemi, longitude, poc_idt_gps.longitude);
	OSI_LOGXI(OSI_LOGPAR_IFF, 0,"[GPS][EVENT]:((up)%c%f(ori)%d)", poc_idt_gps.ewhemi, latitude, poc_idt_gps.latitude);
#endif
	if(poc_idt_gps.gpssta == 1
		|| poc_idt_gps.gpssta == 2
		|| poc_idt_gps.gpssta == 6)//locate
	{
		//copy data
		gps_t.longitude = longitude;
		gps_t.latitude = latitude;
		gps_t.speed = poc_idt_gps.speed;
		gps_t.direction = 0;
		gps_t.year = poc_idt_gps.utc.year;
		gps_t.month = poc_idt_gps.utc.month;
		gps_t.day = poc_idt_gps.utc.date;
		gps_t.hour = poc_idt_gps.utc.hour + 8;
		gps_t.minute = poc_idt_gps.utc.min;
		gps_t.second = poc_idt_gps.utc.sec;

		if(gps_t.speed == 0
			|| gps_t.longitude <= 0
		    || gps_t.longitude >= 180
		    || gps_t.latitude <= 0
		    || gps_t.latitude >= 90)
		{
			OSI_LOGI(0,"[GPS][EVENT]:gps data error");
			goto gpsdataerror;
		}

		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GPS_UPLOADING_IND, &gps_t);
		if(!pubPocIdtGpsLocationStatus()
			|| pocGpsIdtAttr.gps_error_data_status)
		{
			lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_GPS_LOCATION_REPORT_FREQ, NULL);
		}
		pocGpsIdtAttr.gps_location_status = true;
		pocGpsIdtAttr.timeout_nolocation_status = false;
		pocGpsIdtAttr.errornumber = 0;
		pocGpsIdtAttr.scannumber = 0;
		pocGpsIdtAttr.poweronlocationnumber++;
		prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
		prvlvPocGpsI2cLowOut();//low out
		lv_poc_set_auto_deepsleep(true);
		prvlvPocGpsIdtComCreateCountDownTask();
		OSI_LOGI(0,"[GPS][EVENT]:(%d):location, to send msg, allow enter deep sleep", __LINE__);
	}
	else
	{
		if(pubPocIdtGpsLocationStatus())
		{
			lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_GPS_NO_LOCATION_CHECK_FREQ, NULL);
		}
		pocGpsIdtAttr.gps_location_status = false;
		pocGpsIdtAttr.scannumber++;
		OSI_LOGI(0,"[GPS][EVENT]:no location");
	}

	if(pocGpsIdtAttr.gps_location_status)
	{
		lv_poc_show_gps_location_status_img(true);
	}

	if(!pubPocIdtGpsLocationStatus()
			&& (pocGpsIdtAttr.scannumber >= GPS_NO_LOCATION_OUTAGE_FREQ))//2min未定位上
	{
		//no location
		pocGpsIdtAttr.scannumber = 0;
		pocGpsIdtAttr.timeout_nolocation_status = true;
		lv_poc_show_gps_location_status_img(false);
		osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
		osiTimerStart(pocGpsIdtAttr.GpsSuspendTimer, GPS_RUN_TIMER_CB_FREQ);
		prvlvPocGpsIdtComCreateCountDownTask();
	}

	return;

gpsdataerror:
	OSI_LOGI(0,"[GPS][EVENT][ERROR]:(%d)analyze data error", __LINE__);
	pocGpsIdtAttr.scannumber++;
	if(pubPocIdtGpsLocationStatus())
	{
		lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_GPS_NO_LOCATION_CHECK_FREQ, NULL);
	}

	if(pocGpsIdtAttr.scannumber >= GPS_NO_LOCATION_OUTAGE_FREQ)
	{
		//no location
		pocGpsIdtAttr.gps_location_status = false;
		pocGpsIdtAttr.timeout_nolocation_status = true;
		lv_poc_show_gps_location_status_img(false);
		pocGpsIdtAttr.scannumber = 0;
		osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
		osiTimerStart(pocGpsIdtAttr.GpsSuspendTimer, GPS_RUN_TIMER_CB_FREQ);
		prvlvPocGpsIdtComCreateCountDownTask();
	}

	if(pocGpsIdtAttr.scannumber % 2 == 0)
	{
		prvlvPocGpsIdtComReconfig();
		OSI_LOGI(0, "[GPS][EVENT][ERROR](%d):reconfigure the GPS param, start to get gps info", __LINE__);
	}
}

static
void prvlvPocGpsIdtComErrorData(void)
{
	OSI_LOGI(0, "[GPS][EVENT][ERROR](%d):Error data", __LINE__);
	pocGpsIdtAttr.scannumber++;
	pocGpsIdtAttr.getgpsdatanumber = 0;
	if(pubPocIdtGpsLocationStatus())
	{
		lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_GPS_NO_LOCATION_CHECK_FREQ, NULL);
	}

	if(pocGpsIdtAttr.scannumber >= GPS_NO_LOCATION_OUTAGE_FREQ)
	{
		//no location
		pocGpsIdtAttr.gps_location_status = false;
		pocGpsIdtAttr.timeout_nolocation_status = true;
		lv_poc_show_gps_location_status_img(false);
		pocGpsIdtAttr.scannumber = 0;
		osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
		osiTimerStart(pocGpsIdtAttr.GpsSuspendTimer, GPS_RUN_TIMER_CB_FREQ);
		prvlvPocGpsIdtComCreateCountDownTask();
	}

	if(pocGpsIdtAttr.scannumber % 2 == 0)
	{
		prvlvPocGpsIdtComReconfig();
		OSI_LOGI(0, "[GPS][EVENT][ERROR](%d):reconfigure the GPS param, start to get gps info", __LINE__);
	}

    if(pocGpsIdtAttr.GpsGetInfo != NULL)
    {
		lv_task_del(pocGpsIdtAttr.GpsGetInfo);
		pocGpsIdtAttr.GpsGetInfo = NULL;
	}
}

static
void prvlvPocGpsIdtComSuspend(void)
{
	if(pocGpsIdtAttr.thread != NULL)
	{
		osiThreadSuspend(pocGpsIdtAttr.thread);
		OSI_LOGI(0, "[GPS][EVENT]GPS Thread Suspend!");
	}
}

static
void prvlvPocGpsIdtComResume(void)
{
	if(pocGpsIdtAttr.thread != NULL)
	{
		osiThreadResume(pocGpsIdtAttr.thread);
		OSI_LOGI(0, "[GPS][EVENT]GPS Thread Resume!");
	}
}

static
void prvlvPocGpsIdtComOpenGpsPm(void)
{
	poc_set_gps_ant_status(true);
    halPmuSwitchPower(HAL_POWER_VIBR, true, false);//最后一个true:休眠不断电
    halPmuSwitchPower(HAL_POWER_CAMA, true, false);
}

static
void prvlvPocGpsIdtComCloseGpsPm(void)
{
	poc_set_gps_ant_status(false);
    halPmuSwitchPower(HAL_POWER_VIBR, false, false);
    halPmuSwitchPower(HAL_POWER_CAMA, false, false);
}

static
void prvlvPocGpsIdtComReconfig(void)
{
	//need to config gps, due to gps power off
	if(!prvGPSWriteRegList(grmc, 12))
	{
		OSI_LOGE(0, "[GPS]grmc-->config fail");
	}

	if(!prvGPSWriteRegList(ggga, 12))
	{
		OSI_LOGE(0, "[GPS]ggga-->config fail");
	}
	prvGPSWriteRegList(ggsa, 12);
	prvGPSWriteRegList(ggsv, 12);
}

static
void prvlvPocGpsIdtComCreateCountDownTask(void)
{
	if(pocGpsIdtAttr.countdownlocationtask == NULL)
	{
		pocGpsIdtAttr.countdownlocation = 0;
		pocGpsIdtAttr.countdownlocationtask = lv_task_create(prvlvPocGpsIdtComHandleLocationCountDowncb, GPS_COUNTDOWN_LOCATION_CB_FREQ, LV_TASK_PRIO_HIGH, NULL);
	}
}

static
void prvlvPocGpsIdtComDelCountDownTask(void)
{
	if(pocGpsIdtAttr.countdownlocationtask != NULL)
	{
		lv_task_del(pocGpsIdtAttr.countdownlocationtask);
		pocGpsIdtAttr.countdownlocationtask = NULL;

		if(pubPocIdtGpsLocationStatus())
		{
			pocGpsIdtAttr.countdownlocation = GPS_LOCATION_CHECK_REPORT_FREQ / 1000;
		}
		else
		{
			pocGpsIdtAttr.countdownlocation = GPS_NO_LOCATION_STOP_WORK_FREQ / 1000;
		}
	}
}

static
void prvGpsThreadEntry(void *ctx)
{
	osiEvent_t event = {0};

	pocGpsIdtAttr.isReady = true;
	pocGpsIdtAttr.GpsInfoTimer = osiTimerCreate(NULL, prvlvPocGpsIdtComHandleGpsTimercb, NULL);
	pocGpsIdtAttr.GpsSuspendTimer = osiTimerCreate(NULL, prvlvPocGpsIdtComHandleSuspendTimercb, NULL);
    pocGpsIdtAttr.RestartGpsTimer = osiTimerCreate(NULL, prvlvPocGpsIdtComHandleRestartTimercb, NULL);
    pocGpsIdtAttr.SleepGpsTimer = osiTimerCreate(NULL, prvlvPocGpsIdtComHandleSleepTimercb, NULL);
	OSI_LOGI(0, "[GPS][EVENT]GPS start!");
    while(1)
    {
		if(!osiEventWait(pocGpsIdtAttr.thread , &event))
		{
			continue;
		}

		if(event.id != 105)
		{
			continue;
		}

		switch(event.param1)
		{
			case LVPOCGPSIDTCOM_SIGNAL_GET_DATA_IND:
			{
				OSI_LOGI(0, "[GPS][EVENT]rec refr data");
				if(pocGpsIdtAttr.GpsGetInfo == NULL)
				{
					pocGpsIdtAttr.GpsGetInfo = lv_task_create(prvlvPocGpsIdtComGetInfo, GPS_GET_INFO_TASK_CB_FREQ, LV_TASK_PRIO_HIGH, NULL);
				}
				break;
			}

			case LVPOCGPSIDTCOM_SIGNAL_CHECK_DATA_IND:
			{
				OSI_LOGI(0, "[GPS][EVENT]rec check data");
				prvlvPocGpsIdtComCheckData();
				break;
			}

			case LVPOCGPSIDTCOM_SIGNAL_OPEN_GPS_REPORT:
			{
				if(!pubPocIdtGpsLocationStatus())
				{
					osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ);
				}
				else
				{
					osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_LOCATION_CHECK_REPORT_FREQ);
				}
				break;
			}

			case LVPOCGPSIDTCOM_SIGNAL_CLOSE_GPS_REPORT:
			{
				osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
				break;
			}

			case LVPOCGPSIDTCOM_SIGNAL_GPS_NO_LOCATION_CHECK_FREQ:
			{
				OSI_LOGI(0, "[GPS][EVENT]modify check_scan period");
				pubPocIdtGpsLocationStatus() ? (pocGpsIdtAttr.gps_error_data_status = true) : 0;
				osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
				osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ);
				break;
			}

			case LVPOCGPSIDTCOM_SIGNAL_GPS_LOCATION_REPORT_FREQ:
			{
				OSI_LOGI(0, "[GPS][EVENT]modify report period");
				pubPocIdtGpsLocationStatus() ? (pocGpsIdtAttr.gps_error_data_status = false) : 0;
				osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
				osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_LOCATION_CHECK_REPORT_FREQ);

//				OSI_PRINTFI("[poc][gps](%s)[%d]remain(%lld), expir(%lld)", __func__, __LINE__, osiTimerRemaining(pocGpsIdtAttr.GpsInfoTimer), osiTimerExpiration(pocGpsIdtAttr.GpsInfoTimer));
				break;
			}

			case LVPOCGPSIDTCOM_SIGNAL_START_GPS_LOCATION:
			{
				if(!osiTimerIsRunning(pocGpsIdtAttr.GpsInfoTimer))
				{
					osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ);
					OSI_LOGI(0, "[GPS][EVENT]GPS start");
				}
				break;
			}

			case LVPOCGPSIDTCOM_SIGNAL_STOP_GPS_LOCATION:
			{
				if(lvPocGuiComCitStatus(LVPOCCIT_TYPE_READ_STATUS) == LVPOCCIT_TYPE_ENTER)
				{
					OSI_LOGI(0, "[GPS][EVENT]cit status, GPS not stop, just break");
					break;
				}

//				OSI_PRINTFI("[poc][gps](%s)[%d]remain(%lld), expir(%lld)", __func__, __LINE__, osiTimerRemaining(pocGpsIdtAttr.GpsInfoTimer), osiTimerExpiration(pocGpsIdtAttr.GpsInfoTimer));
				if(osiTimerIsRunning(pocGpsIdtAttr.GpsInfoTimer))
				{
					osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
					OSI_LOGI(0, "[GPS][EVENT]GPS stop");
				}
				break;
			}

			default:
			{
				break;
			}
		}
    }
}

static
void prvlvPocGpsIdtComHandleSuspendTimercb(void *ctx)
{
	if(!pubPocIdtGpsLocationStatus())
	{
	    osiTimerStart(pocGpsIdtAttr.SleepGpsTimer, GPS_NO_LOCATION_STOP_WORK_FREQ);
		prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
		prvlvPocGpsI2cLowOut();//low out
		lv_poc_set_auto_deepsleep(true);//allow enter deep sleep
		OSI_LOGI(0, "[GPS][EVENT](%d):stop GPS work, 2min, allow enter deep sleep", __LINE__);
	}
	else
	{
		//need to config gps, due to gps power off
		prvlvPocGpsI2cOpen(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
		prvlvPocGpsIdtComReconfig();
		lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_GET_DATA_IND, NULL);
		OSI_LOGI(0, "[GPS][EVENT][location](%d):reconfigure the GPS param, start to get gps info", __LINE__);
	}
}

static
void prvlvPocGpsIdtComHandleRestartTimercb(void *ctx)
{
	prvlvPocGpsI2cOpen(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	prvlvPocGpsIdtComReconfig();
	//config finish, start goto get info
	osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ);
	OSI_LOGI(0, "[GPS][EVENT](%d):2min time out, config finish, start goto to location", __LINE__);
}

static
void prvlvPocGpsIdtComHandleSleepTimercb(void *ctx)
{
	OSI_LOGI(0, "[GPS][EVENT][location](%d):wait wake system, not allow enter deep sleep", __LINE__);
	pocGpsIdtAttr.thislocationtime = 0;
	prvlvPocGpsI2cLowOut();//low out
	lv_poc_set_auto_deepsleep(false);//not allow enter deep sleep
	osiTimerStart(pocGpsIdtAttr.RestartGpsTimer, GPS_DEEP_SLEEP_RESPONSE_TIMR);
	prvlvPocGpsIdtComDelCountDownTask();
}

static
void prvlvPocGpsIdtComHandleGpsTimercb(void *ctx)
{
	if(!pubPocIdtGpsLocationStatus()
		|| pocGpsIdtAttr.gps_error_data_status)
	{
		OSI_LOGI(0, "[GPS][EVENT][no location](%d):start to get gps info", __LINE__);
		lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_GET_DATA_IND, NULL);
	}
	else
	{
		pocGpsIdtAttr.errornumber++;
		pocGpsIdtAttr.errornumber >= 2 ? (pocGpsIdtAttr.gps_error_data_status = true) : 0;
		pocGpsIdtAttr.errornumber >= 5 ? (pocGpsIdtAttr.gps_location_status = false) : 0;

		OSI_LOGI(0, "[GPS][EVENT][location](%d):wait wake system, not allow enter deep sleep", __LINE__);
		pocGpsIdtAttr.thislocationtime = 0;
		prvlvPocGpsI2cLowOut();//low out
		lv_poc_set_auto_deepsleep(false);
		osiTimerStart(pocGpsIdtAttr.GpsSuspendTimer, GPS_DEEP_SLEEP_RESPONSE_TIMR);
		prvlvPocGpsIdtComDelCountDownTask();
	}
	pocGpsIdtAttr.thislocationtime += GPS_NO_LOCATION_CHECK_SCAN_FREQ;
}

static
void prvlvPocGpsIdtComHandleLocationCountDowncb(lv_task_t * task)
{
	pocGpsIdtAttr.countdownlocation += GPS_COUNTDOWN_LOCATION_CB_FREQ;
}

void publvPocGpsIdtComInit(void)
{
	memset(&pocGpsIdtAttr, 0, sizeof(PocGpsIdtComAttr_t));
	pocGpsIdtAttr.timeout_nolocation_status = true;
	prvlvPocGpsI2cLowOut();//low out
	prvlvPocGpsIdtComOpenGpsPm();

    if (!prvlvPocGpsI2cOpen(DRV_NAME_I2C2, DRV_I2C_BPS_100K))
    {
        goto gps_fail;
    }
	else
	{
		for(int i = 0; i < 5; i++)
		{
			if(prvGPSWriteRegList(grmc, 12))
			{
				goto gps_success;
			}
			osiThreadSleepRelaxed(300, OSI_WAIT_FOREVER);//300ms
		}
		goto gps_fail;
	}

gps_success:
	pocGpsIdtAttr.ishavegps = true;
	if(!prvGPSReadReg(0x82, gps_data, GPS_RX_BUF_SIZE))
	{
		if(!prvGPSReadReg(0x82, gps_data, GPS_RX_BUF_SIZE))
		{
			goto gps_fail;
		}
	}
	prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	prvlvPocGpsI2cLowOut();//low out
	memset(&gps_data, 0, sizeof(GPS_RX_BUF_SIZE));
    pocGpsIdtAttr.thread = osiThreadCreate("GpsThread", prvGpsThreadEntry, NULL, OSI_PRIORITY_NORMAL, 2048, 64);
	OSI_LOGI(0, "[GPS][EVENT][POWERON]have GPS module");
	return;

gps_fail:
	prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	prvlvPocGpsI2cLowOut();//low out
	pocGpsIdtAttr.ishavegps = false;
	prvlvPocGpsIdtComCloseGpsPm();
	lv_poc_set_auto_deepsleep(true);
	OSI_LOGI(0, "[GPS][EVENT][POWERON]no GPS module");
	return;
}

bool pubPocIdtGpsLocationStatus(void)
{
	return pocGpsIdtAttr.gps_location_status;
}

bool pubPocIdtGpsTimeoutNoLocationStatus(void)
{
	return pocGpsIdtAttr.timeout_nolocation_status;
}

bool pubPocIdtIsHaveExistGps(void)
{
	return pocGpsIdtAttr.ishavegps;
}

bool pubPocIdtGpsTaskStatus(void)
{
	return pocGpsIdtAttr.task_status;
}

void publvPocGpsIdtComSleep(void)
{
	if(pocGpsIdtAttr.ishavegps == false)
	{
		OSI_LOGI(0, "[GPS][EVENT]sleep and no GPS module");
		return;
	}
	pocGpsIdtAttr.isReady = false;
	pocGpsIdtAttr.task_status = false;
	prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	prvlvPocGpsI2cLowOut();//low out
	prvlvPocGpsIdtComSuspend();
	osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
	osiTimerStop(pocGpsIdtAttr.GpsSuspendTimer);
	osiTimerStop(pocGpsIdtAttr.RestartGpsTimer);
	osiTimerStop(pocGpsIdtAttr.SleepGpsTimer);
	OSI_LOGI(0, "[GPS][EVENT]GPS close");
}

void publvPocGpsIdtComWake(void)
{
	if(pocGpsIdtAttr.ishavegps == false)
	{
		OSI_LOGI(0, "[GPS][EVENT]wake and no GPS module");
		return;
	}
	pocGpsIdtAttr.thislocationtime = 0;
	pocGpsIdtAttr.countdownlocation = 0;
	pocGpsIdtAttr.gps_location_status = false;
	pocGpsIdtAttr.timeout_nolocation_status = true;
	pocGpsIdtAttr.isReady = true;
	pocGpsIdtAttr.task_status = true;
	lv_poc_show_gps_location_status_img(false);
	memset(&poc_idt_gps, 0, sizeof(nmea_msg));
	memset(&poc_gps_monitor, 0, sizeof(nmea_msg));
	//config gps
	prvlvPocGpsI2cOpen(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	prvlvPocGpsIdtComReconfig();
	prvlvPocGpsIdtComResume();
	osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ);
	OSI_LOGI(0, "[GPS][EVENT]GPS open");
}

void *publvPocGpsIdtComDataInfo(void)
{
	return (void *)&poc_gps_monitor;
}

int publvPocGpsIdtComGetLocationCountDown(void)
{
	if(pubPocIdtGpsLocationStatus())
	{
		return ((GPS_LOCATION_CHECK_REPORT_FREQ - pocGpsIdtAttr.countdownlocation)/1000);
	}
	else
	{
		return ((GPS_NO_LOCATION_STOP_WORK_FREQ - pocGpsIdtAttr.countdownlocation)/1000);
	}
}

int publvPocGpsIdtComGetThisLocationTime(void)
{
	return (pocGpsIdtAttr.thislocationtime/1000);
}

int publvPocGpsIdtComGetPoweronLocationNumber(void)
{
	return pocGpsIdtAttr.poweronlocationnumber;
}

bool
lvPocGpsIdtCom_Msg(LVPOCIDTCOM_Gps_SignalType_t signal, void *ctx)
{
	if (pocGpsIdtAttr.thread == NULL || pocGpsIdtAttr.isReady == false)
	{
		OSI_LOGI(0, "[GPS][EVENT]no GPS module");
		return false;
	}

	osiEvent_t event = {0};
	memset(&event, 0, sizeof(osiEvent_t));
	event.id = 105;
	event.param1 = signal;
	event.param2 = (uint32_t)ctx;
	return osiEventSend(pocGpsIdtAttr.thread, &event);
}
#endif

