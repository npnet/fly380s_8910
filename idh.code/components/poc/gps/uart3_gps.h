#ifndef _UART3_GPS_H_
#define _UART3_GPS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_POC_GUI_GPS_SUPPORT
bool lvPocGpsIdtCom_Msg(LVPOCIDTCOM_Gps_SignalType_t signal, void *ctx);

void publvPocGpsIdtComInit(void);

bool pubPocIdtGpsLocationStatus(void);

bool pubPocIdtGpsTimeoutNoLocationStatus(void);

bool pubPocIdtIsHaveExistGps(void);

void publvPocGpsIdtComSleep(void);

void publvPocGpsIdtComWake(void);

void *publvPocGpsIdtComDataInfo(void);

int publvPocGpsIdtComGetLocationCountDown(void);

int publvPocGpsIdtComGetThisLocationTime(void);

int publvPocGpsIdtComGetPoweronLocationNumber(void);

#endif

#ifdef __cplusplus
	}
#endif

#endif

