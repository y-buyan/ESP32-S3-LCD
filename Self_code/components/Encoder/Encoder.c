#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "Encoder.h"


static const char *EC11_TAG = "Encoder";
static const char *KEY_TAG  = "Key";

int pulse_count = 0;
int event_count = 0;

void Encoder_Init(void)
{
    // Initialization code for the encoder
    ESP_LOGI(EC11_TAG, "Encoder initialized");
    pcnt_unit_config_t uint_config = {
            .high_limit = _PCNT_HIGH_LIMIT,
            .low_limit = _PCNT_LOW_LIMIT,
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    pcnt_new_unit(&uint_config, &pcnt_unit);

    ESP_LOGI(EC11_TAG, "set glitch filter");
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = Self_EC11_A,
        .level_gpio_num = Self_EC11_B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a);
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = Self_EC11_B,
        .level_gpio_num = Self_EC11_A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b);

    ESP_LOGI(EC11_TAG, "set edge and level actions for pcnt channels");
    pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    ESP_LOGI(EC11_TAG, "enable pcnt unit");
    pcnt_unit_enable(pcnt_unit);
    ESP_LOGI(EC11_TAG, "clear pcnt unit");
    pcnt_unit_clear_count(pcnt_unit);
    ESP_LOGI(EC11_TAG, "start pcnt unit");
    pcnt_unit_start(pcnt_unit);

}

void Encoder_Task(void)
{
    // Task code for handling encoder events
    
}

void KEY_Init(void)
{
    // Initialization code for the key
}

void KEY_Task(void)
{
    // Task code for handling key events
}
