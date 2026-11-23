"""Задание 3.3.4: реальный обмен с Pico по USB CDC и журнал испытаний.

Это совместимая реализация по образцу check-3-3-3.py из проекта.
Никаких результатов не подставляет: их считывают непосредственно с платы.
"""
import re
import sys
import time
from datetime import datetime
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    raise SystemExit("Не найден pyserial. Проверьте: py -3 -m pip show pyserial")

VENDOR_ID = 0x2E8A
PRODUCT_ID = 0x000A
TASK = "3.3.4"
PROJECT = "331-uart"
LOG_NAME = "device-3-3-4.log"
BACKSPACE = "\x7f"
STEPS = [
    ("\r", 0.5),
    ("help\r", 1.0),
    ("info\r", 1.0),
    ("loop pins\r", 0.5),
    ("baud_shift 0\r", 0.5),
    ("parity none none\r", 0.5),
    ("block irq\r", 0.7),
    ("block poll\r", 0.7),
    ("block irq\r", 0.7),
    ("block poll\r", 0.7),
    ("block xx\r", 0.5),
]


def find_board():
    for port in list_ports.comports():
        if port.vid == VENDOR_ID and port.pid == PRODUCT_ID:
            return port
    return None


def printable(text):
    return text.replace(BACKSPACE, "<BS>").replace("\b", "<BS>").replace("\r", "").replace("\n", "")


def talk(board):
    exchange = []
    with serial.Serial(board.device, timeout=0.05) as port:
        time.sleep(0.2)
        port.reset_input_buffer()
        started = time.monotonic()
        received = ""
        for frame, listen_s in STEPS:
            port.write(frame.encode("ascii"))
            exchange.append((time.monotonic() - started, "-->", printable(frame)))
            deadline = time.monotonic() + listen_s
            while time.monotonic() < deadline:
                received += port.read(256).decode("ascii", "replace")
                while "\n" in received:
                    line, received = received.split("\n", 1)
                    line = printable(line).strip()
                    if line:
                        exchange.append((time.monotonic() - started, "<--", line))
                        print(line)
    return exchange


def write_log(board, exchange):
    with Path(LOG_NAME).open("w", encoding="utf-8", newline="\n") as log:
        log.write("задание: " + TASK + "\n")
        log.write("проект: " + PROJECT + "\n")
        log.write("устройство: %04x:%04x\n" % (board.vid, board.pid))
        log.write("серийный номер: " + str(board.serial_number) + "\n")
        log.write("порт: " + board.device + "\n")
        log.write("начало: " + datetime.now().isoformat(timespec="seconds") + "\n")
        for moment, direction, line in exchange:
            log.write("%8.3f %s %s\n" % (moment, direction, line))
        answers = len(exchange) - len(STEPS)
        log.write("итог: отправлено кадров %d, принято строк %d\n" % (len(STEPS), answers))


def check_results(exchange):
    # Проверяем реальные ответы, а не только факт создания файла.
    replies = [line for _, direction, line in exchange if direction == "<--"]
    irq = sum(bool(re.fullmatch(r"block irq:\s+64/64\s+match,\s+err:\s+none", line)) for line in replies)
    poll = sum(bool(re.fullmatch(r"block poll:\s+32/64\s+match,\s+err:.*\bOE\b.*", line)) for line in replies)
    usage = "error: usage block irq|poll" in replies
    if irq >= 2 and poll >= 2 and usage:
        print("ПРОВЕРКА ПРОЙДЕНА: IRQ 2/2, POLL 2/2, ошибка аргумента обработана")
        return True
    print(f"ПРОВЕРКА НЕ ПРОЙДЕНА: IRQ {irq}/2, POLL {poll}/2, аргумент: {usage}")
    return False


def main():
    board = find_board()
    if board is None:
        print("Плата не найдена в обычном USB-режиме. Проверьте кабель и прошивку.")
        return 1
    print("Плата на порту " + board.device + ", отправляю команды")
    try:
        exchange = talk(board)
    except serial.SerialException as exc:
        print("Ошибка подключения к порту:", exc)
        print("Закройте другие программы, использующие этот COM-порт.")
        return 1
    write_log(board, exchange)
    print("Обмен записан в " + LOG_NAME)
    return 0 if check_results(exchange) else 2


if __name__ == "__main__":
    sys.exit(main())
