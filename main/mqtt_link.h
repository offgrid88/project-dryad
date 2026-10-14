#ifndef MQTT_LINK_H
#define MQTT_LINK_H

#include "link_logic.h"

/* MQTT client. Compiled in, left disconnected until link_mqtt_should_start()
 * is true (the switch is on and a broker URI is stored). The shipped default
 * is off: there is no broker yet.
 */

typedef void (*mqtt_cmd_fn)(const char *json, int len);

void mqtt_link_start(const link_settings_t *settings, const char *node_id, mqtt_cmd_fn on_cmd);
void mqtt_link_publish_status(const char *json);

#endif
