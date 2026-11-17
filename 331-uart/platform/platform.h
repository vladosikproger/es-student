#pragma once

#include <stdbool.h>

// Запускает стандартный ввод-вывод и настраивает выводы.
void platform_init(void);

// Устанавливает состояние светодиода: true — включён, false — выключен.
void platform_led_set(bool on);

// Возвращает true, если кнопка сейчас нажата.
bool platform_button_read(void);

// Напряжение узла RC на GP26 (ADC0), в вольтах.
float platform_voltage_read(void);

typedef enum {
    PLATFORM_LINE_PUSH_PULL,
    PLATFORM_LINE_OPEN_DRAIN,
} platform_line_mode_t;

// В открытом стоке high означает «отпустить», а не «выдать 3,3 В».
void platform_line_set(platform_line_mode_t mode, bool high);
