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

configure_file(include/poc_config.h.in ${BINARY_TOP_DIR}/include/poc_config.h)

set(target poc)
add_library(${target} STATIC src/poc.c src/poc_audio_player.c src/poc_audio_recorder.c src/poc_keypad.c)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_POC)
target_include_directories(${target} PUBLIC include)
target_include_targets(${target} PRIVATE kernel driver hal fs audio ml lvgl)

relative_glob(srcs */*.h */*.c)
beautify_c_code(${target} ${srcs})
