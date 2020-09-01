#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"
#include <stdlib.h>
#include "sockets.h"

#define AT_SNTP_SERVER_MAX_LENTH 128

/*授时中心IP地址 美国时间*/
static char *serverip = "40.81.188.85";//40.81.188.85 //210.72.145.44
static CFW_SNTP_CONFIG *sntpClientOpt = NULL;

/*sntp 外部定义函数*/
extern void sntpConfigInit();
extern sntp_status_t sntpUpdateStart();

/*外部nas信令校时*/
extern bool Net_Time_flag;


/*
	  name : lv_poc_sntp_Update_Time
	 param : none
	author : wangls
  describe : 网络更新时间
	  date : 2020-06-09
*/
void lv_poc_sntp_Update_Time(void *sntpopt)
{
	sntp_status_t result;//同步更新时间结果
	sntpClientOpt =(CFW_SNTP_CONFIG *)sntpopt;
	if(Net_Time_flag == true) return;//外部已通过nas信令完成校时 判断为移动或电信卡

	if (sntpClientOpt == NULL)
	{
		sntpConfigInit();
		if (sntpClientOpt == NULL)
		{
			OSI_LOGI(0, "[song]AT_CmdFunc_SNTP:  malloc failed,ERR_AT_CME_NO_MEMORY\n");
		}
	}
	else
	{
		OSI_LOGI(0, "[song]AT_CmdFunc_SNTP:  sync time processing,ERR_AT_CME_SNTP_SYNCING\n");
	}

	strncpy(sntpClientOpt->cServer, serverip, AT_SNTP_SERVER_MAX_LENTH);
	sntpClientOpt->cServer[AT_SNTP_SERVER_MAX_LENTH] = '\0';

	OSI_LOGXI(OSI_LOGPAR_S, 0, "[song]AT_CmdFunc_SNTP: ntpServer = %s", sntpClientOpt->cServer);

	result = sntpUpdateStart(sntpClientOpt);

	if (result == CFW_SNTP_READY)
	{
		OSI_LOGI(0, "[song]AT_CmdFunc_SNTP: CFW_SNTP_READY\n");
	}
	else if (result == CFW_SNTP_PARAM_INVALID)
	{
		OSI_LOGI(0, "[song]AT_CmdFunc_SNTP: CFW_SNTP_PARAM_INVALID\n");
		free(sntpClientOpt);//ERR_AT_CME_PARAM_INVALID
		sntpClientOpt = NULL;
	}
	else
	{
		OSI_LOGI(0, "[song]AT_CmdFunc_SNTP:  sync time is processing,ERR_AT_CME_SNTP_SYNCING\n");
	}

}


#ifdef __cplusplus
}
#endif
