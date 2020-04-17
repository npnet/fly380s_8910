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

#undef mips
#define mips mips
OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(mips)
ENTRY(start_entry)
EXTERN(exception_handler)
EXTERN(bcpubin)

#ifdef CONFIG_SOC_8909
EXTERN(tlb_exception_handler)
#endif

#define GOTO_CACHABLE . -= 0x20000000
#define GOTO_UNCACHABLE . += 0x20000000

#define FLASH_SECT(name)                            \
    .name __flash_location : AT(ADDR(.name))        \
    {                                               \
        __##name##_start = .;                       \
        name##_contents                             \
        __##name##_end = .;                         \
        . = ALIGN(0x10);                            \
    }                                               \
    __flash_location += SIZEOF(.name);

#define FLASH_LOAD_SECT(name)                       \
    .name . : AT(__flash_location)                  \
    {                                               \
        __##name##_start = .;                       \
        name##_contents                             \
        __##name##_end = .;                         \
        . = ALIGN(0x10);                            \
        __##name##_load_start = LOADADDR(.name);    \
    }                                               \
    __flash_location += SIZEOF(.name);

#define NOLOAD_SECT(name)                           \
    .name . (NOLOAD) : AT(ADDR(.name))              \
    {                                               \
        __##name##_start = .;                       \
        name##_contents                             \
        . = ALIGN(0x10);                            \
        __##name##_end = .;                         \
    }                                               \

#define SKIP_CHECK(start, end, err)                 \
    ASSERT(. <= start, err)                         \
    . = end;                                        \
    . = ALIGN(0x10);                                \

#define start_entry_contents \
        LONG(0xFFFFFFFF) \
        LONG(0) \
        LONG(0) \
        LONG(0) \
        *(.start_entry) \
        . = ALIGN(0x40);

#define DEF_RO .text .text.* .gnu.linkonce.t.* .rdata .rodata .rodata.str1.4 .rodata.* .rodata1
#define DEF_RW .data .data.* .sdata .sdata.* .data1
#define DEF_ZI .bss .bss.* COMMON .scommon .sbss .sbss.* .sbss2 .sbss2.* .gnu.linkonce.b.*

#define FLASH_RAMTEXT           *libdriver.a:drv_spi_flash.*(DEF_RO) *libhal.a:hal_spi_flash.*(DEF_RO)
#define RFD_RAMTEXT             *drv_xcv_*(DEF_RO)
#define NBIOT_RAMTEXT           *libnbiot.a:nl1c*(DEF_RO) *libnbiot.a:tra*(DEF_RO) *libnbiot.a:osi*(DEF_RO)
#define LIBGCC_RAMTEXT          *libgcc.a:*(DEF_RO)
#define KERNEL_RAMTEXT          *libkernel.a:*(DEF_RO) *libosi_core.a:*(DEF_RO) *libosi_lib.a:*(DEF_RO) *libsx.a:*(DEF_RO)
#define HAL_RAMTEXT             *libhal.a:*(DEF_RO) *libmodem_hal.a:*(DEF_RO)

#define bcpubin_contents        KEEP(bcpubin.*(.rodata))
#define exception_contents      *(.exception)
#define sramtext_contents       *(.sramtext .sramtext.* .syssram_L1_text) FLASH_RAMTEXT
#define sramboottext_contents   *(.sramboottext .sramboottext.*)
#define sramdata_contents       *(.sramdata .sramdata.* .bootsramdata .bootsramdata.*)
#define srambss_contents        *(.srambss .srambss.*)
#define sramuninit_contents     *(.sramuninit .sramuninit.*)
#define sramucdata_contents     *(.sramucdata .sramucdata.*)
#define sramucbss_contents      *(.sramucbss .sramucbss.*)
#define sramucuninit_contents   *(.sramucuninit .sramucuninit.*)
#define tlb_exception_contents  *(.tlb_exception)
#define ramtext_contents        *(.ram) NBIOT_RAMTEXT RFD_RAMTEXT LIBGCC_RAMTEXT KERNEL_RAMTEXT
#define ramdata_contents        *(DEF_RW) KEEP(*(.rwkeep))
#define rambss_contents         *(DEF_ZI)
#define ramuninit_contents      *(.ramuninit .ramuninit.*)
#define ramucdata_contents      *(.ramucdata .ramucdata.* .ucdata .ucdata.*)
#define ramucbss_contents       *(.ramucbss .ramucbss.* .ucbss .ucbass.*)
#define ramucuninit_contents    *(.ramucuninit .ramucuninit.* .ucbackup)
#define flashtext_contents      *(.boottext DEF_RO) KEEP(*(.rokeep))
#define init_array_contents     KEEP(*(.init_array*))

MEMORY {
    flash(rx): ORIGIN = CONFIG_NOR_PHY_ADDRESS+CONFIG_APP_FLASH_OFFSET,
               LENGTH = CONFIG_APP_FLASH_SIZE
    ram(rwx):  ORIGIN = CONFIG_RAM_PHY_ADDRESS+CONFIG_APP_RAM_OFFSET,
               LENGTH = CONFIG_APP_RAM_SIZE
    sram(rwx): ORIGIN = CONFIG_SRAM_PHY_ADDRESS+CONFIG_APP_SRAM_OFFSET,
               LENGTH = CONFIG_APP_SRAM_SIZE
}

SECTIONS {
    PROVIDE(g_sppMapAccess = 0);
#include "bcpu_for_xcpu.ld"
#include "sysrom_for_xcpu.ld"

    . = ORIGIN(flash);
    HIDDEN(__flash_location = ORIGIN(flash));

    FLASH_SECT(start_entry)

    ASSERT(. == CONFIG_NOR_PHY_ADDRESS + CONFIG_BCPU_FLASH_OFFSET,
           "CONFIG_BCPU_FLASH_OFFSET is wrong")

    FLASH_SECT(bcpubin)

    . = ORIGIN(sram);
    __fixptr = ORIGIN(sram) - 4;
    __sram_start = .;
    __sram_end = ORIGIN(sram) + LENGTH(sram);

    FLASH_LOAD_SECT(exception)
    SKIP_CHECK(_bcpu_sys_sram_start, _bcpu_sys_sram_end, "_bcpu_sys_sram_start is wrong")
    FLASH_LOAD_SECT(sramtext)
    FLASH_LOAD_SECT(sramboottext)
    FLASH_LOAD_SECT(sramdata)
    NOLOAD_SECT(srambss)
    NOLOAD_SECT(sramuninit)

    GOTO_UNCACHABLE;
    FLASH_LOAD_SECT(sramucdata)
    NOLOAD_SECT(sramucbss)
    NOLOAD_SECT(sramucuninit)
    GOTO_CACHABLE;

    ASSERT(. + CONFIG_IRQ_STACK_SIZE <= __sram_end, "SRAM overflow")

    __sram_heap_start = .;
    __sram_heap_end = __sram_end - CONFIG_IRQ_STACK_SIZE;
    __sram_irq_stack_bottom = __sram_end - CONFIG_IRQ_STACK_SIZE;
    __sram_irq_stack_top = __sram_end;
    __sram_blue_screen_stack_top = __sram_irq_stack_bottom + CONFIG_BLUE_SCREEN_STACK_SIZE;

    . = ORIGIN(ram);
    __ram_start = .;
    __ram_end = ORIGIN(ram) + LENGTH(ram);

#ifdef CONFIG_SOC_8909
    FLASH_LOAD_SECT(tlb_exception)
#endif
    SKIP_CHECK(_bcpu_ram_start, _bcpu_ram_end, "_bcpu_ram_start is wrong")
    FLASH_LOAD_SECT(ramtext)
    FLASH_LOAD_SECT(ramdata)
    NOLOAD_SECT(rambss)
    NOLOAD_SECT(ramuninit)

    GOTO_UNCACHABLE;
    FLASH_LOAD_SECT(ramucdata)
    NOLOAD_SECT(ramucbss)
    NOLOAD_SECT(ramucuninit)
    GOTO_CACHABLE;

    ASSERT(. <= __ram_end, "RAM overflow")

    __heap_start = .;
    __heap_end = __ram_end;

    FLASH_SECT(init_array)
    FLASH_SECT(flashtext)

    __flash_start = ORIGIN(flash);
    __flash_end = __flash_location;

    ASSERT(__flash_end <= ORIGIN(flash) + LENGTH(flash), "FLASH overflow")

    _calib_start = CONFIG_NOR_PHY_ADDRESS+CONFIG_CALIB_GSM_FLASH_OFFSET;
#ifdef CONFIG_CALIB_NB_FLASH_OFFSET
    _ncalib_start = CONFIG_NOR_PHY_ADDRESS+CONFIG_CALIB_NB_FLASH_OFFSET;
#endif
    _factory_start = CONFIG_NOR_PHY_ADDRESS+CONFIG_FACTORY_FLASH_OFFSET;

    .comment 0 : { *(.comment) }
    .MIPS.abiflags 0 : {*(.MIPS.abiflags) }
    /DISCARD/ : { *(.dynstr*) }
    /DISCARD/ : { *(.dynamic*) }
    /DISCARD/ : { *(.plt*) }
    /DISCARD/ : { *(.interp*) }
    /DISCARD/ : { *(.gnu*) }
}
