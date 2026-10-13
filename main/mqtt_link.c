#include "mqtt_link.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "mqtt_link";

static esp_mqtt_client_handle_t s_client;
static mqtt_cmd_fn s_on_cmd;
static char s_topic_status[48];
static char s_topic_cmd[48];
static int s_started;

static void mqtt_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    esp_mqtt_event_handle_t ev = data;
    if (id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "connected");
        esp_mqtt_client_subscribe(s_client, s_topic_cmd, 0);
    } else if (id == MQTT_EVENT_DATA && s_on_cmd && ev->data && ev->data_len > 0) {
        s_on_cmd(ev->data, ev->data_len);
    } else if (id == MQTT_EVENT_ERROR) {
        ESP_LOGW(TAG, "broker error");
    }
}

void mqtt_link_start(const link_settings_t *settings, const char *node_id, mqtt_cmd_fn on_cmd)
{
    if (!link_mqtt_should_start(settings)) {
        if (s_started && s_client) {
            esp_mqtt_client_handle_t client = s_client;
            s_started = 0;
            s_client = NULL;
            s_on_cmd = NULL;
            esp_mqtt_client_stop(client);
            esp_mqtt_client_destroy(client);
            ESP_LOGI(TAG, "MQTT stopped");
        } else {
            ESP_LOGI(TAG, "MQTT present, left disabled");
        }
        return;
    }
    s_on_cmd = on_cmd;
    if (s_started) {
        return;
    }
    snprintf(s_topic_status, sizeof(s_topic_status), "dryad/%s/status", node_id);
    snprintf(s_topic_cmd, sizeof(s_topic_cmd), "dryad/%s/cmd", node_id);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = settings->broker,
    };
    s_client = esp_mqtt_client_init(&cfg);
    if (!s_client) {
        ESP_LOGE(TAG, "client init failed");
        return;
    }
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event, NULL);
    esp_mqtt_client_start(s_client);
    s_started = 1;
    ESP_LOGI(TAG, "connecting to %s", settings->broker);
}

void mqtt_link_publish_status(const char *json)
{
    if (!s_started || !s_client || !json) {
        return;
    }
    esp_mqtt_client_publish(s_client, s_topic_status, json, 0, 0, 0);
}
