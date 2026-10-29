#pragma once

#include <stdbool.h>

// Запускает стандартный ввод-вывод и настраивает выводы.
void platform_init(void);

// Устанавливает состояние светодиода: true — включён, false — выключен.
void platform_led_set(bool on);

// Возвращает true, если кнопка сейчас нажата.
bool platform_button_read(void);

// Напряжение на входе АЦП GP26, в вольтах.
float platform_voltage_read(void);

// Температура кристалла RP2040 по встроенному датчику, в градусах Цельсия.
float platform_temp_read(void);
