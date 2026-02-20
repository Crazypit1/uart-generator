/**
 * Генератор сигналов на LEDC (PWM), ESP-IDF.
 */
#include "generator.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include <string.h>

#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_10_BIT
#define LEDC_MAX_DUTY       ((1u << 10) - 1)
#define DEFAULT_FREQ_HZ     1000u
#define DEFAULT_DUTY_PERCENT 50u
#define MIN_FREQ_HZ         1u
#define MAX_FREQ_HZ         40000000u

static bool s_running = false;
static uint32_t s_freq_hz = DEFAULT_FREQ_HZ;
static uint8_t s_duty_percent = DEFAULT_DUTY_PERCENT;
static bool s_initialized = false;

static void apply_output(void)
{
    if (!s_initialized) return;
    if (!s_running) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
        return;
    }
    uint32_t duty = (uint32_t)s_duty_percent * (LEDC_MAX_DUTY + 1) / 100u;
    if (duty > LEDC_MAX_DUTY) duty = LEDC_MAX_DUTY;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, (uint32_t)duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

int generator_init(void)
{
    if (s_initialized) return 0;

    ledc_timer_config_t timer = {
        .speed_mode         = LEDC_MODE,
        .duty_resolution    = LEDC_DUTY_RES,
        .timer_num          = LEDC_TIMER,
        .freq_hz            = (uint32_t)DEFAULT_FREQ_HZ,
        .clk_cfg            = LEDC_AUTO_CLK,
    };
    esp_err_t e = ledc_timer_config(&timer);
    if (e != ESP_OK) return (int)e;

    ledc_channel_config_t ch = {
        .gpio_num   = GENERATOR_GPIO_PIN,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    e = ledc_channel_config(&ch);
    if (e != ESP_OK) return (int)e;

    s_initialized = true;
    s_freq_hz = DEFAULT_FREQ_HZ;
    s_duty_percent = DEFAULT_DUTY_PERCENT;
    s_running = false;
    apply_output();
    return 0;
}

int generator_set_freq_hz(uint32_t freq_hz)
{
    if (!s_initialized) return -1;
    if (freq_hz < MIN_FREQ_HZ || freq_hz > MAX_FREQ_HZ) return -2;

    esp_err_t e = ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq_hz);
    if (e != ESP_OK) return (int)e;
    s_freq_hz = freq_hz;
    return 0;
}

int generator_set_duty_percent(uint8_t duty_percent)
{
    if (!s_initialized) return -1;
    if (duty_percent > 100) return -2;
    s_duty_percent = duty_percent;
    apply_output();
    return 0;
}

void generator_start(void)
{
    s_running = true;
    apply_output();
}

void generator_stop(void)
{
    s_running = false;
    apply_output();
}

bool generator_is_running(void)
{
    return s_running;
}

uint32_t generator_get_freq_hz(void)
{
    return s_freq_hz;
}

uint8_t generator_get_duty_percent(void)
{
    return s_duty_percent;
}
