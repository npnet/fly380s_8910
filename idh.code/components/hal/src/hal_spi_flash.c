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

#include "hal_spi_flash.h"
#include "hal_chip.h"
#include "hwregs.h"
#include "osi_api.h"
#include "osi_profile.h"
#include "osi_byte_buf.h"

// spi_config.tx_rx_size
enum
{
    RX_FIFO_WIDTH_8BIT = 0,
    RX_FIFO_WIDTH_16BIT = 1,
    RX_FIFO_WIDTH_32BIT = 2,
    RX_FIFO_WIDTH_24BIT = 3,
};

#define DELAY_AFTER_RESET (100)                  // us, tRST_R(20), tRST_P(20), tRST_E(12ms)
#define DELAY_AFTER_RELEASE_DEEP_POWER_DOWN (20) // us, tRES1(20), tRES2(20)

#define SIZE_4K (4 * 1024)
#define SIZE_32K (32 * 1024)
#define SIZE_64K (64 * 1024)

#define MID_WINBOND (0xEF)
#define MID_GD (0xC8)
#define MID(id) ((id)&0xff)

#define IS_GD(id) (MID(id) == MID_GD)
#define IS_WINBOND(id) (MID(id) == MID_WINBOND)

#define CAPACITY_BYTES(id) (1 << (((id) >> 16) & 0xff))

#define STREG_WIP (1 << 0)
#define STREG_WEL (1 << 1)
#define STREG_BP0 (1 << 2)
#define STREG_BP1 (1 << 3)
#define STREG_BP2 (1 << 4)
#define STREG_BP3 (1 << 5)
#define STREG_BP4 (1 << 6)
#define STREG_QE (1 << 9)
#define STREG_SUS2 (1 << 10)
#define STREG_SUS1 (1 << 15)
#define STREG_LB1 (1 << 11)
#define STREG_LB2 (1 << 12)
#define STREG_LB3 (1 << 13)
#define STREG_CMP (1 << 14)

#define OPCODE_WRITE_ENABLE 0x06
#define OPCODE_WRITE_VOLATILE_STATUS_ENABLE 0x50
#define OPCODE_WRITE_DISABLE 0x04
#define OPCODE_READ_STATUS 0x05
#define OPCODE_READ_STATUS_2 0x35
#define OPCODE_READ_STATUS_3 0x15
#define OPCODE_WRITE_STATUS 0x01
#define OPCODE_WRITE_STATUS_2 0x31
#define OPCODE_WRITE_STATUS_3 0x11
#define OPCODE_SECTOR_ERASE 0x20
#define OPCODE_BLOCK_ERASE 0xd8
#define OPCODE_BLOCK32K_ERASE 0x52
#define OPCODE_CHIP_ERASE 0xc7 // or 0x60
#define OPCODE_ERASE_SUSPEND 0x75
#define OPCODE_ERASE_RESUME 0x7a
#define OPCODE_PROGRAM_SUSPEND 0x75
#define OPCODE_PROGRAM_RESUME 0x7a
#define OPCODE_RESET_ENABLE 0x66
#define OPCODE_RESET 0x99
#define OPCODE_PAGE_PROGRAM 0x02
#define OPCODE_READ_ID 0x9f
#define OPCODE_SECURITY_ERASE 0x44
#define OPCODE_SECURITY_PROGRAM 0x42
#define OPCODE_SECURITY_READ 0x48
#define OPCODE_POWER_DOWN 0xb9
#define OPCODE_RELEASE_POWER_DOWN 0xab
#define OPCODE_QUAD_FAST_READ 0xeb
#define OPCODE_READ_UNIQUE_ID 0x4b

#define WP_GD_WINBOND_MASK (STREG_CMP | STREG_BP0 | STREG_BP1 | STREG_BP2 | STREG_BP3 | STREG_BP4)
#define WP_GD_WINBOND_NONE (0x0000)
#define WP_GD_WINBOND_4K (STREG_BP4 | STREG_BP3 | STREG_BP0)
#define WP_GD_WINBOND_8K (STREG_BP4 | STREG_BP3 | STREG_BP1)
#define WP_GD_WINBOND_16K (STREG_BP4 | STREG_BP3 | STREG_BP1 | STREG_BP0)
#define WP_GD_WINBOND_32K (STREG_BP4 | STREG_BP3 | STREG_BP2 | STREG_BP1)
#define WP_GD_WINBOND_1_64 (STREG_BP3 | STREG_BP0)
#define WP_GD_WINBOND_1_32 (STREG_BP3 | STREG_BP1)
#define WP_GD_WINBOND_1_16 (STREG_BP3 | STREG_BP1 | STREG_BP0)
#define WP_GD_WINBOND_1_8 (STREG_BP3 | STREG_BP2)
#define WP_GD_WINBOND_1_4 (STREG_BP3 | STREG_BP2 | STREG_BP0)
#define WP_GD_WINBOND_1_2 (STREG_BP3 | STREG_BP2 | STREG_BP1)
#define WP_GD_WINBOND_3_4 (STREG_CMP | STREG_BP2 | STREG_BP0)
#define WP_GD_WINBOND_7_8 (STREG_CMP | STREG_BP2)
#define WP_GD_WINBOND_15_16 (STREG_CMP | STREG_BP1 | STREG_BP0)
#define WP_GD_WINBOND_31_32 (STREG_CMP | STREG_BP1)
#define WP_GD_WINBOND_63_64 (STREG_CMP | STREG_BP0)
#define WP_GD_WINBOND_ALL (STREG_BP2 | STREG_BP1 | STREG_BP0)

#define CMD_ADDRESS(opcode, address) ((opcode) | ((address) << 8))
#define EXTCMD_NORX(opcode) ((opcode) << 8)
#define EXTCMD_SRX(opcode) (((opcode) << 8) | (1 << 16))             // rx single mode
#define EXTCMD_QRX(opcode) (((opcode) << 8) | (1 << 16) | (1 << 17)) // rx quad mode

#ifdef CONFIG_SOC_8910
static inline void prvWaitNotBusy(uintptr_t d)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_STATUS_T status;
    REG_WAIT_FIELD_EQZ(status, hwp->spi_status, spi_flash_busy);
    REG_WAIT_FIELD_EQZ(status, hwp->spi_status, spi_flash_busy);
}

static inline void prvClearFifo(uintptr_t d)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_FIFO_CONTROL_T fifo_control = {
        .b.rx_fifo_clr = 1,
        .b.tx_fifo_clr = 1,
    };
    hwp->spi_fifo_control = fifo_control.v;
}

static inline void prvSetRxSize(uintptr_t d, unsigned size)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_BLOCK_SIZE_T block_size = {hwp->spi_block_size};
    block_size.b.spi_rw_blk_size = size;
    hwp->spi_block_size = block_size.v;
}

static inline void prvSetFifoWidth(uintptr_t d, unsigned width)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_CONFIG_T config = {hwp->spi_config};
    if (width == 1)
        config.b.tx_rx_size = RX_FIFO_WIDTH_8BIT;
    else if (width == 2)
        config.b.tx_rx_size = RX_FIFO_WIDTH_16BIT;
    else if (width == 3)
        config.b.tx_rx_size = RX_FIFO_WIDTH_24BIT;
    else
        config.b.tx_rx_size = RX_FIFO_WIDTH_32BIT;
    hwp->spi_config = config.v;
}

static inline void prvWriteCommand(uintptr_t d, uint32_t cmd)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    hwp->spi_cmd_addr = cmd;
    (void)hwp->spi_cmd_addr;
}

static inline uint32_t prvReadBack(uintptr_t d)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    return hwp->rx_status;
}

static inline void prvWriteFifo8(uintptr_t d, const uint8_t *txdata, unsigned txsize)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;

    for (unsigned n = 0; n < txsize; n++)
        hwp->spi_data_fifo = txdata[n];
}

static inline void prvReadFifo8(uintptr_t d, uint8_t *data, unsigned size)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    while (size > 0)
    {
        REG_SPI_FLASH_SPI_STATUS_T spi_status = {hwp->spi_status};
        int count = OSI_MIN(int, spi_status.b.rx_fifo_count, size);
        for (int n = 0; n < count; n++)
            *data++ = hwp->spi_data_fifo;
        size -= count;
    }
}
#endif

#ifdef CONFIG_SOC_8955
static inline void prvWaitNotBusy(uintptr_t d)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_DATA_FIFO_RO_T status;
    REG_WAIT_FIELD_EQZ(status, hwp->spi_data_fifo_ro, spi_flash_busy);
    REG_WAIT_FIELD_EQZ(status, hwp->spi_data_fifo_ro, spi_flash_busy);
}

static inline void prvClearFifo(uintptr_t d)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_FIFO_CONTROL_T fifo_control = {
        .b.rx_fifo_clr = 1,
        .b.tx_fifo_clr = 1,
    };
    hwp->spi_fifo_control = fifo_control.v;
}

static inline void prvSetRxSize(uintptr_t d, unsigned size)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_BLOCK_SIZE_T block_size = {hwp->spi_block_size};
    block_size.b.spi_rw_blk_size = size;
    hwp->spi_block_size = block_size.v;
}

static inline void prvSetFifoWidth(uintptr_t d, unsigned width)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_CONFIG_T config = {hwp->spi_config};
    if (width == 1)
        config.b.tx_rx_size = RX_FIFO_WIDTH_8BIT;
    else if (width == 2)
        config.b.tx_rx_size = RX_FIFO_WIDTH_16BIT;
    else if (width == 3)
        config.b.tx_rx_size = RX_FIFO_WIDTH_24BIT;
    else
        config.b.tx_rx_size = RX_FIFO_WIDTH_32BIT;
    hwp->spi_config = config.v;
}

static inline void prvWriteCommand(uintptr_t d, uint32_t cmd)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    hwp->spi_cmd_addr = cmd;
    (void)hwp->spi_cmd_addr;
}

static inline uint32_t prvReadBack(uintptr_t d)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    return hwp->spi_read_back;
}

static inline void prvWriteFifo8(uintptr_t d, const uint8_t *txdata, unsigned txsize)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;

    for (unsigned n = 0; n < txsize; n++)
        hwp->spi_data_fifo_wo = txdata[n];
}

static inline void prvReadFifo8(uintptr_t d, uint8_t *data, unsigned size)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    while (size > 0)
    {
        REG_SPI_FLASH_SPI_DATA_FIFO_RO_T spi_status = {hwp->spi_data_fifo_ro};
        int count = OSI_MIN(int, spi_status.b.rx_fifo_count, size);
        for (int n = 0; n < count; n++)
            *data++ = hwp->spi_data_fifo_wo;
        size -= count;
    }
}
#endif

#ifdef CONFIG_SOC_8909
static inline void prvWaitNotBusy(uintptr_t d)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_FIFO_STATUS_T status;
    REG_WAIT_FIELD_EQZ(status, hwp->spi_fifo_status, spi_flash_busy);
    REG_WAIT_FIELD_EQZ(status, hwp->spi_fifo_status, spi_flash_busy);
}

static inline void prvClearFifo(uintptr_t d)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_FIFO_CONTROL_T fifo_control = {
        .b.rx_fifo_clr = 1,
        .b.tx_fifo_clr = 1,
    };
    hwp->spi_fifo_control = fifo_control.v;
}

static inline void prvSetRxSize(uintptr_t d, unsigned size)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_BLOCK_SIZE_T block_size = {hwp->spi_block_size};
    block_size.b.spi_rw_blk_size = size;
    hwp->spi_block_size = block_size.v;
}

static inline void prvSetFifoWidth(uintptr_t d, unsigned width)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    REG_SPI_FLASH_SPI_CONFIG_T config = {hwp->spi_config};
    if (width == 1)
        config.b.tx_rx_size = RX_FIFO_WIDTH_8BIT;
    else if (width == 2)
        config.b.tx_rx_size = RX_FIFO_WIDTH_16BIT;
    else if (width == 3)
        config.b.tx_rx_size = RX_FIFO_WIDTH_24BIT;
    else
        config.b.tx_rx_size = RX_FIFO_WIDTH_32BIT;
    hwp->spi_config = config.v;
}

static inline void prvWriteCommand(uintptr_t d, uint32_t cmd)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    hwp->spi_cmd_addr = cmd;
    (void)hwp->spi_cmd_addr;
}

static inline uint32_t prvReadBack(uintptr_t d)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    return hwp->spi_read_back;
}

static inline void prvWriteFifo8(uintptr_t d, const uint8_t *txdata, unsigned txsize)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;

    for (unsigned n = 0; n < txsize; n++)
        hwp->spi_data_fifo = txdata[n];
}

static inline void prvReadFifo8(uintptr_t d, uint8_t *data, unsigned size)
{
    HWP_SPI_FLASH_T *hwp = (HWP_SPI_FLASH_T *)d;
    while (size > 0)
    {
        REG_SPI_FLASH_SPI_FIFO_STATUS_T spi_status = {hwp->spi_fifo_status};
        int count = OSI_MIN(int, spi_status.b.rx_fifo_count, size);
        for (int n = 0; n < count; n++)
            *data++ = hwp->spi_data_fifo;
        size -= count;
    }
}
#endif

// Send command (basic or extended), with additional tx data, without rx data
static void prvCmdNoRx(uintptr_t hwp, uint32_t cmd, const void *tx_data, unsigned tx_size)
{
    prvWaitNotBusy(hwp);
    prvClearFifo(hwp);
    prvSetRxSize(hwp, 0);
    prvWriteFifo8(hwp, tx_data, tx_size);
    prvWriteCommand(hwp, cmd);
    prvWaitNotBusy(hwp);
}

// Send command (basic or extended), without additional tx data, without rx data
static void prvCmdOnlyNoRx(uintptr_t hwp, uint32_t cmd)
{
    prvCmdNoRx(hwp, cmd, NULL, 0);
}

// Send command (basic or extended), with additional tx data, get rx data by readback
static uint32_t prvCmdRxReadback(uintptr_t hwp, uint32_t cmd, unsigned rx_size,
                                 const void *tx_data, unsigned tx_size)
{
    prvWaitNotBusy(hwp);
    prvClearFifo(hwp);
    prvSetRxSize(hwp, rx_size);
    prvSetFifoWidth(hwp, rx_size);
    prvWriteFifo8(hwp, tx_data, tx_size);
    prvWriteCommand(hwp, cmd);
    prvWaitNotBusy(hwp);

    uint32_t result = prvReadBack(hwp) >> ((4 - rx_size) * 8);
    prvSetRxSize(hwp, 0);
    return result;
}

// Send command (basic or extended), without additional tx data, get rx data by readback
static uint32_t prvCmdOnlyReadback(uintptr_t hwp, uint32_t cmd, unsigned rx_size)
{
    return prvCmdRxReadback(hwp, cmd, rx_size, NULL, 0);
}

// Send command (basic or extended), without additional tx data, get rx data by fifo
static void prvCmdRxFifo(uintptr_t hwp, uint32_t cmd, const void *tx_data, unsigned tx_size,
                         void *rx_data, unsigned rx_size)
{
    prvWaitNotBusy(hwp);
    prvClearFifo(hwp);
    prvSetRxSize(hwp, rx_size);
    prvSetFifoWidth(hwp, 1);
    prvWriteFifo8(hwp, tx_data, tx_size);
    prvWriteCommand(hwp, cmd);

    prvReadFifo8(hwp, rx_data, rx_size);
    prvWaitNotBusy(hwp);
    prvSetRxSize(hwp, 0);
}

uint32_t halSpiFlashReadId(uintptr_t hwp)
{
    return prvCmdOnlyReadback(hwp, EXTCMD_SRX(OPCODE_READ_ID), 3);
}

void halSpiFlashWriteEnable(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_WRITE_ENABLE));
}

void halSpiFlashWriteDisable(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_WRITE_DISABLE));
}

void halSpiFlashProgramSuspend(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_PROGRAM_SUSPEND));
}

void halSpiFlashEraseSuspend(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_ERASE_SUSPEND));
}

void halSpiFlashProgramResume(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_PROGRAM_RESUME));
}

void halSpiFlashEraseResume(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_ERASE_RESUME));
}

void halSpiFlashChipErase(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_CHIP_ERASE));
}

void halSpiFlashResetEnable(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_RESET_ENABLE));
}

void halSpiFlashReset(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_RESET));
}

uint8_t halSpiFlashReadStatus(uintptr_t hwp)
{
    return prvCmdOnlyReadback(hwp, EXTCMD_SRX(OPCODE_READ_STATUS), 1) & 0xff;
}

uint8_t halSpiFlashReadStatus2(uintptr_t hwp)
{
    return prvCmdOnlyReadback(hwp, EXTCMD_SRX(OPCODE_READ_STATUS_2), 1) & 0xff;
}

void halSpiFlashWriteVolatileStatusEnable(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_WRITE_VOLATILE_STATUS_ENABLE));
}

void halSpiFlashPageProgram(uintptr_t hwp, uint32_t offset, const void *data, uint32_t size)
{
    prvCmdNoRx(hwp, CMD_ADDRESS(OPCODE_PAGE_PROGRAM, offset), data, size);
}

void halSpiFlashErase4K(uintptr_t hwp, uint32_t offset)
{
    prvCmdOnlyNoRx(hwp, CMD_ADDRESS(OPCODE_SECTOR_ERASE, offset));
}

void halSpiFlashErase32K(uintptr_t hwp, uint32_t offset)
{
    prvCmdOnlyNoRx(hwp, CMD_ADDRESS(OPCODE_BLOCK32K_ERASE, offset));
}

void halSpiFlashErase64K(uintptr_t hwp, uint32_t offset)
{
    prvCmdOnlyNoRx(hwp, CMD_ADDRESS(OPCODE_BLOCK_ERASE, offset));
}

void halSpiFlashDeepPowerDown(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_POWER_DOWN));
}

void halSpiFlashReleaseDeepPowerDown(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_RELEASE_POWER_DOWN));
    osiDelayUS(DELAY_AFTER_RELEASE_DEEP_POWER_DOWN);
}

void halSpiFlashProgramSecurityRegister(uintptr_t hwp, uint32_t address, const void *data, uint32_t size)
{
    prvCmdNoRx(hwp, CMD_ADDRESS(OPCODE_SECURITY_PROGRAM, address), data, size);
}

void halSpiFlashEraseSecurityRegister(uintptr_t hwp, uint32_t address)
{
    prvCmdOnlyNoRx(hwp, CMD_ADDRESS(OPCODE_SECURITY_ERASE, address));
}

uint16_t halSpiFlashReadStatus12(uintptr_t hwp)
{
    uint8_t lo = halSpiFlashReadStatus(hwp);
    uint8_t hi = halSpiFlashReadStatus2(hwp);
    return (hi << 8) | lo;
}

void halSpiFlashWriteStatus12(uintptr_t hwp, uint16_t status)
{
    uint8_t data[2] = {status & 0xff, (status >> 8) & 0xff};
    prvCmdNoRx(hwp, OPCODE_WRITE_STATUS, data, 2);
}

void halSpiFlashErase(uintptr_t hwp, uint32_t offset, uint32_t size)
{
    if (size == SIZE_64K)
        halSpiFlashErase64K(hwp, offset);
    else if (size == SIZE_32K)
        halSpiFlashErase32K(hwp, offset);
    else
        halSpiFlashErase4K(hwp, offset);
}

void halSpiFlashReadUniqueId(uintptr_t hwp, uint8_t uid[8])
{
    uint8_t tx_data[4] = {};
    prvCmdRxFifo(hwp, EXTCMD_SRX(OPCODE_READ_UNIQUE_ID), tx_data, 4, uid, 8);
}

uint16_t halSpiFlashReadCpId(uintptr_t hwp)
{
    uint8_t uid[18];
    uint8_t tx_data[4] = {};
    prvCmdRxFifo(hwp, EXTCMD_SRX(OPCODE_READ_UNIQUE_ID), tx_data, 4, uid, 18);
    return osiBytesGetLe16(&uid[16]);
}

void halSpiFlashReadSecurityRegister(uintptr_t hwp, uint32_t address, void *data, uint32_t size)
{
    uint8_t tx_data[4] = {(address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff, 0};
    uint32_t val = prvCmdRxReadback(hwp, EXTCMD_SRX(OPCODE_SECURITY_READ), size, tx_data, 4);

    uint8_t *pin = (uint8_t *)&val;
    uint8_t *pout = (uint8_t *)data;
    for (unsigned n = 0; n < size; n++)
        *pout++ = *pin++;
}

bool halSpiFlashIsWipFinished(uintptr_t hwp)
{
    osiDelayUS(1);
    if (halSpiFlashReadStatus(hwp) & STREG_WIP)
        return false;
    if (halSpiFlashReadStatus(hwp) & STREG_WIP)
        return false;
    return true;
}

void halSpiFlashWaitWipFinish(uintptr_t hwp)
{
    OSI_LOOP_WAIT(halSpiFlashIsWipFinished(hwp));
}

uint16_t halSpiFlashStatusWpRange(uint32_t id, uint16_t status, uint32_t offset, uint32_t size)
{
    if (!IS_GD(id) && !IS_WINBOND(id))
        return status;

    uint32_t capacity = CAPACITY_BYTES(id);
    uint32_t wp = WP_GD_WINBOND_NONE;
    // try to write protect low address as possible
    if (offset >= capacity - capacity / 64)
        wp = WP_GD_WINBOND_63_64;
    else if (offset >= capacity - capacity / 32)
        wp = WP_GD_WINBOND_31_32;
    else if (offset >= capacity - capacity / 16)
        wp = WP_GD_WINBOND_15_16;
    else if (offset >= capacity - capacity / 8)
        wp = WP_GD_WINBOND_7_8;
    else if (offset >= capacity - capacity / 4)
        wp = WP_GD_WINBOND_3_4;
    else if (offset >= capacity / 2)
        wp = WP_GD_WINBOND_1_2;
    else if (offset >= capacity / 4)
        wp = WP_GD_WINBOND_1_4;
    else if (offset >= capacity / 8)
        wp = WP_GD_WINBOND_1_8;
    else if (offset >= capacity / 16)
        wp = WP_GD_WINBOND_1_16;
    else if (offset >= capacity / 32)
        wp = WP_GD_WINBOND_1_32;
    else if (offset >= capacity / 64)
        wp = WP_GD_WINBOND_1_64;
    else if (offset >= 32 * 1024)
        wp = WP_GD_WINBOND_32K;
    else if (offset >= 16 * 1024)
        wp = WP_GD_WINBOND_16K;
    else if (offset >= 8 * 1024)
        wp = WP_GD_WINBOND_8K;
    else if (offset >= 4 * 1024)
        wp = WP_GD_WINBOND_4K;

    return (status & ~WP_GD_WINBOND_MASK) | wp;
}

uint16_t halSpiFlashStatusWpAll(uint32_t id, uint16_t status)
{
    if (!IS_GD(id) && !IS_WINBOND(id))
        return status;

    return (status & ~WP_GD_WINBOND_MASK) | WP_GD_WINBOND_ALL;
}

uint16_t halSpiFlashStatusWpNone(uint32_t id, uint16_t status)
{
    if (!IS_GD(id) && !IS_WINBOND(id))
        return status;

    return (status & ~WP_GD_WINBOND_MASK) | WP_GD_WINBOND_NONE;
}

void halSpiFlashPrepareEraseProgram(uintptr_t hwp, uint32_t mid, uint32_t offset, uint32_t size)
{
    uint16_t status = halSpiFlashReadStatus12(hwp);
    uint16_t status_open = halSpiFlashStatusWpRange(mid, status, offset, size);
    while (status_open != status)
    {
        halSpiFlashWriteVolatileStatusEnable(hwp);
        halSpiFlashWriteStatus12(hwp, status_open);
        status = halSpiFlashReadStatus12(hwp);
    }

    halSpiFlashWriteEnable(hwp);
}

void halSpiFlashFinishEraseProgram(uintptr_t hwp, uint32_t mid)
{
    uint16_t status = halSpiFlashReadStatus12(hwp);
    uint16_t status_close = halSpiFlashStatusWpAll(mid, status);
    while (status_close != status)
    {
        halSpiFlashWriteVolatileStatusEnable(hwp);
        halSpiFlashWriteStatus12(hwp, status_close);
        status = halSpiFlashReadStatus12(hwp);
    }
}

void halSpiFlashStatusCheck(uintptr_t hwp)
{
    if (hwp == 0)
        hwp = (uintptr_t)hwp_spiFlash;

    uint32_t mid = halSpiFlashReadId(hwp);
    uint16_t status = halSpiFlashReadStatus12(hwp);

    bool reset_needed = false;
    if (IS_GD(mid) && (status & (STREG_SUS1 | STREG_SUS2)))
        reset_needed = true;
    if (IS_WINBOND(mid) && (status & STREG_SUS1))
        reset_needed = true;

    if (reset_needed)
    {
        halSpiFlashResetEnable(hwp);
        halSpiFlashReset(hwp);
        osiDelayUS(DELAY_AFTER_RESET);
    }

    uint16_t status_needed = halSpiFlashStatusWpAll(mid, status) | STREG_QE;
    if (IS_GD(mid))
        status_needed &= ~(STREG_WIP | STREG_WEL | STREG_SUS2 | STREG_SUS1);
    if (IS_WINBOND(mid))
        status_needed &= ~(STREG_WIP | STREG_WEL | STREG_SUS1);

    if (status != status_needed)
    {
        // Write to non-volatile status
        halSpiFlashWriteEnable(hwp);
        halSpiFlashWriteStatus12(hwp, status_needed);
        halSpiFlashWaitWipFinish(hwp);
    }
}
