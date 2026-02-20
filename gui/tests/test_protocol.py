"""Тесты протокола (команды и разбор ответов)."""
import pytest
from protocol import (
    probe_generator_on_port,
    build_id_cmd,
    is_our_generator_response,
    build_freq_cmd,
    build_duty_cmd,
    build_on_cmd,
    build_off_cmd,
    build_status_cmd,
    parse_status_line,
    is_ok_response,
    is_err_response,
    FREQ_MIN,
    FREQ_MAX,
    DUTY_MIN,
    DUTY_MAX,
)


class TestBuildCommands:
    def test_freq_valid(self):
        assert build_freq_cmd(1) == "FREQ 1"
        assert build_freq_cmd(1000) == "FREQ 1000"
        assert build_freq_cmd(40_000_000) == "FREQ 40000000"

    def test_freq_invalid(self):
        with pytest.raises(ValueError):
            build_freq_cmd(0)
        with pytest.raises(ValueError):
            build_freq_cmd(40_000_001)
        with pytest.raises(ValueError):
            build_freq_cmd(-1)

    def test_duty_valid(self):
        assert build_duty_cmd(0) == "DUTY 0"
        assert build_duty_cmd(50) == "DUTY 50"
        assert build_duty_cmd(100) == "DUTY 100"

    def test_duty_invalid(self):
        with pytest.raises(ValueError):
            build_duty_cmd(101)
        with pytest.raises(ValueError):
            build_duty_cmd(-1)

    def test_on_off_status(self):
        assert build_on_cmd() == "ON"
        assert build_off_cmd() == "OFF"
        assert build_status_cmd() == "?"
        assert build_id_cmd() == "VER?"


class TestDeviceId:
    def test_id_response_recognized(self):
        assert is_our_generator_response("UART-GEN,1.0\r\n") is True
        assert is_our_generator_response("junk UART-GEN,1.0 more") is True
        assert is_our_generator_response("UART-GEN") is True
        assert is_our_generator_response("UART-GEN,2.0") is True

    def test_id_response_rejected(self):
        assert is_our_generator_response("") is False
        assert is_our_generator_response("FREQ=1000 DUTY=50 OFF") is False
        assert is_our_generator_response("OK VER?") is False
        assert is_our_generator_response("other device") is False


class TestParseStatus:
    def test_valid_status(self):
        assert parse_status_line("FREQ=5000 DUTY=30 ON") == {
            "freq": 5000,
            "duty": 30,
            "on": True,
        }
        assert parse_status_line("FREQ=1 DUTY=0 OFF") == {
            "freq": 1,
            "duty": 0,
            "on": False,
        }
        assert parse_status_line("  FREQ=100 DUTY=100 ON  ") == {
            "freq": 100,
            "duty": 100,
            "on": True,
        }

    def test_invalid_status(self):
        assert parse_status_line("") is None
        assert parse_status_line("OK FREQ 1000") is None
        assert parse_status_line("FREQ=100") is None
        assert parse_status_line("invalid") is None


class TestResponseType:
    def test_ok(self):
        assert is_ok_response("OK ON") is True
        assert is_ok_response("OK FREQ 1000") is True
        assert is_ok_response("  OK OFF  ") is True
        assert is_ok_response("ERR FREQ") is False

    def test_err(self):
        assert is_err_response("ERR FREQ") is True
        assert is_err_response("ERR DUTY 0..100") is True
        assert is_err_response("  ERR unknown  ") is True
        assert is_err_response("OK ON") is False


class TestProbePort:
    """Проверка порта на наличие генератора (без реального устройства — только отрицательный результат)."""

    def test_nonexistent_port_returns_false(self):
        assert probe_generator_on_port("COM__NO_SUCH_PORT__", timeout=0.1) is False

    def test_empty_port_name_fails(self):
        # Пустое имя или несуществующий порт — не крашится, возвращает False
        assert probe_generator_on_port("", timeout=0.05) is False
