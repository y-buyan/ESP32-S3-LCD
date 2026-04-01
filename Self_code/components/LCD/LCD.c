#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_check.h"
#include "demos/lv_demos.h"
#include "lv_demos.h"
#include "LCD.h"

static char *LCD_TAG = "gc9a01";
static char *Lvgl_TAG = "lvgl";
static char *Touch_TAG = "touch";

LV_IMAGE_DECLARE(su);

static SemaphoreHandle_t refresh_finish = NULL;

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;

i2c_master_bus_handle_t i2c_handle = NULL;

static bool _notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}


static void LCD_BL_Init(void)
{
    const gpio_config_t LCD_BL_gpio_config = {
        .pin_bit_mask = (1ULL << Self_PIN_NUM_LCD_BL), // 同时配置 LCD_BL 和触摸屏复位引脚
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};

    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 8000,
        .clk_cfg = LEDC_AUTO_CLK};

    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = Self_PIN_NUM_LCD_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags.output_invert = true};
    gpio_config(&LCD_BL_gpio_config);
    ledc_timer_config(&LCD_backlight_timer);
    ledc_channel_config(&LCD_backlight_channel);
    ledc_fade_func_install(0);
}

void LCD_BL_SET(uint8_t bl_duty)
{
    if (bl_duty <= 0)
    {
        bl_duty = 0;
    }
    else if (bl_duty >= 100)
    {
        bl_duty = 100;
    }
    uint32_t duty_cycle = (bl_duty * 8191) / 100;
    ESP_LOGI(LCD_TAG, "Setting backlight duty: %d%% -> %d (raw)", bl_duty, duty_cycle);

    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8191 - duty_cycle, 500);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}

/*
    设置 LCD 背景色 , 大小端转换
*/
void lcd_set_color(esp_lcd_panel_handle_t panel_handle, uint16_t color)
{
    // 1. 分配一行数据的缓冲区 (使用 uint8_t 以便控制字节序)
    // 大小 = 宽度 * 2 字节
    size_t line_size = Self_LCD_H_RES * 2;

    // 优先尝试分配内部 DMA 内存 (速度最快，无需缓存同步)
    // 如果内部内存不足，再回退到普通 malloc
    uint8_t *line_buffer = (uint8_t *)heap_caps_malloc(line_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

    if (line_buffer == NULL)
    {
        line_buffer = (uint8_t *)malloc(line_size);
        if (line_buffer == NULL)
        {
            return;
        }
    }

    // 2. 【核心修复】手动填充数据，强制转换为 Big-Endian (高位在前)
    // RGB565: 高字节 (R5 + G3), 低字节 (G3 + B5)
    uint8_t high_byte = (color >> 8) & 0xFF; // 取高字节
    uint8_t low_byte = color & 0xFF;

    // 使用 memset 快速填充 (比循环快得多)
    // 我们需要交替填充 [high, low, high, low...]
    // 由于 memset 只能填同一个值，这里还是需要用循环，但可以优化
    for (int i = 0; i < Self_LCD_H_RES; i++)
    {
        line_buffer[i * 2] = high_byte;
        line_buffer[i * 2 + 1] = low_byte;
    }
    // 3. 循环绘制每一行
    // 注意：如果屏幕驱动支持 "fill_rect" 或 "draw_bitmap" 带重复计数，效率会更高
    // 但为了兼容性，我们保持逐行绘制逻辑
    for (int y = 0; y < Self_LCD_V_RES; y++)
    {
        esp_err_t ret = esp_lcd_panel_draw_bitmap(
            panel_handle,
            0, y,                  // x1, y1
            Self_LCD_H_RES, y + 1, // x2, y2 (exclusive)
            line_buffer            // 数据指针 (已确保字节序正确)
        );

        if (ret != ESP_OK)
        {
            break;
        }
    }

    // 4. 释放内存
    free(line_buffer);
}

/*
    LVGL 填充整个屏幕
*/
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

// 这里的参数 image 应该是你转换后的图片描述符，类型通常是 const lv_image_dsc_t *
void lvgl_show_image(const lv_image_dsc_t *image)
{
    // 1. 创建一个图像对象（在当前活动屏幕上）
    lv_obj_t *img_obj = lv_image_create(lv_screen_active());

    // 2. 设置图像源
    // 在 v9 中，推荐使用 lv_image_set_src，虽然旧的 lv_img_set_src 可能还能用
    lv_image_set_src(img_obj, image);

    // 3. 居中显示（注意：参数必须是 img_obj，即你刚刚创建的对象）
    lv_obj_center(img_obj);

    // 4. (可选) 如果你想让图片自动调整大小以适应对象
    // lv_obj_set_size(img_obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
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
    const esp_lcd_panel_io_spi_config_t io_config = GC9A01_PANEL_IO_SPI_CONFIG(Self_PIN_NUM_LCD_CS, Self_PIN_NUM_LCD_DC,
                                                                               NULL, NULL);
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)Self_LCD_HOST, &io_config, &io_handle);

    /* LCD panel driver */
    ESP_LOGI(LCD_TAG, "Install gc9a01 panel driver");
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

}

void LV_Init(void)
{
    ESP_LOGI(Lvgl_TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();

    lv_init();

    lvgl_port_init(&lvgl_cfg);

    /*Add rendering buffers to the screen.
     *Here adding a smaller partial buffer assuming 16-bit (RGB565 color format)*/

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = Self_LCD_H_RES * 40, // LVGL缓存大小
        .double_buffer = true,             // 是否开启双缓存
        .hres = Self_LCD_H_RES,             // 液晶屏的宽
        .vres = Self_LCD_V_RES,             // 液晶屏的高
        .monochrome = false,                // 是否单色显示器
        /* Rotation的值必须和液晶屏初始化里面设置的 翻转 和 镜像 一样 */
        .rotation = {
            .swap_xy = false,  // 是否翻转
            .mirror_x = true,  // x方向是否镜像
            .mirror_y = false, // y方向是否镜像
        },
        .flags = {
            .buff_dma = false,    // 是否使用DMA 注意：dma与spiram不能同时为true
            .buff_spiram = false, // 是否使用PSRAM 注意：dma与spiram不能同时为true
        }
    };

    static uint8_t buf[Self_LCD_H_RES * Self_LCD_V_RES / 10 * 2]; /* x2 because of 16-bit color depth */

    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);

    
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);

}

static void _LCD_Handle(void *pvParameters)
{
if (lvgl_port_lock(0)) {
    // 这里创建你的 UI 元素
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lvgl_port_unlock();
    }
}

void  LCD_Task(void)
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