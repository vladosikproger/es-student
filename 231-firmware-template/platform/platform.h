#pragma once

#include <stdbool.h>

// Запускает стандартный ввод-вывод и настраивает выводы.
void platform_init(void);

// Устанавливает состояние светодиода: true — включён, false — выключен.
void platform_led_set(bool on);

// Возвращает true, если кнопка сейчас нажата.
bool platform_button_read(void);