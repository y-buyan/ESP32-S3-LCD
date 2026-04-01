#ifndef ENCODER_H
#define ENCODER_H

#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

#define Self_EC11_A (GPIO_NUM_37)
#define Self_EC11_B (GPIO_NUM_36)
#define Self_EC11_SW (GPIO_NUM_35)

#define _PCNT_HIGH_LIMIT 100
#define _PCNT_LOW_LIMIT -100

void Encoder_Init(void);

void Encoder_Task(void);

void KEY_Init(void);

#endif // ENCODER_H