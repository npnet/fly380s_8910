/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
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
#include "cfw.h"

#ifdef CONFIG_AT_MY_ACCOUNT_SUPPORT
#include "osi_api.h"
#include "osi_log.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "at_response.h"
#include "at_cfw.h"
#include "at_engine.h"
#include "at_command.h"
#include "cfw.h"
#include "at_cmd_mynet.h"
#include "at_cfg.h"
#include "sockets.h"
#include "at_cfg.h"
#include "ppp_interface.h"
#include "lv_include/lv_poc_lib.h"
#include "guiOemCom_api.h"


void atCmdHandleLOGACCOUNT(atCommand_t *cmd)
{
    char rspStr[200];
    bool paramok = true;
    char *pocparam = NULL;
    //int userOpt = 0;

    if(cmd->type == AT_CMD_TEST)
    {
        atCmdRespInfoText(cmd->engine, "+LOGACCOUNT=<opt>\n<pocparam>");
        atCmdRespOK(cmd->engine);
        return;
    }

    nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
    if(poc_config == NULL)
    {
        atCmdRespInfoText(cmd->engine, "can not read nvm");
        atCmdRespOK(cmd->engine);
        return;
    }

    switch (cmd->type)
    {
    case AT_CMD_EXE:
        strcpy(rspStr, "pocparam:");
        strcat(rspStr, poc_config->account_name);
        do
        {
            if(lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_EXIT_IND, NULL))
            {
                strcat(rspStr, "\nexit log:");
            }

            if(!lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_LOGIN_IND, NULL))
            {
                break;
            }
            strcat(rspStr, "\nrestart log:");
        }while(0);

        atCmdRespInfoText(cmd->engine, rspStr);
        atCmdRespOK(cmd->engine);
        break;

    case AT_CMD_READ:
        strcpy(rspStr, "pocparam:");
        strcat(rspStr, poc_config->account_name);
        atCmdRespInfoText(cmd->engine, rspStr);
        atCmdRespOK(cmd->engine);
        break;

    case AT_CMD_SET:
        do{
            if(cmd->param_count < 1)
            {
                atCmdRespInfoText(cmd->engine, "too low params\n");
                break;
            }
            else if(cmd->param_count == 1)
            {
                do{
                    if (!paramok)
                    {
                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                        break;
                    }

                    pocparam = (char *)atParamStr(cmd->params[0], &paramok);

					if(!paramok || strlen((char *)pocparam) > 128)
                    {
                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                        break;
                    }

					if(NULL != strstr((const char *)pocparam, (const char *)"info"))
					{
						char pocinfo[256];
						nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
						strcpy(pocinfo, "your'info:");
						strcat(pocinfo, poc_config->poc_info);
						atCmdRespInfoText(cmd->engine, pocinfo);
						break;
					}
                    else
                    {
						lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SETPOC_IND, (void *)pocparam);
						OSI_LOGXI(OSI_LOGPAR_SI, 0, "[poc][at]pocParam is %s", pocparam);
					}
                }while(0);
                break;
            }

            if(pocparam != NULL)
                strcpy(poc_config->account_name, (char *)pocparam);

            if(lv_poc_setting_conf_write() < 1)
            {
                atCmdRespInfoText(cmd->engine, "can not write nvm\n");
            }
        }while(0);
        atCmdRespOK(cmd->engine);
        break;

    default:
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_NOT_SURPORT);
        break;
    }
}


#endif
