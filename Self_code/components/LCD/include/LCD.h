#ifndef LCD_H
#define LCD_H

#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_gc9a01.h"
#include "driver/ledc.h"
#include "lvgl.h"
#include "lv_demos.h"
#include "lv_conf.h"
#include "lv_examples.h"
#include "esp_lvgl_port.h"
#include "driver/i2c_master.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_cst816s.h"
#include "../lvgl_private.h"



#define Self_LCD_HOST SPI2_HOST
#define Self_LCD_H_RES (240)
#define Self_LCD_V_RES (240)
#define Self_LCD_BIT_PER_PIXEL (16)

#define Self_PIN_NUM_LCD_CS (GPIO_NUM_2)
#define Self_PIN_NUM_LCD_PCLK (GPIO_NUM_3)
#define Self_PIN_NUM_LCD_DATA0 (GPIO_NUM_10)
#define Self_PIN_NUM_LCD_RST (GPIO_NUM_21)
#define Self_PIN_NUM_LCD_DC (GPIO_NUM_15)
#define Self_PIN_NUM_LCD_BL (GPIO_NUM_42)

#define Self_DELAY_TIME_MS (3000)                                    

#define Self_lvgl_tick_ms (2)
#define Self_LVGL_TASK_MAX_DELAY_MS (500)
#define Self_LVGL_TASK_MIN_DELAY_MS (1)
#define Self_LVGL_TASK_STACK_SIZE (5 * 1024)
#define Self_LVGL_TASK_PRIORITY (2)

#define Self_TOUCH_I2C_NUM (0)
#define Self_TOUCH_I2C_SCL (GPIO_NUM_18)
#define Self_TOUCH_I2C_SDA (GPIO_NUM_8)
#define Self_TOUCH_GPIO_INT (GPIO_NUM_4)
#define Self_TOUCH_I2C_CLK_HZ (100000)

typedef struct
{
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    uint8_t fb_bits_per_pixel;
    uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_val; // save current value of LCD_CMD_COLMOD register
    const gc9a01_lcd_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
} gc9a01_panel_t;

void LCD_Init(void);
void LV_Init(void);
void LCD_Task(void);
void LCD_TOUCH_Init(void);
void LCD_BL_SET(uint8_t bl_duty);

#endif // LCD_H