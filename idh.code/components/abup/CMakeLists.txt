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

configure_file(include/abup_config.h.in ${BINARY_TOP_DIR}/include/abup_config.h)

if(CONFIG_SOC_89101)
	set(target abup_lib)
	add_library(${target} STATIC
	    src/abup_hmd5.c
	    src/abup_http.c
	    src/abup_md5.c
	)
	set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
	target_link_libraries(${target} PRIVATE kernel driver hal fs net lwip cfw)
	target_include_directories(${target} PUBLIC include)
else()
	set(target abup_lib)
	add_library(${target} STATIC IMPORTED GLOBAL)
	set_target_properties(${target} PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libabup_lib.a)
endif()

set(target abup_pub)
add_library(${target} STATIC
    src/abup_common.c
    src/abup_file.c
    src/abup_fota.c
)
#set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_include_directories(${target} PUBLIC include)
target_link_libraries(${target} PRIVATE kernel driver hal fs net lwip cfw lvgl poc)
relative_glob(srcs */*.h */*.c)
beautify_c_code(${target} ${srcs})


