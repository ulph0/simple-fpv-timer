// SPDX-License-Identifier: GPL-3.0+


#include <string.h>
#include "esp_err.h"
#include <freertos/FreeRTOS.h>
#include "esp_wifi_types_generic.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_interface.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_netif.h"

#if !CONFIG_IDF_TARGET_LINUX
#include <esp_wifi.h>
#include <esp_system.h>
#endif  // !CONFIG_IDF_TARGET_LINUX
//
#include "config.h"
#include "wifi.h"
#include "timer.h"

static const char* TAG = "wifi";

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{

}

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{

}

void espnow_deinit(wifi_t *wifi)
{
    esp_now_del_peer(wifi->peer.peer_addr);
    ESP_ERROR_CHECK(esp_now_deinit() );
}

void espnow_init(wifi_t *wifi, const config_data_t *cfg)
{
    if (!cfg_has_elrs_uid(cfg))
        return;
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
//    ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
//    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

    wifi->peer.channel = 0;
    wifi->peer.encrypt = false;
    memcpy(wifi->peer.peer_addr, cfg->elrs_uid, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(&wifi->peer) );
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    wifi_t *wifi = (wifi_t*) arg;

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Station started");

    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
        wifi->sta_connected = true;
    }
}

static void wifi_sta_got_ip(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    wifi_t *wifi = (wifi_t*) arg;

    if (wifi->state == WIFI_STA) {
        wifi->sta_connected = true;
    }
}


void wifi_setup(wifi_t *wifi, const config_data_t *cfg)
{
    ESP_LOGI(TAG, "wifi_setup: state: %d", wifi->state);

    if (wifi->state == WIFI_NONE) {

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        esp_wifi_set_mode(WIFI_MODE_NULL);
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            wifi,
                                                            NULL));

        ESP_ERROR_CHECK(
            esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                       wifi_sta_got_ip, wifi));
        esp_wifi_set_storage(WIFI_STORAGE_RAM);

        esp_netif_create_default_wifi_ap();
        esp_netif_create_default_wifi_sta();

    }  else if (wifi->state == WIFI_AP) {
        espnow_deinit(wifi);
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));

    } else if (wifi->state == WIFI_STA) {
        espnow_deinit(wifi);
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));

    } else {
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }

    ESP_LOGI(TAG, "do ELRS");

    if (cfg_has_elrs_uid(cfg)) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        esp_wifi_set_mac(WIFI_IF_STA, cfg->elrs_uid);
    }

    if (cfg->wifi_mode == CFG_WIFI_MODE_AP) {
        wifi->state = WIFI_AP;

        ESP_LOGI(TAG, "configure AP");
        wifi_config_t wifi_config = {
            .ap = {
                .max_connection = 8,
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
        };

        strcpy((char*)wifi_config.ap.ssid, cfg->ssid);
        wifi_config.ap.ssid_len = strlen(cfg->ssid);

        if (strlen(cfg->passphrase) == 0) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        } else {
            strcpy((char*)wifi_config.ap.password, cfg->passphrase);
        }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
                 wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.channel);


    } else {
        millis_t start;

        wifi->sta_connected = false;
        wifi->state = WIFI_STA;

        wifi_config_t wifi_config = {
            .sta = {
                .failure_retry_cnt = 8,
                .scan_method = WIFI_ALL_CHANNEL_SCAN,
                .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
                .threshold.authmode = WIFI_AUTH_OPEN,
                .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            },
        };
        strcpy((char*)wifi_config.sta.ssid, cfg->ssid);
        if (strlen(cfg->passphrase) > 0) {
            strcpy((char*)wifi_config.sta.password, cfg->passphrase);
        }
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        ESP_ERROR_CHECK(esp_wifi_start() );

        start = get_millis();
        while ((get_millis() - start) < 30000 && !wifi->sta_connected ) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI(TAG, "wait for Wifi connection");
        }
    }

    espnow_init(wifi, cfg);
}

