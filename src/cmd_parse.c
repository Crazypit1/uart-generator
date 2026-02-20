/**
 * Парсер команд: FREQ, DUTY, ON, OFF, ?, STATUS, HELP.
 * Без зависимостей от ESP-IDF.
 */
#include "cmd_parse.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void trim(char *line, size_t *len)
{
    while (*len > 0 && (line[*len - 1] == '\r' || line[*len - 1] == '\n' || line[*len - 1] == ' '))
        (*len)--;
    line[*len] = '\0';
    char *p = line;
    while (*p == ' ')
        p++;
    if (p != line) {
        size_t n = strlen(p);
        memmove(line, p, n + 1);
        *len = n;
    }
}

int cmd_parse(const char *line, cmd_result_t *out)
{
    if (!line || !out)
        return -1;

    char buf[64];
    size_t len = 0;
    while (line[len] && len < sizeof(buf) - 1 && line[len] != '\r' && line[len] != '\n') {
        buf[len] = line[len];
        len++;
    }
    buf[len] = '\0';
    trim(buf, &len);
    if (len == 0) {
        out->type = CMD_NONE;
        return 0;
    }

    const char *p = buf;

    if (strcmp(p, "?") == 0 || strcmp(p, "STATUS") == 0) {
        out->type = CMD_STATUS;
        return 0;
    }
    if (strcmp(p, "ON") == 0 || strcmp(p, "START") == 0) {
        out->type = CMD_ON;
        return 0;
    }
    if (strcmp(p, "OFF") == 0 || strcmp(p, "STOP") == 0) {
        out->type = CMD_OFF;
        return 0;
    }
    if (strcmp(p, "HELP") == 0) {
        out->type = CMD_HELP;
        return 0;
    }

    if (strncmp(p, "FREQ ", 5) == 0) {
        char *endptr;
        unsigned long val = strtoul((char *)(p + 5), &endptr, 0);
        while (*endptr == ' ') endptr++;
        if (*endptr != '\0')
            return -1;
        if (val < CMD_FREQ_MIN || val > CMD_FREQ_MAX)
            return -2;
        out->type = CMD_FREQ;
        out->freq = (uint32_t)val;
        return 0;
    }

    if (strncmp(p, "DUTY ", 5) == 0) {
        char *endptr;
        unsigned long val = strtoul((char *)(p + 5), &endptr, 0);
        while (*endptr == ' ') endptr++;
        if (*endptr != '\0')
            return -1;
        if (val > CMD_DUTY_MAX)
            return -2;
        out->type = CMD_DUTY;
        out->duty = (uint8_t)val;
        return 0;
    }

    return -1;
}
