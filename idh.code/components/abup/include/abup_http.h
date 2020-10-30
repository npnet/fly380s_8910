/*****************************************************************************
* Copyright (c) 2019 ABUP.Co.Ltd. All rights reserved.
* File name: adups_http.h
* Description:
* Author: WQH
* Version: v1.0
* Date: 20190303
*****************************************************************************/
#ifndef __ABUP_HTTP_H__
#define __ABUP_HTTP_H__
#include <stdint.h>
#include <stdbool.h>
#include "abup_fota.h"

typedef enum
{
	FOTA_SUCCESS=1000,
	FOTA_PID_ERROR,
	FOTA_PROJECT_ERROR,
	FOTA_PARAM_INVAILD,
	FOTA_PARAM_LOST,
	FOTA_SYS_ERROR,
	FOTA_JSON_ERROR,
    FOTA_MAXINUM_VISITS = 1010,
	FOTA_RG_SIGN_ERROR=2001,
	FOTA_CV_LAST_VERSION=2101,
	FOTA_CV_INVAILD_VERSION,
	FOTA_CV_UNREG_DEVICE,
	FOTA_DL_STATE_ERROR=2201,
	FOTA_DL_DELTAID_ERROR,
	FOTA_RP_DELTAID_ERROR=2301,
	FOTA_RP_UPGRADE_STATE_ERROR	
}FOTACODE;

void abup_set_fota_status(ABUP_FOTA_STATUS status);
void abup_get_http_data(char *out_buf, int len);
void abup_parse_http_data(char *p_data, uint32_t len);
ABUP_FOTA_STATUS abup_get_fota_status(void);
bool abup_get_prev_version(char *prev_version, uint32_t ver_len);
void abup_fota_exit(void);
void abup_delta_get_md5(char *file_md5);
bool abup_get_pid_psec(void);
void abup_set_continue_read(void);
void abup_reset_continue_read(void);
bool abup_get_sn_key(void);
char * abup_get_fota_server(void);
char * abup_get_fota_port(void);
char *abup_get_fota_dl_server(void);
char *abup_get_fota_dl_port(void);
uint8_t abup_get_socket_type(void);
uint8_t abup_get_http_dl_state(void);
uint32_t abup_get_download_delta_size(void);

#endif

