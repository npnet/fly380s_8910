﻿/*****************************************************************************
* Copyright (c) 2019 ABUP.Co.Ltd. All rights reserved.
* File name: abup_fota.h
* Description:
* Author: WQH
* Version: v1.0
* Date: 20190903
*****************************************************************************/
#ifndef __ABUP_FOTA_H__
#define __ABUP_FOTA_H__
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
	ABUP_FOTA_IDLE,
	ABUP_FOTA_RG,	//注册设备信息
	ABUP_FOTA_CV,	//检测版本
	ABUP_FOTA_DL,	//下载差分包
	ABUP_FOTA_RD,	//下载结果上报
	ABUP_FOTA_RU	//升级结果上报
}ABUP_FOTA_STATUS;

typedef enum
{
	ABUP_FOTA_START,
	ABUP_FOTA_IDLEI, //FOTA BUSY
	ABUP_FOTA_CHECK,//检查版本
	ABUP_FOTA_READY,//准备环境
	ABUP_FOTA_RMI,	//注册设备信息
	ABUP_FOTA_CVI,	//检测版本
	ABUP_FOTA_DLI,	//下载差分包
	ABUP_FOTA_NO_NETWORK, //无网络
	ABUP_FOTA_NO_NEW_VERSION, //无新版本
	ABUP_FOTA_NOT_ENOUGH_SPACE, //空间不足
	ABUP_FOTA_NO_ACCESS_TIMES,  //当天访问次数上限
	ABUP_FOTA_REBOOT_UPDATE,	//准备重启更新
	ABUP_FOTA_ERROR,	//升级异常
	ABUP_FOTA_END,
}ABUP_FOTA_PROCESS_STATUS;

typedef enum
{
    ABUP_ERROR_TYPE_NONE,
    ABUP_ERROR_TYPE_UART_TIMEOUT,// 1 //串口超时
    ABUP_ERROR_TYPE_NO_NETWORK,		//无网络
    ABUP_ERROR_TYPE_DNS_FAIL,// 3	//DNS解析失败
    ABUP_ERROR_TYPE_CREATE_SOCKET_FAIL,
    ABUP_ERROR_TYPE_NETWORK_ERROR,// 5
    ABUP_ERROR_TYPE_NO_NEW_UPDATE,		//无最新版本
    ABUP_ERROR_TYPE_MD5_NOT_MATCH,// 7	//MD5校验失败
    ABUP_ERROR_TYPE_NOT_ENOUGH_SPACE,	//空间不足
    ABUP_ERROR_TYPE_ERASE_FLASH,// 9
    ABUP_ERROR_TYPE_WRITE_FLASH,
    ABUP_ERROR_TYPE_NO_ACCESS_TIMES,// 11	//当天此IMEI达到最大访问次数 ,100次
    ABUP_ERROR_TYPE_UNKNOW,
}ABUP_ERROR_TYPE_ENUM;

void abup_create_fota_task(void);
void abup_task_exit(void);
void abup_free_dns(void);
void abup_close_socket(void);
int abup_socket_read_ex(char *buf, int len);
void abup_set_fota_ratio(uint16_t ratio);
uint32_t abup_get_fota_ratio(void);
void abup_exit_restore_context(void);
uint32_t abup_download_len(void);
uint32_t abup_socket_buff_len(void);
void abup_start_download_packet(void);
void abup_device_reboot(void);
char *abup_get_host_domain(void);
char *abup_get_host_port(void);
void abup_recv_buf_memory(void);
uint32_t abup_recv_buff_len(void);
void abup_check_update_result(void);
void abup_check_version(void);
uint8_t abup_update_status(void);
void abup_set_status(uint8_t status);
bool abup_system_run_status(void);

#endif

