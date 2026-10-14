#pragma once
#include <stdint.h>
#include <stdbool.h>

// Максимальное количество секундомеров, которое поддерживает система
#define PROFILING_MAX_STOPWATCHES 8

/*
 * Ограничение архитектуры: 
 * Сумма замеров (sum_us) хранится в 32-битном целом числе (uint32_t).
 * Максимальное значение: 4 294 967 295 мкс.
 * При непрерывной работе одной задачи её сумма переполнится примерно через 71 минуту.
 */

// Структура, которую модуль будет отдавать наружу по запросу терминала
typedef struct {
    const char *name; // Имя секундомера (например, "loop")
    uint32_t count;   // Сколько раз задача была вызвана
    double mean_us;   // Среднее арифметическое время выполнения в микросекундах
    uint32_t max_us;  // Худшее (максимальное) время выполнения в микросекундах
} profiling_result_t;

// Прототипы функций для управления секундомерами
void profiling_stopwatch_init(uint32_t id, const char *name);
void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);