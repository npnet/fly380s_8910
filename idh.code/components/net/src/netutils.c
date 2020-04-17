
#include "netutils.h"

#include "cfw.h"
#include "cfw_errorcode.h"

struct netif *getGprsNetIf(uint8_t nSim, uint8_t nCid)
{
    return netif_get_by_cid(nSim << 4 | nCid);
}

struct netif *getEtherNetIf(uint8_t nCid)
{
    if (nCid != 0x11)
        sys_arch_printf("getEtherNetIf nCid:%d is Error\n", nCid);
    return netif_get_by_cid(0xf0 | nCid);
}

struct netif *
ip4_src_route(const ip4_addr_t *dst, const ip4_addr_t *src)
{
    LWIP_UNUSED_ARG(dst); /* in case IPv4-only and source-based routing is disabled */
    struct netif *netif;

    if (src == NULL)
        return NULL;
    /* iterate through netifs */
    for (netif = netif_list; netif != NULL; netif = netif->next)
    {
        /* is the netif up, does it have a link and a valid address? */
        if (netif_is_up(netif) && netif_is_link_up(netif) && !ip4_addr_isany_val(*netif_ip4_addr(netif)))
        {
            /* network mask matches? */
            if (ip4_addr_cmp(src, netif_ip4_addr(netif)))
            {
                /* return netif on which to forward IP packet */
                return netif;
            }
        }
    }
    return NULL;
}

uint16_t Sulgsmbcd2asciiEx(uint8_t *pBcd, uint16_t nBcdLen, uint8_t *pNumber)
{
    uint16_t i;
    uint8_t *pTmp = pNumber;
    for (i = 0; i < nBcdLen; i++)
    {
        uint16_t high4bits = pBcd[i] >> 4;
        uint16_t low4bits = pBcd[i] & 0x0F;

        if (low4bits < 0x0A)
            *pTmp++ = low4bits + '0'; // 0 - 0x09+
        else if (low4bits < 0x0F)     // 0x0A - 0x0F
            *pTmp++ = low4bits - 0x0A + 'A';

        if (high4bits < 0x0A)
            *pTmp++ = high4bits + '0'; // 0 - 9
        else if (high4bits < 0x0F)     // 0x0A - 0x0F
            *pTmp++ = high4bits - 0x0A + 'A';
    }
    return pTmp - pNumber;
}

bool getSimIccid(uint8_t simId, uint8_t *simiccid, uint8_t *len)
{
    if (*len < 20)
    {
        sys_arch_printf("getSimIccid params error:%d", *len);
        return false;
    }
    uint8_t *pICCID = CFW_GetICCID(simId);
    if (pICCID != NULL)
    {
        *len = Sulgsmbcd2asciiEx(pICCID, 10, simiccid);
        sys_arch_printf("getSimIccid:%s, %d", simiccid, *len);
    }
    return true;
}

bool getSimImei(uint8_t simId, uint8_t *imei, uint8_t *len)
{
    if (*len < 16)
    {
        sys_arch_printf("getSimImei params error:%d", *len);
        return false;
    }

    CFW_EmodGetIMEI(imei, len, simId);
    sys_arch_printf("getSimImei:%s, %d", imei, *len);
    return true;
}

void cfwIMSItoASC(uint8_t *InPut, uint8_t *OutPut, uint8_t *OutLen)
{
    uint8_t i;

    OutPut[0] = ((InPut[1] & 0xF0) >> 4) + 0x30;
    for (i = 2; i < InPut[0] + 1; i++)
    {
        OutPut[2 * (i - 1) - 1] = (InPut[i] & 0x0F) + 0x30;
        OutPut[2 * (i - 1)] = ((InPut[i] & 0xF0) >> 4) + 0x30;
    }
    OutPut[2 * InPut[0] - 1] = 0x00;
    *OutLen = 2 * InPut[0] - 1;
}

bool getSimImsi(uint8_t simId, uint8_t *imsi, uint8_t *len)
{
    uint8_t nPreIMSI[16] = {0};
    uint32_t nRet = CFW_CfgGetIMSI(nPreIMSI, simId);
    if (ERR_SUCCESS == nRet)
    {
        if (*len < 16)
        {
            sys_arch_printf("getSimImsi params error:%d", *len);
            return false;
        }
        cfwIMSItoASC(nPreIMSI, imsi, len);
        sys_arch_printf("getSimImsi:%s ,%d", imsi, *len);
        return true;
    }
    return false;
}
