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

#ifdef CONFIG_AT_MY_APZZD_SUPPORT
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
#include "ppp_interface.h"
#include "guiOemCom_api.h"


void atCmdHandleLOGAPZZD(atCommand_t *cmd)
{
	char rspStr[200];
	bool paramok = true;
	char *apzzddata = NULL;

	if(cmd->type == AT_CMD_TEST)
	{
        atCmdRespInfoText(cmd->engine, "+POC=<opt>\n<param>");
        atCmdRespOK(cmd->engine);
        return;
	}

    switch (cmd->type)
    {
    case AT_CMD_EXE:
	    strcpy(rspStr, "POC:OK");

        atCmdRespInfoText(cmd->engine, rspStr);
        atCmdRespOK(cmd->engine);
        break;

    case AT_CMD_READ:
	    strcpy(rspStr, "POC:READ");

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
			        apzzddata = (char *)atParamStr(cmd->params[0], &paramok);
			        if (!paramok || strlen((char *)apzzddata) > 128)
			        {
			            RETURN_CME_ERR(cmd->engine, ERR_AT_CME_PARAM_INVALID);
			            break;
		            }

//					if(ap_Oem_engine == NULL)
//					{
//						ap_Oem_engine = cmd->engine;
//					}

					lvPocGuiOemCom_Msg(LVPOCGUIOEMCOM_SIGNAL_AP_POC_IND, (void *)apzzddata);
					OSI_LOGXI(OSI_LOGPAR_SI, 0, "[song]apzzddata is %s", apzzddata);
			    }while(0);
		    }

	    }while(0);
		/*ack*/
        atCmdRespOK(cmd->engine);
	    break;

    default:
        atCmdRespCmeError(cmd->engine, ERR_AT_CME_EXE_NOT_SURPORT);
        break;
    }
}


#endif
