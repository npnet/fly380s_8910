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

#ifndef _POC_AUDIO_RECORDER_H_
#define _POC_AUDIO_RECORDER_H_

#include "osi_compiler.h"
#include "poc_config.h"
#include "poc_types.h"
#include "audio_recorder.h"

OSI_EXTERN_C_BEGIN
/**
 * \brief initialize poc audio recorder
 *
 * poc Audio recorder is designed as singleton.
 */
void pocAudioRecorderInit(void);

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
											   pocAudioRecorderCallback_t callback);

/**
 * \brief start poc audio recorder
 *
 * param recorder_id   ID of POC audio recorder
 *
 * return false is failed, true is success to record
 */
bool pocAudioRecorderStart(POCAUDIORECORDER_HANDLE recorder_id);

/**
 * \brief reset poc audio recorder
 *
 * param recorder_id   ID of POC audio recorder
 *
 * return none
 */
int pocAudioRecorderReset(POCAUDIORECORDER_HANDLE recorder_id);

/**
 * \brief stop poc audio recorder
 *
 * param recorder_id   ID of POC audio recorder
 *
 * return false is fail to stop record, true is success
 */
bool pocAudioRecorderStop(POCAUDIORECORDER_HANDLE recorder_id);

/**
 * \brief delete poc audio recorder
 *
 * param recorder_id   ID of POC audio recorder
 *
 * return return false is fail to delete record, true is success
 */
bool pocAudioRecorderDelete(POCAUDIORECORDER_HANDLE recorder_id);
/**
 * \brief get status of poc audio recorder
 *
 * param player_id  ID of POC audio player
 *
 * return true is recording
 */
bool pocAudioRecorderGetStatus(POCAUDIORECORDER_HANDLE recorder_id);

void lv_poc_start_recordwriter(void);
void lv_poc_start_playfile(void);

osiThread_t *pocAudioRecorderThread(void);

OSI_EXTERN_C_END

#endif


