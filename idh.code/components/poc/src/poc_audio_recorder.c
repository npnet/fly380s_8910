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

#include "poc_audio_recorder.h"
#ifdef CONFIG_POC_AUDIO_RECORDER_SUPPORT

static void prvPocAudioRecorderMemWriterDelete(auWriter_t *d)
{
    auPocMemWriter_t *p = (auPocMemWriter_t *)d;

    if (p == NULL)
    {
        return;
	}

    OSI_LOGI(0, "audio memory writer deleted");
    free(p->buf);
    free(p);
}

static int prvPocAudioRecorderMemWriterWrite(auWriter_t *d, const void *buf, unsigned size)
{
    auPocMemWriter_t *p = (auPocMemWriter_t *)d;
    pocAudioRecorder_t *r = (pocAudioRecorder_t *)p->user;

    if (size < 1)
    {
        return 0;
	}

    if (buf == NULL)
    {
        return -1;
	}

	if(p->pos + size <= p->max_size - 1)
	{
		memcpy((char *)p->buf + p->pos, (char *)buf, size);
		p->pos = p->pos + size;
	}
	else
	{
		if(p->max_size - 1 > p->pos)
		{
			memcpy((char *)p->buf + p->pos, buf, p->max_size - 1 - p->pos);
		}
		memcpy((char *)p->buf, buf + ( p->max_size - 1 - p->pos ), size - ( p->max_size - 1 - p->pos ));
		p->pos = size - ( p->max_size - 1 - p->pos );
		r->prvRestart = true;
	}
	p->size = p->pos + 1;

    return size;
}

static int prvPocAudioRecorderMemWriterSeek(auWriter_t *d, int offset, int whence)
{
    auPocMemWriter_t *p = (auPocMemWriter_t *)d;

    switch (whence)
    {
    case SEEK_SET:
        p->pos = OSI_MAX(int, 0, OSI_MIN(int, p->size, offset));
        break;
    case SEEK_CUR:
        p->pos = OSI_MAX(int, 0, OSI_MIN(int, p->size, p->pos + offset));
        break;
    case SEEK_END:
        p->pos = OSI_MAX(int, 0, OSI_MIN(int, p->size, p->size + offset));
        break;
    default:
        return -1;
    }

    return p->pos;
}

static void prvPocAudioRecorderMemReaderDelete(auReader_t *d)
{
    auPocMemReader_t *p = (auPocMemReader_t *)d;
    if (p == NULL)
    {
        return;
	}

    OSI_LOGI(0, " poc audio meory reader delete");
    free(p);
}

static int prvPocAudioRecorderMemReaderRead(auReader_t *d, void *buf, unsigned size)
{
    auPocMemReader_t *p = (auPocMemReader_t *)d;
    pocAudioRecorder_t *r = (pocAudioRecorder_t *)p->user;

    if (size == 0)
    {
        return 0;
	}

    if (buf == NULL)
    {
        return -1;
	}

	if(p->pos + size <= p->size - 1)
	{
		memcpy(buf, (const char *)p->buf + p->pos, size);
		memset((void *)p->buf + p->pos, 0, size);
		p->pos = p->pos + size;
	}
	else if(r->prvRestart)
	{
		if(p->size - 1 > p->pos)
		{
			memcpy(buf, (const char *)p->buf + p->pos, p->size - 1 - p->pos);
			memset((void *)p->buf + p->pos, 0, p->size - 1 - p->pos);
		}
		memcpy((char *)buf + (p->size - 1 - p->pos), (const char *)p->buf, size - (p->size - 1 - p->pos));
		memset((void *)p->buf, 0, size - (p->size - 1 - p->pos));
		p->pos = size - ( p->size - 1 - p->pos );
		r->prvRestart = false;
	}
	else
	{
		memset((char *)buf, 0, size);
 	}

    return size;
}

static int prvPocAudioRecorderMemReaderSeek(auReader_t *d, int offset, int whence)
{
    auPocMemReader_t *p = (auPocMemReader_t *)d;

    switch (whence)
    {
    case SEEK_SET:
        p->pos = OSI_MAX(int, 0, OSI_MIN(int, p->size, offset));
        break;
    case SEEK_CUR:
        p->pos = OSI_MAX(int, 0, OSI_MIN(int, p->size, p->pos + offset));
        break;
    case SEEK_END:
        p->pos = OSI_MAX(int, 0, OSI_MIN(int, p->size, p->size + offset));
        break;
    default:
        return -1;
    }

    return p->pos;
}

static bool prvPocAudioRecorderMemReaderEof(auReader_t *d)
{
    return false;
}

static auPocMemReader_t *prvPocAudioRecorderMemReaderCreate(const void *buf, unsigned size)
{
    OSI_LOGI(0, "audio memory reader create, buf/0x%x size/%d", buf, size);

    if (size > 0 && buf == NULL)
    {
        return NULL;
	}

    auPocMemReader_t *p = (auPocMemReader_t *)calloc(1, sizeof(auPocMemReader_t));
    if (p == NULL)
    {
        return NULL;
	}

    p->ops.destroy = prvPocAudioRecorderMemReaderDelete;
    p->ops.read = prvPocAudioRecorderMemReaderRead;
    p->ops.seek = prvPocAudioRecorderMemReaderSeek;
    p->ops.is_eof = prvPocAudioRecorderMemReaderEof;
    p->buf = buf;
    p->size = size;
    p->pos = 0;
    return p;
}

static auPocMemWriter_t * prvPocAudioRecorderMemWriterCreate(uint32_t max_size)
{
    OSI_LOGI(0, "poc audio recorder memory writer create, max size/%d", max_size);

    if (max_size == 0)
    {
        return NULL;
	}

    auPocMemWriter_t *p = (auPocMemWriter_t *)calloc(1, sizeof(auPocMemWriter_t));
    if (p == NULL)
    {
        return NULL;
	}

	p->buf = (char *)calloc(1, sizeof(char) * max_size);
	if(p->buf == NULL)
	{
		free(p);
		return NULL;
	}

    p->ops.destroy = prvPocAudioRecorderMemWriterDelete;
    p->ops.write = prvPocAudioRecorderMemWriterWrite;
    p->ops.seek = prvPocAudioRecorderMemWriterSeek;
    p->max_size = max_size;
    p->size = 0;
    p->pos = 0;
    return p;
}

void prvPocAudioRecorderThreadCallback(void *ctx)
{
	if(ctx == NULL)
	{
		osiThreadExit();
		return;
	}

	pocAudioRecorder_t * recorder = (pocAudioRecorder_t *)ctx;
	osiEvent_t event = {0};

	while(recorder->callback == NULL || recorder->prvSwapData == NULL || recorder->prvSwapDataLength < 1 || recorder->writer == NULL || recorder->reader == NULL)
	{
		osiThreadSleep(30);
	}
	memset(&event, 0, sizeof(osiEvent_t));
	recorder->prvThreadID = osiThreadCurrent();

	while(1)
	{
		while(!osiEventTryWait(recorder->prvThreadID, &event, recorder->prvDuration))
		{
			if(!recorder->status)
			{
				continue;
			}

			if(!recorder->prvRestart)
			{
				if(recorder->writer->pos - recorder->reader->pos < recorder->prvSwapDataLength)
				{
					continue;
				}
			}
			else
			{
				if(recorder->reader->pos > recorder->writer->pos)
				{
					if(recorder->reader->size - recorder->reader->pos + recorder->writer->pos < recorder->prvSwapDataLength)
					{
						continue;
					}
				}
				else
				{
					if(recorder->writer->pos - recorder->reader->pos < recorder->prvSwapDataLength)
					{
						continue;
					}
				}
			}

			if(-1 == auReaderRead((auReader_t *)recorder->reader, recorder->prvSwapData, recorder->prvSwapDataLength))
			{
				continue;
			}

			recorder->callback(recorder->prvSwapData, recorder->prvSwapDataLength);
		}

		if(event.id == OSI_EVENT_ID_QUIT)
		{
			break;
		}
	}

	auRecorderDelete((auRecorder_t *)recorder->recorder);
	auReaderDelete((auReader_t *)recorder->reader);
	auWriterDelete((auWriter_t *)recorder->writer);
	free(recorder->prvSwapData);
	free(recorder);
	recorder->prvThreadID = NULL;
	osiThreadExit();
}

/**
 * \brief initialize poc audio recorder
 *
 * poc Audio recorder is designed as singleton.
 */
void pocAudioRecorderInit(void)
{

}

/**
 * \brief create poc audio recorder
 *
 * param max_size     length of storage data
 *       data_length  length of swap data which should be send others by callback
 *       duration     duration of timer execute callback
 *       callback     the timer execute the callback
 *
 * return ID of POC audio recorder, zero is failed
 */
POCAUDIORECORDER_HANDLE pocAudioRecorderCreate(const uint32_t max_size,
													const uint32_t data_length,
													const uint32_t duration,
													pocAudioRecorderCallback_t callback)
{
	if(max_size == 0)
	{
		return 0;
	}

	pocAudioRecorder_t * recorder = (pocAudioRecorder_t *)malloc(sizeof(pocAudioRecorder_t));
	if(NULL == recorder)
	{
		return 0;
	}
	recorder->status = false;
	recorder->prvRestart = false;
	recorder->prvDuration = duration;
	recorder->callback = callback;

	recorder->recorder = auRecorderCreate();
	if(recorder->recorder == NULL)
	{
		free(recorder);
		return 0;
	}

	recorder->writer = prvPocAudioRecorderMemWriterCreate(max_size);
	if(recorder->writer == NULL)
	{
		auRecorderDelete((auRecorder_t *)recorder->recorder);
		free(recorder);
		return 0;
	}
	recorder->writer->user = (void *)recorder;

	recorder->reader = prvPocAudioRecorderMemReaderCreate(recorder->writer->buf, recorder->writer->max_size);
	if(recorder->reader == NULL)
	{
		auWriterDelete((auWriter_t *)recorder->writer);
		auRecorderDelete((auRecorder_t *)recorder->recorder);
		free(recorder);
		return 0;
	}
	recorder->reader->user = (void *)recorder;

	recorder->prvThreadID = osiThreadCreate("redr_cb_thd", prvPocAudioRecorderThreadCallback, (void *)recorder, OSI_PRIORITY_NORMAL, 2480, 64);
	if(recorder->prvThreadID == NULL)
	{
		auReaderDelete((auReader_t *)recorder->reader);
		auWriterDelete((auWriter_t *)recorder->writer);
		auRecorderDelete((auRecorder_t *)recorder->recorder);
		free(recorder);
		return 0;
	}

	recorder->prvSwapDataLength = data_length;
	recorder->prvSwapData = (uint8_t *)malloc(sizeof(uint8_t) * (recorder->prvSwapDataLength + 10));
	if(recorder->prvSwapData == NULL)
	{
		osiSendQuitEvent(recorder->prvThreadID, false);
		auReaderDelete((auReader_t *)recorder->reader);
		auWriterDelete((auWriter_t *)recorder->writer);
		auRecorderDelete((auRecorder_t *)recorder->recorder);
		free(recorder);
		return 0;
	}

	return (POCAUDIORECORDER_HANDLE)recorder;
}

/**
 * \brief start poc audio recorder
 *
 * param recorder_id   ID of POC audio recorder
 *
 * return false is failed, true is success to record
 */
bool pocAudioRecorderStart(POCAUDIORECORDER_HANDLE recorder_id)
{
	if(recorder_id == 0)
	{
		return false;
	}
	pocAudioRecorder_t * recorder = (pocAudioRecorder_t *)recorder_id;
	recorder->prvRestart = false;
	recorder->writer->pos = 0;
	recorder->writer->size = 0;
	recorder->reader->pos = 0;

	if(recorder->status)
	{
		return true;
	}
	recorder->status = true;

	bool ret = auRecorderStartWriter(recorder->recorder, AUDEV_RECORD_TYPE_MIC, AUSTREAM_FORMAT_PCM, NULL, (auWriter_t *)recorder->writer);
	if(ret == false)
	{
		recorder->status = false;
	}
	return ret;
}

/**
 * \brief reset poc audio recorder
 *
 * param recorder_id   ID of POC audio recorder
 *
 * return none
 */
int pocAudioRecorderReset(POCAUDIORECORDER_HANDLE recorder_id)
{
	if(recorder_id == 0)
	{
		return -1;
	}
	pocAudioRecorder_t * recorder = (pocAudioRecorder_t *)recorder_id;

	recorder->writer->pos  = 0;
	recorder->writer->size = 0;
	recorder->reader->pos  = 0;
	recorder->prvRestart = false;

	memset(recorder->writer->buf, 0, recorder->prvSwapDataLength * 2);
	memset(recorder->prvSwapData, 0, recorder->prvSwapDataLength);
	return 0;
}

/**
 * \brief stop poc audio recorder
 *
 * param recorder_id   ID of POC audio recorder
 *
 * return false is fail to stop record, true is success
 */
bool pocAudioRecorderStop(POCAUDIORECORDER_HANDLE recorder_id)
{
	if(recorder_id == 0)
	{
		return false;
	}
	bool ret = false;
	pocAudioRecorder_t * recorder = (pocAudioRecorder_t *)recorder_id;

	if(!recorder->status)
	{
		return true;
	}

	recorder->status = false;

	if(auRecorderStop((auRecorder_t *)recorder->recorder))
	{
		ret = true;
	}
	pocAudioRecorderReset(recorder_id);

	return ret;
}

/**
 * \brief delete poc audio recorder
 *
 * param recorder_id   ID of POC audio recorder
 *
 * return return false is fail to delete record, true is success
 */
bool pocAudioRecorderDelete(POCAUDIORECORDER_HANDLE       recorder_id)
{
	if(recorder_id == 0)
	{
		return false;
	}
	pocAudioRecorder_t * recorder = (pocAudioRecorder_t *)recorder_id;

	if(!auRecorderStop((auRecorder_t *)recorder->recorder))
	{
		return false;
	}
	osiSendQuitEvent(recorder->prvThreadID, false);

	return true;
}

/**
 * \brief get status of poc audio recorder
 *
 * param player_id  ID of POC audio player
 *
 * return true is recording
 */
bool pocAudioRecorderGetStatus(POCAUDIORECORDER_HANDLE recorder_id)
{
	if(recorder_id == 0)
	{
		return false;
	}
	pocAudioRecorder_t * recorder = (pocAudioRecorder_t *)recorder_id;

	return recorder->status;
}

#endif

