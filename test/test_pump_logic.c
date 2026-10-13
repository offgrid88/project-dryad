#include <stdio.h>
#include <stdlib.h>

#include "pump_logic.h"

static int failures;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        failures++;
    }
}

static pump_config_t cfg(void)
{
    pump_config_t c = {
        .on_below_percent = 35,
        .off_above_percent = 45,
        .pwm_percent = 70,
        .max_on_ms = 20000,
        .min_off_ms = 60000,
    };
    return c;
}

static void test_dry_boot_starts(void)
{
    pump_config_t c = cfg();
    pump_state_t s;
    pump_logic_init(&s, &c);
    pump_logic_step(&c, &s, 20, 2000);
    expect(s.running, "dry soil at boot starts the pump");
    expect(s.duty_percent == 70, "running duty is the configured PWM");
}

static void test_wet_stays_off(void)
{
    pump_config_t c = cfg();
    pump_state_t s;
    pump_logic_init(&s, &c);
    pump_logic_step(&c, &s, 80, 2000);
    expect(!s.running, "wet soil does not start the pump");
    expect(s.duty_percent == 0, "idle duty is zero");
}

static void test_threshold_is_strict(void)
{
    pump_config_t c = cfg();
    pump_state_t s;
    pump_logic_init(&s, &c);
    pump_logic_step(&c, &s, 35, 2000);
    expect(!s.running, "moisture equal to the on threshold stays off");
    pump_logic_step(&c, &s, 34, 0);
    expect(s.running, "one percent drier than the threshold starts");
}

static void test_hysteresis_holds(void)
{
    pump_config_t c = cfg();
    pump_state_t s;
    pump_logic_init(&s, &c);
    pump_logic_step(&c, &s, 10, 1000);
    expect(s.running, "pump is on before the band");
    pump_logic_step(&c, &s, 40, 1000);
    expect(s.running, "between thresholds the pump stays on");
    pump_logic_step(&c, &s, 45, 1000);
    expect(!s.running, "reaching the off threshold stops the pump");
    pump_logic_step(&c, &s, 40, 60000);
    expect(!s.running, "between thresholds the pump stays off");
}

static void test_max_on_time(void)
{
    pump_config_t c = cfg();
    pump_state_t s;
    pump_logic_init(&s, &c);
    pump_logic_step(&c, &s, 10, 1000);
    pump_logic_step(&c, &s, 10, 20000);
    expect(!s.running, "max on time stops the pump while soil is still dry");
    pump_logic_step(&c, &s, 10, 59000);
    expect(!s.running, "min off time blocks an immediate restart");
    pump_logic_step(&c, &s, 10, 1000);
    expect(s.running, "pump restarts once the off time has elapsed");
}

static void test_moisture_is_clamped(void)
{
    pump_config_t c = cfg();
    pump_state_t s;
    pump_logic_init(&s, &c);
    pump_logic_step(&c, &s, -5, 1000);
    expect(s.running, "negative moisture is treated as dry");
    pump_logic_step(&c, &s, 140, 1000);
    expect(!s.running, "moisture above 100 is treated as saturated");
}

int main(void)
{
    test_dry_boot_starts();
    test_wet_stays_off();
    test_threshold_is_strict();
    test_hysteresis_holds();
    test_max_on_time();
    test_moisture_is_clamped();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return EXIT_FAILURE;
    }
    printf("pump logic ok\n");
    return EXIT_SUCCESS;
}
