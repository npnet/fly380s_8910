/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#include "osi_log.h"
#include "at_command.h"
#include "at_response.h"
#include "at_cfw.h"
#include "at_cfg.h"
#include "cfw_event.h"
#include "cfw_errorcode.h"
#include "drv_rtc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef CONFIG_SOC_6760
#include "diag.h"
#include "diag_nw.h"
#endif
#include "nvm.h"
#include "netmain.h"
#include "ppp_interface.h"

uint32_t g_COPN_Index = 1;
uint8_t g_COPS_Mode = 2;
uint32_t g_COPS_Index = 0;
uint8_t *g_COPS_Name = 0;
uint8_t g_FirstREG = 1;
uint8_t g_COPS_OperID[6] = {0};
uint8_t g_COPS_OperID_Temp[6] = {0};
uint16_t nFormat = 0;
uint8_t g_CSCON_state = 7;
uint8_t g_CSCON_access = 3;
extern bool Speeching;        // added by yangtt at 2008-05-05 for bug 8227
extern uint8_t g_cfg_cfun[2]; // added by yangtt at 2008-05-05 for bug 8411
extern bool g_auto_calib;
extern uint8_t g_set_ACLB;
extern uint8_t g_ACLB_FM;
extern uint8_t g_ACLB_SIM1_FM;
extern bool gATSATLocalInfoFlag[];
extern bool gATSATComQualifier[];
osiTimer_t *ccedtimer = NULL;
osiTimer_t *ccedtimerV2 = NULL;
osiTimer_t *ccedtimerV3 = NULL;
osiTimer_t *ccedtimerV4 = NULL;

extern uint32_t _SetACLBStatus(uint8_t nACLBNWStatus);
void AT_NW_GetStoreListString(uint8_t nSim, uint8_t *nDataString);
extern uint32_t CFW_NW_SendPLMNList(CFW_StoredPlmnList *plmnlist, uint8_t nSimID);
extern uint32_t CFW_SimWritePreferPLMN(uint8_t index, uint8_t *operator, uint8_t nSize, uint16_t nUTI, CFW_SIM_ID nSimID);
extern uint32_t CFW_SimReadPreferPLMN(uint16_t nUTI, CFW_SIM_ID nSimID);
extern uint32_t CFW_NwGetLteSignalQuality(uint8_t *pSignalLevel, uint8_t *pBitError, CFW_SIM_ID nSimID);
extern uint32_t CFW_CfgNwSetDetectMBS(uint8_t nMode);
bool AT_EMOD_CCED_ProcessData(uint8_t nSim, CFW_TSM_CURR_CELL_INFO *pCurrCellInfo,
                              CFW_TSM_ALL_NEBCELL_INFO *pNeighborCellInfo, char *nStrPosiList);
uint8_t Mapping_Creg_From_PsType(uint8_t pstype);
#ifdef AT_OCEANCONNECT_SUPPORT
extern int8_t get_Oceanconnect_ref(void);
#endif
#define NITZ_SIGN_BIT 0x08
#define NITZ_HIGH_BIT 0xF0
#define NITZ_LOW_BIT 0x0F
#define NITZ_TZ_BIT 0x07

void atSetFlightModeFlag(uint8_t flag, CFW_SIM_ID nSimID)
{
    gAtSetting.sim[nSimID].flight_mode = flag;
    return;
}
uint8_t atGetFlightModeFlag(CFW_SIM_ID nSimID)
{
    return gAtSetting.sim[nSimID].flight_mode;
}

void CFW_GetIMEIFromAP(uint8_t *pImei, uint8_t *pImeiLen, CFW_SIM_ID nSimID)
{
    uint8_t nImei[16] = {0};
    int nImeiLen = nvmReadImei(nSimID, (nvmImei_t *)nImei);
    if (nImeiLen != -1)
    {
        *pImeiLen = 16;
        memcpy(pImei, nImei, 14);
        memcpy(pImei + 14, "00", 2);
    }
    else
    {
        *pImeiLen = 0;
    }
}

static int _printOperID(char *s, uint8_t *oper_id)
{
    char *start = s;
    for (int n = 0; n < 6; n++)
    {
        if (oper_id[n] > 9)
            break;
        *s++ = '0' + oper_id[n];
    }
    *s++ = '\0';
    return s - start - 1;
}

#ifdef AT_NITZ_SUPPORT
typedef struct at_nw_time_zone_time_struct
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t time_zone;
} at_nw_time_zone_time_struct;

typedef struct
{
    U16 nYear;
    uint8_t nMonth;
    uint8_t nDay;
    uint8_t nHour;
    uint8_t nMin;
    uint8_t nSec;
    uint8_t DayIndex; /* 0=Sunday */
} AT_MYTIME;

#define YEARFORMATE 2000
#define NUM_MONTHS 12
#define NUM_MIN_HR 60
#define NUM_HRS_DAY 24
#define JAN 1
#define FEB 2
#define DEC 12

const uint8_t gATDaysInMonth[NUM_MONTHS] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
AT_MYTIME AT_NITZ_RecvTime; //receive nitz system time
AT_MYTIME AT_NITZ_Time;     //net time convert to local time
AT_MYTIME AT_NITZ_RefTime;  // net time
int16_t AT_NITZ_TimeZone;

uint8_t gNitzGet = 0;
#endif

bool Net_Time_flag = false;//是否有下发网络时间

typedef struct AT_CCED
{
    uint8_t sMcc[3];
    uint8_t sMnc[3];
    uint16_t sLac;
    uint16_t sCellID;
    uint8_t iBsic;
    uint8_t iRxLev;
    uint8_t iRxLevSub;
    uint8_t iEndED;
    uint16_t nArfcn;
} AT_CCED;

typedef struct AT_NETMSG_GSMDATA
{
    uint8_t sMcc[3];
    uint8_t sMnc[3];
    uint16_t sLac;
    uint16_t sCellID;
    uint8_t iBsic;
    uint8_t iNetMode;
    uint8_t iBandInfo;
    int16_t rxdBm;
    int16_t txdBm;
    uint8_t iRssi;
    uint32_t nArfcn;
} AT_NETMSG_GSMDATA;
// AT_20071112_CAOW_E
#ifdef AT_FTP_SUPPORT
bool AT_SetOTATimer(uint32_t nElapse);
#endif
AT_CS_STAUTS gCsStauts;

void cfw_PLMNtoBCD(uint8_t *pIn, uint8_t *pOut, uint8_t *nInLength);

#ifdef PORTING_ON_GOING

void NWSATCellInfo(CFW_EVENT cfwEvent)
{
    uint8_t nSim = cfw_event->nFlag;

    OSI_LOGI(0x100052d0, "NWSATCellInfo cfw_event->nType0x%x!\n", cfw_event->nType);

    if (cfw_event->nType == 0xF0)
    {
        OSI_LOGI(0x100048a3, "NW_AsyncEvent ERROR");
        uint32_t nErrorCode = AT_SetCmeErrorCode(cfw_event->nParam1, false);
        AT_NW_Result_Err(nErrorCode, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
        return;
    }
    if (cfw_event->nType != 0)
    {
        OSI_LOGI(0x100052d1, "NW_AsyncEvent:    ERROR!!!---cfw_event->nType --0x%x\n\r", cfw_event->nType);
        AT_NW_Result_Err(ERR_AT_CME_EXE_FAIL, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
        return;
    }
    OSI_LOGI(0x100052d2, "NWSATCellInfo cfw_event->nParam2 0x%x!\n", cfw_event->nParam2);

    if (cfw_event->nParam2 == CFW_TSM_CURRENT_CELL)
    {
        CFW_TSM_CURR_CELL_INFO tCurrCellInf;
        CFW_TSM_ALL_NEBCELL_INFO tNeighborCellInfo;

        AT_MemSet(&tCurrCellInf, 0x0, sizeof(CFW_TSM_CURR_CELL_INFO));
        AT_MemSet(&tNeighborCellInfo, 0x0, sizeof(CFW_TSM_ALL_NEBCELL_INFO));

        uint32_t nRet = CFW_GetCellInfo(&tCurrCellInf, &tNeighborCellInfo, nSim);
        CFW_EmodOutfieldTestEnd(cfw_event->nUTI, nSim);

        // hal_DbgAssert("_onEV_CFW_TSM_INFO_IND CFW_TSM_CURRENT_CELL", NULL);
        if (nRet != 0)
        {
            OSI_LOGI(0x100048a7, "NW_AsyncEvent ERROR nRet:0x%x!\n", nRet);
            AT_NW_Result_Err(ERR_AT_CME_EXE_FAIL, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
            return;
        }
        if (0x00 == gATSATComQualifier[nSim])
        {
            SAT_LOCALINFO_RSP LocaInf;
            uint8_t nMNClen = 3;
            memset(&LocaInf, 0, sizeof(SAT_LOCALINFO_RSP));
            LocaInf.nCellIdentify = (tCurrCellInf.nTSM_CellID[0] << 8) | (tCurrCellInf.nTSM_CellID[1]);

            cfw_PLMNtoBCD(tCurrCellInf.nTSM_LAI, LocaInf.nMCC_MNC_Code, &nMNClen);

            LocaInf.nLocalAreaCode = (tCurrCellInf.nTSM_LAI[3] << 8) | (tCurrCellInf.nTSM_LAI[4]);

            uint8_t nUTI = 0x00;
            AT_GetFreeUTI(CFW_NW_SRV_ID, &nUTI);

            CFW_NW_STATUS_INFO nStatusInfo;
            uint32_t nStatus = 0x00;
            if (0 != CFW_NwGetStatus(&nStatusInfo, nSim))
            {
                OSI_LOGI(0x0810095c, "CFW_NwGetStatus Error\n");

                AT_NW_Result_Err(ERR_AT_CME_OPERATION_NOT_ALLOWED, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
                return;
            }
            OSI_LOGI(0x0810095d, "NWSATCellInfo nStatusInfo.nStatus  %d\n", nStatusInfo.nStatus);

            if (nStatusInfo.nStatus == CFW_NW_STATUS_NOTREGISTERED_SEARCHING || nStatusInfo.nStatus == CFW_NW_STATUS_NOTREGISTERED_NOTSEARCHING ||
                nStatusInfo.nStatus == CFW_NW_STATUS_REGISTRATION_DENIED || nStatusInfo.nStatus == CFW_NW_STATUS_UNKNOW)
            {
                nStatus = 0x06;
            }

            if (0x00 == nStatus)
                nRet = CFW_SatResponse(0x26, 0, 0x0, &LocaInf, sizeof(SAT_LOCALINFO_RSP), nUTI, nSim);
            else
            {
                memset(&LocaInf, 0, sizeof(LocaInf));
                nRet = CFW_SatResponse(0x26, 0, 0x0, &LocaInf, sizeof(SAT_LOCALINFO_RSP), nUTI, nSim);
            }
        }
        else if (0x05 == gATSATComQualifier[nSim])
        {
            SAT_TIMINGADV_RSP nTA;
            nTA.nME_Status = 0x00;
            nTA.nTimingAdv = tCurrCellInf.nTSM_TimeADV;
            uint32_t nStatus = 0x00;
            CFW_NW_STATUS_INFO nStatusInfo;
            if (0 != CFW_NwGetStatus(&nStatusInfo, nSim))
            {
                OSI_LOGI(0x0810095c, "CFW_NwGetStatus Error\n");

                AT_NW_Result_Err(ERR_AT_CME_OPERATION_NOT_ALLOWED, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
                return;
            }
            OSI_LOGI(0x0810095d, "NWSATCellInfo nStatusInfo.nStatus  %d\n", nStatusInfo.nStatus);

            if (nStatusInfo.nStatus == CFW_NW_STATUS_NOTREGISTERED_SEARCHING || nStatusInfo.nStatus == CFW_NW_STATUS_NOTREGISTERED_NOTSEARCHING ||
                nStatusInfo.nStatus == CFW_NW_STATUS_REGISTRATION_DENIED || nStatusInfo.nStatus == CFW_NW_STATUS_UNKNOW)
            {
                nStatus = 0x06;
            }
            if (0x00 == nStatus)
                nRet = CFW_SatResponse(0x26, nStatus, 0x0, &nTA, sizeof(SAT_TIMINGADV_RSP), cfw_event->nUTI, nSim);
            else
                nRet = CFW_SatResponse(0x26, nStatus, 0x0, NULL, 0, cfw_event->nUTI, nSim);
        }

        OSI_LOGI(0x10004592, "CFW_SatResponse nRet:0x%x!\n", nRet);
    }
}
#endif

extern uint32_t gSATCurrentCmdStamp[];
void _onEV_CFW_TSM_INFO_IND(const osiEvent_t *event)
{
    OSI_LOGI(0, "_onEV_CFW_TSM_INFO_IND EV_CFW_TSM_INFO_IND come");
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_SIM_ID nSim = (CFW_SIM_ID)cfw_event->nFlag;
    uint32_t nRet = 0;
    CFW_TSM_CURR_CELL_INFO tCurrCellInf;
    CFW_TSM_ALL_NEBCELL_INFO tNeighborCellInfo;
    char pTxtBuf[500] = {0};
    memset(&tCurrCellInf, 0, sizeof(CFW_TSM_CURR_CELL_INFO));
    memset(&tNeighborCellInfo, 0, sizeof(CFW_TSM_ALL_NEBCELL_INFO));
    if (cfw_event->nType != 0)
        return;
    nRet = CFW_GetCellInfo(&tCurrCellInf, &tNeighborCellInfo, nSim);
    if (nRet != ERR_SUCCESS)
        return;
    if (cfw_event->nParam2 == CFW_TSM_CURRENT_CELL)
        AT_EMOD_CCED_ProcessData(nSim, &tCurrCellInf, NULL, pTxtBuf);
    else
        AT_EMOD_CCED_ProcessData(nSim, NULL, &tNeighborCellInfo, pTxtBuf);
    atCmdRespSimUrcText(nSim, pTxtBuf);
    CFW_EmodOutfieldTestEnd(cfw_event->nUTI, nSim);
}

bool g_simopen_creg_flag = false;

typedef struct
{
    uint16_t Arfcn; // [0..1024]
    uint16_t Rssi;  // [0..120] dBm after L1_LOW_RLA  it's noise only
} NW_FREQ_INFO;

#ifdef LTE_NBIOT_SUPPORT
extern void AT_TryAutoPdnAttach(uint8_t dlci, CFW_SIM_ID nSim);
#endif

static void _onEV_CFW_NW_SIGNAL_QUALITY_IND(const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    uint8_t nSim = cfw_event->nFlag;

    if (gAtSetting.cmer_ind == 2 &&
        gAtSetting.sim[nSim].ecsq == 1 &&
        cfw_event->nType == 0)
    {
        char rsp[32];
        sprintf(rsp, "+ECSQ: %d,%d", (int)cfw_event->nParam1,
                (int)cfw_event->nParam2);
        atCmdRespSimUrcText(nSim, rsp);
    }
}

static void _onEV_CFW_NW_REG_STATUS_IND(const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    uint8_t nSim = cfw_event->nFlag;
    char rsp[32];

    gCsStauts.nParam1 = cfw_event->nParam1;
    gCsStauts.nType = cfw_event->nType;

    if (2 == cfw_event->nType)
    {
        if (g_simopen_creg_flag == true)
        {
            g_simopen_creg_flag = false;
            return;
        }

        if (gAtSetting.cmer_ind == 2)
        {
            if (cfw_event->nParam1 == CFW_NW_STATUS_REGISTERED_HOME ||
                cfw_event->nParam1 == CFW_NW_STATUS_REGISTERED_ROAMING)
                atCmdRespSimUrcText(nSim, "+CIEV: service, 1");
            else
                atCmdRespSimUrcText(nSim, "+CIEV: service, 0");

            if (cfw_event->nParam1 == CFW_NW_STATUS_REGISTERED_ROAMING)
                atCmdRespSimUrcText(nSim, "+CIEV: roam, 1");
            else
                atCmdRespSimUrcText(nSim, "+CIEV: roam, 0");

            if (2 == cfw_event->nParam2)
                atCmdRespSimUrcText(nSim, "+GSM    Service");
            else if (4 == cfw_event->nParam2)
                atCmdRespSimUrcText(nSim, "+UMTS Service");
        }

        uint8_t nw_status = gAtSetting.sim[nSim].creg;
        if (nw_status != 0)
        {
            if (2 == nw_status)
            {
                //uint16_t cell_code = cfw_event->nParam2 & 0xffff;
                //uint16_t area_code = (cfw_event->nParam2 >> 16) & 0xffff;
                CFW_NW_STATUS_INFO nStatusInfo;
                CFW_NwGetStatus(&nStatusInfo, nSim);
                sprintf(rsp, "+CREG: %d,%d,%02x%02x,%02x%02x,%d",
                        nw_status, nStatusInfo.nStatus, nStatusInfo.nAreaCode[3],
                        nStatusInfo.nAreaCode[4], nStatusInfo.nCellId[0],
                        nStatusInfo.nCellId[1], Mapping_Creg_From_PsType((uint8_t)cfw_event->nParam2));

                //sprintf(rsp, "+CREG: %d,\"%04X\",\"%04X\"",
                //        (int)cfw_event->nParam1, area_code, cell_code);
            }
            else
            {
                sprintf(rsp, "+CREG: %d", (int)cfw_event->nParam1);
            }
            atCmdRespSimUrcText(nSim, rsp);

            if (2 == cfw_event->nParam2 && (1 == (int)cfw_event->nParam1))
                atCmdRespSimUrcText(nSim, "+GSM Service");
            else if (4 == cfw_event->nParam2)
                atCmdRespSimUrcText(nSim, "+UMTS Service");
        }
    }
}

static void _onEV_CFW_NW_STORELIST_IND(const osiEvent_t *event)
{
}

static int8_t _timezoneFromNw(uint8_t tz)
{
    int timezone = (tz & 0x07) * 10 + (tz >> 4);
    return (tz & 0x08) ? -timezone : timezone;
}

static uint8_t _fromNwTimeBcd(uint8_t t)
{
    return (t & 0xf) + (t >> 4) * 10;
}

static void _tmFromNw(uint8_t *nw, struct tm *tm)
{
    memset(tm, 0, sizeof(struct tm));
    tm->tm_year = _fromNwTimeBcd(nw[0]) + (2000 - 1900); // Year-2000 -> Year-1900
    tm->tm_mon = _fromNwTimeBcd(nw[1]) - 1;              // (1-12) -> (0-11)
    tm->tm_mday = _fromNwTimeBcd(nw[2]);                 // (1-31)
    tm->tm_hour = _fromNwTimeBcd(nw[3]);                 // (0-23)
    tm->tm_min = _fromNwTimeBcd(nw[4]);                  // (0-59)
    tm->tm_sec = _fromNwTimeBcd(nw[5]);                  // (0-59)
}

static void _onEV_CFW_NW_NETWORKINFO_IND(const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_NW_NETWORK_INFO *p = (CFW_NW_NETWORK_INFO *)cfw_event->nParam1;

    atMemFreeLater(p);

#if defined(AT_CUCC_DATAREG) || defined(AT_CTCC_DATAREG)
    //startIotSelfRegOnce(gAtCurrentSetting.self_register);
#endif
	/*！----------自定义加入这层*/
	/*判断是否有网络下发校时功能*/
	if(p->nUniversalTimeZoneLen == 0)
		Net_Time_flag = false;
	else
 		Net_Time_flag = true;
	/*----------------------！*/

    if (p->nUniversalTimeZoneLen > 0)
    {
        int8_t timezone = _timezoneFromNw(p->nUniversalTimeZone[6]);
        uint8_t timezone_dst = p->nDaylightSavingTime;

        if (gAtSetting.ctzu)
        {
            if (timezone != gAtSetting.timezone ||
                timezone_dst != gAtSetting.timezone_dst)
            {
                gAtSetting.timezone = timezone;
                gAtSetting.timezone_dst = timezone_dst;
                atCfgAutoSave();
            }
        }
    }
    if (p->nUniversalTimeZoneLen > 0)
    {
        int timeoffset = gAtSetting.timezone * CFW_TIME_ZONE_SECOND;

        struct tm tm;
        _tmFromNw(p->nUniversalTimeZone, &tm);

        if (gAtSetting.ctzu)
        {
            osiSetEpochTime(mktime(&tm) * 1000LL);
            drvRtcUpdateTime();
        }

        if (gAtSetting.ctzr != 0)
        {
            char rsp[64];
            if (gAtSetting.ctzr == 1)
            {
                sprintf(rsp, "+CTZV: %+03d", gAtSetting.timezone);
            }
            else if (gAtSetting.ctzr == 2)
            {
                // convert to localtime
                time_t lt = mktime(&tm) + timeoffset;
                struct tm ltm;
                gmtime_r(&lt, &ltm);
                sprintf(rsp, "+CTZE: %+03d,%d,\"%04d/%02d/%02d,%02d:%02d:%02d\"",
                        gAtSetting.timezone, gAtSetting.timezone_dst,
                        ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
                        ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
            }
            else // 3
            {
                sprintf(rsp, "+CTZEU: %+03d,%d,\"%04d/%02d/%02d,%02d:%02d:%02d\"",
                        gAtSetting.timezone, gAtSetting.timezone_dst,
                        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                        tm.tm_hour, tm.tm_min, tm.tm_sec);
            }
            atCmdRespDefUrcText(rsp);
        }
    }
}
#ifdef CFW_VOLTE_SUPPORT
extern CFW_NW_EMC_NUM_LIST_INFO gAtNwEmcNumList;

static void _onEV_CFW_EMC_NUM_LIST_IND(const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    OSI_LOGI(0, "EV_CFW_EMC_NUM_LIST_IND coming");
    if (0x00 == cfw_event->nType)
        memcpy(&gAtNwEmcNumList, (uint8_t *)(cfw_event->nParam1), sizeof(CFW_NW_EMC_NUM_LIST_INFO));
}
#endif

#ifdef CONFIG_SOC_6760
static void _onEV_CFW_NW_PAY_LOAD_INFO_IND(const osiEvent_t *event)
{
#ifdef CONFIG_DIAG_ENABLED
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_NW_PSSTATSINDLTE_INFO *p = (CFW_NW_PSSTATSINDLTE_INFO *)cfw_event->nParam1;
    size_t length = sizeof(CFW_NW_PSSTATSINDLTE_INFO) + p->MsgLen;

    atMemFreeLater(p);

    OSI_LOGI(0x10005301, "We got EV_CFW_NW_PAY_LOAD_INFO_IND,MsgId: 0x%x, MsgLen: 0x%x", p->MsgId, p->MsgLen);

    DiagNwStatusInd(p, length);

#else
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_NW_PSSTATSINDLTE_INFO *p = (CFW_NW_PSSTATSINDLTE_INFO *)cfw_event->nParam1;
    //size_t length = sizeof(CFW_NW_PSSTATSINDLTE_INFO) + p->MsgLen;

    atMemFreeLater(p);
#endif
}
#endif
//MJDC
static void _onEV_CFW_NW_JAMMING_DETECT_IND(const osiEvent_t *event)
{
    char rsp[32] = {
        0,
    };
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_NW_JAMMING_DETECT_GET_IND *p = (CFW_NW_JAMMING_DETECT_GET_IND *)cfw_event->nParam1;

    atMemFreeLater(p);

    if (1 == p->nMode)
    {
        sprintf(rsp, "+MJDC: %d", p->nJamming);
        atCmdRespDefUrcText(rsp);
    }
    else
    {
        OSI_LOGI(0, "_onEV_CFW_NW_JAMMING_DETECT_IND ERROR:nMode: %d", p->nMode);
    }
}

static void _onEV_CFW_ERRC_CONNSTATUS_IND(const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    uint8_t nSim = cfw_event->nFlag;
    uint8_t mode = cfw_event->nType;
    uint32_t nParam2 = cfw_event->nParam2;
    OSI_LOGI(0, "EV_CFW_ERRC_CONNSTATUS_IND:nParam2: 0x%x", cfw_event->nParam2);

    g_CSCON_access = (uint8_t)(nParam2 & 0x000000ff);
    g_CSCON_state = (uint8_t)((nParam2 >> 8) & 0x000000ff);

    OSI_LOGI(0, "EV_CFW_ERRC_CONNSTATUS_IND:access: %d,state: %d", g_CSCON_access, g_CSCON_state);
    if (mode == gAtCfwCtx.sim[nSim].cscon_mode)
        return;

    gAtCfwCtx.sim[nSim].cscon_mode = mode;
    if (gAtSetting.sim[nSim].cscon == 0)
        return;

    char rsp[64];
    if (gAtSetting.sim[nSim].cscon == 2 && mode == 1)
        sprintf(rsp, "+CSCON: %d,%d", mode, g_CSCON_state);
    else if (gAtSetting.sim[nSim].cscon == 3 && mode == 1)
        sprintf(rsp, "+CSCON: %d,%d,%d", mode, g_CSCON_state, g_CSCON_access);
    else
        sprintf(rsp, "+CSCON: %d", mode);
    atCmdRespSimUrcText(nSim, rsp);
}

static void _onEV_CFW_GPRS_EDRX_IND(const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    uint8_t nSim = cfw_event->nFlag;

    uint32_t edrx_value = cfw_event->nParam1;
    uint32_t edrx_ptw = cfw_event->nParam2;
    uint8_t edrx_enable = 2; // nbiot_nvGetEdrxEnable()
    if ((edrx_value != gAtCfwCtx.sim[nSim].edrx_value ||
         edrx_ptw != gAtCfwCtx.sim[nSim].edrx_ptw) &&
        edrx_enable == 2)
    {
        uint8_t requested_edrx = 0; // nbiot_nvGetEdrxValue()
        gAtCfwCtx.sim[nSim].edrx_value = edrx_value;
        gAtCfwCtx.sim[nSim].edrx_ptw = edrx_ptw;

        char rsp[32];
        sprintf(rsp, "+CEDRXP: 5, %d, %d, %d", requested_edrx,
                (int)edrx_value, (int)edrx_ptw);
        atCmdRespSimUrcText(nSim, rsp);
    }
}

static void _onEV_CFW_GPRS_CCIOTOPT_IND(const osiEvent_t *event)
{
#ifdef PORTING_ON_GOING
    OSI_LOGI(0x100048ce, "We got EV_CFW_GPRS_CCIOTOPT_IND");
    uint8_t nResp[25] = {
        0x00,
    };
    if (nbiot_nvGetCiotReport() == 1)
    {
        sprintf(nResp, "+CCIOTOPTI:  %d", cfw_event->nParam1);
        atCmdRespUrcText(engine, nResp);
    }
#endif
}

static void _onEV_CFW_NW_CELL_LIST_INFO_IND(const osiEvent_t *event)
{
#ifdef PORTING_ON_GOING
    CFW_ALL_CELL_INFO *pAllCellData = NULL;
    CFW_TSM_NEIGHBOR_CELL_INFO *pCell = NULL;
    uint8_t outstr[40] = {
        0,
    };
    uint8_t outemp[240] = {
        0,
    };
    uint16_t i = 0;
    uint32_t nRet = 0;

    if (0 == cfw_event->nType)
    {
        uint8_t *pOperName = NULL;
        uint8_t *pOperShortName = NULL;
        uint8_t nPlmn[6];
        uint8_t nMcc[3];
        uint8_t nMnc[3];
        uint8_t nMncLen = 3;
        uint8_t nLac[2];
        uint8_t nOpretorName[20];
        uint8_t uFormat = 0;

        pAllCellData = (CFW_ALL_CELL_INFO *)(cfw_event->nParam1);
        pCell = (CFW_TSM_NEIGHBOR_CELL_INFO *)pAllCellData->pCellInfos;
        SUL_ZeroMemory8(outstr, 40);
        SUL_StrCat(outstr, "  ");
        AT_NW_Result_OK(CMD_FUNC_CONTINUE, CMD_RC_OK, 0, outstr, AT_StrLen(outstr), cfw_event->nUTI, nSim);
        for (i = 0; i < pAllCellData->nAllCellNUM; i++)
        {
            SUL_ZeroMemory8(outemp, 240);
            SUL_ZeroMemory8(nPlmn, 6);
            SUL_ZeroMemory8(nMcc, 3);
            SUL_ZeroMemory8(nMnc, 3);
            SUL_ZeroMemory8(nLac, 2);

            SUL_ZeroMemory8(nOpretorName, 20);
            //get PLMN from LAI
            PlmnApi2Array(pCell[i].nTSM_LAI, nPlmn);
            //get MCC MNC
            SUL_MemCopy8(nMcc, nPlmn, 3);
            SUL_MemCopy8(nMnc, &nPlmn[3], 3);
            if (0xf == nMnc[2])
                nMncLen = 2;
            //get Lac
            SUL_MemCopy8(nLac, &(pCell[i].nTSM_LAI[3]), 2);
            //get opretor name

            OSI_LOGI(0x100048cf, "Get EV_CFW_NW_CELL_LIST_INFO_IND plmn:%d%d%d%d%d%d \r\n", nPlmn[0], nPlmn[1], nPlmn[2], nPlmn[3], nPlmn[4], nPlmn[5]);

            nRet = CFW_CfgNwGetOperatorNameEx(nPlmn, &pOperName, &pOperShortName);
            if (nRet != 0)
            {
                if (pAllCellData != NULL)
                {
                    AT_FREE(pAllCellData->pCellInfos);
                    pAllCellData->pCellInfos = NULL;
                }
                AT_NW_Result_Err(ERR_AT_CME_EXT_REG_NOT_EXIT, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
                return;
            }
            //get set format
            nRet = CFW_CfgNwGetAllCellF(&uFormat);
            if (nRet != 0)
            {
                OSI_LOGI(0x100048d0, "AT+CNETSCAN READ ERROR:nRet: %d", nRet);
                if (pAllCellData != NULL)
                {
                    AT_FREE(pAllCellData->pCellInfos);
                    pAllCellData->pCellInfos = NULL;
                }
                AT_NW_Result_Err(ERR_AT_CME_PARAM_INVALID, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
                return;
            }
            if (0 == uFormat)
            {
                if (3 == nMncLen)
                {
                    SUL_StrPrint(outemp, "\r\nOperator:\"%s\",MCC:%x%x%x,MNC:%x%x%x,Rxlev:%d,Cellid:%d%d,Arfcn:%d", pOperName, nMcc[0], nMcc[1], nMcc[2], nMnc[0], nMnc[1], nMnc[2], pCell[i].nTSM_RxLevAM, pCell[i].nTSM_CellID[0], pCell[i].nTSM_CellID[1], pCell[i].nTSM_Arfcn);
                    AT_NW_Result_OK(CMD_FUNC_CONTINUE, CMD_RC_OK, 0, outemp, AT_StrLen(outemp), cfw_event->nUTI, nSim);
                }
                else if (2 == nMncLen)
                {
                    SUL_StrPrint(outemp, "\r\nOperator:\"%s\",MCC:%x%x%x,MNC:%x%x,Rxlev:%d,Cellid:%d%d,Arfcn:%d", pOperName, nMcc[0], nMcc[1], nMcc[2], nMnc[0], nMnc[1], pCell[i].nTSM_RxLevAM, pCell[i].nTSM_CellID[0], pCell[i].nTSM_CellID[1], pCell[i].nTSM_Arfcn);
                    AT_NW_Result_OK(CMD_FUNC_CONTINUE, CMD_RC_OK, 0, outemp, AT_StrLen(outemp), cfw_event->nUTI, nSim);
                }
                else
                {
                    if (pAllCellData != NULL)
                    {
                        AT_FREE(pAllCellData->pCellInfos);
                        pAllCellData->pCellInfos = NULL;
                    }
                    OSI_LOGI(0x100048d1, "AT+CNETSCAN READ ERROR:nMncLen: %d", nMncLen);
                    AT_NW_Result_Err(ERR_AT_CME_OPTION_NOT_SURPORT, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
                    return;
                }
            }
            else if (1 == uFormat)
            {
                if (3 == nMncLen)
                {
                    SUL_StrPrint(outemp, "\r\nOperator:\"%s\",MCC:%x%x%x,MNC:%x%x%x,Rxlev:%d,Cellid:%d%d,Arfcn:%d,Lac:%d%d,Bsic:%d", pOperName, nMcc[0], nMcc[1], nMcc[2], nMnc[0], nMnc[1], nMnc[2], pCell[i].nTSM_RxLevAM, pCell[i].nTSM_CellID[0], pCell[i].nTSM_CellID[1], pCell[i].nTSM_Arfcn, nLac[0], nLac[1], pCell[i].nTSM_Bsic);
                    AT_NW_Result_OK(CMD_FUNC_CONTINUE, CMD_RC_OK, 0, outemp, AT_StrLen(outemp), cfw_event->nUTI, nSim);
                }
                else if (2 == nMncLen)
                {
                    SUL_StrPrint(outemp, "\r\nOperator:\"%s\",MCC:%x%x%x,MNC:%x%x,Rxlev:%d,Cellid:%d%d,Arfcn:%d,Lac:%d%d,Bsic:%d", pOperName, nMcc[0], nMcc[1], nMcc[2], nMnc[0], nMnc[1], pCell[i].nTSM_RxLevAM, pCell[i].nTSM_CellID[0], pCell[i].nTSM_CellID[1], pCell[i].nTSM_Arfcn, nLac[0], nLac[1], pCell[i].nTSM_Bsic);
                    AT_NW_Result_OK(CMD_FUNC_CONTINUE, CMD_RC_OK, 0, outemp, AT_StrLen(outemp), cfw_event->nUTI, nSim);
                }
                else
                {
                    if (pAllCellData != NULL)
                    {
                        AT_FREE(pAllCellData->pCellInfos);
                        pAllCellData->pCellInfos = NULL;
                    }
                    OSI_LOGI(0x100048d1, "AT+CNETSCAN READ ERROR:nMncLen: %d", nMncLen);
                    AT_NW_Result_Err(ERR_AT_CME_OPTION_NOT_SURPORT, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
                    return;
                }
            }
            else
            {
                if (pAllCellData != NULL)
                {
                    AT_FREE(pAllCellData->pCellInfos);
                    pAllCellData->pCellInfos = NULL;
                }
                OSI_LOGI(0x100048d2, "AT+CNETSCAN READ ERROR:uFormat: %d", uFormat);
                AT_NW_Result_Err(ERR_AT_CME_OPTION_NOT_SURPORT, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
                return;
            }
        }
        if (pAllCellData != NULL)
        {
            AT_FREE(pAllCellData->pCellInfos);
            pAllCellData->pCellInfos = NULL;
        }
        AT_NW_Result_OK(CMD_FUNC_SUCC, CMD_RC_OK, 0, NULL, 0, cfw_event->nUTI, nSim);
        return;
    }
    else
    {
        AT_NW_Result_Err(pEvent->nParam1 - ERR_CME_PHONE_FAILURE, CMD_ERROR_CODE_TYPE_CME, cfw_event->nUTI, nSim);
        cfw_event->nParam1 = 0;
        return;
    }
#endif
}

typedef struct
{
    bool try_auto_at_fail;
} copsAsyncCtx_t;

static void _copsSetRegRspTimeOutCB(atCommand_t *cmd)
{
    cfwReleaseUTI(cmd->uti);
    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_SEND_TIMEOUT));
}
static void _copsGetAvailRspTimeOutCB(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint32_t res;

    if ((res = CFW_NwAbortListOperators(cmd->uti, nSim)) != 0) //abort list req
    {
        cfwReleaseUTI(cmd->uti);
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
    }
    RETURN_FOR_ASYNC();
}
static void _copsSetRegRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    uint8_t nSim = atCmdGetSim(cmd->engine);
    copsAsyncCtx_t *async = (copsAsyncCtx_t *)cmd->async_ctx;

    if (cfw_event->nType != 0 && async->try_auto_at_fail)
    {
        async->try_auto_at_fail = false;

        cmd->uti = cfwRequestUTI((osiEventCallback_t)_copsSetRegRsp, cmd);
        uint8_t oper_id[6] = {};
        CFW_NwSetRegistration(oper_id, 0, NW_SET_AUTO_RAT, cmd->uti, nSim);
        RETURN_FOR_ASYNC();
    }
    if (cfw_event->nType == 0)
        RETURN_OK(cmd->engine);

    RETURN_CME_CFW_ERR(cmd->engine, cfw_event->nParam1);
}

static void _copsDeregRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    uint8_t nSim = atCmdGetSim(cmd->engine);

    if (0 == cfw_event->nParam1)
    {
        if ((CFW_NWGetStackRat(nSim) == 4 /*API_RAT_LTE*/)) /*4 is NBIOT or LTE*/
            atCmdRespInfoText(cmd->engine, "+CEREG: 0");
        else
            atCmdRespInfoText(cmd->engine, "+CREG: 0");
        atCmdRespOK(cmd->engine);
    }
    else
    {
        RETURN_CME_CFW_ERR(cmd->engine, cfw_event->nParam1);
    }
}

static void _copsGetAvailRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_NW_OPERATOR_INFO *pOperInfo = NULL;
    unsigned char *pOperName = NULL;
    unsigned char *pOpershortName = NULL;
    //unsigned char nOprNam[10] = {0,};

    char rsp[400];
    char *prsp = rsp;

    if (cfw_event->nEventId == EV_CFW_NW_ABORT_LIST_OPERATORS_RSP)
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NETWORK_TIMOUT);

    if (0 == cfw_event->nType)
    {
        pOperInfo = (CFW_NW_OPERATOR_INFO *)cfw_event->nParam1;
        prsp += sprintf(prsp, "+COPS: ");
        for (int i = 0; i < cfw_event->nParam2; i++)
        {

            int nRet = CFW_CfgNwGetOperatorName(pOperInfo[i].nOperatorId, &pOperName, &pOpershortName);
            if (i > 0)
            {
                prsp += sprintf(prsp, ",");
            }

            if (0x0f == pOperInfo[i].nOperatorId[5])
            {
                if (nRet != 0)
                {
                    prsp += sprintf(prsp, "(%d,\"%d%d%d%d%d\",\"%d%d%d%d%d", pOperInfo[i].nStatus,
                                    pOperInfo[i].nOperatorId[0], pOperInfo[i].nOperatorId[1], pOperInfo[i].nOperatorId[2],
                                    pOperInfo[i].nOperatorId[3], pOperInfo[i].nOperatorId[4],
                                    pOperInfo[i].nOperatorId[0], pOperInfo[i].nOperatorId[1], pOperInfo[i].nOperatorId[2],
                                    pOperInfo[i].nOperatorId[3], pOperInfo[i].nOperatorId[4]);
                }
                else
                {
                    prsp += sprintf(prsp, "(%d,\"%s\",\"%s", pOperInfo[i].nStatus, pOperName, pOpershortName);
                }
                prsp += sprintf(prsp, "\",\"%d%d%d%d%d", pOperInfo[i].nOperatorId[0], pOperInfo[i].nOperatorId[1], pOperInfo[i].nOperatorId[2],
                                pOperInfo[i].nOperatorId[3], pOperInfo[i].nOperatorId[4]);
            }
            else
            {
                if (nRet != 0)
                {
                    prsp += sprintf(prsp, "(%d,\"%d%d%d%d%d%d\",\"%d%d%d%d%d%d", pOperInfo[i].nStatus,
                                    pOperInfo[i].nOperatorId[0], pOperInfo[i].nOperatorId[1], pOperInfo[i].nOperatorId[2],
                                    pOperInfo[i].nOperatorId[3], pOperInfo[i].nOperatorId[4], pOperInfo[i].nOperatorId[5],
                                    pOperInfo[i].nOperatorId[0], pOperInfo[i].nOperatorId[1], pOperInfo[i].nOperatorId[2],
                                    pOperInfo[i].nOperatorId[3], pOperInfo[i].nOperatorId[4], pOperInfo[i].nOperatorId[5]);
                }
                else
                {
                    prsp += sprintf(prsp, "(%d,\"%s\",\"%s", pOperInfo[i].nStatus, pOperName, pOpershortName);
                }
                prsp += sprintf(prsp, "\",\"%d%d%d%d%d%d", pOperInfo[i].nOperatorId[0], pOperInfo[i].nOperatorId[1], pOperInfo[i].nOperatorId[2],
                                pOperInfo[i].nOperatorId[3], pOperInfo[i].nOperatorId[4], pOperInfo[i].nOperatorId[5]);
            }
#if defined(LTE_NBIOT_SUPPORT) || defined(LTE_SUPPORT)
            prsp += sprintf(prsp, "\",%d)", pOperInfo[i].nRat);
#else
            prsp += sprintf(prsp, "\")");
#endif
        }
        prsp += sprintf(prsp, ",,(0,1,2,3,4),(0,1,2)");
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
        return;
    }
    else
    {
        if (cfw_event->nParam1 == ERR_CME_OPERATION_TEMPORARILY_NOT_ALLOWED)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NETWORK_REFUSED);

        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

// 7.3 PLMN selection +COPS
void atCmdHandleCOPS(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint32_t res;

    if (cmd->type == AT_CMD_SET)
    {
        //This command should be abortable when registration/deregistration attempt is made

        // +COPS=[<mode>[,<format>[,<oper>[,<AcT>]]]]
        bool paramok = true;
        uint8_t mode = atParamUintInRange(cmd->params[0], COPS_MODE_AUTOMATIC, COPS_MODE_MANUAL_AUTOMATIC, &paramok);
        uint8_t format = atParamDefUintInRange(cmd->params[1], 0, 0, 2, &paramok);
        const char *oper = atParamDefStr(cmd->params[2], "", &paramok);
        uint8_t AcT = atParamDefUintInRange(cmd->params[3], NW_SET_AUTO_RAT, COPS_ACT_GSM, COPS_ACT_UNDEFINED, &paramok);
        if (!paramok || cmd->param_count > 4)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
#if defined(CONFIG_SOC_8910)
        if (AcT == 9)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
#endif
        gAtSetting.sim[nSim].cops_act = AcT;
        if ((mode == COPS_MODE_AUTOMATIC && cmd->param_count > 2) ||
            (mode == COPS_MODE_DEREGISTER && cmd->param_count > 1) ||
            (mode == COPS_MODE_SET_ONLY && cmd->param_count > 2))
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        uint8_t oper_id[6] = {};
        if (mode == COPS_MODE_MANUAL || mode == COPS_MODE_MANUAL_AUTOMATIC)
        {
            if (atParamIsEmpty(cmd->params[2]))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

            if (format == COPS_FORMAT_LONG || format == COPS_FORMAT_SHORT)
            {
                uint8_t *roper_id;
                if (CFW_CfgNwGetOperatorId(&roper_id, format, (uint8_t *)oper) != 0)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                memcpy(oper_id, roper_id, 6);
            }
            else
            {
                size_t oper_len = strlen(oper);
                oper_id[5] = 0xf;
                if (oper_len != 5 && oper_len != 6)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

                for (size_t n = 0; n < oper_len; n++)
                {
                    if (oper[n] >= '0' && oper[n] <= '9')
                        oper_id[n] = oper[n] - '0';
                    else
                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                }
            }
        }

        copsAsyncCtx_t *async = (copsAsyncCtx_t *)malloc(sizeof(*async));
        cmd->async_ctx = async;
        cmd->async_ctx_destroy = atCommandAsyncCtxFree;
        async->try_auto_at_fail = (mode == COPS_MODE_MANUAL_AUTOMATIC);

        if ((format != COPS_FORMAT_UNDEFINED) && (cmd->param_count > 1)) //&&(cmd->param_count > 1) modify for bug1208993
            gAtSetting.sim[nSim].cops_format = format;
        if (mode != COPS_MODE_UNDEFINED && mode != COPS_MODE_SET_ONLY)
            gAtSetting.sim[nSim].cops_mode = mode;

        if (mode == COPS_MODE_AUTOMATIC ||
            mode == COPS_MODE_MANUAL ||
            mode == COPS_MODE_MANUAL_AUTOMATIC)
        {
            cmd->uti = cfwRequestUTI((osiEventCallback_t)_copsSetRegRsp, cmd);
            atCmdSetTimeoutHandler(cmd->engine, 1000 * 60 * 2, _copsSetRegRspTimeOutCB);
            if ((res = CFW_NwSetRegistration(oper_id, mode, AcT, cmd->uti, nSim)) != 0)
            {
                cfwReleaseUTI(cmd->uti);
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            }
            RETURN_FOR_ASYNC();
        }

        if (mode == COPS_MODE_DEREGISTER)
        {
            uint8_t nCid;
            cmd->uti = cfwRequestUTI((osiEventCallback_t)_copsDeregRsp, cmd);
            if (0 != CFW_NwDeRegister(cmd->uti, nSim))
            {
                cfwReleaseUTI(cmd->uti);
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            }
            for (nCid = AT_PDPCID_MIN; nCid <= AT_PDPCID_MAX; nCid++)
            {
                OSI_LOGI(0, "atCmdHandleCOPS nSim = %d nCid = %d", nSim, nCid);
                pppSessionDeleteByNetifDestoryed(nSim, nCid);

                //TCPIP_netif_destory(nCid, nSim);
                osiEvent_t tcpip_event;
                memset(&tcpip_event, 0, sizeof(osiEvent_t));
                tcpip_event.id = EV_TCPIP_CFW_GPRS_DEACT;
                tcpip_event.param1 = nCid;
                tcpip_event.param3 = nSim;
                OSI_LOGI(0x0, "atCmdHandleCOPS Send EV_TCPIP_CFW_GPRS_DEACT to NetThread!!!!");
                osiEventSend(netGetTaskID(), (const osiEvent_t *)&tcpip_event);
                OSI_LOGI(0, "atCmdHandleCOPS loop nSim = %d nCid = %d", nSim, nCid);
            }
            CFW_CfgNwSetFM(CFW_DISABLE_COMM, nSim);
            RETURN_FOR_ASYNC();
        }

        if (mode == COPS_MODE_SET_ONLY)
            RETURN_OK(cmd->engine);

        // Why comes here?
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        CFW_NW_STATUS_INFO nw_info;
        CFW_COMM_MODE nFM = CFW_ENABLE_COMM;
        if (CFW_NwGetStatus(&nw_info, nSim) != 0)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
        if (ERR_SUCCESS != CFW_GetComm(&nFM, nSim))
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
        }
        if (CFW_DISABLE_COMM == nFM)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

        cmd->uti = cfwRequestUTI((osiEventCallback_t)_copsGetAvailRsp, cmd);
        atCmdSetTimeoutHandler(cmd->engine, 1000 * 60 * 3, _copsGetAvailRspTimeOutCB);
        if ((res = CFW_NwGetAvailableOperators(cmd->uti, nSim)) != 0)
        {
            cfwReleaseUTI(cmd->uti);
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }
        RETURN_FOR_ASYNC();
    }
    else if (cmd->type == AT_CMD_READ)
    {
        CFW_NW_STATUS_INFO nw_info;
        char rsp[64];

#if defined(LTE_NBIOT_SUPPORT) || defined(LTE_SUPPORT)
        if (CFW_NwGetStatus(&nw_info, nSim) != 0)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

        if (nw_info.nStatus == CFW_NW_STATUS_NOTREGISTERED_NOTSEARCHING ||
            nw_info.nStatus == CFW_NW_STATUS_NOTREGISTERED_SEARCHING ||
            nw_info.nStatus == CFW_NW_STATUS_REGISTRATION_DENIED || nw_info.nStatus == CFW_NW_STATUS_UNKNOW)
        {
            if (ERR_SUCCESS != CFW_GprsGetstatus(&nw_info, nSim))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
        }
#else
        if (CFW_NwGetStatus(&nw_info, nSim) != 0)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
#endif

        if (nw_info.nStatus == CFW_NW_STATUS_NOTREGISTERED_NOTSEARCHING ||
            nw_info.nStatus == CFW_NW_STATUS_NOTREGISTERED_SEARCHING ||
            nw_info.nStatus == CFW_NW_STATUS_REGISTRATION_DENIED || nw_info.nStatus == CFW_NW_STATUS_UNKNOW)
        {
            sprintf(rsp, "+COPS: %d", gAtSetting.sim[nSim].cops_mode);
            atCmdRespInfoText(cmd->engine, rsp);
            RETURN_OK(cmd->engine);
        }

        uint8_t oper_id[6];
        uint8_t mode;
        if (CFW_NwGetCurrentOperator(oper_id, &mode, nSim) != 0)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

        unsigned char *oper_name = NULL;
        unsigned char *oper_short_name = NULL;
        uint8_t format = gAtSetting.sim[nSim].cops_format;
        if (COPS_MODE_SET_ONLY == gAtSetting.sim[nSim].cops_mode)
            mode = gAtSetting.sim[nSim].cops_mode;
        if ((COPS_FORMAT_LONG == format) || (COPS_FORMAT_SHORT == format))
        {
            if (CFW_CfgNwGetOperatorName(oper_id, &oper_name, &oper_short_name) != 0 ||
                oper_name == NULL || oper_short_name == NULL)
                /* not found operator name, use numberic*/
                format = COPS_FORMAT_NUMERIC;
        }

        char *prsp = rsp;
        prsp += sprintf(prsp, "+COPS: %d,%d,\"", mode, format);
        if (format == COPS_FORMAT_NUMERIC)
        {
            prsp += _printOperID(prsp, oper_id);
        }

        else if (format == COPS_FORMAT_LONG)
        {
            prsp += sprintf(prsp, "%s", oper_name);
        }
        else
        {
            prsp += sprintf(prsp, "%s", oper_short_name);
        }
        prsp += sprintf(prsp, "\"");
        uint8_t Rat = CFW_NWGetStackRat(nSim);
        if (Rat == 2 && gAtSetting.sim[nSim].cops_act == 7) //cops_act default is 7
            gAtSetting.sim[nSim].cops_act = 0;
        else
            gAtSetting.sim[nSim].cops_act = Mapping_Creg_From_PsType(Rat);
        prsp += sprintf(prsp, ",%d", gAtSetting.sim[nSim].cops_act);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

// 7.21 Read operator names +COPN
void atCmdHandleCOPN(atCommand_t *cmd)
{
    if (cmd->type == AT_CMD_EXE)
    {
        uint8_t *oper_id;
        uint8_t *oper_name;
        char rsp[64];

        for (uint32_t index = 1;; index++)
        {
            uint32_t res = CFW_CfgNwGetOperatorInfo(&oper_id, &oper_name, index);

            if (res == ERR_CFW_NOT_EXIST_OPERATOR_ID)
                RETURN_OK(cmd->engine);

            if (res != 0)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

            char *prsp = rsp;
            prsp += sprintf(prsp, "+COPN: \"");
            prsp += _printOperID(prsp, oper_id);
            prsp += sprintf(prsp, "\"");
            prsp += sprintf(prsp, ",\"%s\"", oper_name);
            atCmdRespInfoText(cmd->engine, rsp);
        }
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

bool AT_CIND_NetWork(uint8_t *pSignalQ, uint8_t *pRegistS, uint8_t *pRoamS, uint8_t nDLCI, uint8_t nSim)
{
#ifdef PORTING_ON_GOING
    uint8_t nSignalLevel = 0;
    uint8_t nBitError = 0;
    CFW_NW_STATUS_INFO nStatusInfo;
    uint32_t nRet;
    nRet = CFW_NwGetStatus(&nStatusInfo, nSim);

    if (0 != nRet)
    {
        AT_NW_Result_Err(ERR_AT_CME_PARAM_INVALID, CMD_ERROR_CODE_TYPE_CME, nDLCI, nSim);
        return false;
    }

    if (nStatusInfo.nStatus == CFW_NW_STATUS_REGISTERED_ROAMING)
    {
        *pRoamS = 1;
        *pRegistS = 1;
    }
    else if (nStatusInfo.nStatus == CFW_NW_STATUS_REGISTERED_HOME)
    {
        *pRoamS = 0;
        *pRegistS = 1;
    }
    else
    {
        *pRoamS = 0;
        *pRegistS = 0;
    }
    nRet = CFW_NwGetSignalQuality(&nSignalLevel, &nBitError, nSim);
    if (0 != nRet)
    {
        AT_NW_Result_Err(ERR_AT_CME_PARAM_INVALID, CMD_ERROR_CODE_TYPE_CME, nDLCI, nSim);
        return false;
    }

    if (nSignalLevel == 0)
        *pSignalQ = 0;
    else if (1 <= nSignalLevel && 7 > nSignalLevel)
        *pSignalQ = 1;
    else if (7 <= nSignalLevel && 13 > nSignalLevel)
        *pSignalQ = 2;
    else if (13 <= nSignalLevel && 19 > nSignalLevel)
        *pSignalQ = 3;
    else if (19 <= nSignalLevel && 25 > nSignalLevel)
        *pSignalQ = 4;
    else if (25 <= nSignalLevel && 31 >= nSignalLevel)
        *pSignalQ = 5;
#endif
    return true;
}

static void _cpolReadListRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    uint32_t res;
    uint8_t nSim = cfw_event->nFlag;
    uint8_t *data = (uint8_t *)(cfw_event->nParam1);
    uint16_t count = cfw_event->nParam2;

    uint8_t *oper_name;
    uint8_t *oper_short_name;
    uint8_t nOperatorId[6] = {0};
    if (0 != cfw_event->nType)
        RETURN_CME_CFW_ERR(cmd->engine, cfw_event->nParam1);
    OSI_SXDUMP(0, "PLMN LIST:", data, count);
    atMemFreeLater(data);
    uint8_t step = CFW_GetSimType(nSim) ? 5 : 3;
    OSI_LOGI(0, "step  = %d", step);
    OSI_LOGI(0, "data  = %x", data);
    OSI_LOGI(0, "count  = %d", count);

    for (uint16_t n = 0; n < count; n += step)
    {
        if ((data[n] != 0xFF) && (data[n + 1] != 0xFF) && (data[n + 2] != 0xFF))
        {
            char rsp[100] = {0};
            uint8_t index = n / step + 1;
            nOperatorId[0] = data[n] & 0x0F;
            nOperatorId[1] = data[n] >> 4;
            nOperatorId[2] = data[n + 1] & 0x0F;
            nOperatorId[3] = data[n + 2] & 0x0F;
            nOperatorId[4] = data[n + 2] >> 4;
            uint16_t offset = 0;
            OSI_LOGI(0, "index = %x", index);
            if ((data[n + 1] & 0xF0) == 0xF0)
            {
                nOperatorId[5] = 0x0F;
                res = CFW_CfgNwGetOperatorName(nOperatorId, &oper_name, &oper_short_name);
                if ((gAtSetting.sim[nSim].cpol_format == 2) || (res == ERR_CFW_NOT_EXIST_OPERATOR_ID))
                {
                    offset = sprintf(rsp, "+CPOL: %u,%u,\"%u%u%u%u%u\"", index, gAtSetting.sim[nSim].cpol_format,
                                     nOperatorId[0], nOperatorId[1], nOperatorId[2],
                                     nOperatorId[3], nOperatorId[4]);
                }
            }
            else
            {
                nOperatorId[5] = data[n + 1] >> 4;
                res = CFW_CfgNwGetOperatorName(nOperatorId, &oper_name, &oper_short_name);
                if ((gAtSetting.sim[nSim].cpol_format == 2) || (res == ERR_CFW_NOT_EXIST_OPERATOR_ID))
                {
                    offset = sprintf(rsp, "+CPOL: %u,%u,\"%u%u%u%u%u%u\"", index, gAtSetting.sim[nSim].cpol_format,
                                     nOperatorId[0], nOperatorId[1], nOperatorId[2],
                                     nOperatorId[3], nOperatorId[4], nOperatorId[5]);
                }
            }

            if ((gAtSetting.sim[nSim].cpol_format == 0) && (res == ERR_SUCCESS))
            {
                offset = sprintf(rsp, "+CPOL: %u,%u,\"%s\"", index, gAtSetting.sim[nSim].cpol_format, oper_name);
            }
            if ((gAtSetting.sim[nSim].cpol_format == 1) && (res == ERR_SUCCESS))
            {
                offset = sprintf(rsp, "+CPOL: %u,%u,\"%s\"", index, gAtSetting.sim[nSim].cpol_format, oper_short_name);
            }
            if (step == 5)
            {
                uint8_t gsm_act = data[n + 4] >> 7;
                uint8_t gsm_compact_act = (data[n + 4] >> 6) & 0x01;
                uint8_t utran_act = data[n + 3] >> 7;
                uint8_t eutran_act = (data[n + 3] >> 6) & 0x01;
                sprintf(rsp + offset, ",%d,%d,%d,%d\r\n", gsm_act, gsm_compact_act, utran_act, eutran_act);
            }
            OSI_LOGI(0, "offset  = %d", offset);
            OSI_SXDUMP(0, "rsp:", rsp, offset + 10);
            atCmdRespInfoText(cmd->engine, rsp);
        }
    }

    atCmdRespOK(cmd->engine);
}

static void _cpolGetNumRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;

    if (cfw_event->nType == 0)
    {
        char rsp[32];
        sprintf(rsp, "+CPOL: (1-%u),(0,1,2)", (int)cfw_event->nParam1);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        RETURN_CME_CFW_ERR(cmd->engine, cfw_event->nParam1);
    }
}

static OSI_UNUSED void _cpolAddDelRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    uint8_t nSim = atCmdGetSim(cmd->engine);

    if (cfw_event->nType == 0)
    {
        gAtSetting.sim[nSim].cpol_format = nFormat;
        atCmdRespOK(cmd->engine);
    }
    else
        RETURN_CME_CFW_ERR(cmd->engine, cfw_event->nParam1);
}

// 7.19 Preferred PLMN list +CPOL
void atCmdHandleCPOL(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint32_t res;

    if (cmd->type == AT_CMD_SET)
    {
        OSI_LOGXI(OSI_LOGPAR_S, 0x00000000, "CPOL Set in");
        bool paramok = true;
        uint8_t length = 1;
        uint8_t *operator= NULL;
        uint8_t nSize;
        uint16_t nFrmt = 0;
        uint8_t gsm_act = 0;
        uint8_t gsm_compact_act = 0;
        uint8_t utran_act = 0;
        uint8_t eutran_act = 0;
        if (cmd->param_count > 7 || cmd->param_count == 0)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        bool index_present = !atParamIsEmpty(cmd->params[0]);
        bool format_present = !atParamIsEmpty(cmd->params[1]);

        //at+cpol=,0/1/2
        if (!index_present && format_present && cmd->param_count == 2)
        {
            gAtSetting.sim[nSim].cpol_format = atParamDefUintInRange(cmd->params[1], gAtSetting.sim[nSim].cpol_format, 0, 2, &paramok);
            if (!paramok)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            else
                RETURN_OK(cmd->engine);
        }

        uint8_t index = atParamDefUintInRange(cmd->params[0], gAtSetting.sim[nSim].cpol_index, 1, 0xff, &paramok);
        if (!paramok || index == 0)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }

        uint8_t PLMN[10] = {0};

        if (cmd->param_count > 1)
        {
            length = 1;
            nFrmt = atParamDefUintInRange(cmd->params[1], gAtSetting.sim[nSim].cpol_format, 0, 2, &paramok);
            if (!paramok || nFrmt > 2)
            {
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            }

            if (cmd->param_count > 2)
            {
                length = sizeof(PLMN);
                const char *oper = atParamDefStr(cmd->params[2], "", &paramok);

                length = strlen(oper);
                if (((length != 5) && (length != 6)) || (nFrmt != 2))
                {
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                }

                for (int i = 0; i < length; i++)
                {
                    PLMN[i] = *oper;
                    oper++;
                }
            }
            if (cmd->param_count > 3)
            {
                gsm_act = atParamUintInRange(cmd->params[3], 0, 1, &paramok);
                if (paramok != true)
                {
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                }
                gsm_compact_act = atParamUintInRange(cmd->params[4], 0, 1, &paramok);
                if (paramok != true)
                {
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                }
                utran_act = atParamUintInRange(cmd->params[5], 0, 1, &paramok);
                if (paramok != true)
                {
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                }
                eutran_act = atParamUintInRange(cmd->params[6], 0, 1, &paramok);
                if (paramok != true)
                {
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                }
                PLMN[length] = ((utran_act << 3) | (eutran_act << 2) | (gsm_act << 1) | gsm_compact_act);
                OSI_LOGI(0, "eutran_act = %d, utran_act = %d, gsm_act =%d, gsm_compact_act= %d", eutran_act, utran_act, gsm_act, gsm_compact_act);
                OSI_LOGI(0, "length = %d, PLMN[length] = %d", length, PLMN[length]);
            }
            else
            {
                PLMN[length] = 0;
            }
            length++;
            nFormat = nFrmt;
        }

        nSize = length;
        operator= PLMN;
        cmd->uti = cfwRequestUTI((osiEventCallback_t)_cpolAddDelRsp, cmd);
        if ((res = CFW_SimWritePreferPLMN(index, operator, nSize, cmd->uti, nSim)) != 0)
        {
            cfwReleaseUTI(cmd->uti);
            RETURN_CME_CFW_ERR(cmd->engine, res);
        }
        RETURN_FOR_ASYNC();
    }
    else if (cmd->type == AT_CMD_READ)
    {
        cmd->uti = cfwRequestUTI((osiEventCallback_t)_cpolReadListRsp, cmd);
        if ((res = CFW_SimReadPreferPLMN(cmd->uti, nSim)) != 0)
        {
            cfwReleaseUTI(cmd->uti);
            RETURN_CME_CFW_ERR(cmd->engine, res);
        }
        RETURN_FOR_ASYNC();
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        cmd->uti = cfwRequestUTI((osiEventCallback_t)_cpolGetNumRsp, cmd);
        if ((res = CFW_SimGetPrefOperatorListMaxNum(cmd->uti, nSim)) != 0)
        {
            cfwReleaseUTI(cmd->uti);
            RETURN_CME_CFW_ERR(cmd->engine, res);
        }
        RETURN_FOR_ASYNC();
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

// 7.20 Selection of preferred PLMN list
void atCmdHandleCPLS(atCommand_t *cmd)
{
    if (AT_CMD_SET == cmd->type)
    {
        // +CPLS=[<list>]
        bool paramok = true;
        uint8_t nlist = atParamDefUintInRange(cmd->params[0], 0, 0, 2, &paramok);
        if (!paramok || cmd->param_count > 1)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        // TODO: what API should be called?
        (void)nlist;
        atCmdRespOK(cmd->engine);
    }
    else if (AT_CMD_READ == cmd->type)
    {
        // TODO: what API should be called?
        uint8_t nlist = 0;

        char rsp[64];
        sprintf(rsp, "+CPLS: %d", nlist);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else if (AT_CMD_TEST == cmd->type)
    {
        atCmdRespInfoText(cmd->engine, "+CPLS: (0-2)");
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
    }
}

#if defined(LTE_NBIOT_SUPPORT) || defined(LTE_SUPPORT)
void atCmdHandleCSCON(atCommand_t *cmd)
{
    // "CDMA-IS95B  5","LTE 14"
    uint8_t nSim = atCmdGetSim(cmd->engine);

    if (AT_CMD_SET == cmd->type) // Set command.
    {
        // +CSCON=[<n>]
        bool paramok = true;
        uint8_t n = atParamUintInRange(cmd->params[0], 0, 3, &paramok);
        if (!paramok || cmd->param_count > 1)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        gAtSetting.sim[nSim].cscon = n;
        CFW_NW_STATUS_INFO nw_status;
        if (CFW_GprsGetstatus(&nw_status, nSim) != 0)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

        // WHAT is the log about state
        char rsp[20];
        if (gAtSetting.sim[nSim].cscon == 2 && nw_status.ConnMode == 1)
            sprintf(rsp, "+CSCON: %d,%d,%d", gAtSetting.sim[nSim].cscon,
                    nw_status.ConnMode, g_CSCON_state);
        else if (gAtSetting.sim[nSim].cscon == 3 && nw_status.ConnMode == 1)
            sprintf(rsp, "+CSCON: %d,%d,%d,%d", gAtSetting.sim[nSim].cscon,
                    nw_status.ConnMode, g_CSCON_state, g_CSCON_access);
        else
            sprintf(rsp, "+CSCON: %d,%d", gAtSetting.sim[nSim].cscon,
                    nw_status.ConnMode);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else if (AT_CMD_READ == cmd->type)
    {
        CFW_NW_STATUS_INFO nw_status;
        if (CFW_GprsGetstatus(&nw_status, nSim) != 0)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

        // WHAT is the log about state
        char rsp[64];
        if (gAtSetting.sim[nSim].cscon == 2 && nw_status.ConnMode == 1)
            sprintf(rsp, "+CSCON: %d,%d,%d", gAtSetting.sim[nSim].cscon,
                    nw_status.ConnMode, g_CSCON_state);
        else if (gAtSetting.sim[nSim].cscon == 3 && nw_status.ConnMode == 1)
            sprintf(rsp, "+CSCON: %d,%d,%d,%d", gAtSetting.sim[nSim].cscon,
                    nw_status.ConnMode, g_CSCON_state, g_CSCON_access);
        else
            sprintf(rsp, "+CSCON: %d,%d", gAtSetting.sim[nSim].cscon,
                    nw_status.ConnMode);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else if (AT_CMD_TEST == cmd->type)
    {
        atCmdRespInfoText(cmd->engine, "+CSCON: (0-3)");
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_NOT_SURPORT);
    }
}
#endif

//frank add end
void AT_NW_CmdFunc_CNETSCAN(atCommand_t *cmd)
{
    atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
#ifdef PORTING_ON_GOING
    INT32 iResult = 0;
    uint8_t uParaCount = 0;
    uint8_t uIndex = 0;
    uint16_t uSize = 0;
    uint8_t uFormat = 0;
    uint8_t uOutStr[20] = {
        0,
    };
    uint32_t ret = 0;
    uint8_t nSim = AT_SIM_ID(pParam->nDLCI);

    OSI_LOGI(0x10004965, "CNETSCAN, start!\n");

#ifdef LTE_NBIOT_SUPPORT
    if (CFW_RAT_NBIOT_ONLY == CFW_NWGetRat(nSim))
    {
        AT_NW_Result_Err(ERR_AT_CME_OPERATION_NOT_SUPPORTED, CMD_ERROR_CODE_TYPE_CME, pParam->nDLCI, nSim);
        return;
    }
#endif

    switch (cmd->type)
    {
    case AT_CMD_SET:
        iResult = AT_Util_GetParaCount(pParam->pPara, &uParaCount);
        if ((iResult != 0) || (uParaCount != 1))
        {
            OSI_LOGI(0x10004966, "CNETSCAN, GetParaCount error!\n");
            AT_NW_Result_Err(ERR_AT_CME_PARAM_INVALID, CMD_ERROR_CODE_TYPE_CME, pParam->nDLCI, nSim);
            return;
        }

        uIndex = 0;
        uSize = SIZEOF(uFormat);
        iResult = AT_Util_GetParaWithRule(pParam->pPara, uIndex, AT_UTIL_PARA_TYPE_uint8_t, &uFormat, &uSize);
        if (iResult != 0)
        {
            OSI_LOGI(0x10004967, "CNETSCAN, get param1 error!\n");
            AT_NW_Result_Err(ERR_AT_CME_PARAM_INVALID, CMD_ERROR_CODE_TYPE_CME, pParam->nDLCI, nSim);
            return;
        }
        ret = CFW_CfgNwSetAllCellF(uFormat);
        if (ret != 0)
        {
            OSI_LOGI(0x10004968, "AT+CNETSCAN ERROR:ret: %d", ret);
            AT_NW_Result_Err(ERR_AT_CME_PARAM_INVALID, CMD_ERROR_CODE_TYPE_CME, pParam->nDLCI, nSim);
            return;
        }
        AT_NW_Result_OK(CMD_FUNC_SUCC, CMD_RC_OK, 0, NULL, 0, pParam->nDLCI, nSim);
        break;

    case AT_CMD_EXE:
        ret = CFW_GetAllCellInfo(pParam->nDLCI, nSim);
        if (ret != 0)
        {
            OSI_LOGI(0x10004968, "AT+CNETSCAN ERROR:ret: %d", ret);
            AT_NW_Result_Err(ERR_AT_CME_PARAM_INVALID, CMD_ERROR_CODE_TYPE_CME, pParam->nDLCI, nSim);
            return;
        }
        AT_NW_Result_OK(CMD_FUNC_SUCC_ASYN, CMD_RC_OK, 0, NULL, 0, pParam->nDLCI, nSim);
        break;

    case AT_CMD_READ:
        OSI_LOGI(0x10004969, "CNETSCAN,This is the READ!\n");
        //maybe we need get the value from cfg
        ret = CFW_CfgNwGetAllCellF(&uFormat);
        if (ret != 0)
        {
            OSI_LOGI(0x1000496a, "AT+CNETSCAN READ ERROR:ret: %d", ret);
            AT_NW_Result_Err(ERR_AT_CME_PARAM_INVALID, CMD_ERROR_CODE_TYPE_CME, pParam->nDLCI, nSim);
            return;
        }
        sprintf(uOutStr, "+CNETSCAN:%d", uFormat);
        AT_NW_Result_OK(CMD_FUNC_SUCC, CMD_RC_OK, 0, uOutStr, AT_StrLen(uOutStr), pParam->nDLCI, nSim);
        break;

    case AT_CMD_TEST:
        OSI_LOGI(0x1000496b, "CNETSCAN,This is the TEST!\n");
        sprintf(uOutStr, "+CNETSCAN:(0,l)");
        AT_NW_Result_OK(CMD_FUNC_SUCC, CMD_RC_OK, 0, uOutStr, AT_StrLen(uOutStr), pParam->nDLCI, nSim);
        break;
    default:
        AT_NW_Result_Err(ERR_AT_CME_OPERATION_NOT_ALLOWED, CMD_ERROR_CODE_TYPE_CME, pParam->nDLCI, nSim);
        break;
    }
    return;
#endif
}

uint8_t Mapping_Creg_From_PsType(uint8_t pstype)
{
    switch (pstype)
    {
    case 4:
    case 5:
    case 6:
        return 7;
    case 1:
        return 0;
    case 2:
        return 0;
    default:
        return 0;
    }
}

void atCmdHandleCREG(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);

    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint8_t status = atParamUintInRange(cmd->params[0], 0, 2, &paramok);
        if (!paramok || cmd->param_count > 1)
        {
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            return;
        }
        gAtSetting.sim[nSim].creg = status;
        CFW_CfgSetNwStatus(status, nSim);
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        atCmdRespInfoText(cmd->engine, "+CREG: (0,1,2)");
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ)
    {
        CFW_NW_STATUS_INFO nStatusInfo;
        uint8_t status;
        uint8_t nCurrRat = CFW_NWGetStackRat(nSim);

        if (CFW_CfgGetNwStatus(&status, nSim) != 0 ||
            CFW_NwGetStatus(&nStatusInfo, nSim) != 0)
        {
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            return;
        }

        char rsp[40];
        sprintf(rsp, "+CREG: %d,%d", gAtSetting.sim[nSim].creg, nStatusInfo.nStatus);

        if (gAtSetting.sim[nSim].creg == 2)
        {
            sprintf(rsp, "+CREG: %d,%d,\"%02x%02x\",\"%02x%02x\",%d",
                    gAtSetting.sim[nSim].creg, nStatusInfo.nStatus, nStatusInfo.nAreaCode[3],
                    nStatusInfo.nAreaCode[4], nStatusInfo.nCellId[0],
                    nStatusInfo.nCellId[1], Mapping_Creg_From_PsType(nCurrRat));
        }
        else
        {
            sprintf(rsp, "+CREG: %d,%d", gAtSetting.sim[nSim].creg, nStatusInfo.nStatus);
        }
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

// Enhanced signal quality report
void atCmdHandleECSQ(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);

    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint8_t mode = atParamUintInRange(cmd->params[0], 0, 1, &paramok);
        if (!paramok || cmd->param_count > 1)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        gAtSetting.sim[nSim].ecsq = mode;
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        atCmdRespInfoText(cmd->engine, "+ECSQ: (0,1)");
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ)
    {
        char rsp[40];
        sprintf(rsp, "+ECSQ: %d", gAtSetting.sim[nSim].ecsq);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

void atCmdHandleCSQ(atCommand_t *cmd)
{
#ifndef CONFIG_SOC_6760
    uint8_t nSim = atCmdGetSim(cmd->engine);
#endif
    if (cmd->type == AT_CMD_EXE)
    {
        uint8_t nSignalLevel = 99;
        uint8_t nBitError = 99;
#ifndef CONFIG_SOC_6760
        uint32_t ret = 0;
        uint8_t rat = 0;

        CFW_COMM_MODE nFM = CFW_DISABLE_COMM;
        ret = CFW_GetComm(&nFM, nSim);
        if (nFM != CFW_DISABLE_COMM)
        {
            rat = CFW_NWGetStackRat(nSim);
            OSI_LOGXI(OSI_LOGPAR_I, 0, "rat: %d", rat);
            if (rat == 4)
            {
                CFW_NW_QUAL_INFO iQualReport;

                ret = CFW_NwGetLteSignalQuality(&nSignalLevel, &nBitError, nSim);
                if (ret != 0)
                {
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
                }
                ret = CFW_NwGetQualReport(&iQualReport, nSim);
                if (ret != 0)
                {
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
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
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }
#else
        extern void nbiot_atGetUeRssiAndBer(uint8_t * rssi, uint8_t * ber);
        nbiot_atGetUeRssiAndBer(&nSignalLevel, &nBitError);
#endif
        char rsp[40];
        sprintf(rsp, "+CSQ: %d,%d", nSignalLevel, nBitError);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        atCmdRespInfoText(cmd->engine, "+CSQ: (0-31,99),(0-7,99)");
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

static void _rsrqFromNrsrq(uint8_t *rsrq, int8_t nRsrq)
{
    *rsrq = nRsrq;
}

static void _rsrpFromNrsrp(uint8_t *rsrp, int8_t nRsrp)
{
#if 0
    if (nRsrp > 97)
    {
        *rsrp = 31;
    }
    else if (nRsrp < 0)
    {
        *rsrp = 0;
    }
    else if (nRsrp >= 0 && nRsrp <= 97)
    {
        *rsrp = nRsrp / 3;
    }
#else
    *rsrp = nRsrp;
#endif
}

void atCmdHandleCESQ(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);

    if (cmd->type == AT_CMD_EXE)
    {
        CFW_NW_QUAL_INFO sQualReport;
        uint8_t rxlev = 99;
        uint8_t ber = 99;
        uint8_t rsrq = 255;
        uint8_t rsrp = 255;
        uint32_t ret = 0;
        uint8_t rat = 0;

        rat = CFW_NWGetStackRat(nSim);
        OSI_LOGXI(OSI_LOGPAR_I, 0, "CESQ:rat: %d", rat);

        ret = CFW_NwGetQualReport(&sQualReport, nSim);
        if (ret != 0)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
        OSI_LOGXI(OSI_LOGPAR_I, 0, "CESQ:nRssi: %d,nRxQual: %d,iRsrq: %d,iRsrp: %d", sQualReport.nRssi, sQualReport.nRxQual, sQualReport.iRsrq, sQualReport.iRsrp);
        if (rat == 4)
        {
            //_rsrqFromNrsrq(&rsrq, sQualReport.iRsrq);
            _rsrpFromNrsrp(&rsrp, sQualReport.iRsrp + 17);
            if (rsrp < 17)
            {
                rsrp = 0;
            }
            else if ((17 < rsrp) && (rsrp < 114))
            {
                rsrp -= 17;
            }
            else
            {
                rsrp = 97;
            }
            if (sQualReport.iRsrq < 0)
            {
                rsrq = 0;
            }
            else if (sQualReport.iRsrq > 34)
            {
                rsrq = 34;
            }
            else
            {
                _rsrqFromNrsrq(&rsrq, sQualReport.iRsrq);
            }
        }
        else
        {
            CFW_NwGetSignalQuality(&rxlev, &ber, nSim);
            if (rxlev > 110)
            {
                rxlev = 0;
            }
            else if ((rxlev <= 110) && (rxlev >= 48))
            {
                rxlev = (uint8_t)(63 - (rxlev - 48));
            }
            else
            {
                rxlev = 63;
            }
        }

        char rsp[60] = {0};
        sprintf(rsp, "+CESQ: %d,%d,255,255,%d,%d", rxlev, ber, rsrq, rsrp);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        atCmdRespInfoText(cmd->engine, "+CESQ: (0-62,99),(0-7,99),(255),(255),(0-34,255),(0-97,255)");
        atCmdRespOK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

#define CSW_GSM_450 (1 << 0)
#define CSW_GSM_480 (1 << 1)
#define CSW_GSM_900P (1 << 2)
#define CSW_GSM_900E (1 << 3)
#define CSW_GSM_900R (1 << 4)
#define CSW_GSM_850 (1 << 5)
#define CSW_DCS_1800 (1 << 6)
#define CSW_PCS_1900 (1 << 7)
bool atCheckBandInArfcnList(uint8_t Band, uint16_t Bcch)
{
    bool Ret = false;

    switch (Band)
    {
    case 0:
        if ((Bcch <= 124))
            Ret = true;
        else if ((Bcch >= 128) && (Bcch <= 251))
            Ret = true;
        else if ((Bcch >= 512) && (Bcch <= 885))
            Ret = true;
        else if ((Bcch >= 975) && (Bcch <= 1023))
            Ret = true;
        break;

    case 1:
        if ((Bcch >= 975) && (Bcch <= 1023))
            Ret = true;
        break;

    case 2:
        if ((Bcch >= 512) && (Bcch <= 885))
            Ret = true;
        break;

    default:
        break;
    }
    OSI_LOGI(0, "atCheckBandInArfcnList end %d:\n", Ret);
    return Ret;
}

extern void atPlmnArray2Api(uint8_t *nOperatorId, uint8_t *pLAI);
extern char CFW_NwGetLockBCCH();
extern uint32_t CFW_NwSetLockBCCH(bool bLock, int16_t nUTI, CFW_SIM_ID nSimID);
//ChinaMobile freq:0x1d,0x14 We can use this two freqs do the test in beijing office
void atCmdHandleMYBCCH(atCommand_t *cmd)
{
    int32_t iResult = 0;
    uint16_t freqnb = 0;
    uint8_t i = 0;
    char pRsp[40] = {
        0,
    };
    uint32_t ret = 0;
    uint8_t nOperatorId[7] = {
        0,
    };
    uint16_t bcch[3] = {
        0,
    };
    uint8_t nMode = 0;
    CFW_NW_STATUS_INFO nStatusInfo;
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint8_t nBand = 0;

    OSI_LOGI(0, "MYBCCH, start!\n");

    switch (cmd->type)
    {
    case AT_CMD_SET:
        if (strcmp("+LOCKBCCH", cmd->desc->name) != 0)
        {
            ret = CFW_GprsGetstatus(&nStatusInfo, nSim);
            if ((ERR_SUCCESS != ret) || ((CFW_NW_STATUS_REGISTERED_HOME != nStatusInfo.nStatus) && (CFW_NW_STATUS_REGISTERED_ROAMING != nStatusInfo.nStatus)))
            {
                OSI_LOGI(0, "MYBCCH, NwGetStatus error!nStatus:%d\n", nStatusInfo.nStatus);
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
            }
        }
        ret = CFW_CfgNwGetFrequencyBand(&nBand, nSim);
        if (ERR_SUCCESS != ret)
        {
            OSI_LOGI(0, "AT+MYBAND:Get band error %d\n\r", ret);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            return;
        }
        if ((nBand & 0xff) == CSW_GSM_900E)
        {
            nBand = 1;
        }
        else if ((nBand & 0xff) == CSW_DCS_1800)
        {
            nBand = 2;
        }
        else
            nBand = 0;
        // uMode   0:close lock freq 1:open lock freq
        bool paramok = true;
        uint8_t uMode = atParamDefUintInRange(cmd->params[0], 0, 0, 1, &paramok);
        if (cmd->param_count > 1)
        {
            bcch[0] = atParamUintInRange(cmd->params[1], 0, 1024, &paramok);
            if (!atCheckBandInArfcnList(nBand, bcch[0]))
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }
        if (cmd->param_count > 2)
        {
            bcch[1] = atParamUintInRange(cmd->params[2], 0, 1024, &paramok);
            if (!atCheckBandInArfcnList(nBand, bcch[1]))
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }
        if (cmd->param_count > 3)
        {
            bcch[2] = atParamUintInRange(cmd->params[3], 0, 1024, &paramok);
            if (!atCheckBandInArfcnList(nBand, bcch[2]))
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }

        if (!paramok || cmd->param_count > 4)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        if ((1 == uMode) && (1 == cmd->param_count))
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        if ((cmd->param_count == 1) && (uMode == gAtSetting.bcchmode))
            AT_CMD_RETURN(atCmdRespOK(cmd->engine));
        CFW_StoredPlmnList nStorePlmnList;
        memset(&nStorePlmnList, 0x00, sizeof(CFW_StoredPlmnList));
        ret = CFW_NwGetCurrentOperator(nOperatorId, &nMode, nSim);
        if (ERR_SUCCESS != ret)
        {
            OSI_LOGI(0, "MYBCCH, GetCurrentOperator error!\n");
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
        }
        atPlmnArray2Api(nOperatorId, nStorePlmnList.Plmn);

        OSI_LOGI(0, "MYBCCH  freqnb:%d", freqnb);
        for (i = 0; i < cmd->param_count - 1; i++)
        {
            nStorePlmnList.ArfcnL[i] = bcch[i];
            OSI_LOGI(0, "MYBCCH: bcch:%d i %d ArfcnL 0x%x", bcch[i], i, nStorePlmnList.ArfcnL[i]);
        }
        nStorePlmnList.nCount = cmd->param_count - 1;
        OSI_LOGI(0, "MYBCCH 11111  bcch num:%d", cmd->param_count - 1);
        iResult = CFW_CfgSetStoredPlmnList(&nStorePlmnList, nSim);
        if (iResult != ERR_SUCCESS)
        {
            OSI_LOGI(0, "MYBCCH:Set store list error %d\n\r", iResult);
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
        }

        iResult = CFW_NwSetLockBCCH(uMode, 0, nSim);
        if (iResult != ERR_SUCCESS)
        {
            OSI_LOGI(0, "MYBCCH:Set lock BCCH error %d\n\r", iResult);
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
        }
        gAtSetting.bcchmode = uMode;
        atCfgAutoSave();
        atCmdRespOK(cmd->engine);
        break;

    case AT_CMD_TEST:
        memset(&pRsp, 0x0, 40);
        if (strcmp("+LOCKBCCH", cmd->desc->name) == 0)
        {
            sprintf(pRsp, "+LOCKBCCH: (0-1),(0-1024)\n");
        }
        else
        {
            sprintf(pRsp, "$MYBCCH: (0-1),(0-1024)\n");
        }
        atCmdRespInfoText(cmd->engine, pRsp);
        atCmdRespOK(cmd->engine);
        break;

    case AT_CMD_READ:
        memset(&pRsp, 0x0, 40);
        if (strcmp("+LOCKBCCH", cmd->desc->name) == 0)
        {
            sprintf(pRsp, "+LOCKBCCH: %d", gAtSetting.bcchmode);
        }
        else
        {
            sprintf(pRsp, "$MYBCCH: %d", gAtSetting.bcchmode);
        }
        atCmdRespInfoText(cmd->engine, pRsp);
        atCmdRespOK(cmd->engine);
        break;

    default:
        atCmdRespCmsError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
        break;
    }
    return;
}

static void _ctecSetCommEnableRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    CFW_EVENT cfwEvent = *(const CFW_EVENT *)event;
    uint8_t nSim = atCmdGetSim(cmd->engine);
    OSI_LOGI(0, "_ctecSetCommEnableRsp\n\r");
    if (cfwEvent.nType != 0)
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

    uint8_t nPreferRat = CFW_NWGetRat(nSim);
    char rsp[40];
    sprintf(rsp, "+CTEC: %d,%d", nPreferRat, nPreferRat);
    atCmdRespInfoText(cmd->engine, rsp);
    atCmdRespOK(cmd->engine);
}

static void _ctecSetCommRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    CFW_EVENT cfwEvent = *(const CFW_EVENT *)event;
    uint8_t nSim = atCmdGetSim(cmd->engine);
    OSI_LOGI(0, "_ctecSetCommRsp\n\r");
    if (cfwEvent.nType != 0)
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
    CFW_COMM_MODE nFM = CFW_DISABLE_COMM;
    CFW_GetComm(&nFM, nSim);
    OSI_LOGI(0, "_ctecSetCommRsp %d\n\r", nFM);
    if (CFW_ENABLE_COMM != nFM)
    {
        cmd->uti = cfwRequestUTI((osiEventCallback_t)_ctecSetCommEnableRsp, cmd);
        atSetFlightModeFlag(AT_ENABLE_COMM, nSim);
        if (0 != CFW_SetComm(CFW_ENABLE_COMM, 0, cmd->uti, nSim))
        {
            cfwReleaseUTI(cmd->uti);
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }
    }
    else
        atCmdRespOK(cmd->engine);
    RETURN_FOR_ASYNC();
}

void atCmdHandleCTEC(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint32_t ret;

    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint8_t nCurrentRat = atParamUint(cmd->params[0], &paramok);
        uint8_t nPreferRat = atParamUint(cmd->params[1], &paramok);
        if (!paramok || cmd->param_count != 2)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        if (nPreferRat != CFW_RAT_AUTO && nPreferRat != CFW_RAT_GSM_ONLY && nPreferRat != CFW_RAT_LTE_ONLY && nPreferRat != CFW_RAT_NBIOT_ONLY)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }
        if (nCurrentRat != CFW_NWGetRat(nSim))
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        else
        {
            if (nCurrentRat == nPreferRat)
                RETURN_OK(cmd->engine);
            else
            {
                // Rat,   0:is CFW_RAT_AUTO , 2 is CFW_RAT_GSM_ONLY, 4 is CFW_RAT_UMTS_ONLY.
                if ((ret = CFW_NWSetRat(nPreferRat, nSim)) != 0)
                {
                    OSI_LOGI(0x1000491b, "AT+CTEC:eParamOk:333 %d\n\r", ret);
                    atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
                    return;
                }
                else
                {
                    //  uint8_t nCurrentStackRat = CFW_NWGetStackRat(nSim);
                    //   if (nPreferRat == nCurrentStackRat)
                    //    RETURN_OK(cmd->engine);
                    CFW_COMM_MODE nFM = CFW_DISABLE_COMM;
                    if (0 != CFW_GetComm(&nFM, nSim))
                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
                    if (CFW_DISABLE_COMM == nFM)
                        RETURN_OK(cmd->engine);
                    else
                    {
                        cmd->uti = cfwRequestUTI((osiEventCallback_t)_ctecSetCommRsp, cmd);
                        atSetFlightModeFlag(AT_DISABLE_COMM, nSim);
                        uint8_t nFlag = 0;
                        if (CFW_RAT_GSM_ONLY == nCurrentRat)
                            nFlag = 3; //0---->3 modify for bug1235060
                        if (0 != CFW_SetComm(CFW_DISABLE_COMM, nFlag, cmd->uti, nSim))
                        {
                            cfwReleaseUTI(cmd->uti);
                            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
                        }
                        RETURN_FOR_ASYNC();
                    }
                }
            }
        }
    }
    else if (cmd->type == AT_CMD_READ)
    {
        uint8_t nPreferRat = CFW_NWGetRat(nSim);
        char rsp[40];
        sprintf(rsp, "+CTEC: %d,%d", nPreferRat, nPreferRat);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

#ifdef GPS_SUPPORT
AT_CCED gpsPosiList;
#endif
/*
 *******************************************************************************
 *  +CCED related process                                                   *
 *******************************************************************************
*/
#define CFW_RAT_LTE 4

typedef struct
{
    uint8_t lteReqType;
    uint8_t replyErrCode;
} atCcedContext_t;
typedef struct
{
    uint8_t nSim;
    uint16_t nUti;
    uint8_t nReqType;
    uint8_t nCurNetMode;
} atCcedTimerContext_t;

bool AT_EMOD_CCED_ProcessData(uint8_t nSim, CFW_TSM_CURR_CELL_INFO *pCurrCellInfo,
                              CFW_TSM_ALL_NEBCELL_INFO *pNeighborCellInfo, char *nStrPosiList)
{
    uint8_t i = 0;
    volatile AT_CCED PosiList;
    char nStr[100] = {0};
    memset((void *)&PosiList, 0, sizeof(AT_CCED));
    OSI_LOGI(0x100048ad, "AT_EMOD_CCED_ProcessData!");

    if ((NULL == pCurrCellInfo && NULL == pNeighborCellInfo) || (NULL == nStrPosiList))
    {
        OSI_LOGI(0x100048ae, "AT_EMOD_CCED_ProcessData InputErr\n");
        return false;
    }

    OSI_LOGI(0x100048af, "pCurrCellInfo != NULL");
    if (pCurrCellInfo != NULL)
    {
        strcat(nStrPosiList, "+CCED:GSM current cell info:");
        PosiList.sMcc[0] = pCurrCellInfo->nTSM_LAI[0] & 0x0F;
        PosiList.sMcc[1] = pCurrCellInfo->nTSM_LAI[0] >> 4;
        PosiList.sMcc[2] = pCurrCellInfo->nTSM_LAI[1] & 0x0F;
        PosiList.nArfcn = pCurrCellInfo->nTSM_Arfcn;

        PosiList.sMnc[0] = pCurrCellInfo->nTSM_LAI[2] & 0x0F;
        PosiList.sMnc[1] = pCurrCellInfo->nTSM_LAI[2] >> 4;
        PosiList.sMnc[2] = pCurrCellInfo->nTSM_LAI[1] >> 4;

        PosiList.sLac = (pCurrCellInfo->nTSM_LAI[3] << 8) | (pCurrCellInfo->nTSM_LAI[4]);
        PosiList.sCellID = (pCurrCellInfo->nTSM_CellID[0] << 8) | (pCurrCellInfo->nTSM_CellID[1]);

        PosiList.iBsic = pCurrCellInfo->nTSM_Bsic;

        //PosiList.iRxLev    = ((110 - pCurrCellInfo->nTSM_AvRxLevel) * 100) / 64;
        //Get Real RSSI for 8810 CCED issue by XP 20130930

        PosiList.iRxLevSub = pCurrCellInfo->nTSM_RxLevSub;
        PosiList.iRxLev = pCurrCellInfo->nTSM_RxLev;

        OSI_LOGI(0x100048b0, "AT_EMOD_CCED_ProcessData pCurrCellInfo\n sMcc=%d%d%d,sMnc=%d%d%d,sLac=%x,sCellID=%x,iBsic=%d,iRxLev=%d,iRxLevSub=%d PosiList.nArfcn=0x%x", PosiList.sMcc[0], PosiList.sMcc[1], PosiList.sMcc[2], PosiList.sMnc[0], PosiList.sMnc[1], PosiList.sMnc[2], PosiList.sLac, PosiList.sCellID, PosiList.iBsic, PosiList.iRxLev, PosiList.iRxLevSub, PosiList.nArfcn);
#ifndef CHIP_HAS_AP
        if (PosiList.sMnc[2] > 9)
        {
            sprintf(nStr, "%d%d%d,%d%d,%x,%x,%d,%d,%d,%x",
                    PosiList.sMcc[0], PosiList.sMcc[1], PosiList.sMcc[2],
                    PosiList.sMnc[0], PosiList.sMnc[1],
                    PosiList.sLac, PosiList.sCellID, PosiList.iBsic, PosiList.iRxLev, PosiList.iRxLevSub, PosiList.nArfcn);
        }
        else
        {
            sprintf(nStr, "%d%d%d,%d%d%d,%x,%x,%d,%d,%d,%x",
                    PosiList.sMcc[0], PosiList.sMcc[1], PosiList.sMcc[2],
                    PosiList.sMnc[0], PosiList.sMnc[1], PosiList.sMnc[2],
                    PosiList.sLac, PosiList.sCellID, PosiList.iBsic, PosiList.iRxLev, PosiList.iRxLevSub, PosiList.nArfcn);
        }
#else
        if (PosiList.sMnc[2] > 9)
        {
            sprintf(nStr, "%d%d%d,%d%d,%x,%x,%d,%d,%d",
                    PosiList.sMcc[0], PosiList.sMcc[1], PosiList.sMcc[2],
                    PosiList.sMnc[0], PosiList.sMnc[1],
                    PosiList.sLac, PosiList.sCellID, PosiList.iBsic, PosiList.iRxLev, PosiList.iRxLevSub);
        }
        else
        {
            sprintf(nStr, "%d%d%d,%d%d%d,%x,%x,%d,%d,%d",
                    PosiList.sMcc[0], PosiList.sMcc[1], PosiList.sMcc[2],
                    PosiList.sMnc[0], PosiList.sMnc[1], PosiList.sMnc[2],
                    PosiList.sLac, PosiList.sCellID, PosiList.iBsic, PosiList.iRxLev, PosiList.iRxLevSub);
        }
#endif
        strcat(nStrPosiList, nStr);
    }

    if (pNeighborCellInfo != NULL)
    {
        OSI_LOGI(0, "AT_EMOD_CCED_ProcessData-->pNeighborCellInfo != Null");
        int iCount = 0x00;
        strcat(nStrPosiList, "+CCED:GSM neighbor cell info:");
        for (i = 0; i < pNeighborCellInfo->nTSM_NebCellNUM; i++)
        {
            PosiList.sMcc[0] = pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[0] & 0x0F;
            PosiList.sMcc[1] = pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[0] >> 4;
            PosiList.sMcc[2] = pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[1] & 0x0F;
#if 1
            PosiList.sMnc[0] = pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[2] & 0x0F;
            PosiList.sMnc[1] = pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[2] >> 4;
            PosiList.sMnc[2] = pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[1] >> 4;
#else
            PosiList.sMnc[0] = pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[1] >> 4;
            if (PosiList.sMnc[0] > 9)
                PosiList.sMnc[0] = 0;
            PosiList.sMnc[1] = pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[2] & 0x0F;
            PosiList.sMnc[2] = pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[2] >> 4;
#endif
            PosiList.sLac =
                (pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[3] << 8) | (pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[4]);
            PosiList.sCellID =
                (pNeighborCellInfo->nTSM_NebCell[i].nTSM_CellID[0] << 8) | (pNeighborCellInfo->nTSM_NebCell[i].nTSM_CellID[1]);
            PosiList.iBsic = pNeighborCellInfo->nTSM_NebCell[i].nTSM_Bsic;
            //Get Real RSSI for 8810 CCED issue by XP 20130930
            PosiList.iRxLev = pNeighborCellInfo->nTSM_NebCell[i].nTSM_AvRxLevel;

            if (pCurrCellInfo != NULL || i > 0)
            {
                OSI_LOGI(0x100048b1, "pCurrCellInfo 0x%x,i=%d", pCurrCellInfo, i);
                if (iCount > 0)
                    strcat(nStrPosiList, "+CCED:GSM neighbor cell info:");
            }
            iCount++;
            if (PosiList.sMnc[2] > 9)
            {
                sprintf(nStr, "%d%d%d,%d%d,%d,%d,%d,%d\r\n",
                        PosiList.sMcc[0], PosiList.sMcc[1], PosiList.sMcc[2],
                        PosiList.sMnc[0], PosiList.sMnc[1],
                        PosiList.sLac, PosiList.sCellID, PosiList.iBsic, PosiList.iRxLev);
            }
            else
            {
                sprintf(nStr, "%d%d%d,%d%d%d,%d,%d,%d,%d\r\n",
                        PosiList.sMcc[0], PosiList.sMcc[1], PosiList.sMcc[2],
                        PosiList.sMnc[0], PosiList.sMnc[1], PosiList.sMnc[2],
                        PosiList.sLac, PosiList.sCellID, PosiList.iBsic, PosiList.iRxLev);
            }
            OSI_LOGI(0x100048b2, "AT_EMOD_CCED_ProcessData pNeighborCellInfo[%d] sMcc=%d%d%d,sMnc=%d%d%d,sLac=%x,sCellID=%x,iBsic=%d,iRxLev=%d,iRxLevSub=%d", i, PosiList.sMcc[0], PosiList.sMcc[1], PosiList.sMcc[2], PosiList.sMnc[0], PosiList.sMnc[1], PosiList.sMnc[2], PosiList.sLac, PosiList.sCellID, PosiList.iBsic, PosiList.iRxLev);
            strcat(nStrPosiList, nStr);
        }
        if (i == 0)
        {
            strcat(nStrPosiList, "0,0,0,0,0,0\r\n");
        }
    }
#ifdef GPS_SUPPORT
    memcpy(&gpsPosiList, &PosiList, sizeof(gpsPosiList));
#endif
    return true;
}
static const osiValueStrMap_t gCcedStr[] = {
    {0, "n6"},
    {1, "n15"},
    {2, "n25"},
    {3, "n50"},
    {4, "n75"},
    {5, "n100"}};

static void _ccedProcessLTEData(CFW_NET_INFO_T *pNetinfo, char *nStr, uint8_t nRequestedDump)
{
    OSI_LOGI(0, "_ccedProcessLTEData nRequestedDump = %d", nRequestedDump);
    if (nRequestedDump == 1)
    {
        const char *str = osiVsmapFindStr(gCcedStr, pNetinfo->nwCapabilityLte.lteScell.bandwidth, NULL);
        if (str == NULL)
        {
            OSI_LOGI(0, "_ccedProcessLTEData bandwidth = NULL");
            static char strTemp[10] = "NULL";
            str = &strTemp[0];
        }
        //for LTE rat Main Cell:plmn,imsi,roamingFlag,bandInfo,bandwidth,dlEarfcn,pcid,rsrp,rsrq
        nStr += sprintf(nStr, "+CCED:LTE current cell:");
        //%x%x%x,%x%x%x,",
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[0] & 0x0F) != 0x0F)
        {
            nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[0] & 0x0F));
        }
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[0] >> 4) != 0x0F)
        {
            nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[0] >> 4));
        }
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[1] & 0x0F) != 0x0F)
        {
            nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[1] & 0x0F));
        }
        nStr += sprintf(nStr, ",");
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[2] & 0x0F) != 0x0F)
        {
            nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[2] & 0x0F));
        }
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[2] >> 4) != 0x0F)
        {
            nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[2] >> 4 & 0x0F));
        }
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[1] >> 4) != 0x0F)
        {
            nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[1] >> 4 & 0x0F));
        }
        nStr += sprintf(nStr, ",");
        if (pNetinfo->nwCapabilityLte.lteScell.imsi[0] != 0)
        {
            for (int i = 0; i < pNetinfo->nwCapabilityLte.lteScell.imsi[0]; i++)
            {
                nStr += sprintf(nStr, "%d", pNetinfo->nwCapabilityLte.lteScell.imsi[i + 1]);
            }
        }
        sprintf(nStr, ",%d,%d,%s,%ld,%ld,%d,%d,%d,%d,%d", pNetinfo->nwCapabilityLte.lteScell.roamingFlag,
                pNetinfo->nwCapabilityLte.lteScell.bandInfo,
                str,
                pNetinfo->nwCapabilityLte.lteScell.dlEarfcn,
                ((pNetinfo->nwCapabilityLte.lteScell.enodeBID << 8) | (pNetinfo->nwCapabilityLte.lteScell.cellId)),
                pNetinfo->nwCapabilityLte.lteScell.rsrp,
                pNetinfo->nwCapabilityLte.lteScell.rsrq,
                pNetinfo->nwCapabilityLte.lteScell.tac,
                pNetinfo->nwCapabilityLte.lteScell.Srxlev,
                pNetinfo->nwCapabilityLte.lteScell.pcid);
    }
    else
    {
        int j = 0;
        //for LTE rat Neighbour Cell:frequency,pcid,rsrp,rsrq
        if (pNetinfo->nwCapabilityLte.lteNcellNum > 7)
        {
            pNetinfo->nwCapabilityLte.lteNcellNum = 7;
        }
        for (j = 0; j < pNetinfo->nwCapabilityLte.lteNcellNum; j++)
        {
            nStr += sprintf(nStr, "+CCED:LTE neighbor cell:");
            if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[0] & 0x0F) != 0x0F)
            {
                nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[0] & 0x0F));
            }
            if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[0] >> 4) != 0x0F)
            {
                nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[0] >> 4));
            }
            if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[1] & 0x0F) != 0x0F)
            {
                nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[1] & 0x0F));
            }
            nStr += sprintf(nStr, ",");
            if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[2] & 0x0F) != 0x0F)
            {
                nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[2] & 0x0F));
            }
            if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[2] >> 4) != 0x0F)
            {
                nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[2] >> 4 & 0x0F));
            }
            if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[1] >> 4) != 0x0F)
            {
                nStr += sprintf(nStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[1] >> 4 & 0x0F));
            }
            nStr += sprintf(nStr, ",%ld,%ld,%d,%d,%d,%d,%d\r\n",
                            pNetinfo->nwCapabilityLte.lteNcell[j].frequency,
                            ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.enodeBID << 8) | (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.cellId)),
                            pNetinfo->nwCapabilityLte.lteNcell[j].rsrp,
                            pNetinfo->nwCapabilityLte.lteNcell[j].rsrq,
                            pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.tac,
                            pNetinfo->nwCapabilityLte.lteNcell[j].Srxlev,
                            pNetinfo->nwCapabilityLte.lteNcell[j].pcid);
        }
        if (j == 0)
        {
            sprintf(nStr, "+CCED:LTE neighbor cell:0,0,0,0,0,0,0,0,0\r\n");
        }
    }
}
static void CCED_TimerV1CB(void *ctx)
{
    int32_t iRet = 0;
    uint8_t nSignalLevel = 0;
    uint8_t nBitError = 0;
    char nBuf[20];

    atCcedTimerContext_t *pCtx = (atCcedTimerContext_t *)ctx;
    OSI_LOGI(0, "Come In CCED_TimerCB nSim ==%d", pCtx->nSim);
    pCtx->nCurNetMode = CFW_NWGetStackRat(pCtx->nSim);
    OSI_LOGXI(OSI_LOGPAR_I, 0, "rat: %d", pCtx->nCurNetMode);
    if (pCtx->nCurNetMode == CFW_RAT_LTE)
    {
        iRet = CFW_NwGetLteSignalQuality(&nSignalLevel, &nBitError, pCtx->nSim);
        if (nSignalLevel < 17)
        {
            nSignalLevel = 0;
        }
        else if ((17 <= nSignalLevel) && (nSignalLevel < 113))
        {
            nSignalLevel = (uint8_t)((nSignalLevel - 17) / 3);
        }
        else
        {
            nSignalLevel = 31;
        }
    }
    else
    {
        iRet = CFW_NwGetSignalQuality(&nSignalLevel, &nBitError, pCtx->nSim);
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
    OSI_LOGI(0, "CCED_TimerCB: Finish CFW_NwGetSignalQuality,iRet=%d  iRet==%x", iRet, iRet);
    osiTimerStart(ccedtimer, 10000);
    if (ERR_SUCCESS == iRet)
    {
        memset(nBuf, 0, 20);
        char *pBuf = &nBuf[0];
        pBuf += sprintf(pBuf, "+CCED: %d,%d", nSignalLevel, nBitError);
        atCmdRespDefUrcText(nBuf);
    }
}
static void CCED_TimerRspCB(void *ctx, const osiEvent_t *event)
{
    OSI_LOGI(0, "CCED_TimerRspCB------");
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_SIM_ID nSim = (CFW_SIM_ID)cfw_event->nFlag;
    atCcedTimerContext_t *pCtx = (atCcedTimerContext_t *)ctx;
    uint32_t nRet = 0;
    bool bRet = false;

    CFW_TSM_CURR_CELL_INFO tCurrCellInf;
    CFW_TSM_ALL_NEBCELL_INFO tNeighborCellInfo;
    CFW_NET_INFO_T tNetLTEinfo;

    char *pTxtBuf = malloc(1024);
    memset(&tCurrCellInf, 0, sizeof(CFW_TSM_CURR_CELL_INFO));
    memset(&tNeighborCellInfo, 0, sizeof(CFW_TSM_ALL_NEBCELL_INFO));
    memset(&tNetLTEinfo, 0, sizeof(CFW_NET_INFO_T));

    if (cfw_event->nEventId == EV_CFW_TSM_INFO_IND)
    {
        if (cfw_event->nType != 0)
        {
            OSI_LOGI(0, "CCED_TimerRspCB:   ERROR!!!---CfwEvent.nType --0x%x\n\r", cfw_event->nType);
            sprintf(pTxtBuf, "+CCED: %d", ERR_AT_CME_EXE_FAIL);
            atCmdRespDefUrcText(pTxtBuf);
            free(pTxtBuf);
            osiTimerDelete(ccedtimerV2);
            ccedtimerV2 = NULL;
            return;
        }
        if (cfw_event->nParam2 == CFW_TSM_CURRENT_CELL)
        {
            if (ccedtimerV2 != NULL)
            {
                nRet = CFW_GetCellInfo(&tCurrCellInf, &tNeighborCellInfo, nSim);
                if (nRet != ERR_SUCCESS)
                {
                    OSI_LOGI(0, "NW_AsyncEvent ERROR nRet:0x%x!\n", nRet);
                    sprintf(pTxtBuf, "+CCED: %d", ERR_AT_CME_EXE_FAIL);
                    atCmdRespDefUrcText(pTxtBuf);
                }
                else
                {
                    bRet = AT_EMOD_CCED_ProcessData(nSim, &tCurrCellInf, NULL, pTxtBuf);
                    if (bRet)
                    {
                        atCmdRespDefUrcText(pTxtBuf);
                    }
                    else
                    {
                        sprintf(pTxtBuf, "+CCED: %d", ERR_AT_CME_EXE_FAIL);
                        atCmdRespDefUrcText(pTxtBuf);
                    }
                }
            }
            pCtx->nUti = cfwRequestUTI((osiEventCallback_t)CCED_TimerRspCB, pCtx);
            if (0 != CFW_EmodOutfieldTestEnd(pCtx->nUti, nSim))
            {
                cfwReleaseUTI(pCtx->nUti);
                osiTimerDelete(ccedtimerV2);
                ccedtimerV2 = NULL;
                return;
            }
        }
        else if (cfw_event->nParam2 == CFW_TSM_NEIGHBOR_CELL)
        {
            if (ccedtimerV2 != NULL)
            {
                nRet = CFW_GetCellInfo(&tCurrCellInf, &tNeighborCellInfo, nSim);
                if (nRet != ERR_SUCCESS)
                {
                    OSI_LOGI(0, "NW_AsyncEvent ERROR nRet:0x%x!\n", nRet);
                    sprintf(pTxtBuf, "+CCED: %d", ERR_AT_CME_EXE_FAIL);
                    atCmdRespDefUrcText(pTxtBuf);
                }
                else
                {
                    OSI_LOGI(0, "NW_AsyncEvent nTSM_NebCellNUM :%d!\n", tNeighborCellInfo.nTSM_NebCellNUM);
                    bRet = AT_EMOD_CCED_ProcessData(nSim, NULL, &tNeighborCellInfo, pTxtBuf);
                    if (bRet)
                    {
                        atCmdRespDefUrcText(pTxtBuf);
                    }
                    else
                    {
                        sprintf(pTxtBuf, "+CCED: %d", ERR_AT_CME_EXE_FAIL);
                        atCmdRespDefUrcText(pTxtBuf);
                    }
                }
            }
            pCtx->nUti = cfwRequestUTI((osiEventCallback_t)CCED_TimerRspCB, pCtx);
            if (0 != CFW_EmodOutfieldTestEnd(pCtx->nUti, nSim))
            {
                cfwReleaseUTI(pCtx->nUti);
                osiTimerDelete(ccedtimerV2);
                ccedtimerV2 = NULL;
                return;
            }
        }
        else if (cfw_event->nParam2 == CFW_TSM_PLOT_LTE)
        {
            if (ccedtimerV2 != NULL)
            {
                nRet = CFW_GetLETRealTimeInfo(&tNetLTEinfo, nSim);
                if (nRet != ERR_SUCCESS)
                {
                    OSI_LOGI(0, "NW_AsyncEvent CFW_GetNetLETInfo ERROR:0x%x !\n", nRet);
                    sprintf(pTxtBuf, "+CCED: %d", ERR_AT_CME_EXE_FAIL);
                }
                else
                {
                    _ccedProcessLTEData(&tNetLTEinfo, pTxtBuf, pCtx->nReqType);
                }
                atCmdRespDefUrcText(pTxtBuf);
            }
            pCtx->nUti = cfwRequestUTI((osiEventCallback_t)CCED_TimerRspCB, pCtx);
            if (0 != CFW_EmodOutfieldTestEnd(pCtx->nUti, nSim))
            {
                cfwReleaseUTI(pCtx->nUti);
                osiTimerDelete(ccedtimerV2);
                ccedtimerV2 = NULL;
                return;
            }
        }
    }
    else if (cfw_event->nEventId == EV_CFW_TSM_INFO_END_RSP)
    {
        free(pTxtBuf);
        osiTimerStart(ccedtimerV2, 5000);
        return;
    }
    free(pTxtBuf);
}
static void _ccedTimerV2CB(void *ctx)
{
    OSI_LOGI(0, "CCED Timer Start! ------");
    atCcedTimerContext_t *pCtx = (atCcedTimerContext_t *)ctx;

    CFW_TSM_FUNCTION_SELECT tSelecFUN;
    memset(&tSelecFUN, 0, sizeof(CFW_TSM_FUNCTION_SELECT));
    if (pCtx->nReqType == 1)
    {
        tSelecFUN.nServingCell = 1;
    }
    else
    {
        tSelecFUN.nNeighborCell = 1;
    }
    tSelecFUN.nCurrentNetMode = pCtx->nCurNetMode = CFW_NWGetStackRat(pCtx->nSim);
    pCtx->nUti = cfwRequestUTI((osiEventCallback_t)CCED_TimerRspCB, pCtx);
    if (0 != CFW_EmodOutfieldTestStart(&tSelecFUN, pCtx->nUti, pCtx->nSim))
    {
        cfwReleaseUTI(pCtx->nUti);
        osiTimerDelete(ccedtimerV2);
        ccedtimerV2 = NULL;
        return;
    }
}

static void CCED_TimerV3CB(void *ctx)
{
    OSI_LOGI(0, "CCED TimerV3 Start! ------");
    atCommand_t *cmd = (void *)ctx;
    uint32_t nSimID = atCmdGetSim(cmd->engine);
    uint32_t hAo = CFW_GetAoHandle(0, 0xFFFF, CFW_EMOD_SRV_ID, nSimID);
    if (0 != hAo)
    {
        osiTimerStart(ccedtimerV3, 1000);
        RETURN_FOR_ASYNC();
    }
    else
    {
        osiTimerDelete(ccedtimerV3);
        ccedtimerV3 = NULL;
        atCmdRespOK(cmd->engine);
    }
}

static uint8_t _ccedCreatTimer(uint8_t nRequest, uint8_t nNetMode, uint8_t nSim)
{
    atCcedTimerContext_t *ctx = (atCcedTimerContext_t *)calloc(1, sizeof(*ctx));
    if (ctx == NULL)
    {
        return ERR_AT_CME_NO_MEMORY;
    }

    memset(ctx, 0, sizeof(atCcedTimerContext_t));
    ctx->nSim = nSim;

    if (nRequest == 8)
    {
        ctx->nCurNetMode = nNetMode;
        ccedtimer = osiTimerCreate(atEngineGetThreadId(), CCED_TimerV1CB, (void *)ctx);
        osiTimerStart(ccedtimer, 10000);
    }
    else
    {
        ctx->nReqType = nRequest;
        ctx->nCurNetMode = nNetMode;
        ccedtimerV2 = osiTimerCreate(atEngineGetThreadId(), _ccedTimerV2CB, (void *)ctx);
    }
    return 0;
}
static bool _ccedDeleteTimer(uint8_t nRequest)
{
    if (nRequest == 8 && ccedtimer != NULL)
    {
        osiTimerDelete(ccedtimer);
        ccedtimer = NULL;
    }
    else if ((nRequest == 1 || nRequest == 2) && (ccedtimerV2 != NULL))
    {
        atCcedTimerContext_t *pCtx = (atCcedTimerContext_t *)osiTimerGetCbx(ccedtimerV2);
        if (pCtx->nReqType == nRequest)
        {
            osiTimerDelete(ccedtimerV2);
            ccedtimerV2 = NULL;
            return false;
        }
    }
    return true;
}
static void CCED_RspCB(atCommand_t *cmd, const osiEvent_t *event)
{
    OSI_LOGI(0, "CCED_RspCB------");
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_SIM_ID nSim = (CFW_SIM_ID)cfw_event->nFlag;
    atCcedContext_t *pCtx = (atCcedContext_t *)cmd->async_ctx;
    uint32_t nRet = 0;
    bool bRet = false;
    CFW_TSM_CURR_CELL_INFO tCurrCellInf;
    CFW_TSM_ALL_NEBCELL_INFO tNeighborCellInfo;
    CFW_NET_INFO_T tNetLTEinfo;

    char *pTxtBuf = malloc(1024);
    memset(&tCurrCellInf, 0, sizeof(CFW_TSM_CURR_CELL_INFO));
    memset(&tNeighborCellInfo, 0, sizeof(CFW_TSM_ALL_NEBCELL_INFO));
    memset(&tNetLTEinfo, 0, sizeof(CFW_NET_INFO_T));

    if (cfw_event->nEventId == EV_CFW_TSM_INFO_IND)
    {
        if (cfw_event->nType != 0)
        {
            OSI_LOGI(0, "CCED_RspCB: ERROR!!!---CfwEvent.nType --0x%x\n\r", cfw_event->nType);
            if (ccedtimerV2 != NULL)
            {
                _ccedDeleteTimer(pCtx->lteReqType);
            }
            free(pTxtBuf);
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
        }
        if (cfw_event->nParam2 == CFW_TSM_CURRENT_CELL)
        {
            nRet = CFW_GetCellInfo(&tCurrCellInf, &tNeighborCellInfo, nSim);
            if (nRet != ERR_SUCCESS)
            {
                OSI_LOGI(0, "NW_AsyncEvent ERROR nRet:0x%x!\n", nRet);
                pCtx->replyErrCode = 1;
            }
            else
            {
                bRet = AT_EMOD_CCED_ProcessData(nSim, &tCurrCellInf, NULL, pTxtBuf);
                if (bRet)
                {
                    atCmdRespInfoText(cmd->engine, pTxtBuf);
                    pCtx->replyErrCode = 0;
                }
                else
                {
                    pCtx->replyErrCode = 1;
                }
            }
            cmd->uti = cfwRequestUTI((osiEventCallback_t)CCED_RspCB, cmd);
            CFW_EmodOutfieldTestEnd(cmd->uti, nSim);
        }
        else if (cfw_event->nParam2 == CFW_TSM_NEIGHBOR_CELL)
        {
            nRet = CFW_GetCellInfo(&tCurrCellInf, &tNeighborCellInfo, nSim);
            if (nRet != ERR_SUCCESS)
            {
                OSI_LOGI(0, "NW_AsyncEvent NEIGHBOR_CELL ERROR:0x!%x\n", nRet);
                pCtx->replyErrCode = 1;
            }
            else
            {
                OSI_LOGI(0, "NW_AsyncEvent nTSM_NebCellNUM :%d!\n", tNeighborCellInfo.nTSM_NebCellNUM);
                bRet = AT_EMOD_CCED_ProcessData(nSim, NULL, &tNeighborCellInfo, pTxtBuf);
                if (bRet)
                {
                    atCmdRespInfoText(cmd->engine, pTxtBuf);
                    pCtx->replyErrCode = 0;
                }
                else
                {
                    pCtx->replyErrCode = 1;
                }
            }
            cmd->uti = cfwRequestUTI((osiEventCallback_t)CCED_RspCB, cmd);
            CFW_EmodOutfieldTestEnd(cmd->uti, nSim);
        }
        else if (cfw_event->nParam2 == CFW_TSM_PLOT_LTE)
        {
            nRet = CFW_GetLETRealTimeInfo(&tNetLTEinfo, nSim);
            if (nRet != ERR_SUCCESS)
            {
                OSI_LOGI(0, "NW_AsyncEvent CFW_GetNetLETInfo ERROR:0x%x !\n", nRet);
                pCtx->replyErrCode = 1;
            }
            else
            {
                _ccedProcessLTEData(&tNetLTEinfo, pTxtBuf, pCtx->lteReqType);
                atCmdRespInfoText(cmd->engine, pTxtBuf);
                pCtx->replyErrCode = 0;
            }
            cmd->uti = cfwRequestUTI((osiEventCallback_t)CCED_RspCB, cmd);
            CFW_EmodOutfieldTestEnd(cmd->uti, nSim);
        }
    }
    else if (cfw_event->nEventId == EV_CFW_TSM_INFO_END_RSP)
    {
        if (cfw_event->nType != 0)
        {
            if (ccedtimerV2 != NULL)
            {
                _ccedDeleteTimer(pCtx->lteReqType);
            }
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }
        else
        {
            if (pCtx->replyErrCode == 0)
            {
                if (ccedtimerV2 != NULL)
                {
                    osiTimerStart(ccedtimerV2, 5000);
                }
                atCmdRespOK(cmd->engine);
            }
            else
            {
                if (ccedtimerV2 != NULL)
                {
                    _ccedDeleteTimer(pCtx->lteReqType);
                }
                atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            }
        }
    }
    free(pTxtBuf);
}
/*
 *******************************************************************************
 *  possible input from ATM:                                                   *
 *  AT+cced=?   -->test, return +CCED: (0,1),(1,2,8)
                                OK                                             *
 *  AT+cced?    -->not support                                                 *
 *  AT+cced=X,X -->set                                                         *
 *******************************************************************************
*/
void atCmdHandleCCED(atCommand_t *cmd)
{
    int32_t iRet = 0;
    uint32_t nSim = atCmdGetSim(cmd->engine);
    char rsp[40] = {0};
    char *prsp = rsp;
    OSI_LOGI(0x100052d4, "AT+CCED:AT_NW_CmdFunc_CCED nSim==%d", nSim);
    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        if (cmd->param_count != 2)
        {
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }

        //nMode
        // 0: single
        // 1: auto shot
        // 2: stop auto shot
        uint8_t nMode = atParamUint(cmd->params[0], &paramok);
        if (!paramok || nMode > 2)
        {
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }

        //nRequestedDump:
        // 1: Current serving cell
        // 2: Neghborcell cell
        // 8: Get RSSI LTE&GSM
        uint8_t nRequestedDump = atParamUint(cmd->params[1], &paramok);
        if (!paramok || (nRequestedDump != 1 && nRequestedDump != 2 && nRequestedDump != 8))
        {
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }

        if ((nMode != 2) && ((ccedtimer != NULL) || (ccedtimerV2 != NULL)))
        {
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }

        if (nMode == 2)
        {
            paramok = _ccedDeleteTimer(nRequestedDump);
            if (paramok)
            {
                AT_CMD_RETURN(atCmdRespOK(cmd->engine));
            }
            else
            {
                uint32_t hAo = CFW_GetAoHandle(0, 0xFFFF, CFW_EMOD_SRV_ID, nSim);
                if (0 != hAo)
                {
                    ccedtimerV3 = osiTimerCreate(atEngineGetThreadId(), CCED_TimerV3CB, (void *)cmd);
                    osiTimerStart(ccedtimerV3, 1000);
                    RETURN_FOR_ASYNC();
                }
                else
                {
                    AT_CMD_RETURN(atCmdRespOK(cmd->engine));
                }
            }
        }

        uint8_t rat = CFW_NWGetStackRat(nSim);

        if (8 == nRequestedDump)
        {
            //Get RSSI LTE&GSM
            uint8_t nSignalLevel = 0;
            uint8_t nBitError = 0;
            if (nMode == 1)
            {
                _ccedCreatTimer(nRequestedDump, rat, nSim);
            }
            if (rat == CFW_RAT_LTE)
            {
                iRet = CFW_NwGetLteSignalQuality(&nSignalLevel, &nBitError, nSim);
                if (nSignalLevel < 17)
                {
                    nSignalLevel = 0;
                }
                else if ((17 <= nSignalLevel) && (nSignalLevel < 113))
                {
                    nSignalLevel = (uint8_t)((nSignalLevel - 17) / 3);
                }
                else
                {
                    nSignalLevel = 31;
                }
            }
            else
            {
                iRet = CFW_NwGetSignalQuality(&nSignalLevel, &nBitError, nSim);
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
            if (iRet != ERR_SUCCESS)
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));

            prsp += sprintf(prsp, "+CCED: %d,%d", nSignalLevel, nBitError);
            atCmdRespInfoText(cmd->engine, rsp);
            atCmdRespOK(cmd->engine);
        }
        else
        {
            //Get Cell info
            CFW_TSM_FUNCTION_SELECT tSelecFUN;
            memset(&tSelecFUN, 0, sizeof(CFW_TSM_FUNCTION_SELECT));
            if (1 == nRequestedDump)
            {
                tSelecFUN.nServingCell = 1;
            }
            else
            {
                tSelecFUN.nNeighborCell = 1;
            }
            tSelecFUN.nCurrentNetMode = rat;

            atCcedContext_t *ctx = (atCcedContext_t *)calloc(1, sizeof(*ctx));
            if (ctx == NULL)
            {
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);
            }
            ctx->lteReqType = nRequestedDump;
            ctx->replyErrCode = 0;
            cmd->async_ctx = ctx;
            cmd->async_ctx_destroy = atCommandAsyncCtxFree;

            cmd->uti = cfwRequestUTI((osiEventCallback_t)CCED_RspCB, cmd);
            if (nMode == 1)
            {
                _ccedCreatTimer(nRequestedDump, rat, nSim);
            }

            if (ERR_SUCCESS != (iRet = CFW_EmodOutfieldTestStart(&tSelecFUN, cmd->uti, nSim)))
            {
                OSI_LOGI(0x100048bf, "AT+CCED ERROR:iRet: %d, Emod", iRet);
                cfwReleaseUTI(cmd->uti);
                _ccedDeleteTimer(nRequestedDump);
                atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            }
        }
    }
    else if (cmd->type == AT_CMD_READ)
    {
        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED));
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        char AtTri[64];
        sprintf(AtTri, "+CCED: (0,1),(1,2,8)");
        atCmdRespInfoText(cmd->engine, AtTri);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED));
    }
}
/*
 *******************************************************************************
 *  possible input from ATM:                                                   *                                           *
 *  AT+GTCCINFO?    -->
 *  AT+GTCCINFO=?   -->not support                                                 *
 *  AT+GTCCINFO=X,X -->not support                                                 *
 *******************************************************************************
*/
typedef struct
{
    uint8_t currReqType;
    uint8_t currNetMode;
    uint8_t replyErrCode;
} atGtccinfoContext_t;
static const osiValueStrMap_t gGtccinfoStr[] = {
    {0, "n6"},
    {1, "n15"},
    {2, "n25"},
    {3, "n50"},
    {4, "n75"},
    {5, "n100"}};
void _atGtccinfoLTEProcData(CFW_NET_INFO_T *pNetinfo, atCommand_t *cmd)
{
    OSI_LOGI(0, "_atGtccinfoLTEProcData Entry");
    char nStr[100] = {0};
    char *pStr = NULL;
    char strTemp[5] = "NULL";

    pStr = &nStr[0];
    const char *str = osiVsmapFindStr(gGtccinfoStr, pNetinfo->nwCapabilityLte.lteScell.bandwidth, NULL);
    if (str == NULL)
    {
        str = strTemp;
    }

    if (pNetinfo->nwCapabilityLte.lteScell.imsi[0] != 0)
    {
        pStr += sprintf(pStr, "+GTCCINFO:LTE service cell:1,4,");

        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[0] & 0x0F) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[0] & 0x0F));
        }
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[0] >> 4) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[0] >> 4));
        }
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[1] & 0x0F) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[1] & 0x0F));
        }
        pStr += sprintf(pStr, ",");
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[2] & 0x0F) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[2] & 0x0F));
        }
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[2] >> 4) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[2] >> 4 & 0x0F));
        }
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[1] >> 4) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteScell.plmn[1] >> 4 & 0x0F));
        }
        sprintf(pStr, ",%d,%ld,%ld,%d,%d,%s,%d,%d,%d,%d", pNetinfo->nwCapabilityLte.lteScell.tac,
                ((pNetinfo->nwCapabilityLte.lteScell.enodeBID << 8) | (pNetinfo->nwCapabilityLte.lteScell.cellId)),
                pNetinfo->nwCapabilityLte.lteScell.dlEarfcn,
                pNetinfo->nwCapabilityLte.lteScell.pcid,
                pNetinfo->nwCapabilityLte.lteScell.bandInfo,
                str,
                pNetinfo->nwCapabilityLte.lteScell.SINR,
                pNetinfo->nwCapabilityLte.lteScell.Srxlev,
                pNetinfo->nwCapabilityLte.lteScell.rsrp,
                pNetinfo->nwCapabilityLte.lteScell.rsrq);
    }
    else
        sprintf(pStr, "+GTCCINFO:LTE service cell:1,4,%x%x%x,,%d,%ld,%ld,%d,%d,%s,%d,%d,%d,%d",
                pNetinfo->nwCapabilityLte.lteScell.plmn[0],
                pNetinfo->nwCapabilityLte.lteScell.plmn[1],
                pNetinfo->nwCapabilityLte.lteScell.plmn[2],
                pNetinfo->nwCapabilityLte.lteScell.tac,
                ((pNetinfo->nwCapabilityLte.lteScell.enodeBID << 8) | (pNetinfo->nwCapabilityLte.lteScell.cellId)),
                pNetinfo->nwCapabilityLte.lteScell.dlEarfcn,
                pNetinfo->nwCapabilityLte.lteScell.pcid,
                pNetinfo->nwCapabilityLte.lteScell.bandInfo,
                str,
                pNetinfo->nwCapabilityLte.lteScell.SINR,
                pNetinfo->nwCapabilityLte.lteScell.Srxlev,
                pNetinfo->nwCapabilityLte.lteScell.rsrp,
                pNetinfo->nwCapabilityLte.lteScell.rsrq);
    atCmdRespInfoText(cmd->engine, nStr);

    int j = 0;
    memset(nStr, 0, sizeof(nStr));

    if (pNetinfo->nwCapabilityLte.lteNcellNum > 9)
    {
        pNetinfo->nwCapabilityLte.lteNcellNum = 9;
    }
    for (j = 0; j < pNetinfo->nwCapabilityLte.lteNcellNum; j++)
    {
        pStr = &nStr[0];
        pStr += sprintf(pStr, "+GTCCINFO:LTE neighbor cell:2,4,");
        if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[0] & 0x0F) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[0] & 0x0F));
        }
        if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[0] >> 4) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[0] >> 4));
        }
        if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[1] & 0x0F) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[1] & 0x0F));
        }
        pStr += sprintf(pStr, ",");
        if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[2] & 0x0F) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[2] & 0x0F));
        }
        if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[2] >> 4) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[2] >> 4 & 0x0F));
        }
        if ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[1] >> 4) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.plmn[1] >> 4 & 0x0F));
        }
        str = osiVsmapFindStr(gGtccinfoStr, pNetinfo->nwCapabilityLte.lteNcell[j].bandWidth, NULL);
        if (str == NULL)
        {
            str = strTemp;
        }
        pStr += sprintf(pStr, ",%d,%ld,%ld,%d,%s,%d,%d,%d",
                        pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.tac,
                        ((pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.enodeBID << 8) | (pNetinfo->nwCapabilityLte.lteNcell[j].cellAccRelInfo.cellId)),
                        pNetinfo->nwCapabilityLte.lteNcell[j].frequency, //sure
                        pNetinfo->nwCapabilityLte.lteNcell[j].pcid,
                        str,
                        pNetinfo->nwCapabilityLte.lteNcell[j].Srxlev,
                        pNetinfo->nwCapabilityLte.lteNcell[j].rsrp,
                        pNetinfo->nwCapabilityLte.lteNcell[j].rsrq);
        atCmdRespInfoText(cmd->engine, nStr);
        memset(nStr, 0, sizeof(nStr));
    }
    if (j == 0)
    {
        sprintf(nStr, "+GTCCINFO:LTE neighbor cell:0,0,0,0,0,0,0,0,0");
        atCmdRespInfoText(cmd->engine, nStr);
    }
}
bool atGtccinfoGSMProcData(CFW_TSM_CURR_CELL_INFO *pCurrCellInfo,
                           CFW_TSM_ALL_NEBCELL_INFO *pNeighborCellInfo, atCommand_t *cmd)
{
    uint8_t i = 0;
    char nStr[100] = {0};
    char *pStr = NULL;

    if ((NULL == pCurrCellInfo) && (NULL == pNeighborCellInfo))
    {
        return false;
    }
    pStr = &nStr[0];
    if (pCurrCellInfo != NULL)
    {
        OSI_LOGI(0, "pCurrCellInfo != NULL");

        pStr += sprintf(pStr, "+GTCCINFO:GSM service cell:1,2,");
        if ((pCurrCellInfo->nTSM_LAI[0] & 0x0F) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pCurrCellInfo->nTSM_LAI[0] & 0x0F));
        }
        if ((pCurrCellInfo->nTSM_LAI[0] >> 4) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pCurrCellInfo->nTSM_LAI[0] >> 4));
        }
        if ((pCurrCellInfo->nTSM_LAI[1] & 0x0F) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pCurrCellInfo->nTSM_LAI[1] & 0x0F));
        }
        pStr += sprintf(pStr, ",");
        if ((pCurrCellInfo->nTSM_LAI[2] & 0x0F) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pCurrCellInfo->nTSM_LAI[2] & 0x0F));
        }
        if ((pCurrCellInfo->nTSM_LAI[2] >> 4) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pCurrCellInfo->nTSM_LAI[2] >> 4));
        }
        if ((pCurrCellInfo->nTSM_LAI[1] >> 4) != 0x0F)
        {
            pStr += sprintf(pStr, "%x", (pCurrCellInfo->nTSM_LAI[1] >> 4));
        }

        pStr += sprintf(pStr, ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                        ((pCurrCellInfo->nTSM_LAI[3] << 8) | (pCurrCellInfo->nTSM_LAI[4])),
                        ((pCurrCellInfo->nTSM_CellID[0] << 8) | (pCurrCellInfo->nTSM_CellID[1])),
                        pCurrCellInfo->nTSM_Arfcn, pCurrCellInfo->nTSM_Bsic,
                        pCurrCellInfo->nTSM_CurrBand, pCurrCellInfo->nTSM_TxDbm,
                        pCurrCellInfo->nTSM_Dtx_used, pCurrCellInfo->nTSM_C1Value,
                        pCurrCellInfo->nTSM_C2, 0, pCurrCellInfo->nTSM_BsPaMfrms,
                        pCurrCellInfo->nTSM_Maio, pCurrCellInfo->nTSM_Hsn,
                        pCurrCellInfo->nTSM_RxLevSub, pCurrCellInfo->nTSM_RxLevFull,
                        pCurrCellInfo->nTSM_RxQualSub, pCurrCellInfo->nTSM_RxQualFull,
                        pCurrCellInfo->nTSM_RxLev, pCurrCellInfo->nTSM_AvRxLevel, pCurrCellInfo->nTSM_BER);
        atCmdRespInfoText(cmd->engine, nStr);
    }

    memset(nStr, 0, sizeof(nStr));
    if (pNeighborCellInfo != NULL)
    {
        OSI_LOGI(0, "pNeighborCellInfo != Null");
        if (pNeighborCellInfo->nTSM_NebCellNUM > 10)
        {
            pNeighborCellInfo->nTSM_NebCellNUM = 10;
        }
        for (i = 0; i < pNeighborCellInfo->nTSM_NebCellNUM; i++)
        {
            if (!((pNeighborCellInfo->nTSM_NebCell[i].nTSM_CellID[0] == 0) && (pNeighborCellInfo->nTSM_NebCell[i].nTSM_CellID[1] == 0)))
            {
                pStr = &nStr[0];
                pStr += sprintf(pStr, "+GTCCINFO:GSM neighbor cell:1,2,");
                if ((pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[0] & 0x0F) != 0x0F)
                {
                    pStr += sprintf(pStr, "%x", (pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[0] & 0x0F));
                }
                if ((pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[0] >> 4) != 0x0F)
                {
                    pStr += sprintf(pStr, "%x", (pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[0] >> 4));
                }
                if ((pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[1] & 0x0F) != 0x0F)
                {
                    pStr += sprintf(pStr, "%x", (pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[1] & 0x0F));
                }
                pStr += sprintf(pStr, ",");
                if ((pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[2] & 0x0F) != 0x0F)
                {
                    pStr += sprintf(pStr, "%x", (pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[2] & 0x0F));
                }
                if ((pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[2] >> 4) != 0x0F)
                {
                    pStr += sprintf(pStr, "%x", (pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[2] >> 4));
                }
                if ((pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[1] >> 4) != 0x0F)
                {
                    pStr += sprintf(pStr, "%x", (pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[1] >> 4));
                }
                pStr += sprintf(pStr, ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                                ((pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[3] << 8) | (pNeighborCellInfo->nTSM_NebCell[i].nTSM_LAI[4])),
                                ((pNeighborCellInfo->nTSM_NebCell[i].nTSM_CellID[0] << 8) | (pNeighborCellInfo->nTSM_NebCell[i].nTSM_CellID[1])),
                                pNeighborCellInfo->nTSM_NebCell[i].nTSM_Arfcn,
                                pNeighborCellInfo->nTSM_NebCell[i].nTSM_Bsic,
                                pNeighborCellInfo->nTSM_NebCell[i].nTSM_C1Value,
                                pNeighborCellInfo->nTSM_NebCell[i].nTSM_C2,
                                pNeighborCellInfo->nTSM_NebCell[i].nTSM_C31,
                                pNeighborCellInfo->nTSM_NebCell[i].nTSM_C32,
                                (pNeighborCellInfo->nTSM_NebCell[i].nTSM_AvRxLevel),
                                pNeighborCellInfo->nTSM_NebCell[i].nTSM_AvRxLevel);
                atCmdRespInfoText(cmd->engine, nStr);
                memset(nStr, 0, sizeof(nStr));
            }
        }
        if (i == 0)
        {
            pStr += sprintf(pStr, "+GTCCINFO:GSM neighbor cell:1,2,0,0,0,0,0,0,0,0,0,0,0,0\r\n");
            atCmdRespInfoText(cmd->engine, nStr);
        }
    }
    return true;
}
static void GTCCINFO_RspCB(atCommand_t *cmd, const osiEvent_t *event)
{
    OSI_LOGI(0, "GTCCINFO_RspCB------");
    uint32_t nRet = 0;
    bool bRet = false;
    CFW_TSM_CURR_CELL_INFO tCurrCellInf;
    CFW_TSM_ALL_NEBCELL_INFO tNeighborCellInfo;
    CFW_NET_INFO_T tNetLTEinfo;

    memset(&tCurrCellInf, 0, sizeof(CFW_TSM_CURR_CELL_INFO));
    memset(&tNeighborCellInfo, 0, sizeof(CFW_TSM_ALL_NEBCELL_INFO));
    memset(&tNetLTEinfo, 0, sizeof(CFW_NET_INFO_T));

    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_SIM_ID nSim = (CFW_SIM_ID)cfw_event->nFlag;
    atGtccinfoContext_t *pCtx = (atGtccinfoContext_t *)cmd->async_ctx;

    if (cfw_event->nEventId == EV_CFW_TSM_INFO_IND)
    {
        if (cfw_event->nType != 0)
        {
            OSI_LOGI(0, "CCED_RspCB: ERROR!!!---CfwEvent.nType --0x%x\n\r", cfw_event->nType);
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
        }
        if (cfw_event->nParam2 == CFW_TSM_CURRENT_CELL)
        {
            nRet = CFW_GetCellInfo(&tCurrCellInf, &tNeighborCellInfo, nSim);
            if (nRet != ERR_SUCCESS)
            {
                OSI_LOGI(0, "NW_AsyncEvent ERROR nRet:0x%x!\n", nRet);
                pCtx->replyErrCode = 1;
            }
            else
            {
                bRet = atGtccinfoGSMProcData(&tCurrCellInf, NULL, cmd);
                if (bRet)
                {
                    pCtx->replyErrCode = 0;
                }
                else
                {
                    pCtx->replyErrCode = 1;
                }
            }
            cmd->uti = cfwRequestUTI((osiEventCallback_t)GTCCINFO_RspCB, cmd);
            CFW_EmodOutfieldTestEnd(cmd->uti, nSim);
        }
        else if (cfw_event->nParam2 == CFW_TSM_NEIGHBOR_CELL)
        {
            OSI_LOGI(0, "tNeighborCellInfo--%d", sizeof(CFW_TSM_ALL_NEBCELL_INFO));
            OSI_LOGI(0, "tNeighborCellInfo--0x%x", &tNeighborCellInfo);
            nRet = CFW_GetCellInfo(&tCurrCellInf, &tNeighborCellInfo, nSim);
            if (nRet != ERR_SUCCESS)
            {
                OSI_LOGI(0, "NW_AsyncEvent NEIGHBOR_CELL ERROR:0x!%x\n", nRet);
                pCtx->replyErrCode = 1;
            }
            else
            {
                OSI_LOGI(0, "NW_AsyncEvent nTSM_NebCellNUM :%d!\n", tNeighborCellInfo.nTSM_NebCellNUM);
                bRet = atGtccinfoGSMProcData(NULL, &tNeighborCellInfo, cmd);
                if (bRet)
                {
                    pCtx->replyErrCode = 0;
                }
                else
                {
                    pCtx->replyErrCode = 1;
                }
            }
            cmd->uti = cfwRequestUTI((osiEventCallback_t)GTCCINFO_RspCB, cmd);
            CFW_EmodOutfieldTestEnd(cmd->uti, nSim);
        }
        else if (cfw_event->nParam2 == CFW_TSM_PLOT_LTE)
        {
            OSI_LOGI(0, "tNetLTEinfo--%d, %x", sizeof(CFW_NET_INFO_T), &tNetLTEinfo);
            nRet = CFW_GetLETRealTimeInfo(&tNetLTEinfo, nSim);
            if (nRet != ERR_SUCCESS)
            {
                OSI_LOGI(0, "NW_AsyncEvent CFW_GetNetLETInfo ERROR:0x%x !\n", nRet);
                pCtx->replyErrCode = 1;
            }
            else
            {
                _atGtccinfoLTEProcData(&tNetLTEinfo, cmd);
                pCtx->replyErrCode = 0;
            }
            cmd->uti = cfwRequestUTI((osiEventCallback_t)GTCCINFO_RspCB, cmd);
            CFW_EmodOutfieldTestEnd(cmd->uti, nSim);
        }
    }
    else if (cfw_event->nEventId == EV_CFW_TSM_INFO_END_RSP)
    {
        if ((pCtx->currNetMode == CFW_RAT_GSM_ONLY) && (pCtx->currReqType == 1))
        {
            CFW_TSM_FUNCTION_SELECT tSelecFUN;
            memset(&tSelecFUN, 0, sizeof(CFW_TSM_FUNCTION_SELECT));

            tSelecFUN.nNeighborCell = 1;
            tSelecFUN.nCurrentNetMode = CFW_RAT_GSM_ONLY;
            pCtx->currReqType = 2;
            cmd->uti = cfwRequestUTI((osiEventCallback_t)GTCCINFO_RspCB, cmd);
            if (0 != CFW_EmodOutfieldTestStart(&tSelecFUN, cmd->uti, nSim))
            {
                cfwReleaseUTI(cmd->uti);
                atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            }
        }
        else
        {
            if (cfw_event->nType != 0)
            {
                atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            }
            else
            {
                if (pCtx->replyErrCode == 0)
                {
                    atCmdRespOK(cmd->engine);
                }
                else
                {
                    atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
                }
            }
        }
    }
}
void atCmdHandleGTCCINFO(atCommand_t *cmd)
{
    uint32_t nSim = atCmdGetSim(cmd->engine);

    if (cmd->type == AT_CMD_READ)
    {
        uint8_t rat = CFW_NWGetStackRat(nSim);

        CFW_TSM_FUNCTION_SELECT tSelecFUN;
        memset(&tSelecFUN, 0, sizeof(CFW_TSM_FUNCTION_SELECT));

        tSelecFUN.nCurrentNetMode = rat;
        if (rat == CFW_RAT_LTE_ONLY)
        {
            tSelecFUN.nNeighborCell = true;
        }
        else
        {
            tSelecFUN.nServingCell = true;
        }
        atGtccinfoContext_t *ctx = (atGtccinfoContext_t *)calloc(1, sizeof(*ctx));
        if (ctx == NULL)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);
        }
        ctx->currReqType = 1;
        ctx->currNetMode = rat;
        ctx->replyErrCode = 0;
        cmd->async_ctx = ctx;
        cmd->async_ctx_destroy = atCommandAsyncCtxFree;

        cmd->uti = cfwRequestUTI((osiEventCallback_t)GTCCINFO_RspCB, cmd);
        if (0 != CFW_EmodOutfieldTestStart(&tSelecFUN, cmd->uti, nSim))
        {
            OSI_LOGI(0, "AT+GTCCINFO ERROR: Emod");
            cfwReleaseUTI(cmd->uti);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_SUPPORTED));
    }
}
int32_t at_StrCaselessCompare(
    const char *stringa,
    const char *stringb)
{
    register int8_t __res = 0;

    for (;;)
    {
        if (((*stringa - *stringb) != 32) && ((*stringa - *stringb) != -32) && ((*stringa - *stringb) != 0))
        {
            __res = (char)(*stringa - *stringb);
            break;
        }
        if (!*stringa++ || !*stringb++)
        {
            break;
        }
    }

    return __res;
}
static void _setlockSetCommEnableRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    CFW_EVENT cfwEvent = *(const CFW_EVENT *)event;
    OSI_LOGI(0, "_setlockSetCommEnableRsp\n\r");
    if (cfwEvent.nType != 0)
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

    atCmdRespOK(cmd->engine);
}
static void _setlockSetCommRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    CFW_EVENT cfwEvent = *(const CFW_EVENT *)event;
    uint8_t nSim = atCmdGetSim(cmd->engine);
    OSI_LOGI(0, "_setlockSetCommRsp\n\r");
    if (cfwEvent.nType != 0)
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
    CFW_COMM_MODE nFM = CFW_DISABLE_COMM;
    CFW_GetComm(&nFM, nSim);
    OSI_LOGI(0, "_setlockSetCommRsp %d\n\r", nFM);
    if (CFW_ENABLE_COMM != nFM)
    {
        cmd->uti = cfwRequestUTI((osiEventCallback_t)_setlockSetCommEnableRsp, cmd);
        atSetFlightModeFlag(AT_ENABLE_COMM, nSim);
        if (0 != CFW_SetComm(CFW_ENABLE_COMM, 0, cmd->uti, nSim))
        {
            cfwReleaseUTI(cmd->uti);
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }
    }
    else
        atCmdRespOK(cmd->engine);
    RETURN_FOR_ASYNC();
}

void atCmdHandleSETLOCK(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    char rsp[80] = {0};
    char *prsp = rsp;

    if (cmd->type == AT_CMD_SET)
    {
        uint8_t number = 0;
        uint16_t band[20] = {0};
        uint8_t i = 0, n = 0;
        uint32_t freq[20] = {0};
        bool paramok = true;
        uint16_t Cell;
        uint8_t tddfdd;
        if (cmd->param_count < 2)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        uint8_t nOper = atParamUintInRange(cmd->params[0], 0, 2, &paramok);
        uint8_t nType = atParamUintInRange(cmd->params[1], 0, 3, &paramok);
        if (!paramok)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        if (nOper == 2)
        {
            if (cmd->param_count > 2)
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            switch (nType)
            {
            case 0:
            {
                CFW_EmodGetLteLockBandV2(&number, band, nSim);
                prsp += sprintf(prsp, "+SETLOCK: %d", number);
                for (i = 0; i < number; i++)
                {
                    prsp += sprintf(prsp, ",%d", band[i]);
                }
            }
            break;
            case 1:
            {
                CFW_EmodGetLteLockFreq(&number, freq, nSim);
                prsp += sprintf(prsp, "+SETLOCK: %d", number);
                for (i = 0; i < number; i++)
                {
                    prsp += sprintf(prsp, ",%ld", freq[i]);
                }
            }
            break;
            case 2:
            {
                CFW_EmodGetLteLockCell(&Cell, nSim);
                if (65535 == Cell)
                    prsp += sprintf(prsp, "+SETLOCK: Cell Unlock");
                else
                    prsp += sprintf(prsp, "+SETLOCK: %d", Cell);
            }
            break;
            case 3:
            {
                CFW_EmodGetLteLockTddOrFdd(&tddfdd, nSim);
                if (tddfdd == 1)
                    prsp += sprintf(prsp, "+SETLOCK: TDD");
                else if (tddfdd == 2)
                    prsp += sprintf(prsp, "+SETLOCK: FDD");
                else
                    prsp += sprintf(prsp, "+SETLOCK: FDD/TDD Unlock");
            }
            break;
            default:
            {
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            }
            break;
            }
            atCmdRespInfoText(cmd->engine, rsp);
            atCmdRespOK(cmd->engine);
        }
        else if (nOper == 1)
        {
            CFW_COMM_MODE nFM = CFW_DISABLE_COMM;
            switch (nType)
            {
            case 0:
            {
                uint16_t checkBand[20] = {0};
                bool checkFlag = false;
                if (cmd->param_count > 7)
                    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                CFW_EmodGetLteLockFreq(&number, freq, nSim);
                if (number > 0)
                {
                    for (i = 0; i < number; i++)
                    {
                        checkBand[i] = CFW_GetBandFromFreq(freq[i]);
                    }
                }
                for (n = 0; n < cmd->param_count - 2; n++)
                {
                    band[n] = atParamUint(cmd->params[n + 2], &paramok);
                    if ((!paramok) || (!CFW_CheckSupportLteBand(band[n], nSim)))
                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                    if ((number > 0) && (false == checkFlag))
                    {
                        for (i = 0; i < number; i++)
                        {
                            if (band[n] == checkBand[i])
                            {
                                checkFlag = true;
                                break;
                            }
                        }
                    }
                }
                if ((number > 0) && (false == checkFlag))
                {
                    OSI_LOGI(0, "=======SETLOCK CFW_GetBandFromFreq number is %d", number);
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                }

                number = n;

                if (0 != CFW_GetComm(&nFM, nSim))
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

                OSI_LOGI(0, "=======SETLOCK CFW_EmodSetLteLockBand number is %d", number);
                CFW_EmodSetLteLockBandV2(number, band, nSim);
            }
            break;
            case 1:
            {
                if (cmd->param_count > 11)
                    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));

                for (n = 0; n < cmd->param_count - 2; n++)
                {
                    freq[n] = atParamUint(cmd->params[n + 2], &paramok);
                    if (!CFW_CheckSupportLteFreq(freq[n], 0, nSim))
                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                }
                if (!paramok)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                number = n;
                Cell = 0xffff;

                if (0 != CFW_GetComm(&nFM, nSim))
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
                OSI_LOGI(0, "=======SETLOCK CFW_EmodSetLteLockFreq number is %d", number);
                CFW_EmodSetLteLockFreq(number, freq, nSim);
                //for clean cell
                CFW_EmodSetLteLockCell(Cell, nSim);
            }
            break;
            case 2:
            {
                if (cmd->param_count > 4)
                    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));

                uint32_t freqcell = atParamUint(cmd->params[2], &paramok);
                Cell = atParamUintInRange(cmd->params[3], 0, 503, &paramok);
                //support cell range:0~503
                if ((!paramok) || (!CFW_CheckSupportLteFreq(freqcell, 0, nSim)))
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

                if (0 != CFW_GetComm(&nFM, nSim))
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
                OSI_LOGI(0, "=======SETLOCK CFW_EmodSetLteLockCell Cell is %d", Cell);
                CFW_EmodSetLteLockFreq(1, &freqcell, nSim);
                CFW_EmodSetLteLockCell(Cell, nSim);
            }
            break;
            case 3:
            {
                if (cmd->param_count > 3)
                    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                const char *mode = atParamStr(cmd->params[2], &paramok);
                if (!paramok)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                if (strlen(mode) != 3)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

                if ((at_StrCaselessCompare(mode, "TDD") == 0))
                    tddfdd = 1;
                else if ((at_StrCaselessCompare(mode, "FDD") == 0))
                    tddfdd = 2;
                else
                    tddfdd = 0;

                if (0 != CFW_GetComm(&nFM, nSim))
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

                CFW_EmodSetLteLockTddOrFdd(tddfdd, nSim);
            }
            break;
            default:
            {
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            }
            break;
            }
            if (CFW_DISABLE_COMM == nFM)
                RETURN_OK(cmd->engine);
            else
            {
                cmd->uti = cfwRequestUTI((osiEventCallback_t)_setlockSetCommRsp, cmd);
                atSetFlightModeFlag(AT_DISABLE_COMM, nSim);
                if (0 != CFW_SetComm(CFW_DISABLE_COMM, 0, cmd->uti, nSim))
                {
                    cfwReleaseUTI(cmd->uti);
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
                }
                RETURN_FOR_ASYNC();
            }
        }
        else if (nOper == 0)
        {
            if (cmd->param_count > 2)
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            switch (nType)
            {
            case 1:
            {
                OSI_LOGI(0, "=======UNLOCK cell and freq");
                CFW_EmodSetLteLockFreq(0, NULL, nSim);
                CFW_EmodSetLteLockCell(65535, nSim);
                break;
            }
            case 2:
            {
                OSI_LOGI(0, "=======UNLOCK cell !!!");
                CFW_EmodSetLteLockCell(65535, nSim);
                CFW_EmodSetLteLockFreq(0, NULL, nSim);
                break;
            }
            case 0:
            {
                OSI_LOGI(0, "=======UNLOCK band");
                CFW_EmodSetLteLockBandV2(0, NULL, nSim);
            }
            break;
            case 3:
            {
                OSI_LOGI(0, "=======UNLOCK TDD/FDD");
                CFW_EmodSetLteLockTddOrFdd(0, nSim);
            }
            break;
            default:
            {
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            }
            break;
            }
            atCmdRespOK(cmd->engine);
        }
        else
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        AT_CMD_RETURN(atCmdRespOK(cmd->engine));
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
    }
}

void atCmdHandleMYBAND(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint8_t nBand = 0;
    uint32_t ret = ERR_SUCCESS;

#ifdef CONFIG_SOC_6760
    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_SUPPORTED);
#endif

    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint8_t index = 0;
        bool checkFlag = false;
        uint8_t nMyband = atParamDefUintInRange(cmd->params[0], 0, 0, 2, &paramok);

        if (!paramok || cmd->param_count != 1)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));

        if (1 == nMyband)
        {
            nBand = CSW_GSM_900E;
        }
        else if (2 == nMyband)
        {
            nBand = CSW_DCS_1800;
        }
        else
        {
            nBand = CSW_GSM_900P | CSW_GSM_900E | CSW_GSM_850 | CSW_DCS_1800 | CSW_PCS_1900;
        }

        if (1 == gAtSetting.bcchmode)
        {
            CFW_StoredPlmnList nStorePlmnList;
            memset(&nStorePlmnList, 0x00, sizeof(CFW_StoredPlmnList));
            ret = CFW_CfgGetStoredPlmnList(&nStorePlmnList, nSim);
            if (ret != ERR_SUCCESS)
            {
                OSI_LOGI(0, "MYBAND:Set store list error %d\n\r", ret);
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
            }
            if (nStorePlmnList.nCount > 0)
            {
                for (index = 0; index < nStorePlmnList.nCount; index++)
                {
                    if ((1 == nMyband) && (nStorePlmnList.ArfcnL[index] >= 975) && (nStorePlmnList.ArfcnL[index] <= 1023))
                    {
                        checkFlag = true;
                        break;
                    }
                    else if ((2 == nMyband) && (nStorePlmnList.ArfcnL[index] >= 512) && (nStorePlmnList.ArfcnL[index] <= 885))
                    {
                        checkFlag = true;
                        break;
                    }
                    else if (0 == nMyband)
                    {
                        if ((nStorePlmnList.ArfcnL[index] <= 124))
                            checkFlag = true;
                        else if ((nStorePlmnList.ArfcnL[index] >= 128) && (nStorePlmnList.ArfcnL[index] <= 251))
                            checkFlag = true;
                        else if ((nStorePlmnList.ArfcnL[index] >= 512) && (nStorePlmnList.ArfcnL[index] <= 885))
                            checkFlag = true;
                        else if ((nStorePlmnList.ArfcnL[index] >= 975) && (nStorePlmnList.ArfcnL[index] <= 1023))
                            checkFlag = true;

                        if (true == checkFlag)
                            break;
                    }
                }
            }
            if (false == checkFlag)
            {
                OSI_LOGI(0, "Warring:BAND and FREQ do not match!\n\r");
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
            }
        }
        ret = CFW_CfgNwSetFrequencyBand(nBand, nSim);
        if (ERR_SUCCESS != ret)
        {
            OSI_LOGI(0, "AT+MYBAND:Set band error %d\n\r", ret);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            return;
        }
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ)
    {
        char rsp[40] = {
            0,
        };
        ret = CFW_CfgNwGetFrequencyBand(&nBand, nSim);
        if (ERR_SUCCESS != ret)
        {
            OSI_LOGI(0, "AT+MYBAND:Get band error %d\n\r", ret);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            return;
        }
        if ((nBand & 0xff) == CSW_GSM_900E)
        {
            nBand = 1;
        }
        else if ((nBand & 0xff) == CSW_DCS_1800)
        {
            nBand = 2;
        }
        else
        {
            nBand = 0;
        }
        sprintf(rsp, "+SETBAND: %d", nBand);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmsError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

void atCmdHandleMYSYSINFO(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint32_t ret = ERR_SUCCESS;

    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint8_t nSysMode = atParamDefUintInRange(cmd->params[0], 1, 1, 7, &paramok);

        if (!paramok || cmd->param_count != 1)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));

        //We need do something
        if (ERR_SUCCESS != ret)
        {
            OSI_LOGI(0, "MYSYSINFO:Set mode error %d,%d\n\r", ret, nSysMode);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            return;
        }
        atCmdRespOK(cmd->engine);
    }
    else if ((cmd->type == AT_CMD_READ) || (cmd->type == AT_CMD_EXE))
    {
        //We need do something
        uint8_t nOperatorId[7] = {
            0,
        };
        char nMnc[4] = {
            0,
        };
        uint8_t nMode = 0;
        uint8_t nRat = 0;
        char rsp[40] = {
            0,
        };

        ret = CFW_NwGetCurrentOperator(nOperatorId, &nMode, nSim);
        if (ERR_SUCCESS != ret)
        {
            OSI_LOGI(0, "MYSYSINFO:get operator error %d\n\r", ret);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            return;
        }
        if ((0 == nOperatorId[0]) && (0 == nOperatorId[1]))
        {
            strcpy(nMnc, "00");
        }
        else if ((0 == nOperatorId[3]) && (0 == nOperatorId[4]))
        {
            strcpy(nMnc, "01");
        }
        else if ((0 == nOperatorId[3]) && (1 == nOperatorId[4]))
        {
            strcpy(nMnc, "02");
        }
        else if ((0 == nOperatorId[3]) && (3 == nOperatorId[4]))
        {
            strcpy(nMnc, "03");
        }
        else if (7 == nOperatorId[3])
        {
            strcpy(nMnc, "754");
        }
        else if (8 == nOperatorId[3])
        {
            strcpy(nMnc, "854");
        }
        else if (9 == nOperatorId[3])
        {
            strcpy(nMnc, "954");
        }
        else
        {
            OSI_LOGI(0, "MYSYSINFO:need add new mnc str error\n\r");
        }
        nRat = CFW_NWGetStackRat(nSim);

        sprintf(rsp, "$MYSYSINFO: %d,", nRat);
        strcat(rsp, nMnc);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        atCmdRespInfoText(cmd->engine, "$MYSYSINFO:(1-7)");
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmsError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}
uint8_t gXact_mode = 0;
void atCmdHandleMYSYSMODE(atCommand_t *cmd)
{
    uint32_t ret = ERR_SUCCESS;

    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint8_t nActMode = atParamDefUintInRange(cmd->params[0], 0, 0, 4, &paramok);

        if (!paramok || cmd->param_count != 1)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));

        //Maybe be We need do something
        gXact_mode = nActMode;
        if (ERR_SUCCESS != ret)
        {
            OSI_LOGI(0, "AT+MYBAND:Set band error %d\n\r", ret);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            return;
        }
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ)
    {
        //We need do something
        char rsp[40] = {
            0,
        };
        sprintf(rsp, "$MYSYSMODE: %d", gXact_mode);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        atCmdRespInfoText(cmd->engine, "$MYSYSMODE:(0,2,3,4)");
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmsError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

#if defined(LTE_NBIOT_SUPPORT) || defined(LTE_SUPPORT)
static void _atPlmnApi2Array(uint8_t *pLAI, uint8_t *nOperatorId)
{
    nOperatorId[0] = pLAI[0] & 0x0F;
    nOperatorId[1] = (pLAI[0] & 0xF0) >> 4;
    nOperatorId[2] = pLAI[1] & 0x0F;
    nOperatorId[3] = pLAI[2] & 0x0F;
    nOperatorId[4] = (pLAI[2] & 0xF0) >> 4;
    nOperatorId[5] = (pLAI[1] & 0xF0) >> 4;
}

static void _qscanfExeRegRspTimeOutCB(atCommand_t *cmd)
{
    cfwReleaseUTI(cmd->uti);
    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_SEND_TIMEOUT));
}
static void _qscanfGetFreqScanRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    NW_FREQ_SCAN_INFO *pScanInfo = NULL;
    //unsigned char nOprNam[10] = {0,};

    if (0 == cfw_event->nType)
    {
        uint8_t *pPlmn = NULL;
        uint16_t nTac = 0;
        uint8_t nOperatorId[6] = {
            0,
        };
        char *rsp = malloc(1300);
        char *prsp = rsp;
        if (rsp == NULL)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_MEMORY_FAILURE);
        memset(rsp, 0, 1300);
        pScanInfo = (NW_FREQ_SCAN_INFO *)cfw_event->nParam1;
        prsp += sprintf(prsp, "+QSCANF: ");
        for (int i = 0; i < pScanInfo->nPlmnNb; i++)
        {
            pPlmn = &(pScanInfo->nPlmnL[3 * i]);
            memset(&nOperatorId, 0, 6 * sizeof(uint8_t));
            _atPlmnApi2Array(pPlmn, nOperatorId);
            nTac = (pScanInfo->nCellInfo[i].nTac[0] << 8) | (pScanInfo->nCellInfo[i].nTac[1]);
            if (nOperatorId[5] <= 9)
            {
                prsp += sprintf(prsp, "\n%d,%ld,%d%d%x,%d%d%d%d%d%x,%d%d%d,%d,%d,%d,%d,%d", pScanInfo->nCellInfo[i].nBand, pScanInfo->nCellInfo[i].nArfcn,
                                nOperatorId[3], nOperatorId[4], nOperatorId[5],
                                nOperatorId[0], nOperatorId[1], nOperatorId[2], nOperatorId[3], nOperatorId[4], nOperatorId[5],
                                nOperatorId[0], nOperatorId[1], nOperatorId[2],
                                nTac, pScanInfo->nCellInfo[i].nPci, pScanInfo->nCellInfo[i].nRSRP,
                                pScanInfo->nCellInfo[i].nRSRQ, pScanInfo->nCellInfo[i].nRSSI);
            }
            else
            {
                prsp += sprintf(prsp, "\n%d,%ld,%d%d,%d%d%d%d%d,%d%d%d,%d,%d,%d,%d,%d", pScanInfo->nCellInfo[i].nBand, pScanInfo->nCellInfo[i].nArfcn,
                                nOperatorId[3], nOperatorId[4],
                                nOperatorId[0], nOperatorId[1], nOperatorId[2], nOperatorId[3], nOperatorId[4],
                                nOperatorId[0], nOperatorId[1], nOperatorId[2],
                                nTac, pScanInfo->nCellInfo[i].nPci, pScanInfo->nCellInfo[i].nRSRP,
                                pScanInfo->nCellInfo[i].nRSRQ, pScanInfo->nCellInfo[i].nRSSI);
            }
        }
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
        if (rsp != NULL)
            free(rsp);
        return;
    }
    else
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
}

// freq scan +QSCANF

void atCmdHandleQSCANF(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint32_t res;
#if 0
    if (API_RAT_LTE != CFW_NWGetStackRat(nSim))
    {
        OSI_LOGI(0, "QSCANF:Just do it in Rat lte\n\r");
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
#endif
    if (cmd->type == AT_CMD_EXE)
    {
        CFW_COMM_MODE nFM = CFW_ENABLE_COMM;
        if (ERR_SUCCESS != CFW_GetComm(&nFM, nSim))
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
        }
        if (CFW_ENABLE_COMM == nFM)
        {
            OSI_LOGI(0, "QSCANF:Just do it in flght mode\n\r");
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
        }

        cmd->uti = cfwRequestUTI((osiEventCallback_t)_qscanfGetFreqScanRsp, cmd);
        atCmdSetTimeoutHandler(cmd->engine, 1000 * 60 * 3, _qscanfExeRegRspTimeOutCB);
        if ((res = CFW_GetFreqScanInfo(cmd->uti, nSim)) != 0)
        {
            cfwReleaseUTI(cmd->uti);
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }
        RETURN_FOR_ASYNC();
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}
//mode : 0 is means CLASS_TYPE_B 1: is means CLASS_TYPE_CG(close CS domain)
void atCmdHandleTSTSETCS(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint8_t mode = 0;
    uint32_t ret = ERR_SUCCESS;
    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint8_t nEmodeMode = 2;
        uint8_t nGetMode = atParamDefUintInRange(cmd->params[0], 0, 0, 1, &paramok);

        if (!paramok || cmd->param_count != 1)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));

        if (0 != CFW_CfgNwGetCgclassType(&mode, nSim))
        {
            OSI_LOGI(0, "TSTSETCS:Get CgclassType error\n\r");
            return;
        }

        CFW_EmodGetCEMode(&nEmodeMode, nSim);
        //gsm mode is the same and lte mode is the same
        if (((nGetMode == mode) && (0 == mode) && (2 == nEmodeMode)) || ((nGetMode == mode) && (1 == mode) && (0 == nEmodeMode)))
        {
            OSI_LOGI(0, "TSTSETCS:The current mode is the same as the one you want to set.\n\r");
            AT_CMD_RETURN(atCmdRespOK(cmd->engine));
        }

        ret = CFW_NwSetCSMode(nGetMode, nSim);
        if (ERR_SUCCESS != ret)
        {
            OSI_LOGI(0, "TSTSETCS:Set mode error %d\n\r", ret);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            return;
        }
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ)
    {
        char rsp[40] = {
            0,
        };
        ret = CFW_CfgNwGetCgclassType(&mode, nSim);
        if (ERR_SUCCESS != ret)
        {
            OSI_LOGI(0, "TSTSETCS:Get mode error %d\n\r", ret);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
            return;
        }

        sprintf(rsp, "TSTSETCS: %d", mode);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmsError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

#endif
static void _mjdcTestRegRspTimeOutCB(atCommand_t *cmd)
{
    cfwReleaseUTI(cmd->uti);
    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_SEND_TIMEOUT));
}

static void _mjdcTestRegRsp(atCommand_t *cmd, const osiEvent_t *event)
{
    char rsp[32] = {
        0,
    };
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_NW_JAMMING_DETECT_GET_IND *p = (CFW_NW_JAMMING_DETECT_GET_IND *)cfw_event->nParam1;

    if (cfw_event->nType == 0)
    {
        if (0 == p->nMode)
        {
            sprintf(rsp, "+MJDC: %d", p->nMode);
        }
        else if (1 == p->nMode)
        {
            sprintf(rsp, "+MJDC: %d,%d", p->nMode, p->nJamming);
        }
        else
        {
            OSI_LOGI(0, "_mjdcTestRegRsp ERROR:nMode: %d", p->nMode);
        }

        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }

    RETURN_CME_CFW_ERR(cmd->engine, cfw_event->nParam1);
}

void atCmdHandleMJDC(atCommand_t *cmd)
{
    uint8_t nSim = atCmdGetSim(cmd->engine);
    uint32_t ret = ERR_SUCCESS;
    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint8_t nGetMode = atParamDefUintInRange(cmd->params[0], 0, 0, 1, &paramok);

        if (!paramok || cmd->param_count != 1)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));

        if (0 != CFW_SetJammingDetectFlag(nGetMode, nSim))
        {
            OSI_LOGI(0, "MJDC:Set Jamming error\n\r");
            return;
        }

        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        char rsp[40] = {
            0,
        };
        sprintf(rsp, "+MJDC:(0,1)");
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ)
    {
        cmd->uti = cfwRequestUTI((osiEventCallback_t)_mjdcTestRegRsp, cmd);
        atCmdSetTimeoutHandler(cmd->engine, 1000 * 60 * 2, _mjdcTestRegRspTimeOutCB);
        if ((ret = CFW_GetJammingDetect(cmd->uti, nSim)) != 0)
        {
            cfwReleaseUTI(cmd->uti);
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }
        RETURN_FOR_ASYNC();
    }
    else
    {
        atCmdRespCmsError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

static int nw_paramHex2Num(uint8_t c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return 0xa + (c - 'A');
    if (c >= 'a' && c <= 'f')
        return 0xa + (c - 'a');
    return -1;
}

static bool nw_paramHexDataParse(uint8_t *dest, uint8_t *src, uint16_t length)
{
    OSI_LOGI(0, "_paramHexDataParse in(%d): %s", length, src);

    const uint8_t *pv = src;
    uint16_t n;
    for (n = 0; n < length; n++)
    {
        if (nw_paramHex2Num(*pv++) < 0)
        {
            OSI_LOGI(0, "_paramHexDataParse invalid character");
            return false;
        }
    }

    uint8_t *s = src;
    length /= 2;
    for (n = 0; n < length; n++)
    {
        int num0 = nw_paramHex2Num(s[0]);
        int num1 = nw_paramHex2Num(s[1]);
        s += 2;
        dest[n] = (num0 << 4) | num1;
    }
    return true;
}

void atCmdHandleBLACKLIST(atCommand_t *cmd)
{
    const char *temgsm = NULL;
    char rsp[64] = {0};
    uint8_t nSim = atCmdGetSim(cmd->engine);

    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint16_t i, j, NextParamCount, GsmStrLen;
        uint32_t nSimid = 0, nEnable = 2;
        CFW_BlackCellList nBlackCellList = {0};

        uint8_t nBlackFlag = atParamUintInRange(cmd->params[0], 0, 4, &paramok);

        if (!paramok || cmd->param_count > 24)
        {
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }
        OSI_LOGI(0, "at-black:nBlackFlag %d, param_count %d\n", nBlackFlag, cmd->param_count);
        if (nBlackFlag == 0) //quary
        {
            if (cmd->param_count != 1)
            {
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            }
            else
            {
                nEnable = CFW_nvGetBlackListEnable(nSimid);
                OSI_LOGI(0, "at-black:quary nEnable %d, cardid %d\n", nEnable, nSimid);
                sprintf(rsp, "^BLACKLIST: Get nEnable %lu", nEnable);
                atCmdRespInfoText(cmd->engine, rsp);
                atCmdRespOK(cmd->engine);
            }
        }
        else if (nBlackFlag == 1) //enable
        {
            if (cmd->param_count != 2)
            {
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            }
            else
            {
                nEnable = atParamUintInRange(cmd->params[1], 0, 4, &paramok);
                OSI_LOGI(0, "at-black:set nEnbale %d, cardid %d, paramok %d\n", nEnable, nSimid, paramok);

                if (!paramok)
                {
                    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                }
                OSI_LOGI(0, "at-black:nEnbale %d, cardid %d\n", nEnable, nSimid);
                if (nEnable == 0 || nEnable == 1)
                {
                    CFW_nvSetBlackListEnable(nEnable, nSimid);
                    sprintf(rsp, "^BLACKLIST: Set nEnable %lu", nEnable);
                    atCmdRespInfoText(cmd->engine, rsp);
                    atCmdRespOK(cmd->engine);
                }
                else
                {
                    OSI_LOGI(0, "at-black:set nEnbale %d err\n", nEnable);
                    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                }
            }
        }
        else if (nBlackFlag == 2) //Gsm
        {
            uint16_t nBlackCellTotalNumGL = 0;
            nEnable = CFW_nvGetBlackListEnable(nSimid);
            if (nEnable != 1)
            {
                OSI_LOGI(0, "Operation Not Allowed, nEnable:%d", nEnable);
                sprintf(rsp, "+CME ERROR:Operation Not Allowed");
                AT_CMD_RETURN(atCmdRespErrorText(cmd->engine, rsp));
            }
            nBlackCellList.nGsmBlackcellnum = atParamUintInRange(cmd->params[1], 1, 4, &paramok);
            if (!paramok)
            {
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            }
            OSI_LOGI(0, "at-black:nGsmBlackcellnum %d\n", nBlackCellList.nGsmBlackcellnum);
            if ((nBlackCellList.nGsmBlackcellnum == 0) || (nBlackCellList.nGsmBlackcellnum > 4))
            {
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            }
            else
            {
                for (i = 0; i < nBlackCellList.nGsmBlackcellnum; i++) //gsm nA,nL,nB
                {
                    OSI_LOGI(0, "at-black:next gsmmm i %d, count %d\n", i, 2 + i * 3);
                    nBlackCellList.nGsmBlackCellList[i].nArfcn = atParamUintInRange(cmd->params[2 + i * 3], 0, 65536, &paramok);
                    if (!paramok)
                    {
                        OSI_LOGI(0, "at-black:gsm i %d, err1\n", i);
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                    temgsm = atParamStr(cmd->params[3 + i * 3], &paramok);
                    GsmStrLen = strlen(temgsm);
                    OSI_LOGI(0, "at-black:gsm lai_len %d\n", GsmStrLen);
                    if (strlen(temgsm) != 10)
                    {
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                    if (!paramok)
                    {
                        OSI_LOGI(0, "at-black:gsm i %d, err2\n", i);
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                    nw_paramHexDataParse(nBlackCellList.nGsmBlackCellList[i].nLai, (uint8_t *)temgsm, 10);

                    nBlackCellList.nGsmBlackCellList[i].nBsic = atParamUintInRange(cmd->params[4 + i * 3], 0, 255, &paramok);
                    if (!paramok)
                    {
                        OSI_LOGI(0, "at-black:gsm i %d, err3\n", i);
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                }
                NextParamCount = 2 + i * 3;
                OSI_LOGI(0, "at-black: NextLteParamCount %d, i %d\n", NextParamCount, i);
                if ((NextParamCount < cmd->param_count) && (atParamUintInRange(cmd->params[NextParamCount], 0, 4, &paramok) == 4)) //lte
                {
                    nBlackCellList.nLteBlackCellNum = atParamUintInRange(cmd->params[NextParamCount + 1], 0, 4, &paramok);
                    if (!paramok)
                    {
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                    nBlackCellTotalNumGL = nBlackCellList.nLteBlackCellNum + nBlackCellList.nGsmBlackcellnum;
                    OSI_LOGI(0, "at-black:next nLetBlackcellnum %d, totalNum %d\n", nBlackCellList.nLteBlackCellNum, nBlackCellTotalNumGL);
                    if ((nBlackCellList.nLteBlackCellNum == 0) || (nBlackCellList.nLteBlackCellNum > 4) || (nBlackCellTotalNumGL > 4))
                    {
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                    else
                    {
                        for (i = 0; i < nBlackCellList.nLteBlackCellNum; i++) //lte feq & cellid
                        {
                            nBlackCellList.nLteBlackCellList[i].nFrequency = atParamUintInRange(cmd->params[NextParamCount + 2 + i * 2], 0, 262143, &paramok);
                            if (!paramok)
                            {
                                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                            }
                            nBlackCellList.nLteBlackCellList[i].nPhyCellId = atParamUintInRange(cmd->params[NextParamCount + 3 + i * 2], 0, 65536, &paramok);
                            if (!paramok)
                            {
                                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                            }
                        }
                    }
                    //set stack

                    CFW_NwSetBlackList(nBlackCellList, nSim);

                    sprintf(rsp, "^BLACKLIST: GSM AND LTE OK");
                    atCmdRespInfoText(cmd->engine, rsp);
                    atCmdRespOK(cmd->engine);
                }
                else
                {
                    CFW_NwSetBlackList(nBlackCellList, nSim);

                    sprintf(rsp, "^BLACKLIST: SET GSM ONLY OK");
                    atCmdRespInfoText(cmd->engine, rsp);
                    atCmdRespOK(cmd->engine);
                }
            }
        }
        else if (nBlackFlag == 4) //Lte
        {
            uint16_t nBlackCellTotalNumLG = 0;
            uint32_t nIfGsmflag = 0;

            nEnable = CFW_nvGetBlackListEnable(nSimid);
            if (nEnable != 1)
            {
                OSI_LOGI(0, "Operation Not Allowed, nEnable:%d", nEnable);
                sprintf(rsp, "+CME ERROR:Operation Not Allowed");
                AT_CMD_RETURN(atCmdRespErrorText(cmd->engine, rsp));
            }
            nBlackCellList.nLteBlackCellNum = atParamUintInRange(cmd->params[1], 1, 4, &paramok);
            if (!paramok)
            {
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            }
            OSI_LOGI(0, "at-black:nLteBlackCellNum %d\n", nBlackCellList.nLteBlackCellNum);
            if ((nBlackCellList.nLteBlackCellNum == 0) || (nBlackCellList.nLteBlackCellNum > 4))
            {
                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
            }
            else
            {
                for (i = 0; i < nBlackCellList.nLteBlackCellNum; i++) //lte feq & cellid
                {
                    nBlackCellList.nLteBlackCellList[i].nFrequency = atParamUintInRange(cmd->params[2 + i * 2], 0, 262143, &paramok);
                    if (!paramok)
                    {
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                    nBlackCellList.nLteBlackCellList[i].nPhyCellId = atParamUintInRange(cmd->params[3 + i * 2], 0, 65536, &paramok);
                    if (!paramok)
                    {
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                }

                NextParamCount = 2 + i * 2;
                OSI_LOGI(0, "at-black:NextGsmParamCount %d, i %d\n", NextParamCount, i);
                if (cmd->param_count > 10)
                {
                    nIfGsmflag = atParamUintInRange(cmd->params[NextParamCount], 0, 4, &paramok);
                    OSI_LOGI(0, "at-black: nIfGsmflag %d\n", nIfGsmflag);
                    if (nIfGsmflag != 2)
                    {
                        sprintf(rsp, "+CME ERROR:Operation Not Allowed");
                        AT_CMD_RETURN(atCmdRespErrorText(cmd->engine, rsp));
                    }
                }
                if ((NextParamCount < cmd->param_count) && (atParamUintInRange(cmd->params[NextParamCount], 0, 4, &paramok) == 2)) //gsm
                {
                    nBlackCellList.nGsmBlackcellnum = atParamUintInRange(cmd->params[NextParamCount + 1], 0, 4, &paramok);
                    if (!paramok)
                    {
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                    nBlackCellTotalNumLG = nBlackCellList.nLteBlackCellNum + nBlackCellList.nGsmBlackcellnum;
                    OSI_LOGI(0, "at-black:next nGsmBlackcellnum %d, total %d\n", nBlackCellList.nGsmBlackcellnum, nBlackCellTotalNumLG);

                    if ((nBlackCellList.nGsmBlackcellnum == 0) || (nBlackCellList.nGsmBlackcellnum > 4) || (nBlackCellTotalNumLG > 4))
                    {
                        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                    }
                    else
                    {
                        for (j = 0; j < nBlackCellList.nGsmBlackcellnum; j++) //gsm nA,nL,nB
                        {
                            OSI_LOGI(0, "at-black:next NextParamCountfff %d, j %d\n", NextParamCount + 2 + j * 3, j);
                            nBlackCellList.nGsmBlackCellList[j].nArfcn = atParamUintInRange(cmd->params[NextParamCount + 2 + j * 3], 0, 65536, &paramok);
                            if (!paramok)
                            {
                                OSI_LOGI(0, "at-black:nGsmBlackcellnum j %d, err1\n", j);
                                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                            }
                            temgsm = atParamStr(cmd->params[NextParamCount + 3 + j * 3], &paramok);
                            GsmStrLen = strlen(temgsm);
                            OSI_LOGI(0, "at-black:gsm down lai_len %d, %02x\n", GsmStrLen, temgsm[0], temgsm[1], temgsm[2], temgsm[3], temgsm[4]);

                            if (strlen(temgsm) != 10)
                            {
                                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                            }
                            if (!paramok)
                            {
                                OSI_LOGI(0, "at-black:nGsmBlackcellnum j %d, err2\n", j);
                                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                            }

                            nw_paramHexDataParse(nBlackCellList.nGsmBlackCellList[j].nLai, (uint8_t *)temgsm, 10);

                            nBlackCellList.nGsmBlackCellList[j].nBsic = atParamUintInRange(cmd->params[NextParamCount + 4 + j * 3], 0, 255, &paramok);
                            if (!paramok)
                            {
                                OSI_LOGI(0, "at-black:nGsmBlackcellnum j %d, err3\n", j);
                                AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
                            }
                        }
                    }
                    //set stack

                    CFW_NwSetBlackList(nBlackCellList, nSim);

                    sprintf(rsp, "^BLACKLIST: LTE AND GSM OK");
                    atCmdRespInfoText(cmd->engine, rsp);
                    atCmdRespOK(cmd->engine);
                }
                else
                {
                    CFW_NwSetBlackList(nBlackCellList, nSim);

                    sprintf(rsp, "^BLACKLIST: SET LTE ONLY OK");
                    atCmdRespInfoText(cmd->engine, rsp);
                    atCmdRespOK(cmd->engine);
                }
            }
        }
        else
        {
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED));
        }
    }
    else if (cmd->type == AT_CMD_READ)
    {
        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED));
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        sprintf(rsp, "^BLACKLIST: 4,1,10,20,2,1,8,'64F0010001',9");
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED));
    }
}

#define NETMSG_BAND_GSM900 9
#define NETMSG_BAND_DCS1800 18
#define NETMSG_BAND_PCS1900 19
#define NETMSG_BAND_INVALID 0xFF

bool AT_EMOD_NETMSG_ProcessData(CFW_SIM_ID nSim, CFW_TSM_CURR_CELL_INFO *pCurrCellInfo,
                                char *nStrPosiList)
{
    volatile AT_NETMSG_GSMDATA PosiList;
    char nStr[100] = {0};
    memset((void *)&PosiList, 0, sizeof(AT_NETMSG_GSMDATA));
    OSI_LOGI(0, "AT_EMOD_NETMSG_ProcessData!");

    if ((NULL == pCurrCellInfo) || (NULL == nStrPosiList))
    {
        OSI_LOGI(0, "AT_EMOD_NETMSG_ProcessData InputErr\n");
        return false;
    }
    sprintf(nStrPosiList, "+NETMSG GSM:");

    PosiList.sMcc[0] = pCurrCellInfo->nTSM_LAI[0] & 0x0F;
    PosiList.sMcc[1] = pCurrCellInfo->nTSM_LAI[0] >> 4;
    PosiList.sMcc[2] = pCurrCellInfo->nTSM_LAI[1] & 0x0F;
    PosiList.nArfcn = pCurrCellInfo->nTSM_Arfcn;

    PosiList.sMnc[0] = pCurrCellInfo->nTSM_LAI[2] & 0x0F;
    PosiList.sMnc[1] = pCurrCellInfo->nTSM_LAI[2] >> 4;
    PosiList.sMnc[2] = pCurrCellInfo->nTSM_LAI[1] >> 4;

    PosiList.sLac = (pCurrCellInfo->nTSM_LAI[3] << 8) | (pCurrCellInfo->nTSM_LAI[4]);
    PosiList.sCellID = (pCurrCellInfo->nTSM_CellID[0] << 8) | (pCurrCellInfo->nTSM_CellID[1]);
    PosiList.iBsic = pCurrCellInfo->nTSM_Bsic;
    PosiList.iNetMode = pCurrCellInfo->nTSM_NetMode;
    PosiList.iBandInfo = pCurrCellInfo->nTSM_CurrBand;
    PosiList.nArfcn = pCurrCellInfo->nTSM_Arfcn;
    PosiList.rxdBm = pCurrCellInfo->nTSM_RxDbm;
    if (pCurrCellInfo->nTSM_TxDbm != 255)
    {
        if (pCurrCellInfo->nTSM_CurrBand == NETMSG_BAND_GSM900)
        {
            PosiList.txdBm = 33 - 2 * (pCurrCellInfo->nTSM_TxDbm - 5);
        }
        else if (pCurrCellInfo->nTSM_CurrBand == NETMSG_BAND_DCS1800 || pCurrCellInfo->nTSM_CurrBand == NETMSG_BAND_PCS1900)
        {
            PosiList.txdBm = 30 - 2 * pCurrCellInfo->nTSM_TxDbm;
        }
        else
        {
            PosiList.txdBm = 255;
        }
    }
    else
    {
        PosiList.txdBm = 255;
    }
    PosiList.iRssi = pCurrCellInfo->nTSM_AvRxLevel;
    sprintf(nStr, "%x%x%x,%x%x%x,%d,%d,%d,%d,%d,%ld,-%d,%d,%d",
            PosiList.sMcc[0], PosiList.sMcc[1], PosiList.sMcc[2],
            PosiList.sMnc[0], PosiList.sMnc[1], PosiList.sMnc[2],
            PosiList.sLac, PosiList.sCellID, PosiList.iBsic, PosiList.iNetMode, PosiList.iBandInfo,
            PosiList.nArfcn, PosiList.rxdBm, PosiList.txdBm, PosiList.iRssi);
    strcat(nStrPosiList, nStr);
    return true;
}
static void NETMSG_RspCB(atCommand_t *cmd, const osiEvent_t *event)
{
    OSI_LOGI(0, "NETMSG_RspCB------Back");
    const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
    CFW_SIM_ID nSim = (CFW_SIM_ID)cfw_event->nFlag;
    uint32_t nRet = 0;
    bool bRet = false;
    CFW_TSM_CURR_CELL_INFO tCurrCellInf;
    CFW_TSM_ALL_NEBCELL_INFO tNeighborCellInfo;
    memset(&tCurrCellInf, 0, sizeof(CFW_TSM_CURR_CELL_INFO));
    memset(&tNeighborCellInfo, 0, sizeof(CFW_TSM_ALL_NEBCELL_INFO));

    if (cfw_event->nType != 0)
    {
        OSI_LOGI(0, "NW_AsyncEvent:   ERROR!!!---CfwEvent.nType --0x%x\n\r", cfw_event->nType);
        AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
    }
    if (cfw_event->nParam2 == CFW_TSM_CURRENT_CELL)
    {
        char rsp[100] = {0};
        nRet = CFW_GetCellInfo(&tCurrCellInf, &tNeighborCellInfo, nSim);
        if (nRet != ERR_SUCCESS)
        {
            OSI_LOGI(0, "NW_AsyncEvent ERROR nRet:0x%x!\n", nRet);
            CFW_EmodOutfieldTestEnd(cfw_event->nUTI, nSim);
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
        }
        bRet = AT_EMOD_NETMSG_ProcessData(nSim, &tCurrCellInf, rsp);
        if (bRet)
        {
            atCmdRespInfoText(cmd->engine, rsp);
            atCmdRespOK(cmd->engine);
        }
        else
        {
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }
        CFW_EmodOutfieldTestEnd(cfw_event->nUTI, nSim);
    }
    else
    {
        CFW_EmodOutfieldTestEnd(cfw_event->nUTI, nSim);
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL);
    }
}

void atCmdHandleNETMSG(atCommand_t *cmd)
{
    char rsp[100] = {0};
    char *iRsp = rsp;
    uint32_t nSim = atCmdGetSim(cmd->engine);
    uint8_t rat = CFW_NWGetStackRat(nSim);
    if (rat == 4)
    {
        CFW_NET_INFO_T *pNetinfo = (CFW_NET_INFO_T *)malloc(sizeof(CFW_NET_INFO_T));
        memset(pNetinfo, 0, sizeof(CFW_NET_INFO_T));
        if (0 != CFW_GetNetinfo(pNetinfo, nSim))
        {
            free(pNetinfo);
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_FAIL));
        }
        if ((pNetinfo->nwCapabilityLte.lteScell.plmn[1] >> 4) <= 9)
        {
            iRsp += sprintf(iRsp, "+NETMSG LTE:%x%x%x,%x%x%x,", pNetinfo->nwCapabilityLte.lteScell.plmn[0] & 0x0F,
                            pNetinfo->nwCapabilityLte.lteScell.plmn[0] >> 4,
                            pNetinfo->nwCapabilityLte.lteScell.plmn[1] & 0x0F,
                            pNetinfo->nwCapabilityLte.lteScell.plmn[2] & 0x0F,
                            pNetinfo->nwCapabilityLte.lteScell.plmn[2] >> 4,
                            pNetinfo->nwCapabilityLte.lteScell.plmn[1] >> 4);
        }
        else
        {
            iRsp += sprintf(iRsp, "+NETMSG LTE:%x%x%x,%x%x,", pNetinfo->nwCapabilityLte.lteScell.plmn[0] & 0x0F,
                            pNetinfo->nwCapabilityLte.lteScell.plmn[0] >> 4,
                            pNetinfo->nwCapabilityLte.lteScell.plmn[1] & 0x0F,
                            pNetinfo->nwCapabilityLte.lteScell.plmn[2] & 0x0F,
                            pNetinfo->nwCapabilityLte.lteScell.plmn[2] >> 4);
        }
        iRsp += sprintf(iRsp, "%d,%ld,%d,%d,%d,%ld,%.2f,%d,%d,%d,%d", pNetinfo->nwCapabilityLte.lteScell.tac,
                        (uint32_t)((pNetinfo->nwCapabilityLte.lteScell.enodeBID << 8) + pNetinfo->nwCapabilityLte.lteScell.cellId),
                        pNetinfo->nwCapabilityLte.lteScell.pcid,
                        pNetinfo->nwCapabilityLte.lteScell.netMode,
                        pNetinfo->nwCapabilityLte.lteScell.bandInfo,
                        pNetinfo->nwCapabilityLte.lteScell.dlEarfcn,
                        (double)(pNetinfo->nwCapabilityLte.lteScell.rxPower / 16),
                        pNetinfo->nwCapabilityLte.lteScell.txPower,
                        pNetinfo->nwCapabilityLte.lteScell.rsrp,
                        pNetinfo->nwCapabilityLte.lteScell.rsrq,
                        pNetinfo->nwCapabilityLte.lteScell.SINR);
        free(pNetinfo);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        uint32_t iRet = 0;
        CFW_TSM_FUNCTION_SELECT tSelecFUN;
        memset(&tSelecFUN, 0, sizeof(CFW_TSM_FUNCTION_SELECT));
        tSelecFUN.nServingCell = 1;
        cmd->uti = cfwRequestUTI((osiEventCallback_t)NETMSG_RspCB, cmd);
        iRet = CFW_EmodOutfieldTestStart(&tSelecFUN, cmd->uti, nSim);
        if (0 != iRet)
        {
            OSI_LOGI(0, "AT+NETMSG ERROR:iRet: %d, Emod", iRet);
            cfwReleaseUTI(cmd->uti);
            atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }
    }
}
void atCfwNwInit(void)
{
    atEventsRegister(
        EV_CFW_NW_SIGNAL_QUALITY_IND, _onEV_CFW_NW_SIGNAL_QUALITY_IND,
        EV_CFW_NW_REG_STATUS_IND, _onEV_CFW_NW_REG_STATUS_IND,
        //EV_CFW_TSM_INFO_IND, _onEV_CFW_TSM_INFO_IND,
        EV_CFW_NW_STORELIST_IND, _onEV_CFW_NW_STORELIST_IND,
        EV_CFW_NW_NETWORKINFO_IND, _onEV_CFW_NW_NETWORKINFO_IND,
        EV_CFW_ERRC_CONNSTATUS_IND, _onEV_CFW_ERRC_CONNSTATUS_IND,
        EV_CFW_GPRS_EDRX_IND, _onEV_CFW_GPRS_EDRX_IND,
        EV_CFW_GPRS_CCIOTOPT_IND, _onEV_CFW_GPRS_CCIOTOPT_IND,
        EV_CFW_NW_CELL_LIST_INFO_IND, _onEV_CFW_NW_CELL_LIST_INFO_IND,
#ifdef CONFIG_SOC_6760
        EV_CFW_NW_PAY_LOAD_INFO_IND, _onEV_CFW_NW_PAY_LOAD_INFO_IND,
#endif
        EV_CFW_NW_JAMMING_DETECT_IND, _onEV_CFW_NW_JAMMING_DETECT_IND,
        EV_CFW_EMC_NUM_LIST_IND, _onEV_CFW_EMC_NUM_LIST_IND,

        0);
}
