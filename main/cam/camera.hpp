#pragma once

#include <string>
#include <stdint.h>
#include "esp_camera.h"
#include "config.h"
#include "sd_card.hpp"

class Camera {
public:
    bool init();
    bool take_photo_and_process();

private:
    SD_card sd_card;
    uint8_t roi_buf[ROI_SIZE];
    static inline uint8_t digit_buf[DIGIT_SIZE];

    bool camera_initialized = false;
    int image_count = 1;

    void extract_roi_and_recognize(camera_fb_t* fb);
    void extract_roi(camera_fb_t* fb);
    void extract_digit(const int item);
    char recognize_digit();
    static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
};