#ifndef _UART3_GPS_H_
#define _UART3_GPS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define POCIDTGPSTHREADEVENT

void gpsInit(void);

bool lvPocGpsIdtCom_Msg(LVPOCIDTCOM_Gps_SignalType_t signal, void *ctx);

void prvlvPocGpsIdtComOpenGps(void);

void prvlvPocGpsIdtComCloseGps(void);

bool pubPocIdtGpsLocationStatus(void);

#ifdef __cplusplus
	}
#endif

#endif

