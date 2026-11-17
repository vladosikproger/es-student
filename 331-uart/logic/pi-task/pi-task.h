#pragma once
#include <stdbool.h>
#include <stdint.h>

// Описываем три состояния нашей задачи (как того требует задание)
typedef enum {
    PI_STATE_NOT_STARTED,
    PI_STATE_RUNNING,
    PI_STATE_READY
} pi_state_t;

// Инициализация задачи (вызовем один раз до while(1))
void pi_task_init(void);

// Обработчик задачи (вызовем внутри while(1))
void pi_task_handle(void);

// Попытка запустить расчет. Вернет false, если расчет уже идет.
bool pi_task_start(uint32_t terms);

// Получить результат, затраченное время и текущее состояние
pi_state_t pi_task_get_result(double *result, uint32_t *time_ms);