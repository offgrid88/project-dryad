#include "mesh_net.h"

#include "link_logic.h"
#include "mqtt_link.h"
#include "pump.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_mesh.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "mesh_net";

#define DRYAD_MESH_PASS "dryad-mesh"
#define DRYAD_AP_IP "192.168.4.1"

static const uint8_t MESH_ID[6] = {'D', 'R', 'Y', 'A', 'D', '1'};

static link_settings_t s_cfg;
static link_mesh_t s_mesh;
static link_status_t s_self;
static SemaphoreHandle_t s_lock;
static esp_netif_t *s_ap;
static volatile int s_mesh_ready;
static volatile int s_mesh_up;
static int s_seq;

static void mqtt_cmd(const char *json, int len);

static void lock(void)
{
    if (s_lock) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
    }
}

static void unlock(void)
{
    if (s_lock) {
        xSemaphoreGive(s_lock);
    }
}

static void load_nvs(void)
{
    nvs_handle_t nvs;
    if (nvs_open("dryad", NVS_READONLY, &nvs) != ESP_OK) {
        return;
    }
    size_t n = sizeof(s_cfg.name);
    nvs_get_str(nvs, "name", s_cfg.name, &n);
    n = sizeof(s_cfg.broker);
    nvs_get_str(nvs, "broker", s_cfg.broker, &n);
    n = sizeof(s_cfg.ssid);
    nvs_get_str(nvs, "ssid", s_cfg.ssid, &n);
    n = sizeof(s_cfg.pass);
    nvs_get_str(nvs, "pass", s_cfg.pass, &n);
    int32_t v = 0;
    if (nvs_get_i32(nvs, "on", &v) == ESP_OK) {
        s_cfg.on_below = (int)v;
    }
    if (nvs_get_i32(nvs, "off", &v) == ESP_OK) {
        s_cfg.off_above = (int)v;
    }
    if (nvs_get_i32(nvs, "pwm", &v) == ESP_OK) {
        s_cfg.pwm = (int)v;
    }
    if (nvs_get_i32(nvs, "mqtt", &v) == ESP_OK) {
        s_cfg.mqtt_enabled = (int)v;
    }
    nvs_close(nvs);
}

static void save_nvs(void)
{
    nvs_handle_t nvs;
    if (nvs_open("dryad", NVS_READWRITE, &nvs) != ESP_OK) {
        return;
    }
    nvs_set_str(nvs, "name", s_cfg.name);
    nvs_set_str(nvs, "broker", s_cfg.broker);
    nvs_set_str(nvs, "ssid", s_cfg.ssid);
    nvs_set_str(nvs, "pass", s_cfg.pass);
    nvs_set_i32(nvs, "on", s_cfg.on_below);
    nvs_set_i32(nvs, "off", s_cfg.off_above);
    nvs_set_i32(nvs, "pwm", s_cfg.pwm);
    nvs_set_i32(nvs, "mqtt", s_cfg.mqtt_enabled);
    nvs_commit(nvs);
    nvs_close(nvs);
}

static void push_pump(void)
{
    pump_set_thresholds(s_cfg.on_below, s_cfg.off_above, s_cfg.pwm);
    pump_set_manual(s_cfg.manual_run);
}

static void fill_self_locked(void)
{
    snprintf(s_self.id, sizeof(s_self.id), "%s", s_mesh.self_id);
    snprintf(s_self.name, sizeof(s_self.name), "%s", s_cfg.name);
    s_self.on_below = s_cfg.on_below;
    s_self.off_above = s_cfg.off_above;
    s_self.pwm = s_cfg.pwm;
    s_self.mqtt = s_cfg.mqtt_enabled;
    s_self.hop = 0;
    s_self.root = (s_mesh_ready && esp_mesh_is_root()) ? 1 : 0;
}

static int format_snapshot(char *buf, int cap)
{
    char one[240];
    fill_self_locked();
    int n = snprintf(buf, (size_t)cap, "{\"self\":");
    if (n < 0 || n >= cap) {
        return -1;
    }
    int m = link_format_status(one, sizeof(one), &s_self);
    if (m < 0 || n + m >= cap) {
        return -1;
    }
    memcpy(buf + n, one, (size_t)m);
    n += m;
    int tail = snprintf(buf + n, (size_t)(cap - n), ",\"peers\":[");
    if (tail < 0 || n + tail >= cap) {
        return -1;
    }
    n += tail;
    int first = 1;
    for (int i = 0; i < LINK_MAX_PEERS; i++) {
        if (!s_mesh.peers[i].used) {
            continue;
        }
        link_status_t peer = {
            .moisture = s_mesh.peers[i].moisture,
            .pump = s_mesh.peers[i].pump,
            .duty = s_mesh.peers[i].duty,
            .on_below = 0,
            .off_above = 0,
            .pwm = 0,
            .hop = s_mesh.peers[i].hop,
            .seq = s_mesh.peers[i].seq,
            .mqtt = 0,
            .root = 0,
        };
        snprintf(peer.id, sizeof(peer.id), "%s", s_mesh.peers[i].id);
        snprintf(peer.name, sizeof(peer.name), "%s", s_mesh.peers[i].name);
        m = link_format_status(one, sizeof(one), &peer);
        tail = snprintf(buf + n, (size_t)(cap - n), "%s%s", first ? "" : ",", one);
        if (tail < 0 || n + tail >= cap) {
            return -1;
        }
        n += tail;
        first = 0;
        (void)m;
    }
    tail = snprintf(buf + n, (size_t)(cap - n), "],\"mesh\":\"DRYAD1\",\"mqtt\":%d}", s_cfg.mqtt_enabled);
    if (tail < 0 || n + tail >= cap) {
        return -1;
    }
    return n + tail;
}

static void mesh_send_line(const char *line)
{
    if (!s_mesh_up || !line) {
        return;
    }
    mesh_data_t data = {
        .data = (uint8_t *)line,
        .size = (uint16_t)strlen(line),
        .proto = MESH_PROTO_BIN,
        .tos = MESH_TOS_DEF,
    };
    mesh_addr_t to;
    memset(to.addr, 0xff, sizeof(to.addr));
    esp_err_t err = esp_mesh_send(&to, &data, MESH_DATA_P2P, NULL, 0);
    if (err != ESP_OK && err != ESP_ERR_MESH_NO_ROUTE_FOUND && err != ESP_ERR_MESH_DISCONNECTED) {
        ESP_LOGD(TAG, "mesh send %s", esp_err_to_name(err));
    }
}

/* from_mesh is reserved so a later hop can tell a phone command from a relay. */
static void apply_line(const char *line, int from_mesh)
{
    (void)from_mesh;
    link_msg_t msg;
    if (!link_parse(line, &msg)) {
        return;
    }
    int restart = 0;
    int relay = 0;
    int touch_mqtt = 0;
    char forward[256];
    link_settings_t cfg_copy;
    char id_copy[LINK_ID_LEN];
    forward[0] = '\0';
    id_copy[0] = '\0';
    memset(&cfg_copy, 0, sizeof(cfg_copy));

    lock();
    if (strcmp(msg.type, "status") == 0) {
        relay = link_mesh_ingest(&s_mesh, &msg, LINK_MAX_HOP);
        if (relay) {
            link_status_t st = {
                .moisture = msg.moisture,
                .pump = msg.pump == LINK_ABSENT ? 0 : msg.pump,
                .duty = msg.duty == LINK_ABSENT ? 0 : msg.duty,
                .on_below = 0,
                .off_above = 0,
                .pwm = 0,
                .hop = msg.hop + 1,
                .seq = msg.seq,
                .mqtt = 0,
                .root = 0,
            };
            snprintf(st.id, sizeof(st.id), "%s", msg.id);
            snprintf(st.name, sizeof(st.name), "%s", msg.name);
            link_format_status(forward, sizeof(forward), &st);
        }
    } else if (link_for_self(&msg, s_mesh.self_id)) {
        restart = link_apply(&s_cfg, &msg);
        push_pump();
        save_nvs();
        cfg_copy = s_cfg;
        snprintf(id_copy, sizeof(id_copy), "%s", s_mesh.self_id);
        touch_mqtt = 1;
    } else {
        snprintf(forward, sizeof(forward), "%s", line);
        relay = 1;
    }
    unlock();

    if (touch_mqtt) {
        mqtt_link_start(&cfg_copy, id_copy, mqtt_cmd);
    }
    if (relay && forward[0]) {
        mesh_send_line(forward);
    }
    if (restart) {
        ESP_LOGI(TAG, "home Wi-Fi changed, restarting into the new mesh");
        vTaskDelay(pdMS_TO_TICKS(400));
        esp_restart();
    }
}

static void mqtt_cmd(const char *json, int len)
{
    char line[256];
    int n = len < (int)sizeof(line) - 1 ? len : (int)sizeof(line) - 1;
    memcpy(line, json, (size_t)n);
    line[n] = '\0';
    apply_line(line, 0);
}

void mesh_net_publish(int moisture_percent, int pump_on)
{
    char line[256];
    lock();
    s_seq++;
    fill_self_locked();
    s_self.moisture = moisture_percent;
    s_self.pump = pump_on ? 1 : 0;
    s_self.duty = pump_on ? s_cfg.pwm : 0;
    s_self.seq = s_seq;
    link_format_status(line, sizeof(line), &s_self);
    unlock();
    mesh_send_line(line);
    mqtt_link_publish_status(line);
}

static esp_err_t read_body(httpd_req_t *req, char *buf, int cap)
{
    int total = 0;
    while (total < cap - 1 && total < (int)req->content_len) {
        int n = httpd_req_recv(req, buf + total, (size_t)(cap - 1 - total));
        if (n == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (n <= 0) {
            return ESP_FAIL;
        }
        total += n;
    }
    buf[total] = '\0';
    return ESP_OK;
}

static esp_err_t send_snapshot(httpd_req_t *req)
{
    char body[1400];
    lock();
    int n = format_snapshot(body, sizeof(body));
    unlock();
    if (n < 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "snapshot");
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, body, n);
    return ESP_OK;
}

static esp_err_t status_get(httpd_req_t *req)
{
    return send_snapshot(req);
}

static esp_err_t cmd_post(httpd_req_t *req)
{
    char line[256];
    if (read_body(req, line, sizeof(line)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "body");
        return ESP_FAIL;
    }
    apply_line(line, 0);
    return send_snapshot(req);
}

static void start_http(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = 80;
    cfg.ctrl_port = 32768;
    cfg.max_uri_handlers = 4;
    cfg.stack_size = 8192;
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "HTTP server failed");
        return;
    }
    httpd_uri_t status = {.uri = "/status", .method = HTTP_GET, .handler = status_get};
    httpd_uri_t cmd = {.uri = "/cmd", .method = HTTP_POST, .handler = cmd_post};
    httpd_register_uri_handler(server, &status);
    httpd_register_uri_handler(server, &cmd);
    ESP_LOGI(TAG, "phone API http://" DRYAD_AP_IP "/status  Wi-Fi DRYAD1 / " DRYAD_MESH_PASS);
}

static void start_phone_dhcp(void)
{
    if (!s_ap) {
        return;
    }
    esp_netif_dhcps_stop(s_ap);
    esp_netif_ip_info_t ip = {
        .ip.addr = esp_ip4addr_aton(DRYAD_AP_IP),
        .gw.addr = esp_ip4addr_aton(DRYAD_AP_IP),
        .netmask.addr = esp_ip4addr_aton("255.255.255.0"),
    };
    esp_netif_set_ip_info(s_ap, &ip);
    esp_netif_dhcps_start(s_ap);
}

static void mesh_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    (void)data;
    if (id == MESH_EVENT_STARTED || id == MESH_EVENT_PARENT_CONNECTED || id == MESH_EVENT_ROOT_ADDRESS) {
        s_mesh_up = 1;
        start_phone_dhcp();
    } else if (id == MESH_EVENT_STOPPED) {
        s_mesh_up = 0;
    }
}

static void mesh_rx_task(void *arg)
{
    (void)arg;
    uint8_t buf[256];
    while (1) {
        mesh_addr_t from;
        mesh_data_t data = {.data = buf, .size = sizeof(buf)};
        int flag = 0;
        esp_err_t err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        if (err != ESP_OK) {
            continue;
        }
        int n = data.size < sizeof(buf) - 1 ? data.size : (int)sizeof(buf) - 1;
        buf[n] = '\0';
        apply_line((char *)buf, 1);
    }
}

static void mesh_bringup(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    link_settings_init(&s_cfg);
    load_nvs();
    push_pump();

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char id[LINK_ID_LEN];
    snprintf(id, sizeof(id), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    link_mesh_init(&s_mesh, id);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(NULL, &s_ap));

    wifi_init_config_t wifi = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event, NULL));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(6));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_disable_ps());
    ESP_ERROR_CHECK(esp_mesh_send_block_time(2000));

    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    memcpy(cfg.mesh_id.addr, MESH_ID, 6);
    cfg.channel = 6;
    if (s_cfg.ssid[0]) {
        cfg.router.ssid_len = (uint8_t)strlen(s_cfg.ssid);
        memcpy(cfg.router.ssid, s_cfg.ssid, cfg.router.ssid_len);
        memset(cfg.router.password, 0, sizeof(cfg.router.password));
        memcpy(cfg.router.password, s_cfg.pass, strlen(s_cfg.pass));
        ESP_LOGI(TAG, "root may join home Wi-Fi \"%s\"", s_cfg.ssid);
    } else {
        cfg.router.ssid_len = 0;
        ESP_LOGI(TAG, "offline mesh, no home Wi-Fi");
    }
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK));
    cfg.mesh_ap.max_connection = 6;
    cfg.mesh_ap.nonmesh_max_connection = 3;
    memset(cfg.mesh_ap.password, 0, sizeof(cfg.mesh_ap.password));
    memcpy(cfg.mesh_ap.password, DRYAD_MESH_PASS, sizeof(DRYAD_MESH_PASS));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    ESP_ERROR_CHECK(esp_mesh_start());
    s_mesh_ready = 1;

    start_http();
    xTaskCreate(mesh_rx_task, "mesh_rx", 6144, NULL, 5, NULL);
    mqtt_link_start(&s_cfg, s_mesh.self_id, mqtt_cmd);
    ESP_LOGI(TAG, "node %s MQTT %s", id, s_cfg.mqtt_enabled ? "armed" : "disabled");
}

static void mesh_task(void *arg)
{
    (void)arg;
    mesh_bringup();
    vTaskDelete(NULL);
}

void mesh_net_start(void)
{
    s_lock = xSemaphoreCreateMutex();
    xTaskCreate(mesh_task, "mesh", 8192, NULL, 4, NULL);
}
