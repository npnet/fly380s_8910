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
#include "audio_tonegen.h"
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
    audevPlayType_t type;
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

typedef uint32_t (*PFN_AT_CC_CB)(const osiEvent_t *event);

extern PFN_AT_CC_CB pAT_CC_SPEECH_CALL_IND_CB;
extern PFN_AT_CC_CB pAT_CC_RELEASE_CALL_IND_CB;
extern PFN_AT_CC_CB pAT_CC_PROGRESS_IND_CB;
extern PFN_AT_CC_CB pAT_CC_ERROR_IND_CB;
extern PFN_AT_CC_CB pAT_CC_CALL_INFO_IND_CB;
extern PFN_AT_CC_CB pAT_CC_CRSSINFO_IND_CB;
extern PFN_AT_CC_CB pAT_CC_AUDIOON_IND_CB;
extern PFN_AT_CC_CB pAT_CC_ALERT_IND_CB;
extern PFN_AT_CC_CB pAT_CC_ACCEPT_SPEECH_CALL_RSP_CB;
extern PFN_AT_CC_CB pAT_CC_CALL_PATH_IND_CB;
extern PFN_AT_CC_CB pAT_CC_CALL_TI_ASSIGNED_IND_CB;
extern PFN_AT_CC_CB pAT_CC_AUDIO_RESTART_IND_CB;
extern PFN_AT_CC_CB pAT_CC_INITIATE_SPEECH_CALL_RSP_CB;
extern PFN_AT_CC_CB pAT_CC_RELEASE_CALL_RSP_CB;

/*
Check if Call is running,
local audio application not allowed when,when call is running
*/
extern uint8_t AT_CC_GetCCCount(uint8_t sim);

static bool prvIsAudioInCallMode()
{
    uint8_t cnt = AT_CC_GetCCCount(CFW_SIM_0);
    if (cnt)
        return true;
    else
        return false;
}

static bool prvAudioRegisterCCEventCB(PFN_AT_CC_CB pAT_CC_CB)
{
    if (pAT_CC_CB)
    {
        if ((pAT_CC_SPEECH_CALL_IND_CB != NULL) || (pAT_CC_RELEASE_CALL_IND_CB != NULL) || (pAT_CC_PROGRESS_IND_CB != NULL) || (pAT_CC_ERROR_IND_CB != NULL) || (pAT_CC_CALL_INFO_IND_CB != NULL) || (pAT_CC_CRSSINFO_IND_CB != NULL) || (pAT_CC_AUDIOON_IND_CB != NULL) || (pAT_CC_ALERT_IND_CB != NULL) || (pAT_CC_ACCEPT_SPEECH_CALL_RSP_CB != NULL) || (pAT_CC_CALL_PATH_IND_CB != NULL) || (pAT_CC_CALL_TI_ASSIGNED_IND_CB != NULL) || (pAT_CC_AUDIO_RESTART_IND_CB != NULL) || (pAT_CC_INITIATE_SPEECH_CALL_RSP_CB != NULL) || (pAT_CC_RELEASE_CALL_RSP_CB != NULL))
            return false;
    }

    //when call mt or call mo,stop local player
    pAT_CC_SPEECH_CALL_IND_CB = pAT_CC_CB;
    pAT_CC_RELEASE_CALL_IND_CB = pAT_CC_CB;
    pAT_CC_PROGRESS_IND_CB = pAT_CC_CB;
    pAT_CC_ERROR_IND_CB = pAT_CC_CB;
    pAT_CC_CALL_INFO_IND_CB = pAT_CC_CB;
    pAT_CC_CRSSINFO_IND_CB = pAT_CC_CB;
    pAT_CC_AUDIOON_IND_CB = pAT_CC_CB;
    pAT_CC_ALERT_IND_CB = pAT_CC_CB;
    pAT_CC_ACCEPT_SPEECH_CALL_RSP_CB = pAT_CC_CB;
    pAT_CC_CALL_PATH_IND_CB = pAT_CC_CB;
    pAT_CC_CALL_TI_ASSIGNED_IND_CB = pAT_CC_CB;
    pAT_CC_AUDIO_RESTART_IND_CB = pAT_CC_CB;
    pAT_CC_INITIATE_SPEECH_CALL_RSP_CB = pAT_CC_CB;
    pAT_CC_RELEASE_CALL_RSP_CB = pAT_CC_CB;
    return true;
}

static void prvPlayFinish(void *param)
{
    if (gAtAudioCtx.player == NULL)
        return;
    prvAudioRegisterCCEventCB(NULL);
    auPlayerStop(gAtAudioCtx.player);
    auPlayerDelete(gAtAudioCtx.player);
    gAtAudioCtx.player = NULL;
}

static void prvPlayEventCallback(void *param, auPlayerEvent_t event)
{
    if (event == AUPLAYER_EVENT_FINISHED)
        atEngineCallback(prvPlayFinish, NULL);
}

/**
 * Stop player. Return cme error on fail.
 */
static int prvPlayerStop(atAudioContext_t *d)
{
    // It is regarded as OK if recording is not started
    if (d->player == NULL)
        return 0;

    if (!auPlayerStop(d->player))
        return ERR_AT_CME_EXE_FAIL;

    auPlayerDelete(d->player);
    d->player = NULL;
    return 0;
}

uint32_t auLocalPlayerOnCCEventCB(const osiEvent_t *event)
{
    //stop LocalPlayer when call is comming
    if (atCheckCfwEvent(event, EV_CFW_CC_SPEECH_CALL_IND))
    {
        prvPlayerStop(&gAtAudioCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop LocalPlayer when call is outgoing
    if (atCheckCfwEvent(event, EV_CFW_CC_CALL_TI_ASSIGNED_IND))
    {
        prvPlayerStop(&gAtAudioCtx);
        prvAudioRegisterCCEventCB(NULL);
    }

    return 0;
}

uint32_t auCallPlayerOnCCEventCB(const osiEvent_t *event)
{
    //stop CallPlayer when call is comming
    if (atCheckCfwEvent(event, EV_CFW_CC_SPEECH_CALL_IND))
    {
        prvPlayerStop(&gAtAudioCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop CallPlayer when call is outgoing
    if (atCheckCfwEvent(event, EV_CFW_CC_CALL_TI_ASSIGNED_IND))
    {
        prvPlayerStop(&gAtAudioCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop CallPlayer when ATD stop call
    if (atCheckCfwEvent(event, EV_CFW_CC_RELEASE_CALL_RSP))
    {
        prvPlayerStop(&gAtAudioCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop CallPlayer when remote stop call
    if (atCheckCfwEvent(event, EV_CFW_CC_RELEASE_CALL_IND))
    {
        prvPlayerStop(&gAtAudioCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop CallPlayer when call error
    if (atCheckCfwEvent(event, EV_CFW_CC_ERROR_IND))
    {
        prvPlayerStop(&gAtAudioCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop CallPlayer when volte call switch to gsm
    if (atCheckCfwEvent(event, EV_CFW_CC_AUDIO_RESTART_IND))
    {
        prvPlayerStop(&gAtAudioCtx);
        prvAudioRegisterCCEventCB(NULL);
    }

    return 0;
}

void atCmdHandleCAUDPLAY(atCommand_t *cmd)
{
    if (AT_CMD_SET == cmd->type)
    {
        // START:   AT+CAUDPLAY=1,"filename"[,<type>]
        // STOP:    AT+CAUDPLAY=2
        // PAUSE:   AT+CAUDPLAY=3
        // RESUME:  AT+CAUDPLAY=4
        // type:    1/local audio path 2/voice call path default set to 1
        unsigned type = 1;
        bool paramok = true;
        unsigned oper = atParamUintInRange(cmd->params[0], 1, 4, &paramok);
        if (!paramok)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

        if (oper == 1) // start
        {
            const char *fname = atParamStr(cmd->params[1], &paramok);
            if (!paramok || cmd->param_count > 3)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

            if (cmd->param_count == 3)
            {
                static uint32_t valid_types[] = {1, 2};
                type = atParamUintInList(cmd->params[2], valid_types, OSI_ARRAY_SIZE(valid_types), &paramok);
                if (!paramok)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            }

            //local play is not allowed,during call
            if ((prvIsAudioInCallMode() == true) && (type == 1))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

            //fix Bug 1326270 local play is not allowed,when output path is receiver
            audevOutput_t outdev = audevGetOutput();
            if ((outdev == AUDEV_OUTPUT_RECEIVER) && (type == 1))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

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

            gAtAudioCtx.type = type;

            if (gAtAudioCtx.type == 1)
            {
                if (prvAudioRegisterCCEventCB(auLocalPlayerOnCCEventCB) == false)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
            }
            else if (gAtAudioCtx.type == 2)
            {
                if (prvAudioRegisterCCEventCB(auCallPlayerOnCCEventCB) == false)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
            }

            auPlayerSetEventCallback(gAtAudioCtx.player, prvPlayEventCallback, NULL);
            if (!auPlayerStartFileV2(gAtAudioCtx.player, (audevPlayType_t)type, format, NULL, fname))
            {
                auPlayerDelete(gAtAudioCtx.player);
                gAtAudioCtx.player = NULL;
                prvAudioRegisterCCEventCB(NULL);
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
            }

            RETURN_OK(cmd->engine);
        }
        else if (oper == 2) // stop
        {
            if (cmd->param_count > 1)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);

            int res = prvPlayerStop(&gAtAudioCtx);
            prvAudioRegisterCCEventCB(NULL);
            RETURN_OK_CME_ERR(cmd->engine, res);
        }
        else if (oper == 3)
        {
            if (cmd->param_count > 1)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            if (gAtAudioCtx.player == NULL)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

            if (!auPlayerPause(gAtAudioCtx.player))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

            prvAudioRegisterCCEventCB(NULL);

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

            if (gAtAudioCtx.type == 1)
            {
                if (prvAudioRegisterCCEventCB(auLocalPlayerOnCCEventCB) == false)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
            }
            else if (gAtAudioCtx.type == 2)
            {
                if (prvAudioRegisterCCEventCB(auCallPlayerOnCCEventCB) == false)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
            }

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
        sprintf(rsp, "%s: (1-4),<filename>,(1-2)", cmd->desc->name);
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
    audevRecordType_t type;
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

        if (vfs_write(d->fd, buf, bytes) <= 0)
            osiPipeSetEof(d->pipe);
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
    prvAudioRegisterCCEventCB(NULL);
}

/**
 * Voice call closed, stop recorder.
 */
static void prvRecordVoiceClosed(void *param)
{
    prvRecordStop((atAudioRecordContext_t *)param);
    prvAudioRegisterCCEventCB(NULL);
}

/**
 * Recorder callback, called by Recorder
 */
static void prvRecordEventCallback(void *param, auRecorderEvent_t event)
{
    if (event == AURECORDER_EVENT_FINISHED)
        atEngineCallback(prvRecordVoiceClosed, param);
}

uint32_t auLocalRecorderOnCCEventCB(const osiEvent_t *event)
{
    //stop LocalRecorder when call is comming
    if (atCheckCfwEvent(event, EV_CFW_CC_SPEECH_CALL_IND))
    {
        prvRecordStop(&gAtRecordCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop LocalRecorder when call is outgoing
    if (atCheckCfwEvent(event, EV_CFW_CC_CALL_TI_ASSIGNED_IND))
    {
        prvRecordStop(&gAtRecordCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    return 0;
}

uint32_t auCallRecorderOnCCEventCB(const osiEvent_t *event)
{
    //stop LocalRecorder when call is comming
    if (atCheckCfwEvent(event, EV_CFW_CC_SPEECH_CALL_IND))
    {
        prvRecordStop(&gAtRecordCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop LocalRecorder when call is outgoing
    if (atCheckCfwEvent(event, EV_CFW_CC_CALL_TI_ASSIGNED_IND))
    {
        prvRecordStop(&gAtRecordCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop LocalRecorder when ATD stop call
    if (atCheckCfwEvent(event, EV_CFW_CC_RELEASE_CALL_RSP))
    {
        prvRecordStop(&gAtRecordCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop LocalRecorder when remote stop call
    if (atCheckCfwEvent(event, EV_CFW_CC_RELEASE_CALL_IND))
    {
        prvRecordStop(&gAtRecordCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //stop LocalRecorder when call error
    if (atCheckCfwEvent(event, EV_CFW_CC_ERROR_IND))
    {
        prvRecordStop(&gAtRecordCtx);
        prvAudioRegisterCCEventCB(NULL);
    }
    //restart voicerecord when volte call switch to gsm
    if (atCheckCfwEvent(event, EV_CFW_CC_AUDIO_RESTART_IND))
    {
        if (audevRestartVoiceRecord() == false)
        {
            prvRecordStop(&gAtRecordCtx);
            prvAudioRegisterCCEventCB(NULL);
        }
    }
    return 0;
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
    //local record is not allowed,during call
    if ((prvIsAudioInCallMode() == true) && (type == AUDEV_RECORD_TYPE_MIC))
        return ERR_AT_CME_OPERATION_NOT_ALLOWED;
    //call record is not allowed,during idle
    if ((prvIsAudioInCallMode() == false) && (type != AUDEV_RECORD_TYPE_MIC))
        return ERR_AT_CME_OPERATION_NOT_ALLOWED;
    //set call event callback handle
    if (type == AUDEV_RECORD_TYPE_MIC)
    {
        if (prvAudioRegisterCCEventCB(auLocalRecorderOnCCEventCB) == false)
            return ERR_AT_CME_OPERATION_NOT_ALLOWED;
    }
    if (type != AUDEV_RECORD_TYPE_MIC)
    {
        if (prvAudioRegisterCCEventCB(auCallRecorderOnCCEventCB) == false)
            return ERR_AT_CME_OPERATION_NOT_ALLOWED;
    }

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

    if ((type == AUDEV_RECORD_TYPE_VOICE) || (type == AUDEV_RECORD_TYPE_VOICE_DUAL) || (type == AUDEV_RECORD_TYPE_MIC))
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
    prvAudioRegisterCCEventCB(NULL);
    return ERR_AT_CME_NO_MEMORY;

failed:
    prvRecordDelete(d);
    prvAudioRegisterCCEventCB(NULL);
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
        unsigned oper = atParamUintInRange(cmd->params[0], 1, 4, &paramok);
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

            gAtRecordCtx.type = type;
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
            prvAudioRegisterCCEventCB(NULL);
            RETURN_OK_CME_ERR(cmd->engine, res);
        }
        else if (oper == 3)
        {
            if (cmd->param_count > 1)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            if (gAtRecordCtx.recorder == NULL)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

            if (!auRecorderPause(gAtRecordCtx.recorder))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

            prvAudioRegisterCCEventCB(NULL);

            RETURN_OK(cmd->engine);
        }
        else if (oper == 4)
        {
            if (cmd->param_count > 1)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
            if (gAtRecordCtx.recorder == NULL)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

            if (!auRecorderResume(gAtRecordCtx.recorder))
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);

            if (gAtRecordCtx.type == AUDEV_RECORD_TYPE_MIC)
            {
                if (prvAudioRegisterCCEventCB(auLocalRecorderOnCCEventCB) == false)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
            }
            if (gAtRecordCtx.type != AUDEV_RECORD_TYPE_MIC)
            {
                if (prvAudioRegisterCCEventCB(auCallRecorderOnCCEventCB) == false)
                    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);
            }
            RETURN_OK(cmd->engine);
        }

        else
        {
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
        }
    }
    else if (cmd->type == AT_CMD_READ)
    {
        // +CAUDREC: (0,1,2)
        // 0/stop 1/record 2/pause (same as auRecorderStatus_t)
        char rsp[64];
        int status = (gAtRecordCtx.recorder == NULL)
                         ? AURECORDER_STATUS_IDLE
                         : auRecorderGetStatus(gAtRecordCtx.recorder);
        sprintf(rsp, "%s: %d", cmd->desc->name, status);
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

/**
 * PipePlayer for some stream audio app use.
 */

typedef struct
{
    auStreamFormat_t format;
    auPlayer_t *player;
    osiPipe_t *pipe;
} auPipePlayerContext_t;

#define PIPEPLAYER_PIPE_SIZE (2048)
#define PIPEPLAYER_WRITE_PIPE_TMEOUT (500)
#define PIPEPLAYER_AUSTREAM_FORMAT_PCM_SRATE 16000

static bool auPipePlayerPipeWrite(auPipePlayerContext_t *d, const void *data, unsigned size)
{
    if (d->pipe == NULL)
        return false;

    int written = osiPipeWriteAll(d->pipe, data, size, PIPEPLAYER_WRITE_PIPE_TMEOUT);
    OSI_LOGD(0, "pcm write size/%d written/%d", size, written);
    return (written == size);
}

static void auPipePlayerDelete(auPipePlayerContext_t *d)
{
    auPlayerDelete(d->player);
    osiPipeDelete(d->pipe);
    d->player = NULL;
    d->pipe = NULL;
    free(d);
    d = NULL;
}

static auPipePlayerContext_t *auPipePlayerCreate(auStreamFormat_t format)
{
    if (format != AUSTREAM_FORMAT_PCM &&
        format != AUSTREAM_FORMAT_WAVPCM &&
        format != AUSTREAM_FORMAT_MP3 &&
        format != AUSTREAM_FORMAT_AMRNB &&
        format != AUSTREAM_FORMAT_AMRWB)
        return NULL;

    auPipePlayerContext_t *d = (auPipePlayerContext_t *)calloc(1, sizeof(*d));
    if (d == NULL)
        goto failed_nomem;

    d->pipe = osiPipeCreate(PIPEPLAYER_PIPE_SIZE);
    if (d->pipe == NULL)
        goto failed_nomem;

    osiPipeReset(d->pipe);

    d->player = auPlayerCreate();
    if (d->player == NULL)
        goto failed_nomem;

    if (format == AUSTREAM_FORMAT_PCM)
    {
        auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = PIPEPLAYER_AUSTREAM_FORMAT_PCM_SRATE, .channel_count = 1};
        auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};

        if (!auPlayerStartPipeV2(d->player, AUDEV_PLAY_TYPE_LOCAL, format, params, d->pipe))
            goto failed;
        return d;
    }
    else
    {
        if (!auPlayerStartPipeV2(d->player, AUDEV_PLAY_TYPE_LOCAL, format, NULL, d->pipe))
            goto failed;
        return d;
    }
failed_nomem:
    auPipePlayerDelete(d);
    return NULL;

failed:
    auPipePlayerDelete(d);
    return NULL;
}

/**
 * DTMF TONE Generate by software,write gen pcm date to pipeplayer
 * before use this case,we need call auPipePlayerCreate start a pipe player
 */

#define DTMF_TONE_PCM_SAMPLE_RATE (PIPEPLAYER_AUSTREAM_FORMAT_PCM_SRATE)
#define DTMF_TONE_PCM_1MS_SAMPLEBYTES (DTMF_TONE_PCM_SAMPLE_RATE * 2 / 1000)
#define DTMF_TONE_GAIN (0x2000) // -6db

typedef bool (*auTonePlayerCB_t)(void *ctx, const void *data, unsigned size);
typedef struct
{
    auToneGen_t *tonegen; //tone software genenrator
    auTonePlayerCB_t cb;
    void *cb_ctx;
} auTonePlayerContext_t;

static auTonePlayerContext_t gauTonePlayerCtx = {0};

void auTonePlaySetWriter(auTonePlayerCB_t cb, void *ctx)
{
    auTonePlayerContext_t *p = &gauTonePlayerCtx;
    p->cb = cb;
    p->cb_ctx = ctx;
}
static bool auTonePlay(unsigned low_freq, unsigned high_freq, unsigned duration, unsigned silence)
{
    auTonePlayerContext_t *p = &gauTonePlayerCtx;
    short samples[DTMF_TONE_PCM_1MS_SAMPLEBYTES];
    uint32_t tonebytes = DTMF_TONE_PCM_1MS_SAMPLEBYTES;
    auFrame_t frame;
    frame.data = (uintptr_t)samples;
    auToneGen_t *tonegen = p->tonegen;
    if (!auToneGenStart(tonegen, DTMF_TONE_GAIN,
                        low_freq, high_freq,
                        duration, silence))
        return false;

    for (int i = 0; i < (duration + silence); i++)
    {
        frame.bytes = 0;
        int bytes = auToneGenSamples(tonegen, &frame, tonebytes);
        if (bytes <= 0)
            break;
        OSI_LOGD(0, "auToneGenSamples, bytes %d tonebytes %d", bytes, tonebytes);

        if (p->cb)
        {
            if (!p->cb(p->cb_ctx, samples, bytes))
                return false;
        }
    }
    return true;
}

static bool auDTMFTonePlay(uint8_t tone, uint16_t duration, uint16_t silence)
{
    if (tone > 15)
        return false;
    return (auTonePlay(gDtmfToneFreq[tone][0], gDtmfToneFreq[tone][1], duration, silence));
}

static bool auDTMFSPlay(const uint8_t *dtmf, unsigned count, uint16_t duration)
{
    auTonePlayerContext_t *p = &gauTonePlayerCtx;

    if (dtmf == NULL)
        return false;

    if (count == 0)
        return true;

    p->tonegen = auToneGenCreate(PIPEPLAYER_AUSTREAM_FORMAT_PCM_SRATE);
    if (p->tonegen == NULL)
        return false;

    for (int i = 0; i < count; i++)
    {
        int tone = (int)(dtmf[i]) & 0xf;
        if (!auDTMFTonePlay(tone, duration, 0))
        {
            auToneGenDelete(p->tonegen);
            OSI_LOGD(0, "auDTMFTonePlay, !auPlayDTMFTone");
            return false;
        }
    }
    auToneGenDelete(p->tonegen);
    return true;
}

typedef struct
{
    uint8_t *dtmf;
    unsigned duration; // ms
    unsigned count;
    unsigned pos;
    auPipePlayerContext_t *d;
} cdtmfAsyncContext_t;

static void prvCdtmfPlay(void *param)
{
    atCommand_t *cmd = (atCommand_t *)param;
    cdtmfAsyncContext_t *ctx = (cdtmfAsyncContext_t *)cmd->async_ctx;
    auPipePlayerContext_t *d = ctx->d;

    //set pipeplayer callback
    auTonePlaySetWriter((auTonePlayerCB_t)auPipePlayerPipeWrite, d);

    if (!auDTMFSPlay(ctx->dtmf, ctx->count, ctx->duration))
    {
        OSI_LOGE(0, "auDTMFTonePlay failed, stop pipe");
        osiPipeStop(d->pipe);
        goto failed;
    }
    osiPipeSetEof(d->pipe);
    auPlayerWaitFinish(d->player, OSI_WAIT_FOREVER);
    osiPipeStop(d->pipe);
    auPipePlayerDelete(d);
    auTonePlaySetWriter(NULL, NULL);
    RETURN_OK(cmd->engine);

failed:
    OSI_LOGI(0, "prvCdtmfPlay failed");
    auPipePlayerDelete(d);
    auTonePlaySetWriter(NULL, NULL);
    RETURN_CME_ERR(cmd->engine, ERR_AT_CME_EXE_FAIL);
}

static void prvCdtmfAsyncDelete(atCommand_t *cmd)
{
    cdtmfAsyncContext_t *ctx = (cdtmfAsyncContext_t *)cmd->async_ctx;
    if (ctx == NULL)
        return;

    free(ctx);
    cmd->async_ctx = NULL;
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

        //local DTMF play is not allowed,during call
        if (prvIsAudioInCallMode() == true)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

        cdtmfAsyncContext_t *ctx = (cdtmfAsyncContext_t *)calloc(1, sizeof(*ctx) + len);
        if (ctx == NULL)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);

        cmd->async_ctx = ctx;
        cmd->async_ctx_destroy = prvCdtmfAsyncDelete;
        ctx->dtmf = (uint8_t *)ctx + sizeof(*ctx);
        ctx->duration = duration * 100; // 100ms -> ms
        ctx->count = len;
        ctx->pos = 0;
        for (int n = 0; n < len; n++)
        {
            int tone = cfwDtmfCharToTone(dtmf[n]);
            if (tone < 0)
                RETURN_CME_ERR(cmd->engine, ERR_AT_CME_INVALID_CHAR_INTEXT);
            ctx->dtmf[n] = tone;
        }

        ctx->d = auPipePlayerCreate(AUSTREAM_FORMAT_PCM);
        if (ctx->d == NULL)
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_NO_MEMORY);
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

void atCmdHandleCALM(atCommand_t *cmd)
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
        OSI_LOGI(0, "CALM: callmode %d, smsmode %d", gAtSetting.callmode, gAtSetting.smsmode);

        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_READ) //AT+CLAM?
    {
        sprintf(rsp, "+CALM: %d, %d", gAtSetting.callmode, gAtSetting.smsmode);
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else if (cmd->type == AT_CMD_TEST) //AT+CLAM=?
    {
        char rsp[64];
        sprintf(rsp, "+CALM: (0-1), (0-1)");
        atCmdRespInfoText(cmd->engine, rsp);
        RETURN_OK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
    }
}

#endif

void atCmdHandleMMICG(atCommand_t *cmd)
{
    if (cmd->type == AT_CMD_SET)
    {
        uint8_t resultCode = 0;
        uint8_t hasMsg = 0;
        uint8_t resultMsg[16] = {0};
        uint8_t mode = 0;
        uint8_t path = 0;
        uint8_t ctrl = 0;
        const char *gain = NULL;

        bool paramok = true;
        path = atParamUintInRange(cmd->params[0], 0, 5, &paramok);
        gain = atParamStr(cmd->params[1], &paramok);
        if (cmd->param_count != 2 || !paramok)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));

        uint8_t cnt = AT_CC_GetCCCount(CFW_SIM_0);
        if (cnt)
        {
            ctrl = 0;
        }
        else if (gAtRecordCtx.recorder != NULL)
        {
            mode = 2;
            ctrl = 6;
        }
        else
            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPERATION_NOT_ALLOWED);

        const char *param = gain;
        CSW_SetAndGetMicGain(&resultCode, &hasMsg, resultMsg, mode, path, ctrl, (uint8_t *)param, AT_StrLen(param));

        OSI_LOGI(0, "+MMICG resultCode: %d", resultCode);
        if (resultCode)
            AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_PARAM_INVALID));
        else
            RETURN_OK(cmd->engine);
    }
    else
    {
        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_OPTION_NOT_SURPORT);
    }
}

void atAudioInit(void)
{
    gAtAudioCtx.player = NULL;
}

#endif // CONFIG_AT_AUDIO_SUPPORT
