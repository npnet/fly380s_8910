/* Copyright (C) 2017 RDA Technologies Limited and/or its affiliates("RDA").
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

#define OSI_LOCAL_LOG_TAG OSI_MAKE_LOG_TAG('F', 'D', 'L', '1')
#define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "boot_fdl.h"
#include "boot_entry.h"
#include "boot_platform.h"
#include "boot_debuguart.h"
#include "boot_fdl_channel.h"
#include "boot_mem.h"
#include "boot_adi_bus.h"
#include "boot_spi_flash.h"
#include "hal_chip.h"
#include "boot_secure.h"
#include "hal_config.h"
#include "hal_spi_flash.h"
#include "cmsis_core.h"
#include "osi_log.h"
#include <sys/reent.h>
#include <stdint.h>
#include <string.h>
#include <machine/endian.h>
#include "image_header.h"

typedef struct
{
    fdlDeviceInfo_t device;
    fdlDnld_t dnld;
} fdl1_t;

typedef void (*bootJumpFunc_t)(uint32_t param);

static void fdl1EngineSetBaud(fdlEngine_t *fdl, fdlPacket_t *pkt, fdlDeviceInfo_t *device)
{
    uint32_t *ptr = (uint32_t *)pkt->content;
    uint32_t baud = __ntohl(*ptr++);
    OSI_LOGD(0, "FDL1: change baud %d", baud);

    // This is special, ACK must be sent in old baud rate.
    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
    bootDelayUS(200);

    if (device->b.type == FDL_DEVICE_UART)
    {
        fdlEngineSetBaud(fdl, baud);
        device->b.baud = baud;
    }

    // if FDL_DEVICE_USB_SERIAL do nothing
}

static void fdl1EngineStart(fdlEngine_t *fdl, fdlPacket_t *pkt, fdlDnld_t *dnld)
{
    uint32_t *ptr = (uint32_t *)pkt->content;
    uint32_t start_addr = __ntohl(*ptr++);
    uint32_t file_size = __ntohl(*ptr++);

    OSI_LOGI(0, "FDL1: start_addr 0x%x, size %d ......", start_addr, file_size);
    if (CONFIG_FDL2_IMAGE_START == start_addr)
    {
        if (file_size > CONFIG_FDL2_IMAGE_SIZE)
        {
            OSI_LOGE(0, "fdl2 size beyond the limit...");

            fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_SIZE_ERROR);
            bootPanic();
        }

        memset((void *)start_addr, 0, file_size);

        fdlEngineSendRespNoData(fdl, BSL_REP_ACK);

        dnld->start_address = start_addr;
        dnld->write_address = start_addr;
        dnld->total_size = file_size;
        dnld->received_size = 0;
        dnld->stage = SYS_STAGE_START;
    }
    else
    {
        OSI_LOGE(0, "fdl2 addr error...");
        fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_DEST_ERROR);
        bootPanic();
    }
}

static void fdl1EngineMidst(fdlEngine_t *fdl, fdlPacket_t *pkt, fdlDnld_t *dnld)
{
    uint16_t data_len = pkt->size;
    if ((dnld->stage != SYS_STAGE_START) && (dnld->stage != SYS_STAGE_GATHER))
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_NOT_START);
        OSI_LOGE(0, "FDL1: down start err...");
        bootPanic();
    }

    if (dnld->received_size + data_len > dnld->total_size)
    {
        fdlEngineSendRespNoData(fdl, BSL_REP_DOWN_SIZE_ERROR);
        OSI_LOGE(0, "FDL1: down size err...");
        bootPanic();
    }

    OSI_LOGV(0, "write addr %x, size %d", dnld->write_address, data_len);
    memcpy((void *)dnld->write_address, (const void *)pkt->content, data_len);
    dnld->write_address += data_len;
    dnld->received_size += data_len;
    dnld->stage = SYS_STAGE_GATHER;

    fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
}

static void fdl1EngineEnd(fdlEngine_t *fdl, fdlDnld_t *dnld)
{
    int ret = checkUimageSign((void *)dnld->start_address);
    if (!ret)
    {
        //recv all packets
        OSI_LOGD(0, "FDL1: write file ok...");
        fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
        dnld->stage = SYS_STAGE_END;
    }
    else
    {
        OSI_LOGD(0, "FDL1: secure boot verify fail...");
        fdlEngineSendRespNoData(fdl, BSL_REP_SECURITY_VERIFICATION_FAIL);
    }
}

static void fdl1EngineExec(uint32_t param)
{
    const image_header_t *header = (const image_header_t *)CONFIG_FDL2_IMAGE_START;
    if (__ntohl(header->ih_magic) == IH_MAGIC)
    {
        bootJumpFunc_t entry = (bootJumpFunc_t)__ntohl(header->ih_ep);
        entry(param);
    }
}

static void _process_fdl1(fdlEngine_t *fdl, fdlPacket_t *pkt, void *f_)
{
    if (fdl == NULL || pkt == NULL)
        bootPanic();

    fdl1_t *f = (fdl1_t *)f_;
    switch (pkt->type)
    {
    case BSL_CMD_CONNECT:
        fdlEngineSendRespNoData(fdl, BSL_REP_ACK);
        break;

    case BSL_SET_BAUDRATE:
        fdl1EngineSetBaud(fdl, pkt, &f->device);
        break;

    case BSL_CMD_START_DATA:
        fdl1EngineStart(fdl, pkt, &f->dnld);
        break;

    case BSL_CMD_MIDST_DATA:
        fdl1EngineMidst(fdl, pkt, &f->dnld);
        break;

    case BSL_CMD_END_DATA:
        fdl1EngineEnd(fdl, &f->dnld);
        break;

    case BSL_CMD_EXEC_DATA:
        fdl1EngineExec(f->device.info);
        break;

    default:
        OSI_LOGE(0, "FDL1, cmd not support yet %x", pkt->type);
        bootPanic();
        break;
    }
}

static fdlEngine_t *_createFdlEngine(bool usb_serl)
{
    fdlChannel_t *ch = usb_serl ? fdlOpenUsbSerial() : fdlOpenUart(CONFIG_FDL_UART_BAUD);
    if (ch == NULL)
    {
        OSI_LOGE(0, "Fail to open (usb #%c) fdl channel", usb_serl ? 't' : 'f');
        return NULL;
    }

    fdlEngine_t *fdl = fdlEngineCreate(ch);
    if (fdl == NULL)
    {
        if (ch->destroy)
            ch->destroy(ch);
        return NULL;
    }
    return fdl;
}

static fdlEngine_t *_fdlInit(fdlDeviceInfo_t *device)
{
    fdlEngine_t *fdl_usb = _createFdlEngine(true);
    fdlEngine_t *fdl_uart = _createFdlEngine(false);
    if (!fdl_usb && !fdl_uart)
    {
        OSI_LOGE(0, "Can not create any fdl engine");
        return NULL;
    }

    fdlEngine_t *fdl = NULL;
    for (;;)
    {
        uint8_t sync;
        if (fdl_usb && fdlEngineReadRaw(fdl_usb, &sync, 1) && sync == HDLC_FLAG)
        {
            device->b.type = FDL_DEVICE_USB_SERIAL;
            fdl = fdl_usb;
            fdlEngineDestroy(fdl_uart);
            break;
        }

        if (fdl_uart && fdlEngineReadRaw(fdl_uart, &sync, 1) && sync == HDLC_FLAG)
        {
            device->b.type = FDL_DEVICE_UART;
            fdl = fdl_uart;
            fdlEngineDestroy(fdl_usb);
            break;
        }
    }

    if (!fdlEngineSendVersion(fdl))
    {
        OSI_LOGE(0, "FDL %d fail to send version string", device->b.type);
        bootPanic();
    }

    return fdl;
}

void bootStart(uint32_t param)
{
    halSpiFlashStatusCheck(0);
    halClockInit();

    bootInitRunTime();
    __FPU_Enable();
    _impure_ptr = _GLOBAL_REENT;

    bootDebuguartInit();
    bootAdiBusInit();
    bootHeapInit((uint32_t *)CONFIG_BOOT_SRAM_HEAP_START, CONFIG_BOOT_SRAM_HEAP_SIZE, 0, 0);

    OSI_LOGI(0, "FDL1RUN ......");

    fdlDeviceInfo_t device = {
        .b.type = FDL_DEVICE_UART,
        .b.baud = CONFIG_FDL_UART_BAUD,
    };

    fdlEngine_t *fdl = _fdlInit(&device);
    if (!fdl)
    {
        OSI_LOGE(0, "FDL1 fail, can not create fdl engine");
        bootPanic();
    }

    OSI_LOGI(0, "FDL1 device %d", device.b.type);
    fdl1_t fdl1_context = {
        .device = device,
    };
    fdlEngineProcess(fdl, _process_fdl1, &fdl1_context);

    // never return here
    fdlEngineDestroy(fdl);
    bootPanic();
}
