
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

#define GPS_RX_BUF_SIZE (4*1024)
#define GPS_TX_BUF_SIZE (4*1024)


/***********************************************************************************/



osiWork_t *test_rx_work;
nmea_msg gpsx; 											//GPSÐÅÏ¢
uint8_t gps_data[GPS_RX_BUF_SIZE]={0};

//I2C
static drvI2cMaster_t *i2c_p;
uint8_t salve_i2c_addr_w = 0x60 >> 1;
uint8_t salve_i2c_addr_r = 0x61 >> 1;

//å¼€å¯RMC
uint8_t grmc[12] = {0x23,0x3E,0x03,0x51,0x04,0x00,0x03,0x04,0x01,0x01,0x61,0x82};
uint8_t ggga[12] = {0x23,0x3E,0x03,0x51,0x04,0x00,0x04,0x04,0x01,0x01,0x62,0x86};
uint8_t ggsa[12] = {0x23,0x3E,0x03,0x51,0x04,0x00,0x05,0x04,0x01,0x01,0x63,0x8A};
uint8_t ggsv[12] = {0x23,0x3E,0x03,0x51,0x04,0x00,0x06,0x04,0x01,0x01,0x64,0x8E};


#ifdef SUPPORTONEREG
static void prvGPSWriteOneReg(uint8_t addr, uint8_t data)
{
    drvI2cSlave_t idAddress = {salve_i2c_addr_w, addr, 0, false};
    if (i2c_p != NULL)
    {
        drvI2cWrite(i2c_p, &idAddress, &data, 1);
    }
    else
    {
        OSI_LOGE(0, "[GPS]i2c is not open");
    }
}
#endif

static void prvGPSReadReg(uint8_t addr, uint8_t *data, uint32_t len)
{
    drvI2cSlave_t idAddress = {salve_i2c_addr_w, addr, 0, false};
    if (i2c_p != NULL)
    {
        drvI2cRead(i2c_p, &idAddress, data, len);
    }
    else
    {
        OSI_LOGE(0, "[GPS]i2c is not open");
    }
}


static bool prvGPSWriteRegList(uint8_t *data, uint16_t len)
{
    drvI2cSlave_t wirte_data = {salve_i2c_addr_w, 0, 0, false};

    if (drvI2cWrite(i2c_p, &wirte_data, data, len))
        return true;
    else
        return false;

}


void GpsThreadEntry(void *ctx)
{
	float longitude = 0;
	float latitude = 0;
	uint16_t plen = 0;
	unsigned char buf[2] = {0xff, 0xff};

	i2c_p = drvI2cMasterAcquire(DRV_NAME_I2C2, DRV_I2C_BPS_100K);

	hwp_iomux->pad_gpio_14_cfg_reg |= IOMUX_PAD_GPIO_14_SEL_V_FUN_I2C_M2_SCL_SEL;
    hwp_iomux->pad_gpio_15_cfg_reg |= IOMUX_PAD_GPIO_15_SEL_V_FUN_I2C_M2_SDA_SEL;

	if (i2c_p == NULL)
	{
		  OSI_LOGI(0, "[GPS]drvI2cMasterAcquire open fail");
		  if (i2c_p != NULL)
          	drvI2cMasterRelease(i2c_p);
	}
	else
	{
		prvGPSReadReg(0x82, gps_data, 4000);

		if (!prvGPSWriteRegList(grmc, 12))
		{
			OSI_LOGI(0, "[GPS]prvGPSWriteRegList fail");
		}
		prvGPSWriteRegList(ggga, 12);
		prvGPSWriteRegList(ggsa, 12);
		prvGPSWriteRegList(ggsv, 12);
	}
	while(1)
	{
		prvGPSReadReg(0x80, buf, 2);
		plen = (buf[0]<<8) | (buf[1]);

		if (plen != 0)
		{
			prvGPSReadReg(0x82, gps_data, plen);
			GPS_Analysis(&gpsx,(uint8_t*)gps_data);
		}

		OSI_LOGI(0,"[GPS]:data len = %d \n", plen);

		longitude = gpsx.longitude / 100000.0;
		latitude  = gpsx.latitude  / 100000.0;
		if('S' == gpsx.nshemi)
		{
			longitude = -longitude;
		}
		if('W' == gpsx.ewhemi)
		{
			latitude = -latitude;
		}
#if 0
        OSI_LOGI(0,"[GPS]: gpsx.nshemi(%c)\r\n", gpsx.nshemi);
        OSI_LOGI(0,"[GPS]: gpsx.ewhemi(%c)\r\n", gpsx.ewhemi);
        OSI_LOGI(0,"[GPS]: gpsx.gpssta(%d)\r\n", gpsx.gpssta);
        OSI_LOGI(0,"[GPS]: gpsx.fixmode(%d)\r\n", gpsx.fixmode);
		OSI_LOGI(0,"[GPS]:UTC Date:%04d/%02d/%02d\r\n", gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);
		OSI_LOGI(0,"[GPS]:UTC Time:%02d:%02d:%02d\r\n", gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);
		OSI_LOGI(0,"[GPS]:speed:%d\r\n",gpsx.speed);
		OSI_LOGXI(OSI_LOGPAR_F,	0,"[GPS]: test read buf(%f)\r\n",longitude);
		OSI_LOGXI(OSI_LOGPAR_F,	0,"[GPS]: test read buf(%f)\r\n",latitude);
        OSI_LOGXI(OSI_LOGPAR_S, 0,"[GPS]1: test read buf(%s)\r\n",gps_data);
		OSI_LOGI(0,"[GPS] data:");
		for(int i = 0; i<plen; i++)
		{
			OSI_LOGI(0," %s ", gps_data[i]);
		}
		OSI_LOGI(0,"\n");
#endif

		OSI_LOGXI(OSI_LOGPAR_S, 0,"[GPS]gps read buf(%s)\r\n",gps_data);
		osiThreadSleep(1000);
	}
}



void gpsInit(void)
{
    OSI_LOGI(0, "[GPS]gpsInit enter!");
    osiThreadCreate("GpsThread", GpsThreadEntry, NULL, OSI_PRIORITY_NORMAL, 1024, 4);
}

