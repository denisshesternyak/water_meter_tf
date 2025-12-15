#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "camera.hpp"

static Camera g_camera;

static const char* TAG = "MAIN";

void camera_task(void* pvParameters) {
    while (true) {
        g_camera.take_photo_and_process();
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

    if (!g_camera.init()) {
        ESP_LOGE(TAG, "Camera initialization failed — stopping");
        return;
    }

    ESP_LOGI(TAG, "Camera initialized. Starting capture task...");

    xTaskCreate(camera_task, "camera_task", 8192, NULL, 5, nullptr);
    
    ESP_LOGI(TAG, "System started");
}