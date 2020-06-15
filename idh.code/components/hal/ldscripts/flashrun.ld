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

#include "link_defs.ld"

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)
EXTERN(Reset_Handler)

#define ttb_contents            *(.TTBL1) *(.TTBL2)
#define entry_contents          *(RESET)
#define sramboottext_contents   SORT(*)(.sramboottext .sramboottext.*)
#define sramtext_contents       *(VECTORS) \
                                *libkernel.a:portASM.*(DEF_RO) \
                                *libkernel.a:port.*(DEF_RO) \
                                *libkernel.a:osi_irq.*(DEF_RO) \
                                *libkernel.a:osi_profile.*(DEF_RO) \
                                SORT(*)(.sramtext .sramtext.*)
#define ramtext_contents        SORT(*)(.ramtext .ramtext.*)
#define text_contents           SORT(*)(DEF_RO) KEEP(*(.rokeep))
#define sramdata_contents       SORT(*)(.sramdata .sramdata.*)
#define data_contents           SORT(*)(DEF_RW) KEEP(*(.rwkeep))
#define srambss_contents        *(.srambss .srambss.*)
#define bss_contents            *(DEF_ZI)
#define init_array_contents     KEEP(*(.init_array*))

#ifdef CONFIG_APP_FLASH2_ENABLED
/* libraries on external flash, should be tuned by requirement. */
#define flash2text_contents     SORT(*libonextflash.a:*)(DEF_RO)
#endif

MEMORY {
    flash(rx): ORIGIN = CONFIG_APP_FLASH_ADDRESS+CONFIG_UIMAGE_HEADER_SIZE,
               LENGTH = CONFIG_APP_FLASH_SIZE-CONFIG_UIMAGE_HEADER_SIZE
    ram(rwx):  ORIGIN = CONFIG_RAM_PHY_ADDRESS+CONFIG_APP_RAM_OFFSET,
               LENGTH = CONFIG_APP_RAM_SIZE
    sram(rwx): ORIGIN = CONFIG_SRAM_PHY_ADDRESS+CONFIG_APP_SRAM_OFFSET,
               LENGTH = CONFIG_APP_SRAM_SIZE
#ifdef CONFIG_APP_FLASH2_ENABLED
    flash2(rx): ORIGIN = CONFIG_APP_FLASH2_ADDRESS,
                LENGTH = CONFIG_APP_FLASH2_SIZE
#endif
}

SECTIONS {
    NOAT_SECT_ALIGNED(ttb, 16384) > ram
    NOAT_SECT(entry) > flash
    LOAD_SECT(sramboottext) >sram AT>flash
    LOAD_SECT(sramtext) >sram AT>flash
    LOAD_SECT(ramtext) >ram AT>flash
    NOAT_SECT(init_array) > flash
#ifdef CONFIG_APP_FLASH2_ENABLED
    NOAT_SECT(flash2text) > flash2
    ANCHOR_SECT(lastflash2) > flash2
    __flash2_start = ORIGIN(flash2);
    __flash2_end = ADDR(.lastflash2);
#endif
    NOAT_SECT(text) > flash
    LOAD_SECT(sramdata) >sram AT>flash
    LOAD_SECT(data) >ram AT>flash
    NOLOAD_SECT(srambss) > sram
    ANCHOR_SECT(lastsram) > sram
    NOLOAD_SECT(bss) > ram
    ANCHOR_SECT(lastram) > ram
    ANCHOR_SECT(lastflash) > flash

    __flash_start = ORIGIN(flash);
    __flash_end = ADDR(.lastflash);
    __sram_start = ORIGIN(sram);
    __sram_end = ADDR(.lastsram);
    __ram_start = ORIGIN(ram);
    __ram_end = ADDR(.lastram);

    __stack_top = ORIGIN(sram) + LENGTH(sram);
    __fiq_stack_top = __stack_top - CONFIG_FIQ_STACK_TOP;
    __irq_stack_top = __stack_top - CONFIG_IRQ_STACK_TOP;
    __svc_stack_top = __stack_top - CONFIG_SVC_STACK_TOP;
    __abt_stack_top = __stack_top - CONFIG_ABT_STACK_TOP;
    __und_stack_top = __stack_top - CONFIG_UND_STACK_TOP;
    __sys_stack_top = __stack_top - CONFIG_SYS_STACK_TOP;
    __stack_bottom = __stack_top - CONFIG_STACK_BOTTOM;
    __blue_screen_end = __und_stack_top;

    ASSERT(__stack_bottom > __sram_end, "SRAM overflow")

    __heap_start = __ram_end;
    __heap_end = ORIGIN(ram) + LENGTH(ram);

    IGNORE_DISCARD_SECTS
}
