// include moisture
#pragma once

void setup_adc();
int read_moisture_percent();
void moisture_sensor_task(void* pvParameters);