#include "ats_config.h"
//#ifdef AT_MQTT_SUPPORT
#if 1
#include "stdio.h"
#include "cfw.h"
#include "at_cfg.h"
#include "sockets.h"
#include "unistd.h"
#include "osi_api.h"
#include "osi_log.h"
#include "at_response.h"
#include "lwip_mqtt_api.h"
#include "at_command.h"
#include "at_engine.h"
//#include "base.h"

// MQTT event
#define AT_CMD_MQTT_CONN_RSP 1
#define AT_CMD_MQTT_SUB_RSP 2
#define AT_CMD_MQTT_UNSUB_RSP 3
#define AT_CMD_MQTT_PUB_RSP 4
#define AT_CMD_MQTT_PING_RSP 5
#define AT_CMD_MQTT_DISCONN_RSP 6
#define AT_CMD_MQTT_PUB_COMP 7
#define AT_CMD_MQTT_ERROR_RSP 8

#define AT_MQTT_TIMEOUT 100 // in seconds
#define AT_MQTT_CONNECT 10

mqtt_client_t at_static_client;

static char g_Topic[MQTT_STRING_SIZE];
static int g_msg_count = 0;
static atCmdEngine_t *g_atCmdEngine = NULL;

static int connflag = 0;

typedef struct at_mqtt_timer_param
{
    uint32_t event;
    void *param;
} AT_MQTT_TM_PARAM;
static AT_MQTT_TM_PARAM s_mqtt_tm_param;

extern void mbedtls_debug_set_threshold(int threshold);
extern struct altcp_tls_config *
altcp_tls_create_config_client2(char *altcp_ca_pem, char *altcp_cert_pem, char *altcp_key_pem);
static void at_mqtt_stop_callback_timeout();

static void mqtt_CommandNotResponse(atCommand_t *cmd)
{
    AT_CMD_RETURN(atCmdRespCmeError(cmd->engine, ERR_AT_CME_NETWORK_TIMOUT));
}

static void AT_MQTT_AsyncEventProcess(void *param)
{
    osiEvent_t *pEvent = (osiEvent_t *)param;
    //bool resp = true;
    uint32_t rc = pEvent->param1;
    bool stopTimer = false;
    OSI_LOGI(0, "AT_MQTT_AsyncEventProcess() line[%d]:pEvent->nEventId=%d, rc=%d", __LINE__, pEvent->id, rc);
    switch (pEvent->id)
    {
    case AT_CMD_MQTT_CONN_RSP:
        OSI_LOGI(0, "AT_MQTT_AsyncEventProcess() line[%d]:AT_CMD_MQTT_CONN_RSP", __LINE__);
        stopTimer = true;

        if ((NULL != g_atCmdEngine) && (0 == rc))
        {
            atCmdRespOK(g_atCmdEngine);
        }
        else if ((NULL != g_atCmdEngine) && (0 != rc) && (1 == connflag))
        {
            atCmdRespCmeError(g_atCmdEngine, ERR_AT_CME_EXE_FAIL);
            g_atCmdEngine = NULL;
        }
        else if ((NULL != g_atCmdEngine) && (0 != rc) && (0 == connflag))
        {
            uint8_t rsp[30] = {0};
            stopTimer = false;
            sprintf((char *)rsp, "+MQTTDISCONNECTED:%ld", rc);
            atCmdRespUrcText(g_atCmdEngine, (char *)rsp);
            g_atCmdEngine = NULL;
        }

        connflag = 0;
        break;
    case AT_CMD_MQTT_SUB_RSP:
        OSI_LOGI(0, "AT_MQTT_AsyncEventProcess() line[%d]:AT_CMD_MQTT_SUB_UNSUB_RSP", __LINE__);
        ((rc == 0) ? (atCmdRespOK(g_atCmdEngine)) : (atCmdRespCmeError(g_atCmdEngine, ERR_AT_CME_EXE_FAIL)));
        break;
    case AT_CMD_MQTT_UNSUB_RSP:
        OSI_LOGI(0, "AT_MQTT_AsyncEventProcess() line[%d]:AT_CMD_MQTT_UNSUB_RSP", __LINE__);
        ((rc == 0) ? (atCmdRespOK(g_atCmdEngine)) : (atCmdRespCmeError(g_atCmdEngine, ERR_AT_CME_EXE_FAIL)));
        break;
    case AT_CMD_MQTT_PUB_RSP:
        OSI_LOGI(0, "AT_MQTT_AsyncEventProcess() line[%d]:AT_CMD_MQTT_PUB_RSP", __LINE__);
        ((rc == 0) ? (atCmdRespOK(g_atCmdEngine)) : (atCmdRespCmeError(g_atCmdEngine, ERR_AT_CME_EXE_FAIL)));
        break;
    case AT_CMD_MQTT_DISCONN_RSP:
        OSI_LOGI(0, "AT_MQTT_AsyncEventProcess() line[%d]:AT_CMD_MQTT_DISCONN_RSP", __LINE__);
        if ((NULL != g_atCmdEngine) && (0 == rc))
        {
            atCmdRespOK(g_atCmdEngine);
            g_atCmdEngine = NULL;
        }
        else if ((NULL != g_atCmdEngine) && (0 != rc))
        {
            atCmdRespCmeError(g_atCmdEngine, ERR_AT_CME_EXE_FAIL);
            g_atCmdEngine = NULL;
        }
        break;
    default:
        OSI_LOGI(0, "AT_MQTT_AsyncEventProcess() line[%d]:ERROR", __LINE__);
        break;
    }

    if (stopTimer)
    {
        at_mqtt_stop_callback_timeout();
    }

    if (NULL != pEvent)
    {
        free(pEvent);
    }

    return;
}

void CFW_MQTTPostEvent(uint32_t nEventId, uint32_t response)
{
    osiEvent_t *ev = malloc(sizeof(osiEvent_t));
    ev->id = nEventId;
    ev->param1 = response;

    osiThreadCallback(atEngineGetThreadId(), AT_MQTT_AsyncEventProcess, (void *)ev);
    return;
}

static void at_mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    CFW_MQTTPostEvent(AT_CMD_MQTT_CONN_RSP, status);
    OSI_LOGI(0, "at_mqtt_connection_cb() line[%d]:status=%d.", __LINE__, status);
    return;
}

#if LWIP_ALTCP && LWIP_ALTCP_TLS
static uint8_t *ca_crt = "-----BEGIN CERTIFICATE-----\n\
MIIC8DCCAlmgAwIBAgIJAOD63PlXjJi8MA0GCSqGSIb3DQEBBQUAMIGQMQswCQYD\n\
VQQGEwJHQjEXMBUGA1UECAwOVW5pdGVkIEtpbmdkb20xDjAMBgNVBAcMBURlcmJ5\n\
MRIwEAYDVQQKDAlNb3NxdWl0dG8xCzAJBgNVBAsMAkNBMRYwFAYDVQQDDA1tb3Nx\n\
dWl0dG8ub3JnMR8wHQYJKoZIhvcNAQkBFhByb2dlckBhdGNob28ub3JnMB4XDTEy\n\
MDYyOTIyMTE1OVoXDTIyMDYyNzIyMTE1OVowgZAxCzAJBgNVBAYTAkdCMRcwFQYD\n\
VQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwGA1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1v\n\
c3F1aXR0bzELMAkGA1UECwwCQ0ExFjAUBgNVBAMMDW1vc3F1aXR0by5vcmcxHzAd\n\
BgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hvby5vcmcwgZ8wDQYJKoZIhvcNAQEBBQAD\n\
gY0AMIGJAoGBAMYkLmX7SqOT/jJCZoQ1NWdCrr/pq47m3xxyXcI+FLEmwbE3R9vM\n\
rE6sRbP2S89pfrCt7iuITXPKycpUcIU0mtcT1OqxGBV2lb6RaOT2gC5pxyGaFJ+h\n\
A+GIbdYKO3JprPxSBoRponZJvDGEZuM3N7p3S/lRoi7G5wG5mvUmaE5RAgMBAAGj\n\
UDBOMB0GA1UdDgQWBBTad2QneVztIPQzRRGj6ZHKqJTv5jAfBgNVHSMEGDAWgBTa\n\
d2QneVztIPQzRRGj6ZHKqJTv5jAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUA\n\
A4GBAAqw1rK4NlRUCUBLhEFUQasjP7xfFqlVbE2cRy0Rs4o3KS0JwzQVBwG85xge\n\
REyPOFdGdhBY2P1FNRy0MDr6xr+D2ZOwxs63dG1nnAnWZg7qwoLgpZ4fESPD3PkA\n\
1ZgKJc2zbSQ9fCPxt2W3mdVav66c6fsb7els2W2Iz7gERJSX\n\
-----END CERTIFICATE-----";

static uint8_t *priv_key = "-----BEGIN RSA PRIVATE KEY-----\n\
MIIEowIBAAKCAQEAxXtfrKLZcwh7NvqhmS3ndFaKZ+cdRYFhdYT3cnjx4N+qUUxQ\n\
GYz3tLvL/d2XPsi0gdkvYFxuyhcmSchR/EcBALfBuRCD0op9PzdcwZDPdTV6xFBw\n\
BM+pEoNNodbfGnCajfZsU6PyIw3PUSNKirIVByiHUZG71Kx3bs95V7kqwUmrpYbw\n\
Ckhku62kLjasgF9USTNB5BLzrI4vFMbKC8yb7G7YZE9PQpXFrgyjs47brBmebzEw\n\
BpMiq4ZPQXlaBWEmijEM5c80PwPC+0cdCrRkWGv/Ne2udxdEQmBHyg7azap0S+bI\n\
7cxg1BCcdwEeArLxpcehNYZd0mMhMHxJYFZfDwIDAQABAoIBABTMN3lA9VGuFPvn\n\
TVmKghO4gX8L0gNd6turkzvVDPQ32gIR3WWzeOrp1EP6MFGGZaw/KUQmt+xEn92p\n\
YSkGeU6QC7Dn0lo8B5C+zd/81jxr03AVYJbBH/c5KNXggUKRlVVaHHJ6k6epbqoG\n\
43TT6G8Gj6nBhVyShUmARDE+6QfOAQonUhm7Wvb/Y8+wzPNuzj0LK5rEUtfsYsVZ\n\
z6vAL670Q8ex1jKsVmzMzpFB8hP0YgRY2jpLGugvcliEOceABjFIIU2UzjcKaE0B\n\
/l1V5w+7uDJ9R9sbAkMsbfFWZ2adGZtKHeYVbkE8v0+nMDDU9LFSxtwjO6MsVJ7F\n\
kQmw1QECgYEA57uefgB49TsssKQF0lMNC3QCDaZIWc82cOc/oL8zSUZv7DIz56b4\n\
y58BtmTFM/sFOMgN7wKfWM25hkyBpyy+0HfF2xX3o3NN28eYRshbhfx3AoWiW7mW\n\
OxC0StGhAsfp28FbO4Than0nnxbob+gSda2GvExyhY7/UaIhUiWQSU8CgYEA2imK\n\
0bF6/cTPxrG08g4GobUq8uPP4eK2X2I/1cmGk3DPRVz/hDkUlzT6VXS/40NZKg7i\n\
nuL3YQAPAl/qe71XuJxPWthLccsrD1sP9rOJvUmt7IJlMx/pIsekFWJ+4M/zxdoA\n\
7/1ONFSSxUeGGnt3G9H+3a5SiVG1RQS97kf7nkECgYAuwyyrjpraGdPd3WuTUtEa\n\
E5HqgTiubuM0i2XKoWrPCZjCWCsjPw2LwY2N6e3lnG0kVTKCncvcFr+uezfarWvg\n\
E5top5kucbEvfaqb9giKFDVL2/20dtH7H1bSllTgg8kMYItxilIWtCTPtMa49mLr\n\
PlYnH2Io039wUMBanFvufwKBgGJ/QQq9u87ZTgnib9SdWocl+ZciHdhZeIeGH1YR\n\
n63vz9T8UOzMrKw8Ig31ITGlVZt0jwK3bIMh+KTU01Q8v189S2KzpxRgazeKoKF8\n\
m+KXYmwWz0QO0z59rmptRuWPXftlGuk/j1B3IrDpQvWv6q3n2M8ThnYzKslXg70S\n\
SNtBAoGBALPx/0Z4Qbn2CZNx0bKyrDdvnlgaSj7y6iKYNeut0SS/Tc2PGWOMaC82\n\
ATfgIxh3gUlQ1iFSOFKYA/b0R3C6OxJ6ni2BspRL/+8ZeNglkzyoQSYUZV/Zqa49\n\
t7rTXkFLckXpLYAmpyoTinPvZ8gDgBFzXqFO1t5atyCiK6LxNe5T\n\
-----END RSA PRIVATE KEY-----";

static uint8_t *client_crt = "-----BEGIN CERTIFICATE-----\n\
MIIC4jCCAkugAwIBAgIBADANBgkqhkiG9w0BAQUFADCBkDELMAkGA1UEBhMCR0Ix\n\
FzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTESMBAGA1UE\n\
CgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVpdHRvLm9y\n\
ZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzAeFw0xNzExMjgwMzE4\n\
MTNaFw0xODAyMjYwMzE4MTNaMD0xCzAJBgNVBAYTAkNOMQ4wDAYDVQQIDAVDaGlu\n\
YTEMMAoGA1UECgwDcmRhMRAwDgYDVQQDDAdjaGVuZ2xpMIIBIjANBgkqhkiG9w0B\n\
AQEFAAOCAQ8AMIIBCgKCAQEAxXtfrKLZcwh7NvqhmS3ndFaKZ+cdRYFhdYT3cnjx\n\
4N+qUUxQGYz3tLvL/d2XPsi0gdkvYFxuyhcmSchR/EcBALfBuRCD0op9PzdcwZDP\n\
dTV6xFBwBM+pEoNNodbfGnCajfZsU6PyIw3PUSNKirIVByiHUZG71Kx3bs95V7kq\n\
wUmrpYbwCkhku62kLjasgF9USTNB5BLzrI4vFMbKC8yb7G7YZE9PQpXFrgyjs47b\n\
rBmebzEwBpMiq4ZPQXlaBWEmijEM5c80PwPC+0cdCrRkWGv/Ne2udxdEQmBHyg7a\n\
zap0S+bI7cxg1BCcdwEeArLxpcehNYZd0mMhMHxJYFZfDwIDAQABoxowGDAJBgNV\n\
HRMEAjAAMAsGA1UdDwQEAwIF4DANBgkqhkiG9w0BAQUFAAOBgQB009GFbDbkCirn\n\
144Y4bAoNpXDSdCfxFuhoLyJRWpD7raqWV8CCvTknhZTLWhUvyyaan3zf4OIDVg8\n\
csQRsKfn8l9NLXV+gSIbZVONI3NyDn6aGluGmZaHLsbwyz7kvCJSEWZjQJ3MJQkF\n\
siOUuUHHXlkY9pgH6gEJEy34Y1PKCg==\n\
-----END CERTIFICATE-----";
#endif
#if 0
static void at_mqtt_timeout_handler(void* param) {
    AT_MQTT_TM_PARAM *tm_param = (AT_MQTT_TM_PARAM *)param;
    bool forceTimeout = false;
    COS_LOGI(0x10004566, "at_mqtt_timeout_handler event:%d", tm_param->event);
    if (tm_param->event == AT_MQTT_CONNECT) {
        lwip_mqtt_disconnect(&at_static_client);
        forceTimeout = true;
    } else {
        COS_LOGI(0x10004567, "at_mqtt_timeout_handler UNHANDLE event:%d", tm_param->event);
    }
    if (forceTimeout) {
        COS_LOGI(0x10004568, "at_mqtt_timeout_handler force timeout");
        AT_SetAsyncTimerMux(g_atCmdEngine, 1);
    }
}
#endif
static void at_mqtt_start_callback_timeout(uint32_t timeout, uint32_t event, void *param)
{
    OSI_LOGI(0, "at_mqtt_start_callback_timeout() line[%d]", __LINE__);
    memset(&s_mqtt_tm_param, 0, sizeof(AT_MQTT_TM_PARAM));
    s_mqtt_tm_param.event = event;
    s_mqtt_tm_param.param = param;
    //at_StartCallbackTimer(timeout*1000, at_mqtt_timeout_handler, (void *)&s_mqtt_tm_param);
}

static void at_mqtt_stop_callback_timeout()
{
    OSI_LOGI(0, "at_mqtt_stop_callback_timeout() line[%d]", __LINE__);
    //at_StopCallbackTimer(at_mqtt_timeout_handler, &s_mqtt_tm_param);
}

int at_mqtt_connect(uint8_t *clientid, uint16_t aliveSeconds, const ip_addr_t *server_ip, uint16_t port,
                    uint8_t cleansession, uint8_t *username, uint8_t *password)
{
    struct mqtt_connect_client_info_t ci;
    err_t err;

    ci.client_id = (char *)clientid;
    ci.keep_alive = aliveSeconds;
    ci.clean_session = cleansession;
    ci.client_user = (char *)username;
    ci.client_pass = (char *)password;
    ci.will_topic = NULL;
    mbedtls_debug_set_threshold(3);

#if LWIP_ALTCP && LWIP_ALTCP_TLS
    // Now we don't have TLS requirement,only remove it.
    /* if (1883 != port && 6002 != port)
    {
        //ci.tls_config = altcp_tls_create_config_client(cacrt,strlen(cacrt));
        ci.tls_config = altcp_tls_create_config_client2(ca_crt, client_crt, priv_key);
    }
    else*/
    {
        ci.tls_config = NULL;
        OSI_LOGI(0, "at_mqtt_connect() line[%d]:The port is %d set the tls config to null.", __LINE__, port);
    }
#endif

    err = lwip_mqtt_connect(&at_static_client, server_ip, port, at_mqtt_connection_cb, 0, &ci);

    if (err != ERR_OK)
    {
        OSI_LOGI(0, "at_mqtt_connect() line[%d]:err = %d.", __LINE__, err);
    }
    return err;
}

static void at_mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t total_len)
{
    memset(g_Topic, 0, sizeof(g_Topic));
    if (strlen(topic) < sizeof(g_Topic) - 1)
    {
        strcpy(g_Topic, topic);
    }
    else
    {
        strncpy(g_Topic, topic, sizeof(g_Topic) - 4);
        strncat(g_Topic, "...", 3);
    }

    return;
}

static void at_mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t data_len, u8_t flags)
{
    char *urc_msg = NULL;
    unsigned int tmplen = 0;
    unsigned int urc_len = data_len + strlen(g_Topic) + 48;

    g_msg_count++;
    urc_msg = (char *)malloc(urc_len);
    if (NULL == urc_msg)
    {
        RETURN_CME_ERR(g_atCmdEngine, ERR_AT_CME_NO_MEMORY);
    }
    else
    {
        memset(urc_msg, 0, urc_len);
        tmplen = sprintf(urc_msg, "+MQTTPUBLISH:%d,%s,%d,", g_msg_count, g_Topic, data_len);
        strncpy(urc_msg + tmplen, (char *)data, data_len);
    }

    atCmdRespUrcText(g_atCmdEngine, urc_msg);

    OSI_LOGI(0, "at_mqtt_incoming_data_cb() line[%d]:Response urc:%s", __LINE__, urc_msg);
    free(urc_msg);
    memset(g_Topic, 0, sizeof(g_Topic));

    return;
}

static void at_mqtt_sub_request_cb(void *arg, err_t result)
{
    OSI_LOGI(0, "at_mqtt_sub_request_cb() line[%d]:result=%d", __LINE__, result);
    CFW_MQTTPostEvent(AT_CMD_MQTT_SUB_RSP, 0x0);
}

static void at_mqtt_unsub_request_cb(void *arg, err_t result)
{
    OSI_LOGI(0, "at_mqtt_unsub_request_cb() line[%d]:result=%d", __LINE__, result);
    CFW_MQTTPostEvent(AT_CMD_MQTT_UNSUB_RSP, 0x0);
}

/**
 * sub 1 for subscribe, 0 for unsubscribe
 */
int at_mqtt_sub_unsub(char *topic, int sub, int qos)
{
    OSI_LOGI(0, "at_mqtt_sub_unsub() line[%d]:sub=%d, qos=%d", __LINE__, sub, qos);
    err_t err;
    if (sub)
    {
        g_msg_count = 0;

        /* Setup callback for incoming publish requests */
        lwip_mqtt_set_inpub_callback(&at_static_client, at_mqtt_incoming_publish_cb, at_mqtt_incoming_data_cb, 0);

        err = lwip_mqtt_subscribe(&at_static_client, topic, qos, at_mqtt_sub_request_cb, 0);
    }
    else
    {
        err = lwip_mqtt_unsubscribe(&at_static_client, topic, at_mqtt_unsub_request_cb, 0);
    }

    if (ERR_OK != err)
    {
        OSI_LOGI(0, "at_mqtt_sub_unsub() line[%d]:err=%d", __LINE__, err);
    }

    return err;
}

void at_mqtt_pub_request_cb(void *arg, err_t result)
{
    OSI_LOGI(0, "at_mqtt_pub_request_cb() line[%d]:result=%d", __LINE__, result);
    CFW_MQTTPostEvent(AT_CMD_MQTT_PUB_RSP, 0x0);
    return;
}

int at_mqtt_publish(char *topic, char *pub_payload, uint8_t dup, uint8_t qos, uint8_t remain)
{
    err_t err;
    err = lwip_mqtt_publish(&at_static_client, topic, pub_payload, strlen(pub_payload), dup, qos, remain, at_mqtt_pub_request_cb, 0);
    if (err != ERR_OK)
    {
        OSI_LOGI(0, "at_mqtt_publish() line[%d]:err=%d", __LINE__, err);
    }
    return err;
}

typedef struct _mqtt_dns_req_t
{
    uint8_t *clientid;
    uint8_t cleansession;
    uint16_t aliveSeconds;
    uint16_t port;
    atCommand_t *pParam;
} MQTT_REQ_T;

static void dnsReq_callback(void *param)
{
    osiEvent_t *ev = (osiEvent_t *)param;
    MQTT_REQ_T *callback_param = (MQTT_REQ_T *)ev->param3;
    uint8_t *clientid = callback_param->clientid;
    atCommand_t *atCmd = callback_param->pParam;

    if (ev->id == EV_CFW_DNS_RESOLV_SUC_IND)
    {
        ip_addr_t *server_ip = (ip_addr_t *)ev->param1;
        uint16_t port = callback_param->port;
        uint16_t aliveSeconds = callback_param->aliveSeconds;
        uint8_t cleansession = callback_param->cleansession;
        uint8_t *username = clientid[MQTT_STRING_SIZE] != 0 ? clientid + MQTT_STRING_SIZE : NULL;
        uint8_t *password = clientid[MQTT_STRING_SIZE * 2] != 0 ? clientid + MQTT_STRING_SIZE * 2 : NULL;

        connflag = 1;
        if (at_mqtt_connect(clientid, aliveSeconds, server_ip, port, cleansession, username, password) != 0)
        {
            at_mqtt_stop_callback_timeout();
            atCmdRespCmeError(atCmd->engine, ERR_AT_CME_EXE_FAIL);
            g_atCmdEngine = NULL;
        }
    }
    else
    {
        at_mqtt_stop_callback_timeout();
        atCmdRespCmeError(atCmd->engine, ERR_AT_CME_EXE_FAIL);
        g_atCmdEngine = NULL;
    }

    if (NULL != clientid)
    {
        free(clientid);
    }

    if (NULL != callback_param)
    {
        free(callback_param);
    }
    if (NULL != ev)
    {
        free(ev);
    }

    return;
}

void AT_GPRS_CmdFunc_MQTTCONN(atCommand_t *pParam)
{
    uint8_t uParamCount = pParam->param_count;
    uint8_t index = 0;
    uint32_t err;

    uint8_t *clientid = NULL;
    uint8_t *tmp_clientid;
    MQTT_REQ_T *callback_param = NULL;
    ip_addr_t server_ip;
    uint16_t port;
    uint16_t aliveSeconds;
    uint8_t cleansession;
    uint8_t *username = NULL;
    uint8_t *tmp_username = NULL;
    uint8_t *password = NULL;
    uint8_t *tmp_password = NULL;
    uint8_t *hostname = NULL;
    bool paramRet = true;
    int ret;

    OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Invalide param-count:%d", __LINE__, uParamCount);

    if (NULL != g_atCmdEngine)
    {
        RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
    }
    if (netif_default == NULL)
    {
        OSI_LOGI(0, "MQTT# AT_GPRS_CmdFunc_MQTTCONN netif_default is NULL");
        RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
    }

    g_atCmdEngine = pParam->engine;
    OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:g_atCmdEngine=%p", __LINE__, g_atCmdEngine);
    switch (pParam->type)
    {
    case AT_CMD_SET:
    {
        if (uParamCount != 5 && uParamCount != 7)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Invalide param-count:%d", __LINE__, uParamCount);
            g_atCmdEngine = NULL;
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }

        hostname = (uint8_t *)atParamStr(pParam->params[index++], &paramRet);
        if (!paramRet || strlen((char *)hostname) > MQTT_STRING_SIZE - 1)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Get hostname fail.", __LINE__);
            g_atCmdEngine = NULL;
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }
        port = (uint16_t)atParamUintInRange(pParam->params[index++], 1, 65535, &paramRet);
        if (!paramRet)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Get port fail.", __LINE__);
            g_atCmdEngine = NULL;
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }

        clientid = malloc(MQTT_STRING_SIZE * 3);
        if (clientid == NULL)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Malloc for clientid fail", __LINE__);
            goto error;
        }
        memset(clientid, 0, MQTT_STRING_SIZE * 3);
        tmp_clientid = (uint8_t *)atParamStr(pParam->params[index++], &paramRet);
        if (!paramRet || strlen((char *)tmp_clientid) > MQTT_STRING_SIZE - 1)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Get clientid fail.", __LINE__);
            goto error;
        }
        strcpy((char *)clientid, (char *)tmp_clientid);

        aliveSeconds = (uint16_t)atParamUintInRange(pParam->params[index++], 1, 3600, &paramRet);
        if (!paramRet)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Get keepalive fail.", __LINE__);
            goto error;
        }

        cleansession = atParamUintInRange(pParam->params[index++], 0, 1, &paramRet);
        if (!paramRet)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Get cleanssion fail.", __LINE__);
            goto error;
        }

        if (uParamCount == 7)
        {
            username = clientid + MQTT_STRING_SIZE;
            password = clientid + MQTT_STRING_SIZE * 2;
            tmp_username = (uint8_t *)atParamStr(pParam->params[index++], &paramRet);
            if (!paramRet || strlen((char *)tmp_username) > MQTT_STRING_SIZE - 1)
            {
                OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Get username fail.", __LINE__);
                goto error;
            }
            strcpy((char *)username, (char *)tmp_username);
            tmp_password = (uint8_t *)atParamStr(pParam->params[index++], &paramRet);
            if (!paramRet || strlen((char *)tmp_password) > MQTT_STRING_SIZE - 1)
            {
                OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Get password fail.", __LINE__);
                goto error;
            }
            strcpy((char *)password, (char *)tmp_password);
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:clientid:%s username:%s password:%s", __LINE__, clientid, username, password);
        }
        callback_param = (MQTT_REQ_T *)malloc(sizeof(MQTT_REQ_T));
        if (callback_param == NULL)
        {
            goto error;
        }
        callback_param->clientid = clientid;
        callback_param->cleansession = cleansession;
        callback_param->aliveSeconds = aliveSeconds;
        callback_param->port = port;
        callback_param->pParam = pParam;
        err = CFW_GethostbynameEX((char *)hostname, &server_ip, -1, -1, dnsReq_callback, callback_param);
        if (err == RESOLV_QUERY_INVALID)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:Dns query invalid!", __LINE__);
            goto error;
        }
        else if (err == RESOLV_COMPLETE)
        {
            connflag = 1;
            ret = at_mqtt_connect(clientid, aliveSeconds, &server_ip, port, cleansession, username, password);
            free(clientid);
            free(callback_param);
            if (ret != 0)
            {
                g_atCmdEngine = NULL;
                RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
            }
        }
        at_mqtt_start_callback_timeout(AT_MQTT_TIMEOUT, AT_MQTT_CONNECT, NULL);
        return;
    }
    default:
    {
        OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTCONN() line[%d]:ERROR", __LINE__);
        g_atCmdEngine = NULL;
        RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_NOT_SURPORT);
    }
    }
error:
    if (NULL != clientid)
    {
        free(clientid);
    }
    if (NULL != callback_param)
    {
        free(callback_param);
    }

    g_atCmdEngine = NULL;
    RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
}

void AT_GPRS_CmdFunc_MQTTPUB(atCommand_t *pParam)
{
    uint8_t uParamCount = pParam->param_count;
    uint8_t index = 0;

    char *topic = NULL;
    char *message = NULL;
    uint8_t qos;
    uint8_t dup;
    uint8_t retain;
    bool paramRet = true;

    if (g_atCmdEngine != pParam->engine)
    {
        RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
    }

    switch (pParam->type)
    {
    case AT_CMD_SET:
    {
        if (!mqtt_client_is_connected(&at_static_client))
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTPUB() line[%d]:MQTT not connected", __LINE__);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
        }
        if (uParamCount < 4)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTPUB() line[%d]:Invalid uParamCount:%d", __LINE__, uParamCount);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }

        if (5 == uParamCount)
        {
            topic = (char *)atParamStr(pParam->params[index++], &paramRet);
            if (!paramRet || strlen(topic) > 64)
            {
                OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTPUB() line[%d]:Get topic fail", __LINE__);
                RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
            }
        }
        else
        {
            topic = "/pk/devicename/update";
        }

        message = (char *)atParamStr(pParam->params[index++], &paramRet);
        if (!paramRet || strlen(message) > MQTT_STRING_SIZE - 8)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTPUB() line[%d]:Get message fail", __LINE__);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }

        if (strlen(message) + strlen(topic) > MQTT_STRING_SIZE - 16)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTPUB() line[%d]:Topic too long", __LINE__);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }

        qos = atParamUintInRange(pParam->params[index++], 0, 2, &paramRet);
        dup = atParamUintInRange(pParam->params[index++], 0, 1, &paramRet);
        retain = atParamUintInRange(pParam->params[index++], 0, 1, &paramRet);
        if (!paramRet)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTPUB() line[%d]:Get param qos/dup/retain  fail", __LINE__);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }
        if (at_mqtt_publish(topic, message, qos, dup, retain) == 0)
        {
            //AT_SetAsyncTimerMux(pParam->engine, AT_MQTT_TIMEOUT);
            atCmdSetTimeoutHandler(pParam->engine, 60 * 1000, mqtt_CommandNotResponse);
        }
        else
        {
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
        }
        break;
    }
    default:
    {
        OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTPUB() line[%d]:ERROR", __LINE__);
        RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_NOT_SURPORT);
    }
    }

    return;
}

void AT_GPRS_CmdFunc_MQTT_SUB_UNSUB(atCommand_t *pParam)
{
    uint8_t uParamCount = pParam->param_count;
    uint8_t index = 0;

    char *topic = NULL;
    uint8_t sub = 0;
    uint8_t qos = 0;
    bool paramRet = true;

    if (g_atCmdEngine != pParam->engine)
    {
        RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
    }

    switch (pParam->type)
    {
    case AT_CMD_SET:
    {
        if (!mqtt_client_is_connected(&at_static_client))
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTT_SUB_UNSUB() line[%d]:MQTT not connected", __LINE__);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
        }
        if (2 != uParamCount && 3 != uParamCount)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTT_SUB_UNSUB() line[%d]:Invalid uParamCount:%d", __LINE__, uParamCount);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }

        topic = (char *)atParamStr(pParam->params[index++], &paramRet);
        if (!paramRet || strlen(topic) > 64)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTT_SUB_UNSUB() line[%d]:Get topic fail", __LINE__);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }

        sub = atParamUintInRange(pParam->params[index++], 0, 1, &paramRet);
        if (!paramRet)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTT_SUB_UNSUB() line[%d]:Get param sub fail", __LINE__);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }

        if (sub)
        {
            qos = atParamUintInRange(pParam->params[index++], 0, 2, &paramRet);
            if (!paramRet)
            {
                OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTT_SUB_UNSUB() line[%d]:Get param qos fail", __LINE__);
                RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
            }
        }
        else if (2 != uParamCount)
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTT_SUB_UNSUB() line[%d]:Invalid uParamCount:%d", __LINE__, uParamCount);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_PARAM_INVALID);
        }

        if (at_mqtt_sub_unsub(topic, sub, qos) == 0)
        {
            //AT_SetAsyncTimerMux(pParam->engine, AT_MQTT_TIMEOUT);
            atCmdSetTimeoutHandler(pParam->engine, 60 * 1000, mqtt_CommandNotResponse);
        }
        else
        {
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
        }
        break;
    }
    default:
    {
        OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTT_SUB_UNSUB() line[%d]:ERROR", __LINE__);
        RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_NOT_SURPORT);
    }
    }

    return;
}

void AT_GPRS_CmdFunc_MQTTDISCONN(atCommand_t *pParam)
{
    if (g_atCmdEngine != pParam->engine)
    {
        RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
    }

    switch (pParam->type)
    {
    case AT_CMD_EXE:
        g_msg_count = 0;
        memset(g_Topic, 0, sizeof(g_Topic));

        if (!mqtt_client_is_connected(&at_static_client))
        {
            OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTDISCONN() line[%d]:MQTT not connected", __LINE__);
            RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_FAIL);
        }

        lwip_mqtt_disconnect(&at_static_client);
        CFW_MQTTPostEvent(AT_CMD_MQTT_DISCONN_RSP, 0x0);
        break;
    default:
        OSI_LOGI(0, "AT_GPRS_CmdFunc_MQTTDISCONN() line[%d]:ERROR", __LINE__);
        RETURN_CME_ERR(pParam->engine, ERR_AT_CME_EXE_NOT_SURPORT);
    }

    return;
}

#endif
