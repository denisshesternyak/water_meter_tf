#pragma once

#include "esp_http_server.h"
#include "esp_log.h"
#include "camera.hpp"

class WebServer {
public:
    explicit WebServer();
    ~WebServer();

    esp_err_t init(Camera* camera_ptr);

private:
    Camera* camera = nullptr;
    httpd_handle_t server_handle = nullptr;

    static void event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data);

    esp_err_t start_http_server();
    esp_err_t wifi_init_station();

    esp_err_t root_get_handler(httpd_req_t* req);
    esp_err_t readings_get_handler(httpd_req_t* req);
    esp_err_t roi_jpg_handler(httpd_req_t* req);
    esp_err_t full_photo_handler(httpd_req_t* req);

    static esp_err_t root_handler_wrapper(httpd_req_t* req) {
        WebServer* self = static_cast<WebServer*>(req->user_ctx);
        return self->root_get_handler(req);
    }
    static esp_err_t readings_handler_wrapper(httpd_req_t* req) {
        WebServer* self = static_cast<WebServer*>(req->user_ctx);
        return self->readings_get_handler(req);
    }
    static esp_err_t roi_handler_wrapper(httpd_req_t* req) {
        WebServer* self = static_cast<WebServer*>(req->user_ctx);
        return self->roi_jpg_handler(req);
    }
    static esp_err_t full_photo_wrapper(httpd_req_t* req) {
        WebServer* self = static_cast<WebServer*>(req->user_ctx);
        return self->full_photo_handler(req);
    }
};
