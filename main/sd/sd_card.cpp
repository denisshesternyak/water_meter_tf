#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "img_converters.h"
#include "sensor.h"
#include "sd_card.hpp"
#include "config.h"

static const char* TAG = "SD";

bool SD_card::init()
{
    ESP_LOGI(TAG, "Initializing SD card in SPI mode (XIAO ESP32S3)");

    esp_err_t ret;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = SD_PIN_NUM_MOSI,
        .miso_io_num     = SD_PIN_NUM_MISO,
        .sclk_io_num      = SD_PIN_NUM_CLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4000,
        .flags           = 0,
        .intr_flags      = 0,
    };

    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = GPIO_NUM_21;
    slot_config.host_id = SPI2_HOST;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 8,
        .allocation_unit_size   = 16 * 1024,
        .disk_status_check_enable = false
    };

    sdmmc_card_t *card = NULL;

    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return false;
    }

    sdmmc_card_print_info(stdout, card);

    struct stat st;
    if (stat(IMAGES_DIR, &st) != 0) {
        ESP_LOGI(TAG, "Creating directory  %s", IMAGES_DIR);
        if (mkdir(IMAGES_DIR, 0755) != 0) {
            ESP_LOGW(TAG, "Failed to create %s", IMAGES_DIR);
        }
    }

    sd_initialized = true;

    ESP_LOGI(TAG, "SD card mounted successfully at /sdcard");
    return true;
}

bool SD_card::save_as_jpeg(uint8_t* buf, int width, int height, const char* filename, int quality) {
    uint8_t* jpg_buf = NULL;
    size_t jpg_len = 0;

    bool ok = fmt2jpg(buf, width * height * 3, width, height, PIXFORMAT_RGB888, quality, &jpg_buf, &jpg_len);

    if (!ok || jpg_len == 0 || !jpg_buf) {
        ESP_LOGE(TAG, "JPEG encoding failed");
        return false;
    }

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "/sdcard%s", filename);
    
    FILE* file = fopen(full_path, "wb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file %s for writing", full_path);
        free(jpg_buf);
        return false;
    }

    size_t written = fwrite(jpg_buf, 1, jpg_len, file);
    fclose(file);
    free(jpg_buf);

    if (written == jpg_len) {
        ESP_LOGI(TAG, "JPEG saved: %s (%d bytes, %dx%d)", 
                full_path, (int)jpg_len, width, height);
        return true;
    } else {
        ESP_LOGE(TAG, "Write error: %d/%d bytes", (int)written, (int)jpg_len);
        return false;
    }
}