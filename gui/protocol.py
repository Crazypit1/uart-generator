"""
Протокол UART-генератора: формирование команд и разбор ответов.
Вынесено для тестирования без GUI и без serial.
"""
import re

try:
    import serial
except ImportError:
    serial = None

BAUD = 115200
FREQ_MIN, FREQ_MAX = 1, 40_000_000
DUTY_MIN, DUTY_MAX = 0, 100

# Идентификация устройства: запрос VER? → ответ должен содержать этот префикс
DEVICE_ID_PREFIX = "UART-GEN"
STATUS_RE = re.compile(r"FREQ=(\d+)\s+DUTY=(\d+)\s+(ON|OFF)")


def build_freq_cmd(hz: int) -> str:
    """Команда установки частоты (Гц)."""
    if not (FREQ_MIN <= hz <= FREQ_MAX):
        raise ValueError(f"Частота должна быть {FREQ_MIN}…{FREQ_MAX}")
    return f"FREQ {hz}"


def build_duty_cmd(percent: int) -> str:
    """Команда установки скважности (0–100)."""
    if not (DUTY_MIN <= percent <= DUTY_MAX):
        raise ValueError(f"Скважность должна быть {DUTY_MIN}…{DUTY_MAX}")
    return f"DUTY {percent}"


def build_on_cmd() -> str:
    return "ON"


def build_off_cmd() -> str:
    return "OFF"


def build_status_cmd() -> str:
    return "?"


def build_id_cmd() -> str:
    """Команда запроса идентификации (VER?); ответ должен содержать DEVICE_ID_PREFIX."""
    return "VER?"


def is_our_generator_response(received_text: str) -> bool:
    """Проверяет, что в ответе есть подпись нашего генератора (UART-GEN)."""
    return DEVICE_ID_PREFIX in received_text


def parse_status_line(line: str) -> dict | None:
    """
    Разбор ответа статуса вида 'FREQ=5000 DUTY=30 ON' (в любом месте строки).
    Возвращает {'freq': int, 'duty': int, 'on': bool} или None.
    """
    line = line.strip()
    m = STATUS_RE.search(line)  # search — ответ может быть с мусором до/после
    if not m:
        return None
    return {
        "freq": int(m.group(1)),
        "duty": int(m.group(2)),
        "on": m.group(3) == "ON",
    }


def is_ok_response(line: str) -> bool:
    return line.strip().startswith("OK ")


def is_err_response(line: str) -> bool:
    return line.strip().startswith("ERR ")


def probe_generator_on_port(
    port: str,
    baud: int = BAUD,
    timeout: float = 5.0,
    boot_delay: float = 2.5,
) -> bool:
    """
    Проверяет, отвечает ли на порту наш UART-генератор: шлём VER?, по ответу
    определяем устройство (наличие DEVICE_ID_PREFIX в ответе).
    """
    if serial is None:
        return False
    import time
    try:
        ser = serial.Serial(port, baud, timeout=0.15, write_timeout=2.0)
        try:
            ser.dtr = False
            ser.rts = False
            time.sleep(boot_delay)
            ser.reset_input_buffer()
            cmd = (build_id_cmd() + "\n").encode("ascii")
            ser.write(cmd)
            ser.flush()
            time.sleep(0.15)
            ser.write(cmd)
            ser.flush()
            deadline = time.monotonic() + (timeout - boot_delay)
            buf = ""
            while time.monotonic() < deadline:
                chunk = ser.read(512).decode("ascii", errors="replace")
                if chunk:
                    buf += chunk
                else:
                    time.sleep(0.03)
                    continue
                if is_our_generator_response(buf):
                    return True
                if len(buf) > 2048:
                    buf = buf[-1024:]
            return False
        finally:
            ser.close()
    except Exception:
        return False


def probe_generator_debug(port: str, baud: int = BAUD, wait_after_open: float = 2.5) -> tuple[bool, str]:
    """
    Открывает порт, ждёт, шлёт VER?, читает 1.5 сек. Возвращает (найден_генератор, сырой_ответ).
    """
    if serial is None:
        return False, "pyserial не установлен"
    import time
    try:
        ser = serial.Serial(port, baud, timeout=0.2, write_timeout=2.0)
        try:
            ser.dtr = False
            ser.rts = False
            time.sleep(wait_after_open)
            ser.reset_input_buffer()
            ser.write((build_id_cmd() + "\n").encode("ascii"))
            ser.flush()
            time.sleep(0.2)
            ser.write((build_id_cmd() + "\n").encode("ascii"))
            ser.flush()
            deadline = time.monotonic() + 1.5
            buf = ""
            while time.monotonic() < deadline:
                chunk = ser.read(512).decode("ascii", errors="replace")
                if chunk:
                    buf += chunk
                else:
                    time.sleep(0.05)
            found = is_our_generator_response(buf)
            preview = repr(buf[:500]) if len(buf) > 500 else repr(buf)
            return found, preview
        finally:
            ser.close()
    except Exception as e:
        return False, f"Ошибка: {e}"
