#include "profiling.h"
#include "pico/stdlib.h"
#include <stddef.h> // Для константы NULL

// Скрытая структура памяти для одного секундомера. 
// Про неё знает только этот файл, снаружи она недоступна (инкапсуляция).
typedef struct {
    const char *name;      // Указатель на строку с именем. Если NULL — секундомер свободен.
    uint64_t start_time_us;// Время последнего запуска (считается аппаратно)
    uint32_t sum_us;       // Накопленная сумма всех длительностей
    uint32_t count;        // Счетчик вызовов
    uint32_t max_us;       // Худшее время
} stopwatch_t;

// Создаем физический массив из 8 таких структур. 
// Ключевое слово static прячет этот массив от других файлов.
static stopwatch_t stopwatches[PROFILING_MAX_STOPWATCHES];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    // Защита от дурака: если попросили инициализировать секундомер №10, а их всего 8 — выходим.
    if (id >= PROFILING_MAX_STOPWATCHES) {
        return;
    }

    // Обращаемся к ячейке массива под номером id и обнуляем все стартовые значения
    stopwatches[id].name = name;
    stopwatches[id].start_time_us = 0;
    stopwatches[id].sum_us = 0;
    stopwatches[id].count = 0;
    stopwatches[id].max_us = 0;
}

void profiling_start(uint32_t id)
{
    // Если вышли за границы массива или секундомер не инициализирован (name == NULL) — ничего не делаем.
    if (id >= PROFILING_MAX_STOPWATCHES || stopwatches[id].name == NULL) {
        return;
    }

    // Запрашиваем текущее время у аппаратного таймера кристалла и сохраняем в память секундомера.
    stopwatches[id].start_time_us = time_us_64();
}

void profiling_stop(uint32_t id)
{
    if (id >= PROFILING_MAX_STOPWATCHES || stopwatches[id].name == NULL) {
        return;
    }

    // 1. Узнаем текущее время
    uint64_t now_us = time_us_64();
    
    // 2. Вычисляем дельту (сколько прошло с момента profiling_start)
    // Приводим результат к 32 битам, так как задачи у нас короткие.
    uint32_t elapsed_us = (uint32_t)(now_us - stopwatches[id].start_time_us);

    // 3. Добавляем дельту в общую копилку времени и увеличиваем счетчик вызовов
    stopwatches[id].sum_us += elapsed_us;
    stopwatches[id].count++;

    // 4. Если текущий вызов занял больше времени, чем прошлый рекорд — обновляем максимум
    if (elapsed_us > stopwatches[id].max_us) {
        stopwatches[id].max_us = elapsed_us;
    }
}

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_MAX_STOPWATCHES || stopwatches[id].name == NULL) {
        return false; // Сообщаем вызывающей функции, что такого секундомера нет
    }

    // Копируем данные из нашего скрытого массива во внешнюю структуру result (переданную по указателю)
    result->name = stopwatches[id].name;
    result->count = stopwatches[id].count;
    result->max_us = stopwatches[id].max_us;

    // Вычисляем среднее арифметическое. 
    // Защита от деления на ноль (если count == 0).
    if (stopwatches[id].count > 0) {
        // Принудительно превращаем целые числа в числа с плавающей точкой (double) перед делением,
        // чтобы получить точный результат с дробной частью (например, 9.96 мкс).
        result->mean_us = (double)stopwatches[id].sum_us / (double)stopwatches[id].count;
    } else {
        result->mean_us = 0.0;
    }

    return true; // Данные успешно скопированы
}

void profiling_reset(void)
{
    // Циклом пробегаем по всему массиву от 0 до 7
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++) {
        // Обнуляем математику
        stopwatches[i].sum_us = 0;
        stopwatches[i].count = 0;
        stopwatches[i].max_us = 0;
        // ВАЖНО: поле name мы не трогаем! Секундомеры сбрасываются, но не удаляются.
    }
}