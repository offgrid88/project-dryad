#include <stdio.h>
#include <string.h>

#include "link_logic.h"

static int failures;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        failures++;
    }
}

static void test_status_roundtrip(void)
{
    link_status_t st = {
        .id = "aabbccddeeff",
        .name = "Basil",
        .moisture = 42,
        .pump = 0,
        .duty = 0,
        .on_below = 35,
        .off_above = 45,
        .pwm = 70,
        .hop = 0,
        .seq = 3,
        .mqtt = 0,
        .root = 1,
    };
    char buf[256];
    expect(link_format_status(buf, sizeof(buf), &st) > 0, "format status");
    link_msg_t msg;
    expect(link_parse(buf, &msg), "parse status");
    expect(strcmp(msg.type, "status") == 0, "type status");
    expect(msg.moisture == 42, "moisture");
    expect(msg.mqtt == 0, "mqtt stays off in status");
    expect(msg.root == 1, "root flag");
    expect(strcmp(msg.name, "Basil") == 0, "name");
}

static void test_cfg_does_not_enable_mqtt_by_default(void)
{
    link_settings_t s;
    link_settings_init(&s);
    expect(!link_mqtt_should_start(&s), "fresh settings do not start mqtt");

    link_msg_t msg;
    expect(link_parse("{\"t\":\"cfg\",\"name\":\"Herb\"}", &msg), "parse cfg");
    expect(link_apply(&s, &msg) == 0, "name change does not reboot");
    expect(strcmp(s.name, "Herb") == 0, "name applied");
    expect(s.mqtt_enabled == 0, "mqtt still disabled");
    expect(!link_mqtt_should_start(&s), "still no broker");
    expect(s.ssid[0] == '\0', "omitted ssid is not wiped");
}

static void test_mqtt_arms_only_with_broker(void)
{
    link_settings_t s;
    link_settings_init(&s);
    link_msg_t msg;
    link_parse("{\"t\":\"cfg\",\"mqtt\":1}", &msg);
    link_apply(&s, &msg);
    expect(s.mqtt_enabled == 1, "switch can be stored");
    expect(!link_mqtt_should_start(&s), "switch without a broker does not connect");

    link_parse("{\"t\":\"cfg\",\"mqtt\":1,\"broker\":\"mqtt://10.0.0.8\"}", &msg);
    link_apply(&s, &msg);
    expect(link_mqtt_should_start(&s), "broker plus switch may connect");

    link_parse("{\"t\":\"cfg\",\"mqtt\":0}", &msg);
    link_apply(&s, &msg);
    expect(!link_mqtt_should_start(&s), "switch off stops mqtt");
}

static void test_thresholds_and_pump(void)
{
    link_settings_t s;
    link_settings_init(&s);
    link_msg_t msg;
    link_parse("{\"t\":\"set\",\"on\":30,\"off\":55,\"pwm\":60}", &msg);
    link_apply(&s, &msg);
    expect(s.on_below == 30 && s.off_above == 55 && s.pwm == 60, "thresholds applied");

    link_parse("{\"t\":\"set\",\"on\":80,\"off\":40}", &msg);
    link_apply(&s, &msg);
    expect(s.on_below == 30 && s.off_above == 55, "inverted thresholds rejected");

    link_parse("{\"t\":\"pump\",\"run\":1}", &msg);
    link_apply(&s, &msg);
    expect(s.manual_run == 1, "manual on");
    link_parse("{\"t\":\"pump\",\"run\":-1}", &msg);
    link_apply(&s, &msg);
    expect(s.manual_run == -1, "back to auto");
}

static void test_mesh_relay_and_dedup(void)
{
    link_mesh_t mesh;
    link_mesh_init(&mesh, "self00000001");
    link_msg_t msg;
    link_parse("{\"t\":\"status\",\"id\":\"peer00000002\",\"name\":\"Fern\",\"moisture\":18,\"pump\":1,\"hop\":0,\"seq\":7}", &msg);
    expect(link_mesh_ingest(&mesh, &msg, LINK_MAX_HOP) == 1, "new peer is relayed");
    expect(link_mesh_ingest(&mesh, &msg, LINK_MAX_HOP) == 0, "duplicate seq is not relayed");

    link_parse("{\"t\":\"status\",\"id\":\"peer00000002\",\"name\":\"Fern\",\"moisture\":19,\"pump\":1,\"hop\":4,\"seq\":8}", &msg);
    expect(link_mesh_ingest(&mesh, &msg, LINK_MAX_HOP) == 0, "hop limit stores but does not relay");
    expect(mesh.peers[0].moisture == 19, "peer table updated at hop limit");

    link_parse("{\"t\":\"status\",\"id\":\"self00000001\",\"moisture\":10,\"seq\":1}", &msg);
    expect(link_mesh_ingest(&mesh, &msg, LINK_MAX_HOP) == 0, "own status is ignored");
}

static void test_wifi_change_restarts(void)
{
    link_settings_t s;
    link_settings_init(&s);
    link_msg_t msg;
    link_parse("{\"t\":\"cfg\",\"ssid\":\"home\"}", &msg);
    expect(link_apply(&s, &msg) == 1, "new home ssid asks for a restart");
    expect(strcmp(s.ssid, "home") == 0, "ssid stored");
    link_parse("{\"t\":\"cfg\",\"name\":\"Mint\"}", &msg);
    expect(link_apply(&s, &msg) == 0, "unrelated cfg keeps the ssid");
    expect(strcmp(s.ssid, "home") == 0, "ssid survives");
}

int main(void)
{
    test_status_roundtrip();
    test_cfg_does_not_enable_mqtt_by_default();
    test_mqtt_arms_only_with_broker();
    test_thresholds_and_pump();
    test_mesh_relay_and_dedup();
    test_wifi_change_restarts();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("link logic ok\n");
    return 0;
}
