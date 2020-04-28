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

// #define OSI_LOCAL_LOG_LEVEL OSI_LOG_LEVEL_DEBUG

#include "drv_lcd_panel.h"
#include "osi_api.h"
#include "osi_log.h"
#include "hwregs.h"

#define LCD_CMD_READ_ID (0x04)

static void prvSt7735sSetDir(drvLcd_t *d, drvLcdDirection_t dir)
{
    // MY MX MV ML BGR MH x x
    uint8_t ctrl = 0x8; // default value except MY/MX/MV
    if (dir & 1)
        ctrl |= 0x80;
    if (dir & 2)
        ctrl |= 0x40;
    if (dir & 4)
        ctrl |= 0x20;

    drvLcdWriteCmd(d, 0x36); // memory access control
    drvLcdWriteData(d, ctrl);
}

static void prvSt7735sInit(drvLcd_t *d)
{
    const drvLcdPanelDesc_t *desc = drvLcdGetDesc(d);
    drvLcdWriteCmd(d, 0x11); //Sleep out
    osiThreadSleep(100); //Delay 120ms
    //------------------------------------ST7735S Frame Rate-----------------------------------------//
    drvLcdWriteCmd(d, 0xB1);
    drvLcdWriteData(d, 0x05);
    drvLcdWriteData(d, 0x3C);
    drvLcdWriteData(d, 0x3C);
    drvLcdWriteCmd(d, 0xB2);
    drvLcdWriteData(d, 0x05);
    drvLcdWriteData(d, 0x3C);
    drvLcdWriteData(d, 0x3C);
    drvLcdWriteCmd(d, 0xB3);
    drvLcdWriteData(d, 0x05);
    drvLcdWriteData(d, 0x3C);
    drvLcdWriteData(d, 0x3C);
    drvLcdWriteData(d, 0x05);
    drvLcdWriteData(d, 0x3C);
    drvLcdWriteData(d, 0x3C);
    //------------------------------------End ST7735S Frame Rate-----------------------------------------//
    drvLcdWriteCmd(d, 0xB4); //Dot inversion
    drvLcdWriteData(d, 0x03);
    drvLcdWriteCmd(d, 0xC0);
    drvLcdWriteData(d, 0x28);
    drvLcdWriteData(d, 0x08);
    drvLcdWriteData(d, 0x04);
    drvLcdWriteCmd(d, 0xC1);
    drvLcdWriteData(d, 0XC0);
    drvLcdWriteCmd(d, 0xC2);
    drvLcdWriteData(d, 0x0D);
    drvLcdWriteData(d, 0x00);
    drvLcdWriteCmd(d, 0xC3);
    drvLcdWriteData(d, 0x8D);
    drvLcdWriteData(d, 0x2A);
    drvLcdWriteCmd(d, 0xC4);
    drvLcdWriteData(d, 0x8D);
    drvLcdWriteData(d, 0xEE);
    //---------------------------------End ST7735S Power Sequence-------------------------------------//
    drvLcdWriteCmd(d, 0xC5); //VCOM
    drvLcdWriteData(d, 0x1A);
    //drvLcdWriteCmd(d, 0x36); //MX, MY, RGB mode
    //drvLcdWriteData(d, 0xC0);
    prvSt7735sSetDir(d, desc->dir);
    //------------------------------------ST7735S Gamma Sequence-----------------------------------------//
    drvLcdWriteCmd(d, 0xE0);
    drvLcdWriteData(d, 0x04);
    drvLcdWriteData(d, 0x22);
    drvLcdWriteData(d, 0x07);
    drvLcdWriteData(d, 0x0A);
    drvLcdWriteData(d, 0x2E);
    drvLcdWriteData(d, 0x30);
    drvLcdWriteData(d, 0x25);
    drvLcdWriteData(d, 0x2A);
    drvLcdWriteData(d, 0x28);
    drvLcdWriteData(d, 0x26);
    drvLcdWriteData(d, 0x2E);
    drvLcdWriteData(d, 0x3A);
    drvLcdWriteData(d, 0x00);
    drvLcdWriteData(d, 0x01);
    drvLcdWriteData(d, 0x03);
    drvLcdWriteData(d, 0x13);
    drvLcdWriteCmd(d, 0xE1);
    drvLcdWriteData(d, 0x04);
    drvLcdWriteData(d, 0x16);
    drvLcdWriteData(d, 0x06);
    drvLcdWriteData(d, 0x0D);
    drvLcdWriteData(d, 0x2D);
    drvLcdWriteData(d, 0x26);
    drvLcdWriteData(d, 0x23);
    drvLcdWriteData(d, 0x27);
    drvLcdWriteData(d, 0x27);
    drvLcdWriteData(d, 0x25);
    drvLcdWriteData(d, 0x2D);
    drvLcdWriteData(d, 0x3B);
    drvLcdWriteData(d, 0x00);
    drvLcdWriteData(d, 0x01);
    drvLcdWriteData(d, 0x04);
    drvLcdWriteData(d, 0x13);
    //------------------------------------End ST7735S Gamma Sequence-----------------------------------------//
    drvLcdWriteCmd(d, 0x3A); //65k mode
    drvLcdWriteData(d, 0x05);
    drvLcdWriteCmd(d, 0x29); //Display on

    osiThreadSleep(20);
    drvLcdWriteCmd(d, 0x2c);
}

static void prvSt7735sBlitPrepare(drvLcd_t *d, drvLcdDirection_t dir, const drvLcdArea_t *roi)
{
    OSI_LOGD(0, "ST7735S dir/%d roi/%d/%d/%d/%d", dir, roi->x, roi->y, roi->w, roi->h);

    prvSt7735sSetDir(d, dir);

    uint16_t left = roi->x;
    uint16_t right = drvLcdAreaEndX(roi);
    uint16_t top = roi->y;
    uint16_t bot = drvLcdAreaEndY(roi);

    drvLcdWriteCmd(d, 0x2a);                 // set hori start , end (left, right)
    drvLcdWriteData(d, (left >> 8) & 0xff);  // left high 8 b
    drvLcdWriteData(d, left & 0xff);         // left low 8 b
    drvLcdWriteData(d, (right >> 8) & 0xff); // right high 8 b
    drvLcdWriteData(d, right & 0xff);        // right low 8 b

    drvLcdWriteCmd(d, 0x2b);               // set vert start , end (top, bot)
    drvLcdWriteData(d, (top >> 8) & 0xff); // top high 8 b
    drvLcdWriteData(d, top & 0xff);        // top low 8 b
    drvLcdWriteData(d, (bot >> 8) & 0xff); // bot high 8 b
    drvLcdWriteData(d, bot & 0xff);        // bot low 8 b

    drvLcdWriteCmd(d, 0x2c); // recover memory write mode
}

static uint32_t prvSt7735sReadId(drvLcd_t *d)
{
    uint8_t id[4];
    drvLcdReadData(d, LCD_CMD_READ_ID, id, 4);

    uint32_t dev_id = (id[3] << 16) | (id[2] << 8) | id[1];
    OSI_LOGI(0, "ST7735S read id: 0x%08x", dev_id);
    return dev_id;
}

static bool prvSt7735sProbe(drvLcd_t *d)
{
    const drvLcdPanelDesc_t *desc = drvLcdGetDesc(d);

    OSI_LOGI(0, "ST7735S probe");
    return prvSt7735sReadId(d) == desc->dev_id;
}

const drvLcdPanelDesc_t gLcdSt7735sDesc = {
    .ops = {
        .probe = prvSt7735sProbe,
        .init = prvSt7735sInit,
        .blit_prepare = prvSt7735sBlitPrepare,
    },
    .name = "ST7735S",
    .dev_id = 0x7c89f0,
    .reset_us = 20 * 1000,
    .init_delay_us = 100 * 1000,
    .width = 160,
    .height = 128,
    .out_fmt = DRV_LCD_OUT_FMT_16BIT_RGB565,
    .dir = DRV_LCD_DIR_EXCHG_XINV,
    .line_mode = DRV_LCD_SPI_4WIRE,
    .fmark_enabled = false,
    .fmark_delay = 0x2a000,
    .freq = 10000000,
    .frame_us = (unsigned)(10000000 / 28.0),
};
