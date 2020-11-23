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
#include "guiBndCom_api.h"


void atCmdHandleLOGACCOUNT(atCommand_t *cmd)
{
    char rspStr[200];
    bool paramok = true;
    char *pocparam = NULL;
    char accoutparam[128];
	char pskpwd[32];
	bool pfdnormal = false;
	bool is_invalid_account = false;

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
						atCmdRespInfoText(cmd->engine, "error");
                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                        break;
                    }

                    pocparam = (char *)atParamStr(cmd->params[0], &paramok);

					if(!paramok || strlen((char *)pocparam) > 128)
                    {
						atCmdRespInfoText(cmd->engine, "error");
                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
                        break;
                    }
					//read nv
					nv_poc_setting_msg_t *poc_config = lv_poc_setting_conf_read();
					if(NULL != strstr((const char *)pocparam, (const char *)"info"))
					{
						char pocinfo[256];
						strcpy(pocinfo, "your'info:");
						strcat(pocinfo, poc_config->poc_info);
						atCmdRespInfoText(cmd->engine, pocinfo);
						break;
					}
					else if(NULL != strstr((const char *)pocparam, (const char *)"errorfk"))
					{
						char pocinfo[256];
						strcpy(pocinfo, "fk:");
						strcat(pocinfo, poc_config->poc_flash_key);
						strcat(pocinfo, ",sk:");
						strcat(pocinfo, poc_config->poc_secret_key);
						atCmdRespInfoText(cmd->engine, pocinfo);
						break;
					}
                    else
                    {
						char *psk = strstr((const char *)pocparam, (const char *)"sk=");
						char *pfdori = strstr((const char *)pocparam, (const char *)"fd=");
						char *pfd = pfdori;
						if(pfd != NULL)
						{
							pfd+=3;
							char pfd_chr = 0;
							strncpy(&pfd_chr, pfd, 1);
							if(pfd_chr == ';')
		                    {
								pfdnormal = false;
								OSI_PRINTFI("[poc][account](%s)(%d):null fd(%c)", __func__, __LINE__, pfd_chr);
		                    }
							else
							{
								if(0 == strcmp(poc_config->poc_flash_key, "123456"))//默认刷机秘钥
								{
									strncpy(poc_config->poc_flash_key, pfd, (strlen(pfd) - 1));//flash psd
									poc_config->poc_flash_key[strlen(pfd) - 1] = '\0';
									lv_poc_setting_conf_write();
									atCmdRespInfoText(cmd->engine, "set flash key ok");
								}
								else
								{
									OSI_PRINTFI("[poc][account](%s)(%d):exist fd", __func__, __LINE__);
									atCmdRespInfoText(cmd->engine, "exist flash key");
								}
								pfdnormal = true;
							}
						}

						if(psk != NULL)
						{
							psk+=3;
							if(psk == NULL)
		                    {
								atCmdRespInfoText(cmd->engine, "account key error");
		                        RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
		                        break;
		                    }

							char psk_chr = 0;
							strncpy(&psk_chr, psk, 1);
							if(psk_chr == ';')
							{
								is_invalid_account = true;
								OSI_PRINTFI("[poc][account](%s)(%d):null sk(%c)", __func__, __LINE__, psk_chr);
							}

							if(0 == strcmp(poc_config->poc_secret_key, "000000"))
							{
								if(!is_invalid_account)
								{
									if(pfdnormal)
									{
										strncpy(poc_config->poc_secret_key, psk, (strlen(psk) - strlen(pfd) - 4));
										poc_config->poc_secret_key[strlen(psk) - strlen(pfd) - 4] = '\0';
									}
									else
									{
										strncpy(poc_config->poc_secret_key, psk, (strlen(psk) - 5));
										poc_config->poc_secret_key[strlen(psk) - 5] = '\0';

									}
									lv_poc_setting_conf_write();
									OSI_PRINTFI("[poc][account](%s)(%d):valid sk", __func__, __LINE__);
									atCmdRespInfoText(cmd->engine, "set sk success");
								}
								else
								{
									OSI_PRINTFI("[poc][account](%s)(%d):invalid sk", __func__, __LINE__);
								}
								strncpy(accoutparam, pocparam, (strlen(pocparam) - strlen(psk) - 3));
								accoutparam[strlen(pocparam) - strlen(psk) - 3] = '\0';
								OSI_PRINTFI("[poc][account](%s)(%d):null, param(%s)", __func__, __LINE__, accoutparam);
								lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SETPOC_IND, (void *)accoutparam);
							}
							else
							{
								strncpy(pskpwd, psk, (strlen(psk) - strlen(pfdori) - 1));
								pskpwd[strlen(psk) - strlen(pfdori) - 1] = '\0';
								if(0 == strcmp(poc_config->poc_secret_key, pskpwd))
								{
									strncpy(accoutparam, pocparam, (strlen(pocparam) - strlen(psk) - 3));
									accoutparam[strlen(pocparam) - strlen(psk) - 3] = '\0';
									OSI_PRINTFI("[poc][account](%s)(%d):sk correct, param(%s)", __func__, __LINE__, accoutparam);
									atCmdRespInfoText(cmd->engine, "sk correct");
									lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_SETPOC_IND, (void *)accoutparam);
								}
								else
								{
									OSI_PRINTFI("[poc][account](%s)(%d):sk error", __func__, __LINE__);
									atCmdRespInfoText(cmd->engine, "sk error");
									break;
								}
							}
						}
						else
						{
							OSI_PRINTFI("[poc][account](%s)(%d):error", __func__, __LINE__);
							atCmdRespInfoText(cmd->engine, "param error");
							break;
						}
					}
                }while(0);
                break;
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
