# UART Generator

Генератор меандра на **ESP32-C3** с управлением по USB (COM-порт). Частота 1 Гц … 40 МГц, скважность 0–100%. Подходит для тестов UART, таймеров и т.п.

## Оборудование

- **Плата:** ESP32-C3 (например ESP32-C3-DevKitM-1)
- **Выход:** GPIO 5 (LEDC/PWM)
- **Связь:** USB CDC (один кабель — питание и данные)

## Протокол

Текстовые команды по одной на строку, ответы с `\r\n`. Скорость: **115200** бод.

| Команда | Описание |
|--------|----------|
| `VER?` или `ID?` | Идентификация → `UART-GEN,1.0` |
| `FREQ <Hz>` | Частота 1 … 40 000 000 |
| `DUTY <0-100>` | Скважность в % |
| `ON` / `START` | Включить выход |
| `OFF` / `STOP` | Выключить выход |
| `?` / `STATUS` | Текущее состояние |
| `HELP` | Справка по командам |

## Сборка и прошивка (PlatformIO)

```bash
# Сборка
pio run -e esp32c3_arduino

# Прошивка
pio run -e esp32c3_arduino -t upload

# Монитор порта
pio device monitor -e esp32c3_arduino -b 115200
```

Окружение по умолчанию: **esp32c3_arduino** (Arduino + ESP32-C3, USB CDC).

## GUI (управление с ПК)

Приложение на Python: выбор порта, подключение, частота/скважность, вкл/выкл выхода.

```bash
cd gui
pip install -r requirements.txt
python generator_gui.py
```

Подробнее: [gui/README.md](gui/README.md).

## Структура проекта

```
├── src/
│   ├── main_arduino.cpp   # Прошивка Arduino (ESP32-C3)
│   ├── main.cpp           # Заготовка под ESP-IDF
│   └── ...
├── include/
├── gui/                   # GUI и протокол (Python)
│   ├── generator_gui.py
│   ├── protocol.py
│   └── requirements.txt
├── platformio.ini
└── README.md
```

## Лицензия

MIT — см. [LICENSE](LICENSE).
