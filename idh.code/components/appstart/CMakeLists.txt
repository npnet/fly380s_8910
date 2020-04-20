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

configure_file(include/app_config.h.in ${BINARY_TOP_DIR}/include/app_config.h)

add_definitions(-DBUILD_IDENTIFY=\"${BUILD_TARGET}-${BUILD_RELEASE_TYPE}-${BUILD_REVISION}-${BUILD_AUTO_REVISION}\")

if(CONFIG_SOC_8910)
    if(CONFIG_APP_RUN_ON_FLASH)
        set(ldscript ${SOURCE_TOP_DIR}/components/hal/ldscripts/flashrun.ld)
    else()
        set(ldscript ${SOURCE_TOP_DIR}/components/hal/ldscripts/ddrrun.ld)
    endif()

    set(srcs src/app_start.c src/app_keypad.c)

    set(target ${BUILD_TARGET})
    add_uimage(${target} ${ldscript} ${srcs})
    target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_APPSTART)
    target_link_group(${target} PRIVATE
        atr ats diag nvm kernel osi_lib driver ml bdev fs fsmount net hal mal cfw http fupdate
        service apploader audio tts lvgl poc
        ${libc_file_name} ${libgcc_file_name}
    )
endif()

if((CONFIG_SOC_8955) OR (CONFIG_SOC_8909))
    set(ldscript ${SOURCE_TOP_DIR}/components/hal/ldscripts/xcpu_flashrun.ld)
    set(target ${BUILD_TARGET})
    add_flash_lod(${target} ${ldscript} src/app_start_xcpu.c)
    target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_APPSTART)
    target_link_group(${target} PRIVATE
        atr ats diag nvm kernel osi_lib driver ml fs fsmount net hal mal cfw http nvm
        ${modem_libs} ${sysrom_elf} ${libc_file_name} ${libgcc_file_name}
    )
    add_dependencies(${target}_ldscript ${BUILD_TARGET}_bcpu sysrom_for_xcpu)
    target_include_directories(${target}_ldscript PRIVATE ${out_hex_dir})
endif()

relative_glob(srcs include/*.h src/*.c src/*.h)
beautify_c_code(appstart ${srcs})
