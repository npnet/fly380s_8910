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
// * CONFIG_UIMAGE_HEADER_SIZE: uimage header size.
//
// Address between [__flash_start, __flash_end] is romable.

#include "hal_config.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(bootEntry)
EXTERN(bootEntry)

#define ROTEXT() .text .text.* .sramboottext .sramtext .ramtext .gnu.linkonce.t.* .glue_7 .glue_7t .vfp11_veneer .v4_bx
#define RODATA() .rdata .rodata .rodata.* .gnu.linkonce.r.*
#define DATA() .data .data.* .sramdata .ramdata .gnu.linkonce.d.*
#define BSS() .bss .bss.* .srambss .rambss COMMON .scommon .sbss .sbss.* .sbss2 .sbss2.* .gnu.linkonce.b.*

MEMORY {
    sram(rwx): ORIGIN = RAMRUN_IMAGE_START, LENGTH = RAMRUN_IMAGE_SIZE
}

SECTIONS {
    . = ORIGIN(sram);

    // skip uimage header
    . += CONFIG_UIMAGE_HEADER_SIZE;
    __flash_start = .;

    .text . : {
        *(BOOT_ENTRY)
        *(ROTEXT() RODATA())
        . = ALIGN(CONFIG_CACHE_LINE_SIZE);
    }

    .data . : {
        *(DATA())
        . = ALIGN(CONFIG_CACHE_LINE_SIZE);
    }
    __flash_end = .;

#ifdef RAMRUN_FLASH_SIZE
    ASSERT(__flash_end - RAMRUN_IMAGE_START <= RAMRUN_FLASH_SIZE, "flash overflow")
#endif

    .bss . : {
        __bss_start = .;
        *(BSS())
        __bss_end = .;
        . = ALIGN(CONFIG_CACHE_LINE_SIZE);
    }

    __sram_end = .;
    __sram_top = RAMRUN_IMAGE_START + RAMRUN_IMAGE_SIZE;

    /DISCARD/ : { *(.iplt*) }
    /DISCARD/ : { *(.igot*) }
    /DISCARD/ : { *(.rel*) }
    /DISCARD/ : { *(.dynstr*) }
    /DISCARD/ : { *(.dynamic*) }
    /DISCARD/ : { *(.plt*) }
    /DISCARD/ : { *(.interp*) }
    /DISCARD/ : { *(.gnu*) }
}
