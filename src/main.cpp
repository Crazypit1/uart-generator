/**
 * Точка входа: генератор сигналов, управляемый по UART.
 * Плата: ESP32, UART0 115200, выход PWM на GPIO25.
 */
#include "generator.h"
#include "uart_cmd.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "UART Generator start");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    if (generator_init() != 0) {
        ESP_LOGE(TAG, "generator_init failed");
        return;
    }

    uart_cmd_start();

    ESP_LOGI(TAG, "Send commands via UART: FREQ, DUTY, ON, OFF, ?");
}
