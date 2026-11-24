"""Hardware I2C 4.1.1 checker. Requires pyserial; logs real Pico USB exchange.

Examples:
  py -3 check-4-1-1.py
  py -3 check-4-1-1.py --address 0x77
"""
import argparse
import re
import sys
import time
from datetime import datetime
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    raise SystemExit("Нужен pyserial, который использовался в заданиях 3.3.4/3.3.5")

VID = 0x2E8A
PID = 0x000A
OUT = Path(__file__).with_name("device-4-1-1.log")


def find_pico(port_name=None):
    boards = [p for p in list_ports.comports() if p.vid == VID and p.pid == PID]
    if port_name:
        matches = [p for p in boards if p.device.upper() == port_name.upper()]
        if not matches:
            raise RuntimeError(f"На {port_name} не найдено USB-устройство Raspberry Pi Pico")
        return matches[0]
    if len(boards) != 1:
        raise RuntimeError(f"Найдено плат Pico: {len(boards)}. Укажи --port COM3, если плат несколько")
    return boards[0]


class Session:
    def __init__(self, ser):
        self.ser = ser
        self.t0 = time.monotonic()
        self.buffer = ""
        self.lines = []

    def mark(self, direction, line):
        elapsed = time.monotonic() - self.t0
        self.lines.append(f"{elapsed:.3f} {direction} {line}")
        print(f"{direction} {line}")

    def command(self, name, wanted, timeout=5):
        # With the course USB text protocol, commands are terminated by CR.
        self.ser.reset_input_buffer()
        self.buffer = ""
        self.mark("-->", name)
        self.ser.write((name + "\r").encode("ascii"))
        self.ser.flush()
        deadline = time.monotonic() + timeout
        responses = []
        while time.monotonic() < deadline:
            chunk = self.ser.read(512)
            if not chunk:
                continue
            self.buffer += chunk.decode("utf-8", errors="replace")
            while "\n" in self.buffer:
                raw, self.buffer = self.buffer.split("\n", 1)
                line = raw.replace("\r", "").strip()
                if not line:
                    continue
                self.mark("<--", line)
                responses.append(line)
                if re.fullmatch(wanted, re.sub(r"\s+", " ", line)):
                    return responses
                if line.startswith("error:"):
                    raise RuntimeError("Прибор вернул ошибку: " + line)
        raise TimeoutError("Не получен ожидаемый ответ на: " + name)


def main():
    parser = argparse.ArgumentParser(description="Проверка I2C и лог для задания 4.1.1")
    parser.add_argument("--address", default="0x76", choices=("0x76", "0x77"))
    parser.add_argument("--port", help="Порт Pico, например COM3")
    args = parser.parse_args()
    board = find_pico(args.port)
    serial_number = (board.serial_number or "").strip()
    if not re.fullmatch(r"[0-9A-Fa-f]{8,}", serial_number):
        raise RuntimeError("Не удалось прочитать серийный номер Pico; проверь подключение USB")
    print(f"Pico на порту {board.device}, серийный номер {serial_number}")

    with serial.Serial(board.device, 115200, timeout=0.12, write_timeout=2) as port:
        time.sleep(1.1)
        port.reset_input_buffer()
        session = Session(port)
        # Help must precede reading the ID for the official coursework checker.
        session.command("help", r"i2c_read_reg <address> <register>")
        session.command(f"i2c_read_reg {args.address} 0xD0",
                        rf"i2c: addr {args.address} reg 0xD0 value 0x60")

        # Additional real hardware validation of the write command:
        # BME280 config register 0xF5 can be written while in sleep mode.
        session.command(f"i2c_write_reg {args.address} 0xF5 0x08",
                        rf"i2c: addr {args.address} reg 0xF5 sent 0x08")
        session.command(f"i2c_read_reg {args.address} 0xF5",
                        rf"i2c: addr {args.address} reg 0xF5 value 0x08")
        session.command(f"i2c_write_reg {args.address} 0xF5 0x00",
                        rf"i2c: addr {args.address} reg 0xF5 sent 0x00")

    header = [
        "задание: 4.1.1",
        "проект: 411-i2c-spi",
        "устройство: 2e8a:000a",
        "серийный номер: " + serial_number,
        "дата: " + datetime.now().isoformat(timespec="seconds"),
        "",
    ]
    OUT.write_text("\n".join(header + session.lines) + "\n", encoding="utf-8")
    print("Обмен записан в device-4-1-1.log")
    print("ПРОВЕРКА ПРОЙДЕНА: BME280 ID=0x60, запись/чтение регистра 0xF5")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, TimeoutError, serial.SerialException) as exc:
        print("ПРОВЕРКА НЕ ПРОЙДЕНА:", exc, file=sys.stderr)
        sys.exit(1)
