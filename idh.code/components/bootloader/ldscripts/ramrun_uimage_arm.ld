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

// This will link an arm elf for uimage. The definitions needed:
// * RAMRUN_IMAGE_START: The sram start address, including uimage header.
// * RAMRUN_IMAGE_SIZE: The sram size.
// * RAMRUN_FLASH_SIZE: The maximum romable size. When defined, it is only
//      used to check romable size.
// * RAMRUN_MMU_ENABLE: 1 for MMU enabled, 0 for disabled
// * CONFIG_UIMAGE_HEADER_SIZE: uimage header size.
//
// Address between [__flash_start, __flash_end] is romable.

#include "link_defs.ld"
#include "boot_config.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(bootEntry)
EXTERN(bootEntry)

#define text_contents       *(BOOT_ENTRY) \
                            SORT(*)(.sramboottext .sramboottext.*) \
                            SORT(*)(.sramtext .sramtext.*) \
                            SORT(*)(.ramtext .ramtext.*) \
                            SORT(*)(DEF_RO) \
                            KEEP(*(.rokeep))
#define data_contents       SORT(*)(.sramdata .sramdata.*) \
                            SORT(*)(DEF_RW) \
                            KEEP(*(.rwkeep))
#define bss_contents        *(.srambss .srambss.*) \
                            *(DEF_ZI)

MEMORY {
    sram(rwx): ORIGIN = RAMRUN_IMAGE_START+CONFIG_UIMAGE_HEADER_SIZE,
               LENGTH = RAMRUN_IMAGE_SIZE-CONFIG_UIMAGE_HEADER_SIZE
}

SECTIONS {
    NOAT_SECT(text) > sram
    NOAT_SECT(data) > sram
    ANCHOR_SECT(lastflash) > sram
    NOLOAD_SECT(bss) > sram
    ANCHOR_SECT(lastsram) > sram

    __flash_start = LOADADDR(.text);
    __flash_end = ADDR(.lastflash);

#ifdef RAMRUN_FLASH_SIZE
    ASSERT(__flash_end - RAMRUN_IMAGE_START <= RAMRUN_FLASH_SIZE, "flash overflow")
#endif

#if defined(RAMRUN_MMU_ENABLE) && (RAMRUN_MMU_ENABLE)
    __mmu_ttbl1_address = ORIGIN(sram) + LENGTH(sram) - 0x4000;
    __mmu_ttbl2_address = ORIGIN(sram) + LENGTH(sram) - 0x4400;
    __sram_top = __mmu_ttbl2_address;
    ASSERT(__mmu_ttbl1_address % 16384 == 0, "TTB L1 is not aligned")
#else
    __sram_top = ORIGIN(sram) + LENGTH(sram);
#endif

    __sram_end = ADDR(.lastsram);
    ASSERT(__sram_end <= __sram_top, "sram overflow")

    __sram_heap_start = __sram_end;
    __sram_heap_end = __sram_top;
    __blue_screen_end = CONFIG_BOOT_BLUE_SCREEN_STACK_TOP;

    IGNORE_DISCARD_SECTS
}
