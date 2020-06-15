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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('A', 'P', 'L', 'Y')

#include "osi_api.h"
#include "osi_log.h"
#include "osi_pipe.h"
#include "audio_player.h"
#include "audio_recorder.h"
#include "audio_decoder.h"
#include "audio_reader.h"
#include "audio_writer.h"
#include "audio_tonegen.h"
#include "vfs.h"
#include <string.h>
#include <stdlib.h>

#define EXAMPLE_FILE_NAME "/example.pcm"

typedef struct
{
    osiPipe_t *pipe;
    int fd;
} pipeAndFd_t;

static short *prvGenSamples(void)
{
    short *samples = (short *)malloc(32000); // 2s for 8K
    auToneGen_t *tone = auToneGenCreate(8000);
    auToneGenStart(tone, 0x2000, 1209, 697, OSI_WAIT_FOREVER, 0);

    auFrame_t frame = {.data = (uintptr_t)samples, .bytes = 0};
    auToneGenSamples(tone, &frame, 32000);
    auToneGenDelete(tone);
    return samples;
}

static void prvCreateFile(void)
{
    short *samples = prvGenSamples();
    int fd = vfs_open(EXAMPLE_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC);
    vfs_write(fd, samples, 32000);
    vfs_close(fd);
    free(samples);
}

static void prvPlayMem(void)
{
    short *samples = prvGenSamples();

    auPlayer_t *player = auPlayerCreate();
    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
    auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
    auPlayerStartMem(player, AUSTREAM_FORMAT_PCM, params, samples, 32000);

    auPlayerWaitFinish(player, OSI_WAIT_FOREVER);

    auPlayerDelete(player);
    free(samples);
}

static void prvPlayFile(void)
{
    auPlayer_t *player = auPlayerCreate();
    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
    auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
    auPlayerStartFile(player, AUSTREAM_FORMAT_PCM, params, EXAMPLE_FILE_NAME);

    auPlayerWaitFinish(player, OSI_WAIT_FOREVER);
    auPlayerDelete(player);
}

static void prvPipeWriteWork(void *param)
{
    pipeAndFd_t *d = (pipeAndFd_t *)param;

    char buf[256];
    for (;;)
    {
        int bytes = vfs_read(d->fd, buf, 256);
        if (bytes < 0)
        {
            osiPipeStop(d->pipe);
            break;
        }
        else if (bytes == 0)
        {
            osiPipeSetEof(d->pipe);
            break;
        }
        else
        {
            osiPipeWriteAll(d->pipe, buf, bytes, OSI_WAIT_FOREVER);
        }
    }
}

static void prvPlayPipe(void)
{
    // In "auPlayerStartPipe", audio decoder may read data to detect the
    // audio format, such as WAV and MP3. So, it can't be executed in the
    // same thread to write data to pipe, and it is needed to offload to
    // another thread.

    int fd = vfs_open(EXAMPLE_FILE_NAME, O_RDONLY);
    osiPipe_t *pipe = osiPipeCreate(1024);
    pipeAndFd_t pipefd = {.pipe = pipe, .fd = fd};
    osiWork_t *work = osiWorkCreate(prvPipeWriteWork, NULL, &pipefd);
    osiWorkEnqueue(work, osiSysWorkQueueLowPriority());

    auPlayer_t *player = auPlayerCreate();
    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
    auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
    auPlayerStartPipe(player, AUSTREAM_FORMAT_PCM, params, pipe);

    auPlayerWaitFinish(player, OSI_WAIT_FOREVER);
    auPlayerDelete(player);
    osiWorkDelete(work);
    osiPipeDelete(pipe);
    vfs_close(fd);
}

static void prvPlayReader(void)
{
    auReader_t *reader = (auReader_t *)auFileReaderCreate(EXAMPLE_FILE_NAME);
    auPlayer_t *player = auPlayerCreate();
    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
    auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
    auPlayerStartReader(player, AUSTREAM_FORMAT_PCM, params, reader);

    auPlayerWaitFinish(player, OSI_WAIT_FOREVER);
    auPlayerDelete(player);
    auReaderDelete(reader);
}

static void prvPlayDecoder(void)
{
    auReader_t *reader = (auReader_t *)auFileReaderCreate(EXAMPLE_FILE_NAME);
    auDecoder_t *decoder = auDecoderCreate(reader, AUSTREAM_FORMAT_PCM);

    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = 8000, .channel_count = 1};
    auDecoderSetParam(decoder, AU_DEC_PARAM_FORMAT, &frame);

    auPlayer_t *player = auPlayerCreate();
    auPlayerStartDecoder(player, decoder);

    auPlayerWaitFinish(player, OSI_WAIT_FOREVER);
    auPlayerDelete(player);
    auDecoderDelete(decoder);
    auReaderDelete(reader);
}

static void prvRecordMem(void)
{
    auRecorder_t *recorder = auRecorderCreate();
    auRecorderStartMem(recorder, AUDEV_RECORD_TYPE_MIC, AUSTREAM_FORMAT_PCM, NULL, 32000);

    osiThreadSleep(2000);

    auMemWriter_t *writer = (auMemWriter_t *)auRecorderGetWriter(recorder);
    osiBuffer_t wbuf = auMemWriterGetBuf(writer);
    OSI_LOGI(0, "record memory 0x%x size/%x", wbuf.ptr, wbuf.size);

    auRecorderStop(recorder);
    auRecorderDelete(recorder);
}

static void prvRecordFile(void)
{
    auRecorder_t *recorder = auRecorderCreate();
    auRecorderStartFile(recorder, AUDEV_RECORD_TYPE_MIC, AUSTREAM_FORMAT_PCM, NULL, EXAMPLE_FILE_NAME);

    osiThreadSleep(2000);

    auRecorderStop(recorder);
    auRecorderDelete(recorder);

    int file_size = vfs_file_size(EXAMPLE_FILE_NAME);
    OSI_LOGI(0, "record file size/%d", file_size);
}

static void prvPipeReadWork(void *param)
{
    pipeAndFd_t *d = (pipeAndFd_t *)param;

    char buf[256];
    for (;;)
    {
        int bytes = osiPipeRead(d->pipe, buf, 256);
        if (bytes <= 0)
            break;

        vfs_write(d->fd, buf, bytes);
    }
}

static void prvPipeReadCallback(void *param, unsigned event)
{
    osiWork_t *work = (osiWork_t *)param;
    osiWorkEnqueue(work, osiSysWorkQueueLowPriority());
}

static void prvRecordPipe(void)
{
    int fd = vfs_open(EXAMPLE_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC);
    osiPipe_t *pipe = osiPipeCreate(4096);
    pipeAndFd_t pipefd = {.pipe = pipe, .fd = fd};
    osiWork_t *work = osiWorkCreate(prvPipeReadWork, NULL, &pipefd);
    osiPipeSetReaderCallback(pipe, OSI_PIPE_EVENT_RX_ARRIVED, prvPipeReadCallback, work);

    auRecorder_t *recorder = auRecorderCreate();
    auRecorderStartPipe(recorder, AUDEV_RECORD_TYPE_MIC, AUSTREAM_FORMAT_PCM, NULL, pipe);

    osiThreadSleep(2000);

    auRecorderStop(recorder);
    auRecorderDelete(recorder);
    osiWorkWaitFinish(work, OSI_WAIT_FOREVER);
    osiWorkDelete(work);
    osiPipeDelete(pipe);
    vfs_close(fd);

    int file_size = vfs_file_size(EXAMPLE_FILE_NAME);
    OSI_LOGI(0, "record pipe file size/%d", file_size);
}

static void prvRecordWriter(void)
{
    auWriter_t *writer = (auWriter_t *)auFileWriterCreate(EXAMPLE_FILE_NAME);
    auRecorder_t *recorder = auRecorderCreate();
    auRecorderStartWriter(recorder, AUDEV_RECORD_TYPE_MIC, AUSTREAM_FORMAT_PCM, NULL, writer);

    osiThreadSleep(2000);

    auRecorderStop(recorder);
    auRecorderDelete(recorder);
    auWriterDelete(writer);

    int file_size = vfs_file_size(EXAMPLE_FILE_NAME);
    OSI_LOGI(0, "record writer file size/%d", file_size);
}

static void prvRecordEncoder(void)
{
    auWriter_t *writer = (auWriter_t *)auFileWriterCreate(EXAMPLE_FILE_NAME);
    auEncoder_t *encoder = auEncoderCreate(writer, AUSTREAM_FORMAT_PCM);
    auRecorder_t *recorder = auRecorderCreate();
    auRecorderStartEncoder(recorder, AUDEV_RECORD_TYPE_MIC, encoder);

    osiThreadSleep(2000);

    auRecorderStop(recorder);
    auRecorderDelete(recorder);
    auEncoderDelete(encoder);
    auWriterDelete(writer);

    int file_size = vfs_file_size(EXAMPLE_FILE_NAME);
    OSI_LOGI(0, "record encoder file size/%d", file_size);
}

static void prvAudioPlayEntry(void *param)
{
    osiThreadSleep(1000);
    prvCreateFile();

    prvPlayMem();
    osiThreadSleep(1000);

    prvPlayFile();
    osiThreadSleep(1000);

    prvPlayPipe();
    osiThreadSleep(1000);

    prvPlayReader();
    osiThreadSleep(1000);

    prvPlayDecoder();
    osiThreadSleep(1000);

    prvRecordMem();
    prvRecordFile();
    prvRecordPipe();
    prvRecordWriter();
    prvRecordEncoder();

    vfs_unlink(EXAMPLE_FILE_NAME);
    osiThreadExit();
}

int appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    osiThreadCreate("auplay", prvAudioPlayEntry, NULL, OSI_PRIORITY_NORMAL, 1024, 4);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
