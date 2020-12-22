#ifndef __LV_POC_LOG_SWITCH_H_
#define __LV_POC_LOG_SWITCH_H_

#ifdef __cplusplus
extern "C" {
#endif

extern lv_poc_activity_t * poc_record_playback_activity;

void lv_poc_record_playback_open(void);

bool lvPocGuiPcmtoFileAudioPlayStart_Msg(void);

#ifdef __cplusplus
}
#endif

#endif

