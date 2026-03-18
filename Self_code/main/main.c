/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "RGB.h"
#include "LCD.h"


void Driver_init(void)
{
    Rgb_init();
    LCD_Init();
    // LCD_TOUCH_Init();
    LV_Init();
}


void app_main(void)
{
    Driver_init();

    // RGB_Task();
    LCD_Task();
    while (1)
    {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
