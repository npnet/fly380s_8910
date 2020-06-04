#ifndef __LV_POC_DISPLAY_H_
#define __LV_POC_DISPLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

extern lv_poc_activity_t * poc_theme_switch_activity;
extern lv_poc_activity_t * bright_time_activity;

void lv_poc_bright_time_open(void);


#ifdef CONFIG_POC_GUI_CHOICE_THEME_SUPPORT
void lv_poc_theme_switch_open(void);
#endif

void lv_poc_display_open(void);

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_DISPLAY_H_
