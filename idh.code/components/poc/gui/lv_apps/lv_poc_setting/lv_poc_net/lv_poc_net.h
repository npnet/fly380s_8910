#ifndef __LV_POC_NET_H_
#define __LV_POC_NET_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_POC_GUI_CHOICE_NET_TYPE_SUPPORT
extern lv_poc_activity_t * poc_net_switch_activity;

void lv_poc_net_switch_open(void);
#endif

#ifdef __cplusplus
}
#endif

#endif //__LV_POC_NET_H_
