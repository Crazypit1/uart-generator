/**
 * Генератор сигналов (PWM/LEDC) для ESP32.
 * Управление: частота, скважность, вкл/выкл.
 */
#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Пин выхода сигнала по умолчанию (можно переопределить в config).
 *  ESP32: 25 и др.  ESP32-C3: 0..21, без 11..17 (flash); удобно 3,4,5,6,7. */
#ifndef GENERATOR_GPIO_PIN
#define GENERATOR_GPIO_PIN  5
#endif

/** Инициализация генератора. Возвращает 0 при успехе. */
int generator_init(void);

/** Установить частоту в Гц. Допустимый диапазон зависит от разрешения. */
int generator_set_freq_hz(uint32_t freq_hz);

/** Установить скважность 0..100 (проценты). */
int generator_set_duty_percent(uint8_t duty_percent);

/** Включить выход. */
void generator_start(void);

/** Выключить выход. */
void generator_stop(void);

/** Включён ли выход. */
bool generator_is_running(void);

/** Текущая частота, Гц. */
uint32_t generator_get_freq_hz(void);

/** Текущая скважность, 0..100. */
uint8_t generator_get_duty_percent(void);

#ifdef __cplusplus
}
#endif

#endif /* GENERATOR_H */
