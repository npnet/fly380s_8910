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
#configure_file(include/mupnp_config.h.in ${BINARY_TOP_DIR}/include/mupnp_config.h)
configure_file(include/http_config.h.in ${BINARY_TOP_DIR}/include/http_config.h)
set(target http)
add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_NET)
target_link_libraries(${target} PRIVATE kernel driver calclib net lwip cfw atr ats fs json)
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(${target} PRIVATE ../external/platform/include)
target_include_directories(${target} PRIVATE ../mupnp/src/mupnp/include)
target_include_directories(${target} PRIVATE include)
target_include_directories(${target} PRIVATE ../mbedtls/include)
target_include_directories(${target} PRIVATE ../json/cJSON)

set(HTTPFILES  src/http_api.c
	src/http_download.c
	src/http_json.c
	src/http_onenet_api.c
	src/https_api.c
)


target_sources(${target} PRIVATE
    ${HTTPFILES}
	${MUPNPNOAPPSFILES}
)

relative_glob(srcs include/*.h src/*.c)
beautify_c_code(${target} ${srcs})
