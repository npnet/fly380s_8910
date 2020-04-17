/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Christian Renz - Please refer to git log
 *
 *******************************************************************************/

#include <stdlib.h>
#include <string.h>
//#include <ctype.h>
#include "mbedconnection.h"
#include "commandline.h"
#include "internals.h"
#include "mbedtls/debug.h"

#define COAP_PORT "5683"
#define COAPS_PORT "5684"
#define URI_LENGTH 256

#define MAX_PACKET_SIZE 1024

extern int sockaddr_cmp(struct sockaddr *x, struct sockaddr *y);

/********************* Security Obj Helpers **********************/
char *security_get_uri(lwm2m_object_t *obj, int instanceId, char *uriBuffer, int bufferSize)
{
    int size = 1;
    lwm2m_data_t *dataP = lwm2m_data_new(size);
    dataP->id = 0; // security server uri

    obj->readFunc(instanceId, &size, &dataP, obj);
    if (dataP != NULL &&
        dataP->type == LWM2M_TYPE_STRING &&
        dataP->value.asBuffer.length > 0)
    {
        if (bufferSize > dataP->value.asBuffer.length)
        {
            memset(uriBuffer, 0, dataP->value.asBuffer.length + 1);
            strncpy(uriBuffer, (char *)(dataP->value.asBuffer.buffer), dataP->value.asBuffer.length);
            lwm2m_data_free(size, dataP);
            return uriBuffer;
        }
    }
    lwm2m_data_free(size, dataP);
    return NULL;
}

int64_t security_get_mode(lwm2m_object_t *obj, int instanceId)
{
    int64_t mode = 0;
    int size = 1;
    lwm2m_data_t *dataP = lwm2m_data_new(size);
    dataP->id = 2; // security mode

    obj->readFunc(instanceId, &size, &dataP, obj);
    if (0 != lwm2m_data_decode_int(dataP, &mode))
    {
        lwm2m_data_free(size, dataP);
        return mode;
    }

    lwm2m_data_free(size, dataP);
    LOG("Unable to get security mode : use not secure mode");
    return LWM2M_SECURITY_MODE_NONE;
}

char *security_get_public_id(lwm2m_object_t *obj, int instanceId, int *length)
{
    int size = 1;
    lwm2m_data_t *dataP = lwm2m_data_new(size);
    if (dataP == NULL)
        return NULL;
    dataP->id = 3; // public key or id

    obj->readFunc(instanceId, &size, &dataP, obj);
    if (dataP != NULL &&
        dataP->type == LWM2M_TYPE_OPAQUE)
    {
        char *buff;

        buff = (char *)lwm2m_malloc(dataP->value.asBuffer.length);
        if (buff != NULL)
        {
            memcpy(buff, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            *length = dataP->value.asBuffer.length;
        }
        lwm2m_data_free(size, dataP);

        return buff;
    }
    else
    {
        lwm2m_data_free(size, dataP);
        return NULL;
    }
}

char *security_get_secret_key(lwm2m_object_t *obj, int instanceId, int *length)
{
    int size = 1;
    lwm2m_data_t *dataP = lwm2m_data_new(size);
    if (dataP == NULL)
        return NULL;
    dataP->id = 5; // secret key
    LOG("security_get_secret_key");
    obj->readFunc(instanceId, &size, &dataP, obj);
    if (dataP != NULL &&
        dataP->type == LWM2M_TYPE_OPAQUE)
    {
        char *buff;
        buff = (char *)lwm2m_malloc(dataP->value.asBuffer.length);
        if (buff != 0)
        {
            memcpy(buff, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length);
            *length = dataP->value.asBuffer.length;
        }
        lwm2m_data_free(size, dataP);
        return buff;
    }
    else
    {
        return NULL;
    }
}

/********************* Security Obj Helpers Ends **********************/

int send_data(dtls_connection_t *connP,
              uint8_t *buffer,
              size_t length)
{
    int nbSent;
    size_t offset;

#ifdef WITH_LOGS
    char s[INET6_ADDRSTRLEN];
    in_port_t port;

    s[0] = 0;

    if (AF_INET == connP->addr.sin6_family)
    {
        struct sockaddr_in *saddr = (struct sockaddr_in *)&connP->addr;
        inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET6_ADDRSTRLEN);
        port = saddr->sin_port;
    }
    else if (AF_INET6 == connP->addr.sin6_family)
    {
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&connP->addr;
        inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
        port = saddr->sin6_port;
    }

    LOG_ARG("Sending %d bytes to [%s]:%hu\r\n", length, s, ntohs(port));

#endif

    offset = 0;
    while (offset != length)
    {
        nbSent = sendto(connP->sock, buffer + offset, length - offset, connP->lwm2mH->sendflag, (struct sockaddr *)&(connP->addr), connP->addrLen);
        if (nbSent == -1)
            return -1;
        offset += nbSent;
    }
    connP->lastSend = lwm2m_gettime();
    return 0;
}

static int net_would_block(int fd)
{
    /*
     * Never return 'WOULD BLOCK' on a non-blocking socket
     */
    if ((fcntl(fd, F_GETFL, 0) & O_NONBLOCK) != O_NONBLOCK)
        return (0);

    switch (errno)
    {
#if defined EAGAIN
    case EAGAIN:
#endif
#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif
        return (1);
    }
    return (0);
}

/*
 * Write at most 'len' characters
 */
int mbeddtls_lwm2m_send(void *connP, const unsigned char *buf, size_t len)
{
    LOG("mbeddtls_lwm2m_send enter");
    dtls_connection_t *ctx = (dtls_connection_t *)connP;
    int fd = ctx->sock;

    if (fd < 0)
        return (MBEDTLS_ERR_NET_INVALID_CONTEXT);

    int nbSent = -1;
    size_t offset;

//char s[INET6_ADDRSTRLEN];add LWIP_IPV4_ON/LWIP_IPV6_ON
#if defined(LWIP_IPV4_ON) && defined(LWIP_IPV6_ON)
    char s[INET6_ADDRSTRLEN];
#else
#ifdef LWIP_IPV4_ON
    char s[INET_ADDRSTRLEN];
#else
    char s[INET6_ADDRSTRLEN];
#endif
#endif
    in_port_t port = 0;

    s[0] = 0;
#ifdef LWIP_IPV4_ON

    if (AF_INET == ctx->addr.sin_family)
    {
        struct sockaddr_in *saddr = (struct sockaddr_in *)&ctx->addr;
        inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET_ADDRSTRLEN);
        port = saddr->sin_port;
    }
#endif
#ifdef LWIP_IPV6_ON
    if (AF_INET6 == ctx->addr6.sin6_family)
    {
        struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&ctx->addr;
        inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
        port = saddr->sin6_port;
    }
#endif
    LOG_ARG("Sending %d bytes,\r\n", len);
    LOG_ARG("sending port=%u\r\n", port);

    offset = 0;
    while (offset != len)
    {
        nbSent = sendto(fd, buf + offset, len - offset, 0, (struct sockaddr *)&(ctx->addr), ctx->addrLen);
        if (nbSent < 0)
        {

            if (net_would_block(fd) != 0)
                return (MBEDTLS_ERR_SSL_WANT_WRITE);

#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(EFIX64) && \
    !defined(EFI32)
            if (WSAGetLastError() == WSAECONNRESET)
                return (MBEDTLS_ERR_NET_CONN_RESET);
#else
            if (errno == EPIPE || errno == ECONNRESET)
                return (MBEDTLS_ERR_NET_CONN_RESET);

            if (errno == EINTR)
                return (MBEDTLS_ERR_SSL_WANT_WRITE);
#endif
            LOG_ARG("++++Send error errno:%d\r\n", errno);
            return (MBEDTLS_ERR_NET_SEND_FAILED);
        }
        offset += nbSent;
    }
    LOG_ARG("return len = %d", offset);
    return offset;
}

/*
 * Read at most 'len' characters
 */
int mbeddtls_lwm2m_recv(void *ctx, unsigned char *buffer, size_t len)
{

    int fd = ((dtls_connection_t *)ctx)->sock;
    LOG_ARG("mbeddtls_lwm2m_recv enter,fd=%d", fd);
    if (fd < 0)
        return (MBEDTLS_ERR_NET_INVALID_CONTEXT);

    /*
     * This part will set up an interruption until an event happen on SDTIN or the socket until "tv" timed out (set
     * with the precedent function)
     */
    int numBytes;

    struct sockaddr_storage addr;
    socklen_t addrLen;
    addrLen = sizeof(addr);

    /*
             * We retrieve the data received
             */
    numBytes = recvfrom(fd, buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *)&addr, &addrLen);

    if (0 > numBytes)
    {

        if (net_would_block(fd) != 0)
            return (MBEDTLS_ERR_SSL_WANT_READ);

#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(EFIX64) && \
    !defined(EFI32)
        if (WSAGetLastError() == WSAECONNRESET)
            return (MBEDTLS_ERR_NET_CONN_RESET);
#else
        if (errno == EPIPE || errno == ECONNRESET)
            return (MBEDTLS_ERR_NET_CONN_RESET);

        if (errno == EINTR)
            return (MBEDTLS_ERR_SSL_WANT_READ);
#endif

        return (MBEDTLS_ERR_NET_RECV_FAILED);
    }
    else if (0 < numBytes)
    {
        LOG_ARG("mbeddtls_lwm2m_recv ,numBytes=%d", numBytes);
//char s[INET6_ADDRSTRLEN];add LWIP_IPV4_ON/LWIP_IPV6_ON
#if defined(LWIP_IPV4_ON) && defined(LWIP_IPV6_ON)
                char s[INET6_ADDRSTRLEN];
#else
#ifdef LWIP_IPV4_ON
                char s[INET_ADDRSTRLEN];
#else
                char s[INET6_ADDRSTRLEN];
#endif
#endif
                in_port_t port = 0;
#ifdef LWIP_IPV4_ON
                if (AF_INET == addr.ss_family)
                {
                    struct sockaddr_in *saddr = (struct sockaddr_in *)&addr;
                    inet_ntop(saddr->sin_family, &saddr->sin_addr, s, INET_ADDRSTRLEN);
                    port = saddr->sin_port;
                }
#endif
#ifdef LWIP_IPV6_ON
                if (AF_INET6 == addr.ss_family)
                {
                    struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)&addr;
                    inet_ntop(saddr->sin6_family, &saddr->sin6_addr, s, INET6_ADDRSTRLEN);
                    port = saddr->sin6_port;
                }
#endif
        LOG_ARG("mbeddtls_lwm2m_recv:%d bytes received from [%s]:%hu\r\n", numBytes, s, ntohs(port));
    }
    return numBytes;
}

int mbeddtls_lwm2m_recv_timeout(void *ctx, unsigned char *buf, size_t len,
                                uint32_t timeout)
{
    int ret;
    struct timeval tv;
    fd_set read_fds;  
    int fd = ((dtls_connection_t *)ctx)->sock;

    LOG("mbedtls_lwm2m_recv_timeout entering");
    if (fd < 0)
        return (MBEDTLS_ERR_NET_INVALID_CONTEXT);

    
    #if 0
    uint32_t selectms = 500;
    uint32_t trytimes = timeout/selectms;
    uint32_t leftms = timeout%selectms;
    tv.tv_sec = selectms / 1000;
    tv.tv_usec = (tv.tv_sec == 0)? (selectms*1000):(selectms%1000 * 1000);
    for(int i = 0; i< trytimes ; i++){
        if(((dtls_connection_t *)ctx)->ssl->isquit == 1)
            return MBEDTLS_ERR_SSL_QUIT_FORCED;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        ret = select(fd + 1, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv);
        if(ret != 0)
            break;
    }
    if(ret == 0 && leftms > 0){
        if(((dtls_connection_t *)ctx)->ssl->isquit == 1)
            return MBEDTLS_ERR_SSL_QUIT_FORCED;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        tv.tv_sec = leftms/1000;
        tv.tv_usec = (tv.tv_sec == 0)?(leftms*1000):(leftms%1000 * 1000);
        ret = select(fd + 1, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv);
    }
    #else
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    ret = select(fd + 1, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv);
    #endif
    /* Zero fds ready means we timed out */
    if (ret == 0)
        return (MBEDTLS_ERR_SSL_TIMEOUT);

    if (ret < 0)
    {
#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(EFIX64) && \
    !defined(EFI32)
        if (WSAGetLastError() == WSAEINTR)
            return (MBEDTLS_ERR_SSL_WANT_READ);
#else
        if (errno == EINTR)
            return (MBEDTLS_ERR_SSL_WANT_READ);
#endif

        return (MBEDTLS_ERR_NET_RECV_FAILED);
    }

    /* This call will not block */
    return (mbeddtls_lwm2m_recv(ctx, buf, len));
}

static void lwm2m_debug(void *ctx, int level,
                        const char *file, int line,
                        const char *param)
{
    ((void)level);
    LOG_ARG("%s", param);
}
//mbedtls_ssl_context * g_sslContext = NULL;
int get_dtls_context(dtls_connection_t *connList)
{
    int ret;
    const char *pers = "lwm2mclient";
    LOG("Enterring get_dtls_context");

    mbedtls_net_init(&connList->server_fd);
    mbedtls_ssl_init(&connList->ssl);

    mbedtls_ssl_config_init(&connList->conf);

    mbedtls_x509_crt_init(&connList->cacert);

    mbedtls_ctr_drbg_init(&connList->ctr_drbg);

    mbedtls_entropy_init(&connList->entropy);

    if ((ret = mbedtls_ctr_drbg_seed(&connList->ctr_drbg, mbedtls_entropy_func, &connList->entropy,
                                     (const unsigned char *)pers, strlen(pers))) != 0)
    {
        LOG_ARG("mbedtls_ctr_drbg_seed failed...,ret=%d\n", ret);
        return ret;
    }

    connList->server_fd.fd = connList->sock;
    LOG_ARG("mbedtls_use the sock=%d\n", connList->sock);
    if ((ret = mbedtls_ssl_config_defaults(&connList->conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_DATAGRAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        LOG_ARG("mbedtls_ssl_config_defaults failed ret = %d\n", ret);
        return ret;
    }
    ret = mbedtls_net_set_block(&connList->server_fd);
    mbedtls_timing_delay_context *timer = lwm2m_malloc(sizeof(mbedtls_timing_delay_context));
    mbedtls_ssl_set_timer_cb(&connList->ssl, timer, mbedtls_timing_set_delay,
                             mbedtls_timing_get_delay);

    int length = 0;
    int id_len = 0;
    char *psk = security_get_secret_key(connList->securityObj, connList->securityInstId, &length);
    if(psk == NULL)
    {
        LOG("psk == NULL\n");
    }
    
    char *psk_id = security_get_public_id(connList->securityObj, connList->securityInstId, &id_len);
    if(psk_id == NULL)
    {
        LOG("psk_id == NULL\n");
    }

    if ((ret = mbedtls_ssl_conf_psk(&connList->conf, (const unsigned char *)psk, length,
                                    (const unsigned char *)psk_id,
                                    id_len)) != 0)
    {
        lwm2m_free(psk);
        lwm2m_free(psk_id);
        LOG_ARG(" failed! mbedtls_ssl_conf_psk returned %d\n\n", ret);
        return ret;
    }
    LOG_ARG(" mbedtls_ssl_conf_psk returned %d,psk=%s,psk_id=%s\n\n", ret, psk, psk_id);
    lwm2m_free(psk);
    lwm2m_free(psk_id);
    /* OPTIONAL is not optimal for security,
    * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode(&connList->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&connList->conf, &connList->cacert, NULL);
    mbedtls_ssl_conf_rng(&connList->conf, mbedtls_ctr_drbg_random, &connList->ctr_drbg);
    mbedtls_ssl_conf_dbg(&connList->conf, lwm2m_debug, NULL);
    mbedtls_debug_set_threshold(5);

    if ((ret = mbedtls_ssl_setup(&connList->ssl, &connList->conf)) != 0)
    {
        LOG_ARG("mbedtls_ssl_setup failed ret = %d\n", ret);
        return ret;
    }
    //mbedtls_ssl_set_bio(&connList->ssl, connList, mbeddtls_lwm2m_send, mbeddtls_lwm2m_recv, NULL);
    mbedtls_ssl_set_bio(&connList->ssl, connList, mbeddtls_lwm2m_send, mbeddtls_lwm2m_recv, mbeddtls_lwm2m_recv_timeout);
    /*
    * 4. Handshake
    */
    while ((ret = mbedtls_ssl_handshake(&connList->ssl)) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            LOG_ARG(" failed ! mbedtls_ssl_handshake returned %x\n\n", -ret);
            return ret;
        }
    }
    LOG_ARG(" success ! mbedtls_ssl_handshake returned %x\n\n", -ret);
    LOG(" ok\n");
    return 0;
}

/*int get_port(struct sockaddr *x)
{
#ifdef LWIP_IPV4_ON
   if (x->sa_family == AF_INET)
   {
       return ((struct sockaddr_in *)x)->sin_port;
   }
#endif
#ifdef LWIP_IPV6_ON
   if (x->sa_family == AF_INET6) {
       return ((struct sockaddr_in6 *)x)->sin6_port;
   }
#endif
   printf("non IPV4 or IPV6 address\n");
   return  -1;

}*/

int create_socket(const char *portStr, int ai_family)
{
    LOG("Enterring create_socket");
#if 0
    int s = -1;
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_UDP;

    if (0 != getaddrinfo(NULL, portStr, &hints, &res))
    {
        return -1;
    }

    for (p = res; p != NULL && s == -1; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            if (-1 == bind(s, p->ai_addr, p->ai_addrlen))
            {
                close(s);
                s = -1;
            }
        }
    }

    freeaddrinfo(res);
#else
    int s = -1;
    int port_nr = atoi(portStr);
    if ((port_nr <= 0) || (port_nr > 0xffff)) {
      return -1;
    }
    if(netif_default == NULL)
    {
        return -1;
    }
    s = socket(ai_family,SOCK_DGRAM,IPPROTO_UDP);
    if (s >= 0)
    {
        ip4_addr_t *ip_addr = (ip4_addr_t *)netif_ip4_addr(netif_default);
        struct sockaddr_in bindAddr = {0};
        bindAddr.sin_len = sizeof(bindAddr);
        bindAddr.sin_family = AF_INET;
        bindAddr.sin_port = lwip_htons((u16_t)port_nr);
        inet_addr_from_ip4addr(&(bindAddr.sin_addr), ip_addr);
        if(-1 == bind(s,(const struct sockaddr *)&bindAddr,sizeof(bindAddr)))
        {
            close(s);
            s = -1;
        }
    }
#endif
    return s;
}

dtls_connection_t *connection_new_incoming(dtls_connection_t *connList,
                                           int sock,
                                           const struct sockaddr *addr,
                                           size_t addrLen)
{
    dtls_connection_t *connP;

    connP = (dtls_connection_t *)lwm2m_malloc(sizeof(dtls_connection_t));
    if (connP != NULL)
    {
        memset(connP, 0, sizeof(dtls_connection_t));
        connP->sock = sock;
        memcpy(&(connP->addr), addr, addrLen);
        connP->addrLen = addrLen;
        connP->next = connList;
        LOG("new_incoming");

        /*      connP->dtlsSession = (session_t *)lwm2m_malloc(sizeof(session_t));

#ifdef LWIP_IPV6_ON
				connP->dtlsSession->addr.sin6 = connP->addr6;
#else
				connP->dtlsSession->addr.sin = connP->addr;
#endif

        connP->dtlsSession->size = connP->addrLen;
        connP->lastSend = lwm2m_gettime();*/
    }
    else
    {
        LOG("new_incoming,malloc failed!");
    }

    return connP;
}

dtls_connection_t *connection_find(dtls_connection_t *connList,
                                   const struct sockaddr_storage *addr,
                                   size_t addrLen)
{
    dtls_connection_t *connP;

    connP = connList;
    while (connP != NULL)
    {

        if (sockaddr_cmp((struct sockaddr *)(&connP->addr), (struct sockaddr *)addr))
        {
            return connP;
        }

        connP = connP->next;
    }

    return connP;
}

dtls_connection_t *connection_create(dtls_connection_t *connList,
                                     int sock,
                                     lwm2m_object_t *securityObj,
                                     int instanceId,
                                     lwm2m_context_t *lwm2mH,
                                     int addressFamily)
{
    struct addrinfo hints;
    struct addrinfo *servinfo = NULL;
    //struct addrinfo *p;
    //int s;
    //struct sockaddr *sa = NULL;
    //socklen_t sl = 0;
    dtls_connection_t *connP = NULL;
    char uriBuf[URI_LENGTH];
    char *uri;
    char *host;
    char *port;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = addressFamily;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    uri = security_get_uri(securityObj, instanceId, uriBuf, URI_LENGTH);
    if (uri == NULL)
        return NULL;

    // parse uri in the form "coaps://[host]:[port]"
    char *defaultport;
    if (0 == strncmp(uri, "coaps://", strlen("coaps://")))
    {
        host = uri + strlen("coaps://");
        defaultport = COAPS_PORT;
    }
    else if (0 == strncmp(uri, "coap://", strlen("coap://")))
    {
        host = uri + strlen("coap://");
        defaultport = COAP_PORT;
    }
    else
    {
        return NULL;
    }
    port = strrchr(host, ':');
    if (port == NULL)
    {
        port = defaultport;
    }
    else
    {
        // remove brackets
        if (host[0] == '[')
        {
            host++;
            if (*(port - 1) == ']')
            {
                *(port - 1) = 0;
            }
            else
            {
                return NULL;
            }
        }
        // split strings
        *port = 0;
        port++;
    }

    if (0 != getaddrinfo(host, port, &hints, &servinfo) || servinfo == NULL)
        return NULL;

#if 0
    // we test the various addresses
    s = -1;
    for (p = servinfo; p != NULL && s == -1; p = p->ai_next)
    {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s >= 0)
        {
            sa = p->ai_addr;
            sl = p->ai_addrlen;
            if (-1 == connect(s, p->ai_addr, p->ai_addrlen))
            {
                LOG("failed");
                close(s);
                s = -1;
            }
        }
    }
#endif

    if (sock >= 0)
    {
        connP = connection_new_incoming(connList, sock, servinfo->ai_addr, servinfo->ai_addrlen);
        //close(s);
        //connP->sock_id = s;
        // do we need to start tinydtls?
        if (connP != NULL)
        {
            LOG(" connP != NULL");
            connP->securityObj = securityObj;
            connP->securityInstId = instanceId;
            connP->lwm2mH = lwm2mH;

            if (security_get_mode(connP->securityObj, connP->securityInstId) != LWM2M_SECURITY_MODE_NONE)
            {
                connP->issecure = 1;
                int ret = get_dtls_context(connP);
                if (ret != 0)
                {
                    LOG_ARG(" lwm2m get_dtls_context ret=%d", ret);
                    if (NULL != servinfo)
                        freeaddrinfo(servinfo);
                    dtls_connection_t *targetP = connP;
                    connP = targetP->next;
                    targetP->next = NULL;
                    connection_free(targetP);
                    return NULL;
                }
            }
            else
            {
                // no dtls session
                connP->issecure = 0;
            }
        }
    }

    if (NULL != servinfo)
        freeaddrinfo(servinfo);
    return connP;
}

void connection_free(dtls_connection_t *connList)
{
	 LOG("Enter connection_free");
    dtls_connection_t* connList_tmp = connList;
    while (connList_tmp != NULL)
    {
        //connList = connList_tmp;
        dtls_connection_t *nextP = connList_tmp->next;

        if (connList_tmp->issecure != 0)
        {
            mbedtls_net_free(&connList_tmp->server_fd);

            mbedtls_entropy_free(&connList_tmp->entropy);

            mbedtls_ssl_config_free(&connList_tmp->conf);

            mbedtls_x509_crt_free(&connList_tmp->cacert);

            if (connList_tmp->ssl.p_timer){
                lwm2m_free(connList_tmp->ssl.p_timer);
                connList_tmp->ssl.p_timer = NULL;
                }
            mbedtls_ssl_free(&connList_tmp->ssl);

            mbedtls_ctr_drbg_free(&connList_tmp->ctr_drbg);
        }
         sys_arch_printf("M2M# connection_free connList = %p", connList_tmp);
        lwm2m_free(connList_tmp);
        connList_tmp = nextP;
    }
}

int connection_send(dtls_connection_t *connP, uint8_t *buffer, size_t length)
{

    if (connP->issecure == 0)
    {
        if (0 != send_data(connP, buffer, length))
        {
            return -1;
        }
    }
    else
    {
        if (-1 == mbedtls_ssl_write(&connP->ssl, buffer, length))
        {
            return -1;
        }
    }
    LOG("connection_send success");
    return 0;
}

int connection_handle_packet(dtls_connection_t *connP, uint8_t *buffer, size_t numBytes)
{

    if (connP->issecure != 0) //(connP->dtlsSession != NULL)
    {
        LOG("security mode");
        mbedtls_net_set_nonblock(&connP->server_fd);
        mbedtls_ssl_set_bio(&connP->ssl, connP, mbeddtls_lwm2m_send, mbeddtls_lwm2m_recv, NULL);
        int result = mbedtls_ssl_read(&connP->ssl, buffer, numBytes);
        if (result < 0)
        {
            LOG_ARG("error dtls handling message %d\n", result);
            return result;
        }
        LOG_ARG("after mbedtls_ssl_read,relsult=%d", result);
        lwm2m_handle_packet(connP->lwm2mH, buffer, result, (void *)connP);
        return 0;
    }
    else
    {
        // no security, just give the plaintext buffer to liblwm2m
        lwm2m_handle_packet(connP->lwm2mH, buffer, numBytes, (void *)connP);
        return 0;
    }
}

uint8_t lwm2m_buffer_send(void *sessionH,
                          uint8_t *buffer,
                          size_t length,
                          void *userdata)
{
    dtls_connection_t *connP = (dtls_connection_t *)sessionH;

    if (connP == NULL)
    {
        LOG_ARG("#> failed sending %lu bytes, missing connection\r\n", length);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }

    if (-1 == connection_send(connP, buffer, length))
    {
        LOG_ARG("#> failed sending %lu bytes\r\n", length);
        return COAP_500_INTERNAL_SERVER_ERROR;
    }
    LOG("lwm2m_buffer_send success");
    return COAP_NO_ERROR;
}

bool lwm2m_session_is_equal(void *session1,
                            void *session2,
                            void *userData)
{
    return (session1 == session2);
}
