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

#ifdef CONFIG_POC_GUI_GPS_SUPPORT

#define GPS_RX_BUF_SIZE (1024)
#define GPS_TX_BUF_SIZE (1024)
#define GPS_NO_LOCATION_CHECK_SCAN_FREQ   10*1000 //10s
#define GPS_NO_LOCATION_POWER_OFF_FREQ    30*1000 //30s
#define GPS_LOCATION_CHECK_REPORT_FREQ    5*60*1000//5min
#define GPS_NO_LOCATION_OUTAGE_FREQ       12//断电频率(未定位上)--120s
#define GPS_RUN_TIMER_CB_FREQ             500 //500ms

#define GPS_RUN_GET_DATA_NUMBER_FREQ      10
#define GPS_RUN_GET_DATA_ERROR_UP_FREQ    (GPS_RUN_GET_DATA_NUMBER_FREQ -2)
#define GPS_RUN_WRITE_DATA_WAIT_PERIOD    50000//US
#define GPS_RUN_READ_DATA_WAIT_PERIOD     (GPS_RUN_WRITE_DATA_WAIT_PERIOD *2)

#define SUPPORT_GPS_LOG
//#define SUPPORT_ORI_GPS_LOG
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
	bool        gps_i2c_error_status;
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
void prvGPSReadReg(uint8_t addr, uint8_t *data, uint32_t len)
{
    drvI2cSlave_t idAddress = {salve_i2c_addr_w, addr, 0, false};
    if (i2c_p != NULL)
    {
        drvI2cRead(i2c_p, &idAddress, data, len);
    }
    else
    {
        OSI_LOGE(0, "i2c is not open");
    }
}


static
bool prvGPSWriteRegList(uint8_t *data, uint16_t len)
{
    drvI2cSlave_t wirte_data = {salve_i2c_addr_w, 0, 0, false};

    if (drvI2cWrite(i2c_p, &wirte_data, data, len))
        return true;
    else
        return false;

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
	static float longitude = 0;
    static float latitude = 0;
    static uint16_t plen = 0;
    static unsigned char buf[2] = {0xff, 0xff};
	//i2c open
	prvlvPocGpsI2cOpen(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	osiDelayUS(5000);//5ms
	prvGPSReadReg(0x82, gps_data, plen);
	memset(&gps_data, 0, sizeof(GPS_RX_BUF_SIZE));
	osiDelayUS(5000);//5ms

	for(int i = 0; i< GPS_RUN_GET_DATA_NUMBER_FREQ; i++)
	{
		if(!prvGPSWriteRegList(grmc, 12))
		{
			OSI_LOGE(0, "[GPS]grmc send fail");
			goto gpserror;
		}

		if(!prvGPSWriteRegList(ggga, 12))
		{
			OSI_LOGE(0, "[GPS]ggga send fail");
			goto gpserror;
		}

		if(!prvGPSWriteRegList(ggsa, 12))
		{
			OSI_LOGE(0, "[GPS]ggsa send fail");
			goto gpserror;
		}

		if(!prvGPSWriteRegList(ggsv, 12))
		{
			OSI_LOGE(0, "[GPS]ggsv send fail");
			goto gpserror;
		}
		osiDelayUS(GPS_RUN_WRITE_DATA_WAIT_PERIOD);
	    prvGPSReadReg(0x80, buf, 2);
	    plen = (buf[0]<<8) | (buf[1]);

	    if(plen != 0)
	    {
	        prvGPSReadReg(0x82, gps_data, plen);
	        GPS_Analysis(&poc_idt_gps,(uint8_t*)gps_data);

			#ifdef SUPPORT_ORI_GPS_LOG
	         OSI_LOGXI(OSI_LOGPAR_IS, 0,"[GPS][EVENT]len=(%d)(%s)", plen, gps_data);
	         OSI_LOGI(0,"[GPS][EVENT]: ori_longitude(%d)", poc_idt_gps.longitude);
	         OSI_LOGI(0,"[GPS][EVENT]: ori_latitude(%d)", poc_idt_gps.latitude);
			#endif
	    }

		if(poc_idt_gps.longitude != 0
			&& poc_idt_gps.latitude != 0)
		{
	    	longitude = poc_idt_gps.longitude / 100000.0;
	    	latitude  = poc_idt_gps.latitude  / 100000.0;
			if('S' == poc_idt_gps.nshemi)
		    {
		        longitude = -longitude;
		    }
		    if('W' == poc_idt_gps.ewhemi)
		    {
		        latitude = -latitude;
		    }
		}
		osiDelayUS(GPS_RUN_READ_DATA_WAIT_PERIOD);

		if((i >= GPS_RUN_GET_DATA_ERROR_UP_FREQ)
			  && (poc_idt_gps.utc.year == 0
				  || longitude < 0
			      || longitude > 180
			      || latitude < 0
			      || latitude > 90))
		{
			OSI_LOGI(0,"[GPS][EVENT]:gps (utc)(longitude)(latitude) error");
			goto gpserror;
		}

		if(poc_idt_gps.speed != 0)
		{
			break;
		}
	}
#ifdef SUPPORT_GPS_LOG
    OSI_LOGXI(OSI_LOGPAR_IS, 0,"[GPS][EVENT]len=(%d)(%s)", plen, gps_data);
	OSI_LOGI(0,"[GPS][EVENT]: gps.nshemi(%c)", poc_idt_gps.nshemi);//latitude
	OSI_LOGI(0,"[GPS][EVENT]: gps.ewhemi(%c)", poc_idt_gps.ewhemi);//longitude
	OSI_LOGI(0,"[GPS][EVENT]: gps.gpssta(%d)", poc_idt_gps.gpssta);//0--no locate
	OSI_LOGI(0,"[GPS][EVENT]: gps.fixmode(%d)", poc_idt_gps.fixmode);//0--no locate
	OSI_LOGI(0,"[GPS][EVENT]: UTC Date:%04d/%02d/%02d", poc_idt_gps.utc.year,poc_idt_gps.utc.month,poc_idt_gps.utc.date);
	OSI_LOGI(0,"[GPS][EVENT]: UTC Time:%02d:%02d:%02d", poc_idt_gps.utc.hour,poc_idt_gps.utc.min,poc_idt_gps.utc.sec);
	OSI_LOGI(0,"[GPS][EVENT]: speed:%d\r\n",poc_idt_gps.speed);
	OSI_LOGXI(OSI_LOGPAR_IF, 0,"[GPS][EVENT]:(%c%f)", poc_idt_gps.nshemi, longitude);
	OSI_LOGXI(OSI_LOGPAR_IF, 0,"[GPS][EVENT]:(%c%f)", poc_idt_gps.ewhemi, latitude);
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
			|| gps_t.latitude == 0
			|| gps_t.longitude == 0)
		{
			OSI_LOGI(0,"[GPS][EVENT]:gps data error");
			goto gpserror;
		}

		lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_GPS_UPLOADING_IND, &gps_t);
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
	pocGpsIdtAttr.gps_i2c_error_status = true;
	pocGpsIdtAttr.gps_location_status = false;
	prvlvPocGpsI2cClose(DRV_NAME_I2C2, DRV_I2C_BPS_100K);
	osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
	osiTimerStart(pocGpsIdtAttr.GpsSuspendTimer, GPS_RUN_TIMER_CB_FREQ);
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
    halPmuSwitchPower(HAL_POWER_CAMA, true, true);
    halPmuSwitchPower(HAL_POWER_VIBR, true, true);
}

static
void prvlvPocGpsIdtComCloseGpsPm(void)
{
	poc_set_gps_ant_status(false);
	halPmuSwitchPower(HAL_POWER_CAMA, false, false);
	halPmuSwitchPower(HAL_POWER_VIBR, false, false);
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
					osiTimerStartPeriodicRelaxed(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ, OSI_WAIT_FOREVER);
				}
				else
				{
					osiTimerStartPeriodicRelaxed(pocGpsIdtAttr.GpsInfoTimer, GPS_LOCATION_CHECK_REPORT_FREQ, OSI_WAIT_FOREVER);
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
				osiTimerStartPeriodicRelaxed(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ, OSI_WAIT_FOREVER);
				break;
			}

			case LVPOCGPSIDTCOM_SIGNAL_GPS_LOCATION_REPORT_FREQ:
			{
				OSI_LOGI(0, "[GPS][EVENT]modify report period");
				osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
				osiTimerStartPeriodicRelaxed(pocGpsIdtAttr.GpsInfoTimer, GPS_LOCATION_CHECK_REPORT_FREQ, OSI_WAIT_FOREVER);
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
		&& (pocGpsIdtAttr.scannumber >= GPS_NO_LOCATION_OUTAGE_FREQ
			|| pocGpsIdtAttr.gps_i2c_error_status == true))
	{
		pocGpsIdtAttr.gps_i2c_error_status = false;
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
	osiTimerStartRelaxed(pocGpsIdtAttr.RestartGpsTimer, GPS_NO_LOCATION_POWER_OFF_FREQ, OSI_WAIT_FOREVER);
	OSI_LOGI(0, "[GPS][EVENT]start GPS power off");
}

static
void prvlvPocGpsIdtComHandleRestartTimercb(void *ctx)
{
	osiTimerStartPeriodicRelaxed(pocGpsIdtAttr.GpsInfoTimer, GPS_RUN_TIMER_CB_FREQ, OSI_WAIT_FOREVER);
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
	prvGPSReadReg(0x82, gps_data, GPS_RX_BUF_SIZE);
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
	if(pubPocIdtGpsLocationStatus())
	{
		osiTimerStop(pocGpsIdtAttr.GpsInfoTimer);
	}
	pocGpsIdtAttr.isReady = false;
	prvlvPocGpsIdtComSuspend();
	prvlvPocGpsIdtComCloseGpsPm();
}

void publvPocGpsIdtComWake(void)
{
	if(pocGpsIdtAttr.ishavegps == false)
	{
		OSI_LOGI(0, "[GPS][EVENT]wake and no GPS module");
		return;
	}
	pocGpsIdtAttr.isReady = true;
	prvlvPocGpsIdtComOpenGpsPm();
	lv_poc_show_gps_location_status_img(false);
	prvlvPocGpsIdtComResume();
	osiTimerStartPeriodicRelaxed(pocGpsIdtAttr.GpsInfoTimer, GPS_NO_LOCATION_CHECK_SCAN_FREQ, OSI_WAIT_FOREVER);
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

