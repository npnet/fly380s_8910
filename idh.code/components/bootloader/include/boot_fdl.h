#ifndef _BOOT_FDL_H_
#define _BOOT_FDL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "boot_fdl_channel.h"

typedef enum
{
    FDL_DEVICE_UART = 0,
    FDL_DEVICE_USB_SERIAL,
    FDL_DEVICE_NUM,
} fdlDeviceType_t;

#define HDLC_FLAG 0x7E
#define HDLC_ESCAPE 0x7D
#define HDLC_ESCAPE_MASK 0x20

enum CMD_TYPE
{
    BSL_PKT_TYPE_MIN = 0,                /* the bottom of the DL packet type range */
    BSL_CMD_TYPE_MIN = BSL_PKT_TYPE_MIN, /* 0x0 */
    CMD_DL_SYNC = BSL_PKT_TYPE_MIN,      /* 0x0 */
    CMD_DL_BEGIN,                        /* 0x1 */
    CMD_DL_BEGIN_RSP,                    /* 0x2 */
    CMD_DL_DATA,                         /* 0x3 */
    CMD_DL_DATA_RSP,                     /* 0x4 */
    CMD_DL_END,                          /* 0x5 */
    CMD_DL_END_RSP,                      /* 0x6 */
    CMD_RUN_GSMSW,                       /* 0x7 */
    CMD_RUN_GSMSW_RSP,                   /* 0x8 */

    /* Link Control */
    BSL_CMD_CONNECT = BSL_CMD_TYPE_MIN, /* 0x0 */
    /* Data Download */
    /* the start flag of the data downloading */
    BSL_CMD_START_DATA, /* 0x1 */
    /* the midst flag of the data downloading */
    BSL_CMD_MIDST_DATA, /* 0x2 */
    /* the end flag of the data downloading */
    BSL_CMD_END_DATA, /* 0x3 */
    /* Execute from a certain address */
    BSL_CMD_EXEC_DATA,      /* 0x4 */
    BSL_CMD_NORMAL_RESET,   /* 0x5 */
    BSL_CMD_READ_FLASH,     /* 0x6 */
    BSL_CMD_READ_CHIP_TYPE, /* 0x7 */
    BSL_CMD_LOOKUP_NVITEM,  /* 0x8 */
    BSL_SET_BAUDRATE,       /* 0x9 */
    BSL_ERASE_FLASH,        /* 0xA */
    BSL_REPARTITION,        /* 0xB */
    BSL_CMD_POWER_OFF = 0x17,

    /* Start of the Command can be transmited by phone*/
    BSL_REP_TYPE_MIN = 0x80,

    /* The operation acknowledge */
    BSL_REP_ACK = BSL_REP_TYPE_MIN, /* 0x80 */
    BSL_REP_VER,                    /* 0x81 */

    /* the operation not acknowledge */
    /* system  */
    BSL_REP_INVALID_CMD,      /* 0x82 */
    BSL_REP_UNKNOW_CMD,       /* 0x83 */
    BSL_REP_OPERATION_FAILED, /* 0x84 */

    /* Link Control*/
    BSL_REP_NOT_SUPPORT_BAUDRATE, /* 0x85 */

    /* Data Download */
    BSL_REP_DOWN_NOT_START,   /* 0x86 */
    BSL_REP_DOWN_MULTI_START, /* 0x87 */
    BSL_REP_DOWN_EARLY_END,   /* 0x88 */
    BSL_REP_DOWN_DEST_ERROR,  /* 0x89 */
    BSL_REP_DOWN_SIZE_ERROR,  /* 0x8A */
    BSL_REP_VERIFY_ERROR,     /* 0x8B */
    BSL_REP_NOT_VERIFY,       /* 0x8C */

    /* Phone Internal Error */
    BSL_PHONE_NOT_ENOUGH_MEMORY,  /* 0x8D */
    BSL_PHONE_WAIT_INPUT_TIMEOUT, /* 0x8E */

    /* Phone Internal return value */
    BSL_PHONE_SUCCEED,         /* 0x8F */
    BSL_PHONE_VALID_BAUDRATE,  /* 0x90 */
    BSL_PHONE_REPEAT_CONTINUE, /* 0x91 */
    BSL_PHONE_REPEAT_BREAK,    /* 0x92 */

    BSL_REP_READ_FLASH,     /* 0x93 */
    BSL_REP_READ_CHIP_TYPE, /* 0x94 */
    BSL_REP_LOOKUP_NVITEM,  /* 0x95 */

    BSL_INCOMPATIBLE_PARTITION, /* 0x96 */
    BSL_UNKNOWN_DEVICE,         /* 0x97 */
    BSL_INVALID_DEVICE_SIZE,    /* 0x98 */

    BSL_ILLEGAL_SDRAM,         /* 0x99 */
    BSL_WRONG_SDRAM_PARAMETER, /* 0x9a */
    BSL_EEROR_CHECKSUM = 0xA0,
    BSL_CHECKSUM_DIFF,
    BSL_WRITE_ERROR,
    BSL_UDISK_IMAGE_SIZE_OVERFLOW, /* 0xa3 */
    BSL_FLASH_CFG_ERROR = 0xa4,    /* 0xa4 */
    BSL_REP_DOWN_STL_SIZE_ERROR,
    BSL_REP_SECURITY_VERIFICATION_FAIL = 0xa6,
    BSL_REP_LOG = 0xFF, /* 0xff */
    BSL_PKT_TYPE_MAX,
    BSL_CMD_TYPE_MAX = BSL_PKT_TYPE_MAX
};

typedef union {
    struct
    {
        uint32_t type : 8;
        uint32_t baud : 24;
    } b;
    uint32_t info;
} fdlDeviceInfo_t;

/**
 * @brief the FDL instance
 */
typedef struct fdl_engine fdlEngine_t;

struct fdl_packet
{
    uint16_t type;      ///< cpu endian
    uint16_t size;      ///< cpu endian
    uint8_t content[1]; ///< placeholder for data
} __attribute__((packed));

/**
 * @brief FDL packet struct
 */
typedef struct fdl_packet fdlPacket_t;

typedef enum
{
    SYS_STAGE_NONE,
    SYS_STAGE_CONNECTED,
    SYS_STAGE_START,
    SYS_STAGE_GATHER,
    SYS_STAGE_END,
    SYS_STAGE_ERROR
} fdlDnldStage_t;

typedef enum
{
    DNLD_ACTION_NORMAL,
    DNLD_ACTION_RESET,
    DNLD_ACTION_POWEROFF
} fdlDnldAction_t;

/**
 * @brief structure to store download state
 */
typedef struct
{
    fdlDnldStage_t stage;
    fdlDnldAction_t action;
    uint32_t total_size;
    uint32_t received_size;
    uint32_t start_address;
    uint32_t write_address;
    uint32_t data_verify;
    uint32_t detect_time;
} fdlDnld_t;

/**
 * @brief FDL packet processor
 */
typedef void (*fdlProc_t)(fdlEngine_t *fdl, fdlPacket_t *pkt, void *ctx);

/**
 * @brief Create a fdl engine
 *
 * @param channel       the pdl channel
 * @return
 *      - NULL          fail
 *      - otherwise     the fdl engine instance
 */
fdlEngine_t *fdlEngineCreate(fdlChannel_t *channel);

/**
 * @brief Destroy the fdl engine
 *
 * @param fdl   the fdl engine
 */
void fdlEngineDestroy(fdlEngine_t *fdl);

/**
 * @brief Read raw data from fdl
 *
 * @param fdl   the fdl engine
 * @param buf   the room to store data
 * @param size  the buffer size
 * @retuen
 *      - num   actual read size in bytes
 */
uint32_t fdlEngineReadRaw(fdlEngine_t *fdl, void *buf, unsigned size);

/**
 * @brief Write raw data via fdl engine
 *
 * @param fdl       the fdl engine
 * @param data      the data to be written
 * @param length    the data length
 * @return
 *      - num       actual length written
 */
uint32_t fdlEngineWriteRaw(fdlEngine_t *fdl, const void *data, unsigned length);

/**
 * @brief FDL Send version string to the other peer
 *
 * @param fdl   the fdl engine
 * @return
 *      - true  success
 *      - false false
 */
bool fdlEngineSendVersion(fdlEngine_t *fdl);

/**
 * @breif FDL send no data response of specific type
 *
 * @param fdl   the fdl engine
 * @param type  response type
 * @return
 *      - true  success
 *      - false fail
 */

bool fdlEngineSendRespNoData(fdlEngine_t *fdl, uint16_t type);

/**
 * @breif FDL send response of specific type with data
 *
 * @param fdl   the fdl engine
 * @param type  response type
 * @param data  the data to be sent
 * @param len   the data length
 * @return
 *      - true  success
 *      - false fail
 */

bool fdlEngineSendRespData(fdlEngine_t *fdl, uint16_t type, const void *data, unsigned length);

/**
 * @brief FDL change device baud rate
 *
 * @param fdl   the fdl engine
 * @param baud  baud rate to be changed
 * @return
 *      - true on success
 *      - false on fail, usually the baud rate is not supported
 */
bool fdlEngineSetBaud(fdlEngine_t *fdl, unsigned baud);

/**
 * @brief FDL engine process
 *
 * @note never return except failed
 *
 * @param fdl       the fdl engine
 * @param proc      the fdl packet processor
 * @param ctx       caller context
 * @return
 *      - false fail
 */
bool fdlEngineProcess(fdlEngine_t *fdl, fdlProc_t proc, void *ctx);

#ifdef __cplusplus
}
#endif

#endif
