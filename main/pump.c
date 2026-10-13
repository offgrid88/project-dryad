#include "pump.h"

#include "pump_logic.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "pump";

#define PUMP_LEDC_MODE LEDC_LOW_SPEED_MODE
#define PUMP_LEDC_TIMER LEDC_TIMER_0
#define PUMP_LEDC_CHANNEL LEDC_CHANNEL_0
#define PUMP_LEDC_BITS LEDC_TIMER_10_BIT

#if PUMP_DRIVER_MOSFET
#define PUMP_PWM_HZ 20000
#define PUMP_DRIVER_NAME "MOSFET"
#else
#define PUMP_PWM_HZ 1000
#define PUMP_DRIVER_NAME "L298"
#endif

static pump_config_t s_cfg = {
    .on_below_percent = PUMP_ON_BELOW_PERCENT,
    .off_above_percent = PUMP_OFF_ABOVE_PERCENT,
    .pwm_percent = PUMP_PWM_PERCENT,
    .max_on_ms = PUMP_MAX_ON_MS,
    .min_off_ms = PUMP_MIN_OFF_MS,
};

static pump_state_t s_state;
static bool s_ready;
static int s_manual = -1;

static void apply_outputs(void)
{
    uint32_t full = (1u << PUMP_LEDC_BITS) - 1u;
    uint32_t duty = (uint32_t)s_state.duty_percent * full / 100u;

    ledc_set_duty(PUMP_LEDC_MODE, PUMP_LEDC_CHANNEL, duty);
    ledc_update_duty(PUMP_LEDC_MODE, PUMP_LEDC_CHANNEL);
    /* MOSFET mode leaves IN1 low. L298 IN1 high selects the forward path;
     * IN2 is tied to GND on the module so the pump cannot reverse. */
    gpio_set_level(PUMP_IN1_GPIO, (!PUMP_DRIVER_MOSFET && s_state.running) ? 1 : 0);
}

void pump_init(void)
{
    pump_logic_init(&s_state, &s_cfg);

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << PUMP_IN1_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
    gpio_set_level(PUMP_IN1_GPIO, 0);

    ledc_timer_config_t timer = {
        .speed_mode = PUMP_LEDC_MODE,
        .duty_resolution = PUMP_LEDC_BITS,
        .timer_num = PUMP_LEDC_TIMER,
        .freq_hz = PUMP_PWM_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .gpio_num = PUMP_PWM_GPIO,
        .speed_mode = PUMP_LEDC_MODE,
        .channel = PUMP_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = PUMP_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));

    s_ready = true;
    ESP_LOGI(TAG, "%s on GPIO%d PWM %d Hz, IN1 GPIO%d. On <%d%%, off >=%d%%",
             PUMP_DRIVER_NAME, PUMP_PWM_GPIO, PUMP_PWM_HZ, PUMP_IN1_GPIO,
             PUMP_ON_BELOW_PERCENT, PUMP_OFF_ABOVE_PERCENT);
}

void pump_set_thresholds(int on_below, int off_above, int pwm_percent)
{
    if (on_below < 1 || on_below > 98 || off_above < 2 || off_above > 100 ||
        on_below >= off_above || pwm_percent < 0 || pwm_percent > 100) {
        return;
    }
    s_cfg.on_below_percent = on_below;
    s_cfg.off_above_percent = off_above;
    s_cfg.pwm_percent = pwm_percent;
}

void pump_set_manual(int manual)
{
    if (manual == -1 || manual == 0 || manual == 1) {
        s_manual = manual;
    }
}

void pump_update(int moisture_percent, int elapsed_ms)
{
    if (!s_ready) {
        pump_init();
    }

    /* Manual on looks dry, manual off looks wet. The real sample is unchanged
     * for the display and the mesh status. */
    int sample = moisture_percent;
    if (s_manual == 0) {
        sample = 100;
    } else if (s_manual == 1) {
        sample = 0;
    }

    bool was_running = s_state.running;
    pump_logic_step(&s_cfg, &s_state, sample, elapsed_ms);
    apply_outputs();

    if (was_running != s_state.running) {
        ESP_LOGI(TAG, "pump %s (moisture %d%%, duty %d%%)",
                 s_state.running ? "ON" : "OFF", moisture_percent, s_state.duty_percent);
    }
}

bool pump_is_running(void)
{
    return s_state.running;
}
