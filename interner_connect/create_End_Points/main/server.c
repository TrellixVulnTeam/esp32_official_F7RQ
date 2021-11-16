#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_spiffs.h"
#include "dht11.h"

#define TAG "SERVER"
#define LED 2

static esp_err_t on_url_hit(httpd_req_t *req)
{
    ESP_LOGI(TAG, "url %s was hit", req->uri);
    esp_vfs_spiffs_conf_t config = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false};
    esp_vfs_spiffs_register(&config);
    char path[600];
    if (strcmp(req->uri, "/") == 0)
    {
        strcpy(path, "/spiffs/index.html");
    }
    else
    {
        sprintf(path, "/spiffs%s", req->uri);
    }
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        httpd_resp_send_404(req);
    }
    else
    {
        char lineRead[256];
        while (fgets(lineRead, sizeof(lineRead), file))
        {
            httpd_resp_sendstr_chunk(req,lineRead);
        }
        httpd_resp_sendstr_chunk(req, NULL);
    }
    esp_vfs_spiffs_unregister(NULL);
    return ESP_OK;
}


static esp_err_t on_get_temperature(httpd_req_t * req)
{
    ESP_LOGI(TAG, "url %s was hit",req->uri);
    //int temperature = DHT11_read().temperature;
    //char *message[100];
    //sprinf(message, "{"\temperature"\:%f}",temperature);
    char *message = "{\"temperature\":22}";
    httpd_resp_send(req, message, strlen(message));
    return ESP_OK;
}

static esp_err_t on_led_set(httpd_req_t * req)
{
    ESP_LOGI(TAG, "url %s was hit",req->uri);
    char buf[150];
    memset(&buf, 0, strlen(buf));
    httpd_req_recv(req, buf, req->content_len);
    cJSON *payload = cJSON_Parse(buf);
    cJSON *isLedOn = cJSON_GetObjectItem(payload, "isLedOn");
    gpio_set_level(LED, cJSON_IsTrue(isLedOn));
    cJSON_Delete(payload);
    httpd_resp_set_status(req, "240 NO CONTENT");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

void InitializeLed(void)
{
    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
}

void RegisterEndPoints(void)
{
    InitializeLed();
    //DHT11_init();//add gpio num
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "starting sever...");
    if(httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "COULD NOT START SEVER !");

    }

    //read temperature
        httpd_uri_t read_temperature_end_point_config = {
        .uri = "/api/temperature",
        .method = HTTP_GET,
        .handler = on_get_temperature
    };
    httpd_register_uri_handler(server, &read_temperature_end_point_config);
    //control led
        httpd_uri_t led_end_point_config = {
        .uri = "/api/led",
        .method = HTTP_POST,
        .handler = on_led_set
    };
    httpd_register_uri_handler(server, &led_end_point_config);
    //hello word
        httpd_uri_t first_end_point_config = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = on_url_hit
    
    };
    httpd_register_uri_handler(server, &first_end_point_config);
}