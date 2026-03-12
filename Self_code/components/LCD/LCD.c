#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "demos/lv_demos.h"
#include "lv_demos.h"
#include "LCD.h"

static char *LCD_TAG = "gc9a01";
static char *Lvgl_TAG = "lvgl";
static SemaphoreHandle_t refresh_finish = NULL;

static void LCD_BL_Init(void)
{
    // LCD initialization code here
    gpio_config_t bl_config = {
        .pin_bit_mask = (1ULL << Self_PIN_NUM_LCD_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&bl_config);
    

}

void LCD_BL_SW(uint8_t state)
{
    switch (state)
    {
    case 0:
        gpio_set_level(Self_PIN_NUM_LCD_BL, 0); // 关
        break;
    case 1:
        gpio_set_level(Self_PIN_NUM_LCD_BL, 1); // 开
        break;
    default:
        break;
    }
}

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); /* Circular scroll */
    lv_label_set_text(label, "Hello Espressif.");
    /* Size of the screen (if you use rotation 90 or 270, please set disp->driver->ver_res) */
#if LVGL_VERSION_MAJOR >= 9
    lv_obj_set_width(label, lv_display_get_physical_horizontal_resolution(disp));
#else
    lv_obj_set_width(label, disp->driver->hor_res);
#endif
    lv_obj_align(label, LV_ALIGN_CENTER, 25, 0);
}

/*
    设置 LCD 背景色 , 大小端转换
*/
void lcd_set_color(esp_lcd_panel_handle_t panel_handle, uint16_t color) {
    // 1. 分配一行数据的缓冲区 (使用 uint8_t 以便控制字节序)
    // 大小 = 宽度 * 2 字节
    size_t line_size = Self_LCD_H_RES * 2;
    
    // 优先尝试分配内部 DMA 内存 (速度最快，无需缓存同步)
    // 如果内部内存不足，再回退到普通 malloc
    uint8_t *line_buffer = (uint8_t *)heap_caps_malloc(line_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    
    if (line_buffer == NULL) {
        line_buffer = (uint8_t *)malloc(line_size);
        if (line_buffer == NULL) {
            return;
        }
    }

    // 2. 【核心修复】手动填充数据，强制转换为 Big-Endian (高位在前)
    // RGB565: 高字节 (R5 + G3), 低字节 (G3 + B5)
    uint8_t high_byte = (color >> 8)&0xFF; // 取高字节
    uint8_t low_byte  = color & 0xFF;

    // 使用 memset 快速填充 (比循环快得多)
    // 我们需要交替填充 [high, low, high, low...]
    // 由于 memset 只能填同一个值，这里还是需要用循环，但可以优化
    for (int i = 0; i < Self_LCD_H_RES; i++) {
        line_buffer[i * 2]     = high_byte;
        line_buffer[i * 2 + 1] = low_byte;
    }
    // 3. 循环绘制每一行
    // 注意：如果屏幕驱动支持 "fill_rect" 或 "draw_bitmap" 带重复计数，效率会更高
    // 但为了兼容性，我们保持逐行绘制逻辑
    for (int y = 0; y < Self_LCD_V_RES; y++) {
        esp_err_t ret = esp_lcd_panel_draw_bitmap(
            panel_handle,
            0, y,                   // x1, y1
            Self_LCD_H_RES, y + 1,  // x2, y2 (exclusive)
            line_buffer             // 数据指针 (已确保字节序正确)
        );
        
        if (ret != ESP_OK) {
            break;
        }
    }

    // 4. 释放内存
    free(line_buffer);
}

void fill_screen_lvgl(lv_color_t color)
{
    // 创建一个覆盖整个屏幕的对象
    lv_obj_t *bg = lv_obj_create(lv_scr_act());

    // 移除默认样式（边框、阴影等），使其成为纯色块
    lv_obj_remove_style_all(bg);

    // 设置大小为屏幕大小
    lv_obj_set_size(bg, LV_PCT(100), LV_PCT(100));

    // 设置背景颜色
    lv_obj_set_style_bg_color(bg, color, 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);

    // 将其置于最底层（可选，防止遮挡其他后续创建的控件）
    lv_obj_move_background(bg);

    // 如果需要立即刷新看到效果（通常不需要，LVGL 会在空闲时刷新）
    // lv_refr_now(NULL);
}


void LCD_Init(void)
{
    // LCD initialization code here
    LCD_BL_Init();
    /* SPI BUS Init */
    ESP_LOGI(LCD_TAG, "Initialize SPI bus");
    const spi_bus_config_t lcd_buscfg = GC9A01_PANEL_BUS_SPI_CONFIG(Self_PIN_NUM_LCD_PCLK, Self_PIN_NUM_LCD_DATA0,
                                                                    Self_LCD_H_RES * 80 * Self_LCD_BIT_PER_PIXEL / 8);
    spi_bus_initialize(Self_LCD_HOST, &lcd_buscfg, SPI_DMA_CH_AUTO);

    /* LCD panel IO */
    ESP_LOGI(LCD_TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = GC9A01_PANEL_IO_SPI_CONFIG(Self_PIN_NUM_LCD_CS, Self_PIN_NUM_LCD_DC,
                                                                               NULL, NULL);
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)Self_LCD_HOST, &io_config, &io_handle);

    /* LCD panel driver */
    ESP_LOGI(LCD_TAG, "Install gc9a01 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = Self_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = Self_LCD_BIT_PER_PIXEL,
    };

    esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);
    esp_lcd_panel_mirror(panel_handle, true, false);
    esp_lcd_panel_disp_on_off(panel_handle, true);
    LCD_BL_SW(1);


    ESP_LOGI(Lvgl_TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = Self_LCD_H_RES * 20,   // LVGL缓存大小
        .double_buffer = false, // 是否开启双缓存
        .hres = Self_LCD_H_RES, // 液晶屏的宽
        .vres = Self_LCD_V_RES, // 液晶屏的高
        .monochrome = false,  // 是否单色显示器
        /* Rotation的值必须和液晶屏初始化里面设置的 翻转 和 镜像 一样 */
        .rotation = {
            .swap_xy = false,  // 是否翻转
            .mirror_x = true, // x方向是否镜像
            .mirror_y = false, // y方向是否镜像
        },
        .flags = {
            .buff_dma = false,  // 是否使用DMA 注意：dma与spiram不能同时为true
            .buff_spiram = false, // 是否使用PSRAM 注意：dma与spiram不能同时为true
        }
   };

    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
    LCD_BL_SW(1);
     ESP_LOGI(Lvgl_TAG, "Display LVGL Scroll Text");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(0)) {
        /* Rotation of the screen */
    lv_example_get_started_1();
    lvgl_port_unlock();
    }
}

static void _LCD_Handle(void *pvParameters)
{
    while (1)
    {
    }
}

void LCD_Task(void)
{
    // RGB
    xTaskCreatePinnedToCore(
        _LCD_Handle,
        "LCD Demo",
        4096,
        NULL,
        5,
        NULL,
        0);
}