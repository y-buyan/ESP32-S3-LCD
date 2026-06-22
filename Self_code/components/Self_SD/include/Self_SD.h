#ifndef SELF_SD_H
#define SELF_SD_H

#include <string.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"

#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** SPI 引脚定义（SPI 模式连接 SD 卡） */
#define PIN_NUM_MISO  36   /*!< SPI MISO */
#define PIN_NUM_MOSI  35   /*!< SPI MOSI */
#define PIN_NUM_CLK   13   /*!< SPI CLK — 请修改为你实际的 CLK 引脚号 */
#define PIN_NUM_CS    37   /*!< SPI CS   */

/** SD 卡挂载点 */
#define MOUNT_POINT "/sdcard"

/**
 * @brief 初始化 SD 卡（SPI 模式），挂载 FAT 文件系统
 * @return ESP_OK 成功，否则失败
 */
esp_err_t sd_card_init(void);

/**
 * @brief 卸载 SD 卡
 * @return ESP_OK 成功，否则失败
 */
esp_err_t sd_card_deinit(void);

/**
 * @brief 从 SD 卡读取文件
 * @param path 文件路径（相对于挂载点）
 * @param buf  读取缓冲区
 * @param len  读取长度
 * @return ESP_OK 成功，否则失败
 */
esp_err_t sd_card_read(const char *path, void *buf, size_t len);

/**
 * @brief 向 SD 卡写入文件
 * @param path 文件路径（相对于挂载点）
 * @param buf  写入数据
 * @param len  写入长度
 * @return ESP_OK 成功，否则失败
 */
esp_err_t sd_card_write(const char *path, const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // SELF_SD_H
