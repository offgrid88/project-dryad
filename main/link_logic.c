#include "link_logic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void link_settings_init(link_settings_t *s)
{
    memset(s, 0, sizeof(*s));
    snprintf(s->name, sizeof(s->name), "Dryad");
    s->on_below = 35;
    s->off_above = 45;
    s->pwm = 70;
    s->mqtt_enabled = 0;
    s->manual_run = -1;
}

void link_mesh_init(link_mesh_t *mesh, const char *self_id)
{
    memset(mesh, 0, sizeof(*mesh));
    snprintf(mesh->self_id, sizeof(mesh->self_id), "%s", self_id ? self_id : "");
}

static void blank(link_msg_t *m)
{
    memset(m, 0, sizeof(*m));
    m->moisture = LINK_ABSENT;
    m->pump = LINK_ABSENT;
    m->duty = LINK_ABSENT;
    m->on_below = LINK_ABSENT;
    m->off_above = LINK_ABSENT;
    m->pwm = LINK_ABSENT;
    m->hop = 0;
    m->seq = LINK_ABSENT;
    m->mqtt = LINK_ABSENT;
    m->run = LINK_ABSENT;
    m->root = LINK_ABSENT;
}

static const char *find_key(const char *s, const char *key)
{
    char pat[40];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = s;
    size_t n = strlen(pat);
    while ((p = strstr(p, pat)) != NULL) {
        const char *q = p + n;
        while (*q == ' ') {
            q++;
        }
        if (*q == ':') {
            q++;
            while (*q == ' ') {
                q++;
            }
            return q;
        }
        p += n;
    }
    return NULL;
}

static int take_int(const char *s, const char *key, int *out)
{
    const char *p = find_key(s, key);
    if (!p) {
        return 0;
    }
    *out = atoi(p);
    return 1;
}

static int take_str(const char *s, const char *key, char *out, int cap)
{
    const char *p = find_key(s, key);
    if (!p || *p != '"') {
        return 0;
    }
    p++;
    int n = 0;
    while (*p && *p != '"' && n < cap - 1) {
        if (*p == '\\' && p[1]) {
            p++;
        }
        out[n++] = *p++;
    }
    out[n] = '\0';
    return 1;
}

int link_parse(const char *line, link_msg_t *out)
{
    blank(out);
    if (!line || !strchr(line, '{')) {
        return 0;
    }
    if (!take_str(line, "t", out->type, sizeof(out->type))) {
        return 0;
    }
    take_str(line, "id", out->id, sizeof(out->id));
    if (take_str(line, "name", out->name, sizeof(out->name))) {
        out->fields |= LINK_F_NAME;
    }
    if (take_str(line, "broker", out->broker, sizeof(out->broker))) {
        out->fields |= LINK_F_BROKER;
    }
    if (take_str(line, "ssid", out->ssid, sizeof(out->ssid))) {
        out->fields |= LINK_F_SSID;
    }
    if (take_str(line, "pass", out->pass, sizeof(out->pass))) {
        out->fields |= LINK_F_PASS;
    }
    take_int(line, "moisture", &out->moisture);
    take_int(line, "pump", &out->pump);
    take_int(line, "duty", &out->duty);
    take_int(line, "on", &out->on_below);
    take_int(line, "off", &out->off_above);
    take_int(line, "pwm", &out->pwm);
    take_int(line, "hop", &out->hop);
    take_int(line, "seq", &out->seq);
    take_int(line, "mqtt", &out->mqtt);
    take_int(line, "run", &out->run);
    take_int(line, "root", &out->root);
    return 1;
}

static int thresholds_ok(int on, int off, int pwm)
{
    return on >= 1 && on <= 98 && off >= 2 && off <= 100 && on < off && pwm >= 0 && pwm <= 100;
}

int link_apply(link_settings_t *s, const link_msg_t *msg)
{
    int restart = 0;
    if (strcmp(msg->type, "set") == 0) {
        int on = msg->on_below == LINK_ABSENT ? s->on_below : msg->on_below;
        int off = msg->off_above == LINK_ABSENT ? s->off_above : msg->off_above;
        int pwm = msg->pwm == LINK_ABSENT ? s->pwm : msg->pwm;
        if (thresholds_ok(on, off, pwm)) {
            s->on_below = on;
            s->off_above = off;
            s->pwm = pwm;
        }
    } else if (strcmp(msg->type, "pump") == 0) {
        if (msg->run == -1 || msg->run == 0 || msg->run == 1) {
            s->manual_run = msg->run;
        }
    } else if (strcmp(msg->type, "cfg") == 0) {
        if ((msg->fields & LINK_F_NAME) && msg->name[0]) {
            snprintf(s->name, sizeof(s->name), "%s", msg->name);
        }
        if (msg->mqtt == 0 || msg->mqtt == 1) {
            s->mqtt_enabled = msg->mqtt;
        }
        if (msg->fields & LINK_F_BROKER) {
            snprintf(s->broker, sizeof(s->broker), "%s", msg->broker);
        }
        if (msg->fields & LINK_F_SSID) {
            if (strcmp(s->ssid, msg->ssid) != 0) {
                restart = 1;
            }
            snprintf(s->ssid, sizeof(s->ssid), "%s", msg->ssid);
        }
        if (msg->fields & LINK_F_PASS) {
            snprintf(s->pass, sizeof(s->pass), "%s", msg->pass);
        }
    }
    return restart;
}

int link_mqtt_should_start(const link_settings_t *s)
{
    return s->mqtt_enabled && s->broker[0] != '\0';
}

int link_for_self(const link_msg_t *msg, const char *self_id)
{
    return msg->id[0] == '\0' || (self_id && strcmp(msg->id, self_id) == 0);
}

static link_peer_t *slot_for(link_mesh_t *mesh, const char *id)
{
    link_peer_t *free_slot = NULL;
    for (int i = 0; i < LINK_MAX_PEERS; i++) {
        if (mesh->peers[i].used && strcmp(mesh->peers[i].id, id) == 0) {
            return &mesh->peers[i];
        }
        if (!mesh->peers[i].used && !free_slot) {
            free_slot = &mesh->peers[i];
        }
    }
    return free_slot;
}

int link_mesh_ingest(link_mesh_t *mesh, const link_msg_t *msg, int max_hop)
{
    if (strcmp(msg->type, "status") != 0 || msg->id[0] == '\0') {
        return 0;
    }
    if (strcmp(msg->id, mesh->self_id) == 0) {
        return 0;
    }
    if (msg->seq == LINK_ABSENT) {
        return 0;
    }
    link_peer_t *peer = slot_for(mesh, msg->id);
    if (!peer) {
        return 0;
    }
    if (peer->used && peer->seq == msg->seq) {
        return 0;
    }
    peer->used = 1;
    snprintf(peer->id, sizeof(peer->id), "%s", msg->id);
    snprintf(peer->name, sizeof(peer->name), "%s", msg->name);
    peer->moisture = msg->moisture;
    peer->pump = msg->pump == LINK_ABSENT ? 0 : msg->pump;
    peer->duty = msg->duty == LINK_ABSENT ? 0 : msg->duty;
    peer->hop = msg->hop;
    peer->seq = msg->seq;
    return msg->hop < max_hop;
}

int link_format_status(char *buf, int cap, const link_status_t *st)
{
    return snprintf(buf, (size_t)cap,
                    "{\"t\":\"status\",\"id\":\"%s\",\"name\":\"%s\",\"moisture\":%d,"
                    "\"pump\":%d,\"duty\":%d,\"on\":%d,\"off\":%d,\"pwm\":%d,"
                    "\"hop\":%d,\"seq\":%d,\"mqtt\":%d,\"root\":%d}",
                    st->id, st->name, st->moisture, st->pump, st->duty, st->on_below,
                    st->off_above, st->pwm, st->hop, st->seq, st->mqtt, st->root);
}
