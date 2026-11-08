// Задача захвата блока: запускает DMA и передаёт готовый блок callback-функции.
// Сама задача не знает, что получатель будет делать с отсчётами.
#pragma once

#include <stdbool.h>
#include <stdint.h>

// Callback получает массив отсчётов, их количество и длительность захвата.
typedef void (*block_task_callback_t)(const uint16_t *samples,
                                      uint32_t count,
                                      uint32_t time_us);

// false — задача уже занята или platform не смогла начать захват.
bool block_task_start(uint32_t count, block_task_callback_t on_block);

// Вызывается на каждой итерации суперцикла. Не ожидает завершения DMA.
void block_task_handle(void);
