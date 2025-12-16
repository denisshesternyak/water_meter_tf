#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "camera.hpp"
#include "server.hpp"

WebServer server;
static Camera g_camera;

static const char* TAG = "MAIN";

void camera_task(void* pvParameters) {
    while (true) {
        if (xSemaphoreTake(g_camera.camera_mutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
            g_camera.take_photo_and_process();
            xSemaphoreGive(g_camera.camera_mutex);
        } else {
            ESP_LOGW(TAG, "Timeout waiting for camera mutex in EI task");
        }
        vTaskDelay(pdMS_TO_TICKS(UPDATE_MS));
    }
}

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());


    if (!g_camera.init()) {
        ESP_LOGE(TAG, "Camera initialization failed — stopping");
        return;
    }
    ESP_LOGI(TAG, "Camera initialized. Starting capture task...");
        
    server.init(&g_camera);

    xTaskCreate(camera_task, "camera_task", 8192, NULL, 5, nullptr);
    
    ESP_LOGI(TAG, "System started");
}