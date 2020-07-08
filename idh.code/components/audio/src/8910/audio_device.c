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

#include "audio_device.h"
#include "audio_player.h"
#include "audio_recorder.h"
#include "osi_api.h"
#include "osi_log.h"
#include "hal_chip.h"
#include "hal_shmem_region.h"
#include "hal_config.h"
#include "drv_md_ipc.h"
#include "pb_util.h"
#include "audio_device.pb.h"
#include "vfs.h"
#include <stdlib.h>
#include <string.h>

#define INT8 int8_t
#define UINT8 uint8_t
#define INT16 int16_t
#define UINT16 uint16_t
#define INT32 int32_t
#define UINT32 uint32_t
#define BOOL bool
#define VOID void
#define CONST
#define PUBLIC
#define SRVAPI
typedef void *HAL_VOC_IRQ_HANDLER_T; // not care
typedef uint32_t CALIB_AUD_PATH_T;   // not care
typedef uint32_t CALIB_AUD_MODE_T;   // not care
#define MUSIC_SUPPORT
#define SPEECH_SUPPORT
#undef CHIP_HAS_AP
#define AUDIO_CALIB_VER 2
#define BT_A2DP_ANA_DAC_MUTE 0
#include "cpheaders/aud_snd_types.h"
#include "cpheaders/hal_error.h"
#include "cpheaders/hal_speech.h"
#include "cpheaders/hal_volte.h"
#include "cpheaders/hal_aif.h"
#include "cpheaders/snd_types.h"
#include "cpheaders/aud_m.h"
#include "cpheaders/vois_m.h"
#include "cpheaders/dm_audio.h"
#include "cpheaders/hal_aud_zsp.h"

// CP APIs
INT32 DM_VoisSetCfg(AUD_ITF_T itf, VOIS_AUDIO_CFG_T *cfg, AUD_DEVICE_CFG_EXT_T devicecfg, SND_BT_WORK_MODE_T btworkmode);
INT32 DM_StartAudioEx(void);
INT32 DM_RestartAudioEx(void);
INT32 DM_StopAudioEx(void);
INT32 DM_PlayToneEx(UINT8 nToneType, UINT8 nToneAttenuation, UINT16 nDuration, DM_SpeakerGain volumn);
INT32 DM_StopToneEx(void);
INT32 DM_ZspMusicPlayStart(void);
INT32 DM_ZspMusicPlayStop(void);
INT32 DM_VoisTestModeSetup(SND_ITF_T itf, HAL_AIF_STREAM_T *stream, AUD_LEVEL_T *cfg, AUD_TEST_T mode, UINT32 voismode);
AUD_ERR_T aud_bbatPcmBufferPlayStop(SND_ITF_T itf);
AUD_ERR_T aud_bbatPcmBufferPlayStart(SND_ITF_T itf, BBAT_PCM_STREAM_T *playbuffer);

#define AUDEV_NV_FNAME "/nvm/audio_device.nv"
#define AUDEV_INPUT_COUNT (5)
#define AUDEV_OUTPUT_COUNT (3)
#define AUDIO_LEVEL_MAX (15)
#define AUDEV_VOL_MAX (100)
#define AUDEV_WQ_PRIO OSI_PRIORITY_ABOVE_NORMAL
#define AUDEV_RECORD_BUF_SIZE (1024)
#define AUDEV_PLAY_HIDDEN_BUF_SIZE (512)
#define AUDEV_PLAY_OUT_BUF_SIZE (1152 * 2)
#define AUDEV_VOICE_DUMP_CHANNELS (6)
#define AUDEV_AUDIO_PA_TYPE (AUD_INPUT_TYPE_CLASSD)

enum
{
    ZSP_MUSIC_MODE_PLAY_PCM = 1,
    ZSP_MUSIC_MODE_PLAY_MP3 = 2,
    ZSP_MUSIC_MODE_PLAY_WMA = 3,
    ZSP_MUSIC_MODE_PLAY_AAC = 4,
    ZSP_MUSIC_MODE_RECORD_WAV = 5,
    ZSP_MUSIC_MODE_RECORD_AAC = 6,
    ZSP_MUSIC_MODE_RECORD_AMR = 7,
    ZSP_MUSIC_MODE_PLAY_FM = 8,
};

enum
{
    ZSP_VOICE_RECORD_FORMAT_PCM = 0,
    ZSP_VOICE_RECORD_FORMAT_AMR = 1,
    ZSP_VOICE_RECORD_FORMAT_DUMP = 3,
};

/**
 * audio device work in three state,
 * 1\voice call user state
 * 2\other single user state,mutex
 * 3\voice call user + other single user state
 */
typedef enum
{
    AUDEV_CLK_USER_VOICE = (1 << 0),
    AUDEV_CLK_USER_TONE = (1 << 1),
    AUDEV_CLK_USER_PLAY = (1 << 2),
    AUDEV_CLK_USER_RECORD = (1 << 3),
    AUDEV_CLK_USER_LOOPBACK = (1 << 4),
    AUDEV_CLK_USER_PLAYTEST = (1 << 5),
} audevClkUser_t;

typedef struct
{
    audevInput_t indev;      // input device
    audevOutput_t outdev;    // output device
    uint8_t voice_vol;       // volume for voice call
    uint8_t play_vol;        // volume for play and tone
    bool out_mute;           // output mute for voice call, play, tone
    uint32_t sample_rate;    // sample freq for mic record and voice call record
    uint8_t sample_interval; // sample time interval for mic record
} audevSetting_t;

typedef struct
{
    volatile AUD_CP_STATUS_E cpstatus;
    osiMutex_t *lock;
    osiWorkQueue_t *wq;
    osiWork_t *ipc_work;
    osiTimer_t *finish_timer;
    osiWork_t *finish_work;
    osiWork_t *tone_end_work;
    osiSemaphore_t *cpstatus_sema;
    AUD_ZSP_SHAREMEM_T *shmem;
    uint32_t clk_users;
    audevSetting_t cfg;
    bool voice_uplink_mute;

    struct
    {
        audevPlayType_t type;
        bool eos_error;
        uint8_t channel_count;
        unsigned sample_rate;
        unsigned total_bytes;
        audevPlayOps_t ops;
        void *ops_ctx;
        HAL_AIF_STREAM_T stream;
        AUD_LEVEL_T level;
    } play;

    struct
    {
        audevRecordType_t type;
        bool enc_error;
        auFrame_t frame;
        audevRecordOps_t ops;
        void *ops_ctx;
        char buf[AUDEV_RECORD_BUF_SIZE];
    } record;

    struct
    {
        audevOutput_t outdev;
    } bbat;

    struct
    {
        audevOutput_t outdev;
        audevInput_t indev;
        uint8_t volume;
    } loopback;
} audevContext_t;

static audevContext_t gAudevCtx;

/**
 * Convert volume [0,100] to AUD_SPK_LEVEL_T
 */
static inline AUD_SPK_LEVEL_T prvVolumeToLevel(unsigned vol, bool mute)
{
    if (mute || vol == 0)
        return AUD_SPK_MUTE;

    unsigned level = (vol * AUDIO_LEVEL_MAX + (AUDEV_VOL_MAX / 2)) / AUDEV_VOL_MAX;
    level = OSI_MIN(unsigned, AUDIO_LEVEL_MAX, level);
    level = OSI_MAX(unsigned, 1, level);
    return (AUD_SPK_LEVEL_T)level;
}

static inline uint8_t prvVolumeToToneAttenuatio(unsigned vol)
{
    int attenuation = 4 - (vol * 4 / AUDEV_VOL_MAX);
    attenuation = OSI_MIN(int, 3, attenuation);
    attenuation = OSI_MAX(int, 0, attenuation);
    return attenuation;
}

/**
 * Convert audevOutput_t to AUD_ITF_T
 */
static inline AUD_ITF_T prvOutputToAudItf(audevOutput_t dev)
{
    return (AUD_ITF_T)dev; // the enum is the same
}

/**
 * Convert audevOutput_t to SND_ITF_T
 */
static inline SND_ITF_T prvOutputToSndItf(audevOutput_t dev)
{
    return (SND_ITF_T)dev; // the enum is the same
}

/**
 * Convert audevInput_t to AUD_INPUT_TYPE_T
 */
static inline AUD_INPUT_TYPE_T prvInputToType(audevInput_t dev)
{
    return (AUD_INPUT_TYPE_T)dev; // the enum is the same
}

/**
 * Convert audevTone_t to tone type
 */
static inline uint8_t prvToneToDmTone(audevTone_t tone)
{
    return tone; // the value is the same
}

/**
 * Whether audevOutput_t is valid
 */
static inline bool prvIsOutputValid(audevOutput_t dev)
{
    return (dev < AUDEV_OUTPUT_COUNT);
}

/**
 * Whether audevInput_t is valid
 */
static inline bool prvIsInputValid(audevInput_t dev)
{
    return (dev < AUDEV_INPUT_COUNT);
}

/**
 * Enable (request) audio clock
 */
static inline void prvEnableAudioClk(audevClkUser_t user)
{
    audevContext_t *d = &gAudevCtx;
    if (d->clk_users == 0)
    {
#ifdef CONFIG_CAMA_CLK_FOR_AUDIO
        halCameraClockRequest(CLK_CAMA_USER_AUDIO, CAMA_CLK_OUT_FREQ_26M);
#else
        halClock26MRequest(CLK_26M_USER_AUDIO);
#endif
    }
    d->clk_users |= user;
}

/**
 * Disable (release) audio clock
 */
static inline void prvDisableAudioClk(audevClkUser_t user)
{
    audevContext_t *d = &gAudevCtx;
    d->clk_users &= ~user;
    if (d->clk_users == 0)
    {
#ifdef CONFIG_CAMA_CLK_FOR_AUDIO
        halCameraClockRelease(CLK_CAMA_USER_AUDIO);
#else
        halClock26MRelease(CLK_26M_USER_AUDIO);
#endif
    }
}

/**
 * Set CP status, called by CP.
 */
void AUD_SetCodecOpStatus(AUD_CP_STATUS_E status)
{
    audevContext_t *d = &gAudevCtx;

    OSI_LOGI(0, "audio cp status %d -> %d", d->cpstatus, status);
    d->cpstatus = status; // mutex is not needed
    osiSemaphoreRelease(d->cpstatus_sema);
}

/**
 * Triggered by CP call for tone end.
 */
static void prvToneEndWork(void *param)
{
    audevContext_t *d = &gAudevCtx;
    osiMutexLock(d->lock);
    prvDisableAudioClk(AUDEV_CLK_USER_TONE);
    osiMutexUnlock(d->lock);
}

/**
 * Called by CP, use work queue for quicker CP response
 */
void AUD_ToneStopEnd(void)
{
    OSI_LOGI(0, "audio cp tone end");

    audevContext_t *d = &gAudevCtx;
    osiWorkEnqueue(d->tone_end_work, d->wq);
}

/**
 * Wait CP status. AUD_SetCodecOpStatus will be called by CP to
 * update status. Return false at timeout.
 */
static bool prvWaitStatus(AUD_CP_STATUS_E flag)
{
    audevContext_t *d = &gAudevCtx;

    osiElapsedTimer_t elapsed;
    osiElapsedTimerStart(&elapsed);

    for (;;)
    {
        if (d->cpstatus == flag)
        {
            d->cpstatus = AUD_NULL;
            return true;
        }

        int timeout = 1500 - osiElapsedTime(&elapsed);
        if (timeout <= 0)
            break;

        osiSemaphoreTryAcquire(d->cpstatus_sema, timeout);
    }

    OSI_LOGW(0, "audio device wait status timeout, expected/%d status/%d",
             flag, d->cpstatus);
    return false;
}

/**
 * Set configuration for voice
 */
static bool prvSetVoiceConfig(void)
{
    audevContext_t *d = &gAudevCtx;

    VOIS_AUDIO_CFG_T cfg = {
        .spkLevel = prvVolumeToLevel(d->cfg.voice_vol, d->cfg.out_mute),
        .micLevel = d->voice_uplink_mute ? AUD_MIC_MUTE : AUD_MIC_ENABLE,
        .sideLevel = AUD_SIDE_MUTE,
        .toneLevel = AUD_TONE_0dB,
        .encMute = 0,
        .decMute = 0,
    };
    AUD_DEVICE_CFG_EXT_T device_ext = {
        .inputType = prvInputToType(d->cfg.indev),
        .inputCircuityType = AUD_INPUT_MIC_CIRCUITY_TYPE_DIFFERENTIAL,
        .inputPath = AUD_INPUT_PATH1,
        .linePath = AUD_LINE_PATH2,
        .spkpaType = AUDEV_AUDIO_PA_TYPE,
    };

    if (DM_VoisSetCfg(prvOutputToAudItf(d->cfg.outdev), &cfg, device_ext,
                      SND_BT_WORK_MODE_NO) != 0)
        return false;

    if (!prvWaitStatus(VOIS_SETUP_DONE))
        return false;

    return true;
}

/**
 * Set configuration for play
 */
static bool prvSetPlayConfig(void)
{
    audevContext_t *d = &gAudevCtx;

    AUD_LEVEL_T level = {
        .spkLevel = prvVolumeToLevel(d->cfg.play_vol, d->cfg.out_mute),
        .micLevel = SND_MIC_ENABLE,
        .sideLevel = SND_SIDE_VOL_15,
        .toneLevel = SND_TONE_0DB,
        .appMode = SND_APP_MODE_MUSIC,
    };

    if (!DM_AudSetup(prvOutputToSndItf(d->cfg.outdev), &level))
        return false;

    if (!prvWaitStatus(AUD_CODEC_SETUP_DONE))
        return false;

    //update para in play
    d->play.level.spkLevel = level.spkLevel;

    return true;
}

/**
 * Set input device configuration
 */
static bool prvSetDeviceExt(void)
{
    audevContext_t *d = &gAudevCtx;

    AUD_DEVICE_CFG_EXT_T device_ext = {
        .inputType = prvInputToType(d->cfg.indev),
        .inputCircuityType = AUD_INPUT_MIC_CIRCUITY_TYPE_DIFFERENTIAL,
        .inputPath = AUD_INPUT_PATH1,
        .linePath = AUD_LINE_PATH2,
        .spkpaType = AUDEV_AUDIO_PA_TYPE,
    };
    aud_SetAudDeviceCFG(device_ext);
    return true;
}

// Operations on AUDIO_INPUT_PARAM_T, which is located in shared memory.
// There are 2 use cases: PCM play and record
//
// PCM play:
// * AP write to shared memory, update writeOffset
// * ZSP read from shared memory, update readOffset -> it is volatile
//
// Record:
// * AP read from shared memory, update readOffset
// * ZSP write to shared memory, update writeOffset -> it is volatile

#define AUDIOIN_IS_EMPTY (writeOffset == readOffset)
#define AUDIOIN_IS_FULL (writeOffset == (readOffset + inLenth - 4) % inLenth)
#define AUDIOIN_SPACE (AUDIOIN_IS_EMPTY ? inLenth - 4 : AUDIOIN_IS_FULL ? 0 : (writeOffset > readOffset) ? inLenth - (writeOffset - readOffset) - 4 : readOffset - writeOffset - 4)
#define AUDIOIN_BYTES (AUDIOIN_IS_FULL ? inLenth - 4 : AUDIOIN_IS_EMPTY ? 0 : (writeOffset > readOffset) ? writeOffset - readOffset : inLenth - (readOffset - writeOffset))

/**
 * Whether audInPara is empty
 */
OSI_UNUSED static inline bool prvAudioInIsEmpty(AUD_ZSP_SHAREMEM_T *p)
{
    uint16_t readOffset = *(volatile uint16_t *)&p->audInPara.readOffset;
    uint16_t writeOffset = *(volatile uint16_t *)&p->audInPara.writeOffset;
    OSI_BARRIER();
    return AUDIOIN_IS_EMPTY;
}

/**
 * Whether audInPara is full
 */
OSI_UNUSED static inline bool prvAudioInIsFull(AUD_ZSP_SHAREMEM_T *p)
{
    uint16_t readOffset = *(volatile uint16_t *)&p->audInPara.readOffset;
    uint16_t writeOffset = *(volatile uint16_t *)&p->audInPara.writeOffset;
    uint16_t inLenth = *(volatile uint16_t *)&p->audInPara.inLenth;
    OSI_BARRIER();
    return AUDIOIN_IS_FULL;
}

/**
 * Available space (for write) in audInPara
 */
OSI_UNUSED static unsigned prvAudioInSpace(AUD_ZSP_SHAREMEM_T *p)
{
    uint16_t readOffset = *(volatile uint16_t *)&p->audInPara.readOffset;
    uint16_t writeOffset = *(volatile uint16_t *)&p->audInPara.writeOffset;
    uint16_t inLenth = *(volatile uint16_t *)&p->audInPara.inLenth;
    OSI_BARRIER();
    return AUDIOIN_SPACE;
}

/**
 * Available bytes (for read) in audInPara
 */
OSI_UNUSED static unsigned prvAudioInBytes(AUD_ZSP_SHAREMEM_T *p)
{
    uint16_t readOffset = *(volatile uint16_t *)&p->audInPara.readOffset;
    uint16_t writeOffset = *(volatile uint16_t *)&p->audInPara.writeOffset;
    uint16_t inLenth = *(volatile uint16_t *)&p->audInPara.inLenth;
    OSI_BARRIER();
    return AUDIOIN_BYTES;
}

/**
 * Put data to audInPara
 */
OSI_UNUSED static unsigned prvAudioInPut(AUD_ZSP_SHAREMEM_T *p, const void *buf, unsigned size)
{
    uint16_t readOffset = *(volatile uint16_t *)&p->audInPara.readOffset;
    uint16_t writeOffset = *(volatile uint16_t *)&p->audInPara.writeOffset;
    uint16_t inLenth = *(volatile uint16_t *)&p->audInPara.inLenth;
    OSI_BARRIER();

    char *bytePtr = (char *)&p->audInput[0];
    unsigned space = AUDIOIN_SPACE;
    if (space == 0)
        return 0;

    if (size > space)
        size = space;

    unsigned tail = inLenth - writeOffset;
    if (tail >= size)
    {
        memcpy(bytePtr + writeOffset, buf, size);
    }
    else
    {
        memcpy(bytePtr + writeOffset, buf, tail);
        memcpy(bytePtr, (const char *)buf + tail, size - tail);
    }

    OSI_BARRIER();
    *(volatile uint16_t *)&p->audInPara.writeOffset = (writeOffset + size) % inLenth;
    return size;
}

/**
 * Get data from audInPara
 */
OSI_UNUSED static unsigned prvAudioInGet(AUD_ZSP_SHAREMEM_T *p, void *buf, unsigned size)
{
    uint16_t readOffset = *(volatile uint16_t *)&p->audInPara.readOffset;
    uint16_t writeOffset = *(volatile uint16_t *)&p->audInPara.writeOffset;
    uint16_t inLenth = *(volatile uint16_t *)&p->audInPara.inLenth;
    OSI_BARRIER();

    char *bytePtr = (char *)&p->audInput[0];
    unsigned bytes = AUDIOIN_BYTES;
    if (bytes == 0)
        return 0;

    if (size > bytes)
        size = bytes;
    unsigned tail = inLenth - readOffset;
    if (tail >= size)
    {
        memcpy(buf, bytePtr + readOffset, size);
    }
    else
    {
        memcpy(buf, bytePtr + readOffset, tail);
        memcpy((char *)buf + tail, bytePtr, size - tail);
    }

    OSI_BARRIER();
    *(volatile uint16_t *)&p->audInPara.readOffset = (readOffset + size) % inLenth;
    return size;
}

/**
 * Get audio frame from player, and put yto audInPara.
 */
static bool prvPlayGetFramesLocked(void)
{
    audevContext_t *d = &gAudevCtx;

    auFrame_t frame = {
        .sample_format = AUSAMPLE_FORMAT_S16,
        .channel_count = d->play.channel_count,
        .sample_rate = d->play.sample_rate,
    };

    while (!d->play.eos_error)
    {
        unsigned space = prvAudioInSpace(d->shmem);
        if (space == 0)
            return true;

        if (!d->play.ops.get_frame(d->play.ops_ctx, &frame))
        {
            d->play.eos_error = true;
        }
        else
        {
            unsigned bytes = prvAudioInPut(d->shmem, (void *)frame.data, frame.bytes);
            d->play.ops.data_consumed(d->play.ops_ctx, bytes);

            d->play.total_bytes += bytes;
            if (frame.flags & AUFRAME_FLAG_END)
            {
                d->play.eos_error = true;
                d->shmem->audInPara.fileEndFlag = 1;
            }

            OSI_LOGI(0, "audio play space/%d frame/%d put/%d total/%d flags/%d", space,
                     frame.bytes, bytes, d->play.total_bytes, frame.flags);
        }

        if (d->play.eos_error)
        {
            unsigned bytes = prvAudioInBytes(d->shmem);
            unsigned ms = auFrameByteToTime(&frame, bytes + AUDEV_PLAY_HIDDEN_BUF_SIZE);
            osiTimerStart(d->finish_timer, ms);
        }
    }

    return true;
}

/**
 * Get from audInPara, and send the data to recorder.
 */
static void prvRecPutFramesLocked(void)
{
    audevContext_t *d = &gAudevCtx;

    for (;;)
    {
        d->record.frame.bytes = prvAudioInGet(d->shmem, (void *)d->record.frame.data,
                                              AUDEV_RECORD_BUF_SIZE);
        if (d->record.frame.bytes == 0)
            break;

        // We shouldn't break even put_frame failed, to avoid overflow
        // just send finsh event to notify recorder app to stop
        if (!d->record.enc_error)
        {
            d->record.enc_error = !d->record.ops.put_frame(d->record.ops_ctx, &d->record.frame);
            if (d->record.enc_error)
            {
                if ((d->clk_users & AUDEV_CLK_USER_RECORD) &&
                    d->record.enc_error &&
                    d->record.ops.handle_event != NULL)
                    d->record.ops.handle_event(d->record.ops_ctx, AUDEV_RECORD_EVENT_FINISH);
            }
        }
    }
}

/**
 * Work triggered by IPC notify, to be executed in audio thread
 */
static void prvIpcWork(void *param)
{
    audevContext_t *d = &gAudevCtx;

    osiMutexLock(d->lock);

    if (d->clk_users & AUDEV_CLK_USER_PLAY)
        prvPlayGetFramesLocked();

    if (d->clk_users & AUDEV_CLK_USER_RECORD)
        prvRecPutFramesLocked();

    osiMutexUnlock(d->lock);
}

/**
 * IPC notify, called in IPC driver
 */
static void prvIpcNotify(void)
{
    audevContext_t *d = &gAudevCtx;
    osiWorkEnqueue(d->ipc_work, d->wq);
}

/**
 * Playback finish timer timeout work
 */
static void prvFinishWork(void *param)
{
    audevContext_t *d = &gAudevCtx;
    osiMutexLock(d->lock);

    if ((d->clk_users & AUDEV_CLK_USER_PLAY) &&
        d->play.eos_error &&
        d->play.ops.handle_event != NULL)
        d->play.ops.handle_event(d->play.ops_ctx, AUDEV_PLAY_EVENT_FINISH);
    osiMutexUnlock(d->lock);
}

/**
 * Playback finish timer timeout
 */
static void prvFinishTimeout(void *param)
{
    audevContext_t *d = &gAudevCtx;
    osiWorkEnqueue(d->finish_work, d->wq);
}

/**
 * Decode nv into context
 */
static bool prvNvDecode(audevSetting_t *p, uint8_t *buffer, unsigned length)
{
    const pb_field_t *fields = pbAudev_fields;
    pbAudev pbdata = {};
    pbAudev *pbs = &pbdata;

    pb_istream_t is = pb_istream_from_buffer(buffer, length);
    if (!pb_decode(&is, fields, pbs))
        return false;

    PB_OPT_DEC_ASSIGN(p->outdev, outdev);
    PB_OPT_DEC_ASSIGN(p->indev, indev);
    PB_OPT_DEC_ASSIGN(p->voice_vol, voice_vol);
    PB_OPT_DEC_ASSIGN(p->play_vol, play_vol);
    PB_OPT_DEC_ASSIGN(p->out_mute, out_mute);
    PB_OPT_DEC_ASSIGN(p->sample_rate, sample_rate);
    PB_OPT_DEC_ASSIGN(p->sample_interval, sample_interval);
    return true;
}

/**
 * Encode nv from context
 */
static int prvNvEncode(const audevSetting_t *p, void *buffer, unsigned length)
{
    const pb_field_t *fields = pbAudev_fields;
    pbAudev pbdata = {};
    pbAudev *pbs = &pbdata;

    PB_OPT_ENC_ASSIGN(p->outdev, outdev);
    PB_OPT_ENC_ASSIGN(p->indev, indev);
    PB_OPT_ENC_ASSIGN(p->voice_vol, voice_vol);
    PB_OPT_ENC_ASSIGN(p->play_vol, play_vol);
    PB_OPT_ENC_ASSIGN(p->out_mute, out_mute);
    PB_OPT_ENC_ASSIGN(p->sample_rate, sample_rate);
    PB_OPT_ENC_ASSIGN(p->sample_interval, sample_interval);
    return pbEncodeToMem(fields, pbs, buffer, length);
}

/**
 * Load nv from file to context
 */
static bool prvLoadNv(audevSetting_t *p)
{
    OSI_LOGI(0, "audio load nv");

    ssize_t file_size = vfs_sfile_size(AUDEV_NV_FNAME);
    if (file_size <= 0)
        return false;

    uint8_t *buffer = (uint8_t *)calloc(1, file_size);
    if (buffer == NULL)
        return false;

    if (vfs_sfile_read(AUDEV_NV_FNAME, buffer, file_size) != file_size)
        goto failed;

    if (!prvNvDecode(p, buffer, file_size))
        goto failed;

    free(buffer);
    return true;

failed:
    free(buffer);
    OSI_LOGE(0, "audio load nv failed");
    return false;
}

/**
 * Store nv to file from context
 */
static bool prvStoreNv(const audevSetting_t *p)
{
    OSI_LOGI(0, "audio store nv");

    int length = prvNvEncode(p, NULL, 0);
    if (length < 0)
        return false;

    void *buffer = calloc(1, length);
    if (buffer == NULL)
        return false;

    length = prvNvEncode(p, buffer, length);
    if (length < 0)
        goto failed;

    if (vfs_sfile_write(AUDEV_NV_FNAME, buffer, length) != length)
        goto failed;

    free(buffer);
    return true;

failed:
    free(buffer);
    OSI_LOGE(0, "audio store nv failed");
    return false;
}

/**
 * Set to default audio device settings
 */
static void prvSetDefaultSetting(audevSetting_t *p)
{
    p->indev = AUDEV_INPUT_MAINMIC;
    p->outdev = AUDEV_OUTPUT_SPEAKER;
    p->voice_vol = 60;
    p->play_vol = 60;
    p->out_mute = false;
    p->sample_rate = 8000;
    p->sample_interval = 20;
}

/**
 * Initialize audio device
 */
void audevInit(void)
{
    audevContext_t *d = &gAudevCtx;

    const halShmemRegion_t *region = halShmemGetRegion(MEM_AUDIO_SM_NAME);
    if (region == NULL || region->address == 0)
        osiPanic();

    OSI_LOGI(0, "audio device init, shared memory 0x%x", region->address);

    d->shmem = (AUD_ZSP_SHAREMEM_T *)region->address;
    d->cpstatus = AUD_NULL;
    d->lock = osiMutexCreate();
    d->wq = osiWorkQueueCreate("audio", 1, AUDEV_WQ_PRIO, CONFIG_AUDIO_WQ_STACK_SIZE);
    d->ipc_work = osiWorkCreate(prvIpcWork, NULL, NULL);
    d->finish_work = osiWorkCreate(prvFinishWork, NULL, NULL);
    d->finish_timer = osiTimerCreate(NULL, prvFinishTimeout, NULL);
    d->tone_end_work = osiWorkCreate(prvToneEndWork, NULL, NULL);
    d->cpstatus_sema = osiSemaphoreCreate(1, 0);

    prvSetDefaultSetting(&d->cfg);
    prvLoadNv(&d->cfg);
    d->voice_uplink_mute = false;
    d->clk_users = 0;
    d->record.frame.data = (uintptr_t)d->record.buf;

    ipc_register_audio_notify(prvIpcNotify);
    prvSetDeviceExt();
}

/**
 * Reset audio device setting, and clear nv
 */
void audevResetSettings(void)
{
    audevContext_t *d = &gAudevCtx;
    osiMutexLock(d->lock);

    prvSetDefaultSetting(&d->cfg);
    vfs_unlink(AUDEV_NV_FNAME);
    osiMutexUnlock(d->lock);
}

/**
 * Return the internal mutex
 */
osiMutex_t *audevMutex(void)
{
    audevContext_t *d = &gAudevCtx;
    return d->lock;
}

/**
 * Set voice call uplink mute/unmute
 */
bool audevSetVoiceUplinkMute(bool mute)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio uplink mute %d", mute);

    osiMutexLock(d->lock);

    d->voice_uplink_mute = mute;

    if (d->clk_users & AUDEV_CLK_USER_VOICE)
        prvSetVoiceConfig();

    osiMutexUnlock(d->lock);
    return true;
}

/**
 * Whether voice call uplink is muted
 */
bool audevIsVoiceUplinkMute(void)
{
    audevContext_t *d = &gAudevCtx;
    return d->voice_uplink_mute;
}

/**
 * Set output interface
 */
bool audevSetOutput(audevOutput_t dev)
{
    OSI_LOGI(0, "audio set output %d", dev);

    if (!prvIsOutputValid(dev))
        return false;

    audevContext_t *d = &gAudevCtx;
    if (d->cfg.outdev == dev)
        return true;

    osiMutexLock(d->lock);

    if (d->clk_users & AUDEV_CLK_USER_VOICE)
    {
        d->cfg.outdev = dev;
        prvSetVoiceConfig();
    }
    else if ((d->clk_users & AUDEV_CLK_USER_PLAY) && !(d->clk_users & AUDEV_CLK_USER_VOICE))
    {
        if (DM_AudStreamStop(prvOutputToSndItf(d->cfg.outdev)) != 0)
        {
            osiMutexUnlock(d->lock);
            return false;
        }
        if (!prvWaitStatus(CODEC_STREAM_STOP_DONE))
        {
            osiMutexUnlock(d->lock);
            return false;
        }
        d->cfg.outdev = dev;
        if (DM_AudStreamStart(prvOutputToSndItf(d->cfg.outdev), &(d->play.stream), &(d->play.level)) != 0)
        {
            d->play.eos_error = true;
            prvFinishTimeout(NULL);
            osiMutexUnlock(d->lock);
            return false;
        }
        if (!prvWaitStatus(CODEC_STREAM_START_DONE))
        {
            d->play.eos_error = true;
            prvFinishTimeout(NULL);
            osiMutexUnlock(d->lock);
            return false;
        }
    }
    else
    {
        d->cfg.outdev = dev;
    }

    audevSetting_t cfg = d->cfg;
    osiMutexUnlock(d->lock);

    prvStoreNv(&cfg);
    return true;
}

/**
 * Get output interface
 */
audevOutput_t audevGetOutput(void)
{
    audevContext_t *d = &gAudevCtx;
    return d->cfg.outdev;
}

/**
 * Set input interface
 */
bool audevSetInput(audevInput_t dev)
{
    OSI_LOGI(0, "audio set input %d", dev);

    if (!prvIsInputValid(dev))
        return false;

    audevContext_t *d = &gAudevCtx;
    osiMutexLock(d->lock);

    d->cfg.indev = dev;
    prvSetDeviceExt();

    audevSetting_t cfg = d->cfg;
    osiMutexUnlock(d->lock);

    prvStoreNv(&cfg);
    return true;
}

/**
 * Get input interface
 */
audevInput_t audevGetInput(void)
{
    audevContext_t *d = &gAudevCtx;
    return d->cfg.indev;
}

/**
 * Set volume for voice call
 */
bool audevSetVoiceVolume(unsigned vol)
{
    if (vol > AUDEV_VOL_MAX)
        return false;

    audevContext_t *d = &gAudevCtx;
    osiMutexLock(d->lock);

    d->cfg.voice_vol = vol;
    if (d->clk_users & AUDEV_CLK_USER_VOICE)
        prvSetVoiceConfig();

    audevSetting_t cfg = d->cfg;
    osiMutexUnlock(d->lock);

    prvStoreNv(&cfg);
    return true;
}

/**
 * Set volume for voice call
 */
bool audevSetPlayVolume(unsigned vol)
{
    if (vol > AUDEV_VOL_MAX)
        return false;

    audevContext_t *d = &gAudevCtx;
    osiMutexLock(d->lock);

    d->cfg.play_vol = vol;
    if (d->clk_users & AUDEV_CLK_USER_PLAY)
        prvSetPlayConfig();

    audevSetting_t cfg = d->cfg;
    osiMutexUnlock(d->lock);

    prvStoreNv(&cfg);
    return true;
}

/**
 * Get volume for voice call
 */
unsigned audevGetVoiceVolume(void)
{
    audevContext_t *d = &gAudevCtx;
    return d->cfg.voice_vol;
}

/**
 * Get volume for play
 */
unsigned audevGetPlayVolume(void)
{
    audevContext_t *d = &gAudevCtx;
    return d->cfg.play_vol;
}

/**
 * Get current output mute/unmute
 */
bool audevSetOutputMute(bool mute)
{
    audevContext_t *d = &gAudevCtx;
    osiMutexLock(d->lock);

    d->cfg.out_mute = mute;
    if (d->clk_users & AUDEV_CLK_USER_VOICE)
        prvSetVoiceConfig();
    else if (d->clk_users & AUDEV_CLK_USER_PLAY)
        prvSetPlayConfig();

    audevSetting_t cfg = d->cfg;
    osiMutexUnlock(d->lock);

    prvStoreNv(&cfg);
    return true;
}

/**
 * Is current output muted
 */
bool audevIsOutputMute(void)
{
    audevContext_t *d = &gAudevCtx;
    return d->cfg.out_mute;
}

/**
 * Get sample freq for mic record and voice call record
 */
uint32_t audevGetRecordSampleRate(void)
{
    audevContext_t *d = &gAudevCtx;
    return d->cfg.sample_rate;
}

/**
 * Set sample rate for mic record and voice call record
 */
bool audevSetRecordSampleRate(uint32_t samplerate)
{
    audevContext_t *d = &gAudevCtx;
    if ((samplerate != 8000) && (samplerate != 16000))
        return false;

    osiMutexLock(d->lock);
    d->cfg.sample_rate = samplerate;
    audevSetting_t cfg = d->cfg;
    osiMutexUnlock(d->lock);

    prvStoreNv(&cfg);
    return true;
}

/**
 * Get sample time interval for mic record
 */
uint8_t audevGetRecordSampleInterval(void)
{
    audevContext_t *d = &gAudevCtx;
    return d->cfg.sample_interval;
}

/**
 * Set sample time interval for mic record
 */
bool audevSetRecordSampleInterval(uint8_t time_ms)
{
    audevContext_t *d = &gAudevCtx;
    // 1. local mic recorder sample time interval config range in 5ms 10ms 20ms Buffer size
    if ((time_ms != 20) && (time_ms != 10) && (time_ms != 5))
        return false;

    osiMutexLock(d->lock);
    d->cfg.sample_interval = time_ms;
    audevSetting_t cfg = d->cfg;
    osiMutexUnlock(d->lock);

    prvStoreNv(&cfg);
    return true;
}

/**
 * Start voice call
 */
bool audevStartVoice(void)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio start voice, user/0x%x", d->clk_users);

    osiMutexLock(d->lock);

    if (d->clk_users != 0) // disable when any user is working
        goto failed;

    prvEnableAudioClk(AUDEV_CLK_USER_VOICE);
    if (!prvSetVoiceConfig())
        goto failed_disable_clk;

    if (DM_StartAudioEx() != 0)
        goto failed_disable_clk;

    if (!prvWaitStatus(CODEC_VOIS_START_DONE))
        goto failed_disable_clk;

    osiMutexUnlock(d->lock);
    return true;

failed_disable_clk:
    prvDisableAudioClk(AUDEV_CLK_USER_VOICE);
failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio start voice failed");
    return false;
}

/**
 * Stop voice call
 */
bool audevStopVoice(void)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio stop voice, user/0x%x", d->clk_users);

    osiMutexLock(d->lock);

    if ((d->clk_users & AUDEV_CLK_USER_VOICE) == 0)
        goto success;

    if (DM_StopAudioEx() != 0)
        goto failed;

    if (!prvWaitStatus(CODEC_VOIS_STOP_DONE))
        goto failed;

    prvDisableAudioClk(AUDEV_CLK_USER_VOICE);

success:
    osiMutexUnlock(d->lock);
    return true;

failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio stop voice failed");
    return false;
}

/**
 * Restart voice call
 */
bool audevRestartVoice(void)
{
    OSI_LOGI(0, "audio restart voice");

    if (DM_RestartAudioEx() != 0)
        return false;

    if (!prvWaitStatus(CODEC_VOIS_START_DONE))
        return false;

    return true;
}

/**
 * Play audio tone
 */
bool audevPlayTone(audevTone_t tone, unsigned duration)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio play tone %d duration/%d user/0x%x", tone, duration, d->clk_users);

    osiMutexLock(d->lock);

    if ((d->clk_users & ~AUDEV_CLK_USER_VOICE) != 0) // disable when any other users is working
        goto failed;

    prvEnableAudioClk(AUDEV_CLK_USER_TONE);
    if (!prvSetVoiceConfig())
        goto failed_disable_clk;

    if (DM_PlayToneEx(prvToneToDmTone(tone), prvVolumeToToneAttenuatio(d->cfg.play_vol),
                      duration, prvVolumeToLevel(d->cfg.play_vol, d->cfg.out_mute)) != 0)
        goto failed_disable_clk;

    osiMutexUnlock(d->lock);
    return true;

failed_disable_clk:
    prvDisableAudioClk(AUDEV_CLK_USER_TONE);
failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio play tone failed");
    return false;
}

/**
 * Stop audio tone
 */
bool audevStopTone(void)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio stop tone, user/0x%x", d->clk_users);

    osiMutexLock(d->lock);

    if ((d->clk_users & AUDEV_CLK_USER_TONE) == 0)
        goto success;

    if (DM_StopToneEx() != 0)
        goto failed;

    prvDisableAudioClk(AUDEV_CLK_USER_TONE);

success:
    osiMutexUnlock(d->lock);
    return true;

failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio stop tone failed");
    return false;
}

/**
 * Fill configurations for loopback test
 */
static void prvLoopbackCfg(VOIS_AUDIO_CFG_T *vois, AUD_DEVICE_CFG_EXT_T *device_ext,
                           AUD_LEVEL_T *level, HAL_AIF_STREAM_T *stream)
{
    audevContext_t *d = &gAudevCtx;

    vois->spkLevel = prvVolumeToLevel(d->loopback.volume, false);
    vois->micLevel = AUD_MIC_ENABLE;
    vois->sideLevel = AUD_SIDE_MUTE;
    vois->toneLevel = AUD_TONE_0dB;
    vois->encMute = 0;
    vois->decMute = 0;

    device_ext->inputType = prvInputToType(d->loopback.indev);
    device_ext->inputCircuityType = AUD_INPUT_MIC_CIRCUITY_TYPE_DIFFERENTIAL;
    device_ext->inputPath = AUD_INPUT_PATH1;
    device_ext->linePath = AUD_LINE_PATH2;
    device_ext->spkpaType = AUDEV_AUDIO_PA_TYPE;

    level->spkLevel = prvVolumeToLevel(d->loopback.volume, false),
    level->micLevel = SND_MIC_ENABLE;
    level->sideLevel = SND_SIDE_MUTE;
    level->toneLevel = SND_TONE_0DB;
    level->appMode = SND_APP_MODE_VOICENB;

    stream->startAddress = NULL;
    stream->length = 0;
    stream->sampleRate = HAL_AIF_FREQ_8000HZ;
    stream->channelNb = HAL_AIF_MONO;
    stream->voiceQuality = 0;
    stream->playSyncWithRecord = 0;
    stream->halfHandler = NULL;
    stream->endHandler = NULL;
}

/**
 * Start loopback test
 */
bool audevStartLoopbackTest(audevOutput_t outdev, audevInput_t indev, unsigned volume)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio start loopback out/%d in/%d user/0x%x", outdev, indev, d->clk_users);

    if (!prvIsOutputValid(outdev) || !prvIsInputValid(indev))
        return false;

    osiMutexLock(d->lock);

    if ((d->clk_users & ~AUDEV_CLK_USER_VOICE) != 0) // disable when any other users is working
        goto failed;

    d->loopback.outdev = outdev;
    d->loopback.indev = indev;
    d->loopback.volume = volume;

    VOIS_AUDIO_CFG_T vois = {};
    AUD_DEVICE_CFG_EXT_T device_ext = {};
    AUD_LEVEL_T level = {};
    HAL_AIF_STREAM_T stream = {};
    prvLoopbackCfg(&vois, &device_ext, &level, &stream);

    prvEnableAudioClk(AUDEV_CLK_USER_LOOPBACK);
    if (DM_VoisSetCfg(prvOutputToAudItf(outdev), &vois, device_ext, SND_BT_WORK_MODE_NO) != 0)
        goto failed_disable_clk;

    if (DM_VoisTestModeSetup(prvOutputToSndItf(outdev), &stream, &level,
                             AUD_TEST_SIDE_TEST, AUD_TEST_VOICE_MODE_PCM) != 0)
        goto failed_disable_clk;

    osiMutexUnlock(d->lock);
    return true;

failed_disable_clk:
    prvDisableAudioClk(AUDEV_CLK_USER_LOOPBACK);
failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio start loopback failed");
    return false;
}

/**
 * Stop loopback test
 */
bool audevStopLoopbackTest(void)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio stop loopback, user/0x%x", d->clk_users);

    osiMutexLock(d->lock);

    if ((d->clk_users & AUDEV_CLK_USER_LOOPBACK) == 0)
        goto success;

    VOIS_AUDIO_CFG_T vois = {};
    AUD_DEVICE_CFG_EXT_T device_ext = {};
    AUD_LEVEL_T level = {};
    HAL_AIF_STREAM_T stream = {};
    prvLoopbackCfg(&vois, &device_ext, &level, &stream);

    if (DM_VoisTestModeSetup(prvOutputToSndItf(d->loopback.outdev), &stream, &level,
                             AUD_TEST_NO, AUD_TEST_VOICE_MODE_PCM) != 0)
        goto failed;

    prvDisableAudioClk(AUDEV_CLK_USER_LOOPBACK);

success:
    osiMutexUnlock(d->lock);
    return true;

failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio stop loopback failed");
    return false;
}

/**
 * Get remained play time (samples in buffer)
 */
int audevPlayRemainTime(void)
{
    audevContext_t *d = &gAudevCtx;
    osiMutexLock(d->lock);

    int ms = 0;
    if ((d->clk_users & AUDEV_CLK_USER_PLAY) && !d->play.eos_error)
    {
        unsigned bytes = prvAudioInBytes(d->shmem) + AUDEV_PLAY_HIDDEN_BUF_SIZE;
        ms = (bytes * 1000) / (d->play.sample_rate * d->play.channel_count * sizeof(int16_t));
    }
    osiMutexUnlock(d->lock);
    return ms;
}

/**
 * Start play
 */
bool audevStartPlayV2(audevPlayType_t type, const audevPlayOps_t *play_ops, void *play_ctx,
                      const auFrame_t *frame)
{
    audevContext_t *d = &gAudevCtx;
    if (play_ops == NULL || play_ops->get_frame == NULL || play_ops->data_consumed == NULL)
        return false;
    if (frame == NULL)
        return false;

    OSI_LOGI(0, "audio start play, type/%d sample/%d channels/%d rate/%d user/0x%x", type,
             frame->sample_format, frame->channel_count,
             frame->sample_rate, d->clk_users);

    if (frame->sample_format != AUSAMPLE_FORMAT_S16)
        return false;
    if (frame->channel_count != 1 && frame->channel_count != 2)
        return false;

    osiMutexLock(d->lock);
    d->play.type = type;
    if (type == AUDEV_PLAY_TYPE_LOCAL)
    {
        if ((d->clk_users & ~AUDEV_CLK_USER_VOICE) != 0) // disable when any other users is working
            goto failed;

        d->play.channel_count = frame->channel_count;
        d->play.sample_rate = frame->sample_rate;
        d->play.total_bytes = 0;
        d->play.eos_error = false;
        d->play.ops = *play_ops;
        d->play.ops_ctx = play_ctx;

        // 1. half buffer size is determined by channel count and sample rate, for 20ms
        // 2. half buffer size should be 32 bytes aligned
        // 3. 2x half buffer can't exceed AUDIO_OUTPUT_BUF_SIZE
        // 4. set a minimal value, based on 8000Hz mono
        // 5. the unit of audOutPara.length is word (2 bytes)
        unsigned half_buffer_bytes = (frame->channel_count * frame->sample_rate * 2 / 50);
        half_buffer_bytes = OSI_ALIGN_UP(half_buffer_bytes, 32);
        half_buffer_bytes = OSI_MIN(unsigned, AUDIO_OUTPUT_BUF_SIZE / 2, half_buffer_bytes);
        half_buffer_bytes = OSI_MAX(unsigned, 320, half_buffer_bytes);
        unsigned half_buffer_words = half_buffer_bytes / 2;

        HAL_AIF_STREAM_T stream = {
            .startAddress = (UINT32 *)d->shmem->audOutput,
            .length = half_buffer_bytes * 2,
            .channelNb = (frame->channel_count == 1) ? HAL_AIF_MONO : HAL_AIF_STEREO,
            .playSyncWithRecord = 0,
            .sampleRate = frame->sample_rate,
            .voiceQuality = false,
            .halfHandler = NULL,
            .endHandler = NULL,
        };
        AUD_LEVEL_T level = {
            .spkLevel = prvVolumeToLevel(d->cfg.play_vol, d->cfg.out_mute),
            .micLevel = SND_MIC_MUTE,
            .sideLevel = SND_SIDE_MUTE,
            .toneLevel = SND_TONE_0DB,
            .appMode = SND_APP_MODE_MUSIC,
        };

        d->shmem->audInPara.sbcOutFlag = 0;
        d->shmem->audInPara.sampleRate = frame->sample_rate;
        d->shmem->audOutPara.samplerate = frame->sample_rate;
        d->shmem->audInPara.channelNb = stream.channelNb;
        d->shmem->audOutPara.channelNb = stream.channelNb;
        d->shmem->audInPara.bitsPerSample = sizeof(int16_t) * 8; // S16, in bits
        d->shmem->musicMode = ZSP_MUSIC_MODE_PLAY_PCM;
        d->shmem->audOutPara.length = half_buffer_words;
        d->shmem->updateParaInd = 0;
        d->shmem->audInPara.inLenth = AUDIO_INPUT_BUF_SIZE;
        d->shmem->audInPara.fileEndFlag = 0;
        d->shmem->audInPara.readOffset = 0;
        d->shmem->audInPara.writeOffset = 0;
        memset(d->shmem->audOutput, 0, AUDIO_OUTPUT_BUF_SIZE);
        osiDCacheCleanAll();

        prvEnableAudioClk(AUDEV_CLK_USER_PLAY);

        if (DM_ZspMusicPlayStart() != 0)
            goto failed_disable_clk;

        if (!prvWaitStatus(ZSP_START_DONE))
            goto failed_disable_clk;

        aud_SetSharkingMode(0);
        if (DM_AudStreamStart(prvOutputToSndItf(d->cfg.outdev), &stream, &level) != 0)
            goto failed_disable_clk;

        if (!prvWaitStatus(CODEC_STREAM_START_DONE))
            goto failed_disable_clk;

        //for reopen aif when audio output path change
        memcpy(&(d->play.stream), &stream, sizeof(HAL_AIF_STREAM_T));
        memcpy(&(d->play.level), &level, sizeof(AUD_LEVEL_T));

        osiWorkEnqueue(d->ipc_work, d->wq);
        osiMutexUnlock(d->lock);
        return true;
    }
    else if (type == AUDEV_PLAY_TYPE_VOICE)
    {
        if ((d->clk_users & AUDEV_CLK_USER_VOICE) == 0) // only available in voice call
            goto failed;

        d->play.channel_count = frame->channel_count;
        d->play.sample_rate = frame->sample_rate;
        d->play.total_bytes = 0;
        d->play.eos_error = false;
        d->play.ops = *play_ops;
        d->play.ops_ctx = play_ctx;

        // 1. half buffer size is determined by channel count and sample rate, for 20ms
        // 2. half buffer size should be 32 bytes aligned
        // 3. 2x half buffer can't exceed AUDIO_OUTPUT_BUF_SIZE
        // 4. set a minimal value, based on 8000Hz mono
        // 5. the unit of audOutPara.length is word (2 bytes)
        unsigned half_buffer_bytes = (frame->channel_count * frame->sample_rate * 2 / 50);
        half_buffer_bytes = OSI_ALIGN_UP(half_buffer_bytes, 32);
        half_buffer_bytes = OSI_MIN(unsigned, AUDIO_OUTPUT_BUF_SIZE / 2, half_buffer_bytes);
        half_buffer_bytes = OSI_MAX(unsigned, 320, half_buffer_bytes);
        unsigned half_buffer_words = half_buffer_bytes / 2;

        d->shmem->audInPara.sbcOutFlag = 0;
        d->shmem->audInPara.sampleRate = frame->sample_rate;
        d->shmem->audOutPara.samplerate = frame->sample_rate;
        d->shmem->audInPara.channelNb = (frame->channel_count == 1) ? HAL_AIF_MONO : HAL_AIF_STEREO;
        d->shmem->audOutPara.channelNb = (frame->channel_count == 1) ? HAL_AIF_MONO : HAL_AIF_STEREO;
        d->shmem->audInPara.bitsPerSample = sizeof(int16_t) * 8; // S16, in bits
        //d->shmem->musicMode = ZSP_MUSIC_MODE_PLAY_PCM;
        d->shmem->audOutPara.length = half_buffer_words;
        d->shmem->updateParaInd = 0;
        d->shmem->audInPara.inLenth = AUDIO_INPUT_BUF_SIZE;
        d->shmem->audInPara.fileEndFlag = 0;
        d->shmem->audInPara.readOffset = 0;
        d->shmem->audInPara.writeOffset = 0;
        memset(d->shmem->audOutput, 0, AUDIO_OUTPUT_BUF_SIZE);
        osiDCacheCleanAll();

        prvEnableAudioClk(AUDEV_CLK_USER_PLAY);

        if (hal_zspVoicePlayStart() != 0)
        {
            goto failed;
        }
        else
        {
            osiWorkEnqueue(d->ipc_work, d->wq);
            osiMutexUnlock(d->lock);
        }
        return true;
    }

failed_disable_clk:
    prvDisableAudioClk(AUDEV_CLK_USER_PLAY);
failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio start play failed");
    return false;
}

/**
 * Start play
 */
bool audevStartPlay(const audevPlayOps_t *play_ops, void *play_ctx,
                    const auFrame_t *frame)
{
    return (audevStartPlayV2(AUDEV_PLAY_TYPE_LOCAL, play_ops, play_ctx, frame));
}

/**
 * Stop play
 */
bool audevStopPlayV2(void)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio play stop, user/0x%x,type=%d", d->clk_users, d->play.type);

    osiMutexLock(d->lock);
    if (d->play.type == AUDEV_PLAY_TYPE_LOCAL)
    {
        if ((d->clk_users & AUDEV_CLK_USER_PLAY) == 0)
            goto success;

        osiTimerStop(d->finish_timer);

        if (DM_AudStreamStop(prvOutputToSndItf(d->cfg.outdev)) != 0)
            goto failed;

        if (!prvWaitStatus(CODEC_STREAM_STOP_DONE))
            goto failed;

        if (DM_ZspMusicPlayStop() != 0)
            goto failed;

        if (!prvWaitStatus(ZSP_STOP_DONE))
            goto failed;

        prvDisableAudioClk(AUDEV_CLK_USER_PLAY);
    }
    else
    {
        if (hal_zspVoicePlayStop() != 0)
            goto failed;

        prvDisableAudioClk(AUDEV_CLK_USER_PLAY);
    }

success:
    osiMutexUnlock(d->lock);
    return true;

failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio stop play failed");
    return false;
}

/**
 * Stop play
 */
bool audevStopPlay(void)
{
    return (audevStopPlayV2());
}

/**
 * Start record
 */
bool audevStartRecord(audevRecordType_t type, const audevRecordOps_t *rec_ops, void *rec_ctx)
{
    audevContext_t *d = &gAudevCtx;
    SND_ITF_T itf = SND_ITF_RECEIVER;

    if (rec_ops == NULL || rec_ops->put_frame == NULL)
        return false;

    OSI_LOGI(0, "audio record start, type/%d user/0x%x", type, d->clk_users);

    osiMutexLock(d->lock);

    if (type == AUDEV_RECORD_TYPE_MIC)
    {
        if ((d->clk_users & AUDEV_CLK_USER_VOICE)) // not support in voice call
            goto failed;
        if ((d->clk_users & ~(AUDEV_CLK_USER_VOICE | AUDEV_CLK_USER_PLAYTEST)) != 0) // disable when any other users is working
            goto failed;

        // 1. local mic recorder sample time interval config range in 5ms - 20ms Buffer size
        // 2. pcm_buffer_bytes >=5ms buffersize <= 20ms buffersize
        unsigned max_buffer_bytes = ((d->cfg.sample_rate == 8000) ? sizeof(HAL_SPEECH_PCM_BUF_T) : sizeof(HAL_VOLTE_PCM_BUF_T));
        unsigned pcm_buffer_bytes = max_buffer_bytes / (20 / d->cfg.sample_interval);
        pcm_buffer_bytes = OSI_MIN(unsigned, max_buffer_bytes, pcm_buffer_bytes);
        pcm_buffer_bytes = OSI_MAX(unsigned, (max_buffer_bytes / 4), pcm_buffer_bytes);

        HAL_AIF_STREAM_T stream = {
            .startAddress = ((d->cfg.sample_rate == 8000) ? (uint32_t *)d->shmem->txPcmBuffer.pcmBuf : (uint32_t *)d->shmem->txPcmVolte.pcmBuf),
            .length = pcm_buffer_bytes,
            .sampleRate = d->cfg.sample_rate,
            .channelNb = HAL_AIF_MONO,
            .voiceQuality = 0,
            .playSyncWithRecord = 0,
            .halfHandler = NULL,
            .endHandler = NULL,
        };
        AUD_LEVEL_T level = {
            .spkLevel = prvVolumeToLevel(d->cfg.play_vol, d->cfg.out_mute),
            .micLevel = SND_MIC_ENABLE,
            .sideLevel = SND_SIDE_VOL_15,
            .toneLevel = SND_SIDE_VOL_15,
            .appMode = SND_APP_MODE_RECORD,
        };

        d->record.type = type;
        d->record.ops = *rec_ops;
        d->record.ops_ctx = rec_ctx;
        d->record.enc_error = false;

        d->record.frame.sample_format = AUSAMPLE_FORMAT_S16;
        d->record.frame.channel_count = 1;
        d->record.frame.sample_rate = d->cfg.sample_rate;

        d->shmem->updateParaInd = 0;
        d->shmem->audInPara.sampleRate = d->cfg.sample_rate;
        d->shmem->audInPara.channelNb = HAL_AIF_MONO;
        d->shmem->audOutPara.channelNb = HAL_AIF_MONO;
        d->shmem->audInPara.bitsPerSample = sizeof(int16_t) * 8; // S16, in bits
        d->shmem->musicMode = ZSP_MUSIC_MODE_RECORD_WAV;
        d->shmem->audInPara.readOffset = 0;
        d->shmem->audInPara.writeOffset = 0;
        d->shmem->audInPara.sbcOutFlag = 0;
        d->shmem->audInPara.fileEndFlag = 0;
        d->shmem->audInPara.inLenth = AUDIO_INPUT_BUF_SIZE;
        d->shmem->audInPara.outLength = pcm_buffer_bytes / 2; //ping/pang length
        d->shmem->audOutPara.samplerate = HAL_AIF_FREQ_8000HZ;
        d->shmem->audOutPara.length = AUDEV_PLAY_OUT_BUF_SIZE;
        memset(d->shmem->audOutput, 0, AUDIO_OUTPUT_BUF_SIZE);

        prvEnableAudioClk(AUDEV_CLK_USER_RECORD);
        prvSetDeviceExt();

        if (DM_ZspMusicPlayStart() != 0)
            goto failed_disable_clk;

        if (!prvWaitStatus(ZSP_START_DONE))
            goto failed_disable_clk;

        OSI_LOGE(0, "audio start record itf = %d", prvOutputToSndItf(d->cfg.outdev));
        if ((d->clk_users & AUDEV_CLK_USER_PLAYTEST) != 0)
        {
            itf = prvOutputToSndItf(d->bbat.outdev);
        }
        else
        {
            itf = prvOutputToSndItf(d->cfg.outdev);
        }

        if (DM_AudStreamRecord(itf, &stream, &level) != 0)
            goto failed_disable_clk;

        if (!prvWaitStatus(CODEC_RECORD_START_DONE))
            goto failed_disable_clk;
    }
    else if (type == AUDEV_RECORD_TYPE_VOICE ||
             type == AUDEV_RECORD_TYPE_VOICE_DUAL ||
             type == AUDEV_RECORD_TYPE_DEBUG_DUMP)
    {
        if ((d->clk_users & AUDEV_CLK_USER_VOICE) == 0) // only available in voice call
            goto failed;
        if ((d->clk_users & ~AUDEV_CLK_USER_VOICE) != 0) // disable when any other users is working
            goto failed;

        unsigned out_channel_count;
        if (type == AUDEV_RECORD_TYPE_VOICE)
            out_channel_count = 1;
        else if (type == AUDEV_RECORD_TYPE_VOICE_DUAL)
            out_channel_count = 2;
        else
            out_channel_count = 6;

        unsigned recformat = (type == AUDEV_RECORD_TYPE_VOICE)
                                 ? ZSP_VOICE_RECORD_FORMAT_PCM
                                 : ZSP_VOICE_RECORD_FORMAT_DUMP;

        d->record.type = type;
        d->record.ops = *rec_ops;
        d->record.ops_ctx = rec_ctx;
        d->record.enc_error = false;

        d->record.frame.sample_format = AUSAMPLE_FORMAT_S16;
        d->record.frame.channel_count = out_channel_count;
        d->record.frame.sample_rate = d->cfg.sample_rate;

        d->shmem->audInPara.sampleRate = d->cfg.sample_rate;
        d->shmem->audInPara.readOffset = 0;
        d->shmem->audInPara.writeOffset = 0;
        d->shmem->audInPara.inLenth = AUDIO_INPUT_BUF_SIZE;
        d->shmem->voiceRecFormat = recformat;

        prvEnableAudioClk(AUDEV_CLK_USER_RECORD);
        if (hal_zspVoiceRecordStart() != 0)
            goto failed_disable_clk;
    }
    else
    {
        goto failed;
    }

    osiMutexUnlock(d->lock);
    return true;

failed_disable_clk:
    prvDisableAudioClk(AUDEV_CLK_USER_RECORD);
failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio start record failed");
    return false;
}

/**
 * Stop record
 */
bool audevStopRecord(void)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio record stop, user/0x%x", d->clk_users);

    osiMutexLock(d->lock);

    if ((d->clk_users & AUDEV_CLK_USER_RECORD) == 0)
        goto success;

    if (d->record.type == AUDEV_RECORD_TYPE_MIC)
    {
        if (DM_AudStreamStop(prvOutputToSndItf(d->cfg.outdev)) != 0)
            goto failed;

        if (!prvWaitStatus(CODEC_STREAM_STOP_DONE))
            goto failed;

        if (DM_ZspMusicPlayStop() != 0)
            goto failed;

        if (!prvWaitStatus(ZSP_STOP_DONE))
            goto failed;

        prvDisableAudioClk(AUDEV_CLK_USER_RECORD);
    }
    else
    {
        if (hal_zspVoiceRecordStop() != 0)
            goto failed;

        prvDisableAudioClk(AUDEV_CLK_USER_RECORD);
    }

success:
    osiMutexUnlock(d->lock);
    return true;

failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio stop record failed");
    return false;
}

bool audevRestartVoiceRecord(void)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio restart voice record, user/0x%x", d->clk_users);
    osiMutexLock(d->lock);
    if ((d->clk_users & AUDEV_CLK_USER_VOICE) == 0) // only available in voice call
        goto failed;
    if ((d->clk_users & AUDEV_CLK_USER_RECORD) == 0) // only available in voice call record
        goto failed;

    unsigned recformat = (d->record.type == AUDEV_RECORD_TYPE_VOICE)
                             ? ZSP_VOICE_RECORD_FORMAT_PCM
                             : ZSP_VOICE_RECORD_FORMAT_DUMP;

    d->shmem->audInPara.sampleRate = d->cfg.sample_rate;
    d->shmem->audInPara.readOffset = 0;
    d->shmem->audInPara.writeOffset = 0;
    d->shmem->audInPara.inLenth = AUDIO_INPUT_BUF_SIZE;
    d->shmem->voiceRecFormat = recformat;

    if (hal_zspVoiceRecordStart() != 0)
        goto failed;

    osiMutexUnlock(d->lock);
    return true;

failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio restart voice record failed");
    return false;
}

/**
 * Start bbat play
 */
bool audevStartPlayTest(audevOutput_t outdev, const osiBufSize32_t *buf)
{
    audevContext_t *d = &gAudevCtx;
    if (buf == NULL || buf->size == 0 || buf->size != 704 || !prvIsOutputValid(outdev))
        return false;

    OSI_LOGI(0, "audio start bbat play, output/%d user/0x%x", outdev, d->clk_users);

    osiMutexLock(d->lock);

    if ((d->clk_users & AUDEV_CLK_USER_PLAYTEST) != 0) // disable when any user is working
        goto failed;

    d->bbat.outdev = outdev;

    prvEnableAudioClk(AUDEV_CLK_USER_PLAYTEST);
    OSI_LOGE(0, "audio play start itf = %d", prvOutputToSndItf(outdev));
    if (aud_bbatPcmBufferPlayStart(prvOutputToSndItf(outdev), (BBAT_PCM_STREAM_T *)buf) != 0)
        goto failed_disable_clock;

    if (!prvWaitStatus(CODEC_STREAM_START_DONE))
        goto failed_disable_clock;

    osiMutexUnlock(d->lock);
    return true;

failed_disable_clock:
    prvDisableAudioClk(AUDEV_CLK_USER_PLAYTEST);
failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio start bbat play failed");
    return false;
}

/**
 * Stop bbat play
 */
bool audevStopPlayTest(void)
{
    audevContext_t *d = &gAudevCtx;
    OSI_LOGI(0, "audio play stop bbat play, user/0x%x", d->clk_users);

    osiMutexLock(d->lock);

    if ((d->clk_users & AUDEV_CLK_USER_PLAYTEST) == 0)
        goto success;

    if (aud_bbatPcmBufferPlayStop(prvOutputToSndItf(d->bbat.outdev)) != 0)
        goto failed;

    if (!prvWaitStatus(CODEC_STREAM_STOP_DONE))
        goto failed;

    prvDisableAudioClk(AUDEV_CLK_USER_PLAYTEST);

success:
    osiMutexUnlock(d->lock);
    return true;

failed:
    osiMutexUnlock(d->lock);
    OSI_LOGE(0, "audio stop bbat play failed");
    return false;
}

void audSetLdoVB(uint32_t en)
{
    audevContext_t *d = &gAudevCtx;
    osiMutexLock(d->lock);
    aud_SetLdoVB(en);
    osiMutexUnlock(d->lock);
}
