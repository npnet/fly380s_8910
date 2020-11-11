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
#include "guiOemCom_api.h"
#include "uart3_gps.h"
#include "lv_apps/lv_poc_led/lv_poc_led.h"

#ifdef CONFIG_POC_GUI_GPS_SUPPORT

#define GPS_RX_BUF_SIZE (1024)
#define GPS_TX_BUF_SIZE (1024)

#define GPS_NO_LOCATION_CHECK_SCAN_FREQ   5*1000 //5s
#define GPS_NO_LOCATION_POWER_OFF_FREQ    30*1000 //30s
#define GPS_LOCATION_CHECK_REPORT_FREQ    5*60*1000//5min
#define GPS_NO_LOCATION_OUTAGE_FREQ       60//断电频率(未定位上)--300s
#define GPS_RUN_TIMER_CB_FREQ             500 //500ms

#define GPS_RUN_GET_DATA_NUMBER_FREQ      20//至少读1s-->最少有1次数据更新
#define GPS_RUN_WRITE_DATA_WAIT_PERIOD    50000//50ms

#define SUPPORT_GPS_LOG
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
	bool        isReady;
	bool        ishavegps;
	bool        gps_location_status;
	bool        locate_status;
	int         scannumber;
} PocGpsIdtComAttr_t;

static PocGpsIdtComAttr_t pocGpsIdtAttr = {0};

//I2C
static nmea_msg poc_idt_gps;//GPS
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
static void prvlvPocGpsIdtComOpenGpsPm(void);
static void prvlvPocGpsIdtComCloseGpsPm(void);

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
void prvlvPocGpsIdtComGetInfo(void)
{
	float longitude = 0;
    float latitude = 0;
	int gpsvalidnumber = 0;

	prvlvPocGpsI2cOpen(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	memset(&gps_data, 0, sizeof(GPS_RX_BUF_SIZE));
	if(pubPocIdtGpsLocationStatus())
	{
		memset(&poc_idt_gps, 0, sizeof(nmea_msg));
	}

	for(int i = 0; i < GPS_RUN_GET_DATA_NUMBER_FREQ; i++)
	{
        if(!prvGPSReadReg(0x82, gps_data, GPS_RX_BUF_SIZE))
        {
			OSI_LOGE(0, "[GPS]0x82 read fail, may gps no power");
			goto gpserror;
		}
		else
		{
			OSI_PRINTFI("[GPS][EVENT](%s)", gps_data);
		}

		for(int j = 0; j < GPS_RX_BUF_SIZE; j++)//数据有效性检查
		{
			if(gps_data[j] != 0xFF)
			{
				gpsvalidnumber++;
			}
		}

		if(gpsvalidnumber > 80)//看情况修改---有效数据个数(正常定位到360字节左右)
		{
			OSI_LOGI(0, "[GPS]valid data number(%d)", gpsvalidnumber);
			break;
		}
		else
		{
			OSI_LOGE(0, "[GPS]invalid data(%d)", i + 1);
			memset(&gps_data, 0, sizeof(GPS_RX_BUF_SIZE));
			gpsvalidnumber = 0;
		}

		if(i >= (GPS_RUN_GET_DATA_NUMBER_FREQ - 2))
		{
			OSI_LOGE(0, "[GPS]end invalid data");
			goto gpserror;
		}
		osiDelayUS(GPS_RUN_WRITE_DATA_WAIT_PERIOD);
	}
	//Analyze gps data
	GPS_Analysis(&poc_idt_gps,(uint8_t*)gps_data);
	if(poc_idt_gps.longitude != 0
		&& poc_idt_gps.latitude != 0)
	{
		longitude = poc_idt_gps.longitude*1.0 / 100000 + 0.00005;
		latitude  = poc_idt_gps.latitude*1.0  / 100000 + 0.00005;
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
	OSI_LOGI(0,"[GPS][EVENT]: UTC Time:%02d:%02d:%02d", poc_idt_gps.utc.hour,poc_idt_gps.utc.min,poc_idt_gps.utc.sec);
	OSI_LOGI(0,"[GPS][EVENT]: speed:%d\r\n",poc_idt_gps.speed);
	OSI_LOGXI(OSI_LOGPAR_IFF, 0,"[GPS][EVENT]:((up)%c%f(ori)%d)", poc_idt_gps.nshemi, longitude, poc_idt_gps.longitude);
	OSI_LOGXI(OSI_LOGPAR_IFF, 0,"[GPS][EVENT]:((up)%c%f(ori)%d)", poc_idt_gps.ewhemi, latitude, poc_idt_gps.latitude);
#endif
	if(poc_idt_gps.gpssta)//locate
	{
		//copy data
		gps_t.longitude = longitude;
		gps_t.latitude = latitude;
		gps_t.speed = poc_idt_gps.speed;
		gps_t.direction = 0;
		gps_t.year = poc_idt_gps.utc.year;
		gps_t.month = poc_idt_gps.utc.month;
		gps_t.day = poc_idt_gps.utc.date;
		gps_t.hour = poc_idt_gps.utc.hour;
		gps_t.minute = poc_idt_gps.utc.min;
		gps_t.second = poc_idt_gps.utc.sec;

		if(gps_t.speed == 0
			|| gps_t.longitude <= 0
		    || gps_t.longitude >= 180
		    || gps_t.latitude <= 0
		    || gps_t.latitude >= 90)
		{
			OSI_LOGI(0,"[GPS][EVENT]:gps data error");
			goto gpserror;
		}

		lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_GPS_UPLOADING_IND, &gps_t);
		if(!pubPocIdtGpsLocationStatus())
		{
			lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_GPS_LOCATION_REPORT_FREQ, NULL);
		}
		pocGpsIdtAttr.gps_location_status = true;
		OSI_LOGI(0,"[GPS][EVENT]:location, to send msg");
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

	switch(pocGpsIdtAttr.gps_location_status)
	{
		case 0:
		{
			lv_poc_show_gps_location_status_img(false);
			break;
		}

		case 1:
		{
			lv_poc_show_gps_location_status_img(true);
			break;
		}
	}

	if(!pubPocIdtGpsLocationStatus()
			&& (pocGpsIdtAttr.scannumber >= GPS_NO_LOCATION_OUTAGE_FREQ))
	{
		osiTimerStartRelaxed(pocGpsIdtAttr.GpsSuspendTimer, GPS_RUN_TIMER_CB_FREQ, OSI_WAIT_FOREVER);
	}
	prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	return;

gpserror:
	pocGpsIdtAttr.gps_location_status = false;
	prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	lv_poc_show_gps_location_status_img(false);
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
void prvGpsThreadEntry(void *ctx)
{
	osiEvent_t event = {0};

	pocGpsIdtAttr.isReady = true;
	pocGpsIdtAttr.GpsInfoTimer = osiTimerCreate(NULL, prvlvPocGpsIdtComHandleGpsTimercb, NULL);
	pocGpsIdtAttr.GpsSuspendTimer = osiTimerCreate(NULL, prvlvPocGpsIdtComHandleSuspendTimercb, NULL);
    pocGpsIdtAttr.RestartGpsTimer = osiTimerCreate(NULL, prvlvPocGpsIdtComHandleRestartTimercb, NULL);
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
				prvlvPocGpsIdtComGetInfo();
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
				osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
				osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ);
				break;
			}

			case LVPOCGPSIDTCOM_SIGNAL_GPS_LOCATION_REPORT_FREQ:
			{
				OSI_LOGI(0, "[GPS][EVENT]modify report period");
				osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
				osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_LOCATION_CHECK_REPORT_FREQ);
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
void prvlvPocGpsIdtComHandleGpsTimercb(void *ctx)
{
	if(!pubPocIdtGpsLocationStatus()
        && (pocGpsIdtAttr.scannumber >= GPS_NO_LOCATION_OUTAGE_FREQ))
    {
        pocGpsIdtAttr.scannumber = 0;
        OSI_LOGI(0, "[GPS][EVENT]restart location");
        publvPocGpsIdtComWake();
        return;
    }
    else
    {
		OSI_LOGI(0, "[GPS][EVENT]start to get gps info");
		lvPocGpsIdtCom_Msg(LVPOCGPSIDTCOM_SIGNAL_GET_DATA_IND, NULL);
    }
}

static
void prvlvPocGpsIdtComHandleSuspendTimercb(void *ctx)
{
    publvPocGpsIdtComSleep();
    osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
    osiTimerStart(pocGpsIdtAttr.RestartGpsTimer, GPS_NO_LOCATION_POWER_OFF_FREQ);
    OSI_LOGI(0, "[GPS][EVENT]start GPS power off");
}

static
void prvlvPocGpsIdtComHandleRestartTimercb(void *ctx)
{
    osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_RUN_TIMER_CB_FREQ);
    OSI_LOGI(0, "[GPS][EVENT]GPS power on, goto to location");
}

void publvPocGpsIdtComInit(void)
{
	memset(&pocGpsIdtAttr, 0, sizeof(PocGpsIdtComAttr_t));
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
	memset(&gps_data, 0, sizeof(GPS_RX_BUF_SIZE));
    pocGpsIdtAttr.thread = osiThreadCreate("GpsThread", prvGpsThreadEntry, NULL, OSI_PRIORITY_NORMAL, 2048, 64);
	OSI_LOGI(0, "[GPS][EVENT][POWERON]have GPS module");
	return;

gps_fail:
	prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	pocGpsIdtAttr.ishavegps = false;
	prvlvPocGpsIdtComCloseGpsPm();
	OSI_LOGI(0, "[GPS][EVENT][POWERON]no GPS module");
	return;
}

bool pubPocIdtGpsLocationStatus(void)
{
	return pocGpsIdtAttr.gps_location_status;
}

bool pubPocIdtIsHaveExistGps(void)
{
	return pocGpsIdtAttr.ishavegps;
}

void publvPocGpsIdtComSleep(void)
{
	OSI_LOGI(0, "[GPS][EVENT]GPS close!");
	if(pocGpsIdtAttr.ishavegps == false)
	{
		OSI_LOGI(0, "[GPS][EVENT]sleep and no GPS module");
		return;
	}
	osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
	osiTimerStop(pocGpsIdtAttr.GpsSuspendTimer);
	osiTimerStop(pocGpsIdtAttr.RestartGpsTimer);
	pocGpsIdtAttr.isReady = false;
	prvlvPocGpsIdtComSuspend();
}

void publvPocGpsIdtComWake(void)
{
	if(pocGpsIdtAttr.ishavegps == false)
	{
		OSI_LOGI(0, "[GPS][EVENT]wake and no GPS module");
		return;
	}
	pocGpsIdtAttr.isReady = true;
	lv_poc_show_gps_location_status_img(false);
	//config gps
	prvlvPocGpsI2cOpen(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	if(!prvGPSWriteRegList(grmc, 12))
	{
		OSI_LOGE(0, "[GPS]i2c send fail");
	}
	prvGPSWriteRegList(ggga, 12);
	prvGPSWriteRegList(ggsa, 12);
    prvGPSWriteRegList(ggsv, 12);
	prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);

	prvlvPocGpsIdtComResume();
	osiTimerStartPeriodic(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ);
	OSI_LOGI(0, "[GPS][EVENT]GPS open");
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

