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

configure_file(include/app_config.h.in ${out_inc_dir}/app_config.h)
add_definitions(-DBUILD_IDENTIFY=\"${BUILD_TARGET}-${BUILD_RELEASE_TYPE}-${BUILD_REVISION}-${BUILD_AUTO_REVISION}\")

set(target appstart)

if(CONFIG_SOC_8910)
    add_library(${target} STATIC src/app_start.c)
    target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_APPSTART)
    set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
    target_include_targets(${target} PRIVATE
        atr ats diag nvm kernel driver ml bdev fs fsmount
        net hal mal cfw http fupdate service apploader
        audio tts
    )
    target_include_targets_if(CONFIG_SPRD_BT_ENABLE THEN ${target} PRIVATE sprd_bt)
    target_include_targets_if(CONFIG_BLUEU_BT_ENABLE THEN ${target} PRIVATE blueu)

    set_if(ldscript CONFIG_APP_RUN_ON_FLASH
        THEN ${SOURCE_TOP_DIR}/components/hal/ldscripts/flashrun.ld
        OTHERWISE ${SOURCE_TOP_DIR}/components/hal/ldscripts/ddrrun.ld
    )

    set_if(flash2_opt CONFIG_APP_FLASH2_ENABLED THEN WITH_FLASH2)
    add_uimage(${BUILD_TARGET} ${ldscript} ${dummy_cxx_file} ${flash2_opt})
    target_link_libraries(${BUILD_TARGET} PRIVATE all_libs ${target} all_libs)
endif()

relative_glob(srcs include/*.h src/*.c src/*.h)
beautify_c_code(appstart ${srcs})
