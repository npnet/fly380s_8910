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

configure_file(include/hal_config.h.in ${BINARY_TOP_DIR}/include/hal_config.h)
set(keypadcfg_h ${BINARY_TOP_DIR}/include/hal_keypad_def.h)
set(pindef_h ${BINARY_TOP_DIR}/include/hal_iomux_pindef.h)
set(pincfg_h ${BINARY_TOP_DIR}/include/hal_iomux_pincfg.h)
set(csv_abs ${PROJECT_SOURCE_DIR}/${CONFIG_PINMUX_CSV_PATH})
set(csv_keypad ${PROJECT_SOURCE_DIR}/${CONFIG_KEYPAD_CSV_PATH})

add_custom_command(OUTPUT ${pindef_h} ${pincfg_h}
                   DEPENDS ${csv_abs} ${iomuxgen_py}
                   COMMAND python3 ${iomuxgen_py} ${csv_abs} ${pindef_h} ${pincfg_h})
add_custom_command(OUTPUT ${keypadcfg_h}
                   DEPENDS ${csv_keypad} ${keypadgen_py}
                   COMMAND python3 ${keypadgen_py} ${csv_keypad} ${keypadcfg_h})

set(target hal)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy.c "/* dummy */")
add_library(${target} STATIC ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_include_directories(${target} PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)
target_include_targets(${target} PUBLIC kernel)
target_include_targets(${target} PRIVATE calclib fs nvm driver)

if(NOT ${CONFIG_PINMUX_CSV_PATH} STREQUAL "")
    target_sources(${target} PRIVATE ${pindef_h} ${pincfg_h} ${keypadcfg_h})
endif()

if(CONFIG_SOC_8910)
    target_sources(${target} PRIVATE
        src/8910/hal_clock_init.c
        src/8910/hal_chip_8910.c
        src/8910/hal_pmu_8910.c
        src/hal_hw_map.c
        src/hal_iomux.c
        src/hal_reg_apply.c
        src/hal_mmu.c
        src/hal_shmem_region.c
        src/hal_adi_bus.c
        src/hal_hwspinlock.c
        src/hal_cp_ctrl.c
        src/hal_lzma.c
        src/hal_spi_flash.c
	src/hal_gouda.c
    )
endif()

if(CONFIG_SOC_8955)
    target_sources(${target} PRIVATE
        src/8955/hal_clock_init.c
        src/8955/hal_chip_8955.c
        src/8955/hal_ddr_change.c
        src/8955/hal_pmu_8955.c
        src/hal_access_map.c
        src/hal_hw_map.c
        src/hal_iomux.c
        src/hal_reg_apply.c
        src/hal_page_spy.c
        src/hal_spi_flash.c
    )
    set_source_files_properties(src/8955/hal_ddr_change.c PROPERTIES COMPILE_OPTIONS "-mno-mips16")

    rom_for_xcpu_gen(${CMAKE_CURRENT_SOURCE_DIR}/src/8955/xcpu_rom_u02.elf
        ${CMAKE_CURRENT_SOURCE_DIR}/src/8955/xcpu_rom_u02.rename
        ${CMAKE_CURRENT_SOURCE_DIR}/src/8955/xcpu_rom_u02.sym
    )
endif()

if(CONFIG_SOC_8909)
    target_sources(${target} PRIVATE
        src/8909/hal_clock_init.c
        src/8909/hal_chip_8909.c
        src/8909/hal_pmu_8909.c
        src/hal_access_map.c
        src/hal_hw_map.c
        src/hal_iomux.c
        src/hal_reg_apply.c
        src/hal_page_spy.c
        src/hal_spi_flash.c
    )

    rom_for_xcpu_gen(${CMAKE_CURRENT_SOURCE_DIR}/src/8909/xcpu_rom_u02.elf
        ${CMAKE_CURRENT_SOURCE_DIR}/src/8909/xcpu_rom_u02.rename
        ${CMAKE_CURRENT_SOURCE_DIR}/src/8909/xcpu_rom_u02.sym
    )
endif()

relative_glob(srcs include/*.h src/*.c src/*.h
    include/8955/*.h include/8909/*.h include/8910/*.h)
beautify_c_code(${target} ${srcs})
