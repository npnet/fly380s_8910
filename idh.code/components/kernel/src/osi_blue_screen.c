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

#include "osi_api.h"
#include "osi_profile.h"
#include "osi_log.h"
#include "osi_trace.h"
#include "hwregs.h"
#include "drv_uart.h"
#include "drv_axidma.h"
#include "drv_md_ipc.h"
#include "hal_chip.h"
#ifdef CONFIG_CPU_ARM
#include "cmsis_core.h"
#endif
#include <assert.h>

#define GDB_EVENT 0x9db00000

#ifdef CONFIG_CPU_MIPS
#define GDB_CMD_BREAKPOINT 0x10

typedef struct
{
    uint32_t gpreg[32];
    uint32_t sr;
    uint32_t lo;
    uint32_t hi;
    uint32_t bad;
    uint32_t cause;
    uint32_t epc;

    uint32_t cmd;
    uint32_t par1;
    uint32_t par2;
    uint32_t dummy;
} osiMipsContext_t;

static void prvSaveContext(void *ctx)
{
    osiMipsContext_t *regs = (osiMipsContext_t *)ctx;

    regs->cmd = GDB_CMD_BREAKPOINT;
    xcpu_sp_context = (unsigned)regs;
    OSI_LOGE(0, "!!! BLUE SCREEN AT %p", regs->epc);
}
#endif

#ifdef CONFIG_SOC_8910
typedef struct
{
    uint32_t fpscr;
    uint32_t dacr;
    uint32_t ttbr0;
    uint32_t sctlr;
    uint32_t usr_sp; // incoming pointer
    uint32_t usr_lr;
    uint32_t svc_sp;
    uint32_t svc_lr;
    uint32_t sp;
    uint32_t lr;
    uint32_t cpsr;
    uint32_t spsr;
    uint32_t elr;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t ip;
} osiArmContext_t;

uint32_t gGdbCtxType OSI_SECTION_RW_KEEP = 1;
static uint32_t *gCpGdbRegAddress OSI_SECTION_RW_KEEP = (uint32_t *)0x80002800;
uint32_t *gGdbRegAddress = NULL;

static void prvSaveContext(void *ctx)
{
    // ctx points to "usr_sp", gGdbRegAddress points to "sp"
    osiArmContext_t *regs = (osiArmContext_t *)((uintptr_t)ctx - OSI_OFFSETOF(osiArmContext_t, usr_sp));
    gGdbRegAddress = (uint32_t *)((uintptr_t)regs + OSI_OFFSETOF(osiArmContext_t, sp));

    if ((regs->spsr & 0x1f) == 0x1f)
    {
        regs->sp = regs->usr_sp;
        regs->lr = regs->usr_lr;
    }
    else
    {
        regs->sp = regs->svc_sp;
        regs->lr = regs->svc_lr;
    }

    regs->sctlr = __get_SCTLR();
    regs->ttbr0 = __get_TTBR0();
    regs->dacr = __get_DACR();
    regs->fpscr = __get_FPSCR();

    ipc_show_cp_assert();
    ipc_notify_cp_assert();
    OSI_LOGE(0, "!!! BLUE SCREEN pc/0x%x lr/0x%x sp/0x%x cpsr/0x%x spsr/0x%x r0/0x%x r1/0x%x r2/0x%x r3/0x%x r4/0x%x r5/0x%x r6/0x%x r7/0x%x r8/0x%x r9/0x%x r10/0x%x r11/0x%x r12/0x%x",
             regs->elr, regs->lr, regs->sp, regs->cpsr, regs->spsr,
             regs->r0, regs->r1, regs->r2, regs->r3,
             regs->r4, regs->r5, regs->r6, regs->r7,
             regs->r8, regs->r9, regs->r10, regs->r11,
             regs->ip);
}
#endif

typedef struct
{
    osiCallback_t enter;
    osiCallback_t poll;
    void *param;
} osiBlueScreenHandler_t;

static bool gIsPanic = false;
static osiBlueScreenHandler_t gBlueScreenHandlers[CONFIG_KERNEL_BLUE_SCREEN_HANDLER_COUNT];

OSI_NO_RETURN OSI_SECTION_SRAM_TEXT void osiPanic(void)
{
    void *address = __builtin_return_address(0);
    OSI_LOGE(0, "!!! PANIC AT %p", address);
    osiProfileCode(PROFCODE_PANIC);
    __builtin_trap();
}

OSI_NO_RETURN OSI_SECTION_SRAM_TEXT void osiPanicAt(void *address)
{
    OSI_LOGE(0, "!!! PANIC AT %p", address);
    osiProfileCode(PROFCODE_PANIC);
    __builtin_trap();
}

bool osiIsPanic(void)
{
    return gIsPanic;
}

OSI_NO_RETURN void osiBlueScreen(uint32_t type, void *regs)
{
    (void)osiIrqSave(); // disable interrupt

    if (!gIsPanic)
    {
        prvSaveContext(regs);

        osiDCacheCleanInvalidateAll();
        osiDebugEvent(GDB_EVENT);
        osiProfileCode(PROFCODE_BLUE_SCREEN);
        osiDCacheCleanInvalidateAll();

        for (unsigned n = 0; n < CONFIG_KERNEL_BLUE_SCREEN_HANDLER_COUNT; n++)
        {
            if (gBlueScreenHandlers[n].enter != NULL)
                gBlueScreenHandlers[n].enter(gBlueScreenHandlers[n].param);
        }

        osiShowThreadState();
    }

    gIsPanic = true;
    for (;;)
    {
        for (unsigned n = 0; n < CONFIG_KERNEL_BLUE_SCREEN_HANDLER_COUNT; n++)
        {
            if (gBlueScreenHandlers[n].poll != NULL)
                gBlueScreenHandlers[n].poll(gBlueScreenHandlers[n].param);
        }
        osiDCacheCleanInvalidateAll();
    }
}

bool osiRegisterBlueScreenHandler(osiCallback_t enter, osiCallback_t poll, void *param)
{
    if (enter == NULL && poll == NULL)
        return false;

    for (unsigned n = 0; n < CONFIG_KERNEL_BLUE_SCREEN_HANDLER_COUNT; n++)
    {
        osiBlueScreenHandler_t *p = &gBlueScreenHandlers[n];
        if (p->enter == NULL && p->poll == NULL)
        {
            p->enter = enter;
            p->poll = poll;
            p->param = param;
            return true;
        }
    }
    return false;
}
