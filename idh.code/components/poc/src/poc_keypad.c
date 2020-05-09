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

// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "poc_keypad.h"
#ifdef CONFIG_POC_KEYPAD_SUPPORT

#include "osi_log.h"
#include "osi_api.h"
#include "osi_pipe.h"
#include "ml.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "guiIdtCom_api.h"

static keyState_t preKeyState = 0xff;
static keyMap_t   preKey      = 0xff;
bool pocKeypadHandle(keyMap_t id, keyState_t evt, void *p)
{
	bool ret = true;
	if(id == KEY_MAP_4) //poc
	{
		if(preKeyState != evt)
		{
			if(evt == KEY_STATE_PRESS)
			{
				OSI_LOGI(0, "[gic] send LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_IND\n");
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_START_IND, NULL);
			}
			else
			{
				OSI_LOGI(0, "[gic] send LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_IND\n");
				lvPocGuiIdtCom_Msg(LVPOCGUIIDTCOM_SIGNAL_SPEAK_STOP_IND, NULL);
			}
		}
		ret = true;
	}
	preKey = id;
	preKeyState = evt;
	return ret;
}


#endif
