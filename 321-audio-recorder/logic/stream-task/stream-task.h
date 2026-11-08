// Задача потока: забирает готовые DMA-буферы и передаёт их как двоичные кадры.
// Формат кадра и способ передачи находятся за callback-границей.
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STREAM_STATE_IDLE,
    STREAM_STATE_RUN,
} stream_state_t;

// В памяти RP2040 little-endian это байты a5 5a.
#define STREAM_SYNC_WORD 0x5aa5

// Заголовок приложения А: три 16-битных поля без промежутков.
typedef struct {
    uint16_t sync_word;
    uint16_t number;
    uint16_t count;
} stream_header_t;

// Callback получает заголовок, полезные данные и число байтов полезной нагрузки.
typedef void (*stream_task_callback_t)(const stream_header_t *header,
                                       const uint8_t *data,
                                       uint32_t bytes);

void stream_task_init(stream_task_callback_t on_frame);
void stream_task_handle(void);

// bits — 16 или 8; на этапе п3.2.4 платформа поддерживает только 16 бит.
bool stream_task_start(uint8_t bits);
void stream_task_stop(void);

stream_state_t stream_task_get_state(void);
uint8_t stream_task_get_bits(void);

uint32_t stream_task_get_frames(void);
uint32_t stream_task_get_samples(void);
uint32_t stream_task_get_overruns(void);
