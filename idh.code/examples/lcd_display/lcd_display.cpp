/* Copyright (C) 2019 RDA Technologies Limited and/or its affiliates("RDA").
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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('L', 'C', 'D', 'D')

#include <string.h>
#include <stdlib.h>
#include "osi_log.h"
#include "osi_api.h"
#include "osi_compiler.h"
#include "hal_chip.h"
#include "drv_lcd.h"

#define lcde_assert(b)  \
    do                  \
    {                   \
        if (!(b))       \
            osiPanic(); \
    } while (0)

typedef struct
{
    lcdSpec_t info;
    uint16_t *buffer;
} lcdExample_t;

static void prvFillVerticalColorBarRgb565(uint16_t *buffer, unsigned width, unsigned height)
{
    for (unsigned h = 0; h < height; ++h)
    {
        for (unsigned w = 0; w < width; ++w)
        {
            if (w < width / 3)
                buffer[h * width + w] = 0xf800;
            else if (w < width / 3 * 2)
                buffer[h * width + w] = 0x07e0;
            else
                buffer[h * width + w] = 0x001f;
        }
    }
}

static void prvFillHorizontalColorBarRgb565(uint16_t *buffer, unsigned width, unsigned height)
{
    for (unsigned h = 0; h < height; ++h)
    {
        for (unsigned w = 0; w < width; ++w)
        {
            if (h < height / 3)
                buffer[h * width + w] = 0xf800;
            else if (h < height / 3 * 2)
                buffer[h * width + w] = 0x07e0;
            else
                buffer[h * width + w] = 0x001f;
        }
    }
}

static inline void prvFillBufferWhiteScreen(void *buffer, unsigned width, unsigned height)
{
    memset(buffer, 0xff, width * height * sizeof(uint16_t));
}

static void prvLcdClear(lcdExample_t *lcd)
{
    prvFillBufferWhiteScreen(lcd->buffer, lcd->info.width, lcd->info.height);
    lcdFrameBuffer_t fb = {};
    fb.buffer = lcd->buffer;
    fb.colorFormat = LCD_RESOLUTION_RGB565;
    fb.height = lcd->info.height;
    fb.width = lcd->info.width;
    fb.widthOriginal = lcd->info.width;

    lcdDisplay_t rec = {};
    rec.width = lcd->info.width;
    rec.height = lcd->info.height;
    drvLcdBlockTransfer(&fb, &rec);
}

static void prvLcdDisplayColorBar(lcdExample_t *lcd, uint16_t width, uint16_t height,
                                  uint16_t x = 0, uint16_t y = 0, bool vertical = true)
{
    if (width > lcd->info.width)
        width = lcd->info.width;
    if (height > lcd->info.height)
        height = lcd->info.width;

    if (vertical)
        prvFillVerticalColorBarRgb565(lcd->buffer, width, height);
    else
        prvFillHorizontalColorBarRgb565(lcd->buffer, width, height);

    lcdDisplay_t rec = {};
    rec.x = x;
    rec.y = y;
    rec.width = width;
    rec.height = height;

    lcdFrameBuffer_t fb = {};
    fb.buffer = lcd->buffer;
    fb.colorFormat = LCD_RESOLUTION_RGB565;
    fb.height = height;
    fb.width = width;
    fb.widthOriginal = width;
    drvLcdBlockTransfer(&fb, &rec);
}

static void prvLcdInit(lcdExample_t *lcd)
{
    halPmuSwitchPower(HAL_POWER_VDD28, true, true);
    halPmuSwitchPower(HAL_POWER_KEYLED, true, true);
    drvLcdInit();
    drvLcdGetLcdInfo(&lcd->info);
    OSI_LOGI(0, "LCD %u/%u", lcd->info.width, lcd->info.height);
    lcd->buffer = (uint16_t *)malloc(lcd->info.width * lcd->info.height * sizeof(uint16_t));
    lcde_assert(lcd->buffer != NULL);
    prvLcdClear(lcd);
    halPmuSwitchPower(HAL_POWER_BACK_LIGHT, true, true);
}

static void prvLcdThread(void *param)
{
    lcdExample_t lcd;
    prvLcdInit(&lcd);

    using testcase_t = void (*)(lcdExample_t *);
    auto vertical_cb = [](lcdExample_t *lcd) { prvLcdDisplayColorBar(lcd, lcd->info.width, lcd->info.height); };
    auto small_vertical_cb = [](lcdExample_t *lcd) { prvLcdDisplayColorBar(lcd, 120, 120, 100, 120); };
    auto horizontal_cb = [](lcdExample_t *lcd) { prvLcdDisplayColorBar(lcd, 128, 128, 40, 160, false); };
    auto small_horizontal_cb = [](lcdExample_t *lcd) { prvLcdDisplayColorBar(lcd, lcd->info.width, lcd->info.height, 0, 0, false); };

    testcase_t testlist[] = {
        vertical_cb,
        small_vertical_cb,
        horizontal_cb,
        small_horizontal_cb,
    };

    for (unsigned i = 0;; ++i)
    {
        prvLcdClear(&lcd);
        testlist[i % OSI_ARRAY_SIZE(testlist)](&lcd);
        osiThreadSleep(3000);
    }

    osiThreadExit();
}

OSI_EXTERN_C int appimg_enter(void *param)
{
    OSI_LOGI(0, "Lcd example entry");

    osiThreadCreate("lcd-example", prvLcdThread, NULL, OSI_PRIORITY_LOW, 1024, 1);
    return 0;
}

OSI_EXTERN_C void appimg_exit(void *param)
{
    OSI_LOGI(0, "Lcd example exit");
}