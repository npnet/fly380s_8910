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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('S', 'D', 'M', 'C')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_INFO

#include "drv_sdmmc.h"
#include "osi_log.h"
#include "osi_api.h"
#include "hwregs.h"
#include <stdlib.h>
#include "drv_ifc.h"
#include "hal_chip.h"

/**
 * sdmmcCardState_t
 *
 * The state of the card when receiving the command. If the command execution
 * causes a state change, it will be visible to the host in the response to
 * the next command. The four bits are interpreted as a binary coded number
 * between 0 and 15.
 */
typedef enum
{
    SDMMC_CARD_STATE_IDLE = 0,
    SDMMC_CARD_STATE_READY = 1,
    SDMMC_CARD_STATE_IDENT = 2,
    SDMMC_CARD_STATE_STBY = 3,
    SDMMC_CARD_STATE_TRAN = 4,
    SDMMC_CARD_STATE_DATA = 5,
    SDMMC_CARD_STATE_RCV = 6,
    SDMMC_CARD_STATE_PRG = 7,
    SDMMC_CARD_STATE_DIS = 8
} sdmmcCardState_t;

/**
 * sdmmcCardStatus_t
 *
 * Card status as returned by R1 reponses (spec V2 pdf p.)
 */
typedef union {
    uint32_t v;
    struct
    {
        uint32_t : 3;                      // [0:2]
        uint32_t akeSeqError : 1;          // [3]
        uint32_t : 1;                      // [4]
        uint32_t appCmd : 1;               // [5]
        uint32_t : 2;                      // [6:7]
        uint32_t readyForData : 1;         // [8]
        sdmmcCardState_t currentState : 4; // [9:12]
        uint32_t eraseReset : 1;           // [13]
        uint32_t cardEccDisabled : 1;      // [14]
        uint32_t wpEraseSkip : 1;          // [15]
        uint32_t csdOverwrite : 1;         // [16]
        uint32_t : 2;                      // [17:18]
        uint32_t error : 1;                // [19]
        uint32_t ccError : 1;              // [20]
        uint32_t cardEccFailed : 1;        // [21]
        uint32_t illegalCommand : 1;       // [22]
        uint32_t comCrcError : 1;          // [23]
        uint32_t lockUnlockFail : 1;       // [24]
        uint32_t cardIsLocked : 1;         // [25]
        uint32_t wpViolation : 1;          // [26]
        uint32_t eraseParam : 1;           // [27]
        uint32_t eraseSeqError : 1;        // [28]
        uint32_t blockLenError : 1;        // [29]
        uint32_t addressError : 1;         // [30]
        uint32_t outOfRange : 1;           // [31]
    } b;
} sdmmcCardStatus_t;

/**
 * sdmmcCsd_t
 *
 * This structure contains the fields of the MMC chip's register.
 * For more details, please refer to your MMC specification.
 */
typedef struct
{                          // Ver 2. // Ver 1.0 (if different)
    uint8_t csdStructure;  // [127:126]
    uint8_t specVers;      // [125:122]
    uint8_t taac;          // [119:112]
    uint8_t nsac;          // [111:104]
    uint8_t tranSpeed;     // [103:96]
    uint16_t ccc;          //  [95:84]
    uint8_t readBlLen;     //  [83:80]
    bool readBlPartial;    //  [79]
    bool writeBlkMisalign; //  [78]
    bool readBlkMisalign;  //  [77]
    bool dsrImp;           //  [76]
    uint32_t cSize;        //  [69:48] // [73:62]
    uint8_t vddRCurrMin;   //           [61:59]
    uint8_t vddRCurrMax;   //           [58:56]
    uint8_t vddWCurrMin;   //           [55:53]
    uint8_t vddWCurrMax;   //           [52:50]
    uint8_t cSizeMult;     //           [49:47]
    // FIXME
    uint8_t eraseBlkEnable;
    uint8_t eraseGrpSize; //   [46:42]
    // FIXME
    uint8_t sectorSize;
    uint8_t eraseGrpMult; //   [41:37]

    uint8_t wpGrpSize;     //  [38:32]
    bool wpGrpEnable;      //  [31:31]
    uint8_t defaultEcc;    //  [30:29]
    uint8_t r2wFactor;     //  [28:26]
    uint8_t writeBlLen;    //  [25:22]
    bool writeBlPartial;   //  [21]
    bool contentProtApp;   //  [16]
    bool fileFormatGrp;    //  [15]
    bool copy;             //  [14]
    bool permWriteProtect; //  [13]
    bool tmpWriteProtect;  //  [12]
    uint8_t fileFormat;    //  [11:10]
    uint8_t ecc;           //   [9:8]
    uint8_t crc;           //   [7:1]
    /// This field is not from the CSD register.
    /// This is the actual block number.
    uint32_t blockNumber;
} sdmmcCsd_t;

/**
 * sdmmcCardVer_t
 *
 * Card version
 */

typedef enum
{
    SDMMC_CARD_V1,
    SDMMC_CARD_V2
} sdmmcCardVer_t;

/**
 * SDMMC_ACMD_SEL
 * mark a command as application specific
 */
#define SDMMC_ACMD_SEL 0x80000000

/**
 * SDMMC_CMD_MASK
 *
 * Mask to get from a sdmmcCmd_t value the corresponding
 * command index
 */
#define SDMMC_CMD_MASK 0x3F

/**
 * sdmmcCmd_t
 * SD commands index
 */
typedef enum
{
    SDMMC_CMD_GO_IDLE_STATE = 0,
    SDMMC_CMD_MMC_SEND_OP_COND = 1,
    SDMMC_CMD_ALL_SEND_CID = 2,
    SDMMC_CMD_SEND_RELATIVE_ADDR = 3,
    SDMMC_CMD_SET_DSR = 4,
    SDMMC_CMD_SWITCH = 6,
    SDMMC_CMD_SELECT_CARD = 7,
    SDMMC_CMD_SEND_IF_COND = 8,
    SDMMC_CMD_SEND_CSD = 9,
    SDMMC_CMD_STOP_TRANSMISSION = 12,
    SDMMC_CMD_SEND_STATUS = 13,
    SDMMC_CMD_SET_BLOCKLEN = 16,
    SDMMC_CMD_READ_SINGLE_BLOCK = 17,
    SDMMC_CMD_READ_MULT_BLOCK = 18,
    SDMMC_CMD_WRITE_SINGLE_BLOCK = 24,
    SDMMC_CMD_WRITE_MULT_BLOCK = 25,
    SDMMC_CMD_APP_CMD = 55,
    SDMMC_CMD_SET_BUS_WIDTH = (6 | SDMMC_ACMD_SEL),
    SDMMC_CMD_SEND_NUM_WR_BLOCKS = (22 | SDMMC_ACMD_SEL),
    SDMMC_CMD_SET_WR_BLK_COUNT = (23 | SDMMC_ACMD_SEL),
    SDMMC_CMD_SEND_OP_COND = (41 | SDMMC_ACMD_SEL)
} sdmmcCmd_t;

/**
 * sdmmcDirection_t
 *
 * Describe the direction of a transfer between the SDmmc controller and a
 * pluggued card.
 */
typedef enum
{
    SDMMC_DIRECTION_READ,
    SDMMC_DIRECTION_WRITE,

    SDMMC_DIRECTION_QTY
} sdmmcDirection_t;

/**
 * sdmmcCheckStatus_t
 *
 * check register hwp->sdmmc_status different bit
 */
typedef enum
{
    SDMMC_TRANSFER_DONE,
    SDMMC_RESPONSE_OK,
    SDMMC_CRC_STATUS,
    SDMMC_DATA_ERROR,

    SDMMC_QTY
} sdmmcCheckStatus_t;

/**
 * sdmmcDataBusWidth_t
 *
 * Cf spec v2 pdf p. 76 for ACMD6 argument
 * That type is used to describe how many data lines are used to transfer data
 * to and from the SD card.
 */
typedef enum
{
    SDMMC_DATA_BUS_WIDTH_1 = 0x0,
    SDMMC_DATA_BUS_WIDTH_4 = 0x2,
    SDMMC_DATA_BUS_WIDTH_8 = 0x4
} sdmmcDataBusWidth_t;

struct drvSdmmc
{
    uint32_t name;
    HWP_SDMMC_T *hwp;
    drvIfcChannel_t rx_ifc;
    drvIfcChannel_t tx_ifc;
    uint32_t irqn;

    sdmmcCsd_t csd;
    sdmmcCardVer_t sdmmc_ver;
    uint32_t sdmmc_ocr_reg;
    uint8_t clk_adj;
    uint8_t clk_inv;
    bool card_is_sdhc;
    sdmmcDataBusWidth_t bus_width;
    uint32_t rca_reg; ///*rca address is high 16 bits, low 16 bits are stuff.You can set rca 0 or other digitals*/
    uint32_t dsr;
    uint32_t max_clk;
    uint32_t master_clk;

    bool rx_enabled;
    bool tx_enabled;
    /// This address in the system memory
    uint8_t *sys_mem_addr;
    /// Address in the SD card
    uint8_t *sdmmc_addr;
    /// Quantity of data to transfer, in blocks
    uint32_t block_num;

    uint32_t block_num_total;
    /// Block size
    uint32_t block_size;
    sdmmcDirection_t direction;
    osiPmSource_t *pm_source;
    osiSemaphore_t *tx_done_sema;
    osiSemaphore_t *rx_done_sema;
    osiMutex_t *lock;
};

static void _sdmmcSendCmd(drvSdmmc_t *d, sdmmcCmd_t cmd, uint32_t arg)
{
    REG_SDMMC_SDMMC_CONFIG_T sdmmc_config = {};

    switch (cmd)
    {
    case SDMMC_CMD_ALL_SEND_CID:
    case SDMMC_CMD_SEND_CSD:
        sdmmc_config.b.rsp_sel = 2;
        sdmmc_config.b.rsp_en = 1;
        break;

    case SDMMC_CMD_SEND_RELATIVE_ADDR:
    case SDMMC_CMD_SEND_IF_COND:
    case SDMMC_CMD_SELECT_CARD:
    case SDMMC_CMD_SEND_STATUS:
    case SDMMC_CMD_SET_BLOCKLEN:
        sdmmc_config.b.rsp_sel = 0;
        sdmmc_config.b.rsp_en = 1;
        break;

    case SDMMC_CMD_SET_DSR:
    case SDMMC_CMD_GO_IDLE_STATE:
    case SDMMC_CMD_STOP_TRANSMISSION:
        break;

    case SDMMC_CMD_READ_SINGLE_BLOCK:
        sdmmc_config.b.rd_wt_sel = 0;
        sdmmc_config.b.rd_wt_en = 1;
        sdmmc_config.b.rsp_sel = 0;
        sdmmc_config.b.rsp_en = 1;
        break;

    case SDMMC_CMD_READ_MULT_BLOCK:
        sdmmc_config.b.s_m_sel = 1;
        sdmmc_config.b.rd_wt_sel = 0;
        sdmmc_config.b.rd_wt_en = 1;
        sdmmc_config.b.rsp_sel = 0;
        sdmmc_config.b.rsp_en = 1;
        sdmmc_config.b.bit_16 = 1;
        break;

    case SDMMC_CMD_WRITE_SINGLE_BLOCK:
        sdmmc_config.b.rd_wt_sel = 1;
        sdmmc_config.b.rd_wt_en = 1;
        sdmmc_config.b.rsp_sel = 0;
        sdmmc_config.b.rsp_en = 1;
        break;

    case SDMMC_CMD_WRITE_MULT_BLOCK:
        sdmmc_config.b.s_m_sel = 1;
        sdmmc_config.b.rd_wt_sel = 1;
        sdmmc_config.b.rd_wt_en = 1;
        sdmmc_config.b.rsp_sel = 0;
        sdmmc_config.b.rsp_en = 1;
        sdmmc_config.b.bit_16 = 1;
        break;

    case SDMMC_CMD_APP_CMD:
    case SDMMC_CMD_SET_BUS_WIDTH:
    case SDMMC_CMD_SWITCH:
        sdmmc_config.b.rsp_sel = 0;
        sdmmc_config.b.rsp_en = 1;
        break;

    case SDMMC_CMD_SEND_NUM_WR_BLOCKS:
        sdmmc_config.b.s_m_sel = 1;
        sdmmc_config.b.rd_wt_sel = 0;
        sdmmc_config.b.rd_wt_en = 1;
        sdmmc_config.b.rsp_sel = 0;
        sdmmc_config.b.rsp_en = 1;
        sdmmc_config.b.bit_16 = 1;
        break;

    case SDMMC_CMD_SET_WR_BLK_COUNT:
        sdmmc_config.b.rsp_sel = 0;
        sdmmc_config.b.rsp_en = 1;
        break;

    case SDMMC_CMD_MMC_SEND_OP_COND:
    case SDMMC_CMD_SEND_OP_COND:
        sdmmc_config.b.rsp_sel = 1;
        sdmmc_config.b.rsp_en = 1;
        break;

    default:
        break;
    }
    sdmmc_config.b.sdmmc_sendcmd = 1;
    d->hwp->sdmmc_cmd_index = cmd & 0x3f;
    d->hwp->sdmmc_cmd_arg = arg;
    d->hwp->sdmmc_config = sdmmc_config.v;
    OSI_LOGD(0, "sdmmc: cmd:%d,arg:%x,config:%x", cmd & 0x3f, arg, sdmmc_config.v);
}

static bool _sdmmcNeedResponse(sdmmcCmd_t cmd)
{
    switch (cmd)
    {
    case SDMMC_CMD_GO_IDLE_STATE:
    case SDMMC_CMD_SET_DSR:
    case SDMMC_CMD_STOP_TRANSMISSION:
        return false;
        break;

    case SDMMC_CMD_ALL_SEND_CID:
    case SDMMC_CMD_SEND_RELATIVE_ADDR:
    case SDMMC_CMD_SEND_IF_COND:
    case SDMMC_CMD_SELECT_CARD:
    case SDMMC_CMD_SEND_CSD:
    case SDMMC_CMD_SEND_STATUS:
    case SDMMC_CMD_SET_BLOCKLEN:
    case SDMMC_CMD_READ_SINGLE_BLOCK:
    case SDMMC_CMD_READ_MULT_BLOCK:
    case SDMMC_CMD_WRITE_SINGLE_BLOCK:
    case SDMMC_CMD_WRITE_MULT_BLOCK:
    case SDMMC_CMD_APP_CMD:
    case SDMMC_CMD_SET_BUS_WIDTH:
    case SDMMC_CMD_SEND_NUM_WR_BLOCKS:
    case SDMMC_CMD_SET_WR_BLK_COUNT:
    case SDMMC_CMD_MMC_SEND_OP_COND:
    case SDMMC_CMD_SEND_OP_COND:
    case SDMMC_CMD_SWITCH:
        return true;
        break;

    default:
        return false;
        break;
    }
}

static bool _sdmmcCheckStatus(drvSdmmc_t *d, sdmmcCheckStatus_t check)
{
    REG_SDMMC_SDMMC_STATUS_T sdmmc_status = (REG_SDMMC_SDMMC_STATUS_T)d->hwp->sdmmc_status;
    uint32_t check_mask_value;
    bool ret = true;
    switch (check)
    {
    case SDMMC_TRANSFER_DONE:
        check_mask_value = sdmmc_status.b.not_sdmmc_over;
        if (check_mask_value)
            ret = false;
        break;
    case SDMMC_RESPONSE_OK:
        check_mask_value = sdmmc_status.v & (0x03 << 8);
        if (check_mask_value)
            ret = false;
        break;
    case SDMMC_CRC_STATUS:
        check_mask_value = sdmmc_status.b.crc_status;
        if (check_mask_value != 0x02)
            ret = false;
        break;
    case SDMMC_DATA_ERROR:
        check_mask_value = sdmmc_status.b.data_error;
        if (check_mask_value)
            ret = false;
        break;
    default:
        ret = false;
        OSI_LOGE(0, "_sdmmcCheckStatus: get error arg");
        break;
    }
    return ret;
}
static void _sdmmcSetClk(drvSdmmc_t *d, uint32_t clock)
{
    REG_SYS_CTRL_CLK_SYS_AXI_ENABLE_T clk_sys_axi_enable;

    REG_FIELD_WRITE1(hwp_sysCtrl->clk_sys_axi_enable, clk_sys_axi_enable,
                     enable_sys_axi_clk_id_sdmmc1, 1);

    if (clock > d->max_clk)
        clock = d->max_clk;

    uint32_t div = ((CONFIG_DEFAULT_SYSAXI_FREQ + (2 * clock - 1)) / (2 * clock)) - 1;

    OSI_LOGI(0, "sdmmc: Set SDMMC clock DIV = (%d+(2*%d-1) / (2*%d) - 1 = %d\n", CONFIG_DEFAULT_SYSAXI_FREQ, clock, clock, div);
    if (div > 0xff)
        div = 0xff;
    d->hwp->sdmmc_trans_speed_reg = div & 0xff;

    d->hwp->sdmmc_mclk_adjust_reg = (d->clk_adj & 0x0f) | (d->clk_inv << 4);
}

static void _sdmmcGetResp(drvSdmmc_t *d, sdmmcCmd_t cmd, uint32_t *arg)
{
    // TODO Check in the spec for all the commands response types
    switch (cmd)
    {
    // If they require a response, it is cargoed
    // with a 32 bit argument.
    case SDMMC_CMD_SEND_RELATIVE_ADDR:
    case SDMMC_CMD_SEND_IF_COND:
    case SDMMC_CMD_SELECT_CARD:
    case SDMMC_CMD_SEND_STATUS:
    case SDMMC_CMD_SET_BLOCKLEN:
    case SDMMC_CMD_READ_SINGLE_BLOCK:
    case SDMMC_CMD_READ_MULT_BLOCK:
    case SDMMC_CMD_WRITE_SINGLE_BLOCK:
    case SDMMC_CMD_WRITE_MULT_BLOCK:
    case SDMMC_CMD_APP_CMD:
    case SDMMC_CMD_SET_BUS_WIDTH:
    case SDMMC_CMD_SEND_NUM_WR_BLOCKS:
    case SDMMC_CMD_SET_WR_BLK_COUNT:
    case SDMMC_CMD_MMC_SEND_OP_COND:
    case SDMMC_CMD_SEND_OP_COND:
    case SDMMC_CMD_SWITCH:
        arg[0] = d->hwp->sdmmc_resp_arg3;
        arg[1] = 0;
        arg[2] = 0;
        arg[3] = 0;
        break;

    // Those response arguments are 128 bits
    case SDMMC_CMD_ALL_SEND_CID:
    case SDMMC_CMD_SEND_CSD:
        arg[0] = d->hwp->sdmmc_resp_arg0;
        arg[1] = d->hwp->sdmmc_resp_arg1;
        arg[2] = d->hwp->sdmmc_resp_arg2;
        arg[3] = d->hwp->sdmmc_resp_arg3;
        break;

    default:
        break;
    }
}

static void _sdmmcSetDataWidth(drvSdmmc_t *d)
{
    switch (d->bus_width)
    {
    case SDMMC_DATA_BUS_WIDTH_1:
        d->hwp->sdmmc_data_width_reg = 1;
        break;

    case SDMMC_DATA_BUS_WIDTH_4:
        d->hwp->sdmmc_data_width_reg = 4;
        break;

    default:
        break;
    }
}

static void _sdmmcIfcTransfer(drvSdmmc_t *d, sdmmcDirection_t direction)
{
    uint32_t xfersize = 0;

    //OSI_LOGI("IFC size=%d count num=%d\n", lengthExp, d->block_num);

    /* Configure amount of data */
    d->hwp->sdmmc_block_cnt_reg = d->block_num & 0xFFFF;

    /* Configure Bytes reordering */
    REG_SDMMC_APBI_CTRL_SDMMC_T apbi_ctrl_sdmmc = {};
    apbi_ctrl_sdmmc.b.soft_rst_l = 1;
    apbi_ctrl_sdmmc.b.l_endian = 1;
    d->hwp->apbi_ctrl_sdmmc = apbi_ctrl_sdmmc.v;

    xfersize = d->block_num * d->block_size;
    if (direction == SDMMC_DIRECTION_READ)
    {
        uint32_t critical = osiEnterCritical();

        drvIfcFlush(&d->rx_ifc);
        osiDCacheInvalidate(d->sys_mem_addr, xfersize);
        drvIfcStart(&d->rx_ifc, d->sys_mem_addr, xfersize);

        osiExitCritical(critical);
    }
    else
    {
        uint32_t critical = osiEnterCritical();

        //drvIfcFlush(&d->tx_ifc);
        osiDCacheClean(d->sys_mem_addr, xfersize);
        drvIfcStart(&d->tx_ifc, d->sys_mem_addr, xfersize);

        osiExitCritical(critical);
    }
}

static void _sdmmcStopTransfer(drvSdmmc_t *d, sdmmcDirection_t direction)
{
    /* Configure Bytes reordering */
    REG_SDMMC_APBI_CTRL_SDMMC_T apbi_ctrl_sdmmc = {};
    apbi_ctrl_sdmmc.b.soft_rst_l = 0;
    apbi_ctrl_sdmmc.b.l_endian = 1;
    d->hwp->apbi_ctrl_sdmmc = apbi_ctrl_sdmmc.v;

    if (direction == SDMMC_DIRECTION_READ)
    {
        drvIfcFlush(&d->rx_ifc);
        OSI_LOOP_WAIT(drvIfcIsFifoEmpty(&d->rx_ifc));
    }
    else
    {
        OSI_LOOP_WAIT(drvIfcIsFifoEmpty(&d->tx_ifc));
    }
}

// Wait for a command to be done or timeouts
static bool _sdmmcWaitCmdOver(drvSdmmc_t *d)
{
    uint32_t startTime = osiUpTimeUS();
    uint32_t useTime;

    while (!_sdmmcCheckStatus(d, SDMMC_TRANSFER_DONE))
    {
        useTime = (uint32_t)(osiUpTimeUS() - startTime);
        if (useTime > SDMMC_CMD_TIMEOUT)
        {
            OSI_LOGI(0, "sdmmc: _sdmmcWaitCmdOver timeout\n");
            return false;
        }
    }

    return true;
}

static bool _sdmmcWaitResp(drvSdmmc_t *d)
{
    uint32_t startTime = osiUpTimeUS();
    uint32_t rsp_time;

    while (!_sdmmcCheckStatus(d, SDMMC_RESPONSE_OK))
    {
        rsp_time = (uint32_t)(osiUpTimeUS() - startTime);
        if (rsp_time > SDMMC_RESP_TIMEOUT)
        {
            OSI_LOGI(0, "sdmmc:Response error (no response or crc error?)\n");
            return false;
        }
    }

    return true;
}

static bool _sdmmcCheckResponse(drvSdmmc_t *d, sdmmcCmd_t cmd, uint32_t *resp)
{
    sdmmcCardStatus_t card_status = (sdmmcCardStatus_t)resp[0];
    bool ret = true;
    sdmmcCardStatus_t expectedCardStatus;
    sdmmcCardState_t state;
    switch (cmd)
    {
    case SDMMC_CMD_APP_CMD:
        if (!(card_status.b.readyForData) || !(card_status.b.appCmd))
            ret = false;
        break;
    case SDMMC_CMD_SET_BLOCKLEN:
    case SDMMC_CMD_SEND_STATUS:
    case SDMMC_CMD_READ_SINGLE_BLOCK:
    case SDMMC_CMD_READ_MULT_BLOCK:
        expectedCardStatus.v = 0;
        expectedCardStatus.b.readyForData = 1;
        expectedCardStatus.b.currentState = SDMMC_CARD_STATE_TRAN;
        if (card_status.v != expectedCardStatus.v)
            ret = false;
        break;
    case SDMMC_CMD_WRITE_SINGLE_BLOCK:
    case SDMMC_CMD_WRITE_MULT_BLOCK:
        state = card_status.b.currentState;
        if ((card_status.v & 0xffff0000) != 0 || (state != SDMMC_CARD_STATE_TRAN && state != SDMMC_CARD_STATE_RCV && state != SDMMC_CARD_STATE_PRG))
            ret = false;
        break;
    case SDMMC_CMD_SELECT_CARD:
        if (!(card_status.b.readyForData))
            ret = false;
        break;
    case SDMMC_CMD_SET_BUS_WIDTH:
    case SDMMC_CMD_SET_WR_BLK_COUNT:
        expectedCardStatus.v = 0;
        expectedCardStatus.b.readyForData = 1;
        expectedCardStatus.b.appCmd = 1;
        expectedCardStatus.b.currentState = SDMMC_CARD_STATE_TRAN;
        if (card_status.v != expectedCardStatus.v)
            ret = false;
        break;
    case SDMMC_CMD_SEND_RELATIVE_ADDR:
        d->rca_reg = resp[0] & 0xffff0000;
        break;
    case SDMMC_CMD_SEND_IF_COND:
        if ((resp[0] & 0xff) != SDMMC_CMD8_CHECK_PATTERN)
            ret = false;
        break;
    case SDMMC_CMD_SEND_OP_COND:
        if (!(card_status.b.appCmd))
            ret = false;
        break;
    default:
        break;
    }

    return ret;
}
static bool drvSdmmcSendCmd(drvSdmmc_t *d, sdmmcCmd_t cmd, uint32_t arg,
                            uint32_t *resp)
{
    bool err_status = true;
    if (cmd & SDMMC_ACMD_SEL)
    {
        // This is an application specific command,
        // we send the CMD55 first, response r1
        _sdmmcSendCmd(d, SDMMC_CMD_APP_CMD, d->rca_reg);

        // Wait for command over
        if (!_sdmmcWaitCmdOver(d))
        {
            OSI_LOGE(0, "cmd55 timeout\n");
            return false;
        }

        if (!_sdmmcWaitResp(d))
        {
            OSI_LOGE(0, "cmd55 wait response status err\n");
            return false;
        }

        // Fetch response
        _sdmmcGetResp(d, SDMMC_CMD_APP_CMD, resp);
        OSI_LOGD(0, "cmd55 Response=%08x, %08x, %08x, %08x\n", resp[0], resp[1], resp[2], resp[3]);

        err_status = _sdmmcCheckResponse(d, SDMMC_CMD_APP_CMD, resp);
        if (!err_status)
        {
            OSI_LOGE(0, "CMD55 status Fail\n");
            return false;
        }
    }

    // Send proper command. If it was an ACMD, the CMD55 have just been sent.
    _sdmmcSendCmd(d, cmd, arg);

    // Wait for command to be sent
    if (!_sdmmcWaitCmdOver(d))
    {
        OSI_LOGE(0, "sdmmc: CMD %d Sending Timed out", cmd & 0x3f);
        return false;
    }
    // Wait for response and get its argument
    if (_sdmmcNeedResponse(cmd))
    {
        if (!_sdmmcWaitResp(d))
        {
            OSI_LOGE(0, "sdmmc: cmd %d Response err\n", cmd & 0x3f);
            return false;
        }
        // Fetch response
        _sdmmcGetResp(d, cmd, resp);
        OSI_LOGD(0, "cmd:%d Response=%08x, %08x, %08x, %08x\n", cmd & 0x3f, resp[0], resp[1], resp[2], resp[3]);
    }

    return true;
}

static void drvSdmmcInitCsd(drvSdmmc_t *d, uint32_t *csdRaw)
{
    // CF SD spec version2, CSD version 1 ?
    d->csd.csdStructure = (uint8_t)((csdRaw[3] & (0x3 << 30)) >> 30);

    // Byte 47 to 75 are different depending on the version
    // of the CSD srtucture.
    d->csd.specVers = (uint8_t)((csdRaw[3] & (0xf < 26)) >> 26);
    d->csd.taac = (uint8_t)((csdRaw[3] & (0xff << 16)) >> 16);
    d->csd.nsac = (uint8_t)((csdRaw[3] & (0xff << 8)) >> 8);
    d->csd.tranSpeed = (uint8_t)(csdRaw[3] & 0xff);

    d->csd.ccc = (csdRaw[2] & (0xfff << 20)) >> 20;
    d->csd.readBlLen = (uint8_t)((csdRaw[2] & (0xf << 16)) >> 16);
    d->csd.readBlPartial = (uint8_t)((csdRaw[2] & (0x1 << 15)) >> 15);
    d->csd.writeBlkMisalign = (uint8_t)((csdRaw[2] & (0x1 << 14)) >> 14);
    d->csd.readBlkMisalign = (uint8_t)((csdRaw[2] & (0x1 << 13)) >> 13);
    d->csd.dsrImp = (uint8_t)((csdRaw[2] & (0x1 << 12)) >> 12);

    if (d->csd.csdStructure == SDMMC_CSD_VERSION_1)
    {
        d->csd.cSize = (csdRaw[2] & 0x3ff) << 2;

        d->csd.cSize = d->csd.cSize | ((csdRaw[1] & (0x3 << 30)) >> 30);
        d->csd.vddRCurrMin = (uint8_t)((csdRaw[1] & (0x7 << 27)) >> 27);
        d->csd.vddRCurrMax = (uint8_t)((csdRaw[1] & (0x7 << 24)) >> 24);
        d->csd.vddWCurrMin = (uint8_t)((csdRaw[1] & (0x7 << 21)) >> 21);
        d->csd.vddWCurrMax = (uint8_t)((csdRaw[1] & (0x7 << 18)) >> 18);
        d->csd.cSizeMult = (uint8_t)((csdRaw[1] & (0x7 << 15)) >> 15);

        // Block number: cf Spec Version 2 page 103 (116).
        d->csd.blockNumber = (d->csd.cSize + 1) << (d->csd.cSizeMult + 2);
    }
    else
    {
        // d->csd.csdStructure == SDMMC_CSD_VERSION_2
        d->csd.cSize = ((csdRaw[2] & 0x3f)) | ((csdRaw[1] & (0xffff << 16)) >> 16);

        // Other fields are undefined --> zeroed
        d->csd.vddRCurrMin = 0;
        d->csd.vddRCurrMax = 0;
        d->csd.vddWCurrMin = 0;
        d->csd.vddWCurrMax = 0;
        d->csd.cSizeMult = 0;

        // Block number: cf Spec Version 2 page 109 (122).
        d->csd.blockNumber = (d->csd.cSize + 1) * 1024;
    }

    d->csd.eraseBlkEnable = (uint8_t)((csdRaw[1] & (0x1 << 14)) >> 14);
    d->csd.sectorSize = (uint8_t)((csdRaw[1] & (0x7f << 7)) >> 7);
    d->csd.wpGrpSize = (uint8_t)(csdRaw[1] & 0x7f);

    d->csd.wpGrpEnable = (uint8_t)((csdRaw[0] & (0x1 << 31)) >> 31);
    d->csd.r2wFactor = (uint8_t)((csdRaw[0] & (0x7 << 26)) >> 26);
    d->csd.writeBlLen = (uint8_t)((csdRaw[0] & (0xf << 22)) >> 22);
    d->csd.writeBlPartial = (uint8_t)((csdRaw[0] & (0x1 << 21)) >> 21);
    d->csd.fileFormatGrp = (uint8_t)((csdRaw[0] & (0x1 << 15)) >> 15);
    d->csd.copy = (uint8_t)((csdRaw[0] & (0x1 << 14)) >> 14);
    d->csd.permWriteProtect = (uint8_t)((csdRaw[0] & (0x1 << 13)) >> 13);
    d->csd.tmpWriteProtect = (uint8_t)((csdRaw[0] & (0x1 << 12)) >> 12);
    d->csd.fileFormat = (uint8_t)((csdRaw[0] & (0x3 << 10)) >> 10);
    d->csd.crc = (uint8_t)((csdRaw[0] & (0x7f << 1)) >> 1);

    if (d->csd.blockNumber < 300000) //51200
    {
        if (d->card_is_sdhc == false)
        {
            OSI_LOGI(0, "sdmmc: DRV %4c is small card\n", d->name);
        }
    }
}

static void drvSdmmcPrintCSD(drvSdmmc_t *d, bool print_enable)
{
    if (print_enable)
    {
        OSI_LOGI(0, "CSD:csdStructure = %d\n", d->csd.csdStructure);
        OSI_LOGI(0, "CSD:specVers     = %d\n", d->csd.specVers);
        OSI_LOGI(0, "CSD:taac         = %d\n", d->csd.taac);
        OSI_LOGI(0, "CSD:nsac         = %d\n", d->csd.nsac);
        OSI_LOGI(0, "CSD:tranSpeed    = %d\n", d->csd.tranSpeed);
        OSI_LOGI(0, "CSD:ccc          = %d\n", d->csd.ccc);
        OSI_LOGI(0, "CSD:readBlLen    = %d\n", d->csd.readBlLen);
        OSI_LOGI(0, "CSD:readBlPartial = %d\n", d->csd.readBlPartial);
        OSI_LOGI(0, "CSD:writeBlkMisalign = %d\n", d->csd.writeBlkMisalign);
        OSI_LOGI(0, "CSD:readBlkMisalign  = %d\n", d->csd.readBlkMisalign);
        OSI_LOGI(0, "CSD:dsrImp       = %d\n", d->csd.dsrImp);
        OSI_LOGI(0, "CSD:cSize        = %d\n", d->csd.cSize);
        OSI_LOGI(0, "CSD:vddRCurrMin  = %d\n", d->csd.vddRCurrMin);
        OSI_LOGI(0, "CSD:vddRCurrMax  = %d\n", d->csd.vddRCurrMax);
        OSI_LOGI(0, "CSD:vddWCurrMin  = %d\n", d->csd.vddWCurrMin);
        OSI_LOGI(0, "CSD:vddWCurrMax  = %d\n", d->csd.vddWCurrMax);
        OSI_LOGI(0, "CSD:cSizeMult    = %d\n", d->csd.cSizeMult);
        OSI_LOGI(0, "CSD:eraseBlkEnable = %d\n", d->csd.eraseBlkEnable);
        OSI_LOGI(0, "CSD:sectorSize   = %d\n", d->csd.sectorSize);
        OSI_LOGI(0, "CSD:wpGrpSize    = %d\n", d->csd.wpGrpSize);
        OSI_LOGI(0, "CSD:wpGrpEnable  = %d\n", d->csd.wpGrpEnable);
        OSI_LOGI(0, "CSD:r2wFactor    = %d\n", d->csd.r2wFactor);
        OSI_LOGI(0, "CSD:writeBlLen   = %d\n", d->csd.writeBlLen);

        OSI_LOGI(0, "CSD:writeBlPartial = %d\n", d->csd.writeBlPartial);
        OSI_LOGI(0, "CSD:fileFormatGrp = %d\n", d->csd.fileFormatGrp);
        OSI_LOGI(0, "CSD:copy  = %d\n", d->csd.copy);
        OSI_LOGI(0, "CSD:permWriteProtect = %d\n", d->csd.permWriteProtect);
        OSI_LOGI(0, "CSD:tmpWriteProtect  = %d\n", d->csd.tmpWriteProtect);
        OSI_LOGI(0, "CSD:fileFormat       = %d\n", d->csd.fileFormat);
        OSI_LOGI(0, "CSD:crc              = %d\n", d->csd.crc);
        OSI_LOGI(0, "CSD:block number     = %d\n", d->csd.blockNumber);
    }
}

static bool drvSdmmcReadCsd(drvSdmmc_t *d)
{
    bool err_status = true;
    uint32_t response[4];

    // Get card CSD
    err_status = drvSdmmcSendCmd(d, SDMMC_CMD_SEND_CSD, d->rca_reg, response);
    if (err_status)
    {
        drvSdmmcInitCsd(d, response);
        drvSdmmcPrintCSD(d, false);
    }
    else
    {
        OSI_LOGE(0, "sdmmc: drvSdmmcReadCsd CMD9 err\n");
        return false;
    }

    d->block_size = (1 << d->csd.readBlLen);
    d->hwp->sdmmc_block_size_reg = d->csd.readBlLen;
    if (d->block_size > 512)
    {
        d->block_size = 512;
        d->hwp->sdmmc_block_size_reg = 0x9;
    }

    d->block_num_total = d->csd.blockNumber * ((1 << d->csd.readBlLen) / d->block_size);
    OSI_LOGI(0, "sdmmc: sdmmc:  card total block number = %d\n", d->block_num_total);

    return true;
}
static void _sdmmcSuspend(void *ctx, osiSuspendMode_t mode)
{
}

static void prvSdmmcISR(void *param)
{
    drvSdmmc_t *d = (drvSdmmc_t *)param;
    REG_SDMMC_SDMMC_INT_STATUS_T cause = {d->hwp->sdmmc_int_status};
    //OSI_LOGI(0, "hfl_test:prvSdmmcISR--cause:0x%x--1\n",cause.v);
    if (cause.b.txdma_done_int && cause.b.dat_over_int)
    {
        d->hwp->sdmmc_int_clear = (1 << 5) | (1 << 4);
        osiSemaphoreRelease(d->tx_done_sema);
    }
    if (cause.b.rxdma_done_int && cause.b.dat_over_int)
    {
        d->hwp->sdmmc_int_clear = (1 << 6) | (1 << 4);
        osiSemaphoreRelease(d->rx_done_sema);
    }
}

static void _sdmmcResume(void *ctx, osiSuspendMode_t mode, uint32_t source)
{
}
static const osiPmSourceOps_t _sdmmcPmOps = {
    .suspend = _sdmmcSuspend,
    .resume = _sdmmcResume,
};

void drvSdmmcIfcInit(drvSdmmc_t *d)
{

    drvIfcChannelInit(&d->rx_ifc, d->name, DRV_IFC_RX);
    drvIfcChannelInit(&d->tx_ifc, d->name, DRV_IFC_TX);

    d->rx_enabled = true;
    d->tx_enabled = true;

    drvIfcRequestChannel(&d->tx_ifc);
    drvIfcRequestChannel(&d->rx_ifc);
}
drvSdmmc_t *drvSdmmcCreate(uint32_t name)
{
    drvSdmmc_t *d = (drvSdmmc_t *)malloc(sizeof(drvSdmmc_t));
    if (d == NULL)
        return NULL;

    if (name == DRV_NAME_SDMMC1)
    {
        d->hwp = hwp_sdmmc;
        d->irqn = HAL_SYSIRQ_NUM(SYS_IRQ_ID_SDMMC1);
        osiIrqSetPriority(d->irqn, SYS_IRQ_PRIO_SDMMC1);
    }
    else if (name == DRV_NAME_SDMMC2)
    {
        d->hwp = hwp_sdmmc2;
        d->irqn = HAL_SYSIRQ_NUM(SYS_IRQ_ID_SDMMC2);
        osiIrqSetPriority(d->irqn, SYS_IRQ_PRIO_SDMMC2);
    }
    else
    {
        OSI_LOGE(0, "Failed to drvSdmmcCreate, invalid name\n");
        free(d);
        return NULL;
    }
    d->name = name;
    d->sdmmc_ver = SDMMC_CARD_V2;
    d->sdmmc_ocr_reg = 0x00ff8000;
    d->card_is_sdhc = false;
    d->dsr = 0x04040000;
    d->max_clk = 50000000;
    d->master_clk = 25000000;
    d->bus_width = 1;
    d->rca_reg = 0;
    d->clk_adj = 0x01;
    d->clk_inv = 0x01;

    d->hwp->sdmmc_int_mask = 0x0;
    d->pm_source = osiPmSourceCreate(name, &_sdmmcPmOps, d);
    d->lock = osiMutexCreate();
    drvSdmmcIfcInit(d);

    osiIrqSetHandler(d->irqn, prvSdmmcISR, d);
    osiIrqEnable(d->irqn);

    d->tx_done_sema = osiSemaphoreCreate(1, 0);
    if (d->tx_done_sema == NULL)
        goto failed;

    d->rx_done_sema = osiSemaphoreCreate(1, 0);
    if (d->rx_done_sema == NULL)
        goto failed;

    return d;
failed:
    drvSdmmcDestroy(d);
    return NULL;
}

static void _drvRstSdmmcVolt(drvSdmmc_t *d)
{
    REG_SYS_CTRL_AP_APB_RST_SET_T ap_apb_rst_set;
    REG_SYS_CTRL_AP_APB_RST_CLR_T ap_apb_rst_clr;
    REG_SYS_CTRL_CLK_SYS_AXI_ENABLE_T clk_sys_axi_enable;

    hwp_sysCtrl->reg_dbg = SYS_CTRL_PROTECT_UNLOCK;

    REG_FIELD_WRITE1(hwp_sysCtrl->ap_apb_rst_set, ap_apb_rst_set,
                     set_ap_apb_rst_id_sdmmc1, 1);

    osiDelayUS(100);
    REG_FIELD_WRITE1(hwp_sysCtrl->ap_apb_rst_clr, ap_apb_rst_clr,
                     clr_ap_apb_rst_id_sdmmc1, 1);

    osiDelayUS(100);

    REG_FIELD_WRITE1(hwp_sysCtrl->clk_sys_axi_enable, clk_sys_axi_enable,
                     enable_sys_axi_clk_id_sdmmc1, 0);

    hwp_sysCtrl->reg_dbg = SYS_CTRL_PROTECT_LOCK;

    halPmuSwitchPower(HAL_POWER_SD, false, false);
    osiThreadSleep(200);

    halPmuSwitchPower(HAL_POWER_SD, true, true);
    osiDelayUS(100);
}

static void _drvConfigSdmmcIO(drvSdmmc_t *d)
{
    REG_ANALOG_REG_PAD_SDMMC1_OTHERS_CFG_T clk;
    REG_ANALOG_REG_PAD_SDMMC1_DATA_CFG_T data;
    clk.v = hwp_analogReg->pad_sdmmc1_others_cfg;
    data.v = hwp_analogReg->pad_sdmmc1_data_cfg;
    clk.b.pad_sdmmc1_clk_drv = 5;
    clk.b.pad_sdmmc1_clk_ie = 0;
    clk.b.pad_sdmmc1_cmd_drv = 5;
    clk.b.pad_sdmmc1_cmd_spu = 1;
    hwp_analogReg->pad_sdmmc1_others_cfg = clk.v;
    data.b.pad_sdmmc1_data_0_drv = 5;
    data.b.pad_sdmmc1_data_1_drv = 5;
    data.b.pad_sdmmc1_data_2_drv = 5;
    data.b.pad_sdmmc1_data_3_drv = 5;

    data.b.pad_sdmmc1_data_0_spu = 1;

    data.b.pad_sdmmc1_data_1_spu = 1;

    data.b.pad_sdmmc1_data_2_spu = 1;

    data.b.pad_sdmmc1_data_3_spu = 1;
    hwp_analogReg->pad_sdmmc1_data_cfg = data.v;
}

bool drvSdmmcOpen(drvSdmmc_t *d)
{
    bool err_status = true;
    uint32_t response[4] = {0, 0, 0, 0};
    uint32_t startTime = 0;
    OSI_LOGI(0, "sdmmc: DRV %4c Enter drvSdmmcOpen", d->name);
    _drvConfigSdmmcIO(d);
    _drvRstSdmmcVolt(d);
    /* Set mclk output <= 400KHz */
    _sdmmcSetClk(d, 400000);

    osiDelayUS(1000);

    REG_SDMMC_SDMMC_INT_MASK_T irq_mask = {
        .b.txdma_done_mk = 1,
        .b.rxdma_done_mk = 1,
        .b.dat_over_mk = 1,
    };
    d->hwp->sdmmc_int_mask = irq_mask.v;

    // Send Power On command
    drvSdmmcSendCmd(d, SDMMC_CMD_GO_IDLE_STATE, 0, response);

    // Check if the card is a spec vers.2 one, response is r7
    err_status = drvSdmmcSendCmd(d, SDMMC_CMD_SEND_IF_COND, (SDMMC_CMD8_VOLTAGE_SEL | SDMMC_CMD8_CHECK_PATTERN), response);

    if (!err_status)
    {
        OSI_LOGI(0, "sdmmc: CMD8 send err\n");
    }
    else
    {
        OSI_LOGI(0, "sdmmc: CMD8 Done (support spec v2.0) response=0x%8x\n", response[0]);
        err_status = _sdmmcCheckResponse(d, SDMMC_CMD_SEND_IF_COND, response);
        if (!err_status)
        {
            OSI_LOGE(0, "sdmmc CMD8 status Fail\n");
            return false;
        }

        OSI_LOGI(0, "sdmmc: CMD8, Supported\n");
        d->sdmmc_ocr_reg |= SDMMC_OCR_HCS; //ccs = 1
    }
    osiThreadSleep(35);
    // TODO HCS mask bit to ACMD 41 if high capacity
    // Send OCR, as long as the card return busy
    startTime = osiUpTimeUS();
    while (1)
    {
        if (osiUpTimeUS() - startTime > SDMMC_SDMMC_OCR_TIMEOUT)
        {
            OSI_LOGI(0, "sdmmc: ACMD41, Retry Timeout\n");
            return false;
        }

        //OSI_LOGI(0, "ACMD41, response r3\n");
        err_status = drvSdmmcSendCmd(d, SDMMC_CMD_SEND_OP_COND, d->sdmmc_ocr_reg, response);
        if (!err_status)
        {
            OSI_LOGI(0, "sdmmc: ACMD41, No Response, NO Card?\n");
            return false;
        }
        // Bit 31 is power up busy status bit(pdf spec p. 109)
        d->sdmmc_ocr_reg = (response[0] & 0x7fffffff);
        // Power up busy bit is set to 1,
        if (response[0] & 0x80000000)
        {
            OSI_LOGI(0, "sdmmc: ACMD41 Done, response = 0x%8x\n", response[0]);
            //OSI_LOGI(0,"ACMD41, PowerUp done\n");

            // Card is V2: check for high capacity
            if (response[0] & SDMMC_OCR_CCS_MASK)
            {
                d->card_is_sdhc = true;
                OSI_LOGI(0, "sdmmc: Card is SDHC\n");
            }
            else
            {
                d->card_is_sdhc = false;
                OSI_LOGI(0, "sdmmc: Card is V2, but NOT SDHC\n");
            }

            OSI_LOGI(0, "sdmmc: Inserted Card is a SD card\n");
            break;
        }
    }

    // Get the CID of the card.
    err_status = drvSdmmcSendCmd(d, SDMMC_CMD_ALL_SEND_CID, 0, response);
    if (!err_status)
    {
        OSI_LOGE(0, "CMD2 Fail\n");
        return false;
    }

    err_status = drvSdmmcSendCmd(d, SDMMC_CMD_SEND_RELATIVE_ADDR, d->rca_reg, response);
    if (!err_status)
    {
        OSI_LOGE(0, "CMD3 Fail\n");
        return false;
    }
    else
    {
        d->rca_reg = response[0] & 0xffff0000;
        OSI_LOGD(0, "CMD3 Done, response=0x%8x, rca=0x%6x\n",
                 response[0], d->rca_reg);
    }

    err_status = drvSdmmcReadCsd(d);
    if (!err_status)
    {
        OSI_LOGE(0, "Because Get CSD, Initialize Failed\n");
        return false;
    }

    // Select the card
    err_status = drvSdmmcSendCmd(d, SDMMC_CMD_SELECT_CARD, d->rca_reg, response);
    if (!err_status)
    {
        OSI_LOGE(0, "CMD7 Fail\n");
        return false;
    }
    err_status = _sdmmcCheckResponse(d, SDMMC_CMD_SELECT_CARD, response);
    if (!err_status)
    {
        OSI_LOGE(0, "sdmmc CMD 7 status Fail\n");
        return false;
    }

    // Set the bus width (use 4 data lines for SD cards only)
    d->bus_width = SDMMC_CARD_V1 == d->sdmmc_ver ? SDMMC_DATA_BUS_WIDTH_1 : SDMMC_DATA_BUS_WIDTH_4;

    err_status = drvSdmmcSendCmd(d, SDMMC_CMD_SET_BUS_WIDTH, d->bus_width, response);
    if (!err_status)
    {
        OSI_LOGE(0, "sdmmc:ACMD6, SET_BUS_WIDTH fail 1\n");
        return false;
    }
    err_status = _sdmmcCheckResponse(d, SDMMC_CMD_SET_BUS_WIDTH, response);
    if (!err_status)
    {
        OSI_LOGE(0, "sdmmc:ACMD6, SET_BUS_WIDTH fail 2\n");
        return false;
    }

    _sdmmcSetDataWidth(d);
    OSI_LOGD(0, "CMD16 - set block len g_mcdBlockLen=%d\n", d->block_size);

    // Configure the block lenght
    err_status = drvSdmmcSendCmd(d, SDMMC_CMD_SET_BLOCKLEN, d->block_size, response);
    if (!err_status)
    {
        OSI_LOGE(0, "sdmmc:CMD16, SET_BLOCKLEN fail\n");
        return false;
    }
    err_status = _sdmmcCheckResponse(d, SDMMC_CMD_SET_BLOCKLEN, response);
    if (!err_status)
    {
        OSI_LOGE(0, "sdmmc:CMD16, SET_BLOCKLEN fail 2\n");
        return false;
    }

    /* For cards, fpp = 25MHz for sd, fpp = 50MHz, for sdhc */
    _sdmmcSetClk(d, d->master_clk);
    osiDelayUS(100);

    OSI_LOGI(0, "DRV %4c open Done\n", d->name);

    return true;
}

static bool drvSdmmcTranState(drvSdmmc_t *d, uint32_t iter)
{
    uint32_t response[4] = {0, 0, 0, 0};
    bool err_status = true;
    uint32_t startTime = osiUpTimeUS();

    while (1)
    {
        // OSI_LOGI(0, "CMD13, Set Trans State\n");

        err_status = drvSdmmcSendCmd(d, SDMMC_CMD_SEND_STATUS, d->rca_reg, response);
        if (!err_status)
        {
            OSI_LOGE(0, "CMD13, send Fail\n");
            // error while sending the command
            return false;
        }
        err_status = _sdmmcCheckResponse(d, SDMMC_CMD_SEND_STATUS, response);
        if (err_status)
        {
            OSI_LOGD(0, "sdmmc:CMD13, get status ok\n");
            return true;
        }
        // try again
        if (osiUpTimeUS() - startTime > iter)
        {
            OSI_LOGE(0, "CMD13, Timeout\n");
            return false;
        }
    }
}

static bool drvSdmmcMultBlockRead(drvSdmmc_t *d, uint32_t block_address, uint8_t *pRead, uint32_t block_num)
{
    uint32_t cardResponse[4] = {0, 0, 0, 0};
    bool err_status = true;
    sdmmcCmd_t readCmd;

    d->sys_mem_addr = (uint8_t *)pRead;
    d->sdmmc_addr = (uint8_t *)block_address;
    d->block_num = block_num;

    osiSemaphoreTryAcquire(d->rx_done_sema, 0);

    // Command are different for reading one or several blocks of data
    if (block_num == 1)
        readCmd = SDMMC_CMD_READ_SINGLE_BLOCK;
    else
        readCmd = SDMMC_CMD_READ_MULT_BLOCK;
    // Initiate data migration through Ifc.
    _sdmmcIfcTransfer(d, SDMMC_DIRECTION_READ);

    //OSI_LOGI("[MultBlockRead] block_address=%d, buffer=0x%x, block_num=%d\n",
    //  block_address, pRead, block_num);

    // Initiate data migration of multiple blocks through SD bus.
    err_status = drvSdmmcSendCmd(d, readCmd, (uint32_t)block_address, cardResponse);
    if (!err_status)
    {
        OSI_LOGE(0, "send read command err\n");
        goto readfail;
    }

    err_status = _sdmmcCheckResponse(d, readCmd, cardResponse);
    if (!err_status)
    {
        OSI_LOGE(0, "sdmmc:read cmd response status error:0x%x\n", cardResponse[0]);
        goto readfail;
    }

    bool sent = osiSemaphoreTryAcquire(d->rx_done_sema, SDMMC_READ_TIMEOUT);
    if (!sent)
    {
        OSI_LOGE(0, "READ timeout\n");
        // Abort the transfert.
        goto readfail;
    }
    if (!_sdmmcCheckStatus(d, SDMMC_DATA_ERROR))
    {
        OSI_LOGE(0, "sdmmc: read block Bad CRC\n");
        goto readfail;
    }

    // Check that the card is in tran (Transmission) state.
    if (true != drvSdmmcTranState(d, SDMMC_TRAN_TIMEOUT))
    {
        OSI_LOGE(0, "trans state timeout\n");
        goto readfail;
    }

    return true;
readfail:
    _sdmmcStopTransfer(d, SDMMC_DIRECTION_READ);
    return false;
}

bool drvSdmmcRead(drvSdmmc_t *d, uint32_t block_number, void *buffer, uint32_t size)
{
    uint32_t tmpAddress;
    int _start;
    bool ret = true;
    OSI_LOGD(0, "sdmmc: drvSdmmcRead Enter block_number=%d buf=0x%x size=%d \n", block_number, buffer, size);
    _start = osiUpTimeUS();
    osiMutexLock(d->lock);
    /*Addresses are block number for high capacity card , when the capacity is larger than 2GB, sector access mode, or byte access mode*/
    if (d->card_is_sdhc)
    {
        tmpAddress = block_number;
    }
    else
    {
        tmpAddress = (block_number * d->block_size);
    }

    ret = drvSdmmcMultBlockRead(d, tmpAddress, buffer, size / d->block_size);

    if (ret)
        OSI_LOGD(0, "sdmmc: drvSdmmcRead finished , %dus \n", (uint32_t)(osiUpTimeUS() - _start));
    else
        OSI_LOGI(0, "sdmmc: drvSdmmcRead finish failed \n");

    osiMutexUnlock(d->lock);
    return ret;
}

static bool drvSdmmcMultBlockWrite(drvSdmmc_t *d, uint32_t block_address, const uint8_t *pWrite, uint32_t block_num)
{
    uint32_t cardResponse[4] = {0, 0, 0, 0};
    bool err_status = true;

    osiSemaphoreTryAcquire(d->tx_done_sema, 0);

    sdmmcCmd_t writeCmd;

    d->sys_mem_addr = (uint8_t *)pWrite;
    d->sdmmc_addr = (uint8_t *)block_address;
    d->block_num = block_num;
    d->direction = SDMMC_DIRECTION_WRITE;

    // Check buffer.
    if (pWrite == NULL)
    {
        OSI_LOGW(0, "SDMMC write: Buffer is NULL\n");
    }
    if (((uint32_t)pWrite & 0x3) != 0)
    {
        OSI_LOGW(0, "SDMMC write: buffer is not aligned! addr=%08x\n", pWrite);
    }

#define SDMMC_MAX_BLOCK_NUM 2048
    if (block_num < 1 || block_num > SDMMC_MAX_BLOCK_NUM)
    {
        OSI_LOGW(0, "Block Num is overflow\n");
    }

    // Check that the card is in tran (Transmission) state.
    if (true != drvSdmmcTranState(d, SDMMC_TRAN_TIMEOUT))
    // 5200000, 0, initially, that is to say rougly 0.1 sec ?
    {
        OSI_LOGI(0, "Write on Sdmmc while not in Tran state");
        return false;
    }

    // The command for single block or multiple blocks are differents
    if (block_num == 1)
    {
        writeCmd = SDMMC_CMD_WRITE_SINGLE_BLOCK;
    }
    else
    {
        writeCmd = SDMMC_CMD_WRITE_MULT_BLOCK;
    }

    // PreErasing, to accelerate the multi-block write operations
    if (block_num > 1)
    {
        err_status = drvSdmmcSendCmd(d, SDMMC_CMD_SET_WR_BLK_COUNT, block_num, cardResponse);
        if (!err_status)
        {
            OSI_LOGI(0, "Set Pre-erase Failed");
            goto writeFail;
        }

        // Advance compatibility,to support 1.0 t-flash.
        err_status = _sdmmcCheckResponse(d, SDMMC_CMD_SET_WR_BLK_COUNT, cardResponse);
        if (!err_status)
        {
            OSI_LOGE(0, "sdmmc:Pre-erase cmd response status error:0x%x\n", cardResponse[0]);
            goto writeFail;
        }
    }
    // Initiate data migration through Ifc.
    _sdmmcIfcTransfer(d, SDMMC_DIRECTION_WRITE);

    // Initiate data migration of multiple blocks through SD bus.
    err_status = drvSdmmcSendCmd(d, writeCmd, (uint32_t)block_address, cardResponse);
    if (!err_status)
    {
        OSI_LOGI(0, "Set sd write had error");
        goto writeFail;
    }
    err_status = _sdmmcCheckResponse(d, writeCmd, cardResponse);
    if (!err_status)
    {
        OSI_LOGE(0, "sdmmc:Write block,Card Reponse:0x%x\n", cardResponse[0]);
        goto writeFail;
    }
    bool sent = osiSemaphoreTryAcquire(d->tx_done_sema, SDMMC_WRITE_TIMEOUT);
    if (!sent)
    {
        OSI_LOGE(0, "Write on Sdmmc timeout!!!\n");
        goto writeFail;
    }

    // Nota: CMD12 (stop transfer) is automatically
    // sent by the SDMMC controller.

    if (!_sdmmcCheckStatus(d, SDMMC_CRC_STATUS))
    {
        OSI_LOGE(0, "sdmmc: Write block error Bad CRC\n");
        goto writeFail;
    }

    // Check that the card is in tran (Transmission) state.
    if (true != drvSdmmcTranState(d, SDMMC_TRAN_TIMEOUT))
    // 5200000, 0, initially, that is to say rougly 0.1 sec ?
    {
        OSI_LOGE(0, "Write on Sdmmc while not in Tran state");
        goto writeFail;
    }
    return true;
writeFail:
    _sdmmcStopTransfer(d, SDMMC_DIRECTION_WRITE);
    return false;
}

bool drvSdmmcWrite(drvSdmmc_t *d, uint32_t block_number, const void *buffer, uint32_t size)
{
    uint32_t tmpAddress;
    int _start;
    bool value = true;
    OSI_LOGD(0, "sdmmc: drvSdmmcWrite Enter,block_number=%d,  buf=0x%x , size=%d\n", block_number, buffer, size);
    _start = osiUpTimeUS();
    osiMutexLock(d->lock);
    // Addresses are block number for high capacity card
    if (d->card_is_sdhc)
    {
        tmpAddress = (block_number);
    }
    else
    {
        tmpAddress = (block_number * d->block_size);
    }

    value = drvSdmmcMultBlockWrite(d, tmpAddress, buffer, size / d->block_size);

    if (value)
    {
        OSI_LOGD(0, "sdmmc: drvSdmmcWrite finish, %d us \n", (uint32_t)(osiUpTimeUS() - _start));
    }
    else
    {
        OSI_LOGI(0, "sdmmc: drvSdmmcWrite failed \n");
    }
    osiMutexUnlock(d->lock);
    return value;
}
void drvSdmmcClose(drvSdmmc_t *d)
{
    if (d == NULL)
        return;
    // Don't close the MCD driver if a transfer is in progress,
    // and be definitive about it:
    if (!_sdmmcCheckStatus(d, SDMMC_TRANSFER_DONE))
    {
        OSI_LOGW(0, "Attempt to close MCD while a transfer is in progress");
    }
    // Brutal force stop current transfer, if any.
    _sdmmcStopTransfer(d, SDMMC_DIRECTION_WRITE);
    _sdmmcStopTransfer(d, SDMMC_DIRECTION_READ);
}
void drvSdmmcDestroy(drvSdmmc_t *d)
{
    if (d == NULL)
        return;

    drvSdmmcClose(d);
    osiSemaphoreDelete(d->tx_done_sema);
    osiSemaphoreDelete(d->rx_done_sema);
    osiIrqDisable(d->irqn);
    osiIrqSetHandler(d->irqn, NULL, NULL);
    free(d);
}

uint32_t drvSdmmcGetBlockNum(drvSdmmc_t *d)
{
    if (d == NULL)
        return 0;
    return d->block_num_total;
}
