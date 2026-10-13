#pragma once

#include <stdint.h>
#include <stdbool.h>

// 4 состояния автомата кнопки по ТЗ
typedef enum {
    BUTTON_STATE_RELEASED,   // Стабильно отпущена
    BUTTON_STATE_PRESSING,   // Переходный процесс: возможно нажатие, ждем 20 мс
    BUTTON_STATE_PRESSED,    // Стабильно нажата
    BUTTON_STATE_RELEASING   // Переходный процесс: возможно отпускание, ждем 20 мс
} button_state_t;

void button_task_init(void);
void button_task_handle(void);

// Функции-геттеры по ТЗ
bool button_task_is_pressed(void);
uint32_t button_task_get_press_count(void);