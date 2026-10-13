#ifndef PUMP_H
#define PUMP_H

#include <stdbool.h>

/* Automatic watering pump.
 *
 * Default driver is a mini L298 (or L298N) module used in one direction.
 * A water pump must not reverse, so IN2 is tied to GND on the module and
 * firmware only drives ENA (PWM) and IN1.
 *
 *   GPIO6  -> L298 ENA   (PWM, 1 kHz). Same pin drives a MOSFET gate.
 *   GPIO10 -> L298 IN1   (high = run forward). Leave unconnected for a MOSFET.
 *   L298 IN2 tied to GND. Motor supply is the pump supply, not 3.3 V.
 *
 * These two GPIOs are free on ESP32-S3, ESP32-C3, and ESP32-C6 dev boards
 * and are not the moisture ADC pin or the S3 display bus. They are also
 * not strapping pins, so the driver cannot hold the chip in download mode.
 *
 * L298 is what the board is wired for today. It drops about 1.5–2 V and
 * warms up even at pump currents. Prefer one of these instead when you
 * rebuild the driver:
 *
 *   1. Logic-level N-MOSFET low-side switch (AO3400A, or IRLZ44N for a
 *      bigger pump) plus a Schottky flyback diode across the pump.
 *      Gate <- GPIO6 through 100 ohm, source to GND, drain to the pump
 *      negative. Set PUMP_DRIVER_MOSFET to 1. PWM then runs at 20 kHz so
 *      the pump is quiet, and the drop is a few tens of millivolts.
 *   2. TB6612FNG or DRV8870 module if you want a packaged H-bridge. Both
 *      drop far less than an L298. Wire the PWM input to GPIO6 and the
 *      direction pin to GPIO10 the same way.
 *
 * Thresholds (percent moisture, capacitive sensor, higher = wetter):
 *   on below 35, off at or above 45, PWM 70 percent,
 *   at most 20 s on, then at least 60 s off.
 */

#ifndef PUMP_DRIVER_MOSFET
#define PUMP_DRIVER_MOSFET 0
#endif

#define PUMP_PWM_GPIO 6
#define PUMP_IN1_GPIO 10

#define PUMP_ON_BELOW_PERCENT 35
#define PUMP_OFF_ABOVE_PERCENT 45
#define PUMP_PWM_PERCENT 70
#define PUMP_MAX_ON_MS 20000
#define PUMP_MIN_OFF_MS 60000

void pump_init(void);
void pump_update(int moisture_percent, int elapsed_ms);
bool pump_is_running(void);

/* Runtime overrides from the phone. on must stay below off. pwm is 0..100.
 * manual is -1 auto, 0 force off, 1 force on. Max-on and min-off still apply. */
void pump_set_thresholds(int on_below, int off_above, int pwm_percent);
void pump_set_manual(int manual);

#endif
