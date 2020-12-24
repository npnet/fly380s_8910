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

#include "poc_audio_player.h"
#include "poc_audio_recorder.h"
#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc.h"
#include "audio_device.h"

#ifdef CONFIG_POC_AUDIO_PLAYER_SUPPORT

#define PLAYER_POC_MODE 1
#define POC_PLAYER_RECV_IND (6028)

static struct pocAudioThread_t AudioMsgattr = {0};
static POCAUDIOPLAYER_HANDLE PocMode_playerid;

static void prvPocAudioPlayerMemWriterDelete(auWriter_t *d)
{
    auPocMemWriter_t *p = (auPocMemWriter_t *)d;

    if (p == NULL)
    {
        return;
	}
	OSI_PRINTFI("[pocaudioplayer](%s)(%d):writer deleted", __func__, __LINE__);

    free(p->buf);
    free(p);
}

static int prvPocAudioPlayerMemWriterWrite(auWriter_t *d, const void *buf, unsigned size)
{
    auPocMemWriter_t *p = (auPocMemWriter_t *)d;
    pocAudioPlayer_t *player = (pocAudioPlayer_t *)p->user;

    if (size == 0)
    {
        return 0;
	}

    if (buf == NULL)
    {
        return -1;
	}
	// 0 + 320 < 81920
	if(p->pos + size <= p->max_size)//81920
	{	//地址 + pos偏移
		memcpy((char *)p->buf + p->pos, (char *)buf, size);
		p->pos = p->pos + size;//偏移指针
	}
	else
	{
		if(p->max_size > p->pos)
		{
			memcpy((char *)p->buf + p->pos, buf, p->max_size - p->pos);
		}
		memcpy((char *)p->buf, buf + ( p->max_size - p->pos ), size - ( p->max_size - p->pos ));
		p->pos = size - ( p->max_size - p->pos );
		player->restart = true;
	}
	p->size = p->pos;//记录大小
    return size;
}

static int prvPocAudioPlayerMemWriterSeek(auWriter_t *d, int offset, int whence)
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
	OSI_PRINTFI("[pocaudioplayer](%s)(%d):seek pos(%d), offset(%d), whence(%d)", __func__, __LINE__, p->pos, offset, whence);
	return p->pos;
}

static void prvPocAudioPlayerMemReaderDelete(auReader_t *d)
{
    auPocMemReader_t *p = (auPocMemReader_t *)d;
    if (p == NULL)
    {
        return;
	}

	OSI_PRINTFI("[pocaudioplayer](%s)(%d):reader delete", __func__, __LINE__);
    free(p);
}

static int prvPocAudioPlayerMemReaderRead(auReader_t *d, void *buf, unsigned size)
{
	auPocMemReader_t *p = (auPocMemReader_t *)d;
    pocAudioPlayer_t *player = (pocAudioPlayer_t *)p->user;

    if (size == 0)
    {
        return 0;
	}

    if (buf == NULL)
    {
        return -1;
	}
				//writerwrite + 2048(初始值) + 1024
	int count = player->writer->pos - (p->pos + size);//回调写入的数据writer->pos(写入播放器) - (2048(记录读入播放器指针) + 1024)
	if(player->restart == true)
	{
		count = p->size + (count<=0 ? count:-count);
	}

	if(player->raise == true)
	{
		if(count < POCAUDIOPLAYERDATAPREBUFFSIZE/2)
		{
			memset(buf, 0, size);
			return size;
		}
		player->raise = false;
	}

	if(count <= 0)//读指针 > 写指针
	{
		player->raise = true;
		memset(buf, 0, size);
		return size;
	}
	//2048(初始化起始读指针索引) + (读入播放器大小)1024 <= 81920
	if(p->pos + size <= p->size)//未溢出播放器
	{
		memcpy(buf, (const char *)p->buf + p->pos, size);
		memset((void *)p->buf + p->pos, 0, size);
		p->pos = p->pos + size;//? = 2048 + 1024
	}
	else if(player->restart == true)
	{
		if(p->size > p->pos)//81920 > 2048(读指针)
		{
			memcpy(buf, (const char *)p->buf + p->pos, p->size - p->pos);//把剩下没播的数据拷贝到首地址
			memset((void *)p->buf + p->pos, 0, p->size - p->pos);
		}
		memcpy((char *)buf + (p->size - p->pos), (const char *)p->buf, size - (p->size - p->pos));
		memset((void *)p->buf, 0, size - (p->size - p->pos));
		p->pos = size - ( p->size - p->pos );
		player->restart = false;
	}
	else//溢出播放器
	{
		memset(buf, 0, size);
	}
    return size;
}

static int prvPocAudioPlayerMemReaderSeek(auReader_t *d, int offset, int whence)
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

static bool prvPocAudioPlayerMemReaderEof(auReader_t *d)
{
    return false;
}

static auPocMemReader_t *prvPocAudioPlayerMemReaderCreate(const void *buf, unsigned size)
{
	OSI_PRINTFI("[pocaudioplayer](%s)(%d):reader create:max size(%d)", __func__, __LINE__, size);
    if (size > 0 && buf == NULL)
    {
        return NULL;
	}

    auPocMemReader_t *p = (auPocMemReader_t *)calloc(1, sizeof(auPocMemReader_t));
    if (p == NULL)
    {
        return NULL;
	}

    p->ops.destroy = prvPocAudioPlayerMemReaderDelete;
    p->ops.read = prvPocAudioPlayerMemReaderRead;
    p->ops.seek = prvPocAudioPlayerMemReaderSeek;
    p->ops.is_eof = prvPocAudioPlayerMemReaderEof;
    p->buf = buf;
    p->size = size;
    p->pos = 320;
    return p;
}

static auPocMemWriter_t * prvPocAudioPlayerMemWriterCreate(uint32_t max_size)
{
	OSI_PRINTFI("[pocaudioplayer](%s)(%d):writer create:max size(%d)", __func__, __LINE__, max_size);
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

    p->ops.destroy = prvPocAudioPlayerMemWriterDelete;
    p->ops.write = prvPocAudioPlayerMemWriterWrite;
    p->ops.seek = prvPocAudioPlayerMemWriterSeek;
    p->max_size = max_size;
    p->size = 0;
    p->pos = 0;
    return p;
}

int prvPocPlayRecvThreadWriteData(const uint8_t *data, uint32_t length)
{
	int ret = 0;
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)pocAudioPlayerId();
	int count = player->writer->pos - player->reader->pos;

	if(player->restart == true)
	{
		count = player->writer->max_size + (count<=0 ? count:-count);
	}

	if(player->reduce)
	{
		player->reduce_index = !(player->reduce_index);
		if(player->reduce_index)
		{
			ret = auWriterWrite((auWriter_t *)player->writer, data, length);
		}
		if(count <= POCAUDIOPLAYERDATAPREBUFFSIZE/2)
		{
			player->reduce = false;
		}
		return ret;
	}

	if(count >= POCAUDIOPLAYERDATAPREBUFFSIZE)//地址相减 -2048 > 20*320U
	{
		player->reduce = true;
	}

	return auWriterWrite((auWriter_t *)player->writer, data, length);
}

static void prvPocPlayRecvThreadEntry(void *param)
{
    for (;;)
    {
		struct pocAudioMsg_t queue = {0};

        if(osiMessageQueueGet(AudioMsgattr.queue, &queue))
        {
			//send data to player
			if(queue.data == NULL
				|| queue.len == 0)
			{
				continue;
			}
			prvPocPlayRecvThreadWriteData((const uint8_t *)queue.data, queue.len);
		}
    }
}

void pocAudioPlayerInit(void)
{

}

POCAUDIOPLAYER_HANDLE pocAudioPlayerCreate(const uint32_t max_size)
{
	OSI_PRINTFI("[pocaudioplayer](%s)(%d):max size(%d)", __func__, __LINE__, max_size);
	if(max_size == 0)
	{
		return 0;
	}

	pocAudioPlayer_t * player = (pocAudioPlayer_t *)malloc(sizeof(pocAudioPlayer_t));
	if(NULL == player)
	{
		return 0;
	}
	player->status = false;


	player->player = auPlayerCreate();
	if(player->player == NULL)
	{
		free(player);
		return 0;
	}

	player->writer = prvPocAudioPlayerMemWriterCreate(max_size);
	if(player->writer == NULL)
	{
		auPlayerDelete((auPlayer_t *)player->player);
		free(player);
		return 0;
	}

	player->reader = prvPocAudioPlayerMemReaderCreate(player->writer->buf, player->writer->max_size);
	if(player->reader == NULL)
	{
		auWriterDelete((auWriter_t *)player->writer);
		auPlayerDelete((auPlayer_t *)player->player);
		free(player);
		return 0;
	}

	player->writer->user = (void *)player;
	player->reader->user = (void *)player;
	player->restart = false;
	player->reduce  = false;
	player->raise   = false;

    AudioMsgattr.queue = osiMessageQueueCreate(64, sizeof(struct pocAudioMsg_t));
	if(AudioMsgattr.queue == NULL)
	{
		return 0;
	}

    AudioMsgattr.thread = osiThreadCreate("pocPlayerrecv", prvPocPlayRecvThreadEntry, NULL, OSI_PRIORITY_HIGH, (8192 * 4), 32);
	if(AudioMsgattr.thread == NULL)
	{
		return 0;
	}
	pocAudioPlayerThreadSuspend();

#if PLAYER_POC_MODE
	PocMode_playerid = (POCAUDIOPLAYER_HANDLE)player;
#endif

	return (POCAUDIOPLAYER_HANDLE)player;
}

void pocAudioPlayerThreadSuspend(void)
{
	if(AudioMsgattr.thread != NULL)
	{
		osiThreadSuspend(AudioMsgattr.thread);
	}
}

void pocAudioPlayerThreadResume(void)
{
	if(AudioMsgattr.thread != NULL)
	{
		osiThreadResume(AudioMsgattr.thread);
	}
}

bool pocAudioPlayerStart(POCAUDIOPLAYER_HANDLE player_id)
{
	OSI_PRINTFI("[pocaudioplayer](%s)(%d):play start", __func__, __LINE__);
	if(player_id == 0)
	{
		return false;
	}

#if PLAYER_POC_MODE

	if(!pocAudioPlayerStartPocModeReady())
	{
		OSI_LOGI(0, "[pocaudioplayer][readywork][player]ready error");
		return false;
	}

	if(!audevPocModeSwitch(LV_POC_MODE_PLAYER))
	{
		OSI_LOGI(0, "[pocaudioplayer][readywork][player]switch player failed");
		return false;
	}

	//lv_poc_pcm_open_file();

	return true;
#else
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;

    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
   	auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};

	lv_poc_setting_set_current_volume(POC_MMI_VOICE_PLAY, lv_poc_setting_get_current_volume(POC_MMI_VOICE_PLAY), true);
	player->status = auPlayerStartReader(player->player, AUSTREAM_FORMAT_PCM, params, (auReader_t *)player->reader);
	lv_poc_pcm_open_file();
	return player->status;

#endif
}

int pocAudioPlayerReset(POCAUDIOPLAYER_HANDLE player_id)
{
	OSI_PRINTFI("[pocaudioplayer](%s)(%d):play reset", __func__, __LINE__);
	if(player_id == 0)
	{
		return -1;
	}
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;

	player->writer->pos = 0;

	player->reader->pos = 320;//2048

	memset(player->writer->buf, 0, player->writer->max_size);

	player->restart = false;

	player->reduce = false;

	player->reduce_index = false;

	player->raise = false;

	return 0;
}

bool pocAudioPlayerStop(POCAUDIOPLAYER_HANDLE player_id)
{
	OSI_PRINTFI("[pocaudioplayer](%s)(%d):play stop", __func__, __LINE__);
	if(player_id == 0)
	{
		return false;
	}

	//lv_poc_pcm_close_file();
#if PLAYER_POC_MODE

	if(!audevStopPocMode())
	{
		OSI_LOGI(0, "[pocaudioplayer][player]stop poc mode failed");
	}
	pocAudioRecorderStopPocMode();

#endif

	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;
	if(auPlayerStop((auPlayer_t *)player->player))
	{
		player->status = false;
		pocAudioPlayerReset(player_id);
		pocAudioPlayerThreadSuspend();
		return true;
	}

	return false;
}

bool pocAudioPlayerDelete(POCAUDIOPLAYER_HANDLE player_id)
{
	if(player_id == 0)
	{
		return false;
	}
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;

	if(!auPlayerStop((auPlayer_t *)player->player))
	{
		return false;
	}

	auPlayerDelete((auPlayer_t *)player->player);
	auReaderDelete((auReader_t *)player->reader);
	auWriterDelete((auWriter_t *)player->writer);
	free(player);

	return true;
}

bool pocAudioPlayerGetStatus(POCAUDIOPLAYER_HANDLE player_id)
{
	if(player_id == 0)
	{
		return false;
	}
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;

	return player->status;
}

POCAUDIOPLAYER_HANDLE pocAudioPlayerId(void)
{
	return PocMode_playerid;
}

bool pocAudioPlayerStopPocMode(void)
{
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)pocAudioPlayerId();
	if(auPlayerStop((auPlayer_t *)player->player))
	{
		player->status = false;
		pocAudioPlayerReset(pocAudioPlayerId());
		return true;
	}

	return false;
}

bool pocAudioPlayerStartPocModeReady(void)
{
	pocAudioRecorder_t * recorder = (pocAudioRecorder_t *)pocAudioRecordId();
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)pocAudioPlayerId();

	auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
	auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};

	if (audevGetRecordSampleRate() != 8000)
	{
		if(!audevSetRecordSampleRate(8000))
			return false;
	}

	player->status = auPlayerStartReaderV2(player->player, AUDEV_PLAY_TYPE_POC, AUSTREAM_FORMAT_PCM, params, (auReader_t *)player->reader);
	if(!player->status)
	{
		return false;
	}

	recorder->status = auRecorderStartWriter(recorder->recorder, AUDEV_RECORD_TYPE_POC, AUSTREAM_FORMAT_PCM, NULL, (auWriter_t *)recorder->writer);
	if(!recorder->status)
	{
		return false;
	}

	if(!audevStartPocMode(AUPOC_STATUS_HALF_DUPLEX))
	{
		auRecorderStop(recorder->recorder);
		auPlayerStop(player->player);
		return false;
	}

	return true;
}

void pocAudioSendMsgWriteData(const uint8_t *data, uint32_t length)
{
	if(AudioMsgattr.thread == NULL
		|| AudioMsgattr.queue == NULL)
	{
		return;
	}

	struct pocAudioMsg_t msg_t;
	msg_t.data = data;
	msg_t.len = length;
	osiMessageQueuePut(AudioMsgattr.queue, &msg_t);
}

int pocAudioPlayerWriteData(POCAUDIOPLAYER_HANDLE player_id, const uint8_t *data, uint32_t length)
{
	//lv_poc_pcm_write_to_file(data, length);//pcm to file

	int ret = 0;
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)pocAudioPlayerId();
	int count = player->writer->pos - player->reader->pos;

	if(player->restart == true)
	{
		count = player->writer->max_size + (count<=0 ? count:-count);
	}

	if(player->reduce)//give up player->reader->pos(320)
	{
		player->reduce_index = !(player->reduce_index);
		if(player->reduce_index)
		{
			ret = auWriterWrite((auWriter_t *)player->writer, data, length);
		}
		if(count <= POCAUDIOPLAYERDATAPREBUFFSIZE/2)
		{
			player->reduce = false;
		}
		return ret;
	}

	if(count >= POCAUDIOPLAYERDATAPREBUFFSIZE)//地址相减 -2048 > 20*320U
	{
		player->reduce = true;
	}

	return auWriterWrite((auWriter_t *)player->writer, data, length);
}

#endif
