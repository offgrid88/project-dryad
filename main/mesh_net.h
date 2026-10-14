#ifndef MESH_NET_H
#define MESH_NET_H

/* Wi-Fi mesh (device to device) and the HTTP port the phone uses.
 * Safe to call mesh_net_publish from the moisture task. */

void mesh_net_start(void);
void mesh_net_publish(int moisture_percent, int pump_on);

#endif
