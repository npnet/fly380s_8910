/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
* All rights reserved.
*
* This software is supplied "AS IS" without any warranties.
* RDA assumes no responsibility or liability for the use of the software,
* conveys no license or title under any patent, copyright, or mask work
* right to the product. RDA reserves the right to make changes in the
* software without notification.  RDA also make no representation or
* warranty that such application will be suitable for the specified use
* without further testing or modification.
*/
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "cfw.h"
#include "cfw_event.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "tb.h"
#include "tb_internal.h"
#include "osi_log.h"

#include "atr_config.h"

#ifdef CONFIG_ATR_TB_API_SUPPORT

void tbtest_wan_network_info_cb(tb_wan_notify_type_e type, const char *value)
{
    OSI_LOGI(0, "tbtest_wan_network_info_cb, type is %d", type);

    switch (type)
    {
    case TB_WAN_NOTIFY_TYPE_SIG_BAR:
        OSI_LOGXI(OSI_LOGPAR_S, 0, "sig bar is %s", value);
        break;
    case TB_WAN_NOTIFY_TYPE_ROAMING_STATUS:
        OSI_LOGXI(OSI_LOGPAR_S, 0, "roam status is %s", value);
        break;
    case TB_WAN_NOTIFY_TYPE_NET_PROVIDER:
        OSI_LOGXI(OSI_LOGPAR_S, 0, "net provider is %s", value);
        break;
    case TB_WAN_NOTIFY_TYPE_NET_TYPE:
        OSI_LOGXI(OSI_LOGPAR_S, 0, "net type is %s", value);
        break;
    case TB_WAN_NOTIFY_TYPE_CS_PS_STATUS:
        OSI_LOGXI(OSI_LOGPAR_S, 0, "cs ps status is %s", value);
        break;
    case TB_WAN_NOTIFY_TYPE_CELL_ID_INFO:
        OSI_LOGXI(OSI_LOGPAR_S, 0, "cell id is %s", value);
        break;
    default:
        OSI_LOGXI(OSI_LOGPAR_S, 0, "unkwown type");
        break;
    }
}

void tbtest_wan_get_network_type(void)
{
    char NwType[16] = {
        0,
    };
    int size = sizeof(NwType);

    OSI_LOGI(0, "tbtest_wan_get_network_type");

    tb_wan_get_network_type(NwType, size);

    OSI_LOGXI(OSI_LOGPAR_S, 0, "tbtest_wan_get_network_type:%s", (const char *)NwType);
}

void tbtest_wan_get_network_info(void)
{
    tb_wan_network_info_s networkInfo;

    OSI_LOGI(0, "tbtest_wan_get_network_info");

    tb_wan_get_network_info(&networkInfo);

    OSI_LOGXI(OSI_LOGPAR_S, 0, "provider_long_name:%s", (const char *)networkInfo.provider_long_name);
    OSI_LOGXI(OSI_LOGPAR_S, 0, "provider_short_name:%s", (const char *)networkInfo.provider_short_name);
    OSI_LOGXI(OSI_LOGPAR_S, 0, "net_type:%s", (const char *)networkInfo.net_type);
    OSI_LOGI(0, "domain_state:%d", networkInfo.domain_state);
    OSI_LOGI(0, "roam_state:%d", networkInfo.roam_state);
    OSI_LOGXI(OSI_LOGPAR_S, 0, "net_type:%s", (const char *)networkInfo.net_type);
    OSI_LOGI(0, "mcc:%d,%d,%d", networkInfo.mcc[0], networkInfo.mcc[1], networkInfo.mcc[2]);
    OSI_LOGI(0, "mnc:%d,%d,%d", networkInfo.mnc[0], networkInfo.mnc[1], networkInfo.mnc[2]);
    OSI_LOGI(0, "lac_code:%d,%d", networkInfo.lac_code[0], networkInfo.mnc[1]);
    OSI_LOGI(0, "cell_id:%d,%d,%d,%d", networkInfo.cell_id[0], networkInfo.cell_id[1], networkInfo.cell_id[2], networkInfo.cell_id[3]);
    OSI_LOGI(0, "net_band:%d,%d", networkInfo.net_band[0], networkInfo.net_band[1]);

    OSI_LOGI(0, "tbtest_wan_get_network_info, end");
}

void tbtest_wan_get_signal_info(void)
{
    tb_wan_signal_info_s signalInfo = {
        0,
    };

    OSI_LOGI(0, "tbtest_wan_get_signal_info");

    tb_wan_get_signal_info(&signalInfo);

    OSI_LOGI(0, "signal_bar:%d", signalInfo.signal_bar);
    OSI_LOGI(0, "rssi:%d", signalInfo.rssi);
    OSI_LOGI(0, "rsrp:%d", signalInfo.rsrp);
    OSI_LOGI(0, "rsrq:%d", signalInfo.rsrq);
    OSI_LOGI(0, "rscp:%d", signalInfo.rscp);

    OSI_LOGI(0, "tbtest_wan_get_signal_info, end");
}

void tbtest_data_callback_fun(tb_data_profile_id cid, tb_data_dial_status status)
{
    OSI_LOGI(0, "tbtest_data_callback_fun");
    OSI_LOGI(0, "cid %d, status:%d", cid, status);
}

void tbtest_data_wan_disconnect(void)
{
    OSI_LOGI(0, "tbtest_data_wan_disconnect");
    tb_data_wan_disconnect(TB_PROFILE_ID_PUBLIC);
}

void tbtest_data_wan_connect(void)
{
    OSI_LOGI(0, "tbtest_data_wan_connect");
    tb_data_wan_connect(TB_PROFILE_ID_PUBLIC);
}

void tbtest_data_set_connect_parameter(void)
{
    tb_data_profile_id cid = TB_PROFILE_ID_PUBLIC;
    tb_data_connect_profile para = {
        0,
    };

    OSI_LOGI(0, "tbtest_data_set_connect_parameter");

    strcpy(para.apn, "CMNET");
    strcpy(para.config_name, "MYTEST");
    para.pdp_type = TB_PDP_TYPE_IPV4;

    tb_data_set_connect_parameter(cid, para);
}

void tbtest_data_get_connect_parameter(void)
{
    tb_data_profile_id cid = TB_PROFILE_ID_PUBLIC;
    tb_data_connect_profile para = {0};

    OSI_LOGI(0, "tbtest_data_get_connect_parameter");

    tb_data_get_connect_parameter(cid, &para);

    OSI_LOGXI(OSI_LOGPAR_S, 0, "para.apn:%s", (const char *)para.apn);
    OSI_LOGXI(OSI_LOGPAR_S, 0, "para.config_name:%s", (const char *)para.config_name);
    OSI_LOGI(0, "para.pdp_type:%d", para.pdp_type);
}

void tbtest_data_get_dial_status(void)
{
    tb_data_dial_status nStatus;

    OSI_LOGI(0, "tbtest_data_get_dial_status");

    tb_data_get_dial_status(TB_PROFILE_ID_PUBLIC, &nStatus);
    OSI_LOGI(0, "public status is: %d", nStatus);

    tb_data_get_dial_status(TB_PROFILE_ID_PRIVATE, &nStatus);
    OSI_LOGI(0, "private status is: %d", nStatus);
}

void tbtest_data_get_statistics(void)
{
    tb_data_statistics statistics = {
        0,
    };
    OSI_LOGI(0, "tbtest_data_get_statistics");

    tb_data_get_statistics(TB_PROFILE_ID_PUBLIC, &statistics);

    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.realtime_rx_bytes: %lld", statistics.realtime_rx_bytes);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.realtime_tx_bytes:%lld", statistics.realtime_tx_bytes);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.realtime_rx_thrpt:%lld", statistics.realtime_rx_thrpt);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.realtime_tx_thrpt:%lld", statistics.realtime_tx_thrpt);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.realtime_time:%lld", statistics.realtime_time);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.monthly_rx_bytes:%lld", statistics.monthly_rx_bytes);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.monthly_tx_bytes:%lld", statistics.monthly_tx_bytes);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.monthly_time:%lld", statistics.monthly_time);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.total_rx_bytes:%lld", statistics.total_rx_bytes);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.total_tx_bytes:%lld", statistics.total_tx_bytes);
    OSI_LOGXI(OSI_LOGPAR_D, 0, "statistics.total_time:%lld", statistics.total_time);
}

void tbtest_data_get_ipv4_address(void)
{
    tb_data_ipv4_addr address = {
        0,
    };
    OSI_LOGI(0, "tbtest_data_get_ipv4_address");
    if (TB_SUCCESS == tb_data_get_ipv4_address(TB_PROFILE_ID_PUBLIC, &address))
    {
        OSI_LOGXI(OSI_LOGPAR_S, 0, "ip_addr:%s", (const char *)address.ip_addr);
        OSI_LOGXI(OSI_LOGPAR_S, 0, "pref_dns_addr:%s", (const char *)address.pref_dns_addr);
        OSI_LOGXI(OSI_LOGPAR_S, 0, "standy_dns_addr:%s", (const char *)address.standy_dns_addr);
    }
}

void tbtest_sim_callback_fun(tb_sim_status type)
{
    OSI_LOGI(0, "tbtest_sim_callback_fun, type: %d", type);
}

void tbtest_sim_get_sim_state(void)
{
    tb_sim_status simStatus;
    OSI_LOGI(0, "tbtest_sim_get_sim_state");
    if (tb_sim_get_sim_state(&simStatus) == TB_SUCCESS)
    {
        OSI_LOGI(0, "status is: %d", simStatus);
    }
}

void tbtest_sim_get_imsi(void)
{
    char sImsi[16 + 1] = {
        0,
    };
    OSI_LOGI(0, "tbtest_sim_get_imsi");

    if (TB_SUCCESS == tb_sim_get_imsi(sImsi, sizeof(sImsi) - 1))
    {
        OSI_LOGXI(OSI_LOGPAR_S, 0, "imsi:%s", (const char *)sImsi);
    }
}

void tbtest_sim_get_imei(void)
{
    char nImei[16 + 1] = {
        0,
    };
    OSI_LOGI(0, "tbtest_sim_get_imei");
    if (TB_SUCCESS == tb_sim_get_imei(nImei, sizeof(nImei) - 1))
    {
        OSI_LOGXI(OSI_LOGPAR_S, 0, "imei:%s", (const char *)nImei);
    }
}

void tbtest_sim_get_phone_num(void)
{
    char nPhoneNum[41] = {
        0,
    };
    OSI_LOGI(0, "tbtest_sim_get_phone_num");

    if (TB_SUCCESS == tb_sim_get_phone_num(nPhoneNum, sizeof(nPhoneNum) - 1))
    {
        OSI_LOGXI(OSI_LOGPAR_S, 0, "Phone Num:%s", (const char *)nPhoneNum);
    }
}

static int g_Index = 0;
void tbtest_send_message()
{
    OSI_LOGI(0, "tbtest_send_message: send message test!!!");
    tb_sms_msg_type *msg = malloc(800);
    memset(msg, 0x00, 800);
    strncpy((char *)msg->dest_num, "10086", 6);
    memcpy(msg->msg_content, "\xE7\xA5\x81\xE4\xB8\xB9\xE4\xB8\xB9\x00", 10);
    msg->msg_len = 10;
    tb_sms_send_sms(msg);
}

void tbtest_sms_get_sms_withid()
{
    OSI_LOGI(0, "tbtest_sms_get_sms_withid: read message g_Index = %d", g_Index);
    tb_sms_get_sms_withid(g_Index);
}

void tbtest_sms_delete_sms()
{
    OSI_LOGI(0, "tbtest_sms_delete_sms: delete message g_Index = %d", g_Index);
    tb_sms_delete_sms(g_Index);
}

void tbtest_sms_callback_fun(tb_sms_status type, int value)
{
    OSI_LOGI(0, "tbtest_sms_callback_fun, type: %d", type);
    switch (type)
    {
    case TB_SIM_NV_UNREAD_SMS:
        OSI_LOGI(0, "tbtest_sms_callback_fun unread sms is %d", value);
        break;
    case TB_SMS_RECEIVED:
        OSI_LOGI(0, "tbtest_sms_callback_fun received sms is %d", value);
        break;
    case TB_SMS_FULL_FLAG:
        OSI_LOGI(0, "tbtest_sms_callback_fun full flag is %d", value);
        break;
    case TB_SMS_NV_TOTAL_SMS:
        OSI_LOGI(0, "tbtest_sms_callback_fun total sms is %d", value);
        break;
    case TB_SMS_RECEIVED_WITH_ID:
        g_Index = value;
        OSI_LOGI(0, "tbtest_sms_callback_fun received id is %d, %d", value, g_Index);
        break;
    case TB_SMS_DELETE_RESULT:
        OSI_LOGI(0, "tbtest_sms_callback_fun delete result %d", value);
        break;
    case TB_SMS_SEND_RESULT:
        OSI_LOGI(0, "tbtest_sms_callback_fun send sms result %d", value);
        break;
    default:
        OSI_LOGI(0, "tbtest_sms_callback_fun error unkwown type!!!");
        break;
    }
}

void tbtest_sms_get_contents_fun(tb_sms_message *received_message)
{
    int i = 0;
    OSI_LOGI(0, "tbtest_sms_get_contents_fun");
    OSI_LOGI(0, "tbtest_sms_get_contents_fun total = %d", received_message->total);
    OSI_LOGI(0, "tbtest_sms_get_contents_fun current_Index = %d", received_message->current_index);
    OSI_LOGI(0, "tbtest_sms_get_contents_fun index = %d", received_message->index);
    for (i = 0; i < received_message->content_len; i++)
    {
        OSI_LOGI(0, "tbtest_sms_get_contents_fun content[%d] = 0x%x", i, received_message->content[i]);
    }
    OSI_LOGXI(OSI_LOGPAR_S, 0, "tbtest_sms_get_contents_fun Num:%s", (const char *)received_message->phone);
    OSI_LOGI(0, "tbtest_sms_get_contents_fun timeStamp %u/%02u/%02u,%02u:%02u:%02u%+03d",
             received_message->date.year,
             received_message->date.month,
             received_message->date.day,
             received_message->date.hour,
             received_message->date.minute,
             received_message->date.second,
             received_message->date.izone);
}

#endif
