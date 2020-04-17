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

set(CONFIG_GIC_BASE_ADDRESS 0x08201000)
set(CONFIG_GIC_CPU_INTERFACE_OFFSET 0x00001000)

set(CONFIG_UIMAGE_HEADER_SIZE 0x40)
set(CONFIG_ROM_LOAD_SRAM_OFFSET 0xc0)
set(CONFIG_ROM_LOAD_SIZE 0xbf40)
math(EXPR CONFIG_ROM_SRAM_LOAD_ADDRESS "${CONFIG_SRAM_PHY_ADDRESS}+${CONFIG_ROM_LOAD_SRAM_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)

math(EXPR CONFIG_BOOT_EXCEPTION_STACK_SIZE "${CONFIG_BOOT_SVC_STACK_SIZE}+${CONFIG_BOOT_IRQ_STACK_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_BOOT_SVC_STACK_TOP "${CONFIG_BOOT_EXCEPTION_STACK_START}+${CONFIG_BOOT_SVC_STACK_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_BOOT_IRQ_STACK_TOP "${CONFIG_BOOT_SVC_STACK_TOP}+${CONFIG_BOOT_IRQ_STACK_SIZE}" OUTPUT_FORMAT HEXADECIMAL)

math(EXPR CONFIG_BOOTLOADER_SIZE "${CONFIG_BOOT_FLASH_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_BOOT_FLASH_ADDRESS "${CONFIG_NOR_PHY_ADDRESS}+${CONFIG_BOOT_FLASH_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_APP_FLASH_ADDRESS "${CONFIG_NOR_PHY_ADDRESS}+${CONFIG_APP_FLASH_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_FS_MODEM_FLASH_ADDRESS "${CONFIG_NOR_PHY_ADDRESS}+${CONFIG_FS_MODEM_FLASH_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)

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

math(EXPR CONFIG_APPIMG_FLASH_ADDRESS
    "${CONFIG_NOR_PHY_ADDRESS}+${CONFIG_APPIMG_FLASH_OFFSET}"
    OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONFIG_APP_FILEIMG_RAM_OFFSET
    "${CONFIG_APP_RAM_OFFSET}+${CONFIG_APP_TOTAL_RAM_SIZE}-${CONFIG_APP_FILEIMG_RAM_SIZE}"
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

set(modem_libs)
set(sysrom_elf)
set(unittest_ldscript ${SOURCE_TOP_DIR}/components/hal/ldscripts/flashrun.ld)

# Configuration all nvmvariants
set(modembin_dir prebuilts/modem/8910)
set(pdeltanv_dir components/nvitem/8910/param_deltanv)

config_nvm_variant_8910(cat1_128X256_UIS8910A_module
    ${modembin_dir}/cat1_128X256_UIS8910A_module nvitem)

config_nvm_variant_8910(catm_128X256_UIS8910A_NoIratNoDualsim
    ${modembin_dir}/catm_128X256_UIS8910A_NoIratNoDualsim nvitem)

config_nvm_variant_8910(catm_128X256_UIS8910A_NoIratNoDualsim_module_DTForDingLi
    ${modembin_dir}/catm_128X256_UIS8910A_NoIratNoDualsim_module_DTForDingLi nvitem)

config_nvm_variant_8910(cat1_128X256_UIS8910A_module_DTForDingLi
    ${modembin_dir}/cat1_128X256_UIS8910A_module_DTForDingLi nvitem)

config_nvm_variant_8910(catm_128X256_UIS8910A_EM610_NoIratNoDualsim
    ${modembin_dir}/catm_128X256_UIS8910A_EM610_NoIratNoDualsim nvitem)

config_nvm_variant_8910(catm_128X128_UIS8910C_EM610_NoIratNoDualsim
    ${modembin_dir}/catm_128X256_UIS8910A_EM610_NoIratNoDualsim nvitem)

config_nvm_variant_8910(catm_128X128_UIS8910C_EM610V2_NoIratNoDualsim_PHYTEST
    ${modembin_dir}/catm_128X128_UIS8910C_EM610V2_NoIratNoDualsim_PHYTEST nvitem)

config_nvm_variant_8910(cat1_128X128_UIS8910C_EM610V2_NoDualsim_PHYTEST
    ${modembin_dir}/cat1_128X128_UIS8910C_EM610V2_NoDualsim_PHYTEST nvitem)

config_nvm_variant_8910(cat1_128X128_UIS8910C_EM610V2_NoDualsim
    ${modembin_dir}/cat1_128X128_UIS8910C_EM610V2_NoDualsim nvitem)

config_nvm_variant_8910(catm_128X128_UIS8910C_EM610V2_NoIratNoDualsim
    ${modembin_dir}/catm_128X128_UIS8910C_EM610V2_NoIratNoDualsim nvitem)

config_nvm_variant_8910(cat1_UIS8915DM_UIS8910GF_SingleSim
    ${modembin_dir}/cat1_UIS8915DM_UIS8910GF_SingleSim nvitem)

config_nvm_variant_8910(cat1_UIS8910DM_L04_SingleSim
    ${modembin_dir}/cat1_UIS8910DM_L04_SingleSim nvitem)

config_nvm_variant_8910(cat1_64X128_UIS8915DM_SingleSim
    ${modembin_dir}/cat1_64X128_UIS8915DM_SingleSim nvitem)

config_nvm_variant_8910(UIS8915DM_NONBL_SS
    ${modembin_dir}/UIS8915DM_NONBL_SS nvitem)

config_nvm_variant_8910(UIS8915DM_NONBL_SS_PHYTEST
    ${modembin_dir}/UIS8915DM_NONBL_SS_PHYTEST nvitem)

config_nvm_variant_8910(UIS8915DM_NONBL_BB_RF_SS
    ${modembin_dir}/UIS8915DM_NONBL_BB_RF_SS nvitem)

config_nvm_variant_8910(UIS8915DM_NONBL_BB_RF_SS_PHYTEST
    ${modembin_dir}/UIS8915DM_NONBL_BB_RF_SS_PHYTEST nvitem)

config_nvm_variant_8910(cat1_64X128_UIS8915DM_SingleSim_DTForDingLi
    ${modembin_dir}/cat1_64X128_UIS8915DM_SingleSim_DTForDingLi nvitem)

config_nvm_variant_8910(cat1_64X128_UIS8915DM_SingleSim_PHYTEST
    ${modembin_dir}/cat1_64X128_UIS8915DM_SingleSim_PHYTEST nvitem)

config_nvm_variant_8910(cat1_64X128_UIS8915DM_BB_RF_SingleSim
    ${modembin_dir}/cat1_64X128_UIS8915DM_BB_RF_SingleSim nvitem)

config_nvm_variant_8910(cat1_64X128_UIS8915DM_BB_RF_SingleSim_PHYTEST
    ${modembin_dir}/cat1_64X128_UIS8915DM_BB_RF_SingleSim_PHYTEST nvitem)

config_nvm_variant_8910(catm_64X128_UIS8915DM_NoIratSingleSim
    ${modembin_dir}/catm_64X128_UIS8915DM_NoIratSingleSim nvitem)

config_nvm_variant_8910(catm_64X128_UIS8915DM_NoIratSingleSim_DTForDingLi
    ${modembin_dir}/catm_64X128_UIS8915DM_NoIratSingleSim_DTForDingLi nvitem)

config_nvm_variant_8910(catm_64X128_UIS8915DM_NoIratSingleSim_PHYTEST
    ${modembin_dir}/catm_64X128_UIS8915DM_NoIratSingleSim_PHYTEST nvitem)

config_nvm_variant_8910(catm_64X128_UIS8915DM_NoIratSingleSim_3GPPR14
    ${modembin_dir}/catm_64X128_UIS8915DM_NoIratSingleSim_3GPPR14 nvitem)

config_nvm_variant_8910(catm_64X128_UIS8915DM_BB_RF_NoIratSingleSim
    ${modembin_dir}/catm_64X128_UIS8915DM_BB_RF_NoIratSingleSim nvitem)

config_nvm_variant_8910(UIS8915DM_CATM_BB_RF_NoIratSingleSim_3GPPR14
    ${modembin_dir}/UIS8915DM_CATM_BB_RF_NoIratSingleSim_3GPPR14 nvitem)

config_nvm_variant_8910(catm_UIS8915DM_BB_RF_NoIratSingleSim_PHYTEST
    ${modembin_dir}/catm_UIS8915DM_BB_RF_NoIratSingleSim_PHYTEST nvitem)

config_nvm_variant_8910(catm_nbiot_64x128_NoIratSingleSim
    ${modembin_dir}/catm_nbiot_64x128_NoIratSingleSim nvitem)

config_nvm_variant_8910(cat1_UIS8915DM_BB_RF_SS_NoVolte
    ${modembin_dir}/cat1_UIS8915DM_BB_RF_SS_NoVolte nvitem)

