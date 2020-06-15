
/***********************************************************************************/


#include "stdlib.h"
#include "string.h"
#include "poc_config.h"
#include <stdio.h>
#include "osi_log.h"
#include "osi_api.h"
#include "hal_iomux_pindef.h"
#include "hal_iomux.h"
#include "gps_nmea.h"
#include "hal_chip.h"
#include "drv_uart.h"


#define GPS_RX_BUF_SIZE (4*1024)
#define GPS_TX_BUF_SIZE (4*1024)





/***********************************************************************************/


static unsigned int uart_names[3] =  {DRV_NAME_UART1,DRV_NAME_UART2,DRV_NAME_UART3};

static drvUart_t *test_uart;
static uint8_t uartok=0;
osiWork_t *test_rx_work;
nmea_msg gpsx; 											//GPS–≈œ¢
uint8_t test_data[GPS_RX_BUF_SIZE]={0};

static void _testUartCB(void *param, uint32_t event)
{
	OSI_LOGI(0,"[GPS] gps_uart data coming\n");
    if (DRV_UART_EVENT_RX_ARRIVED == event)
    {
        //osiWorkEnqueue(test_rx_work, osiSysWorkQueueLowPriority());
    	uartok = 1;
    }
    else if(DRV_UART_EVENT_RX_OVERFLOW == event)
    {
        OSI_LOGI(0,"[GPS] gps_uart overflow\n");
    }
    return;
}


bool testOpenUart(uint32_t name, unsigned baud)
{
    drvUartCfg_t uart_cfg = {
        .baud = baud,
        .data_bits = DRV_UART_DATA_BITS_8,
        .stop_bits = DRV_UART_STOP_BITS_1,
        .parity = DRV_UART_NO_PARITY,
        .rx_buf_size = GPS_RX_BUF_SIZE,
        .tx_buf_size = GPS_TX_BUF_SIZE,
        //.event_mask = 0,
         .event_mask = DRV_UART_EVENT_RX_ARRIVED | DRV_UART_EVENT_RX_OVERFLOW,
        .event_cb = _testUartCB,
        .event_cb_ctx = NULL,
    };
    test_uart = drvUartCreate(name, &uart_cfg);
    if ((test_uart != NULL) && (drvUartOpen(test_uart)))
    {
        OSI_LOGI(0,"[GPS] test drvUartCreate succ!\n");
    }
    else
    {
		drvUartDestroy(test_uart);
        test_uart = NULL;
        OSI_LOGI(0,"[GPS] test drvUartCreate fail!\n");
    }
	return true;
}


void testThreadEntry(void *ctx)
{
	float longitude = 0;
	float latitude = 0;
    halIomuxRequest(PINFUNC_UART_3_TXD);
    halIomuxRequest(PINFUNC_UART_3_RXD);

    halPmuSetPowerLevel(HAL_POWER_CAMA, SENSOR_VDD_2800MV);
    osiDelayUS(2);

	testOpenUart(uart_names[2],9600);

	while(1)
	{
        if(uartok==1)
        {
		    osiThreadSleep(80);
            int trans = drvUartReceive(test_uart, test_data, 4000);
			GPS_Analysis(&gpsx,(uint8_t*)test_data);
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
            OSI_LOGXI(OSI_LOGPAR_S, 0,"[GPS]1: test read buf(%s)\r\n",test_data);
            OSI_LOGI(0,"[GPS]: test read len(%d)\r\n", trans);
#endif
            uartok = 0;
        }
		osiThreadSleep(2000);
        //OSI_LOGI(0,"[GPS] test drvUartCreate succ!\n");
	}
}


void gpsInit(void)
{
    OSI_LOGI(0, "test gpsInit enter!");
    osiThreadCreate("testThread", testThreadEntry, NULL, OSI_PRIORITY_NORMAL, 1024, 4);
}

