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

configure_file(include/ctelpttdemo_config.h.in ${BINARY_TOP_DIR}/include/ctelpttdemo_config.h)

set(target ctelpttlib)
add_library(${target} STATIC IMPORTED GLOBAL)
set_target_properties(${target} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libctelptt.a)

set(target ctelpttdemo)
add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
  #        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libctelptt.a)
#set_target_properties(${target} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libctelptt.a)
target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_CTEL_PTT)
target_include_directories(${target} PUBLIC include)

target_link_libraries(${target} PRIVATE  ctelpttlib)

target_include_directories(${target} PRIVATE ${out_rpc_dir})
target_include_targets(${target} PRIVATE kernel driver hal fs nanopb audio ml lvgl cfw nvm tts net lwip)

target_sources(${target} PRIVATE
    src/ctel_oem_callback_realize.c
    src/ctelpttdemomain.c
)

relative_glob(srcs */*.h */*.c  )
beautify_c_code(${target} ${srcs})
