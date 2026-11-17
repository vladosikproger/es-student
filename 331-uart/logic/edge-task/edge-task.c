#include "edge-task/edge-task.h"

#include <stddef.h>
#include "systime/systime.h"

#define EMPTY_V    0.05f
#define RISE_V     2.0f
#define FULL_V     3.2f
#define FALL_V     1.3f
#define TIMEOUT_US 2000000u

typedef enum {
    EDGE_IDLE, EDGE_DISCHARGE, EDGE_RISE, EDGE_CHARGE, EDGE_FALL
} edge_state_t;

static edge_state_t state = EDGE_IDLE;
static platform_line_mode_t line_mode;
static edge_task_callback_t callback;
static uint64_t phase_start_us;
static uint32_t rise_us;

// Начинаем отсчёт непосредственно при переключении уровня.
static void enter(edge_state_t next, bool high)
{
    platform_line_set(line_mode, high);
    phase_start_us = systime_us();
    state = next;
}

static void finish(bool ok, uint32_t fall_us)
{
    if (!ok)
        platform_line_set(line_mode, false);

    edge_task_callback_t done = callback;
    callback = NULL;
    state = EDGE_IDLE;
    // После вызова ничего не меняем: из done можно начать следующий замер.
    if (done != NULL)
        done(line_mode, ok, ok ? rise_us : 0u, fall_us);
}

bool edge_task_start(platform_line_mode_t mode, edge_task_callback_t on_done)
{
    if (state != EDGE_IDLE)
        return false;

    line_mode = mode;
    callback = on_done;
    rise_us = 0;
    enter(EDGE_DISCHARGE, false);
    return true;
}

void edge_task_handle(void)
{
    if (state == EDGE_IDLE)
        return;

    const float voltage = platform_voltage_read();
    const uint64_t now_us = systime_us();
    const uint64_t elapsed_us = now_us - phase_start_us;
    if (elapsed_us > TIMEOUT_US)
    {
        finish(false, 0);
        return;
    }

    switch (state)
    {
        case EDGE_DISCHARGE:
            if (voltage < EMPTY_V)
                enter(EDGE_RISE, true);
            break;

        case EDGE_RISE:
            if (voltage > RISE_V)
            {
                rise_us = (uint32_t)elapsed_us;
                // Через 2 В прошли, но линию пока не опускаем.
                phase_start_us = now_us;
                state = EDGE_CHARGE;
            }
            break;

        case EDGE_CHARGE:
            if (voltage > FULL_V)
                enter(EDGE_FALL, false);
            break;

        case EDGE_FALL:
            if (voltage < FALL_V)
                finish(true, (uint32_t)elapsed_us);
            break;

        case EDGE_IDLE:
            break;
    }
}
