#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "platform/platform.h"

typedef void (*button_task_callback_t)(void);

// Инициализирует одну из двух кнопок.
// Если on_long_press != NULL, короткое нажатие подтверждается при отпускании,
// а on_long_press вызывается один раз после удержания в течение 1 секунды.
// Если on_long_press == NULL, on_press вызывается после подтверждения нажатия.
void button_task_init(
    platform_button_t button,
    button_task_callback_t on_press,
    button_task_callback_t on_long_press
);

// Обрабатывает обе кнопки; вызывается из главного суперцикла.
void button_task_handle(void);

bool button_task_is_pressed(platform_button_t button);
uint32_t button_task_get_press_count(platform_button_t button);
