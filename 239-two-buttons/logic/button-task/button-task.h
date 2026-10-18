#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "platform/platform.h"

// Задача сообщает о подтверждённом нажатии через переданный callback.
typedef void (*button_task_callback_t)(void);

// Инициализирует одну кнопку и назначает действие для её нажатия.
void button_task_init(platform_button_t button, button_task_callback_t on_press);

// Обрабатывает все кнопки платы; вызывается в каждой итерации суперцикла.
void button_task_handle(void);

bool button_task_is_pressed(platform_button_t button);
uint32_t button_task_get_press_count(platform_button_t button);
