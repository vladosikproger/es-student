#include "profiling.h"
#include "pico/stdlib.h"

// итерация около 2 мкс, за секунду их около 500 000;
// ближайшая степень двойки 2^19 = 524 288, среднее помнит около секунды
#define AVG_SHIFT 19

// замер в 1/256 долях микросекунды, чтобы среднее не теряло дробную часть
#define FRACTION_SHIFT 8

static uint32_t previous_us = 0;
static uint32_t max_us = 0;
static uint64_t avg_sum = 0;

void profiling_init(void)
{
    previous_us = time_us_32();
}

void profiling_iteration(void)
{
    // 1. Читаем аппаратные часы
    uint32_t now_us = time_us_32();
    
    // 2. Вычисляем длительность текущей итерации
    uint32_t iteration_us = now_us - previous_us;
    previous_us = now_us;

    // 3. Обновляем максимальное значение
    if (iteration_us > max_us)
    {
        max_us = iteration_us;
    }

    // 4. Обновляем среднее за 1 такт, без float и без знака деления
    // (uint64_t) защищает от переполнения при умножении на 256
    avg_sum = avg_sum - (avg_sum >> AVG_SHIFT) + ((uint64_t)iteration_us << FRACTION_SHIFT);
}

// ------------------------------------------------------------------
// Функции выдачи результатов (Геттеры)
// ------------------------------------------------------------------

float profiling_avg_us(void)
{
    // Здесь float допустим: эта функция вызывается только человеком через терминал.
    // Мы делим аккумулятор на N (>> AVG_SHIFT) и переводим обратно из 1/256 долей в микросекунды (1 << FRACTION_SHIFT).
    return (float)(avg_sum >> AVG_SHIFT) / (1 << FRACTION_SHIFT);
}

uint32_t profiling_max_us(void)
{
    return max_us;
}

void profiling_reset_max(void)
{
    max_us = 0;
}