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

set(CROSS_COMPILE arm-none-eabi-)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(ARCH arm)

set(CONFIG_CACHE_LINE_SIZE 32)
set(CONFIG_SRAM_PHY_ADDRESS 0x800000)
set(CONFIG_NOR_PHY_ADDRESS 0x60000000)
set(CONFIG_NOR_EXT_PHY_ADDRESS 0x70000000)
set(CONFIG_RAM_PHY_ADDRESS 0x80000000)

set(CONFIG_SRAM_SIZE 0x40000)
set(CONFIG_GIC_BASE_ADDRESS 0x08201000)
set(CONFIG_GIC_CPU_INTERFACE_OFFSET 0x00001000)
set(CONFIG_GIC_PRIORITY_BITS 5)

set(CONFIG_UIMAGE_HEADER_SIZE 0x40)
set(CONFIG_ROM_LOAD_SRAM_OFFSET 0xc0)
set(CONFIG_ROM_LOAD_SIZE 0xbf40)
math(EXPR CONFIG_ROM_SRAM_LOAD_ADDRESS "${CONFIG_SRAM_PHY_ADDRESS}+${CONFIG_ROM_LOAD_SRAM_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)

set(CONFIG_BOOTLOADER_SIZE ${CONFIG_BOOT_FLASH_SIZE})

set(CONFIG_APP_SRAM_OFFSET 0x100)
set(CONFIG_APP_SRAM_SIZE 0x1f00)
set(CONFIG_APP_SRAM_SHMEM_OFFSET 0x2000)
set(CONFIG_APP_SRAM_SHMEM_SIZE 0x1000)
set(CONFIG_IRQ_STACK_TOP 0x0)
set(CONFIG_SVC_STACK_TOP 0x200)
set(CONFIG_FIQ_STACK_TOP 0x800)
set(CONFIG_ABT_STACK_TOP 0x800)
set(CONFIG_UND_STACK_TOP 0x800)
set(CONFIG_SYS_STACK_TOP 0x800)
set(CONFIG_STACK_BOTTOM 0xc00)

math(EXPR CONFIG_APP_BTFW_RAM_OFFSET
    "${CONFIG_APP_RAM_OFFSET}+${CONFIG_APP_TOTAL_RAM_SIZE}-${CONFIG_APP_BTFW_RAM_SIZE}"
    OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_APP_FILEIMG_RAM_OFFSET
    "${CONFIG_APP_BTFW_RAM_OFFSET}-${CONFIG_APP_FILEIMG_RAM_SIZE}"
    OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_APP_FLASHIMG_RAM_OFFSET
    "${CONFIG_APP_FILEIMG_RAM_OFFSET}-${CONFIG_APP_FLASHIMG_RAM_SIZE}"
    OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_APP_RAM_SIZE
    "${CONFIG_APP_FLASHIMG_RAM_OFFSET}-${CONFIG_APP_RAM_OFFSET}"
    OUTPUT_FORMAT HEXADECIMAL)

set(CONFIG_BOOT_IMAGE_START ${CONFIG_ROM_SRAM_LOAD_ADDRESS})
set(CONFIG_BOOT_IMAGE_SIZE ${CONFIG_ROM_LOAD_SIZE})
math(EXPR CONFIG_BOOT_UNSIGN_IMAGE_SIZE "${CONFIG_ROM_LOAD_SIZE}-0x460" OUTPUT_FORMAT HEXADECIMAL)

math(EXPR CONFIG_DEFAULT_MEMBUS_FREQ "${CONFIG_DEFAULT_MEMPLL_FREQ}*2/${CONFIG_DEFAULT_MEMBUS_HALF_DIV}")
math(EXPR CONFIG_DEFAULT_CPU_FREQ "${CONFIG_DEFAULT_CPUPLL_FREQ}*2/${CONFIG_DEFAULT_CPU_HALF_DIV}")
math(EXPR CONFIG_DEFAULT_SYSAXI_FREQ "${CONFIG_DEFAULT_CPUPLL_FREQ}*2/${CONFIG_DEFAULT_SYSAXI_HALF_DIV}")
math(EXPR CONFIG_DEFAULT_SYSAHB_FREQ "${CONFIG_DEFAULT_SYSAXI_FREQ}*2/${CONFIG_DEFAULT_SYSAHB_HALF_DIV}")
math(EXPR CONFIG_DEFAULT_SPIFLASH_CTRL_FREQ "${CONFIG_DEFAULT_CPUPLL_FREQ}*2/${CONFIG_DEFAULT_SPIFLASH_CTRL_HALF_DIV}")
math(EXPR CONFIG_DEFAULT_SPIFLASH_DEV_FREQ "${CONFIG_DEFAULT_CPUPLL_FREQ}*2/${CONFIG_DEFAULT_SPIFLASH_DEV_HALF_DIV}")
math(EXPR CONFIG_DEFAULT_SPIFLASH_INTF_FREQ "${CONFIG_DEFAULT_SPIFLASH_DEV_FREQ}/${CONFIG_DEFAULT_SPIFLASH_INTF_DIV}")

set(unittest_ldscript ${SOURCE_TOP_DIR}/components/hal/ldscripts/flashrun.ld)
set(pac_fdl_files ${out_hex_dir}/fdl1.sign.img ${out_hex_dir}/fdl2.sign.img)

function(pacvariant_gen)
    # Sign fdl1/fdl2/boot/ap. They are common to all variants
    set(sign_password 17834926)     # customer product shall replace with customer's key 17834926
    set(sign_product k19h_a500l_ver1_0_1)          # customer product shall replace with customer's product name k19h_a500l_ver1_0_1
    set(sign_boot_padlen 0xbce0)    # can't be changed
    add_custom_command(OUTPUT ${pac_fdl_files} ${out_hex_dir}/boot.sign.img ${out_hex_dir}/${BUILD_TARGET}.sign.img
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
        set(modemgen_dir ${BINARY_TOP_DIR}/modemgen/${nvmvariant})
        set(modem_img ${out_hex_dir}/${nvmvariant}.img)
        set(nvitem_bin ${out_hex_dir}/${nvmvariant}_nvitem.bin)
        set(prepack_cpio ${out_hex_dir}/${nvmvariant}_prepack.cpio)
        set(pac_file ${out_hex_dir}/${BUILD_TARGET}-${nvmvariant}-${BUILD_RELEASE_TYPE}.pac)

        add_custom_command(OUTPUT ${modem_img} ${nvitem_bin} ${prepack_cpio}
            COMMAND python3 ${modemgen_py} imggen8910
                --config ${SOURCE_TOP_DIR}/components/hal/config/8910/modem_nvm.json
                --source-top ${SOURCE_TOP_DIR}
                --binary-top ${BINARY_TOP_DIR}
                --cproot ${SOURCE_TOP_DIR}/prebuilts/modem/8910
                --aproot ${SOURCE_TOP_DIR}/components/nvitem/8910
                --workdir ${modemgen_dir}
                --fix-size ${CONFIG_NVBIN_FIXED_SIZE}
                --partinfo ${CONFIG_PARTINFO_JSON_PATH}
                --deltainc ${out_inc_dir}
                --dep ${modemgen_dir}/modem.d
                --deprel ${BINARY_TOP_DIR}
                --prepackfile ${CONFIG_PACKAGE_FILE_JSON_PATH}
                ${nvmvariant}
                ${prepack_cpio} ${nvitem_bin} ${modem_img}
            DEPFILE ${modemgen_dir}/modem.d
            WORKING_DIRECTORY ${SOURCE_TOP_DIR}
        )

        if(CONFIG_APP_FLASH2_ENABLED)
            set(target_flash2_bin ${out_hex_dir}/${BUILD_TARGET}.flash2.bin)
            set(cfg_ap2 cfg-image -i AP2 -a ${CONFIG_APP_FLASH2_ADDRESS}
                -s ${CONFIG_APP_FLASH2_SIZE} -p ${target_flash2_bin})
        endif()
        if(CONFIG_FS_EXT_ENABLED)
            set(cfg_fmt_fext cfg-fmt-flash -i FMT_FSEXT -b FEXT -n)
        endif()

        pac_init_fdl(init_fdl)
        add_custom_command(OUTPUT ${pac_file}
            COMMAND python3 ${pacgen_py} ${init_fdl}
                cfg-nvitem -n "Calibration" -i 0xFFFFFFFF --use 1 --replace 0 --continue 0 --backup 1
                cfg-nvitem -n "GSM Calibration" -i 0x26d --use 1 --replace 0 --continue 1 --backup 1
                cfg-nvitem -n "LTE Calibration" -i 0x26e --use 1 --replace 0 --continue 0 --backup 1
                cfg-nvitem -n "IMEI" -i 0xFFFFFFFF --use 1 --replace 0 --continue 0 --backup 1
                cfg-image -i BOOTLOADER -a ${CONFIG_BOOT_FLASH_ADDRESS} -s ${CONFIG_BOOTLOADER_SIZE}
                    -p ${out_hex_dir}/boot.sign.img
                cfg-image -i AP -a ${CONFIG_APP_FLASH_ADDRESS} -s ${CONFIG_APP_FLASH_SIZE}
                    -p ${out_hex_dir}/${BUILD_TARGET}.sign.img
                ${cfg_ap2}
                cfg-image -i PS -a ${CONFIG_FS_MODEM_FLASH_ADDRESS} -s ${CONFIG_FS_MODEM_FLASH_SIZE}
                    -p ${out_hex_dir}/${nvmvariant}.img
                cfg-fmt-flash -i FMT_FSSYS -b FSYS -n
                ${cfg_fmt_fext}
                cfg-clear-nv
                cfg-nv -s ${CONFIG_NVBIN_FIXED_SIZE} -p ${out_hex_dir}/${nvmvariant}_nvitem.bin
                cfg-pack-cpio -i PREPACK -p ${prepack_cpio}
                cfg-phase-check
                dep-gen --base ${BINARY_TOP_DIR} ${pac_file} ${pac_file}.d
                pac-gen ${pac_file}
            DEPFILE ${pac_file}.d
            DEPENDS ${pac_fdl_files} ${out_hex_dir}/boot.sign.img
                ${out_hex_dir}/${BUILD_TARGET}.sign.img
                ${out_hex_dir}/${nvmvariant}.img
                ${target_flash2_bin}
            WORKING_DIRECTORY ${SOURCE_TOP_DIR}
        )
        add_custom_target(${nvmvariant}_pacgen ALL DEPENDS ${pac_file})
    endforeach()
endfunction()
