

#include "cJSON.h"
#include "netutils.h"
#include "async_worker.h"
#include "mupnp/util/string.h"
#include "http_api.h"
#include "sockets.h"
#include "at_cfg.h"
#include <time.h>
#include "mbedtls/base64.h"
#include "at_engine.h"
#include "osi_log.h"
#include "at_response.h"
#include "stdio.h"
#include "cfw.h"
#include "http_download.h"
#include "http_lunch_api.h"
#include "at_command.h"
#include "vfs.h"

#ifdef CONFIG_SOC_8910

#define UEICCID_LEN 20
#define REG_HTTP_URL "http://zzhc.vnet.cn:9999/"
#define REG_HTTP_CONTENT "application/encrypted-json"
#define NV_SELF_REG "/nvm/dm_self_register.nv"
#define SOFTWARE_VER_NUM "BC60_1268D_V1.0"

osiTimer_t *ghttpregtimer = NULL;

static void do_dxregProcess(uint8_t nCid, uint8_t nSimId);
extern char *vnetregdata;

typedef struct {
    uint8_t simId;
    uint8_t nCid;
    uint8_t resultCode;
    uint8_t retryCount;
    uint8_t resultDesc[12];
}reg_ctrl_t;

reg_ctrl_t gRegCtrl;
reg_ctrl_t *reg_ctrl;
nHttp_info *at_nHttp_reg;

static bool isResultOk(reg_ctrl_t *reg_ctrl)
{
    return reg_ctrl->resultCode == 0 && strcmp((char *)reg_ctrl->resultDesc,"Success") == 0;
}

static void parseJson(char *pMsg, reg_ctrl_t *reg_ctrl)
{
    if (NULL == pMsg)
    {
        sys_arch_printf("parseJson: pMsg is NULL!!!\n");
        return;
    }
    cJSON *pJson = cJSON_Parse(pMsg);
    if (NULL == pJson)
    {
        sys_arch_printf("parseJson: pJson is NULL!!!\n");
        return;
    }
    char *regdatajs = cJSON_PrintUnformatted(pJson);
    sys_arch_printf("parseJson: %s\n", regdatajs);

    cJSON *pSub = cJSON_GetObjectItem(pJson, "resultCode");
    if (NULL != pSub)
    {
        sys_arch_printf("resultCode: %s\n", pSub->valuestring);
        reg_ctrl->resultCode = atoi(pSub->valuestring);
    }

    pSub = cJSON_GetObjectItem(pJson, "resultDesc");
    if (NULL != pSub)
    {
        sys_arch_printf("resultDesc: %s\n", pSub->valuestring);
        strncpy((char *)reg_ctrl->resultDesc,pSub->valuestring,10);
    }
     free(regdatajs);
}

OSI_UNUSED static int OutputTime(char *rsp, struct tm *tm)
{
    static const char *fmt1 = "20%02d-%02d-%02d,%02d:%02d:%02d";
    static const char *fmt2 = "20%04d-%02d-%02d,%02d:%02d:%02d";
    if (gAtSetting.csdf_auxmode == 1)
    {
        return sprintf(rsp, fmt1, (tm->tm_year + 1900) % 100, tm->tm_mon + 1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
    else
    {
        return sprintf(rsp, fmt2, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
}

void httpreg_timeout(void *param)
{
    do_dxregProcess(reg_ctrl->nCid,  reg_ctrl->simId);
}

static char *genRegData(uint8_t simid)
{
    unsigned char *buffer;
    size_t buffer_len;
    size_t len;
    size_t out_len;
    cJSON *pJsonRoot = NULL;
    pJsonRoot = cJSON_CreateObject();
    if (NULL == pJsonRoot)
        return NULL;

    uint8_t imei[16] = {0};
    uint8_t imsi[16] = {0};
    uint8_t simiccid[21] = {0};
    uint8_t imeisize = 16;
    uint8_t imsisize = 16;
    uint8_t iccid_len = 20;
    getSimIccid(simid,simiccid,&iccid_len);
    getSimImei(simid,imei,&imeisize);
    getSimImsi(simid,imsi,&imsisize);
    time_t lt = osiEpochSecond() ;
    struct tm ltm;
    gmtime_r(&lt, &ltm);
    char ts[48];
    OutputTime(ts, &ltm);

    cJSON_AddStringToObject(pJsonRoot, "REGVER", "5.0");
    cJSON_AddStringToObject(pJsonRoot, "MEID", "");
    cJSON_AddStringToObject(pJsonRoot, "MODEL", "BJS-BC60");
    cJSON_AddStringToObject(pJsonRoot, "SWVER", SOFTWARE_VER_NUM);    //软件版本号
    cJSON_AddStringToObject(pJsonRoot, "IMEI1", (char *)imei);
    cJSON_AddStringToObject(pJsonRoot, "SIM1CDMAIMSI", "");
    cJSON_AddStringToObject(pJsonRoot, "SIM1ICCID", (const char *)simiccid);
    cJSON_AddStringToObject(pJsonRoot, "SIM1LTEIMSI", (const char *)imsi);
    cJSON_AddStringToObject(pJsonRoot, "SIM1CELLID", "LTE");
    cJSON_AddStringToObject(pJsonRoot, "REGDATE", (const char *)ts);

    char *regdatajs = cJSON_PrintUnformatted(pJsonRoot);

    if (regdatajs == NULL)
    {
        sys_arch_printf("JsonString error...");
        cJSON_Delete(pJsonRoot);
        return NULL;
    }
    else
        sys_arch_printf("genRegData :%s\n", regdatajs);

    out_len = strlen(regdatajs);

    mbedtls_base64_encode( NULL, 0, &buffer_len, (const unsigned char *)regdatajs, out_len);
    buffer = (unsigned char *)malloc(buffer_len);

    if (buffer != NULL && mbedtls_base64_encode(buffer, buffer_len, &len, (const unsigned char *)regdatajs, out_len) != 0)
    {
        sys_arch_printf("base64 error...");
    }
    free(regdatajs);
    cJSON_Delete(pJsonRoot);
    return (char *)buffer;
}

static AWORKER_RC at_httpreg_worker_handler(AWORKER_REQ *req)
{
    uint32_t event = req->event;
    AWORKER_RC rc = AWRC_SUCCESS;
    bool paramret = true;
    uint16_t index = 0;
    nHttp_info *nHttp_inforeg;
    sys_arch_printf("at_httpx_worker_handler event:%ld", event);
    switch (event)
    {
        case 1:
        {
            uint32_t ADDR;
            ADDR = aworker_param_getu32(req, index++, (bool *)&paramret);
            if (!paramret)
            {
                sys_arch_printf("AT_HTTPpostreg get param fail from async worker");
                rc = AWRC_FAIL;
                break;
            }
            nHttp_inforeg = (nHttp_info *)ADDR;

            if (Http_postnreg(nHttp_inforeg) == false)
            {
                rc = AWRC_FAIL;
	         break;
            }
            else
            {

                rc = AWRC_SUCCESS;
            }

            break;
        }
        default:
            sys_arch_printf("at_httpreg_worker_handler unhandled event:%ld", event);
        break;
    }

    if (rc != AWRC_PROCESSING)
    {
        sys_arch_printf("the httpreg request will be deleted by aworker, and become invalid\n");     
        // if return value was not AWRC_PROCESSING,
        // the request will be deleted by aworker, and become invalid
    }
    return rc;
}

static void at_httpreg_worker_callback(int result, uint32_t event, void *param)
{

    sys_arch_printf("at_httpreg_worker_callback result:%d event:%ld", result, event);
    switch (event)
    {
    case 1:
        break;
    default:
        sys_arch_printf("at_httpreg_worker_callback unhandled event:%ld", event);
        break;
    }
     if (Term_Http(at_nHttp_reg) != true)
     {
            sys_arch_printf("Term fail, please try again later\n");
            return;
      }

    if (result == 0)
    {
        // response operation succuss
        //char *data = NULL;    //data 做一个全局变量下行下去得到返回的数据
       sys_arch_printf("at_httpreg_worker_callback");
       parseJson(vnetregdata, reg_ctrl);
       if(NULL != vnetregdata)
       {
           free(vnetregdata);
           vnetregdata = NULL;
       }
       if (isResultOk(reg_ctrl))
       {
           uint8_t simiccid[UEICCID_LEN+1] = {0};
           uint8_t iccid_len = UEICCID_LEN;
           if (ghttpregtimer != NULL)
           {
               osiTimerStop(ghttpregtimer);
               osiTimerDelete(ghttpregtimer);
               ghttpregtimer = NULL;
           }
           sys_arch_printf("response operation succuss data right");
           getSimIccid(reg_ctrl->simId,simiccid,&iccid_len);
           vfs_file_write(NV_SELF_REG, simiccid, 20);
           //NV_SetUEIccid(simiccid,iccid_len,reg_ctrl->simId);
       }
       else if (reg_ctrl->retryCount < 10)
       {
           reg_ctrl->retryCount++;
           sys_arch_printf("response operation succuss data error");
           ghttpregtimer = osiTimerCreate(osiThreadCurrent(), httpreg_timeout, NULL);
           if (ghttpregtimer == NULL)
           {
               sys_arch_printf("HTTPreg# HTTPregTimmer create timer failed");
               return;
           }
           osiTimerStart(ghttpregtimer , 60 * 60 * 1000);
           //COS_StartFunctionTimer(60*60*1000,do_dxregProcess,reg_ctrl->simId);
	 }

    }
    else
    {
        // response operation fail
        if(reg_ctrl->retryCount < 10)
        {
            reg_ctrl->retryCount++;
            sys_arch_printf("response operation succuss data error result error");
            ghttpregtimer = osiTimerCreate(osiThreadCurrent(), httpreg_timeout, NULL);
            if (ghttpregtimer == NULL)
            {
                sys_arch_printf("HTTPreg# HTTPregTimmer create timer failed result error");
                return;
            }
            osiTimerStart(ghttpregtimer , 60 * 60 * 1000);
        }
        sys_arch_printf("response operation fail:%ld", event);
    }
}

static AWORKER_REQ *at_httpreg_create_async_req(uint32_t event, void *param, uint32_t bufflen)
{
    AWORKER_REQ *areq = aworker_create_request(atEngineGetThreadId(),
                                               at_httpreg_worker_handler, at_httpreg_worker_callback, event, param, bufflen);
    return areq;
}


static void do_dxregProcess(uint8_t nCid, uint8_t nSimId)
{
    char *regdata = NULL;
    AWORKER_REQ *areq;
    bool paramRet = true;

    if (netif_default == NULL)
        goto retry;
    at_nHttp_reg= Init_Http();
    if (at_nHttp_reg == NULL)
    {
        sys_arch_printf("Init fail, please try again later\n");
        goto retry;
    }
    regdata = genRegData(nSimId);
    if (regdata == NULL)
    {
        sys_arch_printf("regdata error...");
        goto retry;
    }
    at_nHttp_reg->CID = nCid;
    sys_arch_printf("do_dxregProcessat_nHttp_reg->CID=%d", at_nHttp_reg->CID);
    strcpy(at_nHttp_reg->url,  REG_HTTP_URL);
    strcpy(at_nHttp_reg->CONTENT_TYPE,  REG_HTTP_CONTENT );
    strcpy(at_nHttp_reg->user_data,  regdata );

    free(regdata);
    (at_nHttp_reg->cg_http_api)->nSIM = 0;
    (at_nHttp_reg->cg_http_api)->nCID = 1;
    areq = at_httpreg_create_async_req( 1, NULL, 0);
    if (areq == NULL)
    {
        sys_arch_printf("httppost create async request fail");
        goto retry;
    }
    reg_ctrl->nCid = nCid;
    reg_ctrl->simId = nSimId;
    aworker_param_putu32(areq, (uint32_t)at_nHttp_reg, &paramRet);
    if (!aworker_post_req_delay(areq, 0, &paramRet))
    {
        sys_arch_printf("httppostreg send async request fail");
        goto retry;
    }

    return;
retry:
    if (reg_ctrl->retryCount < 10)
    {
        reg_ctrl->retryCount++;
        sys_arch_printf("do_dxregProcess error");
        ghttpregtimer = osiTimerCreate(osiThreadCurrent(), httpreg_timeout, NULL);
        if (ghttpregtimer == NULL)
        {
            sys_arch_printf("do_dxregProcess# do_dxregProcessTimmer create timer failed");
            return;
        }
        osiTimerStart(ghttpregtimer , 60 * 60 * 1000);
        //COS_StartFunctionTimer(60*60*1000, do_dxregProcess, gRegCtrl.simId);
    }
    return;
}

bool sul_ZeroMemory8(void  *pBuf, uint32_t count)
{
    char *xs = pBuf;
    while (count--)
        *xs++ = 0x0;
    return 1;
}
int vnet4gSelfRegister(uint8_t nCid, uint8_t nSimId)
{
    uint8_t tmpMode = 0;
    uint8_t operatorId[6];
    uint8_t *pOperName = NULL;
    uint8_t *pOperShortName = NULL;
    uint8_t ueiccid[UEICCID_LEN + 1] = {0};
    uint8_t  nRet = 0;

    sul_ZeroMemory8(operatorId, 6);
    nRet = CFW_NwGetCurrentOperator(operatorId, &tmpMode, nSimId);
    if (nRet != 0)
        return -1;
    sys_arch_printf("vnet4gSelfRegister#CFW_NwGetCurrentOperator operatorId=%s %d", operatorId, tmpMode);
    nRet =  CFW_CfgNwGetOperatorName(operatorId, &pOperName, &pOperShortName);
    if (nRet != 0)
        return -1;
    sys_arch_printf("vnet4gSelfRegister#CFW_CfgNwGetOperatorName pOperName=%s", pOperName);
    if (0 == strcmp("ChinaTelecom", (char*)pOperName))
    {
        uint8_t simiccid[21] = {0};
        uint8_t iccid_len = UEICCID_LEN;
        if (!getSimIccid(nSimId,simiccid,&iccid_len))
            return -1;
        int fd = vfs_open(NV_SELF_REG, 2);
        if (fd < 0)
        {
            sys_arch_printf("vnet4gSelfRegister#This file is not exit vfs_open failed fd =%d", fd);
            vfs_file_write(NV_SELF_REG, "0", 1);
        }
        else
        {
            sys_arch_printf("vnet4gSelfRegister#This file is  exit vfs_open success fd =%d", fd);
            struct stat st = {};
            vfs_fstat(fd, &st);
            int file_size = st.st_size;
            if(file_size == 1 || file_size == 20)
            {
                sys_arch_printf("vnet4gSelfRegister#This file size is right =%d", file_size);
                vfs_read(fd, ueiccid, 20);
                sys_arch_printf("vnet4gSelfRegister#This file ueiccid =%s", ueiccid);
                vfs_close(fd);
            }
            else
            {
                sys_arch_printf("vnet4gSelfRegister#This file size is error =%d", file_size);
                vfs_unlink(NV_SELF_REG);
                vfs_file_write(NV_SELF_REG, "0", 1);
            }

        }
        // NV_GetUEIccid(ueiccid,iccid_len,simId);    //读flag   比较iccid
        if (memcmp(ueiccid, simiccid,iccid_len) == 0)
            return 0;
        else
        {
            sys_arch_printf("guangzusimiccid = %s", simiccid);
            reg_ctrl = (reg_ctrl_t *)malloc(sizeof(reg_ctrl));
            if(NULL == reg_ctrl)
            {
                sys_arch_printf("reg_ctrl malloc error");
                return -1;
            }
            reg_ctrl->retryCount = 0;
            do_dxregProcess(nCid, nSimId);
        }
        return 1;
    }
    else
    {
        sys_arch_printf("vnet4gSelfRegister# dvnet4gSelfRegister operator is not vnet4g");
        return 0;
    }
}



#endif

