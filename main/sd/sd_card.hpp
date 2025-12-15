#pragma once

#include <stdint.h>

#define SD_PIN_NUM_MISO     GPIO_NUM_8
#define SD_PIN_NUM_MOSI     GPIO_NUM_9
#define SD_PIN_NUM_CLK      GPIO_NUM_7
#define SD_PIN_NUM_CS       GPIO_NUM_21

#define IMAGES_DIR      "/sdcard/images"
#define ROI_PATH        "/images/roi_%d.jpg"
#define DIGIT_PATH      "/images/digit%d_%d.jpg"

class SD_card {
public:
    bool init(void);
    bool save_as_jpeg(uint8_t* buf, int width, int height, const char* filename, int quality);
    bool isSDInitialized() { return sd_initialized; }

private:
    bool sd_initialized = false;
};
