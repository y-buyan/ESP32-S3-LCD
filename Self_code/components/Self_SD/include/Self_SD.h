#ifndef SELF_SD_H
#define SELF_SD_H

#include "esp_err.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif


#define PIN_NUM_MISO   36
#define PIN_NUM_MOSI  35
#define PIN_NUM_CLK   
#define PIN_NUM_CS     37

esp_err_t sd_card_init(void);
esp_err_t sd_card_read(void);
esp_err_t sd_card_write(void);

#endif // SELF_SD_H
