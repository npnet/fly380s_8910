/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/reent.h>
#include "hal_config.h"
#include "cmsis_core.h"
#include "osi_api.h"
#include "osi_compiler.h"
#include "osi_profile.h"
#include "osi_tick_unit.h"
#include "osi_mem.h"
#include "osi_log.h"
#include "osi_trace.h"
#include "hwregs.h"
#include "hal_chip.h"
#include "hal_mmu.h"
#include "hal_adi_bus.h"
#include "hal_ram_cfg.h"
#include "hal_cp_ctrl.h"
#include "hal_iomux.h"
#include "drv_md_ipc.h"
#include "drv_md_nvm.h"
#include "drv_ipc_at.h"
#include "hal_spi_flash.h"
#include <string.h>

#if 0
#define SUSPEND_DEBUG
#else
#define SUSPEND_DEBUG hwp_idle->idle_res9 = __LINE__
#endif

#define DELAYUS(us) halApplyRegisters(REG_APPLY_UDELAY(us), REG_APPLY_END)
#define IDLE_SLEEP_ENABLE_MAGIC 0x49444c45
#define AP_WAKEUP_JUMP_MAGIC 0xD8E5BEAF
#define PMIC_PWR_PROT_MAGIC 0x6e7f

#define AP_WAKEUP_JUMP_MAGIC_REG (hwp_idle->idle_res0)
#define AP_WAKEUP_JUMP_ADDR_REG (hwp_idle->idle_res1)

#define AP_SRAM_BACKUP_START_SYM __sramtext_start
#define AP_SRAM_BACKUP_END_SYM __srambss_end

#define AP_SRAM_BACKUP_START ((uintptr_t)(&AP_SRAM_BACKUP_START_SYM))
#define AP_SRAM_BACKUP_END ((uintptr_t)(&AP_SRAM_BACKUP_END_SYM))
#define AP_SRAM_BACKUP_SIZE (AP_SRAM_BACKUP_END - AP_SRAM_BACKUP_START)

#define AP_SRAM_SHMEM_START (CONFIG_SRAM_PHY_ADDRESS + CONFIG_APP_SRAM_SHMEM_OFFSET)

typedef struct
{
    osiSuspendMode_t mode;
    uint32_t reg_sys[11]; // r4-r14
    uint32_t cpsr_sys;
    uint32_t ttbr0;
    uint32_t dacr;
    uint8_t sram[CONFIG_APP_SRAM_SIZE - CONFIG_STACK_BOTTOM];
    uint8_t sram_shmem[CONFIG_APP_SRAM_SHMEM_SIZE];
} cpuSuspendContext_t;

static cpuSuspendContext_t gCpuSuspendCtx;

extern void WakeupPm1_Handler(void);
extern void WakeupPm2_Handler(void);
extern int SaveContext(void *ctx, void (*post)(void));
extern OSI_NO_RETURN void RestoreContext(void *ctx);

static void _sramBackup(void)
{
    extern uint32_t AP_SRAM_BACKUP_START_SYM;
    extern uint32_t AP_SRAM_BACKUP_END_SYM;
    memcpy(gCpuSuspendCtx.sram, (void *)AP_SRAM_BACKUP_START, AP_SRAM_BACKUP_SIZE);
    memcpy(gCpuSuspendCtx.sram_shmem, (void *)AP_SRAM_SHMEM_START, CONFIG_APP_SRAM_SHMEM_SIZE);
}

static void _sramRestore(void)
{
    extern uint32_t AP_SRAM_BACKUP_START_SYM;
    extern uint32_t AP_SRAM_BACKUP_END_SYM;
    memcpy((void *)AP_SRAM_BACKUP_START, gCpuSuspendCtx.sram, AP_SRAM_BACKUP_SIZE);
    memcpy((void *)AP_SRAM_SHMEM_START, gCpuSuspendCtx.sram_shmem, CONFIG_APP_SRAM_SHMEM_SIZE);
}

static inline void _mmuInit(void)
{
    // Invalidate entire Unified TLB
    __set_TLBIALL(0);

    // Invalidate entire branch predictor array
    __set_BPIALL(0);
    __DSB();
    __ISB();

    //  Invalidate instruction cache and flush branch target cache
    __set_ICIALLU(0);
    __DSB();
    __ISB();

    //  Invalidate data cache
    L1C_InvalidateDCacheAll();
    L1C_InvalidateICacheAll();

    // Create Translation Table
    halMmuCreateTable();

    // Enable MMU
    MMU_Enable();

    // Enable Caches
    L1C_EnableCaches();
    L1C_EnableBTAC();
}

static void _mmuWakeInit(void)
{
    // Invalidate entire Unified TLB
    __set_TLBIALL(0);

    // Invalidate entire branch predictor array
    __set_BPIALL(0);
    __DSB();
    __ISB();

    //  Invalidate instruction cache and flush branch target cache
    __set_ICIALLU(0);
    __DSB();
    __ISB();

    //  Invalidate data cache
    L1C_InvalidateDCacheAll();

    // Create Translation Table
    __set_TTBR0(gCpuSuspendCtx.ttbr0);
    __ISB();
    __set_DACR(gCpuSuspendCtx.dacr);
    __ISB();

    // Enable MMU
    MMU_Enable();

    // Enable Caches
    L1C_EnableCaches();
    L1C_EnableBTAC();
}

static void _heapInit(void)
{
    extern uint32_t __heap_start;
    extern uint32_t __heap_end;

    void *heap_start = (void *)&__heap_start;
    uint32_t heap_size = (uint32_t)&__heap_end - (uint32_t)&__heap_start;

    osiMemPool_t *ram_heap = osiBlockPoolInit(heap_start, heap_size, 0);
    osiPoolSetDefault(ram_heap);

    OSI_LOGI(0, "ram heap start/%p size/%d pool/%p", heap_start, heap_size, ram_heap);
}

OSI_NO_RETURN void osiBootStart(uint32_t param)
{
    // I cache is enabled, and D cache is disabled
    OSI_LOAD_SECTION(sramboottext);
    OSI_LOAD_SECTION(sramtext);
    OSI_LOAD_SECTION(sramdata);
    OSI_CLEAR_SECTION(srambss);

    // sync cache after code copy
    L1C_InvalidateICacheAll();

    halClockInit();
    halRamInit();
    _mmuInit();

    OSI_LOAD_SECTION(ramtext);
    OSI_LOAD_SECTION(data);
    OSI_CLEAR_SECTION(bss);

    // sync cache after code copy
    L1C_CleanDCacheAll();
    __DSB();
    __ISB();

    __FPU_Enable();
    AP_WAKEUP_JUMP_MAGIC_REG = 0;
    _impure_ptr = _GLOBAL_REENT;

    osiTraceBufInit();
    osiProfileInit();
    osiPmInit();
    _heapInit();

    halIomuxInit();
    halAdiBusInit();

    halPmuInit();
    halPmuExtFlashPowerOn();
    osiKernelStart();
}

OSI_NO_RETURN void osiWakePm1(void)
{
    // now: I cache is enabled, D cache is disabled
    OSI_LOAD_SECTION(sramboottext);

    // sync cache after code copy
    L1C_InvalidateICacheAll();

    halApplyRegisters((uint32_t)&hwp_spiFlash->spi_config, 0x221,
                      REG_APPLY_END);

    halRamWakeInit();
    _mmuWakeInit();
    __FPU_Enable();

    _sramRestore();

    // sync cache after code copy
    L1C_CleanDCacheAll();
    __DSB();
    __ISB();

    RestoreContext(gCpuSuspendCtx.reg_sys);
}

OSI_NO_RETURN void osiWakePm2(void)
{
    // now: I cache is enabled, D cache is disabled
    OSI_LOAD_SECTION(sramboottext);

    // sync cache after code copy
    L1C_InvalidateICacheAll();

    halClockInit();
    halRamWakeInit();
    _mmuWakeInit();
    __FPU_Enable();

    _sramRestore();

    // sync cache after code copy
    L1C_CleanDCacheAll();
    __DSB();
    __ISB();

    RestoreContext(gCpuSuspendCtx.reg_sys);
}

static uint32_t _clearWakeSource(bool aborted)
{
    REG_CP_IDLE_IDL_AWK_ST_T awk_st = {hwp_idle->idl_awk_st};
    REG_CP_IDLE_IDL_AWK_ST_T awk_st_clr = {
        .b.awk0_awk_stat = 1, // pmic
        .b.awk1_awk_stat = 0, // vad_int
        .b.awk2_awk_stat = 1, // key
        .b.awk3_awk_stat = 1, // gpio1
        .b.awk4_awk_stat = 1, // uart1
        .b.awk5_awk_stat = 1, // pad uart1_rxd
        .b.awk6_awk_stat = 1, // wcn2sys
        .b.awk7_awk_stat = 1, // pad wcn_osc_en
        .b.awk_osw2_stat = 1};
    hwp_idle->idl_awk_st = awk_st_clr.v;

    uint32_t source = 0;
    if (awk_st.b.awk0_awk_stat)
        source |= HAL_RESUME_SRC_PMIC;
    if (awk_st.b.awk1_awk_stat)
        source |= HAL_RESUME_SRC_VAD;
    if (awk_st.b.awk2_awk_stat)
        source |= HAL_RESUME_SRC_KEY;
    if (awk_st.b.awk3_awk_stat)
        source |= HAL_RESUME_SRC_GPIO1;
    if (awk_st.b.awk4_awk_stat)
        source |= HAL_RESUME_SRC_UART1;
    if (awk_st.b.awk5_awk_stat)
        source |= HAL_RESUME_SRC_UART1_RXD;
    if (awk_st.b.awk6_awk_stat)
        source |= HAL_RESUME_SRC_WCN2SYS;
    if (awk_st.b.awk7_awk_stat)
        source |= HAL_RESUME_SRC_WCN_OSC;
    if (awk_st.b.awk_osw1_stat)
        source |= HAL_RESUME_SRC_IDLE_TIMER1;
    if (awk_st.b.awk_osw2_stat)
        source |= HAL_RESUME_SRC_IDLE_TIMER2;
    if (awk_st.b.awk_self_stat)
        source |= HAL_RESUME_SRC_SELF;
    return source;
}

static void _suspendPost(void)
{
    osiSuspendMode_t mode = gCpuSuspendCtx.mode;

    if (mode == OSI_SUSPEND_PM1)
    {
        halPmuEnterPm1();
        AP_WAKEUP_JUMP_ADDR_REG = (uint32_t)WakeupPm1_Handler;
    }
    else
    {
        halPmuEnterPm2();
        AP_WAKEUP_JUMP_ADDR_REG = (uint32_t)WakeupPm2_Handler;
    }

    _sramBackup();

    // go to sleep, really
    L1C_CleanDCacheAll();
    __DSB();

    hwp_idle->idl_ctrl_sys2 = IDLE_SLEEP_ENABLE_MAGIC;
    AP_WAKEUP_JUMP_MAGIC_REG = AP_WAKEUP_JUMP_MAGIC;

    __DSB();
    __ISB();
    __WFI();
}

uint32_t osiPmCpuSuspend(osiSuspendMode_t mode, int64_t sleep_ms)
{
    gCpuSuspendCtx.mode = mode;
    gCpuSuspendCtx.cpsr_sys = __get_CPSR();
    gCpuSuspendCtx.ttbr0 = __get_TTBR0();
    gCpuSuspendCtx.dacr = __get_DACR();

    REG_CP_IDLE_IDL_OSW2_EN_T idl_osw2_en = {};
    if (sleep_ms != INT64_MAX)
    {
        uint32_t ticks = OSI_MS_TO_TICK16K(sleep_ms);

        idl_osw2_en.b.osw2_en = 1;
        idl_osw2_en.b.osw2_time = (ticks >= 0x7fffffff) ? 0x7fffffff : ticks;
    }
    hwp_idle->idl_osw2_en = idl_osw2_en.v;

    if (SaveContext(gCpuSuspendCtx.reg_sys, _suspendPost) == 1)
    {
        // come here when suspend failed.
        hwp_idle->idl_awk_self = 1;
        OSI_POLL_WAIT(hwp_idle->idl_ctrl_sys2 == 0);

        hwp_idle->idl_awk_self = 0;
        AP_WAKEUP_JUMP_MAGIC_REG = 0;

        uint32_t source = _clearWakeSource(true);
        source |= OSI_RESUME_ABORT;

        if (mode == OSI_SUSPEND_PM1)
            halPmuAbortPm1();
        else
            halPmuAbortPm2();
        return source;
    }

    // come here from wakeup handler
    __DMB();

    __set_CPSR(gCpuSuspendCtx.cpsr_sys);

    hwp_idle->idl_osw2_en = 0; // disable it

    if (mode == OSI_SUSPEND_PM1)
        halPmuExitPm1();
    else
        halPmuExitPm2();

    uint32_t source = _clearWakeSource(false);
    if (gCpuSuspendCtx.mode == OSI_SUSPEND_PM2)
    {
        hwp_sysCtrl->cfg_misc_cfg |= (1 << 12); // ap_uart_out_sel
    }
    return source;
}

uint64_t osiCpDeepSleepTime()
{
    int64_t sleep_ms;
    uint32_t tick;

    REG_CP_IDLE_IDL_OSW1_EN_T idl_osw1_en = {hwp_idle->idl_osw1_en};
    tick = idl_osw1_en.b.osw1_time;
    sleep_ms = OSI_TICK16K_TO_MS(tick);

    return sleep_ms;
}
