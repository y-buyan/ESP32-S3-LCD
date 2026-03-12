#include <stdio.h>
#include "RGB.h"

#define _RGB_INDEX_MAX 32

static void Rgb_set_color(uint8_t red_val, uint8_t green_val, uint8_t blue_val);

static uint8_t rgb_data[_RGB_INDEX_MAX][3] = {
    {0, 0, 0},       // Black
    {0, 0, 255},     // Blue
    {0, 128, 0},     // Dark Green
    {0, 128, 128},   // Teal
    {0, 255, 0},     // Green
    {0, 255, 255},   // Cyan
    {32, 178, 170},  // Light Sea Green
    {34, 139, 34},   // Forest Green
    {60, 179, 113},  // Medium Sea Green
    {70, 130, 180},  // Steel Blue
    {75, 0, 130},    // Indigo
    {128, 0, 0},     // Maroon
    {128, 0, 128},   // Purple
    {128, 128, 0},   // Olive
    {128, 128, 128}, // Gray
    {135, 206, 250}, // Light Sky Blue
    {173, 216, 230}, // Light Blue
    {192, 192, 192}, // Silver
    {199, 21, 133},  // Medium Violet Red
    {210, 105, 30},  // Chocolate
    {218, 112, 214}, // Orchid
    {218, 215, 0},   // Gold
    {240, 230, 140}, // Khaki
    {244, 164, 96},  // Sandy Brown
    {255, 0, 0},     // Red
    {255, 0, 255},   // Magenta
    {255, 105, 180}, // Hot Pink
    {255, 165, 0},   // Orange
    {255, 215, 0},   // Gold
    {255, 255, 0},   // Yellow
    {255, 20, 147},  // Deep Pink
    {255, 255, 255}, //
};
static led_strip_handle_t led_strip;

static void Rgb_set_color(uint8_t red_val, uint8_t green_val, uint8_t blue_val)
{
    /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    led_strip_set_pixel(led_strip, 0, red_val, green_val, blue_val);
    /* Refresh the strip to send data */
    led_strip_refresh(led_strip);
}
void Rgb_init(void)
{
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_GPIO_PIN,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

static void _RGB_Handle(void *arg)
{
    static uint8_t index = 0;
    while (1)
    {
        Rgb_set_color(rgb_data[index][0], rgb_data[index][1], rgb_data[index][2]);
        index++;
        if (index >= _RGB_INDEX_MAX)
        {
            index = 0;
        }
        // printf("This RGB Color is R:%d, G:%d, B:%d |index=%d \r\n", rgb_data[index][0], rgb_data[index][1], rgb_data[index][2], index);
        vTaskDelay(1000 / portTICK_PERIOD_MS);    }
}

void RGB_Task(void)
{
    // RGB
    xTaskCreatePinnedToCore(
        _RGB_Handle,
        "RGB Demo",
        4096,
        NULL,
        4,
        NULL,
        0);
}