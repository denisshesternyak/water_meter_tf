#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "server.hpp"
#include "config.h"
#include "server_html.h"

static const char* TAG = "WEBSERVER";

WebServer::WebServer() = default;

WebServer::~WebServer() {
    if (server_handle) {
        httpd_stop(server_handle);
    }
}

esp_err_t WebServer::init(Camera* camera_ptr) {
    camera = camera_ptr;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(wifi_init_station());
    ESP_ERROR_CHECK(start_http_server());

    return ESP_OK;
}

esp_err_t WebServer::wifi_init_station() {
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        nullptr,
        nullptr));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        nullptr,
        nullptr));

    wifi_config_t wifi_config = {};
    memcpy(wifi_config.sta.ssid, ESP_WIFI_SSID, strlen(ESP_WIFI_SSID));
    memcpy(wifi_config.sta.password, ESP_WIFI_PASS, strlen(ESP_WIFI_PASS));

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to Wi-Fi network \"%s\"...", ESP_WIFI_SSID);

    return ESP_OK;
}

esp_err_t WebServer::start_http_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.lru_purge_enable = true;

    httpd_uri_t root = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = root_handler_wrapper,
        .user_ctx = this
    };

    httpd_uri_t readings = {
        .uri      = "/readings",
        .method   = HTTP_GET,
        .handler  = readings_handler_wrapper,
        .user_ctx = this
    };

    httpd_uri_t roi_jpg = {
        .uri      = "/roi.jpg",
        .method   = HTTP_GET,
        .handler  = roi_handler_wrapper,
        .user_ctx = this
    };

    httpd_uri_t photo_download = {
        .uri      = "/download.jpg",
        .method   = HTTP_GET,
        .handler  = full_photo_wrapper,
        .user_ctx = this
    };

    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    if (httpd_start(&server_handle, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }

    httpd_register_uri_handler(server_handle, &root);
    httpd_register_uri_handler(server_handle, &readings);
    httpd_register_uri_handler(server_handle, &roi_jpg);
    httpd_register_uri_handler(server_handle, &photo_download);

    return ESP_OK;
}

esp_err_t WebServer::root_get_handler(httpd_req_t* req) {
    char* html_buffer = (char*)malloc(5000);
    if (!html_buffer) {
        ESP_LOGE(TAG, "Failed to allocate HTML buffer");
        return httpd_resp_send_500(req);
    }

    int written = snprintf(html_buffer, 5000, HTML_TEMPLATE, UPDATE_MS);

    if (written < 0 || written >= 5000) {
        free(html_buffer);
        return httpd_resp_send_500(req);
    }

    esp_err_t res = httpd_resp_send(req, html_buffer, written);
    free(html_buffer);
    return res;
}

void WebServer::event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi-Fi connection lost. Reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected! Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t WebServer::readings_get_handler(httpd_req_t* req) {
    if (!camera) {
        const char* err = "{\"error\":\"no camera\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, err, strlen(err));
        return ESP_FAIL;
    }

    const char* digits = camera->get_digits();
    if (!digits) digits = "-----";

    char json[64];
    snprintf(json, sizeof(json), "{\"digits\":\"%s\"}", digits);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json, strlen(json));

    return ESP_OK;
}

esp_err_t WebServer::roi_jpg_handler(httpd_req_t* req) {
    if (!camera) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    const uint8_t* rgb_buf = camera->get_roi();
    if (!rgb_buf) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    size_t jpg_buf_len = 0;
    uint8_t* jpg_buf = nullptr;

    bool converted = fmt2jpg(const_cast<uint8_t*>(rgb_buf), ROI_SIZE_RGB,
                             ROI_W, ROI_H,
                             PIXFORMAT_RGB888, 12, &jpg_buf, &jpg_buf_len);

    if (!converted || jpg_buf_len == 0) {
        ESP_LOGE(TAG, "JPEG encoding failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");

    esp_err_t res = httpd_resp_send(req, (const char*)jpg_buf, jpg_buf_len);

    free(jpg_buf);

    return res;
}

esp_err_t WebServer::full_photo_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "Start /download.jpg");
    if (!camera) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    camera_fb_t* fb = camera->get_frame_for_download();
    if (!fb) {
        return httpd_resp_send_500(req);
    }

    uint64_t timestamp_ms = esp_timer_get_time();
    char filename[32];
    snprintf(filename, sizeof(filename), "water_meter_%llu.jpg", timestamp_ms);

    httpd_resp_set_type(req, "image/jpeg");
    char disposition[64];
    snprintf(disposition, sizeof(disposition), "attachment; filename=\"%s\"", filename);
    httpd_resp_set_hdr(req, "Content-Disposition", disposition);
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

    esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
    camera->return_frame(fb);
    return res;
}