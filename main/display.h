#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>

// Entry point — launched as a FreeRTOS task from app_main
void display_task(void *pvParameters);

/* Latest moisture reading and whether the pump is running.
 * Safe to call from the sensor task; the display task paints it. */
void display_set_status(int moisture_percent, bool pump_on);

#endif // DISPLAY_H
