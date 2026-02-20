/**
 * Приём команд по UART и управление генератором.
 * Команды: FREQ <Hz>, DUTY <0-100>, ON, OFF, ?
 */
#include "uart_cmd.h"
#include "generator.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "uart_cmd";

#define UART_NUM_CMD     UART_NUM_0
#define BUF_SIZE         256
#define CMD_LINE_MAX      64

static void send_str(const char *s)
{
    uart_write_bytes(UART_NUM_CMD, (const char *)s, strlen(s));
}

static void process_line(char *line)
{
    /* обрезка пробелов и \r\n */
    size_t n = strlen(line);
    while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n' || line[n - 1] == ' ')) {
        line[--n] = '\0';
    }
    char *p = line;
    while (*p == ' ') p++;

    if (*p == '\0') return;

    if (strcmp(p, "?") == 0 || strcmp(p, "STATUS") == 0) {
        char buf[80];
        snprintf(buf, sizeof(buf),
            "FREQ=%lu DUTY=%u %s\r\n",
            (unsigned long)generator_get_freq_hz(),
            (unsigned)generator_get_duty_percent(),
            generator_is_running() ? "ON" : "OFF");
        send_str(buf);
        return;
    }

    if (strcmp(p, "ON") == 0 || strcmp(p, "START") == 0) {
        generator_start();
        send_str("OK ON\r\n");
        return;
    }

    if (strcmp(p, "OFF") == 0 || strcmp(p, "STOP") == 0) {
        generator_stop();
        send_str("OK OFF\r\n");
        return;
    }

    if (strncmp(p, "FREQ ", 5) == 0) {
        unsigned long val = strtoul(p + 5, NULL, 0);
        if (val == 0 || val > 40000000) {
            send_str("ERR FREQ range 1..40000000\r\n");
            return;
        }
        int r = generator_set_freq_hz((uint32_t)val);
        if (r == 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "OK FREQ %lu\r\n", val);
            send_str(buf);
        } else {
            send_str("ERR FREQ\r\n");
        }
        return;
    }

    if (strncmp(p, "DUTY ", 5) == 0) {
        unsigned long val = strtoul(p + 5, NULL, 0);
        if (val > 100) {
            send_str("ERR DUTY 0..100\r\n");
            return;
        }
        int r = generator_set_duty_percent((uint8_t)val);
        if (r == 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "OK DUTY %lu\r\n", val);
            send_str(buf);
        } else {
            send_str("ERR DUTY\r\n");
        }
        return;
    }

    if (strcmp(p, "HELP") == 0) {
        send_str(
            "FREQ <Hz>   - частота 1..40000000\r\n"
            "DUTY <0-100> - скважность %\r\n"
            "ON|START    - включить выход\r\n"
            "OFF|STOP    - выключить выход\r\n"
            "?|STATUS    - состояние\r\n"
            "HELP        - эта справка\r\n"
        );
        return;
    }

    send_str("ERR unknown command (HELP)\r\n");
}

static void uart_task(void *arg)
{
    uint8_t *buf = malloc(BUF_SIZE);
    if (!buf) {
        ESP_LOGE(TAG, "no mem for uart buf");
        vTaskDelete(NULL);
        return;
    }
    char line[CMD_LINE_MAX];
    size_t line_len = 0;

    send_str("\r\nUART Generator. Commands: FREQ, DUTY, ON, OFF, ?\r\n");

    for (;;) {
        int len = uart_read_bytes(UART_NUM_CMD, buf, BUF_SIZE - 1, pdMS_TO_TICKS(50));
        if (len <= 0) {
            vTaskDelay(pdMS_TO_TICKS(10)); /* отдать CPU, избежать TG0WDT на C3/USB */
            continue;
        }

        buf[len] = '\0';
        for (int i = 0; i < len; i++) {
            char c = (char)buf[i];
            if (c == '\n' || c == '\r' || line_len >= CMD_LINE_MAX - 1) {
                line[line_len] = '\0';
                if (line_len > 0)
                    process_line(line);
                line_len = 0;
                if (c != '\n' && c != '\r')
                    line[line_len++] = c;
            } else {
                line[line_len++] = c;
            }
        }
    }
}

void uart_cmd_start(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    esp_err_t e = uart_param_config(UART_NUM_CMD, &uart_config);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config %s", esp_err_to_name(e));
        return;
    }
    e = uart_set_pin(UART_NUM_CMD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin %s", esp_err_to_name(e));
        return;
    }
    e = uart_driver_install(UART_NUM_CMD, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install %s", esp_err_to_name(e));
        return;
    }

    xTaskCreate(uart_task, "uart_cmd", 4096, NULL, 5, NULL);
}
