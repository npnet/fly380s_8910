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

configure_file(include/srv_config.h.in ${BINARY_TOP_DIR}/include/srv_config.h)

set(target service)
add_library(${target} STATIC
    src/srv_usb_trace.c
    src/srv_wdt.c
    src/srv_dtr.c
    src/srv_modem_log.c
    src/srv_mos_log.c
    src/srv_2line_wakeup.c
    src/srv_sim_detect.c
    src/srv_rf_param.c
)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_SRV)
target_include_directories(${target} PUBLIC include)
target_include_targets(${target} PRIVATE kernel driver hal fs)

relative_glob(srcs include/*.h src/*.c src/*.h)
beautify_c_code(${target} ${srcs})
