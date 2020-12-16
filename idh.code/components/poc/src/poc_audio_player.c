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

#include "poc_audio_player.h"
#include "poc_audio_recorder.h"
#include "lv_include/lv_poc_type.h"
#include "lv_include/lv_poc.h"
#include "audio_device.h"

#ifdef CONFIG_POC_AUDIO_PLAYER_SUPPORT

#define PLAYER_POC_MODE 1

static POCAUDIOPLAYER_HANDLE PocMode_playerid;

static void prvPocAudioPlayerMemWriterDelete(auWriter_t *d)
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

	if(p->pos + size <= p->max_size)
	{
		memcpy((char *)p->buf + p->pos, (char *)buf, size);
		p->pos = p->pos + size;
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
	p->size = p->pos;

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

    return p->pos;
}

static void prvPocAudioPlayerMemReaderDelete(auReader_t *d)
{
    auPocMemReader_t *p = (auPocMemReader_t *)d;
    if (p == NULL)
    {
        return;
	}

    OSI_LOGI(0, " poc audio meory reader delete");
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

	int count = player->writer->pos - (p->pos + size);
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

	if(count <= 0)
	{
		player->raise = true;
		memset(buf, 0, size);
		return size;
	}

	if(p->pos + size <= p->size)
	{
		memcpy(buf, (const char *)p->buf + p->pos, size);
		memset((void *)p->buf + p->pos, 0, size);
		p->pos = p->pos + size;
	}
	else if(player->restart == true)
	{
		if(p->size > p->pos)
		{
			memcpy(buf, (const char *)p->buf + p->pos, p->size - p->pos);
			memset((void *)p->buf + p->pos, 0, p->size - p->pos);
		}
		memcpy((char *)buf + (p->size - p->pos), (const char *)p->buf, size - (p->size - p->pos));
		memset((void *)p->buf, 0, size - (p->size - p->pos));
		p->pos = size - ( p->size - p->pos );
		player->restart = false;
	}
	else
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

    p->ops.destroy = prvPocAudioPlayerMemReaderDelete;
    p->ops.read = prvPocAudioPlayerMemReaderRead;
    p->ops.seek = prvPocAudioPlayerMemReaderSeek;
    p->ops.is_eof = prvPocAudioPlayerMemReaderEof;
    p->buf = buf;
    p->size = size;
    p->pos = 2048;
    return p;
}

static auPocMemWriter_t * prvPocAudioPlayerMemWriterCreate(uint32_t max_size)
{
    OSI_LOGI(0, "poc audio player memory writer create, max size/%d", max_size);

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

/**
 * \brief initialize poc audio player
 *
 * poc Audio player is designed as singleton.
 */
void pocAudioPlayerInit(void)
{

}

/**
 * \brief create poc audio player
 *
 * param max_size     length of storage data
 *
 * return ID of POC audio player, zero is failed
 */
POCAUDIOPLAYER_HANDLE pocAudioPlayerCreate(const uint32_t max_size)
{
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

#if PLAYER_POC_MODE
	PocMode_playerid = (POCAUDIOPLAYER_HANDLE)player;
#endif

	return (POCAUDIOPLAYER_HANDLE)player;
}

/**
 * \brief start poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return false is failed to start player, true is success
 */
bool pocAudioPlayerStart(POCAUDIOPLAYER_HANDLE player_id)
{
	if(player_id == 0)
	{
		return false;
	}

#if PLAYER_POC_MODE

	if(!pocAudioPlayerStartPocModeReady())
	{
		OSI_LOGI(0, "[poc][readywork][player]ready error");
		return false;
	}

	if(!audevPocModeSwitch(LV_POC_MODE_PLAYER))
	{
		OSI_LOGI(0, "[poc][readywork][player]switch player failed");
		return false;
	}

	OSI_LOGI(0, "[poc][readywork][player](%d):audio poc'reader launching", __LINE__);

	return true;
#else
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;

    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
   	auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};

	lv_poc_setting_set_current_volume(POC_MMI_VOICE_PLAY, lv_poc_setting_get_current_volume(POC_MMI_VOICE_PLAY), true);
	player->status = auPlayerStartReader(player->player, AUSTREAM_FORMAT_PCM, params, (auReader_t *)player->reader);

	return player->status;

#endif
}

/**
 * \brief reset poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return none
 */
int pocAudioPlayerReset(POCAUDIOPLAYER_HANDLE player_id)
{
	if(player_id == 0)
	{
		return -1;
	}
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;

	player->writer->pos = 0;

	player->reader->pos = 2048;

	memset(player->writer->buf, 0, player->writer->max_size);

	player->restart = false;

	player->reduce = false;

	player->reduce_index = false;

	player->raise = false;

	return 0;
}

/**
 * \brief stop poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return false is failed to stop player, true is success
 */
bool pocAudioPlayerStop(POCAUDIOPLAYER_HANDLE player_id)
{
	if(player_id == 0)
	{
		return false;
	}

#if PLAYER_POC_MODE

	if(!audevStopPocMode())
	{
		OSI_LOGI(0, "[idtpoc][player]stop poc mode failed");
		return false;
	}
	pocAudioRecorderStopPocMode();

#endif

	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;
	if(auPlayerStop((auPlayer_t *)player->player))
	{
		player->status = false;
		pocAudioPlayerReset(player_id);
		return true;
	}

	return false;
}

/**
 * \brief delete poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return false is failed to delete player, true is success
 */
bool pocAudioPlayerDelete(POCAUDIOPLAYER_HANDLE       player_id)
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

/**
 * \brief write data to poc audio player
 *
 * param player_id  ID of POC audio player
 *       data       address of data
 *       length     length of data
 *
 * return -1 is fail write data, others is length of write data
 */
int pocAudioPlayerWriteData(POCAUDIOPLAYER_HANDLE player_id, const uint8_t *data, uint32_t length)
{
	if(player_id == 0)
	{
		return -1;
	}

	int ret = 0;
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;
	int count = player->writer->pos - player->reader->pos;
	if(player->restart == true)
	{
		count = player->writer->max_size + (count<=0 ? count:-count);
	}

	if(player->reduce)
	{
		player->reduce_index = !player->reduce_index;
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
	if(count >= POCAUDIOPLAYERDATAPREBUFFSIZE)
	{
		player->reduce = true;
	}
	return auWriterWrite((auWriter_t *)player->writer, data, length);
}

/**
 * \brief get status of poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return true is playying
 */
bool pocAudioPlayerGetStatus(POCAUDIOPLAYER_HANDLE player_id)
{
	if(player_id == 0)
	{
		return false;
	}
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;

	return player->status;
}

/**
 * \brief get poc audio player id
 *
 * param NULL
 *
 * return id
 */
POCAUDIOPLAYER_HANDLE pocAudioPlayerId(void)
{
	return PocMode_playerid;
}

/**
 * \brief stop poc_mode audio player
 *
 * param NULL
 *
 * return false is failed to stop player, true is success
 */
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

/**
 * \brief start poc read
 *
 * param none
 *
 * return true is ok
 */
bool pocAudioPlayerStartPocModeReady(void)
{
	pocAudioRecorder_t * recorder = (pocAudioRecorder_t *)pocAudioRecordId();
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)pocAudioPlayerId();

	auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
	auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};

	if (!audevSetRecordSampleRate(8000))
	{
		OSI_LOGI(0, "[poc][readywork](%d):audio poc set samplerate fail", __LINE__);
		return false;
	}

	player->status = auPlayerStartReaderV2(player->player, AUDEV_PLAY_TYPE_POC, AUSTREAM_FORMAT_PCM, params, (auReader_t *)player->reader);
	if(!player->status)
	{
		OSI_LOGI(0, "[poc][readywork](%d):recorder's reconfig error", __LINE__);
		return false;
	}

	recorder->status = auRecorderStartWriter(recorder->recorder, AUDEV_RECORD_TYPE_POC, AUSTREAM_FORMAT_PCM, NULL, (auWriter_t *)recorder->writer);
	if(!recorder->status)
	{
		OSI_LOGI(0, "[poc][readywork](%d):recorder's audio poc recorder startwriter failed", __LINE__);
		return false;
	}

	if(!audevStartPocMode(AUPOC_STATUS_HALF_DUPLEX))
	{
		OSI_LOGI(0, "[poc][readywork](%d):poc mode start failed", __LINE__);
		auRecorderStop(recorder->recorder);
		auPlayerStop(player->player);
		return false;
	}

	return true;
}

const unsigned char pcm_di[] = {0x19, 0xEC, 0x41, 0xCC, 0xAC, 0xB3, 0xA4, 0xA8, 0x5F, 0xAC, 0x34, 0xBC, 0x51, 0xD6, 0x11, 0xF7, 0xDF, 0x19, 0xA6, 0x37,
                                0xA3, 0x4B, 0xE1, 0x54, 0xBD, 0x4F, 0xEB, 0x3B, 0xC5, 0x1D, 0x97, 0xFB, 0xBF, 0xD9, 0x21, 0xBD, 0x0D, 0xAC, 0x57, 0xA9, 0xF2, 0xB3, 0x7C, 0xC9, 0x6D, 0xE7, 0x98, 0x09, 0xFD, 0x28, 0xD2, 0x40,
                                0xDC, 0x4E, 0x83, 0x50, 0x8C, 0x43, 0x97, 0x2A, 0x0B, 0x0B, 0x94, 0xE9, 0x26, 0xCB, 0x4C, 0xB5, 0xF1, 0xAC, 0x89, 0xB1, 0xFE, 0xC0, 0x8A, 0xDA, 0xC1, 0xF9, 0xA3, 0x18, 0x5C, 0x32, 0x62, 0x43,
                                0x35, 0x4A, 0x48, 0x43, 0xF7, 0x2E, 0xA4, 0x13, 0x33, 0xF5, 0x8B, 0xD7, 0x85, 0xBF, 0x0C, 0xB3, 0x7C, 0xB3, 0xBA, 0xBE, 0xFC, 0xD3, 0x96, 0xF0, 0x72, 0x0F, 0xF8, 0x29, 0xFA, 0x3B, 0xF1, 0x43,
                                0xFF, 0x3F, 0x74, 0x2F, 0x17, 0x16, 0x33, 0xF9, 0xD9, 0xDB, 0x57, 0xC3, 0x77, 0xB5, 0x2E, 0xB4, 0x1D, 0xBE, 0xE2, 0xD1, 0xDB, 0xEC, 0xE6, 0x0A, 0x17, 0x25, 0x9C, 0x37, 0x0A, 0x41, 0x1F, 0x3F,
                                0xC6, 0x30, 0x3F, 0x18, 0x06, 0xFB, 0x0A, 0xDD, 0xFB, 0xC2, 0xC2, 0xB2, 0x8E, 0xAF, 0x83, 0xB8, 0x7B, 0xCB, 0x5A, 0xE6, 0x68, 0x05, 0xE5, 0x21, 0xA4, 0x37, 0x5C, 0x44, 0x2D, 0x45, 0xC7, 0x38,
                                0x83, 0x20, 0xE5, 0x01, 0x0A, 0xE2, 0x8C, 0xC4, 0xA5, 0xAF, 0x38, 0xA8, 0x71, 0xAE, 0xE8, 0xBF, 0xC0, 0xDA, 0xA7, 0xFB, 0xE3, 0x1B, 0x0D, 0x36, 0x1C, 0x47, 0x3F, 0x4C, 0xC3, 0x43, 0xD4, 0x2D,
                                0xA0, 0x0F, 0xD4, 0xEE, 0x1F, 0xCF, 0xD9, 0xB5, 0x8E, 0xA8, 0x6E, 0xA9, 0x69, 0xB6, 0xEC, 0xCD, 0x27, 0xED, 0x57, 0x0E, 0xD8, 0x2B, 0x11, 0x41, 0xAC, 0x4B, 0x66, 0x49, 0x2C, 0x39, 0x9F, 0x1E,
                                0xE7, 0xFE, 0xB6, 0xDE, 0xD9, 0xC2, 0xA2, 0xB0, 0x6B, 0xAB, 0x89, 0xB2, 0x99, 0xC4, 0xB7, 0xDF, 0x05, 0xFF, 0x90, 0x1C, 0x16, 0x34, 0x36, 0x43, 0xBC, 0x46, 0x02, 0x3D, 0xF6, 0x27, 0x0A, 0x0C,
                                0xFB, 0xED, 0xE1, 0xD1, 0x4D, 0xBD, 0x8F, 0xB4, 0x53, 0xB7, 0x96, 0xC4, 0xDD, 0xDA, 0x7A, 0xF6, 0x85, 0x12, 0x47, 0x2A, 0x15, 0x3B, 0x1B, 0x42, 0xDB, 0x3C, 0x38, 0x2C, 0x1F, 0x14, 0xC5, 0xF8,
                                0xD9, 0xDD, 0x44, 0xC8, 0xBD, 0xBC, 0x6E, 0xBC, 0x29, 0xC6, 0x0F, 0xD9, 0xE5, 0xF2, 0x66, 0x0E, 0xFD, 0x25, 0xF3, 0x36, 0x2C, 0x3F, 0x2F, 0x3C, 0xE5, 0x2D, 0x65, 0x17, 0x0B, 0xFD, 0x9F, 0xE2,
                                0x71, 0xCC, 0x8E, 0xBF, 0xF4, 0xBD, 0x78, 0xC6, 0x4C, 0xD8, 0xFB, 0xF0, 0xF1, 0x0B, 0xF1, 0x23, 0x90, 0x35, 0xE9, 0x3E, 0xAA, 0x3D, 0x46, 0x31, 0x2C, 0x1C, 0xB3, 0x02, 0xBE, 0xE8, 0xD6, 0xD1,
                                0xBE, 0xC2, 0x71, 0xBE, 0x97, 0xC4, 0xBD, 0xD3, 0x6D, 0xEA, 0x1C, 0x05, 0x67, 0x1E, 0x4E, 0x32, 0x5B, 0x3E, 0x95, 0x40, 0xE9, 0x37, 0xAE, 0x25, 0xA2, 0x0D, 0x46, 0xF3, 0x41, 0xDA, 0x9A, 0xC7,
                                0x8E, 0xBF, 0x11, 0xC2, 0xCC, 0xCD, 0xC4, 0xE1, 0xA6, 0xFA, 0xFF, 0x13, 0x89, 0x29, 0xBC, 0x38, 0x91, 0x3F, 0xA2, 0x3B, 0x8B, 0x2D, 0x32, 0x18, 0x4F, 0xFF, 0x5D, 0xE6, 0x67, 0xD1, 0x27, 0xC5,
                                0x19, 0xC3, 0x53, 0xCA, 0x2C, 0xDA, 0x94, 0xF0, 0x2F, 0x09, 0x44, 0x1F, 0x54, 0x30, 0x46, 0x3A, 0x9F, 0x3A, 0xB3, 0x30, 0x24, 0x1F, 0x9F, 0x09, 0xA0, 0xF2, 0x7B, 0xDD, 0x11, 0xCF, 0xCB, 0xC9,
                                0xF4, 0xCC, 0x2D, 0xD8, 0x97, 0xEA, 0xDF, 0x00, 0x35, 0x16, 0xA4, 0x27, 0x1D, 0x33, 0x4A, 0x36, 0xF7, 0x2F, 0xBE, 0x21, 0xBD, 0x0E, 0xD0, 0xF9, 0x79, 0xE5, 0x1A, 0xD6, 0xE5, 0xCE, 0xFC, 0xCF,
                                0xA2, 0xD8, 0x54, 0xE8, 0xAD, 0xFC, 0x47, 0x11, 0x2C, 0x22, 0x7A, 0x2D, 0xDC, 0x31, 0x9E, 0x2D, 0x69, 0x21, 0x1E, 0x10, 0x9F, 0xFC, 0x8B, 0xE9, 0x21, 0xDA, 0x34, 0xD2, 0xA2, 0xD2, 0x28, 0xDA,
                                0x35, 0xE8, 0xE2, 0xFA, 0x6A, 0x0E, 0x47, 0x1F, 0x4F, 0x2B, 0xC5, 0x30, 0x14, 0x2E, 0x46, 0x23, 0xA6, 0x12, 0x9A, 0xFF, 0x67, 0xEC, 0x2E, 0xDC, 0xE3, 0xD2, 0xC2, 0xD1, 0x4F, 0xD8, 0x1F, 0xE5,
                                0xD5, 0xF6, 0x6B, 0x0A, 0xB2, 0x1B, 0x48, 0x28, 0x0E, 0x2F, 0x8D, 0x2E, 0x18, 0x26, 0x60, 0x17, 0x75, 0x05, 0x97, 0xF2, 0x7E, 0xE1, 0xDC, 0xD5, 0x45, 0xD2, 0xE8, 0xD5, 0xC2, 0xDF, 0x6F, 0xEF,
                                0xF7, 0x01, 0xBA, 0x13, 0xF4, 0x21, 0x0D, 0x2B, 0x9B, 0x2D, 0x6D, 0x28, 0x97, 0x1C, 0x78, 0x0C, 0x35, 0xFA, 0xB7, 0xE8, 0x7E, 0xDB, 0x11, 0xD5, 0xCC, 0xD5, 0x39, 0xDD, 0x22, 0xEA, 0x9B, 0xFA,
                                0x8E, 0x0B, 0x1B, 0x1A, 0x75, 0x24, 0xF4, 0x28, 0x6A, 0x26, 0x37, 0x1D, 0x93, 0x0F, 0x0D, 0x00, 0x60, 0xF0, 0x4D, 0xE3, 0xC4, 0xDB, 0x97, 0xDA, 0xFB, 0xDE, 0x7C, 0xE8, 0xD0, 0xF5, 0x58, 0x04,
                                0x60, 0x11, 0x23, 0x1B, 0x89, 0x20, 0x5C, 0x20, 0x32, 0x1A, 0xB3, 0x0F, 0xCE, 0x02, 0x33, 0xF5, 0x2E, 0xE9, 0x4A, 0xE1, 0xD7, 0xDE, 0x49, 0xE1, 0x36, 0xE8, 0x06, 0xF3, 0xB8, 0xFF, 0xCD, 0x0B,
                                0x51, 0x15, 0x07, 0x1B, 0xFF, 0x1B, 0xAE, 0x17, 0x08, 0x0F, 0xFE, 0x03, 0x3E, 0xF8, 0x89, 0xED, 0x0C, 0xE6, 0x0A, 0xE3, 0x6A, 0xE4, 0xB6, 0xE9, 0x67, 0xF2, 0x21, 0xFD, 0xBD, 0x07, 0x67, 0x10,
                                0x1B, 0x16, 0xF5, 0x17, 0x34, 0x15, 0x70, 0x0E, 0x38, 0x05, 0xDD, 0xFA, 0xC3, 0xF0, 0xE6, 0xE8, 0xEF, 0xE4, 0x2D, 0xE5, 0x26, 0xE9, 0x9A, 0xF0, 0x86, 0xFA, 0xB7, 0x04, 0x70, 0x0D, 0x96, 0x13,
                                0x44, 0x16, 0xDA, 0x14, 0x75, 0x0F, 0x55, 0x07, 0xCD, 0xFD, 0x3C, 0xF4, 0x4E, 0xEC, 0x9A, 0xE7, 0xCB, 0xE6, 0xC5, 0xE9, 0xFA, 0xEF, 0x82, 0xF8, 0xCC, 0x01, 0x11, 0x0A, 0x51, 0x10, 0xC1, 0x13,
                                0xB2, 0x13, 0xE0, 0x0F, 0x25, 0x09, 0xE7, 0x00, 0x5A, 0xF8, 0xB3, 0xF0, 0x88, 0xEB, 0xCC, 0xE9, 0x50, 0xEB, 0xC9, 0xEF, 0x9E, 0xF6, 0x8D, 0xFE, 0x31, 0x06, 0x80, 0x0C, 0x86, 0x10, 0x89, 0x11,
                                0x64, 0x0F, 0x8F, 0x0A, 0x11, 0x04, 0xD6, 0xFC, 0xE1, 0xF5, 0xA0, 0xF0, 0x27, 0xEE, 0x70, 0xEE, 0x2A, 0xF1, 0x07, 0xF6, 0x34, 0xFC, 0x73, 0x02, 0xC2, 0x07, 0x97, 0x0B, 0x49, 0x0D, 0x6E, 0x0C,
                                0x33, 0x09, 0x5B, 0x04, 0xCC, 0xFE, 0x30, 0xF9, 0x9A, 0xF4, 0x03, 0xF2, 0x9D, 0xF1, 0x4E, 0xF3, 0xEF, 0xF6, 0xC4, 0xFB, 0xEC, 0x00, 0x7E, 0x05, 0xD0, 0x08, 0x7E, 0x0A, 0x23, 0x0A, 0xF1, 0x07,
                                0x5E, 0x04, 0x2F, 0x00, 0xDB, 0xFB, 0x0E, 0xF8, 0xA9, 0xF5, 0x03, 0xF5, 0x07, 0xF6, 0x7C, 0xF8, 0xEB, 0xFB, 0xBF, 0xFF, 0x53, 0x03, 0x14, 0x06, 0xBC, 0x07, 0xFE, 0x07, 0xB5, 0x06, 0x41, 0x04,
                                0x1E, 0x01, 0xB7, 0xFD, 0xB8, 0xFA, 0xA0, 0xF8, 0xDD, 0xF7, 0x51, 0xF8, 0xDA, 0xF9, 0x60, 0xFC, 0x59, 0xFF, 0x1C, 0x02, 0x4E, 0x04, 0xC2, 0x05, 0x2A, 0x06, 0x5F, 0x05, 0x82, 0x03, 0x18, 0x01,
                                0x8F, 0xFE, 0x30, 0xFC, 0x7B, 0xFA, 0xC0, 0xF9, 0xFF, 0xF9, 0x21, 0xFB, 0xE3, 0xFC, 0xF3, 0xFE, 0xE1, 0x00, 0x65, 0x02, 0x64, 0x03, 0xBC, 0x03, 0x70, 0x03, 0x80, 0x02, 0x37, 0x01, 0xDC, 0xFF,
                                0x38, 0xFE
                               };


bool pocAudioPlayerSound(void)
{
	bool player_status;

    auMemReader_t *memreader = auMemReaderCreate(pcm_di,sizeof(pcm_di));//502
    auReader_t *reader = (auReader_t *)memreader;
    auPlayer_t *player = auPlayerCreate();
    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
    auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
    player_status = auPlayerStartReader(player, AUSTREAM_FORMAT_PCM, params, reader);

    auPlayerWaitFinish(player, OSI_WAIT_FOREVER);

	return player_status;
}

#endif
