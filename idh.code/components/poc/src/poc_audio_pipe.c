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

#include "poc_audio_pipe.h"
#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc.h"
#include "audio_device.h"

#ifdef CONFIG_POC_AUDIO_PLAYER_PIPE_SUPPORT

#define POC_PIPE_RECV_IND (6025)

struct osiPipe
{
    volatile bool running;
    volatile bool eof;
    unsigned size;
    unsigned rd;
    unsigned wr;
    osiSemaphore_t *rd_avail_sema;
    osiSemaphore_t *wr_avail_sema;
    unsigned rd_cb_mask;
    osiPipeEventCallback_t rd_cb;
    void *rd_cb_ctx;
    unsigned wr_cb_mask;
    osiPipeEventCallback_t wr_cb;
    void *wr_cb_ctx;
    char data[];
};

static PocPipeAttr PipeAttr = {0};

static void prvPocPlyPipeWriteCallback(void *param, unsigned event)
{
    PocPipeAttr *d = (PocPipeAttr *)param;
    if (event & OSI_PIPE_EVENT_TX_COMPLETE)
    {
        OSI_LOGXI(OSI_LOGPAR_I, 0, "[pocPipe]prvPocPlyPipeWriteCallback  event:%d", event);
        osiEvent_t poc_event = {.id = POC_PIPE_RECV_IND};
		osiEventSend(d->recv_thread_id, &poc_event);
    }
}

static void prvPocPipeRecvThreadEntry(void *param)
{
    PocPipeAttr *d = &PipeAttr;
    osiThread_t *thread = osiThreadCurrent();

    for (;;)
    {
        osiEvent_t event = {0};
        osiEventWait(thread, &event);
        if (event.id == OSI_EVENT_ID_QUIT)
            break;

        if (event.id == POC_PIPE_RECV_IND)
        {
            //send data to player pipe
            uint8_t *data = (uint8_t *)event.param1;
			if(osiPipeWriteAvail(d->plypipe) < 0
				|| data == NULL
				|| event.param2 == 0)
			{
				continue;
			}
			OSI_LOGI(0, "[pocPipe][thread]pipe writeall -> data(%d), size(%d)", data, event.param2);
            int bytes = osiPipeWriteAll(d->plypipe, data, event.param2, 1);

			if(bytes <= 0)
			{
				OSI_LOGI(0, "[pocPipe][thread]pipe write error");
				break;
			}
        }
    }
	d->recv_thread_id = NULL;
    osiThreadExit();
}

bool pocAudioPipeCreate(const uint32_t max_size)
{
    PipeAttr.recorder = auRecorderCreate();
	if(PipeAttr.recorder == NULL)
	{
		return false;
	}

	PipeAttr.recpipe = osiPipeCreate(12000);
	if(PipeAttr.recpipe == NULL)
	{
		auRecorderDelete(PipeAttr.recorder);
		return false;
	}

	PipeAttr.player = auPlayerCreate();
	if(PipeAttr.player == NULL)
	{
		osiPipeDelete(PipeAttr.recpipe);
		return false;
	}

	PipeAttr.plypipe = osiPipeCreate(max_size);
	if(PipeAttr.plypipe == NULL)
	{
		auPlayerDelete(PipeAttr.player);
		return false;
	}

	OSI_LOGI(0, "[pocPipe](%d):create", __LINE__);
	return true;
}

bool pocAudioPipeStart(void)
{
	if(PipeAttr.player == NULL
		|| PipeAttr.plypipe == NULL)
	{
		return false;
	}

	if(!pocAudioPipeStartPocModeReady())
	{
		OSI_LOGI(0, "[pocPipe][readywork]ready error");
		return false;
	}

	if(!audevPocModeSwitch(LV_POC_MODE_PLAYER))
	{
		OSI_LOGI(0, "[pocPipe][readywork]switch player failed");
		return false;
	}

	lv_poc_pcm_open_file();
	OSI_PRINTFI("[pocPipe](%s)(%d):pipe launching", __func__, __LINE__);

	if(PipeAttr.recv_thread_id == NULL)
	{
		osiPipeSetWriterCallback(PipeAttr.plypipe, OSI_PIPE_EVENT_TX_COMPLETE,
									 prvPocPlyPipeWriteCallback, &PipeAttr);

    	PipeAttr.recv_thread_id = osiThreadCreate("pocPiperecv", prvPocPipeRecvThreadEntry, NULL, OSI_PRIORITY_HIGH, (8192 * 4), 32);
	}

	return true;
}

bool pocAudioPipeReset(void)
{
	if(PipeAttr.plypipe == NULL
		|| PipeAttr.recpipe == NULL)
	{
		return false;
	}
	osiPipeReset(PipeAttr.plypipe);
	osiPipeReset(PipeAttr.recpipe);
	OSI_LOGI(0, "[pocPipe](%d):reset", __LINE__);
	return 0;
}

bool pocAudioPipeStop(void)
{
	if(PipeAttr.plypipe == NULL
		|| PipeAttr.plypipe == NULL)
	{
		return false;
	}

	lv_poc_pcm_close_file();

	if(!audevStopPocMode())
	{
		OSI_LOGI(0, "[pocPipe][Pipe]stop poc mode failed");
	}
	auRecorderStop(PipeAttr.recorder);
	auPlayerStop(PipeAttr.player);	
    osiPipeSetEof(PipeAttr.plypipe);	
    osiPipeSetEof(PipeAttr.recpipe);
	osiPipeStop(PipeAttr.plypipe);
	osiPipeStop(PipeAttr.recpipe);
	pocAudioPipeReset();
	OSI_LOGI(0, "[pocPipe](%d):stop", __LINE__);

	return false;
}

bool pocAudioPipeDelete(void)
{
	if(PipeAttr.plypipe == NULL
		|| PipeAttr.recpipe == NULL)
	{
		return false;
	}

	osiPipeDelete(PipeAttr.plypipe);
	osiPipeDelete(PipeAttr.recpipe);

	auRecorderDelete(PipeAttr.recorder);
	auPlayerDelete(PipeAttr.player);
	OSI_LOGI(0, "[pocPipe](%d):delete", __LINE__);
	return true;
}

void pocAudioPipeWriteData(const uint8_t *data, uint32_t length)
{
	PocPipeAttr *d = &PipeAttr;

	if(PipeAttr.plypipe == NULL
		|| PipeAttr.recpipe == NULL)
	{
		return;
	}

	OSI_PRINTFI("[pocPipe](%s)(%d):data(%d), length(%d)", __func__, __LINE__, data, length);

	osiEvent_t poc_event = {0};
	poc_event.id = POC_PIPE_RECV_IND;
	poc_event.param1 = (uint32_t)data;
	poc_event.param2 = length;
	osiEventSend(d->recv_thread_id, &poc_event);

	lv_poc_pcm_write_to_file(data, length);//pcm to file
}

bool pocAudioPipeGetStatus(void)
{
	if(PipeAttr.plypipe == NULL)
	{
		return false;
	}

	return PipeAttr.plystatus;
}

bool pocAudioPipeStopPocMode(void)
{
	if(audevStopPocMode())
	{
		return false;
	}
	auRecorderStop(PipeAttr.recorder);
	auPlayerStop(PipeAttr.player);
	OSI_LOGI(0, "[pocPipe](%d):stop poc mode", __LINE__);
	return true;
}

bool pocAudioPipeStartPocModeReady(void)
{
	if (!audevSetRecordSampleRate(8000))
	{
		OSI_LOGI(0, "[pocPipe][readywork](%d):audio poc set samplerate fail", __LINE__);
		return false;
	}
    osiPipeReset(PipeAttr.plypipe);

	auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
	auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};

	PipeAttr.plystatus = auPlayerStartPipeV2(PipeAttr.player, AUDEV_PLAY_TYPE_POC, AUSTREAM_FORMAT_PCM, params, PipeAttr.plypipe);
	if(!PipeAttr.plystatus)
	{
		OSI_LOGI(0, "[pocPipe][readywork](%d):recorder's reconfig error", __LINE__);
		return false;
	}
    auPipeReaderSetWait((auPipeReader_t *)auPlayerGetReader(PipeAttr.player), 0);

	PipeAttr.recstatus = auRecorderStartPipe(PipeAttr.recorder, AUDEV_RECORD_TYPE_POC, AUSTREAM_FORMAT_PCM, NULL, PipeAttr.recpipe);
	if(!PipeAttr.recstatus)
	{
		OSI_LOGI(0, "[pocPipe][readywork](%d):recorder's audio poc recorder startwriter failed", __LINE__);
        auPlayerStop(PipeAttr.player);
		return false;
	}

	if(!audevStartPocMode(AUPOC_STATUS_HALF_DUPLEX))
	{
		OSI_LOGI(0, "[pocPipe][readywork](%d):poc mode start failed", __LINE__);
        auRecorderStop(PipeAttr.recorder);
        auPlayerStop(PipeAttr.player);
		return false;
	}

	return true;
}

#endif
