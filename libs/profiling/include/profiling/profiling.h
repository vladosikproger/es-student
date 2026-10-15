#pragma once
#include <stdint.h>
#include <stdbool.h>

// По умолчанию профилирование включено, если рубильник не задан извне
#ifndef PROFILING_ENABLED
#define PROFILING_ENABLED 1
#endif

typedef struct {
    const char *name;
    uint32_t count;
    float mean_us;
    uint32_t max_us;
} profiling_result_t;

#if PROFILING_ENABLED

void profiling_init(void);
uint32_t profiling_allocate(const char *name);

// Функция, которую требуют старые тесты (2.3.5)
void profiling_stopwatch_init(uint32_t id, const char *name);

void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);

#else

// Если профилирование выключено, превращаем вызовы в пустые безопасные макросы
#define profiling_init() do {} while(0)
#define profiling_allocate(name) (0)
#define profiling_stopwatch_init(id, name) do {} while(0)
#define profiling_start(id) do {} while(0)
#define profiling_stop(id) do {} while(0)
#define profiling_get(id, result) (false)
#define profiling_reset() do {} while(0)

#endif