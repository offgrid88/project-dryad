#ifndef LINK_LOGIC_H
#define LINK_LOGIC_H

/* Device link messages for the mesh, the phone, and MQTT.
 * No ESP-IDF types: the host test compiles this file on its own.
 *
 * One flat JSON object per message. Integers that were not in the JSON
 * are LINK_ABSENT. mqtt is carried in status and config, and stays off
 * unless a caller explicitly sets it.
 */

#define LINK_ABSENT (-999)
#define LINK_NAME_LEN 24
#define LINK_ID_LEN 13
#define LINK_BROKER_LEN 64
#define LINK_SSID_LEN 33
#define LINK_PASS_LEN 65
#define LINK_MAX_PEERS 8
#define LINK_MAX_HOP 4

typedef struct {
    char type[12];
    char id[LINK_ID_LEN];
    char name[LINK_NAME_LEN];
    char broker[LINK_BROKER_LEN];
    char ssid[LINK_SSID_LEN];
    char pass[LINK_PASS_LEN];
    int moisture;
    int pump;
    int duty;
    int on_below;
    int off_above;
    int pwm;
    int hop;
    int seq;
    int mqtt;
    int run;
    int root;
    unsigned fields;
} link_msg_t;

#define LINK_F_NAME (1u << 0)
#define LINK_F_BROKER (1u << 1)
#define LINK_F_SSID (1u << 2)
#define LINK_F_PASS (1u << 3)

typedef struct {
    char id[LINK_ID_LEN];
    char name[LINK_NAME_LEN];
    char broker[LINK_BROKER_LEN];
    char ssid[LINK_SSID_LEN];
    char pass[LINK_PASS_LEN];
    int on_below;
    int off_above;
    int pwm;
    int mqtt_enabled;
    int manual_run; /* -1 auto, 0 force off, 1 force on */
} link_settings_t;

typedef struct {
    char id[LINK_ID_LEN];
    char name[LINK_NAME_LEN];
    int moisture;
    int pump;
    int duty;
    int hop;
    int seq;
    int used;
} link_peer_t;

typedef struct {
    char self_id[LINK_ID_LEN];
    link_peer_t peers[LINK_MAX_PEERS];
} link_mesh_t;

typedef struct {
    char id[LINK_ID_LEN];
    char name[LINK_NAME_LEN];
    int moisture;
    int pump;
    int duty;
    int on_below;
    int off_above;
    int pwm;
    int hop;
    int seq;
    int mqtt;
    int root;
} link_status_t;

void link_settings_init(link_settings_t *s);
void link_mesh_init(link_mesh_t *mesh, const char *self_id);

/* Returns 1 when `line` is a JSON object with a "t" field. */
int link_parse(const char *line, link_msg_t *out);

/* Apply set / pump / cfg onto settings. Returns 1 if a restart is needed
 * (home Wi-Fi SSID changed), 0 otherwise. Unknown types are ignored. */
int link_apply(link_settings_t *s, const link_msg_t *msg);

/* 1 when MQTT may open a socket: the switch is on and a broker URI is set. */
int link_mqtt_should_start(const link_settings_t *s);

/* 1 when id is missing or equal to self_id. */
int link_for_self(const link_msg_t *msg, const char *self_id);

/* Remember a status from another node. Returns 1 when this node should
 * relay it one hop further (new sequence, hop still under the limit). */
int link_mesh_ingest(link_mesh_t *mesh, const link_msg_t *msg, int max_hop);

int link_format_status(char *buf, int cap, const link_status_t *st);

#endif
