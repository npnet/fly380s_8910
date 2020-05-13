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

#ifndef _POC_AUDIO_PLAYER_H_
#define _POC_AUDIO_PLAYER_H_

#include "osi_compiler.h"
#include "poc_config.h"
#include "poc_types.h"
#include "audio_player.h"

#define POCAUDIOPLAYERDATAPREBUFFSIZE (40*320U)

OSI_EXTERN_C_BEGIN
/**
 * \brief initialize poc audio player
 *
 * poc Audio player is designed as singleton.
 */
void pocAudioPlayerInit(void);

/**
 * \brief create poc audio player
 *
 * param max_size     length of storage data
 *
 * return ID of POC audio player, zero is failed
 */
POCAUDIOPLAYER_HANDLE pocAudioPlayerCreate(const uint32_t max_size);

/**
 * \brief start poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return false is failed to start player, true is success
 */
bool pocAudioPlayerStart(POCAUDIOPLAYER_HANDLE player_id);

/**
 * \brief reset poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return none
 */
int pocAudioPlayerReset(POCAUDIOPLAYER_HANDLE player_id);

/**
 * \brief stop poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return false is failed to stop player, true is success
 */
bool pocAudioPlayerStop(POCAUDIOPLAYER_HANDLE player_id);

/**
 * \brief delete poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return false is failed to delete player, true is success
 */
bool pocAudioPlayerDelete(POCAUDIOPLAYER_HANDLE       player_id);

/**
 * \brief write data to poc audio player
 *
 * param player_id  ID of POC audio player
 *       data       address of data
 *       length     length of data
 *
 * return -1 is fail write data, others is length of write data
 */
int pocAudioPlayerWriteData(POCAUDIOPLAYER_HANDLE player_id, const uint8_t *data, uint32_t length);

/**
 * \brief get status of poc audio player
 *
 * param player_id  ID of POC audio player
 *
 * return true is playying
 */
bool pocAudioPlayerGetStatus(POCAUDIOPLAYER_HANDLE player_id);

OSI_EXTERN_C_END

#endif

