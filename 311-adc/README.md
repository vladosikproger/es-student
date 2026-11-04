[![2.3.3 Кнопка без ожидания](https://github.com/vladosikproger/es-student/actions/workflows/check-2-3-3.yml/badge.svg)](https://github.com/vladosikproger/es-student/actions/workflows/check-2-3-3.yml)
[![2.3.7 Границы модулей](https://github.com/vladosikproger/es-student/actions/workflows/check-2-3-7.yml/badge.svg)](https://github.com/vladosikproger/es-student/actions/workflows/check-2-3-7.yml)
[![2.3.8 Приёмка прибора](https://github.com/vladosikproger/es-student/actions/workflows/check-2-3-8.yml/badge.svg)](https://github.com/vladosikproger/es-student/actions/workflows/check-2-3-8.yml)
[![3.1.1 Измерение напряжения](https://github.com/vladosikproger/es-student/actions/workflows/check-3-1-1.yml/badge.svg)](https://github.com/vladosikproger/es-student/actions/workflows/check-3-1-1.yml)
[![3.1.2 Измерение температуры](https://github.com/vladosikproger/es-student/actions/workflows/check-3-1-2.yml/badge.svg)](https://github.com/vladosikproger/es-student/actions/workflows/check-3-1-2.yml)
[![3.1.3 Телеметрия измерений](https://github.com/vladosikproger/es-student/actions/workflows/check-3-1-3.yml/badge.svg)](https://github.com/vladosikproger/es-student/actions/workflows/check-3-1-3.yml)
[![3.1.5 Исследование RC-цепи](https://github.com/vladosikproger/es-student/actions/workflows/check-3-1-5.yml/badge.svg)](https://github.com/vladosikproger/es-student/actions/workflows/check-3-1-5.yml)

# п3.1.5 — RC-цепь и две кнопки

Плата `311-adc` управляет RC-цепью через GP22, измеряет напряжение конденсатора на GP26 и сохраняет телеметрию через Python-скрипт `rc.py`.

В этой версии сохранено управление двумя кнопками прежнего проекта:

- первая кнопка — GP15, короткое нажатие переключает LED OFF/ON/BLINK, длительное включает BLINK;
- вторая кнопка — GP14, переключает период мигания 1000/250/500 мс.

Подробная схема всей макетной платы с обеими кнопками: `ПОЛНАЯ_СБОРКА_С_ДВУМЯ_КНОПКАМИ.md`.

Инструкция запуска: `ИНСТРУКЦИЯ_п3.1.5_две_кнопки.md`.

Скрипт эксперимента для резистора 1 кОм и конденсатора 1000 мкФ:

```powershell
python rc.py --r 1 --c 1000
```


## Управление RC-цепью (п3.1.5)

- `charge_on` — установить GP22 в HIGH (около 3,3 В), начинается зарядка конденсатора.
- `charge_off` — установить GP22 в LOW (около 0 В), начинается разрядка через резистор.
