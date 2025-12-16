#include "camera.hpp"
#include "esp_log.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

static const char* TAG = "CAMERA";

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

bool Camera::init() {
    camera_config_t config = {
        .pin_pwdn = PWDN_GPIO_NUM,
        .pin_reset = RESET_GPIO_NUM,
        .pin_xclk = XCLK_GPIO_NUM,
        .pin_sscb_sda = SIOD_GPIO_NUM,
        .pin_sscb_scl = SIOC_GPIO_NUM,
        .pin_d7 = Y9_GPIO_NUM,
        .pin_d6 = Y8_GPIO_NUM,
        .pin_d5 = Y7_GPIO_NUM,
        .pin_d4 = Y6_GPIO_NUM,
        .pin_d3 = Y5_GPIO_NUM,
        .pin_d2 = Y4_GPIO_NUM,
        .pin_d1 = Y3_GPIO_NUM,
        .pin_d0 = Y2_GPIO_NUM,
        .pin_vsync = VSYNC_GPIO_NUM,
        .pin_href = HREF_GPIO_NUM,
        .pin_pclk = PCLK_GPIO_NUM,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_QVGA,
        .jpeg_quality = 12,
        .fb_count = 1,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: 0x%x", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_vflip(s, 1);
    }

    camera_mutex = xSemaphoreCreateMutex();
    if (camera_mutex == NULL) {
        ESP_LOGE("CAM", "Failed to create camera mutex");
        return false;
    }

    camera_initialized = true;
    ESP_LOGI(TAG, "Camera initialized successfully");

    if (sd_card.init()) {
        ESP_LOGW(TAG, "SD card initialization failed, continuing without SD card");
    }

    return true;
}

int Camera::ei_camera_get_data(size_t offset, size_t length, float *out_ptr)
{
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        uint8_t r = digit_buf[pixel_ix + 2];
        uint8_t g = digit_buf[pixel_ix + 1];
        uint8_t b = digit_buf[pixel_ix];
        
        out_ptr[out_ptr_ix] = (r << 16) + (g << 8) + b;

        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    return 0;
}

char Camera::recognize_digit() {
    ei::signal_t signal;
    signal.total_length = DIGIT_W * DIGIT_H;
    signal.get_data = &ei_camera_get_data;

    char best_digit = DIGIT_EMPTY;

    ei_impulse_result_t result = {0};
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    if (res != EI_IMPULSE_OK) {
        ESP_LOGI(TAG, "ERR: run_classifier (%d)\n", res);
        return DIGIT_EMPTY;
    }

    float best_score = THRESHOLD_VAL;

    for (size_t i = 0; i < result.bounding_boxes_count; i++) {
        auto bb = result.bounding_boxes[i];
        if (bb.value > best_score) {
            best_score = bb.value;
            best_digit = bb.label[strlen(bb.label) - 1];
        }
    }

    /*if (best_digit != DIGIT_EMPTY) {
        ESP_LOGI(TAG, " -> %c (%.2f)\n", best_digit, best_score);
    } else {
        ESP_LOGI(TAG, " -> not detected");
    }*/

    return best_digit;
}

bool Camera::take_photo_and_process() {
    if (!camera_initialized) return false;

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Capture failed");
        return false;
    }

    extract_roi(fb);

    for(int i = 0; i < DIGIT_NUM; i++) {
        extract_digit(i);
        digits[i] = recognize_digit();
    }

    digits[DIGIT_NUM] = '\0';

    ESP_LOGI(TAG, "WATER METER READING: [%s]", digits);

    esp_camera_fb_return(fb);
    image_count++;
    return true;
}

void Camera::extract_digit(const int item) {
    int start_x = item * DIGIT_W;

    for (int y = 0; y < DIGIT_H; y++) {
        if (y >= ROI_H) break;

        size_t src_idx = (y * ROI_W + start_x) * 3;
        size_t dst_idx = (y * DIGIT_W) * 3;
        
        memcpy(digit_buf + dst_idx, roi_buf + src_idx, DIGIT_W * 3);
    }

    if (sd_card.isSDInitialized()) {
        char name[64];
        snprintf(name, sizeof(name), DIGIT_PATH, item, image_count);
        sd_card.save_as_jpeg(digit_buf, DIGIT_W, DIGIT_H, name, 80);
    }

    //ESP_LOGI(TAG, "Digit %d: ", item);
}

void Camera::extract_roi(camera_fb_t* fb) {
    size_t rgb888_size = fb->width * fb->height * 3;
    uint8_t* rgb888_buf = (uint8_t*)malloc(rgb888_size);
    if (!rgb888_buf) {
        ESP_LOGE(TAG, "Not enough RAM for full RGB888 decode!");
        return;
    }

    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb888_buf);
    if (!converted) {
        ESP_LOGE(TAG, "JPEG decode failed");
        free(rgb888_buf);
        return;
    }

    for (int y = 0; y < ROI_H; y++) {
        int src_y = ROI_Y + y;
        if (src_y >= fb->height) break;

        for (int x = 0; x < ROI_W; x++) {
            int src_x = ROI_X + x;
            if (src_x >= fb->width) break;

            size_t src_idx = (src_y * fb->width + src_x) * 3;
            size_t dst_idx = (y * ROI_W + x) * 3;

            roi_buf[dst_idx + 0] = rgb888_buf[src_idx + 0];  // R
            roi_buf[dst_idx + 1] = rgb888_buf[src_idx + 1];  // G
            roi_buf[dst_idx + 2] = rgb888_buf[src_idx + 2];  // B
        }
    }

    if(sd_card.isSDInitialized()) {
        char name[64];
        snprintf(name, sizeof(name), ROI_PATH, image_count);
        sd_card.save_as_jpeg(roi_buf, ROI_W, ROI_H, name, 80);
    }

    free(rgb888_buf);
}

camera_fb_t* Camera::get_frame_for_download() {
    if (xSemaphoreTake(camera_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGW("CAM", "Timeout waiting for camera in download handler");
        return nullptr;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE("CAM", "Capture failed in download handler");
        xSemaphoreGive(camera_mutex);
        return nullptr;
    }

    return fb;
}

void Camera::return_frame(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
    xSemaphoreGive(camera_mutex);
}