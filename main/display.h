#ifndef DISPLAY_H
#define DISPLAY_H

// Entry point — launched as a FreeRTOS task from app_main
void display_task(void *pvParameters);

// Called by moisture_sensor_task to push a new reading to the display
void display_set_moisture(int percent);

#endif // DISPLAY_H
