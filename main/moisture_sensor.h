#ifndef MOISTURE_SENSOR_H
#define MOISTURE_SENSOR_H

void setup_adc(void);
int read_moisture_percent(void);
void moisture_sensor_task(void* pvParameters);

#endif // MOISTURE_SENSOR_H