#include "pump_logic.h"

static int clamp_percent(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

void pump_logic_init(pump_state_t *st, const pump_config_t *cfg)
{
    st->running = false;
    st->duty_percent = 0;
    st->on_ms = 0;
    st->off_ms = cfg->min_off_ms;
}

void pump_logic_step(const pump_config_t *cfg, pump_state_t *st, int moisture, int elapsed_ms)
{
    if (elapsed_ms < 0) {
        elapsed_ms = 0;
    }
    moisture = clamp_percent(moisture);

    if (st->running) {
        st->on_ms += elapsed_ms;
        st->off_ms = 0;
        if (moisture >= cfg->off_above_percent || st->on_ms >= cfg->max_on_ms) {
            st->running = false;
            st->duty_percent = 0;
            st->on_ms = 0;
            st->off_ms = 0;
        } else {
            st->duty_percent = clamp_percent(cfg->pwm_percent);
        }
        return;
    }

    st->off_ms += elapsed_ms;
    st->duty_percent = 0;
    if (moisture < cfg->on_below_percent && st->off_ms >= cfg->min_off_ms) {
        st->running = true;
        st->duty_percent = clamp_percent(cfg->pwm_percent);
        st->on_ms = 0;
    }
}
