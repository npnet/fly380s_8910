# Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
# All rights reserved.
#
# This software is supplied "AS IS" without any warranties.
# RDA assumes no responsibility or liability for the use of the software,
# conveys no license or title under any patent, copyright, or mask work
# right to the product. RDA reserves the right to make changes in the
# software without notification.  RDA also make no representation or
# warranty that such application will be suitable for the specified use
# without further testing or modification.

configure_file(include/boot_config.h.in ${BINARY_TOP_DIR}/include/boot_config.h)

if(CONFIG_SOC_8910)
    set(target bootloader)
    add_library(${target} STATIC
        src/boot_entry.S
        src/boot_sffs_port.c
        src/boot_platform.c
        src/boot_mem.c
        src/boot_spi_flash.c
        src/boot_debuguart.c
        src/boot_trace.c
        src/boot_fdl.c
        src/boot_secure.c
        src/boot_aes.c
        src/boot_vfs.c
        src/boot_irq.c
        src/boot_timer.c
        src/boot_adi_bus.c
        src/boot_uart.c
        src/boot_mmu.c
        src/boot_udc.c
        src/boot_usb_fdl_channel.c
    )
    set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
    target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_BOOT)
    target_include_directories(${target} PUBLIC include)
    target_link_libraries(${target} PRIVATE boot_encrypt calclib)
    target_include_targets(${target} PRIVATE hal fs sffs bdev driver)

    # It is a dirty hack, create source for calibparam.bin
    # There may multiple nvm for one project, the first one will be used.
    # And it is assumed that calibparam should be the same in all nvm.
    list(GET CONFIG_NVM_VARIANTS 0 nvmvariant)
    set(calibparam_bin ${out_hex_dir}/calibparam.bin)
    set(nvitem_bin ${out_hex_dir}/${nvmvariant}_nvitem.bin)
    add_custom_command(OUTPUT ${calibparam_bin}
        COMMAND python3 ${nvbintool_py} extract ${nvitem_bin} 2 ${calibparam_bin}
        DEPENDS ${nvbintool_py} ${nvitem_bin}
    )
    target_incbin_size(${target} ${calibparam_bin} gDefCalibParam gDefCalibParamSize)

    set(rmarun_ld ldscripts/ramrun_uimage_arm.ld)

    set(target boot)
    add_uimage(${target} ${rmarun_ld} boot/boot_start.c)
    target_compile_definitions(${target}_ldscript PRIVATE
        RAMRUN_IMAGE_START=${CONFIG_BOOT_IMAGE_START}
        RAMRUN_IMAGE_SIZE=${CONFIG_BOOT_IMAGE_SIZE}
        RAMRUN_FLASH_SIZE=${CONFIG_BOOT_UNSIGN_IMAGE_SIZE}
    )
    target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_BOOT)
    target_link_whole_archive(${target} PRIVATE bootloader)
    target_link_group(${target} PRIVATE hal osi_lib fupdate fsmount sffs bdev)
    target_include_targets(${target} PRIVATE hal fs sffs bdev fupdate)
    target_link_libraries(${target} PRIVATE ${libc_file_name} ${libgcc_file_name})

    set(target fdl1)
    add_uimage(${target} ${rmarun_ld} fdl1/fdl1_main.c)
    target_compile_definitions(${target}_ldscript PRIVATE
        RAMRUN_IMAGE_START=${CONFIG_FDL1_IMAGE_START}
        RAMRUN_IMAGE_SIZE=${CONFIG_FDL1_IMAGE_SIZE}
    )
    target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_BOOT)
    target_link_whole_archive(${target} PRIVATE bootloader)
    target_link_group(${target} PRIVATE hal osi_lib)
    target_link_libraries(${target} PRIVATE ${libc_file_name} ${libgcc_file_name})

    set(target fdl2)
    add_uimage(${target} ${rmarun_ld} fdl2/fdl2_main.c)
    target_compile_definitions(${target}_ldscript PRIVATE
        RAMRUN_IMAGE_START=${CONFIG_FDL2_IMAGE_START}
        RAMRUN_IMAGE_SIZE=${CONFIG_FDL2_IMAGE_SIZE}
    )
    target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_BOOT)
    target_include_targets(${target} PRIVATE fs fsmount nvm calclib cpio_parser)
    target_link_whole_archive(${target} PRIVATE bootloader cpio_parser)
    target_link_group(${target} PRIVATE hal calclib sffs fsmount bdev osi_lib nvm)
    target_link_libraries(${target} PRIVATE ${libc_file_name} ${libgcc_file_name})

    set(target flash_test_fdl)
    add_uimage(${target} ${rmarun_ld} flash_test_fdl/flash_test_main.c)
    target_compile_definitions(${target}_ldscript PRIVATE
        RAMRUN_IMAGE_START=${CONFIG_FDL1_IMAGE_START}
        RAMRUN_IMAGE_SIZE=${CONFIG_FDL1_IMAGE_SIZE}
    )
    target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_BOOT)
    target_link_whole_archive(${target} PRIVATE bootloader)
    target_link_group(${target} PRIVATE hal osi_lib)
    target_link_libraries(${target} PRIVATE ${libc_file_name} ${libgcc_file_name})
endif()

relative_glob(srcs include/*.h src/*.c src/*.h
    fdl1/*.c fdl1/*.h
    fdl2/*.c fdl2/*.h
    flash_test_fdl/*.c flash_test_fdl/*.h
    boot/*.c boot/*.h
    uart_ramrun/*.c uart_ramrun/*.h
)
beautify_c_code(bootloader ${srcs})
