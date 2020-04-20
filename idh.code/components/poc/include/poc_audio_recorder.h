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

POCAUDIORECORDER_HANDLE pocAudioRecorderCreate(const uint32_t max_size,
													const uint32_t data_length,
													const uint32_t duration,
													pocAudioRecorderCallback_t callback);

int pocAudioRecorderStart(POCAUDIORECORDER_HANDLE recorder_id);

int pocAudioRecorderReset(POCAUDIORECORDER_HANDLE recorder_id);

bool pocAudioRecorderStop(POCAUDIORECORDER_HANDLE recorder_id);

bool pocAudioRecorderDelete(POCAUDIORECORDER_HANDLE       recorder_id);

OSI_EXTERN_C_END

#endif


