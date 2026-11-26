"""4.1.3: USB help works while core 1 blinks the built-in LED.

Run from Windows PowerShell: py -3 .\\check-4-1-3.py
The LED is checked visually: it toggles about every 500 ms.
Requires pyserial as in the previous tasks; no additional packages.
"""
import argparse
import re
import sys
import time
from pathlib import Path

repo = Path(__file__).resolve().parent.parent
for candidate in (
    repo / '331-uart' / 'Lib' / 'site-packages',
    repo / '331-uart' / 'Lib',
    Path.home() / 'pico-python-libs',
):
    if candidate.is_dir():
        sys.path.insert(0, str(candidate))
try:
    import serial
    from serial.tools import list_ports
except ImportError:
    raise SystemExit('Не найден pyserial. Используй прежнюю настройку PYTHONPATH для 4.1.2.')

EXPECTED = (
    r'help\s+list of commands',
    r'i2c_write_reg\s+<address> <register> <value>',
    r'i2c_read_reg\s+<address> <register>',
    r'spi_write\s+<cs> <command> <value>',
    r'spi_read\s+<cs> <command> \[pause_us\]',
)


def find_pico(port_name):
    matches = [p for p in list_ports.comports()
               if p.vid == 0x2E8A and p.pid == 0x000A]
    if port_name:
        matches = [p for p in matches if p.device.upper() == port_name.upper()]
    if len(matches) != 1:
        raise RuntimeError(f'Нужна одна Pico с USB CDC, найдено {len(matches)}. '
                           'При необходимости используй --port COM3.')
    return matches[0]


def check_help(port):
    port.reset_input_buffer()
    port.write(b'help\r')
    port.flush()
    found = set()
    deadline = time.monotonic() + 5.0
    buffer = ''
    while time.monotonic() < deadline:
        fragment = port.read(512)
        if fragment:
            buffer += fragment.decode('utf-8', 'replace')
        while '\n' in buffer:
            raw, buffer = buffer.split('\n', 1)
            line = raw.strip().replace('\r', '')
            if not line:
                continue
            print('<--', line)
            for index, pattern in enumerate(EXPECTED):
                if re.fullmatch(pattern, line):
                    found.add(index)
        if len(found) == len(EXPECTED):
            return
    missing = [p for i, p in enumerate(EXPECTED) if i not in found]
    raise RuntimeError('В ответе help не найдены строки: ' + ', '.join(missing))


def main():
    parser = argparse.ArgumentParser(description='Проверка 4.1.3: второе ядро и USB')
    parser.add_argument('--port', help='например COM3')
    args = parser.parse_args()
    board = find_pico(args.port)
    print('Плата:', board.device)
    with serial.Serial(board.device, 115200, timeout=0.15,
                       write_timeout=2) as port:
        time.sleep(1)
        for i in range(3):
            print(f'--> help ({i+1}/3)')
            check_help(port)
            if i != 2:
                time.sleep(0.8)
    print('ПРОВЕРКА USB ПРОЙДЕНА: help отвечает при работе второго ядра.')
    print('ОТДЕЛЬНО ПРОВЕРЬ ГЛАЗАМИ: штатный LED Pico мигает, '
          'меняя состояние раз в ~0,5 секунды.')


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, serial.SerialException) as error:
        print('ПРОВЕРКА НЕ ПРОЙДЕНА:', error, file=sys.stderr)
        sys.exit(1)
