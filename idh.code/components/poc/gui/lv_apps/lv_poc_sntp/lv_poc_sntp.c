#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"
#include <stdlib.h>
#include "sockets.h"

#define AT_SNTP_SERVER_MAX_LENTH 128

/*授时中心*/
static char *serverip = "time.windows.com";

/*sntp 外部定义函数*/
extern void sntpConfigInit();
extern sntp_status_t sntpUpdateStart();
extern CFW_SNTP_CONFIG *sntpClient;

/*外部nas信令校时*/
extern bool Net_Time_flag;


/*
	  name : lv_poc_sntp_Update_Time
	 param : none
	author : wangls
  describe : 网络更新时间
	  date : 2020-06-09
*/
void lv_poc_sntp_Update_Time(void)
{
	sntp_status_t result;//同步更新时间结果

	if(Net_Time_flag == true) return;//外部已通过nas信令完成校时 判断为移动或电信卡

	if (sntpClient == NULL)
	{
		sntpConfigInit();
		if (sntpClient == NULL)
		{
			OSI_LOGI(0, "[song]sntpClient NULL\n");
		}
	}

	strncpy(sntpClient->cServer, serverip, AT_SNTP_SERVER_MAX_LENTH);
	sntpClient->cServer[AT_SNTP_SERVER_MAX_LENTH] = '\0';

	OSI_LOGXI(OSI_LOGPAR_S, 0, "[song]ntpServer = %s", sntpClient->cServer);

	result = sntpUpdateStart(sntpClient);

	if (result == CFW_SNTP_READY)
	{
		OSI_LOGI(0, "[song]sntp ready\n");
	}
	else if (result == CFW_SNTP_PARAM_INVALID)
	{
		OSI_LOGI(0, "[song]param invalid\n");
		free(sntpClient);
		sntpClient = NULL;
	}
	else
	{
		OSI_LOGI(0, "[song]sync time is processing!\n");
	}

}


#ifdef __cplusplus
}
#endif
