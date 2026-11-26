"""4.1.2: read an SPI byte and save a REAL USB exchange log.

Four-pin GYBMEP sensor (connected to I2C):
    py -3 check-4-1-2.py --without-sensor

Six-pin BME280 connected to SPI (not applicable to the four-pin board):
    py -3 check-4-1-2.py

Requires pyserial; scans USB Pico VID/PID 2e8a:000a by default.
"""
import argparse
import re
import sys
import time
from datetime import datetime
from pathlib import Path

# Previous lessons sometimes installed pyserial to one of these locations.
# No package reinstall or modification of the repository is required.
_repo = Path(__file__).resolve().parent.parent
for _candidate in (
    _repo / "331-uart" / "Lib" / "site-packages",
    _repo / "331-uart" / "Lib",
    Path.home() / "pico-python-libs",
):
    if _candidate.is_dir():
        sys.path.insert(0, str(_candidate))
try:
    import serial
    from serial.tools import list_ports
except ImportError:
    raise SystemExit(
        "pyserial не найден. Используй ту же настройку PYTHONPATH, "
        "что была в успешной проверке 4.1.1."
    )

VID, PID = 0x2E8A, 0x000A
OUTPUT = Path(__file__).with_name("device-4-1-2.log")


def find_pico(port_name):
    matches = [p for p in list_ports.comports() if p.vid == VID and p.pid == PID]
    if port_name:
        matches = [p for p in matches if p.device.upper() == port_name.upper()]
    if len(matches) != 1:
        raise RuntimeError(
            "Найдена не одна Pico. Укажи --port COM3 (или свой порт). "
            f"Совпадений: {len(matches)}"
        )
    return matches[0]


class Session:
    def __init__(self, port):
        self.port = port
        self.start = time.monotonic()
        self.log = []

    def record(self, direction, line):
        elapsed = time.monotonic() - self.start
        self.log.append(f"{elapsed:.3f} {direction} {line}")
        print(f"{direction} {line}")

    def command(self, text, expected, timeout=5):
        if isinstance(expected, str):
            expected = [expected]
        missing = list(expected)
        self.port.reset_input_buffer()
        self.record("-->", text)
        self.port.write((text + "\r").encode("ascii"))
        self.port.flush()
        buffer = ""
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            fragment = self.port.read(512)
            if not fragment:
                continue
            buffer += fragment.decode("utf-8", errors="replace")
            while "\n" in buffer:
                raw, buffer = buffer.split("\n", 1)
                line = raw.replace("\r", "").strip()
                if not line:
                    continue
                self.record("<--", line)
                normalized = " ".join(line.split())
                missing = [p for p in missing if not re.fullmatch(p, normalized)]
                if not missing:
                    return
                if line.startswith("error:") and not any("error:" in p for p in expected):
                    raise RuntimeError(f"Ошибка устройства при '{text}': {line}")
        raise TimeoutError(f"Нет ожидаемого ответа на '{text}': {missing}")


def main():
    parser = argparse.ArgumentParser(description="Проверка SPI 4.1.2")
    parser.add_argument("--without-sensor", action="store_true",
                        help="Для четырёхконтактного модуля, который остаётся на I2C")
    parser.add_argument("--port", help="USB порт Pico, например COM3")
    args = parser.parse_args()
    board = find_pico(args.port)
    serial_number = (board.serial_number or "").strip()
    if not re.fullmatch(r"[0-9A-Fa-f]{8,}", serial_number):
        raise RuntimeError("Не удалось получить серийный номер USB Pico")
    print(f"Плата на {board.device}, серийный номер {serial_number}")

    with serial.Serial(board.device, 115200, timeout=0.12, write_timeout=2) as uart:
        time.sleep(1)
        uart.reset_input_buffer()
        session = Session(uart)
        session.command("help", [
            r"i2c_write_reg <address> <register> <value>",
            r"i2c_read_reg <address> <register>",
            r"spi_write <cs> <command> <value>",
            r"spi_read <cs> <command> \[pause_us\]",
        ])
        answer = "0x00" if args.without_sensor else "0x60"
        session.command("spi_read 0x00 0xD0",
                        rf"spi: cs 0x00 command 0xD0 value {answer}")
        # These commands also exercise the specified extra argument and write path.
        session.command("spi_read 0x00 0xD0 0x00",
                        rf"spi: cs 0x00 command 0xD0 value {answer}")
        session.command("spi_write 0x00 0x75 0x08",
                        r"spi: cs 0x00 command 0x75 sent 0x08")
        session.command("spi_read 0x02 0xD0",
                        r"error: usage spi_read <cs> <command> \[pause_us\]")
        session.command("spi_write 0x00 0x75",
                        r"error: usage spi_write <cs> <command> <value>")

    header = [
        "задание: 4.1.2",
        "проект: 411-i2c-spi",
        "режим SPI: без датчика" if args.without_sensor else "режим SPI: BME280",
        "устройство: 2e8a:000a",
        "серийный номер: " + serial_number,
        "дата: " + datetime.now().isoformat(timespec="seconds"),
        "",
    ]
    OUTPUT.write_text("\n".join(header + session.log) + "\n", encoding="utf-8")
    if args.without_sensor:
        print("Предупреждение. SPI вернул 0x00 без датчика на шине.")
    print("Обмен записан в device-4-1-2.log")
    print("ПРОВЕРКА ПРОЙДЕНА: SPI0 отвечает на команды USB")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, TimeoutError, serial.SerialException) as exc:
        print("ПРОВЕРКА НЕ ПРОЙДЕНА:", exc, file=sys.stderr)
        sys.exit(1)
