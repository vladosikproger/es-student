#pragma once

#include <stdint.h>
#include <stdbool.h>



void button_task_init(void);
void button_task_handle(void);

// Функции-геттеры по ТЗ
bool button_task_is_pressed(void);
uint32_t button_task_get_press_count(void);