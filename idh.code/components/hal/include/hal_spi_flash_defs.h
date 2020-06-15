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

#ifndef _HAL_SPI_FLASH_DEFS_H_
#define _HAL_SPI_FLASH_DEFS_H_

#include "osi_api.h"
#include "hwregs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    HAL_SPI_FLASH_TYPE_GD,
    HAL_SPI_FLASH_TYPE_WINBOND,
    HAL_SPI_FLASH_TYPE_XTX,
    HAL_SPI_FLASH_TYPE_XMCA,
    HAL_SPI_FLASH_TYPE_XMCC,
    HAL_SPI_FLASH_TYPE_UNKNOWN = 0xff,
} halSpiFlashType_t;

#define DELAY_AFTER_RESET (100)                  // us, tRST(20), tRST_E(12ms) won't happen
#define DELAY_AFTER_RELEASE_DEEP_POWER_DOWN (50) // us, tRES1(20), tRES2(20)

#define MID(mid) ((mid)&0xff)
#define MEMTYPE(mid) (((mid) >> 8) & 0xff)
#define CAPACITY_BITS(mid) ((((mid) >> 16) & 0xff))
#define CAPACITY_BYTES(mid) (1 << CAPACITY_BITS(mid))

#define SIZE_4K (4 * 1024)
#define SIZE_8K (8 * 1024)
#define SIZE_16K (16 * 1024)
#define SIZE_32K (32 * 1024)
#define SIZE_64K (64 * 1024)
#define PAGE_SIZE (256)

// status register bits, based on GD
#define STREG_WIP REG_BIT(0)
#define STREG_WEL REG_BIT(1)
#define STREG_BP0 REG_BIT(2)
#define STREG_BP1 REG_BIT(3)
#define STREG_BP2 REG_BIT(4)
#define STREG_BP3 REG_BIT(5)
#define STREG_BP4 REG_BIT(6)
#define STREG_QE REG_BIT(9)
#define STREG_SUS2 REG_BIT(10)
#define STREG_LB1 REG_BIT(11) // sreg_3_en
#define STREG_LB2 REG_BIT(12) // sreg_3_en
#define STREG_LB3 REG_BIT(13) // sreg_3_en
#define STREG_CMP REG_BIT(14)
#define STREG_SUS1 REG_BIT(15)

#define STREG_LB REG_BIT(10) // sreg_1_en

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
#define OPCODE_READ_SFDP 0x5a
#define OPCODE_ULTRA_POWER_DOWN 0x79
#define OPCODE_ENTER_OTP_MODE 0x3a // XMCA
#define OPCODE_EXIT_OPT_MODE 0x04  // XMCA, same as write disable

#define OPNAND_WRITE_ENABLE 0x06
#define OPNAND_WRITE_DISABLE 0x04
#define OPNAND_GET_FEATURE 0x0f
#define OPNAND_SET_FEATURE 0x1f
#define OPNAND_PAGE_READ_TO_CACHE 0x13
#define OPNAND_PAGE_READ 0x03 // or 0x0b
#define OPNAND_PAGE_READ_X4 0x6b
#define OPNAND_PAGE_READ_QUAD 0xeb
#define OPNAND_READ_ID 0x9f
#define OPNAND_PROGRAM_LOAD 0x02
#define OPNAND_PROGRAM_LOAD_X4 0x32
#define OPNAND_PROGRAM_EXEC 0x10
#define OPNAND_PROGRAM_LOAD_RANDOM 0x84
#define OPNAND_PROGRAM_LOAD_RANDOM_X4 0xc4 // or 0x34
#define OPNAND_PROGRAM_LOAD_RANDOM_QUAD 0x72
#define OPNAND_BLOCK_ERASE 0xd8
#define OPNAND_RESET 0xff

#define BITSEL(sel, val) ((sel) ? (val) : 0)
#define WP_GD_BITEXP(n) (BITSEL((n)&1, STREG_BP0) |  \
                         BITSEL((n)&2, STREG_BP1) |  \
                         BITSEL((n)&4, STREG_BP2) |  \
                         BITSEL((n)&8, STREG_BP3) |  \
                         BITSEL((n)&16, STREG_BP4) | \
                         BITSEL((n)&32, STREG_CMP))

// This matches GD/WINBOND 4/8/16 MB flash. Though 8/16 MB flash
// support more options to open-protect smaller area at upper address.
// However, it is not worth to bringin this complexity.
#define WP_GD32M_MASK WP_GD_BITEXP(0b111111)
#define WP_GD32M_NONE WP_GD_BITEXP(0b000000)
#define WP_GD32M_4K WP_GD_BITEXP(0b011001)
#define WP_GD32M_8K WP_GD_BITEXP(0b011010)
#define WP_GD32M_16K WP_GD_BITEXP(0b011011)
#define WP_GD32M_32K WP_GD_BITEXP(0b011100)
#define WP_GD32M_1_64 WP_GD_BITEXP(0b001001)
#define WP_GD32M_1_32 WP_GD_BITEXP(0b001010)
#define WP_GD32M_1_16 WP_GD_BITEXP(0b001011)
#define WP_GD32M_1_8 WP_GD_BITEXP(0b001100)
#define WP_GD32M_1_4 WP_GD_BITEXP(0b001101)
#define WP_GD32M_1_2 WP_GD_BITEXP(0b001110)
#define WP_GD32M_3_4 WP_GD_BITEXP(0b100101)
#define WP_GD32M_7_8 WP_GD_BITEXP(0b100100)
#define WP_GD32M_15_16 WP_GD_BITEXP(0b100011)
#define WP_GD32M_31_32 WP_GD_BITEXP(0b100010)
#define WP_GD32M_63_64 WP_GD_BITEXP(0b100001)
#define WP_GD32M_ALL WP_GD_BITEXP(0b011111)

// This matches GD/WINBOND 2MB flash.
#define WP_GD16M_MASK WP_GD_BITEXP(0b111111)
#define WP_GD16M_NONE WP_GD_BITEXP(0b000000)
#define WP_GD16M_4K WP_GD_BITEXP(0b011001)
#define WP_GD16M_8K WP_GD_BITEXP(0b011010)
#define WP_GD16M_16K WP_GD_BITEXP(0b011011)
#define WP_GD16M_32K WP_GD_BITEXP(0b011100)
#define WP_GD16M_1_32 WP_GD_BITEXP(0b001001)
#define WP_GD16M_1_16 WP_GD_BITEXP(0b001010)
#define WP_GD16M_1_8 WP_GD_BITEXP(0b001011)
#define WP_GD16M_1_4 WP_GD_BITEXP(0b001100)
#define WP_GD16M_1_2 WP_GD_BITEXP(0b001101)
#define WP_GD16M_3_4 WP_GD_BITEXP(0b100100)
#define WP_GD16M_7_8 WP_GD_BITEXP(0b100011)
#define WP_GD16M_15_16 WP_GD_BITEXP(0b100010)
#define WP_GD16M_31_32 WP_GD_BITEXP(0b100001)
#define WP_GD16M_ALL WP_GD_BITEXP(0b011111)

// This matches GD/WINBOND 1MB flash.
#define WP_GD8M_MASK WP_GD_BITEXP(0b111111)
#define WP_GD8M_NONE WP_GD_BITEXP(0b000000)
#define WP_GD8M_4K WP_GD_BITEXP(0b011001)
#define WP_GD8M_8K WP_GD_BITEXP(0b011010)
#define WP_GD8M_16K WP_GD_BITEXP(0b011011)
#define WP_GD8M_32K WP_GD_BITEXP(0b011100)
#define WP_GD8M_1_16 WP_GD_BITEXP(0b001001)
#define WP_GD8M_1_8 WP_GD_BITEXP(0b001010)
#define WP_GD8M_1_4 WP_GD_BITEXP(0b001011)
#define WP_GD8M_1_2 WP_GD_BITEXP(0b001100)
#define WP_GD8M_3_4 WP_GD_BITEXP(0b100011)
#define WP_GD8M_7_8 WP_GD_BITEXP(0b100010)
#define WP_GD8M_15_16 WP_GD_BITEXP(0b100001)
#define WP_GD8M_ALL WP_GD_BITEXP(0b011111)

#define WP_XMCA_BITEXP(n) (BITSEL((n)&1, REG_BIT2) | \
                           BITSEL((n)&2, REG_BIT3) | \
                           BITSEL((n)&4, REG_BIT4) | \
                           BITSEL((n)&8, REG_BIT5))

#define WP_XMCA_MASK WP_XMCA_BITEXP(0b1111)
#define WP_XMCA_NONE WP_XMCA_BITEXP(0b0000)
#define WP_XMCA_1_128 WP_XMCA_BITEXP(0b0001)
#define WP_XMCA_2_128 WP_XMCA_BITEXP(0b0010)
#define WP_XMCA_4_128 WP_XMCA_BITEXP(0b0011)
#define WP_XMCA_8_128 WP_XMCA_BITEXP(0b0100)
#define WP_XMCA_16_128 WP_XMCA_BITEXP(0b0101)
#define WP_XMCA_32_128 WP_XMCA_BITEXP(0b0110)
#define WP_XMCA_64_128 WP_XMCA_BITEXP(0b0111)
#define WP_XMCA_96_128 WP_XMCA_BITEXP(0b1000)
#define WP_XMCA_112_128 WP_XMCA_BITEXP(0b1001)
#define WP_XMCA_120_128 WP_XMCA_BITEXP(0b1010)
#define WP_XMCA_124_128 WP_XMCA_BITEXP(0b1011)
#define WP_XMCA_126_128 WP_XMCA_BITEXP(0b1100)
#define WP_XMCA_127_128 WP_XMCA_BITEXP(0b1101)
#define WP_XMCA_ALL WP_XMCA_BITEXP(0b1111)

#ifdef __cplusplus
}
#endif
#endif
