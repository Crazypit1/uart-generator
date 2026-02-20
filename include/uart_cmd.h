/**
 * UART-команды для управления генератором.
 * Протокол: текстовые строки, одна команда на строку (оканчиваются \n или \r\n).
 */
#ifndef UART_CMD_H
#define UART_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

/** Запуск задачи приёма UART и обработки команд. Использует UART_NUM_0, 115200. */
void uart_cmd_start(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_CMD_H */
