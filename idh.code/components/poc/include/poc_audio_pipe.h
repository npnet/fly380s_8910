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

#ifndef _POC_AUDIO_PIPE_H_
#define _POC_AUDIO_PIPE_H_

#include "osi_compiler.h"
#include "poc_config.h"
#include "poc_types.h"
#include "audio_player.h"

OSI_EXTERN_C_BEGIN

bool pocAudioPipeCreate(const uint32_t max_size);

bool pocAudioPipeStartPocModeReady(void);

bool pocAudioPipeStart(void);

bool pocAudioPipeStop(void);

void pocAudioPipeWriteData(const uint8_t *data, uint32_t length);

OSI_EXTERN_C_END

#endif

