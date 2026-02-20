#!/usr/bin/env python3
"""
GUI для управления UART-генератором (ESP32).
Подключение по COM-порту, команды: FREQ, DUTY, ON, OFF, ?
"""
import customtkinter as ctk
import serial
import serial.tools.list_ports
import threading
import queue
import re

from protocol import (
    BAUD,
    FREQ_MIN,
    FREQ_MAX,
    build_freq_cmd,
    build_duty_cmd,
    build_on_cmd,
    build_off_cmd,
    build_status_cmd,
    parse_status_line,
    probe_generator_on_port,
    probe_generator_debug,
)


class GeneratorApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.ser: serial.Serial | None = None
        self.response_queue: queue.Queue[str] = queue.Queue()
        self.after_id = None
        self.auto_refresh_id = None  # таймер автообновления списка портов при отключении

        self.title("UART Generator — управление")
        self.geometry("520x420")
        self.minsize(400, 350)

        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        self._build_ui()

    def _build_ui(self):
        # --- Подключение ---
        conn_frame = ctk.CTkFrame(self, fg_color="transparent")
        conn_frame.pack(fill="x", padx=12, pady=(12, 6))

        ctk.CTkLabel(conn_frame, text="Порт:", width=50).pack(side="left", padx=(0, 4))
        self.port_var = ctk.StringVar(value="")
        self.port_menu = ctk.CTkOptionMenu(
            conn_frame,
            variable=self.port_var,
            values=["— выберите порт —"],
            width=180,
            command=self._on_port_select,
        )
        self.port_menu.pack(side="left", padx=2)

        self.btn_refresh = ctk.CTkButton(
            conn_frame, text="Обновить", width=80, command=self._start_port_scan
        )
        self.btn_refresh.pack(side="left", padx=6)

        self.btn_connect = ctk.CTkButton(
            conn_frame, text="Подключить", width=100, command=self._toggle_connect
        )
        self.btn_connect.pack(side="left", padx=4)

        self.label_status = ctk.CTkLabel(
            conn_frame, text="Не подключено", text_color="gray"
        )
        self.label_status.pack(side="left", padx=12)

        # --- Управление генератором ---
        ctrl_frame = ctk.CTkFrame(self, fg_color="transparent")
        ctrl_frame.pack(fill="x", padx=12, pady=12)

        # Частота
        freq_row = ctk.CTkFrame(ctrl_frame, fg_color="transparent")
        freq_row.pack(fill="x", pady=4)
        ctk.CTkLabel(freq_row, text="Частота (Гц):", width=120).pack(side="left", padx=(0, 8))
        self.freq_entry = ctk.CTkEntry(
            freq_row, width=140, placeholder_text="1000"
        )
        self.freq_entry.insert(0, "1000")
        self.freq_entry.pack(side="left", padx=2)
        ctk.CTkLabel(freq_row, text="1 … 40 000 000", text_color="gray").pack(
            side="left", padx=8
        )
        self.btn_freq = ctk.CTkButton(
            freq_row, text="Применить", width=90, command=self._send_freq
        )
        self.btn_freq.pack(side="left", padx=8)

        # Скважность
        duty_row = ctk.CTkFrame(ctrl_frame, fg_color="transparent")
        duty_row.pack(fill="x", pady=4)
        ctk.CTkLabel(duty_row, text="Скважность (%):", width=120).pack(
            side="left", padx=(0, 8)
        )
        self.duty_slider = ctk.CTkSlider(
            duty_row, from_=0, to=100, number_of_steps=100, width=200
        )
        self.duty_slider.set(50)
        self.duty_slider.pack(side="left", padx=4)
        self.duty_label = ctk.CTkLabel(duty_row, text="50 %", width=45)
        self.duty_label.pack(side="left", padx=4)
        self.duty_slider.configure(command=self._on_duty_slide)
        self.btn_duty = ctk.CTkButton(
            duty_row, text="Применить", width=90, command=self._send_duty
        )
        self.btn_duty.pack(side="left", padx=8)

        # Вкл/выкл
        run_row = ctk.CTkFrame(ctrl_frame, fg_color="transparent")
        run_row.pack(fill="x", pady=12)
        self.btn_start = ctk.CTkButton(
            run_row,
            text="Включить выход",
            width=140,
            fg_color="green",
            hover_color="darkgreen",
            command=self._send_on,
        )
        self.btn_start.pack(side="left", padx=(0, 8))
        self.btn_stop = ctk.CTkButton(
            run_row,
            text="Выключить выход",
            width=140,
            fg_color="firebrick",
            hover_color="darkred",
            command=self._send_off,
        )
        self.btn_stop.pack(side="left", padx=2)
        self.btn_status = ctk.CTkButton(
            run_row, text="Запросить статус", width=120, command=self._send_status
        )
        self.btn_status.pack(side="left", padx=12)

        # --- Состояние (ответ устройства) ---
        state_frame = ctk.CTkFrame(self, fg_color="transparent")
        state_frame.pack(fill="x", padx=12, pady=4)
        ctk.CTkLabel(state_frame, text="Состояние:").pack(anchor="w")
        self.state_label = ctk.CTkLabel(
            state_frame, text="—", text_color="gray", anchor="w"
        )
        self.state_label.pack(fill="x", pady=2)

        # --- Лог обмена ---
        log_frame = ctk.CTkFrame(self, fg_color="transparent")
        log_frame.pack(fill="both", expand=True, padx=12, pady=8)
        ctk.CTkLabel(log_frame, text="Лог обмена:").pack(anchor="w")
        self.log_text = ctk.CTkTextbox(
            log_frame, height=120, font=ctk.CTkFont(family="Consolas", size=12)
        )
        self.log_text.pack(fill="both", expand=True, pady=4)

        self._set_controls_connected(False)
        self._refresh_ports()

    def _start_port_scan(self):
        """Запуск сканирования портов в фоне (только порты с подключённым генератором)."""
        if getattr(self, "_scan_in_progress", False):
            return
        self._scan_in_progress = True
        self.btn_refresh.configure(state="disabled", text="Сканирование...")
        self.label_status.configure(text="Поиск генераторов на COM-портах...", text_color="gray")
        self.port_menu.configure(values=["— сканирование —"])
        self.port_var.set("— сканирование —")

        def scan():
            all_ports = [p.device for p in serial.tools.list_ports.comports()]
            found = []
            for port in all_ports:
                if probe_generator_on_port(port, baud=BAUD):
                    found.append(port)
            self.after(0, lambda: self._on_scan_done(found, all_ports))

        threading.Thread(target=scan, daemon=True).start()

    def _on_scan_done(self, generator_ports: list, all_ports: list | None = None):
        self._scan_in_progress = False
        self.btn_refresh.configure(state="normal", text="Обновить")
        if not generator_ports:
            names = ["— генераторов не найдено —"]
            self.label_status.configure(text="Генераторы не найдены", text_color="gray")
            if all_ports:
                def run_debug():
                    ok, msg = probe_generator_debug(all_ports[0], baud=BAUD)
                    self.after(0, lambda: self._log(f"Диагностика {all_ports[0]}: {msg}", "warn"))
                threading.Thread(target=run_debug, daemon=True).start()
        else:
            names = generator_ports
            self.label_status.configure(
                text=f"Найдено генераторов: {len(names)}",
                text_color="lime" if names else "gray",
            )
        self.port_menu.configure(values=names)
        if names and names[0] != "— генераторов не найдено —":
            self.port_var.set(names[0])
        else:
            self.port_var.set(names[0] if names else "— генераторов не найдено —")
        # Продолжить автообновление списка, если мы отключены
        self._schedule_auto_refresh_ports()

    def _refresh_ports(self):
        """Вызов при старте: сканирование без блокировки (то же что Обновить)."""
        self._start_port_scan()

    def _schedule_auto_refresh_ports(self):
        """Запланировать обновление списка портов через 3 с, только если не подключены."""
        if self.auto_refresh_id:
            self.after_cancel(self.auto_refresh_id)
            self.auto_refresh_id = None
        if self.ser and self.ser.is_open:
            return
        def do_refresh():
            self.auto_refresh_id = None
            if self.ser and self.ser.is_open:
                return
            if not getattr(self, "_scan_in_progress", False):
                self._start_port_scan()
        self.auto_refresh_id = self.after(3000, do_refresh)

    def _cancel_auto_refresh_ports(self):
        if self.auto_refresh_id:
            self.after_cancel(self.auto_refresh_id)
            self.auto_refresh_id = None

    def _on_serial_lost(self):
        """Порт отключился (USB вытащили и т.п.). Отключаемся и обновляем список."""
        self._disconnect()
        self._log("Устройство отключено. Список портов обновлён.", "warn")
        self._start_port_scan()

    def _on_port_select(self, _=None):
        pass

    def _toggle_connect(self):
        if self.ser and self.ser.is_open:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        self._cancel_auto_refresh_ports()
        port = self.port_var.get().strip()
        if not port or port.startswith("—"):
            self._log("Выберите COM-порт.", "warn")
            return
        try:
            self.ser = serial.Serial(port, BAUD, timeout=0.1, write_timeout=1.0)
            self._set_controls_connected(True)
            self.label_status.configure(text=f"Подключено: {port}", text_color="lime")
            self._log(f"Подключено к {port} @ {BAUD}")
            self._start_read_loop()
        except Exception as e:
            self._log(f"Ошибка: {e}", "err")
            self.label_status.configure(text="Ошибка подключения", text_color="red")
            self._schedule_auto_refresh_ports()

    def _disconnect(self):
        if self.after_id:
            self.after_cancel(self.after_id)
            self.after_id = None
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None
        self._set_controls_connected(False)
        self.label_status.configure(text="Не подключено", text_color="gray")
        self.state_label.configure(text="—", text_color="gray")
        self._log("Отключено.")
        self._schedule_auto_refresh_ports()

    def _set_controls_connected(self, connected: bool):
        state = "normal" if connected else "disabled"
        for w in (
            self.btn_freq,
            self.btn_duty,
            self.btn_start,
            self.btn_stop,
            self.btn_status,
        ):
            w.configure(state=state)
        self.port_menu.configure(state="disabled" if connected else "normal")
        self.btn_connect.configure(text="Отключить" if connected else "Подключить")

    def _send(self, cmd: str) -> bool:
        if not self.ser or not self.ser.is_open:
            return False
        try:
            line = (cmd.strip() + "\n").encode("ascii")
            self.ser.write(line)
            self._log(f"→ {cmd.strip()}", "tx")
            return True
        except Exception as e:
            self._log(f"Ошибка отправки: {e}", "err")
            return False

    def _send_freq(self):
        try:
            v = int(self.freq_entry.get().strip())
            self._send(build_freq_cmd(v))
        except ValueError:
            self._log("Введите целое число для частоты.", "warn")

    def _send_duty(self):
        d = int(self.duty_slider.get())
        self._send(build_duty_cmd(d))

    def _send_on(self):
        self._send(build_on_cmd())

    def _send_off(self):
        self._send(build_off_cmd())

    def _send_status(self):
        self._send(build_status_cmd())

    def _on_duty_slide(self, value: float):
        self.duty_label.configure(text=f"{int(value)} %")

    def _log(self, msg: str, kind: str = "info"):
        self.log_text.insert("end", msg + "\n")
        self.log_text.see("end")
        if kind == "err":
            self.state_label.configure(text=msg, text_color="red")
        elif kind == "warn":
            self.state_label.configure(text=msg, text_color="orange")

    def _on_response(self, line: str):
        line = line.strip()
        if not line:
            return
        self._log(f"← {line}", "rx")
        st = parse_status_line(line)
        if st:
            self.after(
                0,
                lambda: self._update_state_from_status(
                    str(st["freq"]), str(st["duty"]), "ON" if st["on"] else "OFF"
                ),
            )
        elif line.startswith("OK ") or line.startswith("ERR "):
            self.state_label.configure(text=line, text_color="orange" if line.startswith("ERR") else "lime")

    def _update_state_from_status(self, freq: str, duty: str, on_off: str):
        self.state_label.configure(
            text=f"Частота {freq} Гц, скважность {duty} %, выход {on_off}",
            text_color="lime",
        )
        try:
            self.freq_entry.delete(0, "end")
            self.freq_entry.insert(0, freq)
        except Exception:
            pass
        try:
            self.duty_slider.set(int(duty))
            self.duty_label.configure(text=f"{duty} %")
        except Exception:
            pass

    def _read_loop(self):
        buf = b""
        while self.ser and self.ser.is_open:
            try:
                chunk = self.ser.read(256)
                if not chunk:
                    continue
                buf += chunk
                while b"\n" in buf or b"\r" in buf:
                    idx = buf.find(b"\n")
                    if idx < 0:
                        idx = buf.find(b"\r")
                    if idx < 0:
                        break
                    line = buf[:idx].decode("ascii", errors="replace").strip()
                    buf = buf[idx + 1 :].lstrip(b"\r\n")
                    if line:
                        self.response_queue.put(line)
            except (serial.SerialException, OSError):
                self.after(0, self._on_serial_lost)
                break
            except Exception:
                self.after(0, self._on_serial_lost)
                break

    def _start_read_loop(self):
        self._read_thread = threading.Thread(target=self._read_loop, daemon=True)
        self._read_thread.start()
        self._poll_responses()

    def _poll_responses(self):
        try:
            while True:
                line = self.response_queue.get_nowait()
                self._on_response(line)
        except queue.Empty:
            pass
        if self.ser and self.ser.is_open:
            self.after_id = self.after(50, self._poll_responses)

    def on_closing(self):
        if self.after_id:
            self.after_cancel(self.after_id)
        self._cancel_auto_refresh_ports()
        self._disconnect()
        self.destroy()


def main():
    app = GeneratorApp()
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()


if __name__ == "__main__":
    main()
