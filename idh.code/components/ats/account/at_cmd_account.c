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
#include "guiIdtCom_api.h"


void atCmdHandleLOGACCOUNT(atCommand_t *cmd)
{
	char rspStr[200];
	char tempStr[10];
	bool paramok = true;
	char *userName = NULL;
	char *userPasswd = NULL;
	char *ip_address = NULL;
	int ip_port = -1;
	int userOpt = 0;

	if(cmd->type == AT_CMD_TEST)
	{
        atCmdRespInfoText(cmd->engine, "+LOGACCOUNT=<opt>\n<account>,<passwd>,[<ip>[,<port>]]");
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
	    strcpy(rspStr, "account:");
	    strcat(rspStr, poc_config->account_name);
	    strcat(rspStr, "\npasswd:");
	    strcat(rspStr, poc_config->account_passwd);
	    do
	    {
		    if(lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_EXIT_IND, NULL))
		    {
			    strcat(rspStr, "\nexit log:");
			    //lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_DELAY_IND, (void *)2000);
			    //strcat(rspStr, "\nwait a moment:");
		    }

		    if(!lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND, NULL))
		    {
			    break;
		    }
		    strcat(rspStr, "\nrestart log:");
	    }while(0);

        atCmdRespInfoText(cmd->engine, rspStr);
        atCmdRespOK(cmd->engine);
        break;

    case AT_CMD_READ:
	    strcpy(rspStr, "account:");
	    strcat(rspStr, poc_config->account_name);
	    strcat(rspStr, "\npasswd:");
	    strcat(rspStr, poc_config->account_passwd);
	    strcat(rspStr, "\nip:");
	    strcat(rspStr, poc_config->ip_address);
	    strcat(rspStr, "\nport:");
	    sprintf(tempStr, "%d", poc_config->ip_port);
	    strcat(rspStr, tempStr);
	    strcat(rspStr, "\nstatus:");
	    sprintf(tempStr, "%d", lvPocGuiIdtCom_get_status());
	    strcat(rspStr, tempStr);
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
				    userOpt = atParamInt(cmd->params[0], &paramok);
					if (!paramok)
					{
						RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
						break;
					}
					if(userOpt < 1)
					{
						if(lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_EXIT_IND, NULL))
						{
							atCmdRespInfoText(cmd->engine, "+LOGACCOUNT:exit log\n");
						}
					}
					else
					{
						if(lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_LOGIN_IND, NULL))
						{
							atCmdRespInfoText(cmd->engine, "+LOGACCOUNT:restart log\n");
						}
					}
			    }while(0);
			    break;
		    }
		    else if(cmd->param_count >= 2)
		    {
			    do{
			        userName = (char *)atParamStr(cmd->params[0], &paramok);
			        if (!paramok || strlen((char *)userName) > 32)
			        {
			            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
			            break;
		            }
			        userPasswd = (char *)atParamStr(cmd->params[1], &paramok);
			        if (!paramok || strlen((char *)userPasswd) > 32)
			        {
			            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
			            break;
		            }
			    }while(0);
		    }

		    if(cmd->param_count >= 3)
		    {
			    ip_address = (char *)atParamStr(cmd->params[2], &paramok);
		        if (!paramok || strlen((char *)ip_address) > 20)
		        {
		            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
		            break;
	            }

		    }

		    if(cmd->param_count >= 4)
		    {
			    ip_port = atParamInt(cmd->params[0], &paramok);
				if (!paramok)
				{
					RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
					break;
				}
		    }

		    if(userName != NULL)
			    strcpy(poc_config->account_name, userName);

		    if(userPasswd != NULL)
			    strcpy(poc_config->account_passwd, userPasswd);

		    if(ip_address != NULL)
			    strcpy(poc_config->ip_address, ip_address);

		    if(ip_port > -1)
			    poc_config->ip_port = ip_port;

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
