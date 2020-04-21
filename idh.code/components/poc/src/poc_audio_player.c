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
#ifdef CONFIG_POC_AUDIO_PLAYER_SUPPORT

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

    if (size == 0)
    {
        return 0;
	}
	
    if (buf == NULL)
    {
        return -1;
	}
	
	if(p->pos + size <= p->size)
	{
		memcpy(buf, (const char *)p->buf + p->pos, size);
		p->pos = p->pos + size;
	}
	else
	{
		if(p->size > p->pos)
		{
			memcpy(buf, (const char *)p->buf + p->pos, p->size - p->pos);
		}
		memcpy((char *)buf + (p->size - p->pos), (const char *)p->buf, size - (p->size - p->pos));
		p->pos = size - ( p->size - p->pos );
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
    p->pos = 0;
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
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;	
    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
    auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
	return auPlayerStartReader(player->player, AUSTREAM_FORMAT_PCM, params, (auReader_t *)player->reader);
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
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;

	player->writer->pos = 0;

	player->reader->pos = 0;
	
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
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;
	
	return auPlayerStop((auPlayer_t *)player->player);
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
	pocAudioPlayer_t * player = (pocAudioPlayer_t *)player_id;
	
	return auWriterWrite((auWriter_t *)player->writer, data, length);
}

#endif
