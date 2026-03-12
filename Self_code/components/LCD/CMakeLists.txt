idf_component_register(SRCS "LCD.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_lcd_gc9a01 lvgl esp_lvgl_port 
                    )
