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

set(AT_MANUFACTURER_ID "UNISOC")
set(AT_MODEL_ID "${BUILD_TARGET}-${BUILD_RELEASE_TYPE}")
set(AT_REVISION_ID "${BUILD_REVISION}-${BUILD_AUTO_REVISION}")

configure_file(include/ats_config.h.in ${BINARY_TOP_DIR}/include/ats_config.h)

set(at_apn_table ${CMAKE_CURRENT_BINARY_DIR}/at_apn_table.h)

if(AT_GLOBAL_APN_SUPPORT)
execute_process(COMMAND python3 apn/csv_parse.py apn/global_apn.csv ${at_apn_table}
         WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
else()
execute_process(COMMAND python3 apn/csv_parse.py apn/local_apn.csv ${at_apn_table}
         WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
endif()

set(target ats)
add_library(${target} STATIC)
target_sources(${target} PRIVATE
    cfw/at_cfw.c
    gc/at_cmd_gc.c
    gc/at_cmd_fupdate.c
    id/at_cmd_id.c
    nw/at_cmd_nw.c
    cc/at_cmd_cc.c
    gprs/at_cmd_gprs.c
    pbk/at_cmd_pbk.c
    nv/at_cmd_nv.c
    ss/at_cmd_ss.c
    sat/at_cmd_sat.c
    sim/at_cmd_sim.c
    myinfo/at_cmd_myinfo.c
    misc/at_cmd_debug.c
    misc/at_cmd_sysnv.c
    emod/at_cmd_emod.c
    audio/at_cmd_audio.c
    audio/at_cmd_tts.c
)
target_sources(${target} PRIVATE ftp/at_cmd_ftp.c)
target_sources(${target} PRIVATE ftp/at_cmd_myftp.c)
target_sources(${target} PRIVATE fs/at_cmd_fs.c)
target_sources(${target} PRIVATE http/at_cmd_http.c)
target_sources(${target} PRIVATE tcpip/at_cmd_tcpip.c)
target_sources(${target} PRIVATE tcpip/at_cmd_udp.c)
target_sources(${target} PRIVATE tcpip/at_cmd_netif.c)
target_sources(${target} PRIVATE coap/at_cmd_coap.c)
target_sources(${target} PRIVATE mqtt/at_cmd_mqtt.c)
target_sources(${target} PRIVATE tcpip/at_cmd_mynet.c)
target_sources(${target} PRIVATE sms/at_cmd_sms.c)
target_sources(${target} PRIVATE tcpip/at_cmd_ping.c)
target_sources(${target} PRIVATE sntp/at_cmd_sntp.c)
if(CONFIG_AT_CISSDK_MIPL_SUPPORT)
target_sources(${target} PRIVATE lwm2m/at_cmd_cissdk_mipl.c)
endif()
if(CONFIG_AT_DM_LWM2M_SUPPORT)
target_sources(${target} PRIVATE dm/at_cmd_dm_lwm2m.c)
endif()
target_sources(${target} PRIVATE battery/at_cmd_battery.c)
target_sources(${target} PRIVATE ssim/at_cmd_ssim.c)
if(CONFIG_AT_CAMERA_SUPPORT)
target_sources(${target} PRIVATE cam/at_cmd_camera.c)
endif()
if(CONFIG_AT_SPBTTEST_SUPPORT)
    target_sources(${target} PRIVATE bt/at_cmd_spbttest.c)
endif()
if(CONFIG_AT_SPBLETEST_SUPPORT)
    target_sources(${target} PRIVATE bt/at_cmd_spbletest.c)
endif()

set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_include_directories(${target} PUBLIC include)
target_link_libraries(${target} PRIVATE
    kernel driver ml hal calclib atr cfw fs fsmount
    aworker net lwip http mupnp mbedtls libcoap
    nvm cis_sdk fupdate audio service tts
)

target_include_directories(${target} PRIVATE ../modem/sx/include)
target_include_directories(${target} PRIVATE ../modem/pal/include)
target_include_directories(${target} PRIVATE ../net/include)
target_include_directories(${target} PRIVATE ../net/http/include)
target_include_directories(${target} PRIVATE ../net/libcoap/include)
target_include_directories(${target} PRIVATE ../net/mupnp/src/mupnp/include)
target_include_directories(${target} PRIVATE ../net/mbedtls/include)
target_include_directories(${target} PRIVATE ../modem/include)
target_include_directories(${target} PRIVATE ../atr/src)
target_include_directories(${target} PRIVATE ../net/cis_sdk/src/NB_SDK2.3.0_D1.1.1/src/ciscore)
target_include_directories(${target} PRIVATE ../diag/include)
target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_include_targets(${target} PRIVATE mal)

relative_glob(srcs */*.h */*.c)
beautify_c_code(${target} ${srcs})
