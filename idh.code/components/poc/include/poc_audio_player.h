﻿/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
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

#define POCAUDIOPLAYERDATAPREBUFFSIZE (30*320U)

OSI_EXTERN_C_BEGIN

void pocAudioPlayerInit(void);

POCAUDIOPLAYER_HANDLE pocAudioPlayerCreate(const uint32_t max_size);

bool pocAudioPlayerStart(POCAUDIOPLAYER_HANDLE player_id);

int pocAudioPlayerReset(POCAUDIOPLAYER_HANDLE player_id);

bool pocAudioPlayerStop(POCAUDIOPLAYER_HANDLE player_id);

bool pocAudioPlayerDelete(POCAUDIOPLAYER_HANDLE player_id);

bool pocAudioPlayerGetStatus(POCAUDIOPLAYER_HANDLE player_id);

POCAUDIOPLAYER_HANDLE pocAudioPlayerId(void);

bool pocAudioPlayerStartPocModeReady(void);

bool pocAudioPlayerStopPocMode(void);

void pocAudioSendMsgWriteData(const uint8_t *data, uint32_t length);

int pocAudioPlayerWriteData(POCAUDIOPLAYER_HANDLE player_id, const uint8_t *data, uint32_t length);

void pocAudioPlayerThreadResume(void);

void pocAudioPlayerThreadSuspend(void);

OSI_EXTERN_C_END

#endif

