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

#ifndef _POC_TYPES_H_
#define _POC_TYPES_H_

#include "osi_compiler.h"
#include "poc_config.h"

#include "osi_log.h"
#include "osi_api.h"
#include "osi_pipe.h"
#include "audio_writer.h"
#include "audio_reader.h"
#include "audio_player.h"
#include "audio_recorder.h"
#include "ml.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

OSI_EXTERN_C_BEGIN
;

typedef uint32_t POCAUDIOPLAYER_HANDLE;
typedef uint32_t POCAUDIORECORDER_HANDLE;
typedef void (*pocAudioRecorderCallback_t)(uint8_t * data, uint32_t length);

/**
 * Memory writer
 */
struct auPocMemWriter
{
    auWriterOps_t ops;
    unsigned max_size;
    unsigned size;
    unsigned pos;
    void    *user;
    char    *buf;
};

typedef struct auPocMemWriter auPocMemWriter_t;

/**
 * memory reader
 */
struct auPocMemReader
{
    auReaderOps_t ops;
    const void *buf;
    unsigned size;
    unsigned pos;
    void    *user;
    osiSemaphore_t *sema;
};

typedef struct auPocMemReader auPocMemReader_t;

typedef struct 
{
	auPocMemWriter_t *writer;
	auPocMemReader_t *reader;
	auPlayer_t       *player;
} pocAudioPlayer_t;

typedef struct
{
	auPocMemWriter_t *writer;
	auPocMemReader_t *reader;
	auRecorder_t     *recorder;
	uint8_t          *prvSwapData;
	uint32_t          prvSwapDataLength;
	uint32_t          prvTimerDuration;
	osiTimer_t       *prvTimerID;
	pocAudioRecorderCallback_t callback;
} pocAudioRecorder_t;

OSI_EXTERN_C_END

#endif


