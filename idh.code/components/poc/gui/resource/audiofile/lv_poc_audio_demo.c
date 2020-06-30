#include "lv_include/lv_poc_type.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifdef CONFIG_POC_AUDIO_SPEAKER_WOMEN
static const LV_ATTRIBUTE_MEM_ALIGN uint8_t lv_poc_audio_join_group_map[3528] = {
};

#else

static const LV_ATTRIBUTE_MEM_ALIGN uint8_t lv_poc_audio_join_group_map[3528] = {
};
#endif

lv_poc_audio_dsc_t lv_poc_audio_join_group = {
#ifdef CONFIG_POC_AUDIO_SPEAKER_WOMEN
    .data_size = 3528,
#else
    .data_size = 3528,
#endif
    .data = (uint8_t *)lv_poc_audio_join_group_map,
};

