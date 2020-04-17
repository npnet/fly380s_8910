
#include <https_api.h>
#include "http_api.h"
#include "mbedtls/timing.h"

#if defined(MUPNP_USE_OPENSSL)
uint8_t *ca_pem = NULL;
uint8_t *cert_pem = NULL;
uint8_t *key_pem = NULL;
#endif

void mbedtls_debug_set_threshold(int threshold);

static void csocket_debug(void *ctx, int level,
                          const char *file, int line,
                          const char *str)
{
    ((void)level);
    sys_arch_printf("https mbedtls_ssl: %s", str);
}

int mupnp_mbedtlssocket_connect(mUpnpSocket *sock, const char *addr, int port)
{

#if defined(MUPNP_USE_OPENSSL)
    int ret;

    sys_arch_printf("cg_socket_connect,begin");

    uint32_t flags;
    const char *pers = addr;

    mbedtls_x509_crt_init(&sock->clicert);

    mbedtls_pk_init(&sock->pkey);

    mbedtls_net_init(&sock->server_fd);

    sock->ssl = malloc(sizeof(mbedtls_ssl_context));
    mbedtls_ssl_init(sock->ssl);

    mbedtls_ssl_config_init(&sock->conf);

    mbedtls_x509_crt_init(&sock->cacert);

    mbedtls_ctr_drbg_init(&sock->ctr_drbg);

    mbedtls_entropy_init(&sock->entropy);

    if ((ret = mbedtls_ctr_drbg_seed(&sock->ctr_drbg, mbedtls_entropy_func, &sock->entropy,
                                     (const unsigned char *)pers, strlen(pers))) != 0)
    {
        sys_arch_printf("mbedtls_ctr_drbg_seed failed...,ret=%d\n", ret);
        return ret;
    }

/* read crt from file */
#if CG_HTTPS_NEED_CERT
    bool result = false;
    //set caCrt through read certificate from file
    result = Https_setCrt(CG_HTTPS_CA_CRT, &ca_pem);
    if (!result || NULL == ca_pem)
    {
        sys_arch_printf("caCrt set fail!");
        //return -1;
    }
    //set clientCrt through read certificate from file
    result = Https_setCrt(CG_HTTPS_CLIENT_CRT, &cert_pem);
    if (!result || NULL == cert_pem)
    {
        free(ca_pem);
        ca_pem = NULL;
        sys_arch_printf("clientCrt set fail!");
        //return -1;
    }
    //set caCrt through read certificate from file
    result = Https_setCrt(CG_HTTPS_CLIENT_PRIVATE_KEY, &key_pem);
    if (!result || NULL == key_pem)
    {
        free(ca_pem);
        ca_pem = NULL;
        free(cert_pem);
        cert_pem = NULL;
        sys_arch_printf("client private key set fail!");
        //return -1;
    }

    ret = mbedtls_x509_crt_parse(&sock->cacert, (const unsigned char *)ca_pem,
                                 (ca_pem == NULL) ? 0 : (strlen((const char *)ca_pem) + 1));
    if (ret < 0)
    {
        free(ca_pem);
        free(cert_pem);
        free(key_pem);
        ca_pem = NULL;
        cert_pem = NULL;
        key_pem = NULL;
        sys_arch_printf("mbedtls_x509_crt_parse ca_pem failed...,ret=0x%x\n", -ret);
        //return ret;
    }

    ret = mbedtls_x509_crt_parse(&sock->clicert, (const unsigned char *)cert_pem,
                                 (cert_pem == NULL) ? 0 : (strlen((const char *)cert_pem) + 1));
    if (ret < 0)
    {
        free(ca_pem);
        free(cert_pem);
        free(key_pem);
        ca_pem = NULL;
        cert_pem = NULL;
        key_pem = NULL;

        sys_arch_printf("mbedtls_x509_crt_parse cert_pem failed...,ret=0x%x\n", -ret);
        //return ret;
    }

    ret = mbedtls_pk_parse_key(&sock->pkey, (const unsigned char *)key_pem,
                               (key_pem == NULL) ? 0 : (strlen((const char *)key_pem) + 1), NULL, 0);

    if (ret < 0)
    {
        free(ca_pem);
        free(cert_pem);
        free(key_pem);
        ca_pem = NULL;
        cert_pem = NULL;
        key_pem = NULL;

        sys_arch_printf("mbedtls_pk_parse_key key_pem failed...,ret=0x%x\n", -ret);
        //return ret;
    }
#endif
    if (ca_pem != NULL)
    {
        free(ca_pem);
        ca_pem = NULL;
        sys_arch_printf("free ca_pem");
    }
    if (cert_pem != NULL)
    {
        free(cert_pem);
        cert_pem = NULL;
        sys_arch_printf("free cert_pem");
    }
    if (key_pem != NULL)
    {
        free(key_pem);
        key_pem = NULL;
        sys_arch_printf("free key_pem");
    }
    ret = mbedtls_ssl_conf_own_cert(&sock->conf, &sock->clicert, &sock->pkey);
    if (ret < 0)
    {
        sys_arch_printf("mbedtls_ssl_conf_own_cert failed...,ret=0x%x\n", -ret);
        return ret;
    }

    //(sock->server_fd).fd = (int)sock->id;
    char portStr[32];
    sprintf(portStr, "%d", port);
    if ((ret = mbedtls_net_connect(&sock->server_fd, addr,
                                   portStr, MBEDTLS_NET_PROTO_TCP)) != 0) //htons(port)
    {
        sys_arch_printf("mbedtls_net_connect failed...,ret=0x%x\n", -ret);
        return ret;
    }
    mupnp_socket_setid(sock, (sock->server_fd).fd);
    mupnp_socket_setdirection(sock, MUPNP_NET_SOCKET_CLIENT);
    sys_arch_printf("begin to mbedtls_ssl_config_defaults\n");
    if ((ret = mbedtls_ssl_config_defaults(&sock->conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        sys_arch_printf("mbedtls_ssl_config_defaults failed ret = %d\n", ret);
        return ret;
    }
    /* OPTIONAL is not optimal for security,
                * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode(&sock->conf, CG_HTTPS_NEED_CERT);
    mbedtls_ssl_conf_ca_chain(&sock->conf, &sock->cacert, NULL);
    mbedtls_ssl_conf_rng(&sock->conf, mbedtls_ctr_drbg_random, &sock->ctr_drbg);
#if define MBEDTLS_DEBUG_C
    mbedtls_debug_set_threshold(5);
#else
    mbedtls_debug_set_threshold(0);
#endif
    mbedtls_ssl_conf_dbg(&sock->conf, csocket_debug, NULL);

    if ((ret = mbedtls_ssl_setup(sock->ssl, &sock->conf)) != 0)
    {
        sys_arch_printf("mbedtls_ssl_setup failed ret = %d\n", ret);
        return ret;
    }

    /*if( ( ret = mbedtls_ssl_set_hostname( sock->ssl, addr ) ) != 0 ) 
        { 
            sys_arch_printf( " failed\n ! mbedtls_ssl_set_hostname returned %d\n\n", ret ); 
            return ret;
        } */
    //mbedtls_timing_delay_context *timer = malloc(sizeof (mbedtls_timing_delay_context));
    mbedtls_ssl_conf_read_timeout(&sock->conf, 30000);
    //mbedtls_ssl_set_timer_cb(sock->ssl, timer, mbedtls_timing_set_delay,
    //  mbedtls_timing_get_delay );

    mbedtls_ssl_set_bio(sock->ssl, &sock->server_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    /* 
        * 4. Handshake 
        */
    sys_arch_printf(" . Performing the SSL/TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(sock->ssl)) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            sys_arch_printf(" failed ! mbedtls_ssl_handshake returned %x\n\n", ret);
            return ret;
        }
    }

    sys_arch_printf(" ok\n");

    /* 
        * 5. Verify the server certificate 
        */
    mupnp_log_debug_l4(" . Verifying peer X.509 certificate...");

    /* In real life, we probably want to bail out when ret != 0 */
    if ((flags = mbedtls_ssl_get_verify_result(sock->ssl)) != 0)
    {
        char vrfy_buf[512];

        sys_arch_printf(" mbedtls_ssl_get_verify_result failed\n");

        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), " ! ", flags);

        sys_arch_printf("%s\n", vrfy_buf);
    }
    else
        sys_arch_printf("mbedtls_ssl_get_verify_result ok\n");

    return ret;
#endif
}
