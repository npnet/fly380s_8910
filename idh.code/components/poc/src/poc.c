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

#include "poc.h"
#ifdef CONFIG_POC_SUPPORT

#include "osi_log.h"
#include "osi_api.h"
#include "osi_pipe.h"
#include "ml.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static POCAUDIOPLAYER_HANDLE player_id     = 0;
static POCAUDIORECORDER_HANDLE recorder_id = 0;

void prvPocAudioDeviceCallback(uint8_t * data, uint32_t length)
{
	if(player_id == 0)
	{
		return;
	}
	pocAudioPlayerWriteData(player_id, data, length);
}

void pocStart(void *ctx)
{
	int cout = 300;
	while(0)
	{
		player_id     = pocAudioPlayerCreate(16000 * 20);
		recorder_id   = pocAudioRecorderCreate(16000 * 20, 320, 20, prvPocAudioDeviceCallback);
		
		if(recorder_id != 0)
		{
			pocAudioRecorderStart(recorder_id);
			osiThreadSleep(10000);
			pocAudioRecorderStop(recorder_id);
		}

		osiThreadSleep(500);
		
		if(player_id != 0)
		{
			pocAudioPlayerStart(player_id);
			osiThreadSleep(10000);
			pocAudioPlayerStop(player_id);
		}

		pocAudioPlayerDelete(player_id);
		pocAudioRecorderDelete(recorder_id);

		player_id = 0;
		recorder_id = 0;
		cout--;
	}
	osiThreadExit();
}


#endif
