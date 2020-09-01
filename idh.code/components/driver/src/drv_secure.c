/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
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

//#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG
#include <string.h>
#include <stdlib.h>
#include "hal_chip.h"
#include "hal_config.h"
#include "hwregs.h"
#include "osi_api.h"
#include "osi_log.h"
#include "hal_adi_bus.h"
#include "drv_efuse.h"
#include "drv_aes.h"
#include "drv_spi_flash.h"
#include <machine/endian.h>
#include "cmsis_core.h"

#define RDA_EFUSE_PUBKEY_START (8)
#define RDA_PUBKEY0_EFUSE_BLOCK_INDEX (8)
#define RDA_PUBKEY1_EFUSE_BLOCK_INDEX (10)
#define RDA_PUBKEY2_EFUSE_BLOCK_INDEX (12)
#define RDA_PUBKEY3_EFUSE_BLOCK_INDEX (14)
#define RDA_PUBKEY4_EFUSE_BLOCK_INDEX (16)
#define RDA_PUBKEY5_EFUSE_BLOCK_INDEX (18)
#define RDA_PUBKEY6_EFUSE_BLOCK_INDEX (20)
#define RDA_PUBKEY7_EFUSE_BLOCK_INDEX (30)

#define RDA_EFUSE_SECURITY_CFG_INDEX (22)
#define RDA_SE_CFG_SECURITY_ENABLE_BIT0 (1 << 4)
#define RDA_SE_CFG_SECURITY_ENABLE_BIT1 (1 << 23)
#define RDA_SE_CFG_SECURITY_ENABLE_BIT2 (1 << 25)

static uint8_t pubkey_addr[] = {
    RDA_PUBKEY0_EFUSE_DOUBLE_BLOCK_INDEX,
    RDA_PUBKEY1_EFUSE_DOUBLE_BLOCK_INDEX,
    RDA_PUBKEY2_EFUSE_DOUBLE_BLOCK_INDEX,
    RDA_PUBKEY3_EFUSE_DOUBLE_BLOCK_INDEX,
    RDA_PUBKEY4_EFUSE_DOUBLE_BLOCK_INDEX,
    RDA_PUBKEY5_EFUSE_DOUBLE_BLOCK_INDEX,
    RDA_PUBKEY6_EFUSE_DOUBLE_BLOCK_INDEX,
    RDA_PUBKEY7_EFUSE_DOUBLE_BLOCK_INDEX,
};

struct chip_id
{
    uint32_t chip; /* CHIP(31:15) NA(15:15) BOND(14:12) METAL(11:0) */
    uint32_t res1;
    uint16_t date; /*  YYYY:MMMM:DDDDD */
    uint16_t wafer;
    uint16_t xy;
    uint16_t res2;
};

struct chip_security_context
{
    uint8_t rda_key_index; /* 0-5 : 0 default key */
    uint8_t vendor_id;     /* 0-50 */
    uint16_t flags;
#define RDA_SE_CFG_UNLOCK_ALLOWED (1 << 0)
#define RDA_SE_CFG_SECURITY_ENABLE_BIT (1 << 4)
#define RDA_SE_CFG_INDIRECT_SIGN_BIT (1 << 5)
#define RDA_SE_CFG_RDCERT_DISABLE_BIT (1 << 6)
#define RDA_SE_CFG_TRACE_DISABLE_BIT (1 << 7)
};

struct chip_unique_id
{
    uint8_t id[32];
};

#define UNIQUE_ID_ANTI_CLONE 0
#define UNIQUE_ID_RD_CERT 1

#define SIGBYTES 64
#define PUBLICBYTES 32
#define FPLEN 8
struct pubkey
{
    uint8_t rdasign[4]; /* RDAS */
    uint8_t pkalg[2];   /*Ed */
    uint8_t dummy[2];
    uint8_t name[16];
    uint8_t fingerprint[FPLEN];
    uint8_t pubkey[PUBLICBYTES];
};

struct sig
{
    uint8_t rdasign[4]; /* RDAS */
    uint8_t pkalg[2];   /* Ed */
    uint8_t hashalg[2]; /* Po/B2/SH */
    uint8_t name[16];
    uint8_t fingerprint[FPLEN];
    uint8_t sig[SIGBYTES];
};

struct spl_security_info
{
    uint32_t version;
    int secure_mode;
    struct chip_id chip_id;
    struct chip_security_context chip_security_context;
    struct chip_unique_id chip_unique_id;
    struct pubkey pubkey;
    uint8_t random[32];
};

struct ROM_crypto_api
{
    char magic[8];    /* "RDA API" */
    unsigned version; /* 100 */

    /* signature */
    int (*signature_open)(
        const uint8_t *message, unsigned length,
        const struct sig *sig,
        const struct pubkey *pubkey);
    /* Return values
     * positive values is for invalid arguments
     */
#define ROM_API_SIGNATURE_OK 0
#define ROM_API_SIGNATURE_FAIL -1

    /* hash */
    unsigned sz_hash_context;
    int (*hash_init)(unsigned *S, uint8_t outlen);
    int (*hash_update)(unsigned *S, const uint8_t *in, unsigned inlen);
    int (*hash_final)(unsigned *S, uint8_t *out, uint8_t outlen);

    /* info API */
    void (*get_chip_id)(struct chip_id *id);
    void (*get_chip_unique)(struct chip_unique_id *out, int kind);
    int (*prvGetChipSecurityContext)(struct chip_security_context *context,
                                     struct pubkey *pubkey);
    /* Return values */
#define ROM_API_SECURITY_ENABLED 0
#define ROM_API_SECURITY_DISABLED 1
#define ROM_API_INVALID_KEYINDEX 2
#define ROM_API_INVALID_VENDOR_ID 3
#define ROM_API_SECURITY_UNAVAILABLE 4

    /* RND */
    void (*get_chip_true_random)(uint8_t *out, uint8_t outlen);

    int (*signature_open_w_hash)(uint8_t message_hash[64],
                                 const struct sig *signature,
                                 const struct pubkey *pubkey);
    void (*decrypt_aes_image)(uint8_t *buf, int len);
    /* Return values
     * same as signature_open() plus ROM_API_UNAVAILABLE if method is unavailable.
     */
#define ROM_API_UNAVAILABLE 100

    /* Future extension */
    unsigned dummy[15];
};

#define SECTOR_SIZE_4K (0x1000)
#define ENCRYPT_OFF 0x1000
#define ENCRYPT_LEN 0x400
#define BOOT_SIGN_SIZE (0xbd40)
#define PUBLICKEYWORDS 8

/* The ROMAPI is allocated either at 0x3f00 or 0xff00. */
#define ROMAPI_BASE 0x3f00
#define ROMAPI_BASE_LIST ROMAPI_BASE, 0xff00, 0

struct ROM_crypto_api *romapi = (void *)ROMAPI_BASE;

/* Detect the ROMAPI base address */
static void prvRomapiSetBase(void)
{
    const unsigned rapi[] = {ROMAPI_BASE_LIST};
    const unsigned *p = &rapi[0];
    while (*p)
    {
        romapi = (void *)*p;
        if (memcmp(romapi->magic, "RDA API", 8) == 0)
            break;
        p++;
    }
}

static void prvGetChipUnique(struct chip_unique_id *id, int kind)
{
    romapi->get_chip_unique(id, kind);
}

static int prvGetChipSecurityContext(struct chip_security_context *context, struct pubkey *pubkey)
{
    int ret;

    halEfuseOpen();
    ret = romapi->prvGetChipSecurityContext(context, pubkey);
    halEfuseClose();

    return ret;
}

static void prvGetDdeviceSecurityContext(struct spl_security_info *info)
{
    info->secure_mode = prvGetChipSecurityContext(
        &info->chip_security_context,
        &info->pubkey);

    int flags = info->chip_security_context.flags;

    /* Fix the return code if security hasn't been enabled */
    if ((flags & RDA_SE_CFG_SECURITY_ENABLE_BIT) == 0)
    {
        info->secure_mode = ROM_API_SECURITY_DISABLED;
        return;
    }

    /* Sanity check for PKEY */
    if (memcmp(&info->pubkey, "RDASEd", 6) != 0)
    {
        OSI_LOGI(0, "BOOTSECURE: Public key for signature check invalid\n");
        info->secure_mode = -1; /* better: ROM_API_SECURITY_INVALID_PKEY */
        return;
    }
}

static void prvAntiCloneEncryption(void *buf, uint32_t len)
{
    struct chip_unique_id rom_id;

    prvRomapiSetBase();
    prvGetChipUnique(&rom_id, UNIQUE_ID_ANTI_CLONE);
    aesEncryptObj(buf, len, &rom_id);
}

static void prvAntiClone(void)
{
    uint8_t *pbuf = malloc(SECTOR_SIZE_4K);

    drvSpiFlash_t *flash = drvSpiFlashOpen(DRV_NAME_SPI_FLASH);

    memcpy(pbuf, (void *)(CONFIG_NOR_PHY_ADDRESS + ENCRYPT_OFF), SECTOR_SIZE_4K);

    prvAntiCloneEncryption(pbuf, ENCRYPT_LEN);

    drvSpiFlashClearRangeWriteProhibit(flash, HAL_FLASH_OFFSET(CONFIG_BOOT_FLASH_ADDRESS),
                                       HAL_FLASH_OFFSET(CONFIG_BOOT_FLASH_ADDRESS) + CONFIG_BOOT_FLASH_SIZE);

    uint32_t critical = osiEnterCritical();
    drvSpiFlashErase(flash, ENCRYPT_OFF, SECTOR_SIZE_4K);
    drvSpiFlashWrite(flash, ENCRYPT_OFF, pbuf, SECTOR_SIZE_4K);
    osiExitCritical(critical);

    free(pbuf);
}

bool prvReadSecurityFlag(void)
{
    struct spl_security_info info;
    prvRomapiSetBase();
    prvGetDdeviceSecurityContext(&info);
    OSI_LOGD(0, "secure: readSecurityFlag secure_mode = %x", info.secure_mode);
    if (!info.secure_mode ||
        info.chip_security_context.flags & RDA_SE_CFG_RDCERT_DISABLE_BIT)
        return true;
    else
        return false;
}

static void prvWritePublicKey(void)
{
    uint32_t i;
    uint32_t pubkey[PUBLICKEYWORDS];
    struct pubkey *pubkey_ptr = (struct pubkey *)(CONFIG_NOR_PHY_ADDRESS + BOOT_SIGN_SIZE - sizeof(struct sig)) - 1;

    for (i = 0; i < PUBLICKEYWORDS; i++)
    {
        pubkey[i] = __ntohl(*(uint32_t *)&pubkey_ptr->pubkey[i * 4]);
        drvEfuseWrite(true, pubkey_addr[i], pubkey[i]);
        OSI_LOGD(0, "pubkey_addr = %08x pubkey[%d] = %08x", pubkey_addr[i], i, pubkey[i]);
    }
}

bool ReadSecurityFlag(void)
{
    uint32_t critical = osiEnterCritical();
    osiDCacheCleanAll();
    L1C_DisableBTAC();
    MMU_Disable();

    struct spl_security_info info;
    prvRomapiSetBase();
    prvGetDdeviceSecurityContext(&info);
    OSI_LOGD(0, "secure: readSecurityFlag secure_mode = %x", info.secure_mode);
    if (!info.secure_mode ||
        info.chip_security_context.flags & RDA_SE_CFG_RDCERT_DISABLE_BIT)
    {
        MMU_Enable();
        L1C_EnableBTAC();
        osiExitCritical(critical);
        return true;
    }
    else
    {
        MMU_Enable();
        L1C_EnableBTAC();
        osiExitCritical(critical);
        return false;
    }
}

bool writeSecuriyFlag(void)
{
    uint32_t critical = osiEnterCritical();
    osiDCacheCleanAll();
    osiICacheInvalidateAll();
    L1C_DisableBTAC();
    MMU_Disable();
    osiExitCritical(critical);

    uint32_t val;
    if (!prvReadSecurityFlag())
    {
        prvAntiClone();
        prvWritePublicKey();
        val = RDA_SE_CFG_SECURITY_ENABLE_BIT0 | RDA_SE_CFG_SECURITY_ENABLE_BIT1 | RDA_SE_CFG_SECURITY_ENABLE_BIT2;
        drvEfuseWrite(true, RDA_EFUSE_DOUBLE_BLOCK_SECURITY_CFG_INDEX, val);
        OSI_LOGD(0, "secure: RDA_EFUSE_SECURITY_CFG =  %08x", val);
    }

    return true;
}

void drvSecureTest(void)
{
    //prvAntiClone();
    //prvWritePublicKey();
    writeSecuriyFlag();
}
