#ifdef __cplusplus
extern "C" {
#endif

#include "lv_include/lv_poc.h"
#include "../include/at_cfg.h"
#include "../include/at_engine.h"

#include <stdlib.h>
#include <stdio.h>
#include "sockets.h"

#define AT_SNTP_SERVER_MAX_LENTH 128

/*授时中心*/
static char *serverip = "time.windows.com";

/*sntp 外部定义函数*/
extern CFW_SNTP_CONFIG *sntpClient;
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
void lv_poc_sntp_Update_Time(void)
{
	sntp_status_t result;//同步更新时间结果

	if(Net_Time_flag == true) return;//外部已通过nas信令完成校时 判断为移动或电信卡

	if (sntpClient == NULL)
	{
		sntpConfigInit();
		if (sntpClient == NULL)
		{
			OSI_LOGI(0, "[sntp]AT_CmdFunc_SNTP:  malloc failed,ERR_AT_CME_NO_MEMORY\n");
		}
	}
	else
	{
		OSI_LOGI(0, "[sntp]AT_CmdFunc_SNTP:  sync time processing,ERR_AT_CME_SNTP_SYNCING\n");
	}

	strncpy(sntpClient->cServer, serverip, AT_SNTP_SERVER_MAX_LENTH);
	sntpClient->cServer[AT_SNTP_SERVER_MAX_LENTH] = '\0';

	OSI_LOGXI(OSI_LOGPAR_S, 0, "[sntp]AT_CmdFunc_SNTP: ntpServer = %s", sntpClient->cServer);

	result = sntpUpdateStart(sntpClient);

	if (result == CFW_SNTP_READY)
	{
		OSI_LOGI(0, "[sntp]AT_CmdFunc_SNTP: CFW_SNTP_READY\n");
	}
	else if (result == CFW_SNTP_PARAM_INVALID)
	{
		OSI_LOGI(0, "[sntp]AT_CmdFunc_SNTP: CFW_SNTP_PARAM_INVALID\n");
		free(sntpClient);//ERR_AT_CME_PARAM_INVALID
		sntpClient = NULL;
	}
	else
	{
		OSI_LOGI(0, "[sntp]AT_CmdFunc_SNTP:  sync time is processing,ERR_AT_CME_SNTP_SYNCING\n");
	}

}

extern uint8_t Mapping_Creg_From_PsType(uint8_t pstype);

uint8_t cereg_Respond(bool reportN)
{
    uint8_t ret = 1;
	uint32_t uErrCode = 0;
	CFW_NW_STATUS_INFO sStatus;
	CFW_NW_STATUS_INFO lastNbPsInfo = {
    	.nStatus = 0xff,
	};
    uint8_t nSim = POC_SIM_1;
    uint8_t nCurrRat = CFW_NWGetStackRat(nSim);
    uint8_t reg = gAtSetting.sim[nSim].cereg;
    uint8_t activeTime[8];
    uint8_t periodicTau[8];

	uErrCode = CFW_GprsGetstatus(&sStatus, nSim);
	if (uErrCode != 0)
    {
        return -1;
    }

    _Cereg_UtoBinString(activeTime, sStatus.activeTime);
    _Cereg_UtoBinString(periodicTau, sStatus.periodicTau);
    bool nwInfoChange = false;
    char *rsp = (char *)malloc(64);
    char *respond = rsp;
    if (reportN == true)
    {
        respond += sprintf(respond, "+CEREG: %d, ", gAtSetting.sim[nSim].cereg);
        if ((gAtSetting.sim[nSim].cereg == 4) || (gAtSetting.sim[nSim].cereg == 5))
        {
            reg = 5;
        }
        else if (gAtSetting.sim[nSim].cereg == 1)
        {
            reg = 1;
        }
        else
        {
            reg = 3;
        }
    }
    else
    {
        respond += sprintf(respond, "+CEREG: ");
    }
    switch (reg)
    {
    case 0:
        ret = 0;
        break;

    case 1:
        if (sStatus.nStatus != lastNbPsInfo.nStatus)
        {
            nwInfoChange = true;
        }
        if ((reportN == true) || ((reportN == false) && (nwInfoChange == true)))
            respond += sprintf(respond, "%d", sStatus.nStatus);
        else
            ret = 0;
        break;

    case 2:
    CeregLabel2:
        if ((sStatus.nStatus != lastNbPsInfo.nStatus) || !_Cereg_MemCompare(lastNbPsInfo.nAreaCode, sStatus.nAreaCode, 2) || !_Cereg_MemCompare(lastNbPsInfo.nCellId, sStatus.nCellId, 4))
        {
            nwInfoChange = true;
        }
        if ((reportN == true) || ((reportN == false) && (nwInfoChange == true)))
        {
            respond += sprintf(respond, "%d,\"%02x%02x\",\"%02x%02x%02x%02x\",%d",
                               sStatus.nStatus, sStatus.nAreaCode[3], sStatus.nAreaCode[4],
                               sStatus.nCellId[0], sStatus.nCellId[1], sStatus.nCellId[2], sStatus.nCellId[3], Mapping_Creg_From_PsType(nCurrRat));
        }
        else
            ret = 0;
        break;

    case 3:
    CeregLabel3:
        if ((sStatus.nStatus != lastNbPsInfo.nStatus) || !_Cereg_MemCompare(lastNbPsInfo.nAreaCode, sStatus.nAreaCode, 2) || !_Cereg_MemCompare(lastNbPsInfo.nCellId, sStatus.nCellId, 4) ||
            (lastNbPsInfo.cause_type != sStatus.cause_type) || (lastNbPsInfo.reject_cause != sStatus.reject_cause))
        {
            nwInfoChange = true;
        }
        if ((sStatus.nStatus == 0) || (sStatus.nStatus == 3) || (sStatus.nStatus == 4))
        {

            if ((reportN == true) || ((reportN == false) && (nwInfoChange == true)))
            {
                respond += sprintf(respond, "%d,\"%02x%02x\",\"%02x%02x%02x%02x\",%d,%d,%d",
                                   sStatus.nStatus, sStatus.nAreaCode[3], sStatus.nAreaCode[4],
                                   sStatus.nCellId[0], sStatus.nCellId[1], sStatus.nCellId[2], sStatus.nCellId[3],
                                   Mapping_Creg_From_PsType(nCurrRat),
                                   sStatus.cause_type, sStatus.reject_cause);
            }
            else
                ret = 0;
        }
        else
        {
            goto CeregLabel2;
        }
        break;

    case 4:
    CeregLabel4:
        if ((sStatus.nStatus != lastNbPsInfo.nStatus) || !_Cereg_MemCompare(lastNbPsInfo.nAreaCode, sStatus.nAreaCode, 2) || !_Cereg_MemCompare(lastNbPsInfo.nCellId, sStatus.nCellId, 4) ||
            (lastNbPsInfo.activeTime != sStatus.activeTime) || (lastNbPsInfo.periodicTau != sStatus.periodicTau))
        {
            nwInfoChange = true;
        }
        if ((sStatus.activeTime != 0xff) && (sStatus.periodicTau != 0xff))
        {

            if ((reportN == true) || ((reportN == false) && (nwInfoChange == true)))
            {
                respond += sprintf(respond, "%d,\"%02x%02x\",\"%02x%02x%02x%02x\", %d,,,\"%c%c%c%c%c%c%c%c\",\"%c%c%c%c%c%c%c%c\"",
                                   sStatus.nStatus, sStatus.nAreaCode[3], sStatus.nAreaCode[4],
                                   sStatus.nCellId[0], sStatus.nCellId[1], sStatus.nCellId[2], sStatus.nCellId[3],
                                   Mapping_Creg_From_PsType(nCurrRat),
                                   activeTime[0], activeTime[1], activeTime[2], activeTime[3],
                                   activeTime[4], activeTime[5], activeTime[6], activeTime[7],
                                   periodicTau[0], periodicTau[1], periodicTau[2], periodicTau[3],
                                   periodicTau[4], periodicTau[5], periodicTau[6], periodicTau[7]);
            }
            else
                ret = 0;
        }
        else if (sStatus.activeTime != 0xff)
        {
            if ((reportN == true) || ((reportN == false) && (nwInfoChange == true)))
            {
                respond += sprintf(respond, "%d,\"%02x%02x\",\"%02x%02x%02x%02x\",%d,,,\"%c%c%c%c%c%c%c%c\"",
                                   sStatus.nStatus, sStatus.nAreaCode[3], sStatus.nAreaCode[4],
                                   sStatus.nCellId[0], sStatus.nCellId[1], sStatus.nCellId[2], sStatus.nCellId[3],
                                   Mapping_Creg_From_PsType(nCurrRat),
                                   activeTime[0], activeTime[1], activeTime[2], activeTime[3],
                                   activeTime[4], activeTime[5], activeTime[6], activeTime[7]);
            }
            else
                ret = 0;
        }
        else if (sStatus.periodicTau != 0xff)
        {
            if ((reportN == true) || ((reportN == false) && (nwInfoChange == true)))
            {
                respond += sprintf(respond, "%d,\"%02x%02x\",\"%02x%02x%02x%02x\", %d,,,,\"%c%c%c%c%c%c%c%c\"",
                                   sStatus.nStatus, sStatus.nAreaCode[3], sStatus.nAreaCode[4],
                                   sStatus.nCellId[0], sStatus.nCellId[1], sStatus.nCellId[2], sStatus.nCellId[3],
                                   Mapping_Creg_From_PsType(nCurrRat),
                                   periodicTau[0], periodicTau[1], periodicTau[2], periodicTau[3],
                                   periodicTau[4], periodicTau[5], periodicTau[6], periodicTau[7]);
            }
            else
                ret = 0;
        }
        else
        {
            goto CeregLabel2;
        }
        break;

    case 5:
        if ((sStatus.nStatus != lastNbPsInfo.nStatus) || !_Cereg_MemCompare(lastNbPsInfo.nAreaCode, sStatus.nAreaCode, 2) || !_Cereg_MemCompare(lastNbPsInfo.nCellId, sStatus.nCellId, 4) ||
            (lastNbPsInfo.activeTime != sStatus.activeTime) || (lastNbPsInfo.periodicTau != sStatus.periodicTau) ||
            (lastNbPsInfo.cause_type != sStatus.cause_type) || (lastNbPsInfo.reject_cause != sStatus.reject_cause))
        {
            nwInfoChange = true;
        }
        if ((sStatus.nStatus == 0) || (sStatus.nStatus == 3) || (sStatus.nStatus == 4))
        {
            if ((sStatus.activeTime != 0xff) && (sStatus.periodicTau != 0xff))
            {
                if ((reportN == true) || ((reportN == false) && (nwInfoChange == true)))
                {
                    respond += sprintf(respond, "%d,\"%02x%02x\",\"%02x%02x%02x%02x\", %d,%d,%d,\"%c%c%c%c%c%c%c%c\",\"%c%c%c%c%c%c%c%c\"",
                                       sStatus.nStatus, sStatus.nAreaCode[3], sStatus.nAreaCode[4],
                                       sStatus.nCellId[0], sStatus.nCellId[1], sStatus.nCellId[2], sStatus.nCellId[3],
                                       Mapping_Creg_From_PsType(nCurrRat),
                                       sStatus.cause_type, sStatus.reject_cause,
                                       activeTime[0], activeTime[1], activeTime[2], activeTime[3],
                                       activeTime[4], activeTime[5], activeTime[6], activeTime[7],
                                       periodicTau[0], periodicTau[1], periodicTau[2], periodicTau[3],
                                       periodicTau[4], periodicTau[5], periodicTau[6], periodicTau[7]);
                }
                else
                    ret = 0;
            }
            else if (sStatus.activeTime != 0xff)
            {
                if ((reportN == true) || ((reportN == false) && (nwInfoChange == true)))
                {
                    respond += sprintf(respond, "%d,\"%02x%02x\",\"%02x%02x%02x%02x\", %d,%d,%d,\"%c%c%c%c%c%c%c%c\"",
                                       sStatus.nStatus, sStatus.nAreaCode[3], sStatus.nAreaCode[4],
                                       sStatus.nCellId[0], sStatus.nCellId[1], sStatus.nCellId[2], sStatus.nCellId[3],
                                       Mapping_Creg_From_PsType(nCurrRat),
                                       sStatus.cause_type, sStatus.reject_cause,
                                       activeTime[0], activeTime[1], activeTime[2], activeTime[3],
                                       activeTime[4], activeTime[5], activeTime[6], activeTime[7]);
                }
            }
            else if (sStatus.periodicTau != 0xff)
            {
                if ((reportN == true) || ((reportN == false) && (nwInfoChange == true)))
                {
                    respond += sprintf(respond, "%d,\"%02x%02x\",\"%02x%02x%02x%02x\", %d,%d,%d,,\"%c%c%c%c%c%c%c%c\"",
                                       sStatus.nStatus, sStatus.nAreaCode[3], sStatus.nAreaCode[4],
                                       sStatus.nCellId[0], sStatus.nCellId[1], sStatus.nCellId[2], sStatus.nCellId[3],
                                       Mapping_Creg_From_PsType(nCurrRat),
                                       sStatus.cause_type, sStatus.reject_cause,
                                       periodicTau[0], periodicTau[1], periodicTau[2], periodicTau[3],
                                       periodicTau[4], periodicTau[5], periodicTau[6], periodicTau[7]);
                }
                else
                    ret = 0;
            }
            else
            {
                goto CeregLabel3;
            }
        }
        else
        {
            goto CeregLabel4;
        }
        break;

    default:
        ret = 2;
        break;
    }
    _Cereg_MemCopy8(&lastNbPsInfo, &sStatus, sizeof(CFW_NW_STATUS_INFO));
	if(NULL != rsp)
	{
		free(rsp);
		rsp = NULL;
	}
    return ret;
}

void _Cereg_UtoBinString(uint8_t *string, uint8_t value)
{
    uint8_t i = 0;
    while (i < 8)
    {
        string[7 - i] = ((value >> i) & 0x01) + '0';
        i++;
    }
}

uint16_t _Cereg_MemCompare(const void *buf1, const void *buf2, uint16_t count)
{
    const char *su1, *su2;
    int res = 0;

    for (su1 = buf1, su2 = buf2; 0 < count; ++su1, ++su2, count--)
        if ((res = *su1 - *su2) != 0)
            break;
    return (uint16_t)res;
}

void *_Cereg_MemCopy8(void *dest, const void *src, uint32_t count)
{
    char *tmp = (char *)dest, *s = (char *)src;

    while (count--)
        *tmp++ = *s++;

    return dest;
}

#ifdef __cplusplus
}
#endif
