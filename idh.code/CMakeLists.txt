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

cmake_minimum_required(VERSION 3.13)

set(SOURCE_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(BINARY_TOP_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)
include(cmake/extension.cmake)

set(out_hex_dir ${CMAKE_CURRENT_BINARY_DIR}/hex)
set(out_lib_dir ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(out_rpc_dir ${CMAKE_CURRENT_BINARY_DIR}/rpcgen)
set(out_rel_dir ${CMAKE_CURRENT_BINARY_DIR}/rel)
set(out_inc_dir ${CMAKE_CURRENT_BINARY_DIR}/include)
set(tools_dir ${CMAKE_CURRENT_SOURCE_DIR}/tools)

set(build_target_dir ${CMAKE_CURRENT_SOURCE_DIR}/target/${BUILD_TARGET})
set(elf2symld_py ${tools_dir}/elf2symld.py)
set(elf2incld_py ${tools_dir}/elf2incld.py)
set(pacgen_py ${tools_dir}/pacgen.py)
set(signature_py ${tools_dir}/signature/signature.py)
set(iomuxgen_py ${tools_dir}/iomuxgen.py)
set(keypadgen_py ${tools_dir}/keypadgen.py)
set(partinfogen_py ${tools_dir}/partinfogen.py)
set(modemgen_py ${tools_dir}/modemgen.py)
set(nvbintool_py ${tools_dir}/nvbintool.py)
set(cpiogen_py ${tools_dir}/cpiogen.py)
set(cmd_symdefgen python3 ${tools_dir}/symdefgen.py)
set(corestubgen dtools expgen)
set(cmd_mkuimage dtools mkuimage)
set(cmd_mkappimg dtools mkappimg)
set(cmd_elf2lod dtools elf2lod)
set(partinfo_bin ${out_hex_dir}/partinfo.bin)
set(core_stub_o ${out_rel_dir}/core_stub.o)

set(AUTO_GENERATION_NOTICE "Auto generated. Don't edit it manually!")

# BUILD_REVISION: A string to identify the build. When not specified, which
#       is common in development, it will be "DEVEL". CI will define it in
#       command line, such as "8915_W19.32.2"
if(NOT BUILD_REVISION)
    set(BUILD_REVISION $ENV{BUILD_REVISION})
    if(NOT BUILD_REVISION)
        set(BUILD_REVISION DEVEL)
    endif()
endif()

# By default, BUILD_GIT_REVISION will be get by git command. However, it is
# possible to be specified by command line option. Due to BUILD_GIT_REVISION
# may be used for macro definition, it will make the build image "stable".
if(NOT BUILD_GIT_REVISION)
    find_package(Git)
    if(GIT_FOUND)
        execute_process(COMMAND "${GIT_EXECUTABLE}" describe --abbrev=8 --always --dirty
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE BUILD_GIT_REVISION
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
endif()

if(NOT BUILD_GIT_REVISION)
    set(BUILD_GIT_REVISION "unknown")
endif()

# BUILD_AUTO_REVISION: A string to identify the source codes revision.
#       When it is built under git working directory, it will be "g1234abcd"
#       or "g1234abcd-dirty" if the working directory is dirty. When not in
#       git working directory, it will be "unknown". And it can be overrided
#       in command line.
if(NOT BUILD_AUTO_REVISION)
    set(BUILD_AUTO_REVISION "g${BUILD_GIT_REVISION}")
endif()

# BUILD_TARGET
if(NOT BUILD_TARGET)
    set(BUILD_TARGET $ENV{BUILD_TARGET})
endif()

# BUILD_RELEASE_TYPE
if (NOT BUILD_RELEASE_TYPE)
    set (BUILD_RELEASE_TYPE $ENV{BUILD_RELEASE_TYPE})
endif()

if("${BUILD_RELEASE_TYPE}" STREQUAL "release")
    set(CONFIG_WDT_ENABLE ON)
endif()

if(NOT BUILD_TARGET)
    message(FATAL_ERROR "BUILD_TARGET is empty")
endif()

# Process and include target config
set(TARGET_CONFIG ${SOURCE_TOP_DIR}/target/${BUILD_TARGET}/target.config)
set(TARGET_CMAKE ${BINARY_TOP_DIR}/target.cmake)
execute_process(
    COMMAND python3 ${tools_dir}/cmakeconfig.py ${TARGET_CONFIG} ${TARGET_CMAKE}
    WORKING_DIRECTORY ${SOURCE_TOP_DIR}
)
include(${TARGET_CMAKE})

# Process and include partition config
execute_process(
    COMMAND python3 ${partinfogen_py} ${CONFIG_PARTINFO_JSON_PATH} ${partinfo_bin} ${BINARY_TOP_DIR}/partinfo.cmake
    WORKING_DIRECTORY ${SOURCE_TOP_DIR}
)
include(${BINARY_TOP_DIR}/partinfo.cmake)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${partinfogen_py} ${CONFIG_PARTINFO_JSON_PATH})

# Include chip config
include(components/chip/chip.cmake)

# Process and include Kconfig dependency
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${TARGET_CONFIG})
set(KCONFIG_DEP ${BINARY_TOP_DIR}/kconfig_dep.cmake)
execute_process(
    COMMAND python3 ${tools_dir}/cmakedep.py ${KCONFIG_DEP}
    WORKING_DIRECTORY ${SOURCE_TOP_DIR}
)
include(${KCONFIG_DEP})

# Include toolchain config
include(cmake/toolchain-gcc.cmake)
set(BUILD_SHARED_LIBS OFF)

project(${BUILD_TARGET} C CXX ASM)
include_directories(${BINARY_TOP_DIR}/include)
include_directories(${SOURCE_TOP_DIR}/components/newlib/include)
add_custom_target(beautify)
add_custom_target(unittests)
add_custom_target(examples ALL)
add_custom_target(rpcgen)
add_appimg_delete()

set(unittest_libs kernel osi_lib unity driver hal calclib
     bdev sffs fatfs fs fsmount atr ats cfw mal audio rpc apploader
    ${modem_libs} ${sysrom_elf}
)

add_subdirectory_if_exist(components)
add_subdirectory_if_exist(unittest)
add_subdirectory_if_exist(examples)

# Generate all nvitem.bin with merge param delta nv
foreach(nvmvariant ${CONFIG_NVM_VARIANTS})
    set(nvmprj ${NVM_VARIANT_${nvmvariant}_NVMPRJ})
    set(nvitem_bin ${out_hex_dir}/${nvmvariant}_nvitem.bin)
    set(prinv_bin ${out_hex_dir}/${nvmvariant}_prinv.bin)
    get_filename_component(nvitemdir ${nvmprj} DIRECTORY)
    file(GLOB nvitems ${nvitemdir}/*.prj ${nvitemdir}/*.nvm)

    add_custom_command(OUTPUT ${prinv_bin}
        COMMAND python3 ${nvbintool_py} gen
            --fix-size ${CONFIG_NVBIN_FIXED_SIZE}
            --gendir ${BINARY_TOP_DIR}/nvgen/${nvmvariant}
            ${nvmprj} ${prinv_bin}
        DEPENDS ${nvbintool_py} ${nvitems}
        WORKING_DIRECTORY ${SOURCE_TOP_DIR}
    )

    set(deltanv_dir ${BINARY_TOP_DIR}/nvgen/${nvmvariant}_Deltanv)
    set(pdnv_bin ${deltanv_dir}/delta_nv.bin)
    file(GLOB pdeltanvs ${pdeltanv_dir}/include_all.nv)
    get_filename_component(nvitemprj ${nvmprj} NAME)
    set(prj_delta ${nvitemprj}_file)
    cpp_only(${prj_delta} ${pdeltanvs} hal)

    add_custom_command(OUTPUT ${pdnv_bin}
        COMMAND python3 ${nvbintool_py} diffs
            --src $<TARGET_OBJECTS:${prj_delta}>
            --des ${deltanv_dir}/config_all.nv
            ${BINARY_TOP_DIR}/nvgen/${nvmvariant}/${nvitemprj}
        DEPENDS ${nvbintool_py} ${nvitems} ${pdeltanvs} ${prinv_bin} ${prj_delta}
        WORKING_DIRECTORY ${SOURCE_TOP_DIR}
    )

    add_custom_command(OUTPUT ${nvitem_bin}
        COMMAND python3 ${nvbintool_py} apply
            --nvbin ${prinv_bin}
            --deltanv ${pdnv_bin}
            --output ${nvitem_bin}
        DEPENDS ${nvbintool_py} ${nvitems} ${pdnv_bin}
        WORKING_DIRECTORY ${SOURCE_TOP_DIR}
    )
endforeach()

if(NOT ${CONFIG_PACKAGE_FILE_JSON_PATH} STREQUAL "")
    set(package_file_config ${out_hex_dir}/package_file.json)
    set(package_file ${out_hex_dir}/package_file.cpio)
    configure_file(${CONFIG_PACKAGE_FILE_JSON_PATH} ${package_file_config})
    execute_process(
        COMMAND python3 ${cpiogen_py} cmake
            --config ${package_file_config}
            --cmake ${BINARY_TOP_DIR}/package_file.cmake
        WORKING_DIRECTORY ${SOURCE_TOP_DIR}
    )
    include(${BINARY_TOP_DIR}/package_file.cmake)
endif()

if(DEFINED package_file_depends)
    set(package_file_cpio package_file.cpio)
    add_custom_command(OUTPUT ${out_hex_dir}/${package_file_cpio}
        COMMAND python3 ${cpiogen_py} archive
            --config ${package_file_config}
            --output ${out_hex_dir}/${package_file_cpio}
        DEPENDS ${cpiogen_py} ${package_file_depends} ${package_file_config}
        WORKING_DIRECTORY ${SOURCE_TOP_DIR}
    )
endif()

if(CONFIG_SOC_8910)
    # Sign fdl1/fdl2/boot/ap. They are common to all variants
    set(sign_password 12345678)     # customer product shall replace with customer's key
    set(sign_product test)          # customer product shall replace with customer's product name
    set(sign_boot_padlen 0xbce0)    # can't be changed
    add_custom_command(OUTPUT ${out_hex_dir}/fdl1.sign.img ${out_hex_dir}/fdl2.sign.img ${out_hex_dir}/boot.sign.img ${out_hex_dir}/${BUILD_TARGET}.sign.img
        COMMAND vlrsign --pw ${sign_password} --pn ${sign_product} --ha Blake2
            --img ${out_hex_dir}/boot.img --out ${out_hex_dir}/boot.sign.img --plen ${sign_boot_padlen}
        COMMAND vlrsign --pw ${sign_password} --pn ${sign_product} --ha Blake2
            --img ${out_hex_dir}/fdl1.img --out ${out_hex_dir}/fdl1.sign.img
        COMMAND vlrsign --pw ${sign_password} --pn ${sign_product} --ha Blake2
            --img ${out_hex_dir}/fdl2.img --out ${out_hex_dir}/fdl2.sign.img
        COMMAND vlrsign --pw ${sign_password} --pn ${sign_product} --ha Blake2
            --img ${out_hex_dir}/${BUILD_TARGET}.img --out ${out_hex_dir}/${BUILD_TARGET}.sign.img
        DEPENDS ${out_hex_dir}/boot.img ${out_hex_dir}/fdl1.img
            ${out_hex_dir}/fdl2.img ${out_hex_dir}/${BUILD_TARGET}.img
    )

    # Create pac for all variants
    foreach(nvmvariant ${CONFIG_NVM_VARIANTS})
        build_modem_image(${nvmvariant})

        set(nvname ${NVM_VARIANT_${nvmvariant}_NVMITEM})
        set(pac_config ${out_hex_dir}/${nvmvariant}.json)
        set(pac_file ${out_hex_dir}/${BUILD_TARGET}-${nvmvariant}-${nvname}_${BUILD_RELEASE_TYPE}.pac)
        pac_init_fdl(init_fdl ${pac_config})
        pac_nvitem_8910(nvitem_8910 ${pac_config})

        if(DEFINED package_file_depends)
            set(pac_package_file cfg-pack-cpio -i PACKAGE_FILE -p ${out_hex_dir}/${package_file_cpio} ${pac_config})
        endif()

        execute_process(
            COMMAND python3 ${pacgen_py} ${init_fdl} ${nvitem_8910}
                cfg-phase-check ${pac_config}
                cfg-image -i BOOTLOADER -a ${CONFIG_BOOT_FLASH_ADDRESS} -s ${CONFIG_BOOT_FLASH_SIZE}
                    -p ${out_hex_dir}/boot.sign.img ${pac_config}
                cfg-image -i AP -a ${CONFIG_APP_FLASH_ADDRESS} -s ${CONFIG_APP_FLASH_SIZE}
                    -p ${out_hex_dir}/${BUILD_TARGET}.sign.img ${pac_config}
                cfg-image -i PS -a ${CONFIG_FS_MODEM_FLASH_ADDRESS} -s ${CONFIG_FS_MODEM_FLASH_SIZE}
                    -p ${out_hex_dir}/${nvmvariant}.img ${pac_config}
                ${pac_package_file}
                cfg-clear-nv ${pac_config}
                cfg-nv -s ${CONFIG_NVBIN_FIXED_SIZE} -p ${out_hex_dir}/${nvmvariant}_nvitem.bin
                    ${pac_config}
                dep-gen --base ${SOURCE_TOP_DIR} ${pac_config}
            OUTPUT_VARIABLE pac_dep
            OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY ${SOURCE_TOP_DIR}
        )

        add_custom_command(OUTPUT ${pac_file}
            COMMAND python3 ${pacgen_py} pac-gen ${pac_config} ${pac_file}
            DEPENDS ${pacgen_py} ${pac_config} ${pac_dep}
            WORKING_DIRECTORY ${SOURCE_TOP_DIR}
        )
        add_custom_target(${nvmvariant}_pacgen ALL DEPENDS ${pac_file})
    endforeach()
endif()
