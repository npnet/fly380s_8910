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

configure_file(include/hal_config.h.in ${out_inc_dir}/hal_config.h)

if(NOT ${CONFIG_PINMUX_CSV_PATH} STREQUAL "")
    set(pindef_h ${out_inc_dir}/hal_iomux_pindef.h)
    set(pincfg_h ${out_inc_dir}/hal_iomux_pincfg.h)
    set(csv_pinmax ${SOURCE_TOP_DIR}/${CONFIG_PINMUX_CSV_PATH})
    execute_process(COMMAND python3 ${iomuxgen_py} ${csv_pinmax} ${pindef_h} ${pincfg_h})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${csv_pinmax})
endif()

if(NOT ${CONFIG_KEYPAD_CSV_PATH} STREQUAL "")
    set(keypadcfg_h ${out_inc_dir}/hal_keypad_def.h)
    set(csv_keypad ${SOURCE_TOP_DIR}/${CONFIG_KEYPAD_CSV_PATH})
    execute_process(COMMAND python3 ${keypadgen_py} ${csv_keypad} ${keypadcfg_h})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${csv_keypad})
endif()

set(target hal)
add_app_libraries($<TARGET_FILE:${target}>)

add_library(${target} STATIC ${dummy_c_file})
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_include_directories(${target} PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)
target_include_targets(${target} PUBLIC kernel)
target_include_targets(${target} PRIVATE calclib fs nvm driver)

if(CONFIG_SOC_8910)
    target_sources(${target} PRIVATE
        src/8910/hal_clock_init.c
        src/8910/hal_chip_8910.c
        src/8910/hal_pmu_8910.c
        src/8910/hal_aes.c
        src/hal_delay.c
        src/hal_hw_map.c
        src/hal_iomux.c
        src/hal_reg_apply.c
        src/hal_chip.c
        src/hal_mmu.c
        src/hal_shmem_region.c
        src/hal_adi_bus.c
        src/hal_hwspinlock.c
        src/hal_cp_ctrl.c
        src/hal_lzma.c
        src/hal_spi_flash.c
        src/hal_spi_flash_init.c
        src/hal_gouda.c
    )
endif()


relative_glob(srcs include/*.h src/*.c src/*.h
    include/8955/*.h include/8909/*.h include/8910/*.h)
beautify_c_code(${target} ${srcs})
