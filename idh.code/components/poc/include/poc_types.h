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
    void    *user;                      //specified data
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
    void    *user;                      //specified data
    osiSemaphore_t *sema;
};

typedef struct auPocMemReader auPocMemReader_t;

/**
 * poc audio player struct
 */
typedef struct
{
	auPocMemWriter_t *writer;           //storage data that through write API to play
	auPocMemReader_t *reader;           //get data from writer to play
	auPlayer_t       *player;           //real player
    bool              restart;
    bool              reduce;
    bool              raise;
    bool              reduce_index;
	bool              status;           //true is playying
} pocAudioPlayer_t;

/**
 * poc audio recorder struct
 */
typedef struct
{
	auPocMemWriter_t *writer;           //storage data that through write API to record
	auPocMemReader_t *reader;           //get data from writer to send others
	bool              prvRestart;       //restart write data
	auRecorder_t     *recorder;         //real recorder
	uint8_t          *prvSwapData;      //storage temp data
	uint32_t          prvSwapDataLength;//length of temp data
	uint32_t          prvDuration; //duration of send data to others
	osiThread_t      *prvThreadID;       //thread which can execute callback
	pocAudioRecorderCallback_t callback;//user specify func
	bool              status;           //status of recorder
} pocAudioRecorder_t;

OSI_EXTERN_C_END

#endif


