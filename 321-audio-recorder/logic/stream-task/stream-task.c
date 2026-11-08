#include "stream-task/stream-task.h"

#include <stddef.h>

#include "platform/platform.h"

// По приложению А заголовок должен занимать ровно шесть байт.
_Static_assert(sizeof(stream_header_t) == 6U, "stream header must be 6 bytes");

#define STREAM_BUFFER_SAMPLE_BYTES_16 2U
#define STREAM_BUFFER_SAMPLE_BYTES_8  1U

static stream_state_t state = STREAM_STATE_IDLE;
static stream_task_callback_t on_frame;
static uint8_t bits = 16U;
static uint16_t next_frame_number;
static uint32_t frames_sent;
static uint32_t samples_sent;

void stream_task_init(stream_task_callback_t callback)
{
    on_frame = callback;
    state = STREAM_STATE_IDLE;
    bits = 16U;
    next_frame_number = 0U;
    frames_sent = 0U;
    samples_sent = 0U;
}

bool stream_task_start(uint8_t new_bits)
{
    if (state == STREAM_STATE_RUN || on_frame == NULL
        || (new_bits != 8U && new_bits != 16U))
    {
        return false;
    }

    if (!platform_stream_start(new_bits))
    {
        return false;
    }

    bits = new_bits;
    next_frame_number = 0U;
    frames_sent = 0U;
    samples_sent = 0U;
    state = STREAM_STATE_RUN;
    return true;
}

void stream_task_stop(void)
{
    if (state == STREAM_STATE_RUN)
    {
        platform_stream_stop();
        state = STREAM_STATE_IDLE;
    }
}

void stream_task_handle(void)
{
    if (state != STREAM_STATE_RUN)
    {
        return;
    }

    uint32_t samples = 0U;
    const uint8_t *data = platform_stream_next(&samples);
    if (data == NULL || samples == 0U)
    {
        return;
    }

    const stream_header_t header = {
        .sync_word = STREAM_SYNC_WORD,
        .number = next_frame_number,
        .count = (uint16_t)samples,
    };

    const uint32_t bytes_per_sample =
        (bits == 16U) ? STREAM_BUFFER_SAMPLE_BYTES_16 : STREAM_BUFFER_SAMPLE_BYTES_8;
    const uint32_t bytes = samples * bytes_per_sample;

    // Callback должен отправить кадр синхронно, до освобождения буфера.
    on_frame(&header, data, bytes);
    platform_stream_release();

    // Номер кадра приложения А циклически переходит через 65535 к нулю.
    next_frame_number++;
    frames_sent++;
    samples_sent += samples;
}

stream_state_t stream_task_get_state(void)
{
    return state;
}

uint8_t stream_task_get_bits(void)
{
    return bits;
}

uint32_t stream_task_get_frames(void)
{
    return frames_sent;
}

uint32_t stream_task_get_samples(void)
{
    return samples_sent;
}

uint32_t stream_task_get_overruns(void)
{
    return platform_stream_overruns();
}
