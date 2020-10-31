#ifndef _UART3_GPS_H_
#define _UART3_GPS_H_

#ifdef __cplusplus
extern "C" {
#endif

bool lvPocGpsIdtCom_Msg(LVPOCIDTCOM_Gps_SignalType_t signal, void *ctx);

void publvPocGpsIdtComInit(void);

bool pubPocIdtGpsLocationStatus(void);

bool pubPocIdtIsHaveExistGps(void);

void publvPocGpsIdtComSleep(void);

void publvPocGpsIdtComWake(void);

#ifdef __cplusplus
	}
#endif

#endif

