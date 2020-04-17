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

configure_file(include/net_config.h.in ${BINARY_TOP_DIR}/include/net_config.h)

set(target net)
add_library(${target} STATIC)
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_NET)
target_include_directories(${target} PUBLIC include)
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(${target} PRIVATE lwip/include)
target_include_directories(${target} PRIVATE lwip/src/include)
target_include_directories(${target} PRIVATE lwip/src/include/lwip)
target_include_directories(${target} PRIVATE lwip/src/include/lwip/priv)
target_include_directories(${target} PRIVATE ../ats/include)
target_include_directories(${target} PRIVATE ../hal/include)
target_include_directories(${target} PRIVATE ../net/mupnp/src/mupnp/include)
target_include_directories(${target} PRIVATE ../net/mbedtls/include)
target_include_directories(${target} PRIVATE http/include)
target_include_directories(${target} PRIVATE ../json/cJSON)
target_include_directories(${target} PRIVATE libcoap/include)
target_include_directories(${target} PRIVATE ../fs/include)

target_link_libraries(${target} PRIVATE kernel driver calclib lwip atr cfw aworker json fs)

target_sources(${target} PRIVATE

	src/netif_gprs.c
	src/lwipports.c
	src/netmain.c
	src/netutils.c
	src/cfw_sockets_api.c
	src/ppp_interface.c
	src/lwip_mqtt_api.c
	src/unicomIot.c
	)

if(CONFIG_SOC_8910)
target_sources(${target} PRIVATE
	myselfsrc/vnetIot.c
	src/netdev_interface.c
	src/dhcp_reply.c
	src/nd6_reply.c
	)
endif()

if((CONFIG_SOC_8955) OR (CONFIG_SOC_8909))
target_sources(${target} PRIVATE

	src/netif_gprs.c
	src/lwipports.c
	src/netmain.c
	src/netutils.c
	src/cfw_sockets_api.c
	src/ppp_interface.c
	src/lwip_mqtt_api.c
	)
endif()
relative_glob(srcs include/*.h src/*.c src/*.h)
beautify_c_code(${target} ${srcs})

add_subdirectory_if_exist(lwip)
add_subdirectory_if_exist(libmqttsn)
add_subdirectory_if_exist(libcoap)
add_subdirectory_if_exist(json)
add_subdirectory_if_exist(mbedtls)
add_subdirectory_if_exist(libexpat)
add_subdirectory_if_exist(edp)
add_subdirectory_if_exist(mupnp)
add_subdirectory_if_exist(liblwm2m)
add_subdirectory_if_exist(http)
add_subdirectory_if_exist(aliotx)
add_subdirectory_if_exist(cis_sdk)
if(CONFIG_NET_ALI_SDK_SUPPORT)
add_subdirectory_if_exist(ali_c_sdk)
endif()