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

#include "hal_config.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)
EXTERN(Reset_Handler)

#define SECT(name) {                            \
    __##name##_start = .;                       \
    name##_contents                             \
    __##name##_end = .;                         \
    . = ALIGN(0x20);                            \
    __##name##_load_start = LOADADDR(.name);    \
}

#define DEF_RO .text .text.* .gnu.linkonce.t.* .rdata .rodata .rodata.* .gnu.linkonce.r.*
#define DEF_RW .data .data.* .gnu.linkonce.d.*
#define DEF_ZI .bss .bss.* COMMON .scommon .sbss .sbss.* .sbss2 .sbss2.* .gnu.linkonce.b.*

#define FLASH_RAMTEXT           *libdriver.a:drv_spi_flash.*(DEF_RO) *libhal.a:hal_spi_flash.*(DEF_RO)

#define sramboottext_contents   *(.sramboottext .sramboottext.*)
#define sramtext_contents       *(VECTORS) FLASH_RAMTEXT \
                                *libkernel.a:portASM.*(DEF_RO) \
                                *libkernel.a:port.*(DEF_RO) \
                                *(.sramtext .sramtext.*)
#define text_contents           *libkernel.a:boot_start.*(DEF_RO) \
                                *libkernel.a:osi_debug.*(DEF_RO) \
                                *libhal.a:hal_clock_init.*(DEF_RO) \
                                *libhal.a:hal_mmu.*(DEF_RO)
#define ramtext_contents        *(DEF_RO) *(.ramtext .ramtext.*)
#define srambootdata_contents   *(.srambootdata .srambootdata.*)
#define sramdata_contents       *(.sramdata .sramdata.*)
#define data_contents           *(DEF_RW)
#define srambootbss_contents    *(.srambootbss .srambootbss.*)
#define srambss_contents        *(.srambss .srambss.*)
#define bss_contents            *(DEF_ZI)
#define init_array_contents     KEEP(*(.init_array*))

MEMORY {
    flash(rx): ORIGIN = CONFIG_NOR_PHY_ADDRESS+CONFIG_APP_FLASH_OFFSET+CONFIG_UIMAGE_HEADER_SIZE,
               LENGTH = CONFIG_APP_FLASH_SIZE-CONFIG_UIMAGE_HEADER_SIZE
    ram(rwx):  ORIGIN = CONFIG_RAM_PHY_ADDRESS+CONFIG_APP_RAM_OFFSET,
               LENGTH = CONFIG_APP_RAM_SIZE
    sram(rwx): ORIGIN = CONFIG_SRAM_PHY_ADDRESS+CONFIG_APP_SRAM_OFFSET,
               LENGTH = CONFIG_APP_SRAM_SIZE
}

SECTIONS {
    .ttb : ALIGN(16384) {
        *(.TTBL1)
        *(.TTBL2)
    } > ram

    .entry : {
        *(RESET)
    } >flash

    .sramboottext : SECT(sramboottext) >sram AT>flash
    .sramtext : SECT(sramtext) >sram AT>flash
    .init_array : SECT(init_array) > flash
    .text : SECT(text) >flash
    .ramtext : SECT(ramtext) >ram AT>flash
    .srambootdata : SECT(srambootdata) >sram AT>flash
    .sramdata : SECT(sramdata) > sram AT>flash
    .data : SECT(data) > ram AT>flash
    .srambootbss : SECT(srambootbss) >sram
    .srambss : SECT(srambss) >sram
    .bss  : SECT(bss) >ram

    __flash_start = ORIGIN(flash);
    __flash_end = __data_load_start + SIZEOF(.data);

    __stack_top = ORIGIN(sram) + LENGTH(sram);
    __fiq_stack_top = __stack_top - CONFIG_FIQ_STACK_TOP;
    __irq_stack_top = __stack_top - CONFIG_IRQ_STACK_TOP;
    __svc_stack_top = __stack_top - CONFIG_SVC_STACK_TOP;
    __abt_stack_top = __stack_top - CONFIG_ABT_STACK_TOP;
    __und_stack_top = __stack_top - CONFIG_UND_STACK_TOP;
    __sys_stack_top = __stack_top - CONFIG_SYS_STACK_TOP;
    __stack_bottom = __stack_top - CONFIG_STACK_BOTTOM;

    __heap_start = __bss_end;
    __heap_end = ORIGIN(ram) + LENGTH(ram);

    .comment 0 : { *(.comment) }
    /DISCARD/ : { *(.dynstr*) }
    /DISCARD/ : { *(.dynamic*) }
    /DISCARD/ : { *(.plt*) }
    /DISCARD/ : { *(.interp*) }
    /DISCARD/ : { *(.gnu*) }
}
