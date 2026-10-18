#pragma once

#include <stdbool.h>

// Кнопки платы. Логика использует идентификаторы, но не знает номера GPIO.
typedef enum
{
    PLATFORM_BUTTON_1,
    PLATFORM_BUTTON_2,
    PLATFORM_BUTTON_COUNT
} platform_button_t;

// Запускает стандартный ввод-вывод и настраивает выводы.
void platform_init(void);

// Устанавливает состояние встроенного светодиода: true — включён.
void platform_led_set(bool on);

// Возвращает true, если указанная кнопка нажата (активный низкий уровень).
bool platform_button_read(platform_button_t button);
