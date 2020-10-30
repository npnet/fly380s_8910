/*****************************************************************************
* Copyright (c) 2020 ABUP.Co.Ltd. All rights reserved.
* File name: abup_common.c
* Description: common func
* Author: YK
* Version: v1.0
* Date: 20200403
*****************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "abup_common.h"
#include "abup_define.h"


void abup_BytePrint(const char *content, uint32_t len)
{
	for(int i = 0; i < len; i++)
	{
		putchar(content[i]);
	}
	putchar('\n');
}

//项目imei，需贵司完成
char *abup_get_device_mid(void)
{
    return (char *)ABUP_DEVICE_MID;
}

//项目PRODUCT ID
char *abup_get_product_id(void)
{
    return (char *)ABUP_FOTA_SERVICE_PRODUCT_ID;
}

//项目PRODUCT SEC
char *abup_get_product_sec(void)
{
    return (char *)ABUP_FOTA_SERVICE_PRODUCT_SEC;
}

uint32_t abup_get_life_time(void)
{
    return 2000;
}

//项目OEM
char *abup_get_manufacturer(void)
{
    return (char *)ABUP_FOTA_SERVICE_OEM;
}

//项目model
char *abup_get_model_number(void)
{
    return (char *)ABUP_FOTA_SERVICE_MODEL;
}

//项目平台
char *abup_get_platform(void)
{
    return (char *)ABUP_FOTA_SERVICE_PLATFORM;
}

//项目设备类型
char *abup_get_device_type(void)
{
    return (char *)ABUP_FOTA_SERVICE_DEVICE_TYPE;
}

//算法版本
char *abup_get_sdk_version(void)
{
    return (char *)ABUP_FOTA_SDK_VER;
}

//预留字段，未使用
char *abup_get_app_version(void)
{
    return (char *)ABUP_FOTA_APP_VER;
}

//设备版本号
char *abup_get_firmware_version(void)
{
    return (char *)ABUP_FIRMWARE_VERSION;
}

//网络类型,上报给服务器的
char *abup_get_network_type(void)
{
    return (char *)ABUP_FOTA_NETWORK_TYPE;
}

//系统时间
uint32_t abup_get_utc_time(void)
{
    return 1333;
}

