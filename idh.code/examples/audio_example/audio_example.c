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
#if 0

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

#else

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
#include "audio_device.h"
#include "vfs.h"
#include <string.h>
#include <stdlib.h>

typedef enum
{
    AUPOC_STATUS_IDLE,
    AUPOC_STATUS_FULL,
    AUPOC_STATUS_HALF_RECV,
    AUPOC_STATUS_HALF_SEND,
    AUPOC_STATUS_FINISHED,
} auPoCStatus_t;

struct auPoC
{
    auPoCStatus_t status; // PoC status
    auRecorder_t *recorder;
    osiPipe_t *rec_pipe;
    auPlayer_t *player;
    osiPipe_t *ply_pipe;
    auFrame_t frame;
    auStreamFormat_t stream_format; // stream format
};
typedef struct auPoC auPoC_t;

#define AUPOC_RECORD_PIPE_SIZE (32000)
#define AUPOC_PLAYER_PIPE_SIZE (32000)

bool auPoCStart(auPoC_t *d, auStreamFormat_t format, uint32_t samplerate, bool fullduplex)
{
    OSI_LOGI(0, "audio poc format/%d samplerate/%d fullduplex/%d", format, samplerate, fullduplex);

    if (d->status != AUPOC_STATUS_IDLE)
    {
        OSI_LOGI(0, "audio poc start fail, status/%d", d->status);
        return false;
    }

    if (!audevSetRecordSampleRate(samplerate))
    {
        OSI_LOGI(0, "audio poc set samplerate fail, samplerate/%d", samplerate);
        return false;
    }

    osiPipeReset(d->ply_pipe);

    auFrame_t frame = {.sample_format = AUSAMPLE_FORMAT_S16, .sample_rate = samplerate, .channel_count = 1};
    auDecoderParamSet_t params[2] = {{AU_DEC_PARAM_FORMAT, &frame}, {0}};
    if (!auPlayerStartPipeV2(d->player, AUDEV_PLAY_TYPE_POC, format, ((format == AUSTREAM_FORMAT_PCM) ? params : NULL), d->ply_pipe))
        return false;
	//set audio_device ipc reader waittime out to 0
	//if player data in pipe is empty ipc work,return immediately
    auPipeReaderSetWait((auPipeReader_t *)auPlayerGetReader(d->player), 0);

    auEncoderParamSet_t encparams[2] = {};
    if (format == AUSTREAM_FORMAT_AMRNB)
    {
        static auAmrnbMode_t gAmrnbQuals[] = {
            AU_AMRNB_MODE_515,  // LOW
            AU_AMRNB_MODE_670,  // MEDIUM
            AU_AMRNB_MODE_795,  // HIGH
            AU_AMRNB_MODE_1220, // BEST
        };
        encparams[0].id = AU_AMRNB_ENC_PARAM_MODE;
        encparams[0].val = &gAmrnbQuals[3];
    }

    if (!auRecorderStartPipe(d->recorder, AUDEV_RECORD_TYPE_POC, format, encparams, d->rec_pipe))
    {
        auPlayerStop(d->player);
        return false;
    }

    if (fullduplex)
        d->status = AUPOC_STATUS_FULL;
    else
        d->status = AUPOC_STATUS_HALF_RECV;

    if (!audevStartPocMode(fullduplex))
    {
        auRecorderStop(d->recorder);
        auPlayerStop(d->player);
        return false;
    }

    return true;
}

bool auPoCStop(auPoC_t *d)
{
    if (d->status != AUPOC_STATUS_IDLE)
    {
        audevStopPocMode();
        auRecorderStop(d->recorder);
        auPlayerStop(d->player);
    }
    d->status = AUPOC_STATUS_IDLE;
    return true;
}

/**
 * Create an audio poc
 */
auPoC_t *auPoCCreate(void)
{
    auPoC_t *d = (auPoC_t *)calloc(1, sizeof(auPoC_t));
    if (d == NULL)
        return d;

    d->recorder = auRecorderCreate();
    if (d->recorder == NULL)
        goto rec_create_failed;

    d->rec_pipe = osiPipeCreate(AUPOC_RECORD_PIPE_SIZE);
    if (d->rec_pipe == NULL)
        goto recpipe_create_failed;

    d->player = auPlayerCreate();
    if (d->player == NULL)
        goto ply_create_failed;

    d->ply_pipe = osiPipeCreate(AUPOC_PLAYER_PIPE_SIZE);
    if (d->ply_pipe == NULL)
        goto plypipe_create_failed;

    d->status = AUPOC_STATUS_IDLE;
    d->stream_format = AUSTREAM_FORMAT_UNKNOWN;
    auInitSampleFormat(&d->frame);

    return d;

plypipe_create_failed:
    auPlayerDelete(d->player);
ply_create_failed:
    osiPipeDelete(d->rec_pipe);
recpipe_create_failed:
    auRecorderDelete(d->recorder);
rec_create_failed:
    free(d);
    return NULL;
}

void auPoCDelete(auPoC_t *d)
{
    if (d == NULL)
        return;

    OSI_LOGI(0, "audio poc delete, status/%d", d->status);

    if (d->status != AUPOC_STATUS_IDLE)
        auPoCStop(d);
    d->status = AUPOC_STATUS_IDLE;

    auPlayerDelete(d->player);
    osiPipeDelete(d->ply_pipe);
    auRecorderDelete(d->recorder);
    osiPipeDelete(d->rec_pipe);

    free(d);
}

// read from a poc recorder
ssize_t auPoCRead(auPoC_t *d, void *dst, size_t size, unsigned timeout)
{
    if ((d == NULL) || (dst == NULL))
        return 0;
    if (d->rec_pipe == NULL)
        return 0;
    return osiPipeReadAll(d->rec_pipe, dst, size, timeout);
}

// write to a poc player
ssize_t auPoCWrite(auPoC_t *d, const void *data, size_t size, unsigned timeout)
{
    if ((d == NULL) || (data == NULL))
        return 0;
    if (d->ply_pipe == NULL)
        return 0;

    ssize_t written = osiPipeWriteAll(d->ply_pipe, data, size, timeout);
    return written;
}

bool auPoCHalfModeSwitch(auPoC_t *d, bool half_recv)
{
    if ((half_recv) && (d->status == AUPOC_STATUS_HALF_RECV))
    {
        return true;
    }
    if ((!half_recv) && (d->status == AUPOC_STATUS_HALF_SEND))
    {
        return true;
    }
    if (half_recv)
    {
        audevPocModeSwitch(2);
        d->status = AUPOC_STATUS_HALF_RECV;
        //osiPipeReset(d->ply_pipe);
    }
    else
    {
        audevPocModeSwitch(1);
        d->status = AUPOC_STATUS_HALF_SEND;
    }
    return true;
}

auPoCStatus_t auPoCGetStatus(auPoC_t *d) { return d->status; }

struct AppPoC
{
    auPoC_t *poc;
    bool duplexmode;
    unsigned samplerate;
    auStreamFormat_t format;

    osiThread_t *cmd_thread_id;
    osiSemaphore_t *cmd_sync_sema;

    int send_fd;
    int recv_fd;
    osiThread_t *recv_thread_id;
    osiThread_t *send_thread_id;
};
typedef struct AppPoC AppPoC_t;
AppPoC_t gAppPoCCtx;

#define EV_POC_STOP_REQ (5000)
#define EV_POC_START_FULL_REQ (5001)
#define EV_POC_START_HALF_REQ (5002)
#define EV_POC_HALF_SEND_REQ (5003)
#define EV_POC_HALF_RECV_REQ (5004)

#define EV_POC_SOCKET_RECV_IND (6001)
#define EV_POC_SOCKET_SEND_REQ (6002)

//#define SIMU_TEST_DATA_IN_SDCARD
#ifdef SIMU_TEST_DATA_IN_SDCARD
#define POC_RECV_FILE_NAME "/sdcard0/poc_downlink_recv.pcm"
#define POC_SEND_FILE_NAME "/sdcard0/poc_uplink_send.pcm"
#endif

static bool prvOpenUplinkSendDataSocket(void)
{
#ifdef SIMU_TEST_DATA_IN_SDCARD
    AppPoC_t *d = &gAppPoCCtx;
    d->send_fd = vfs_open(POC_SEND_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC);
    if (d->send_fd < 0)
        return false;
    else
        return true;
#else
    return true;
#endif
}

static ssize_t prvWriteUplinkSendDataToSocket(AppPoC_t *d, const void *data, size_t size)
{
#ifdef SIMU_TEST_DATA_IN_SDCARD
    return vfs_write(d->send_fd, data, size);
#else
    OSI_LOGXI(OSI_LOGPAR_II, 0, "prvWriteUplinkSendDataToSocket data/%x size/%d", data, size);
    return size;
#endif
}

static bool prvCloseUplinkSendDataSocket(void)
{
#ifdef SIMU_TEST_DATA_IN_SDCARD
    AppPoC_t *d = &gAppPoCCtx;
    if (d->send_fd > 0)
        vfs_close(d->send_fd);
#endif
    return true;
}

static bool prvOpenDownlinkRecvDataSocket(void)
{
#ifdef SIMU_TEST_DATA_IN_SDCARD
    AppPoC_t *d = &gAppPoCCtx;
    d->recv_fd = vfs_open(POC_RECV_FILE_NAME, O_RDONLY);
    if (d->recv_fd < 0)
        return false;
    else
        return true;
#else
    return true;
#endif
}

static short *prvPocGenFakeSamples(size_t size,unsigned sample_rate)
{
    short *samples = (short *)malloc(size); // 2s for 8K
    auToneGen_t *tone = auToneGenCreate(sample_rate);
    auToneGenStart(tone, 0x2000, 1209, 697, OSI_WAIT_FOREVER, 0);

    auFrame_t frame = {.data = (uintptr_t)samples, .bytes = 0};
    auToneGenSamples(tone, &frame, size);
    auToneGenDelete(tone);
    return samples;
}

static ssize_t prvReadDownlinkRecvDataFromSocket(AppPoC_t *d, void *dst, size_t size)
{
#ifdef SIMU_TEST_DATA_IN_SDCARD
    return vfs_read(d->recv_fd, dst, size);
#else
    short *samples = prvPocGenFakeSamples(size,d->samplerate);
    memcpy(dst, samples, size);
    free(samples);
    return size;
#endif
}

static bool prvCloseDownlinkRecvDataSocket(void)
{
#ifdef SIMU_TEST_DATA_IN_SDCARD
    AppPoC_t *d = &gAppPoCCtx;
    if (d->recv_fd > 0)
        vfs_close(d->recv_fd);
#endif
    return true;
}

bool AppPoCHalfModeSwitch(AppPoC_t *d, bool half_recv)
{
    if ((d->poc != NULL) && (d->duplexmode == false))
        auPoCHalfModeSwitch(d->poc, half_recv);
    else
        return false;
    return true;
}

static void prvDummyRecvThreadEntry(void *param)
{
    AppPoC_t *d = &gAppPoCCtx;
    osiThread_t *thread = osiThreadCurrent();

    for (;;)
    {
        osiEvent_t event = {};
        osiEventWait(thread, &event);
        if (event.id == OSI_EVENT_ID_QUIT)
            break;

        if (event.id == EV_POC_SOCKET_RECV_IND)
        {
            char buf[640];
            unsigned readframebytes = d->samplerate * 2 / 50;
            unsigned total_bytes = 0;

            uint32_t elapsetime = 0;
            osiElapsedTimer_t perf_time;

            //OSI_LOGXI(OSI_LOGPAR_I, 0, "prvDummyRecvThreadEntry  start auPoCWrite");
            osiElapsedTimerStart(&perf_time);
            for (;;)
            {
                elapsetime = osiElapsedTime(&perf_time);
                //OSI_LOGXI(OSI_LOGPAR_II, 0, "auPoCWrite DownlinkRecvData elapsetime/%d",elapsetime);
                if ((osiPipeWriteAvail(d->poc->ply_pipe) < readframebytes) || (elapsetime > 4))
                {
                    OSI_LOGXI(OSI_LOGPAR_II, 0, "prvDummyRecvThreadEntry auPoCWrite total_bytes/%d elapsetime/%d", total_bytes, elapsetime);
                    break;
                }
                //get data
                ssize_t bytes = prvReadDownlinkRecvDataFromSocket(d, buf, readframebytes);
                //process data
                if (bytes <= 0)
                {

#ifdef SIMU_TEST_DATA_IN_SDCARD
                    vfs_lseek(d->recv_fd, 0, SEEK_SET);
#endif
                    break;
                }
                //send data to player pipe
                bytes = auPoCWrite(d->poc, buf, bytes, 1);
                if (bytes <= 0)
                {
                    elapsetime = osiElapsedTime(&perf_time);
                    //OSI_LOGXI(OSI_LOGPAR_II, 0, "prvDummyRecvThreadEntry auPoCWrite total_bytes/%d elapsetime/%d", total_bytes,elapsetime);
                    break;
                }
                total_bytes += bytes;
            }
        }
    }
    osiThreadExit();
}

static void prvDummySendThreadEntry(void *param)
{
    AppPoC_t *d = &gAppPoCCtx;
    osiThread_t *thread = osiThreadCurrent();

    for (;;)
    {
        osiEvent_t event = {};
        osiEventWait(thread, &event);
        if (event.id == OSI_EVENT_ID_QUIT)
            break;

        if (event.id == EV_POC_SOCKET_SEND_REQ)
        {
            char buf[640];
            unsigned writeframebytes = d->samplerate * 2 / 50;

            unsigned total_bytes = 0;
            uint32_t elapsetime = 0;
            osiElapsedTimer_t perf_time;

            //OSI_LOGXI(OSI_LOGPAR_I, 0, "prvDummySendThreadEntry  start auPoCRead");
            osiElapsedTimerStart(&perf_time);

            for (;;)
            {
                elapsetime = osiElapsedTime(&perf_time);
                OSI_LOGXI(OSI_LOGPAR_II, 0, "auPoCRead UplinkRecvData elapsetime/%d", elapsetime);
                if ((osiPipeReadAvail(d->poc->rec_pipe) < writeframebytes) || (elapsetime > 10))
                {
                    OSI_LOGXI(OSI_LOGPAR_II, 0, "prvDummySendThreadEntry auPoCRead total_bytes/%d elapsetime/%d", total_bytes, elapsetime);
                    break;
                }
                //read data from poc recorder
                ssize_t bytes = auPoCRead(d->poc, buf, writeframebytes, 20);
                //process data
                if (bytes <= 0)
                    break;
                //send data to uplink socket
                if (prvWriteUplinkSendDataToSocket(d, buf, bytes) <= 0)
                    OSI_LOGI(0, "prvWriteUplinkSendDataToSocket failed");
                else
                    OSI_LOGI(0, "prvWriteUplinkSendDataToSocket ok");
                total_bytes += bytes;
            }
        }
    }
    osiThreadExit();
}

static void prvPoCRecPipeReadCallback(void *param, unsigned event)
{
    AppPoC_t *d = (AppPoC_t *)param;
    if (event & OSI_PIPE_EVENT_RX_ARRIVED)
    {
        OSI_LOGXI(OSI_LOGPAR_I, 0, "prvPoCRecPipeReadCallback  event:%d", event);
        osiEvent_t poc_event = {.id = EV_POC_SOCKET_SEND_REQ};
        osiEventSend(d->send_thread_id, &poc_event);
    }
}

static void prvPoCPlyPipeWriteCallback(void *param, unsigned event)
{
    AppPoC_t *d = (AppPoC_t *)param;
    if (event & OSI_PIPE_EVENT_TX_COMPLETE)
    {
        OSI_LOGXI(OSI_LOGPAR_I, 0, "prvPoCPlyPipeWriteCallback  event:%d", event);
        osiEvent_t poc_event = {.id = EV_POC_SOCKET_RECV_IND};
        osiEventSend(d->recv_thread_id, &poc_event);
    }
}

bool AppPoCStart(AppPoC_t *d, bool fullduplex)
{
    //create platform poc device
    if (d->poc == NULL)
    {
        d->poc = auPoCCreate();
        if (d->poc == NULL)
            return false;
        else
        {
            if (!auPoCStart(d->poc, d->format, d->samplerate, fullduplex))
                return false;
        }
        d->duplexmode = fullduplex;
    }
    if (fullduplex != d->duplexmode)
    {
        return false;
    }

    osiPipeSetReaderCallback(d->poc->rec_pipe, OSI_PIPE_EVENT_RX_ARRIVED,
                             prvPoCRecPipeReadCallback, d);

    osiPipeSetWriterCallback(d->poc->ply_pipe, OSI_PIPE_EVENT_TX_COMPLETE,
                             prvPoCPlyPipeWriteCallback, d);

    //open send data socket
    prvOpenUplinkSendDataSocket();

    //open recv data socket
    prvOpenDownlinkRecvDataSocket();

    //start send and recv flow acording with poc mode
    d->recv_thread_id = osiThreadCreate("dummyrecv", prvDummyRecvThreadEntry, NULL, OSI_PRIORITY_NORMAL, (8192 * 4), 32);
    d->send_thread_id = osiThreadCreate("dummysend", prvDummySendThreadEntry, NULL, OSI_PRIORITY_NORMAL, (8192 * 4), 32);

    //fake trigger data comming
    {
        osiEvent_t poc_event = {.id = EV_POC_SOCKET_RECV_IND};
        osiEventSend(d->recv_thread_id, &poc_event);
    }

    return true;
}

bool AppPoCStop(AppPoC_t *d)
{
    if (d->poc == NULL)
        return true;

    OSI_LOGI(0, "AppPoCStop 1");
    //stop platform poc device
    if (d->poc != NULL)
    {
        if (auPoCStop(d->poc))
        {
            auPoCDelete(d->poc);
            d->poc = NULL;
        }
        else
            return false;
    }

    OSI_LOGI(0, "AppPoCStop 2");
    osiSendQuitEvent(d->recv_thread_id, true);
    OSI_LOGI(0, "AppPoCStop 3");

    osiThreadSleep(10);
    osiSendQuitEvent(d->send_thread_id, true);
    OSI_LOGI(0, "AppPoCStop 4");

    osiThreadSleep(10);
    //close send data socket
    prvCloseUplinkSendDataSocket();
    OSI_LOGI(0, "AppPoCStop 5");

    //close recv data socket
    prvCloseDownlinkRecvDataSocket();

    OSI_LOGI(0, "AppPoCStop 6");

    return true;
}


static void prvAudioPocEntry(void *param)
{
    AppPoC_t *d = &gAppPoCCtx;

    osiThreadSleep(1000);

    //poc mode work in 8000 16000 samplerate
    d->samplerate = 8000;
    d->format = AUSTREAM_FORMAT_PCM;

    //demo for halfduplex mode
    //start poc halfduplex mode
    AppPoCStart(d, false);
    OSI_LOGXI(OSI_LOGPAR_I, 0, "prvPocEntry  start poc halfduplex mode");

    for (int n = 0; n < 3; n++)
    {
        osiThreadSleep(5000);
        //switch to record sound and send
        AppPoCHalfModeSwitch(d, false);
        OSI_LOGXI(OSI_LOGPAR_I, 0, "prvPocEntry  switch to record sound and send");

        osiThreadSleep(5000);
        //switch to recv data and play sound
        AppPoCHalfModeSwitch(d, true);
        OSI_LOGXI(OSI_LOGPAR_I, 0, "prvPocEntry  switch to recv data and play sound");
    }

    osiThreadSleep(5000);
    //stop poc mode
    AppPoCStop(d);
    OSI_LOGXI(OSI_LOGPAR_I, 0, "prvPocEntry  stop poc mode");

    //demo for fullduplex mode
    //start poc fullduplex mode
    AppPoCStart(d, true);
    OSI_LOGXI(OSI_LOGPAR_I, 0, "prvPocEntry  start poc fullduplex mode");

    for (int n = 0; n < 3; n++)
    {
        osiThreadSleep(5000);
        osiThreadSleep(5000);
    }

    osiThreadSleep(5000);
    //stop poc mode
    AppPoCStop(d);
    OSI_LOGXI(OSI_LOGPAR_I, 0, "prvPocEntry  stop poc mode");

    osiThreadExit();
}

int appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    osiThreadCreate("aupoc", prvAudioPocEntry, NULL, OSI_PRIORITY_NORMAL, 1024, 4);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}

#endif
