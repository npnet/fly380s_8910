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
#include "hal_spi_flash_defs.h"

#define FLASHRAM_CODE OSI_SECTION_LINE(.sramtext.flashinit)
#define FLASHRAM_API FLASHRAM_CODE OSI_NO_INLINE
#include "hal_spi_flash_internal.h"

/**
 * Generic flash command
 */
FLASHRAM_API void halSpiFlashCmd(
    uintptr_t hwp, uint32_t cmd,
    const void *tx_data, unsigned tx_size,
    void *rx_data, unsigned rx_size,
    unsigned flags)
{
    prvWaitNotBusy(hwp);
    prvClearFifo(hwp);
    prvSetRxSize(hwp, rx_size);
    prvSetFifoWidth(hwp, (flags & HAL_SPI_FLASH_RX_READBACK) ? rx_size : 1);
    prvWriteFifo8(hwp, tx_data, tx_size, (flags & HAL_SPI_FLASH_TX_QUAD) ? TX_QUAD_MASK : 0);
    prvWriteCommand(hwp, cmd);

    if (!(flags & HAL_SPI_FLASH_RX_READBACK))
        prvReadFifo8(hwp, rx_data, rx_size);

    prvWaitNotBusy(hwp);

    if (flags & HAL_SPI_FLASH_RX_READBACK)
    {
        uint32_t rword = prvReadBack(hwp) >> ((4 - rx_size) * 8);
        uint8_t *rxb = (uint8_t *)rx_data;
        for (unsigned n = 0; n < rx_size; n++)
        {
            *rxb++ = rword & 0xff;
            rword >>= 8;
        }
    }
    prvSetRxSize(hwp, 0);
}

/**
 * OPCODE_READ_ID
 */
FLASHRAM_API uint32_t halSpiFlashReadId(uintptr_t hwp)
{
    return prvCmdOnlyReadback(hwp, EXTCMD_SRX(OPCODE_READ_ID), 3);
}

/**
 * OPCODE_WRITE_ENABLE
 */
FLASHRAM_API void halSpiFlashWriteEnable(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_WRITE_ENABLE));
}

/**
 * OPCODE_WRITE_DISABLE
 */
FLASHRAM_API void halSpiFlashWriteDisable(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_WRITE_DISABLE));
}

/**
 * OPCODE_RESET_ENABLE
 */
FLASHRAM_API void halSpiFlashResetEnable(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_RESET_ENABLE));
}

/**
 * OPCODE_RESET
 */
FLASHRAM_API void halSpiFlashReset(uintptr_t hwp)
{
    prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_RESET));
}

/**
 * OPCODE_READ_STATUS
 */
FLASHRAM_API uint8_t halSpiFlashReadSR1(uintptr_t hwp)
{
    return prvCmdOnlyReadback(hwp, EXTCMD_SRX(OPCODE_READ_STATUS), 1) & 0xff;
}

/**
 * OPCODE_READ_STATUS_2
 */
FLASHRAM_API uint8_t halSpiFlashReadSR2(uintptr_t hwp)
{
    return prvCmdOnlyReadback(hwp, EXTCMD_SRX(OPCODE_READ_STATUS_2), 1) & 0xff;
}

/**
 * OPCODE_WRITE_STATUS
 */
FLASHRAM_API void halSpiFlashWriteSR12(uintptr_t hwp, uint16_t sr)
{
    uint8_t data[2] = {sr & 0xff, (sr >> 8) & 0xff};
    prvCmdNoRx(hwp, EXTCMD_NORX(OPCODE_WRITE_STATUS), data, 2);
}

/**
 * OPCODE_WRITE_STATUS
 */
FLASHRAM_API void halSpiFlashWriteSR1(uintptr_t hwp, uint8_t sr1)
{
    prvCmdNoRx(hwp, EXTCMD_NORX(OPCODE_WRITE_STATUS), &sr1, 1);
}

/**
 * OPCODE_WRITE_STATUS_2
 */
FLASHRAM_API void halSpiFlashWriteSR2(uintptr_t hwp, uint8_t sr2)
{
    prvCmdNoRx(hwp, EXTCMD_NORX(OPCODE_WRITE_STATUS_2), &sr2, 1);
}

/**
 * read SR1 and SR2
 */
FLASHRAM_API uint16_t halSpiFlashReadSR12(uintptr_t hwp)
{
    uint8_t lo = halSpiFlashReadSR1(hwp);
    uint8_t hi = halSpiFlashReadSR2(hwp);
    return (hi << 8) | lo;
}

/**
 * write SR1, and SR2 if supported
 */
FLASHRAM_API void halSpiFlashWriteSRByProp(uintptr_t hwp, const halSpiFlashProp_t *prop, uint16_t sr)
{
    if (!prop->has_sr2)
    {
        halSpiFlashWriteEnable(hwp);
        halSpiFlashWriteSR1(hwp, sr & 0xff);
        halSpiFlashWaitWipFinish(hwp);
    }
    else if (prop->write_sr12)
    {
        halSpiFlashWriteEnable(hwp);
        halSpiFlashWriteSR12(hwp, sr);
        halSpiFlashWaitWipFinish(hwp);
    }
    else
    {
        halSpiFlashWriteEnable(hwp);
        halSpiFlashWriteSR1(hwp, sr & 0xff);
        halSpiFlashWaitWipFinish(hwp);
        halSpiFlashWriteEnable(hwp);
        halSpiFlashWriteSR2(hwp, (sr >> 8) & 0xff);
        halSpiFlashWaitWipFinish(hwp);
    }
}

/**
 * whether WIP is 0
 */
FLASHRAM_API bool halSpiFlashIsWipFinished(uintptr_t hwp)
{
    osiDelayUS(1);
    if (halSpiFlashReadSR1(hwp) & STREG_WIP)
        return false;
    if (halSpiFlashReadSR1(hwp) & STREG_WIP)
        return false;
    return true;
}

/**
 * wait WIP to be 0
 */
FLASHRAM_API void halSpiFlashWaitWipFinish(uintptr_t hwp)
{
    OSI_LOOP_WAIT(halSpiFlashIsWipFinished(hwp));
}

/**
 * SR for all write protected
 */
FLASHRAM_API uint16_t halSpiFlashStatusWpAll(const halSpiFlashProp_t *prop, uint16_t sr)
{
    if (prop->wp_gd_en)
    {
        if (prop->capacity == (1 << 20))
            return (sr & ~WP_GD8M_MASK) | WP_GD8M_ALL;
        else if (prop->capacity == (2 << 20))
            return (sr & ~WP_GD16M_MASK) | WP_GD16M_ALL;
        else
            return (sr & ~WP_GD32M_MASK) | WP_GD32M_ALL;
    }
    else if (prop->wp_xmca_en)
    {
        return (sr & ~WP_XMCA_MASK) | WP_XMCA_ALL;
    }
    else
    {
        return sr;
    }
}

/**
 * SR for none write protected
 */
FLASHRAM_API uint16_t halSpiFlashStatusWpNone(const halSpiFlashProp_t *prop, uint16_t sr)
{
    if (prop->wp_gd_en)
    {
        if (prop->capacity == (1 << 20))
            return (sr & ~WP_GD8M_MASK) | WP_GD8M_NONE;
        else if (prop->capacity == (2 << 20))
            return (sr & ~WP_GD16M_MASK) | WP_GD16M_NONE;
        else
            return (sr & ~WP_GD32M_MASK) | WP_GD32M_NONE;
    }
    else if (prop->wp_xmca_en)
    {
        return (sr & ~WP_XMCA_MASK) | WP_XMCA_NONE;
    }
    else
    {
        return sr;
    }
}

/**
 * whether initial reset is needed
 */
OSI_FORCE_INLINE static bool prvInitResetNeeded(const halSpiFlashProp_t *prop, uint16_t sr)
{
    if (prop->type == HAL_SPI_FLASH_TYPE_GD)
        return (sr & (STREG_SUS1 | STREG_SUS2 | STREG_WEL | STREG_WIP)) ? true : false;
    if (prop->type == HAL_SPI_FLASH_TYPE_WINBOND)
        return (sr & (STREG_SUS1 | STREG_WEL | STREG_WIP)) ? true : false;
    if (prop->type == HAL_SPI_FLASH_TYPE_XMCC)
        return (sr & (STREG_SUS1 | STREG_WEL | STREG_WIP)) ? true : false;
    if (prop->type == HAL_SPI_FLASH_TYPE_XTX)
        return (sr & (STREG_WEL | STREG_WIP)) ? true : false;
    return true; // reset needed if unsure
}

/**
 * whether initial reset is needed
 */
OSI_FORCE_INLINE static uint16_t prvInitSR(const halSpiFlashProp_t *prop, uint16_t sr)
{
    sr = halSpiFlashStatusWpAll(prop, sr);
    if (prop->type == HAL_SPI_FLASH_TYPE_GD)
        return (sr | STREG_QE) & ~(STREG_SUS1 | STREG_SUS2 | STREG_WEL | STREG_WIP);
    if (prop->type == HAL_SPI_FLASH_TYPE_WINBOND)
        return (sr | STREG_QE) & ~(STREG_SUS1 | STREG_WEL | STREG_WIP);
    if (prop->type == HAL_SPI_FLASH_TYPE_XMCC)
        return (sr | STREG_QE) & ~(STREG_SUS1 | STREG_WEL | STREG_WIP);
    if (prop->type == HAL_SPI_FLASH_TYPE_XTX)
        return (sr | STREG_QE) & ~(STREG_WEL | STREG_WIP);
    return sr; // not touch if unsure
}

FLASHRAM_API void halSpiFlashStatusCheck(uintptr_t hwp)
{
    if (hwp == 0)
        hwp = (uintptr_t)hwp_spiFlash;

    uint32_t mid = halSpiFlashReadId(hwp);

    halSpiFlashProp_t prop;
    halSpiFlashPropsByMid(mid, &prop);

    if (prop.type == HAL_SPI_FLASH_TYPE_XMCA)
    {
        halSpiFlashResetEnable(hwp);
        halSpiFlashReset(hwp);
        osiDelayUS(DELAY_AFTER_RESET);

        prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_ENTER_OTP_MODE));
        uint8_t sr_otp = halSpiFlashReadSR1(hwp);
        if ((sr_otp & REG_BIT3) == 0) // TB
        {
            halSpiFlashWriteEnable(hwp);
            halSpiFlashWriteSR1(hwp, sr_otp | REG_BIT3);
            halSpiFlashWaitWipFinish(hwp);
        }
        prvCmdOnlyNoRx(hwp, EXTCMD_NORX(OPCODE_EXIT_OPT_MODE));

        uint8_t sr = halSpiFlashReadSR1(hwp);
        uint16_t sr_needed = halSpiFlashStatusWpAll(&prop, sr);
        sr_needed &= ~REG_BIT6; // EBL
        if (sr != sr_needed)
        {
            halSpiFlashWriteEnable(hwp);
            halSpiFlashWriteSR1(hwp, sr_needed);
            halSpiFlashWaitWipFinish(hwp);
        }
        return;
    }

    if (!prop.init_sr_check)
        return;

    uint16_t sr = halSpiFlashReadSR12(hwp);
    if (prvInitResetNeeded(&prop, sr))
    {
        halSpiFlashResetEnable(hwp);
        halSpiFlashReset(hwp);
        osiDelayUS(DELAY_AFTER_RESET);

        sr = halSpiFlashReadSR12(hwp);
    }

    // Write to non-volatile status
    uint16_t init_sr = prvInitSR(&prop, sr);
    if (sr != init_sr)
        halSpiFlashWriteSRByProp(hwp, &prop, init_sr);
}

#define MID_WINBOND (0xEF)
#define MID_GD (0xC8)
#define MID_XMC (0x20)
#define MID_XTX (0x0B)
#define IS_GD(mid) (MID(mid) == MID_GD)
#define IS_WINBOND(mid) (MID(mid) == MID_WINBOND)
#define IS_XMCC(mid) (MID(mid) == MID_XMC && MEMTYPE(mid) == 0x50 && CAPACITY_BITS(mid) <= 0x16)
#define IS_XMCA(mid) (MID(mid) == MID_XMC && MEMTYPE(mid) == 0x38)
#define IS_XTX(mid) (MID(mid) == MID_XTX)

FLASHRAM_API void halSpiFlashPropsByMid(unsigned mid, halSpiFlashProp_t *prop)
{
    if (IS_GD(mid))
    {
        prop->type = HAL_SPI_FLASH_TYPE_GD;
        prop->volatile_sr_en = true;
        prop->wp_gd_en = true;
        prop->wp_xmca_en = false;
        prop->uid_gd_en = true;
        prop->cpid_en = true;
        prop->qe_gd_en = true;
        prop->suspend_en = true;
        prop->sfdp_en = true;
        prop->sreg_3_en = true;
        prop->sreg_1_en = false;
        prop->write_sr12 = true;
        prop->has_sr2 = true;
        prop->init_sr_check = true;
        prop->uid_len = 8;
        prop->sreg_block_size = 512;
    }
    else if (IS_WINBOND(mid))
    {
        prop->type = HAL_SPI_FLASH_TYPE_WINBOND;
        prop->volatile_sr_en = true;
        prop->wp_gd_en = true;
        prop->wp_xmca_en = false;
        prop->uid_gd_en = true;
        prop->cpid_en = true;
        prop->qe_gd_en = true;
        prop->suspend_en = true;
        prop->sfdp_en = true;
        prop->sreg_3_en = true;
        prop->sreg_1_en = false;
        prop->write_sr12 = true;
        prop->has_sr2 = true;
        prop->init_sr_check = true;
        prop->uid_len = 8;
        prop->sreg_block_size = 256;
    }
    else if (IS_XTX(mid))
    {
        prop->type = HAL_SPI_FLASH_TYPE_XTX;
        prop->volatile_sr_en = true;
        prop->wp_gd_en = true;
        prop->wp_xmca_en = false;
        prop->uid_gd_en = false;
        prop->cpid_en = false;
        prop->qe_gd_en = true;
        prop->suspend_en = false;
        prop->sfdp_en = true;
        prop->sreg_3_en = false;
        prop->sreg_1_en = true;
        prop->write_sr12 = true;
        prop->has_sr2 = true;
        prop->init_sr_check = true;
        prop->uid_len = 16;
        prop->sreg_block_size = 1024;
    }
    else if (IS_XMCA(mid))
    {
        prop->type = HAL_SPI_FLASH_TYPE_XMCA;
        prop->volatile_sr_en = true;
        prop->wp_gd_en = false;
        prop->wp_xmca_en = true;
        prop->uid_gd_en = false;
        prop->cpid_en = false;
        prop->qe_gd_en = false;
        prop->suspend_en = false; // B0H/30H not supported
        prop->sfdp_en = true;
        prop->sreg_3_en = false;
        prop->sreg_1_en = false;
        prop->write_sr12 = false;
        prop->has_sr2 = false;
        prop->init_sr_check = false; // special handled
        prop->uid_len = 12;
        prop->sreg_block_size = 0; // OTP mode for security register not supported
    }
    else if (IS_XMCC(mid))
    {
        prop->type = HAL_SPI_FLASH_TYPE_XMCC;
        prop->volatile_sr_en = true;
        prop->wp_gd_en = true;
        prop->wp_xmca_en = false;
        prop->uid_gd_en = true;
        prop->cpid_en = false;
        prop->qe_gd_en = true;
        prop->suspend_en = true;
        prop->sfdp_en = true;
        prop->sreg_3_en = true;
        prop->sreg_1_en = false;
        prop->write_sr12 = false;
        prop->has_sr2 = true;
        prop->init_sr_check = true;
        prop->uid_len = 8;
        prop->sreg_block_size = 256;
    }
    else
    {
        prop->type = HAL_SPI_FLASH_TYPE_UNKNOWN;
        prop->volatile_sr_en = false;
        prop->wp_gd_en = false;
        prop->wp_xmca_en = false;
        prop->uid_gd_en = false;
        prop->cpid_en = false;
        prop->qe_gd_en = false;
        prop->suspend_en = false;
        prop->sfdp_en = false;
        prop->sreg_3_en = false;
        prop->sreg_1_en = false;
        prop->write_sr12 = false;
        prop->has_sr2 = false;
        prop->init_sr_check = false;
        prop->uid_len = 0;
        prop->sreg_block_size = 0;
    }

    prop->mid = mid;
    prop->capacity = CAPACITY_BYTES(mid);
}
