#include "pi-task.h"
#include "pico/stdlib.h"

// 1. Выбираем порцию. 
// Один член ряда стоит ~4.7 мкс. Нам нужно уложиться в 0.5 мс (500 мкс).
// 100 членов * 4.7 мкс = 470 мкс. Идеально подходит!
#define PI_TERMS_PER_CALL 100 

// 2. Скрытая память задачи (наши "закладки")
static pi_state_t state;
static uint32_t pi_k;
static uint32_t pi_n;
static double pi_sum;
static double pi_sign;

static uint64_t start_time_us;
static uint64_t end_time_us;

void pi_task_init(void)
{
    state = PI_STATE_NOT_STARTED;
}

bool pi_task_start(uint32_t terms)
{
    // Защита: отклоняем повторный запуск, если уже считаем
    if (state == PI_STATE_RUNNING) {
        return false; 
    }

    // Устанавливаем начальные значения для математики
    pi_n = terms;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    
    // Запускаем секундомер и меняем состояние
    start_time_us = time_us_64();
    state = PI_STATE_RUNNING;
    
    return true;
}

void pi_task_handle(void)
{
    // Если не считаем — мгновенно выходим, не тратим время процессора
    if (state != PI_STATE_RUNNING) {
        return;
    }

    // Высчитываем, на каком шаге мы должны остановиться В ЭТОЙ ПОРЦИИ
    uint32_t end = pi_k + PI_TERMS_PER_CALL;
    
    // ВАЖНО: Защита от переполнения (баг из задания на 4 294 967 295 членов)
    // Если после сложения число стало меньше, значит переменная переполнилась по кругу.
    if (end < pi_k || end > pi_n) {
        end = pi_n;
    }

    // Считаем ровно одну порцию (от pi_k до end)
    for (; pi_k < end; pi_k++) {
        pi_sum += pi_sign / (2.0 * pi_k + 1.0);
        pi_sign = -pi_sign;
    }

    // Если дошли до финального шага — завершаем расчет
    if (pi_k == pi_n) {
        end_time_us = time_us_64(); // Останавливаем секундомер
        state = PI_STATE_READY;
    }
}

pi_state_t pi_task_get_result(double *result, uint32_t *time_ms)
{
    if (state == PI_STATE_READY) {
        *result = 4.0 * pi_sum;
        // Переводим микросекунды в миллисекунды для вывода
        *time_ms = (end_time_us - start_time_us) / 1000; 
    }
    return state;
}