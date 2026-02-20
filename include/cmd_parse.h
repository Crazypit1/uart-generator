/**
 * Парсер текстовых команд протокола UART-генератора.
 * Чистый C, без зависимостей от ESP-IDF — для unit-тестов на хосте.
 */
#ifndef CMD_PARSE_H
#define CMD_PARSE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CMD_NONE,
    CMD_FREQ,
    CMD_DUTY,
    CMD_ON,
    CMD_OFF,
    CMD_STATUS,
    CMD_HELP,
} cmd_type_t;

typedef struct {
    cmd_type_t type;
    uint32_t freq;   /* для CMD_FREQ */
    uint8_t duty;    /* для CMD_DUTY */
} cmd_result_t;

#define CMD_FREQ_MIN  1u
#define CMD_FREQ_MAX  40000000u
#define CMD_DUTY_MAX  100u

/**
 * Разбор одной строки команды (без \r\n).
 * out заполняется при успехе.
 * Возврат: 0 — распознано и параметры в допустимых пределах,
 *         -1 — неизвестная команда,
 *         -2 — неверное значение (частота/скважность вне диапазона).
 */
int cmd_parse(const char *line, cmd_result_t *out);

#ifdef __cplusplus
}
#endif

#endif /* CMD_PARSE_H */
