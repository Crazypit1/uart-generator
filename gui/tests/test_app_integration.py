"""Интеграционные тесты GUI: логика без реального serial."""
import pytest

# Импорт после возможной установки зависимостей
pytest.importorskip("customtkinter")

from generator_gui import GeneratorApp, BAUD, FREQ_MIN, FREQ_MAX


class TestGeneratorAppLogic:
    """Тесты, не требующие открытия окна и COM-порта."""

    def test_constants(self):
        assert BAUD == 115200
        assert FREQ_MIN == 1
        assert FREQ_MAX == 40_000_000

    def test_send_when_disconnected_returns_false(self):
        app = GeneratorApp()
        app.withdraw()  # не показывать окно
        assert app.ser is None
        assert app._send("FREQ 1000") is False
        assert app._send("ON") is False
        app.destroy()

    def test_connect_with_invalid_port_does_not_crash(self):
        app = GeneratorApp()
        app.withdraw()
        app.port_var.set("— выберите порт —")
        app._connect()
        assert app.ser is None
        app.port_var.set("")
        app._connect()
        assert app.ser is None
        app.destroy()
