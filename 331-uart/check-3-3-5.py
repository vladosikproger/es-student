"""Real Pico 3.3.5 UART oscilloscope test, log and PNG. Requires only pyserial.

Capture: ADC0/GP26 physically connected to UART0 TX / GP0.
No prerecorded samples and no matplotlib dependency.
"""
import re
import struct
import sys
import time
import zlib
from datetime import datetime
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    raise SystemExit("Требуется pyserial (уже устанавливался для задания 3.3.4)")

HERE = Path(__file__).resolve().parent
LOG_PATH = HERE / "device-3-3-5.log"
PNG_PATH = HERE / "scope-3-3-5.png"
SAMPLE_HZ = 500_000
SAMPLE_COUNT = 4096
BOARD_VID, BOARD_PID = 0x2E8A, 0x000A
HEADER_RE = re.compile(
    r"scope: (\d+) samples, (\d+) Hz, (\d+) baud, uart1 (.*?) err: (\S+)"
)


def board_port():
    return next((p for p in list_ports.comports()
                 if p.vid == BOARD_VID and p.pid == BOARD_PID), None)


class Session:
    def __init__(self, port):
        self.serial = port
        self.rx = ""
        self.start = time.monotonic()
        self.log = []

    def send(self, command):
        self.serial.write((command + "\r").encode("ascii"))
        self.log.append((time.monotonic() - self.start, "-->", command))
        print("ОТПРАВЛЕНО:", command)

    def read_until(self, done, timeout=8, collect=False):
        lines = []
        until = time.monotonic() + timeout
        while time.monotonic() < until:
            piece = self.serial.read(4096).decode("ascii", "replace")
            self.rx += piece
            while "\n" in self.rx:
                line, self.rx = self.rx.split("\n", 1)
                line = line.replace("\r", "").replace("\x7f", "").strip()
                if not line:
                    continue
                self.log.append((time.monotonic() - self.start, "<--", line))
                lines.append(line)
                if not (collect and line[:1].isdigit()):
                    print(line)
                if done(line):
                    return lines
                if line.startswith("error:"):
                    raise RuntimeError("Плата вернула ошибку: " + line)
        raise TimeoutError("Нет ожидаемого ответа от Pico за " + str(timeout) + " с")


def sample_lines(lines):
    out = []
    for line in lines:
        words = line.split()
        if len(words) == 16 and all(s.isdecimal() for s in words):
            values = [int(s) for s in words]
            if not all(0 <= v <= 4095 for v in values):
                raise ValueError("Недопустимое значение ADC")
            out.extend(values)
    return out


def decode_uart(samples, baud):
    """Detect first falling edge, sample middle of 8 data bits and stop bit."""
    if len(samples) < 20 or max(samples) - min(samples) < 800:
        raise ValueError("На GP26 не видно перепадов напряжения: проверь провод GP0->GP26")
    threshold = (max(samples) + min(samples)) / 2
    edge = next((i for i in range(2, len(samples))
                 if samples[i-1] >= threshold and samples[i] < threshold), None)
    if edge is None:
        raise ValueError("Не найден спад старт-бита")
    period = SAMPLE_HZ / baud

    def bit(midpoint):
        index = round(edge + midpoint * period)
        if index >= len(samples):
            raise ValueError("Бит не помещается в захват")
        return int(samples[index] > threshold)

    if bit(.5) != 0 or bit(9.5) != 1:
        raise ValueError("Неверный старт-бит или стоп-бит")
    value = sum(bit(i + 1.5) << i for i in range(8))
    return value, edge


# Simple 5x7 font for dependency-free PNG labels.
FONT = {
    '0': ['111','101','101','101','111'],
    '1': ['010','110','010','010','111'],
    '2': ['111','001','111','100','111'],
    '3': ['111','001','111','001','111'],
    '4': ['101','101','111','001','001'],
    '5': ['111','100','111','001','111'],
    '6': ['111','100','111','101','111'],
    '7': ['111','001','010','010','010'],
    '8': ['111','101','111','101','111'],
    '9': ['111','101','111','001','111'],
    'A': ['010','101','111','101','101'],
    'B': ['110','101','110','101','110'],
    'C': ['011','100','100','100','011'],
    'D': ['110','101','101','101','110'],
    'E': ['111','100','110','100','111'],
    'F': ['111','100','110','100','100'],
    'G': ['011','100','101','101','011'],
    'H': ['101','101','111','101','101'],
    'I': ['111','010','010','010','111'],
    'L': ['100','100','100','100','111'],
    'M': ['101','111','111','101','101'],
    'N': ['101','111','111','111','101'],
    'O': ['111','101','101','101','111'],
    'P': ['110','101','110','100','100'],
    'R': ['110','101','110','101','101'],
    'S': ['011','100','010','001','110'],
    'T': ['111','010','010','010','010'],
    'U': ['101','101','101','101','111'],
    'V': ['101','101','101','101','010'],
    'X': ['101','101','010','101','101'],
    'Y': ['101','101','010','010','010'],
    'Z': ['111','001','010','100','111'],
    ':': ['0','1','0','1','0'],
    '.': ['0','0','0','0','1'],
    '-': ['000','000','111','000','000'],
    '/': ['001','001','010','100','100'],
    ' ': ['000','000','000','000','000'],
}


def make_png(traces):
    width, height = 1500, 850
    canvas = bytearray(b'\xff\xff\xff') * (width * height)

    def pixel(x, y, color):
        if 0 <= x < width and 0 <= y < height:
            off = (y * width + x) * 3
            canvas[off:off+3] = bytes(color)

    def line(x0, y0, x1, y1, color):
        dx, dy = abs(x1-x0), abs(y1-y0)
        sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
        err = dx - dy
        while True:
            pixel(x0,y0,color)
            if (x0,y0) == (x1,y1):
                break
            e2 = err*2
            if e2 > -dy:
                err -= dy; x0 += sx
            if e2 < dx:
                err += dx; y0 += sy

    def text(x, y, string, color=(35,50,70), scale=3):
        for ch in string.upper():
            rows = FONT.get(ch, FONT[' '])
            w = max(map(len,rows))
            for yi, row in enumerate(rows):
                for xi, t in enumerate(row):
                    if t == '1':
                        for ox in range(scale):
                            for oy in range(scale):
                                pixel(x + xi*scale + ox, y + yi*scale + oy, color)
            x += (w+1)*scale

    text(95, 19, 'UART TX / ADC 500000 SAMPLES/S', scale=4)
    for panel, baud in enumerate((115200, 9600)):
        samples, edge = traces[baud]
        left, right = 100, width-50
        top, bottom = (100, 380) if panel == 0 else (480, 760)
        text(left, top - 39, str(baud) + ' BAUD / U 0X55', scale=3)
        for frac in (0, .25, .5, .75, 1):
            y = round(bottom - (bottom-top)*frac)
            line(left,y,right,y,(218,224,235))
            if frac in (0,1):
                text(16,y-8,str(int(frac*4095)),scale=2)
        # Show only the UART frame, not the whole 8.2 ms ADC capture.
        pre = 8 if baud == 115200 else 10
        post = round(11.5 * SAMPLE_HZ/baud)
        i0 = max(0, edge-pre)
        i1 = min(len(samples)-1, edge+post)
        if i1 <= i0: raise ValueError("Слишком мало отсчётов для графика")
        ptx = lambda i: left + round((i-i0)*(right-left)/(i1-i0))
        pty = lambda v: bottom - round(v*(bottom-top)/4095)
        for i in range(i0,i1):
            line(ptx(i), pty(samples[i]),ptx(i+1),pty(samples[i+1]),(15,108,183))
        line(left,top,left,bottom,(78,92,111))
        line(left,bottom,right,bottom,(78,92,111))
        for k in range(5):
            j=i0+round((i1-i0)*k/4)
            x=ptx(j)
            line(x,bottom,x,bottom+6,(80,92,111))
            text(x-8,bottom+11,str(round((j-edge)*2)),scale=2)
        text(right-170,bottom+40,'TIME US',scale=2)
    # PNG encoding using only Python's standard library.
    raw = b''.join(b'\x00' + bytes(canvas[y*width*3:(y+1)*width*3]) for y in range(height))

    def chunk(name,data):
        return struct.pack('>I',len(data)) + name + data + struct.pack('>I',zlib.crc32(name+data)&0xffffffff)
    png = (b'\x89PNG\r\n\x1a\n' +
           chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0)) +
           chunk(b'IDAT',zlib.compress(raw,6)) +
           chunk(b'IEND',b''))
    PNG_PATH.write_bytes(png)


def write_log(board, entries):
    with LOG_PATH.open('w', encoding='utf8', newline='\n') as f:
        f.write('задание: 3.3.5\nпроект: 331-uart\n')
        f.write(f'устройство: {board.vid:04x}:{board.pid:04x}\n')
        f.write(f'серийный номер: {board.serial_number}\nпорт: {board.device}\n')
        f.write('начало: '+datetime.now().isoformat(timespec='seconds')+'\n')
        for t,direction,line in entries:
            f.write(f'{t:8.3f} {direction} {line}\n')


def main():
    board = board_port()
    if board is None:
        print('Плата в обычном USB-режиме не найдена. Проверь подключение.')
        return 1
    print('Плата на',board.device)
    traces = {}
    all_ok = False
    with serial.Serial(board.device,timeout=.08,write_timeout=3) as port:
        time.sleep(.3)
        port.reset_input_buffer()
        session = Session(port)
        try:
            for command, expected in (
                ('help', 'scope'),
                ('loop pins', 'loop: pins'),
                ('baud_shift 0', 'baud_shift:'),
                ('parity none none', 'parity: uart0 none, uart1 none'),
            ):
                session.send(command)
                session.read_until(lambda line, e=expected: line.startswith(e))
            for baud in (115200, 9600):
                session.send(f'scope {baud} U')
                lines = session.read_until(lambda l: l == 'scope: end',timeout=35,collect=True)
                headers = [HEADER_RE.fullmatch(l) for l in lines]
                headers = [m for m in headers if m is not None]
                if len(headers)!=1:
                    raise ValueError(f'Ожидался один scope header для {baud}')
                count, rate, returned_baud, received, errors = headers[0].groups()
                if (int(count),int(rate),int(returned_baud))!=(SAMPLE_COUNT,SAMPLE_HZ,baud):
                    raise ValueError('Неверные параметры осциллограммы: '+headers[0].group())
                if received!='U' or errors!='none':
                    raise ValueError('UART1 не принял U без ошибок: '+headers[0].group())
                samples = sample_lines(lines)
                if len(samples)!=SAMPLE_COUNT:
                    raise ValueError(f'{baud}: пришло {len(samples)} отсчётов вместо {SAMPLE_COUNT}')
                symbol,edge = decode_uart(samples, baud)
                print(f'{baud} бод: декодирован 0x{symbol:02X}, старт-бит в отсчёте {edge}')
                if symbol != ord('U'):
                    raise ValueError(f'{baud}: ожидался U (0x55), получено 0x{symbol:02X}')
                traces[baud] = (samples, edge)
            for cmd in ('scope 1200 U','scope 9600 ABCDE','scope'):
                session.send(cmd)
                # We intentionally expect a usage error here.
                session.read_until(lambda l: l == 'error: usage scope <baud> <text>')
            all_ok = True
        except (ValueError,RuntimeError,TimeoutError,serial.SerialException) as exc:
            print('ПРОВЕРКА НЕ ПРОЙДЕНА:',exc)
        finally:
            write_log(board, session.log)
            print('Обмен записан в',LOG_PATH.name)
    if all_ok:
        make_png(traces)
        print('Осциллограммы записаны в',PNG_PATH.name)
        print('ПРОВЕРКА ПРОЙДЕНА: U на 115200 и 9600, ошибки аргументов обработаны')
        return 0
    return 2


if __name__ == '__main__':
    sys.exit(main())
