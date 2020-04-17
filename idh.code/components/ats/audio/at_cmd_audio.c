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

#include "ats_config.h"
#ifdef CONFIG_AT_AUDIO_SUPPORT

#include "at_cfw.h"
#include "at_command.h"
#include "at_engine.h"
#include "at_response.h"
#include "osi_log.h"
#include "osi_pipe.h"
#include "audio_device.h"
#include "audio_player.h"
#include "audio_recorder.h"
#include "audio_encoder.h"
#include "cfw_chset.h"
#include "vfs.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "nvm.h"
#include "at_cmd_audio.h"

#define AT_StrLen(str) strlen((const char *)str)

typedef struct
{
    auPlayer_t *player;
} atAudioContext_t;

static atAudioContext_t gAtAudioCtx;

void atCmdHandleCACCP(atCommand_t *pParam)
{
    char urcBuffer[1024] = {0};

    uint8_t resultCode = 0;
    uint8_t hasMsg = 0;
    uint8_t resultMsg[900] = {0};

    uint8_t *temp = NULL;

    if (AT_CMD_SET == pParam->type)
    {
        bool paramok = true;

        if (pParam->param_count != 4)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        uint8_t mode = atParamUint(pParam->params[0], &paramok);
        uint8_t path = atParamUint(pParam->params[1], &paramok);
        uint8_t ctrl = atParamUint(pParam->params[2], &paramok);
        const char *param = atParamStr(pParam->params[3], &paramok);

        if (!paramok || (mode < 0 || mode > 6) || (path < 0 || path > 5) || (ctrl < 0 || ctrl > 7))
        {
            free(temp);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        temp = malloc(AT_StrLen(param) + 1);
        if (temp == NULL)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        memset(temp, 0x00, AT_StrLen(param) + 1);
        memcpy(temp, param, AT_StrLen(param));

        uint16_t param_len = AT_StrLen(param);
        CSW_SetAudioCodecCalibParam(&resultCode, &hasMsg, resultMsg, mode, path, ctrl, temp, param_len);

        memset(temp, 0x00, AT_StrLen(param) + 1);
        free(temp);
        temp = NULL;

        if (resultCode)
        {
            urcBuffer[0] = '\0';
            sprintf(urcBuffer, "ERROR %d", resultCode);
            atCmdRespInfoText(pParam->engine, urcBuffer);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        else
        {
            if (hasMsg)
            {
                urcBuffer[0] = '\0';
                sprintf(urcBuffer, "%s", resultMsg);
                atCmdRespInfoText(pParam->engine, urcBuffer);
            }
            atCmdRespOK(pParam->engine);
        }
    }
    else if (AT_CMD_TEST == pParam->type)
    {
        urcBuffer[0] = '\0';
        sprintf(urcBuffer, "+CACCP: (0-6),(0-5),(0-7),");
        atCmdRespInfoText(pParam->engine, urcBuffer);
        atCmdRespOK(pParam->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_OPTION_NOT_SURPORT));
    }
    return;
}

void atCmdHandleCAVQE(atCommand_t *pParam)
{
    char urcBuffer[1024] = {0};

    uint8_t resultCode = 0;
    uint8_t hasMsg = 0;
    uint8_t resultMsg[900] = {0};

    uint8_t *temp = NULL;

    if (AT_CMD_SET == pParam->type)
    {
        bool paramok = true;

        if (pParam->param_count != 4)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        uint8_t mode = atParamUint(pParam->params[0], &paramok);
        uint8_t path = atParamUint(pParam->params[1], &paramok);
        uint8_t ctrl = atParamUint(pParam->params[2], &paramok);
        const char *param = atParamStr(pParam->params[3], &paramok);

        if (!paramok || (mode < 0 || mode > 5) || (path < 0 || path > 5) || (ctrl < 0 || ctrl > 15))
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        temp = malloc(AT_StrLen(param) + 1);
        if (temp == NULL)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        memset(temp, 0x00, AT_StrLen(param) + 1);
        memcpy(temp, param, AT_StrLen(param));

        uint16_t param_len = AT_StrLen(param);
        CSW_SetAudioZspVqeCalibParam(&resultCode, &hasMsg, resultMsg, mode, path, ctrl, temp, param_len);

        memset(temp, 0x00, AT_StrLen(param) + 1);
        free(temp);
        temp = NULL;

        if (resultCode)
        {
            urcBuffer[0] = '\0';
            sprintf(urcBuffer, "ERROR %d", resultCode);
            atCmdRespInfoText(pParam->engine, urcBuffer);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        else
        {
            if (hasMsg)
            {
                urcBuffer[0] = '\0';
                sprintf(urcBuffer, "%s", resultMsg);
                atCmdRespInfoText(pParam->engine, urcBuffer);
            }
            atCmdRespOK(pParam->engine);
        }
    }
    else if (AT_CMD_TEST == pParam->type)
    {
        urcBuffer[0] = '\0';
        sprintf(urcBuffer, "+CAVQE: (0-5),(0-5),(0-15),");
        atCmdRespInfoText(pParam->engine, urcBuffer);
        atCmdRespOK(pParam->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_OPTION_NOT_SURPORT));
    }
    return;
}

void atCmdHandleCAPRE(atCommand_t *pParam)
{
    char urcBuffer[1024] = {0};

    uint8_t resultCode = 0;
    uint8_t hasMsg = 0;
    uint8_t resultMsg[900] = {0};

    uint8_t *temp = NULL;

    if (AT_CMD_SET == pParam->type)
    {
        bool paramok = true;

        if (pParam->param_count != 4)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        uint8_t mode = atParamUint(pParam->params[0], &paramok);
        uint8_t path = atParamUint(pParam->params[1], &paramok);
        uint8_t ctrl = atParamUint(pParam->params[2], &paramok);
        const char *param = atParamStr(pParam->params[3], &paramok);

        if (!paramok || (mode < 0 || mode > 5) || (path < 0 || path > 5) || (ctrl < 0 || ctrl > 5))
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        temp = malloc(AT_StrLen(param) + 1);
        if (temp == NULL)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        memset(temp, 0x00, AT_StrLen(param) + 1);
        memcpy(temp, param, AT_StrLen(param));

        uint16_t param_len = AT_StrLen(param);
        CSW_SetAudioZspPreProcessCalibParam(&resultCode, &hasMsg, resultMsg, mode, path, ctrl, temp, param_len);

        memset(temp, 0x00, AT_StrLen(param) + 1);
        free(temp);
        temp = NULL;

        if (resultCode)
        {
            urcBuffer[0] = '\0';
            sprintf(urcBuffer, "ERROR %d", resultCode);
            atCmdRespInfoText(pParam->engine, urcBuffer);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        else
        {
            if (hasMsg)
            {
                urcBuffer[0] = '\0';
                sprintf(urcBuffer, "%s", resultMsg);
                atCmdRespInfoText(pParam->engine, urcBuffer);
            }
            atCmdRespOK(pParam->engine);
        }
    }
    else if (AT_CMD_TEST == pParam->type)
    {
        urcBuffer[0] = '\0';
        sprintf(urcBuffer, "+CAPRE: (0-5),(0-5),(0-5),");
        atCmdRespInfoText(pParam->engine, urcBuffer);
        atCmdRespOK(pParam->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_OPTION_NOT_SURPORT));
    }
    return;
}

void atCmdHandleCAPOST(atCommand_t *pParam)
{
    char urcBuffer[1024] = {0};

    uint8_t resultCode = 0;
    uint8_t hasMsg = 0;
    uint8_t resultMsg[900] = {0};

    uint8_t *temp = NULL;

    if (AT_CMD_SET == pParam->type)
    {
        bool paramok = true;

        if (pParam->param_count != 4)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        uint8_t mode = atParamUint(pParam->params[0], &paramok);
        uint8_t path = atParamUint(pParam->params[1], &paramok);
        uint8_t ctrl = atParamUint(pParam->params[2], &paramok);
        const char *param = atParamStr(pParam->params[3], &paramok);

        if (!paramok || (mode < 0 || mode > 5) || (path < 0 || path > 5) || (ctrl < 0 || ctrl > 5))
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        temp = malloc(AT_StrLen(param) + 1);
        if (temp == NULL)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        memset(temp, 0x00, AT_StrLen(param) + 1);
        memcpy(temp, param, AT_StrLen(param));

        uint16_t param_len = AT_StrLen(param);
        CSW_SetAudioZspPostProcessCalibParam(&resultCode, &hasMsg, resultMsg, mode, path, ctrl, temp, param_len);

        memset(temp, 0x00, AT_StrLen(param) + 1);
        free(temp);
        temp = NULL;

        if (resultCode)
        {
            urcBuffer[0] = '\0';
            sprintf(urcBuffer, "ERROR %d", resultCode);
            atCmdRespInfoText(pParam->engine, urcBuffer);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        else
        {
            if (hasMsg)
            {
                urcBuffer[0] = '\0';
                sprintf(urcBuffer, "%s", resultMsg);
                atCmdRespInfoText(pParam->engine, urcBuffer);
            }
            atCmdRespOK(pParam->engine);
        }
    }
    else if (AT_CMD_TEST == pParam->type)
    {
        urcBuffer[0] = '\0';
        sprintf(urcBuffer, "+CAPOST: (0-5),(0-5),(0-5),");
        atCmdRespInfoText(pParam->engine, urcBuffer);
        atCmdRespOK(pParam->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_OPTION_NOT_SURPORT));
    }
    return;
}

void atCmdHandleCAWTF(atCommand_t *pParam)
{
    char urcBuffer[1024] = {0};

    uint8_t resultCode = 0;
    uint8_t hasMsg = 0;
    uint8_t resultMsg[900] = {0};

    uint8_t *temp = NULL;

    if (AT_CMD_SET == pParam->type)
    {
        bool paramok = true;

        if (pParam->param_count != 3)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        uint8_t path = atParamUint(pParam->params[0], &paramok);
        uint8_t ctrl = atParamUint(pParam->params[1], &paramok);
        const char *param = atParamStr(pParam->params[2], &paramok);

        if (!paramok || path != 0 || ctrl != 0)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        temp = malloc(AT_StrLen(param) + 1);
        if (temp == NULL)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        memset(temp, 0x00, AT_StrLen(param) + 1);
        memcpy(temp, param, AT_StrLen(param));

        uint16_t param_len = AT_StrLen(param);
        CSW_WriteCalibParamToFlash(&resultCode, &hasMsg, resultMsg, path, ctrl, temp, param_len);

        memset(temp, 0x00, AT_StrLen(param) + 1);
        free(temp);
        temp = NULL;

        if (resultCode)
        {
            urcBuffer[0] = '\0';
            sprintf(urcBuffer, "ERROR %d", resultCode);
            atCmdRespInfoText(pParam->engine, urcBuffer);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        else
        {
            if (hasMsg)
            {
                urcBuffer[0] = '\0';
                sprintf(urcBuffer, "%s", resultMsg);
                atCmdRespInfoText(pParam->engine, urcBuffer);
            }
            atCmdRespOK(pParam->engine);
        }
    }
    else if (AT_CMD_TEST == pParam->type)
    {
        urcBuffer[0] = '\0';
        sprintf(urcBuffer, "+CAWTF: (0),(0),");
        atCmdRespInfoText(pParam->engine, urcBuffer);
        atCmdRespOK(pParam->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_OPTION_NOT_SURPORT));
    }
    return;
}

void atCmdHandleCAIET(atCommand_t *pParam)
{
    char urcBuffer[1024] = {0};

    uint8_t resultCode = 0;
    uint8_t hasMsg = 0;
    uint8_t resultMsg[900] = {0};

    uint8_t *temp = NULL;

    if (AT_CMD_SET == pParam->type)
    {
        bool paramok = true;

        if (pParam->param_count != 5)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        uint8_t path = atParamUint(pParam->params[0], &paramok);
        uint8_t ctrl = atParamUint(pParam->params[1], &paramok);
        uint16_t offset = atParamUint(pParam->params[2], &paramok);
        uint16_t length = atParamUint(pParam->params[3], &paramok);
        const char *param = atParamStr(pParam->params[4], &paramok);

        if (!paramok || path != 0 || (ctrl < 0 || ctrl > 1))
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        temp = malloc(AT_StrLen(param) + 1);
        if (temp == NULL)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        memset(temp, 0x00, AT_StrLen(param) + 1);
        memcpy(temp, param, AT_StrLen(param));

        uint16_t param_len = AT_StrLen(param);
        CSW_ExportCalibOrImportCalibFlashParam(&resultCode, &hasMsg, resultMsg, path, ctrl, offset, length, temp, param_len);

        memset(temp, 0x00, AT_StrLen(param) + 1);
        free(temp);
        temp = NULL;

        if (resultCode)
        {
            urcBuffer[0] = '\0';
            sprintf(urcBuffer, "ERROR %d", resultCode);
            atCmdRespInfoText(pParam->engine, urcBuffer);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        else
        {
            if (hasMsg)
            {
                urcBuffer[0] = '\0';
                sprintf(urcBuffer, "%s", resultMsg);
                atCmdRespInfoText(pParam->engine, urcBuffer);
            }
            atCmdRespOK(pParam->engine);
        }
    }
    else if (AT_CMD_TEST == pParam->type)
    {
        urcBuffer[0] = '\0';
        sprintf(urcBuffer, "+CAIET: (0),(0-1),,,");
        atCmdRespInfoText(pParam->engine, urcBuffer);
        atCmdRespOK(pParam->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_OPTION_NOT_SURPORT));
    }
    return;
}

void atCmdHandleCADTF(atCommand_t *pParam)
{
    char urcBuffer[1024] = {0};

    uint8_t resultCode = 0;
    uint8_t hasMsg = 0;
    uint8_t resultMsg[900] = {0};

    uint8_t *temp = NULL;

    if (AT_CMD_SET == pParam->type)
    {
        bool paramok = true;

        if (pParam->param_count != 3)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        uint8_t path = atParamUint(pParam->params[0], &paramok);
        uint8_t ctrl = atParamUint(pParam->params[1], &paramok);
        const char *param = atParamStr(pParam->params[3], &paramok);

        if (!paramok || path != 0 || (ctrl < 0 || ctrl > 1))
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        temp = malloc(AT_StrLen(param) + 1);
        if (temp == NULL)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        memset(temp, 0x00, AT_StrLen(param) + 1);
        memcpy(temp, param, AT_StrLen(param));

        uint16_t param_len = AT_StrLen(param);
        CSW_DumpPcmDataToTflash(&resultCode, &hasMsg, resultMsg, path, ctrl, temp, param_len);

        memset(temp, 0x00, AT_StrLen(param) + 1);
        free(temp);
        temp = NULL;

        if (resultCode)
        {
            urcBuffer[0] = '\0';
            sprintf(urcBuffer, "ERROR %d", resultCode);
            atCmdRespInfoText(pParam->engine, urcBuffer);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        else
        {
            if (hasMsg)
            {
                urcBuffer[0] = '\0';
                sprintf(urcBuffer, "%s", resultMsg);
                atCmdRespInfoText(pParam->engine, urcBuffer);
            }
            atCmdRespOK(pParam->engine);
        }
    }
    else if (AT_CMD_TEST == pParam->type)
    {
        urcBuffer[0] = '\0';
        sprintf(urcBuffer, "+CADTF: (0),(0-1),");
        atCmdRespInfoText(pParam->engine, urcBuffer);
        atCmdRespOK(pParam->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_OPTION_NOT_SURPORT));
    }
    return;
}

void atCmdHandleCAVCT(atCommand_t *pParam)
{
    char urcBuffer[1024] = {0};

    uint8_t resultCode = 0;
    uint8_t hasMsg = 0;
    uint8_t resultMsg[900] = {0};

    uint8_t *temp = NULL;

    if (AT_CMD_SET == pParam->type)
    {
        bool paramok = true;

        if (pParam->param_count != 3)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        uint8_t path = atParamUint(pParam->params[0], &paramok);
        uint8_t ctrl = atParamUint(pParam->params[1], &paramok);
        const char *param = atParamStr(pParam->params[2], &paramok);

        if (!paramok || path != 0 || ctrl != 0)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        temp = malloc(AT_StrLen(param) + 1);
        if (temp == NULL)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        memset(temp, 0x00, AT_StrLen(param) + 1);
        memcpy(temp, param, AT_StrLen(param));

        uint16_t param_len = AT_StrLen(param);
        CSW_VersionControl(&resultCode, &hasMsg, resultMsg, path, ctrl, temp, param_len);

        memset(temp, 0x00, AT_StrLen(param) + 1);
        free(temp);
        temp = NULL;

        if (resultCode)
        {
            urcBuffer[0] = '\0';
            sprintf(urcBuffer, "ERROR %d", resultCode);
            atCmdRespInfoText(pParam->engine, urcBuffer);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        else
        {
            if (hasMsg)
            {
                urcBuffer[0] = '\0';
                sprintf(urcBuffer, "%s", resultMsg);
                atCmdRespInfoText(pParam->engine, urcBuffer);
            }
            atCmdRespOK(pParam->engine);
        }
    }
    else if (AT_CMD_TEST == pParam->type)
    {
        urcBuffer[0] = '\0';
        sprintf(urcBuffer, "+CAVCT: (0),(0),");
        atCmdRespInfoText(pParam->engine, urcBuffer);
        atCmdRespOK(pParam->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_OPTION_NOT_SURPORT));
    }
    return;
}

void atCmdHandleCANXP(atCommand_t *pParam)
{
    char urcBuffer[1024] = {0};

    uint8_t resultCode = 0;
    uint8_t hasMsg = 0;
    uint8_t resultMsg[900] = {0};

    uint8_t *temp = NULL;

    if (AT_CMD_SET == pParam->type)
    {
        bool paramok = true;

        if (pParam->param_count != 4)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        uint8_t mode = atParamUint(pParam->params[0], &paramok);
        uint8_t path = atParamUint(pParam->params[1], &paramok);
        uint8_t ctrl = atParamUint(pParam->params[2], &paramok);
        const char *param = atParamStr(pParam->params[3], &paramok);

        if (!paramok || (mode < 0 || mode > 5) || (path < 0 || path > 5) || (ctrl < 0 || ctrl > 7))
        {
            free(temp);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }

        temp = malloc(AT_StrLen(param) + 1);
        if (temp == NULL)
        {
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        memset(temp, 0x00, AT_StrLen(param) + 1);
        memcpy(temp, param, AT_StrLen(param));

        uint16_t param_len = AT_StrLen(param);
        CSW_SetAudioZspNxpCalibParam(&resultCode, &hasMsg, resultMsg, mode, path, ctrl, temp, param_len);

        memset(temp, 0x00, AT_StrLen(param) + 1);
        free(temp);
        temp = NULL;

        if (resultCode)
        {
            urcBuffer[0] = '\0';
            sprintf(urcBuffer, "ERROR %d", resultCode);
            atCmdRespInfoText(pParam->engine, urcBuffer);
            AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_PARAM_INVALID));
        }
        else
        {
            if (hasMsg)
            {
                urcBuffer[0] = '\0';
                sprintf(urcBuffer, "%s", resultMsg);
                atCmdRespInfoText(pParam->engine, urcBuffer);
            }
            atCmdRespOK(pParam->engine);
        }
    }
    else if (AT_CMD_TEST == pParam->type)
    {
        urcBuffer[0] = '\0';
        sprintf(urcBuffer, "+CANXP: (0-5),(0-5),(0-7),");
        atCmdRespInfoText(pParam->engine, urcBuffer);
        atCmdRespOK(pParam->engine);
    }
    else
    {
        AT_CMD_RETURN(atCmdRespCmeError(pParam->engine, ERR_AT_CME_OPTION_NOT_SURPORT));
    }
    return;
}

static void prvPlayFinish(void *param)
{
    if (gAtAudioCtx.player == NULL)
        return;

    auPlayerStop(gAtAudioCtx.player);
    auPlayerDelete(gAtAudioCtx.player);
    gAtAudioCtx.player = NULL;
}

static void prvPlayEventCallback(void *param, auPlayerEvent_t event)
{
    if (event == AUPLAYER_EVENT_FINISHED)
        atEngineCallback(prvPlayFinish, NULL);
}

void atCmdHandleCAUDPLAY(atCommand_t *cmd)
{
    if (AT_CMD_SET == cmd->type)
    {
        // START:   AT+CAUDPLAY=1,"filename"
        // STOP:    AT+CAUDPLAY=2
        // PAUSE:   AT+CAUDPLAY=3
        // RESUME:  AT+CAUDPLAY=4
        bool paramok = true;
        unsigned oper = atParamUintInRange(cmd->params[0], 1, 4, &paramok);
        if (!paramok)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        if (oper == 1) // start
        {
            const char *fname = atParamStr(cmd->params[1], &paramok);
            if (!paramok || cmd->param_count > 2)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

            auStreamFormat_t format = auStreamFormatBySuffix(fname);
            if (format != AUSTREAM_FORMAT_PCM &&
                format != AUSTREAM_FORMAT_WAVPCM &&
                format != AUSTREAM_FORMAT_MP3 &&
                format != AUSTREAM_FORMAT_AMRNB)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

            if (gAtAudioCtx.player != NULL)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

            gAtAudioCtx.player = auPlayerCreate();
            if (gAtAudioCtx.player == NULL)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);

            auPlayerSetEventCallback(gAtAudioCtx.player, prvPlayEventCallback, NULL);
            if (!auPlayerStartFile(gAtAudioCtx.player, format, NULL, fname))
            {
                auPlayerDelete(gAtAudioCtx.player);
                gAtAudioCtx.player = NULL;
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
            }

            RETURN_OK(cmd->engine);
        }
        else if (oper == 2) // stop
        {
            if (cmd->param_count > 1)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

            if (gAtAudioCtx.player == NULL)
                RETURN_OK(cmd->engine);

            if (!auPlayerStop(gAtAudioCtx.player))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

            auPlayerDelete(gAtAudioCtx.player);
            gAtAudioCtx.player = NULL;
            RETURN_OK(cmd->engine);
        }
        else if (oper == 3)
        {
            if (cmd->param_count > 1)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            if (gAtAudioCtx.player == NULL)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

            if (!auPlayerPause(gAtAudioCtx.player))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

            RETURN_OK(cmd->engine);
        }
        else if (oper == 4)
        {
            if (cmd->param_count > 1)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            if (gAtAudioCtx.player == NULL)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

            if (!auPlayerResume(gAtAudioCtx.player))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

            RETURN_OK(cmd->engine);
        }
        else
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }
    }
    else if (cmd->type == AT_CMD_READ)
    {
        // +CAUDPLAY: (0,1,2)
        // 0/stop 1/play 2/pause (same as auPlayerStatus_t)
        char rsp[64];
        int status = (gAtAudioCtx.player == NULL)
                         ? AUPLAYER_STATUS_IDLE
                         : auPlayerGetStatus(gAtAudioCtx.player);

        sprintf(rsp, "%s: %d", cmd->desc->name, status);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else if (AT_CMD_TEST == cmd->type)
    {
        char rsp[48];
        sprintf(rsp, "%s: (1-4),<filename>", cmd->desc->name);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
    }
}

// ============================================================================

typedef struct
{
    auRecorder_t *recorder;
    int fd;
    osiPipe_t *pipe;
    osiTimer_t *timer;
    osiNotify_t *notify;
    auStreamFormat_t format;
} atAudioRecordContext_t;

static atAudioRecordContext_t gAtRecordCtx = {};

#define AT_RECORD_PIPE_SIZE (4096)

/**
 * Pipe reader callback, executed in audio thread
 */
static void prvRecordPipeReadCallback(void *param, unsigned event)
{
    atAudioRecordContext_t *d = (atAudioRecordContext_t *)param;
    if (event & OSI_PIPE_EVENT_RX_ARRIVED)
        osiNotifyTrigger(d->notify);
}

/**
 * Read from pipe, and write to file
 */
static void prvRecordPipeRead(void *param)
{
    atAudioRecordContext_t *d = (atAudioRecordContext_t *)param;

    if (d->pipe == NULL || d->fd <= 0)
        return;

    char buf[256];
    for (;;)
    {
        int bytes = osiPipeRead(d->pipe, buf, 256);
        if (bytes <= 0)
            break;

        vfs_write(d->fd, buf, bytes);
    }
}

/**
 * Delete all members of recoder
 */
static void prvRecordDelete(atAudioRecordContext_t *d)
{
    prvRecordPipeRead(d);
    auRecorderDelete(d->recorder);
    osiPipeDelete(d->pipe);
    osiNotifyDelete(d->notify);
    osiTimerDelete(d->timer);
    if (d->fd > 0)
    {
        if (d->format == AUSTREAM_FORMAT_WAVPCM)
            auFixWavHeaderByDesc(d->fd);
        vfs_close(d->fd);
    }

    d->recorder = NULL;
    d->pipe = NULL;
    d->notify = NULL;
    d->timer = NULL;
    d->fd = -1;
    d->format = AUSTREAM_FORMAT_UNKNOWN;
}

/**
 * Stop recorder. Return cme error on fail.
 */
static int prvRecordStop(atAudioRecordContext_t *d)
{
    // It is regarded as OK if recording is not started
    if (d->recorder == NULL)
        return 0;

    if (!auRecorderStop(d->recorder))
        return ERR_AT_CME_EXE_FAIL;

    prvRecordDelete(d);
    return 0;
}

/**
 * Recorder timeout, stop recorder.
 */
static void prvRecordTimeout(void *param)
{
    prvRecordStop((atAudioRecordContext_t *)param);
}

/**
 * Voice call closed, stop recorder.
 */
static void prvRecordVoiceClosed(void *param)
{
    prvRecordStop((atAudioRecordContext_t *)param);
}

/**
 * Recorder callback, called by Recorder
 */
static void prvRecordEventCallback(void *param, auRecorderEvent_t event)
{
    if (event == AURECORDER_EVENT_FINISHED)
        atEngineCallback(prvRecordVoiceClosed, param);
}

/**
 * Start recorder. Return cme error on fail.
 */
static int prvRecordStart(atAudioRecordContext_t *d, audevRecordType_t type,
                          auEncodeQuality_t quality, unsigned timeout,
                          const char *fname)
{
    // already started
    if (d->recorder != NULL)
        return ERR_AT_CME_OPERATION_NOT_ALLOWED;

    d->format = auStreamFormatBySuffix(fname);
    if (d->format != AUSTREAM_FORMAT_PCM &&
        d->format != AUSTREAM_FORMAT_WAVPCM &&
        d->format != AUSTREAM_FORMAT_AMRNB)
        return ERR_AT_CME_PARAM_INVALID;

    d->fd = vfs_open(fname, O_RDWR | O_CREAT | O_TRUNC);
    if (d->fd < 0)
        goto failed;

    d->notify = osiNotifyCreate(atEngineGetThreadId(), prvRecordPipeRead, d);
    if (d->notify == NULL)
        goto failed_nomem;

    d->timer = osiTimerCreate(atEngineGetThreadId(), prvRecordTimeout, d);
    if (d->timer == NULL)
        goto failed_nomem;

    d->pipe = osiPipeCreate(AT_RECORD_PIPE_SIZE);
    if (d->pipe == NULL)
        goto failed_nomem;

    osiPipeSetReaderCallback(d->pipe, OSI_PIPE_EVENT_RX_ARRIVED, prvRecordPipeReadCallback, d);

    auEncoderParamSet_t encparams[2] = {};
    if (d->format == AUSTREAM_FORMAT_AMRNB)
    {
        static auAmrnbMode_t gAmrnbQuals[] = {
            AU_AMRNB_MODE_515,  // LOW
            AU_AMRNB_MODE_670,  // MEDIUM
            AU_AMRNB_MODE_795,  // HIGH
            AU_AMRNB_MODE_1220, // BEST
        };
        encparams[0].id = AU_AMRNB_ENC_PARAM_MODE;
        encparams[0].val = &gAmrnbQuals[quality];
    }

    d->recorder = auRecorderCreate();
    if (d->recorder == NULL)
        goto failed_nomem;

    if ((type == AUDEV_RECORD_TYPE_VOICE) || (type == AUDEV_RECORD_TYPE_VOICE_DUAL))
    {
        auRecorderSetEventCallback(d->recorder, prvRecordEventCallback, d);
    }

    if (!auRecorderStartPipe(d->recorder, type, d->format, encparams, d->pipe))
        goto failed;

    if (timeout != 0 && timeout != OSI_WAIT_FOREVER)
        osiTimerStart(d->timer, timeout * 100); // 100ms -> ms

    return 0;

failed_nomem:
    prvRecordDelete(d);
    return ERR_AT_CME_NO_MEMORY;

failed:
    prvRecordDelete(d);
    return ERR_AT_CME_EXE_FAIL;
}

void atCmdHandleCAUDREC(atCommand_t *cmd)
{
    static uint32_t valid_types[] = {1, 2};

    if (AT_CMD_SET == cmd->type)
    {
        // START:   AT+CAUDREC=1,"filename",type[,quality[,time]]
        // STOP:    AT+CMICREC=2
        // type:    1/mic 2/voice 3/voice dual 4/dump
        bool paramok = true;
        unsigned oper = atParamUintInRange(cmd->params[0], 1, 2, &paramok);
        if (!paramok)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        if (oper == 1) // start
        {
            const char *fname = atParamStr(cmd->params[1], &paramok);
            unsigned type = atParamUintInList(cmd->params[2], valid_types, OSI_ARRAY_SIZE(valid_types), &paramok);
            unsigned quality = atParamDefUintInRange(cmd->params[3], 2, 0, 3, &paramok);
            unsigned time = atParamDefUint(cmd->params[4], 0, &paramok);
            if (!paramok || cmd->param_count > 5)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

            int res = prvRecordStart(&gAtRecordCtx,
                                     (audevRecordType_t)type,    // enum matches
                                     (auEncodeQuality_t)quality, // enum matches
                                     time, fname);
            RETURN_OK_CME_ERR(cmd->engine, res);
        }
        else if (oper == 2) // stop
        {
            if (cmd->param_count > 1)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

            int res = prvRecordStop(&gAtRecordCtx);
            RETURN_OK_CME_ERR(cmd->engine, res);
        }
        else
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }
    }
    else if (cmd->type == AT_CMD_READ)
    {
        // +CAUDREC: (0,1)
        char rsp[64];
        bool started = (gAtRecordCtx.recorder != NULL);
        sprintf(rsp, "%s: %d", cmd->desc->name, started ? 1 : 0);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        char rsp[48];
        sprintf(rsp, "%s: (1,2),<filename>,(1,2),(0-3),<time>", cmd->desc->name);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
    }
}

typedef struct
{
    uint8_t *dtmf;
    unsigned duration; // ms
    unsigned count;
    unsigned pos;
    osiTimer_t *timer;
} cdtmfAsyncContext_t;

static void prvCdtmfAsyncDelete(atCommand_t *cmd)
{
    cdtmfAsyncContext_t *ctx = (cdtmfAsyncContext_t *)cmd->async_ctx;
    if (ctx == NULL)
        return;

    osiTimerDelete(ctx->timer);
    free(ctx);
    cmd->async_ctx = NULL;
}

static void prvCdtmfPlay(void *param)
{
    atCommand_t *cmd = (atCommand_t *)param;
    cdtmfAsyncContext_t *ctx = (cdtmfAsyncContext_t *)cmd->async_ctx;

    if (ctx->pos >= ctx->count)
        RETURN_OK(cmd->engine);

    // The minimal duration for tone is 100ms, and the AT command duration
    // is interpreted as the interval of tones.
    uint8_t tone = ctx->dtmf[ctx->pos++];
    if (!audevPlayTone(tone, 100))
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

    osiTimerStart(ctx->timer, ctx->duration);
}

void atCmdHandleCDTMF(atCommand_t *cmd)
{
    if (cmd->type == AT_CMD_SET)
    {
        // AT+CDTMF=<string>,duration_100ms
        bool paramok = true;
        const char *dtmf = atParamDtmf(cmd->params[0], &paramok);
        unsigned duration = atParamUintInRange(cmd->params[1], 1, 10, &paramok);
        size_t len = strlen(dtmf);
        if (!paramok || len == 0 || cmd->param_count > 2)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        cdtmfAsyncContext_t *ctx = (cdtmfAsyncContext_t *)calloc(1, sizeof(*ctx) + len);
        if (ctx == NULL)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);

        cmd->async_ctx = ctx;
        cmd->async_ctx_destroy = prvCdtmfAsyncDelete;
        ctx->dtmf = (uint8_t *)ctx + sizeof(*ctx);
        ctx->duration = duration * 100; // 100ms -> ms
        ctx->count = len;
        ctx->pos = 0;
        ctx->timer = osiTimerCreate(osiThreadCurrent(), prvCdtmfPlay, cmd);
        if (ctx->timer == NULL)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);

        for (int n = 0; n < len; n++)
        {
            int tone = cfwDtmfCharToTone(dtmf[n]);
            if (tone < 0)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_INVALID_CHAR_INTEXT);
            ctx->dtmf[n] = tone;
        }

        osiThreadCallback(atEngineGetThreadId(), prvCdtmfPlay, cmd);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        char rsp[64];
        sprintf(rsp, "%s: (0-9,*,#,A,B,C,D),(1-10)", cmd->desc->name);
        atCmdRespInfoText(cmd->engine, rsp);
        atCmdRespOK(cmd->engine);
    }
    else
    {
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
    }
}

void atCmdHandleLBTEST(atCommand_t *cmd)
{
    if (AT_CMD_SET == cmd->type)
    {
        // AT+LBTEST=<output>,<input>,<start/stop>
        bool paramok = true;
        uint32_t output = atParamUintInRange(cmd->params[0], 0, 2, &paramok);
        uint32_t input = atParamUintInRange(cmd->params[1], 0, 4, &paramok);
        uint8_t start = atParamUintInRange(cmd->params[2], 0, 1, &paramok);
        if (!paramok || cmd->param_count > 3)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        if (start)
            audevStartLoopbackTest((audevOutput_t)output, (audevInput_t)input, 50);
        else
            audevStopLoopbackTest();

        RETURN_OK(cmd->engine);
    }
    else if (AT_CMD_TEST == cmd->type)
    {
        char rsp[48];
        sprintf(rsp, "%s: (0-2),(0-4),(0,1)", cmd->desc->name);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
    }
}

void atCmdHandleCRSL(atCommand_t *cmd)
{
    if (cmd->type == AT_CMD_SET)
    {
        // AT+CRSL=<level>
        bool paramok = true;
        unsigned level = atParamUintInRange(cmd->params[0], 0, 100, &paramok);
        if (!paramok || cmd->param_count > 1)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        if (!audevSetPlayVolume(level))
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ)
    {
        char rsp[64];
        unsigned level = audevGetPlayVolume();
        sprintf(rsp, "%s: %d", cmd->desc->name, level);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        char rsp[64];
        sprintf(rsp, "%s: (0-100)", cmd->desc->name);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
    }
}

#ifdef CONFIG_SOC_8910

void atCmdHandlePCMMODE(atCommand_t *cmd)
{
    if (cmd->type == AT_CMD_SET)
    {
        //AT+PCMMODE=<mode>
        bool paramok = true;
        uint8_t mode = atParamUintInRange(cmd->params[0], 0, 3, &paramok);
        if (cmd->param_count != 1 || !paramok)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }

        HAL_CODEC_CFG_T halCodecCfg = {0};
        int ret = nvmReadAudioCodec(&halCodecCfg, sizeof(HAL_CODEC_CFG_T));
        if (ret == -1)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }

        if (mode == 0)
        {
            halCodecCfg.i2sAifcfg.lsbFirst = true;
            halCodecCfg.i2sAifcfg.rxDelay = 2;
            halCodecCfg.i2sAifcfg.txDelay = 2;
        }
        else if (mode == 1)
        {
            halCodecCfg.i2sAifcfg.lsbFirst = false;
            halCodecCfg.i2sAifcfg.rxDelay = 1;
            halCodecCfg.i2sAifcfg.txDelay = 1;
        }
        else if (mode == 2)
        {
            halCodecCfg.i2sAifcfg.lsbFirst = true;
            halCodecCfg.i2sAifcfg.rxDelay = 1;
            halCodecCfg.i2sAifcfg.txDelay = 1;
        }
        else
        {
            halCodecCfg.i2sAifcfg.lsbFirst = false;
            halCodecCfg.i2sAifcfg.rxDelay = 2;
            halCodecCfg.i2sAifcfg.txDelay = 2;
        }

        ret = nvmWriteAudioCodec(&halCodecCfg, sizeof(HAL_CODEC_CFG_T));
        if (ret == -1)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }

        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        char rsp[64];
        sprintf(rsp, "%s: (0-3)", cmd->desc->name);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
    }
}

void atCmdHandleMAI2SY(atCommand_t *cmd)
{
    const uint32_t fs_ranges[] = {
        8000,
        16000,
        24000,
        32000,
        44100,
    };

    if (cmd->type == AT_CMD_SET)
    {
        //AT+MAI2SY=<master>,<tran_mode>,<sample>,<width>
        bool paramok = true;

        uint8_t master = atParamUintInRange(cmd->params[0], 1, 1, &paramok);
        uint8_t tran_mode = atParamUintInRange(cmd->params[1], 0, 1, &paramok);
        uint8_t sample = atParamUintInRange(cmd->params[2], 0, 4, &paramok);
        // uint8_t width = atParamUintInRange(cmd->params[3], 0, 0, &paramok);
        if (cmd->param_count != 4 || !paramok)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }

        HAL_CODEC_CFG_T halCodecCfg = {0};
        int ret = nvmReadAudioCodec(&halCodecCfg, sizeof(HAL_CODEC_CFG_T));
        if (ret == -1)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }

        halCodecCfg.codecIsMaster = master;
        halCodecCfg.dataFormat = tran_mode;
        halCodecCfg.i2sAifcfg.fs = fs_ranges[sample];

        ret = nvmWriteAudioCodec(&halCodecCfg, sizeof(HAL_CODEC_CFG_T));
        if (ret == -1)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }

        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ)
    {
        HAL_CODEC_CFG_T halCodecCfg = {0};
        int ret = nvmReadAudioCodec(&halCodecCfg, sizeof(HAL_CODEC_CFG_T));
        if (ret == -1)
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
        }

        uint8_t value = 0;
        for (; value < sizeof(fs_ranges) / sizeof(fs_ranges[0]); value++)
        {
            if (fs_ranges[value] == halCodecCfg.i2sAifcfg.fs)
                break;
        }

        char rsp[100] = {0};
        sprintf(rsp, "+MAI2SY: %d,%d,%d,0", halCodecCfg.codecIsMaster, halCodecCfg.dataFormat, value);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST)
    {
        char rsp[100] = {0};
        sprintf(rsp, "+MAI2SY: 1,(0-1),(0-4),0");
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
}

void atCmdHandleCLAM(atCommand_t *cmd)
{
    char rsp[64];

    if (cmd->type == AT_CMD_SET)
    {
        bool paramok = true;
        uint8_t nCallmode = atParamUintInRange(cmd->params[0], 0, 1, &paramok);

        if (!paramok || cmd->param_count > 2)
        {
            sprintf(rsp, "+CME ERROR: err");
            atCmdRespInfoText(cmd->engine, rsp);
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }
        uint8_t nSmsmode = atParamUintInRange(cmd->params[1], 0, 1, &paramok);
        if (!paramok)
        {
            sprintf(rsp, "+CME ERROR: err");
            atCmdRespInfoText(cmd->engine, rsp);
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        }
        gAtSetting.callmode = nCallmode;
        gAtSetting.smsmode = nSmsmode;
        atCfgAutoSave();
        OSI_LOGI(0, "CLAM: callmode %d, smsmode %d", gAtSetting.callmode, gAtSetting.smsmode);

        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ) //AT+CLAM?
    {
        sprintf(rsp, "+CLAM: %d, %d", gAtSetting.callmode, gAtSetting.smsmode);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST) //AT+CLAM=?
    {
        char rsp[64];
        sprintf(rsp, "+CLAM: (0-1), (0-1)");
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
    }
}

#endif

void atAudioInit(void)
{
    gAtAudioCtx.player = NULL;
}

#endif // CONFIG_AT_AUDIO_SUPPORT
