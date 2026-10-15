#pragma once
#include <stdint.h>
#include <stdbool.h>

// Если рубильник явно не задан, считаем, что профилирование включено (1)
#ifndef PROFILING_ENABLED
#define PROFILING_ENABLED 1
#endif

// Эта структура нужна всегда, чтобы интерфейс программы не сломался
typedef struct {
    const char *name;
    uint32_t count;
    float mean_us;
    uint32_t max_us;
} profiling_result_t;

// Если профилирование ВКЛЮЧЕНО, компилируем настоящие функции
#if PROFILING_ENABLED

void profiling_init(void);
uint32_t profiling_allocate(const char *name);
void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);

// Если профилирование ВЫКЛЮЧЕНО, заменяем функции на пустоту
#else

#define profiling_init() do {} while(0)
#define profiling_allocate(name) (0)
#define profiling_start(id) do {} while(0)
#define profiling_stop(id) do {} while(0)
#define profiling_get(id, result) (false)
#define profiling_reset() do {} while(0)

#endif