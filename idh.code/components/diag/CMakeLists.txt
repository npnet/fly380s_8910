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

configure_file(include/diag_config.h.in ${BINARY_TOP_DIR}/include/diag_config.h)

set(target diag)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy.c "/* dummy */")
add_library(${target} STATIC ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
if(CONFIG_DIAG_ENABLED)

if(CONFIG_SOC_6760)
    target_sources(${target} PRIVATE
        src/diag.c
        src/diag_current_test.c
        src/diag_swver.c
        src/diag_log.c
        src/diag_nw.c
        src/diag_runmode.c
        src/diag_at.c
        src/uis6760/autotest_device.c
        src/diag_nv.c
    )
else()
    target_sources(${target} PRIVATE
        src/diag.c
        src/layer1_diag.c
        src/diag_current_test.c
        src/diag_swver.c
        src/diag_log.c
        src/diag_nvm.c
        src/diag_runmode.c
        src/diag_at.c
        src/diag_sys.c
        src/diag_device_uart.c
        src/diag_device_userial.c
    )
endif()

if(CONFIG_SOC_8910)
    target_sources(${target} PRIVATE
        src/8910/autotest_device.c
    )
endif()

endif()

set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_DIAG)
target_include_directories(${target} PUBLIC include)
target_link_libraries(${target} PRIVATE kernel driver audio hal fs cfw nvm calclib)

relative_glob(srcs include/*.h src/*.c src/8910/*.c src/*.h)
list(FILTER srcs EXCLUDE REGEX ".*/calibration.h")
list(FILTER srcs EXCLUDE REGEX ".*/cmddef.h")
beautify_c_code(${target} ${srcs})
