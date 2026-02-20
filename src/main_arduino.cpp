/**
 * UART-генератор на Arduino (ESP32-C3).
 * Протокол: VER? (идентификация), FREQ, DUTY, ON, OFF, ?
 */
#ifndef CONFIG_IDF_TARGET_ESP32C3
#define CONFIG_IDF_TARGET_ESP32C3 1
#endif
#ifndef ARDUINO_USB_MODE
#define ARDUINO_USB_MODE 1
#endif
#ifndef ARDUINO_USB_CDC_ON_BOOT
#define ARDUINO_USB_CDC_ON_BOOT 1
#endif
#include <Arduino.h>

#define FIRMWARE_ID     "UART-GEN"
#define FIRMWARE_VER    "1.0"
#define GENERATOR_PIN   5
#define LEDC_CHANNEL    0
#define LEDC_RES_BIT    10
#define LEDC_MAX_DUTY   ((1 << LEDC_RES_BIT) - 1)
#define FREQ_MIN        1
#define FREQ_MAX        40000000
#define CMD_LINE_MAX    64

static uint32_t s_freq = 1000;
static uint8_t s_dutyPercent = 50;
static bool s_running = false;
static char s_line[CMD_LINE_MAX];
static size_t s_lineLen = 0;

/* Обработчик USB BUS_RESET: после отключения/подключения кабеля переинициализируем CDC,
   чтобы хост (ПК) снова увидел порт и генератор определялся в программе. */
static void onUsbBusReset(void* arg, esp_event_base_t base, int32_t id, void* data) {
    (void)arg;
    (void)base;
    (void)id;
    (void)data;
    delay(30);
    Serial.end();
    delay(30);
    Serial.begin(115200);
}

static void applyOutput() {
    if (!s_running) {
        ledcWrite(LEDC_CHANNEL, 0);
        return;
    }
    uint32_t d = (uint32_t)s_dutyPercent * (LEDC_MAX_DUTY + 1) / 100;
    if (d > LEDC_MAX_DUTY) d = LEDC_MAX_DUTY;
    ledcWrite(LEDC_CHANNEL, (int)d);
}

static void sendReply(const char* msg) {
    Serial.print(msg);
}

static void processLine(char* p) {
    while (*p == ' ') p++;
    if (!*p) return;

    /* Запрос идентификации: VER? / ID? → UART-GEN,1.0 */
    if (strcmp(p, "VER?") == 0 || strcmp(p, "ID?") == 0) {
        sendReply(FIRMWARE_ID);
        sendReply(",");
        sendReply(FIRMWARE_VER);
        sendReply("\r\n");
        Serial.flush();
        return;
    }
    if (strcmp(p, "?") == 0 || strcmp(p, "STATUS") == 0) {
        char buf[80];
        snprintf(buf, sizeof(buf), "FREQ=%lu DUTY=%u %s\r\n",
                 (unsigned long)s_freq, (unsigned)s_dutyPercent,
                 s_running ? "ON" : "OFF");
        sendReply(buf);
        Serial.flush();
        return;
    }
    if (strcmp(p, "ON") == 0 || strcmp(p, "START") == 0) {
        s_running = true;
        applyOutput();
        sendReply("OK ON\r\n");
        return;
    }
    if (strcmp(p, "OFF") == 0 || strcmp(p, "STOP") == 0) {
        s_running = false;
        applyOutput();
        sendReply("OK OFF\r\n");
        return;
    }
    if (strncmp(p, "FREQ ", 5) == 0) {
        unsigned long v = strtoul(p + 5, nullptr, 0);
        if (v < FREQ_MIN || v > FREQ_MAX) {
            sendReply("ERR FREQ range 1..40000000\r\n");
            return;
        }
        s_freq = (uint32_t)v;
        ledcSetup(LEDC_CHANNEL, s_freq, LEDC_RES_BIT);
        applyOutput();
        sendReply("OK FREQ "); Serial.print(v); sendReply("\r\n");
        return;
    }
    if (strncmp(p, "DUTY ", 5) == 0) {
        unsigned long v = strtoul(p + 5, nullptr, 0);
        if (v > 100) {
            sendReply("ERR DUTY 0..100\r\n");
            return;
        }
        s_dutyPercent = (uint8_t)v;
        applyOutput();
        sendReply("OK DUTY "); Serial.print(v); sendReply("\r\n");
        return;
    }
    if (strcmp(p, "HELP") == 0) {
        sendReply(
            "VER?|ID?    - идентификация (UART-GEN,версия)\r\n"
            "FREQ <Hz>   - частота 1..40000000\r\n"
            "DUTY <0-100> - скважность %\r\n"
            "ON|START    - включить выход\r\n"
            "OFF|STOP    - выключить выход\r\n"
            "?|STATUS    - состояние\r\n"
            "HELP        - эта справка\r\n"
        );
        return;
    }
    sendReply("ERR unknown command (HELP)\r\n");
}

void setup() {
    Serial.begin(115200);
    Serial.onEvent(ARDUINO_HW_CDC_BUS_RESET_EVENT, onUsbBusReset);
    delay(800);  /* USB CDC: дать хосту время увидеть порт и подключиться */
    ledcSetup(LEDC_CHANNEL, s_freq, LEDC_RES_BIT);
    ledcAttachPin(GENERATOR_PIN, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 0);
    s_lineLen = 0;
    Serial.println("\r\nUART Generator (Arduino). Commands: FREQ, DUTY, ON, OFF, VER?");
    Serial.flush();
}

void loop() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r' || s_lineLen >= CMD_LINE_MAX - 1) {
            s_line[s_lineLen] = '\0';
            if (s_lineLen > 0) processLine(s_line);
            s_lineLen = 0;
            if (c != '\n' && c != '\r') s_line[s_lineLen++] = c;
        } else {
            s_line[s_lineLen++] = c;
        }
    }
    delay(1);
}
