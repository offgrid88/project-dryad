#ifndef PUMP_LOGIC_H
#define PUMP_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

/* Pure moisture controller. No ESP-IDF types, so the host test can compile it.
 *
 * The pump turns on only when moisture is strictly below on_below_percent,
 * and only after it has been off for min_off_ms. It turns off when moisture
 * reaches off_above_percent, or when it has already run for max_on_ms.
 * Between the two thresholds the previous state is held (hysteresis).
 */

typedef struct {
    int on_below_percent;
    int off_above_percent;
    int pwm_percent;
    int max_on_ms;
    int min_off_ms;
} pump_config_t;

typedef struct {
    bool running;
    int duty_percent;
    int on_ms;
    int off_ms;
} pump_state_t;

/* off_ms starts at min_off_ms so a dry reading at boot may start the pump. */
void pump_logic_init(pump_state_t *st, const pump_config_t *cfg);

/* elapsed_ms is the time since the previous sample. moisture is clamped to 0..100. */
void pump_logic_step(const pump_config_t *cfg, pump_state_t *st, int moisture, int elapsed_ms);

#endif
