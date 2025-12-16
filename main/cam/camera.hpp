#pragma once

#include <string>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_camera.h"
#include "config.h"
#include "sd_card.hpp"

class Camera {
public:
    SemaphoreHandle_t camera_mutex;

    bool init();
    bool take_photo_and_process();
    const char* get_digits() const { return digits; }
    const uint8_t* get_roi() const { return roi_buf; }
    camera_fb_t* get_frame_for_download();
    void return_frame(camera_fb_t* fb);

private:
    SD_card sd_card;
    uint8_t roi_buf[ROI_SIZE_RGB];
    char digits[DIGIT_NUM+1];
    bool camera_initialized = false;
    int image_count = 1;

    static inline uint8_t digit_buf[DIGIT_SIZE];

    void extract_roi_and_recognize(camera_fb_t* fb);
    void extract_roi(camera_fb_t* fb);
    void extract_digit(const int item);
    char recognize_digit();
    static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
};