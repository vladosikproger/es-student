#include "telemetry-task/telemetry-task.h"

#include <stddef.h>

#include "platform/platform.h"
#include "systime/systime.h"

#define TELEMETRY_MIN_PERIOD_MS 10U
#define TELEMETRY_DEFAULT_PERIOD_MS 100U

// Все эти переменные принадлежат только этому модулю.
static telemetry_state_t state = TELEMETRY_STATE_IDLE;
static telemetry_task_callback_t on_record = NULL;
static uint32_t period_ms = TELEMETRY_DEFAULT_PERIOD_MS;
static uint64_t period_us = (uint64_t)TELEMETRY_DEFAULT_PERIOD_MS * 1000ULL;
static uint64_t last_us = 0;
static bool first_record_pending = false;

void telemetry_task_init(telemetry_task_callback_t callback)
{
    on_record = callback;
    state = TELEMETRY_STATE_IDLE;
    period_ms = TELEMETRY_DEFAULT_PERIOD_MS;
    period_us = (uint64_t)period_ms * 1000ULL;
    last_us = 0;
    first_record_pending = false;
}

bool telemetry_task_start(uint32_t new_period_ms)
{
    if (new_period_ms < TELEMETRY_MIN_PERIOD_MS)
    {
        return false;
    }

    period_ms = new_period_ms;
    // Преобразуем в uint64_t до умножения, чтобы умножение не переполнило uint32_t.
    period_us = (uint64_t)new_period_ms * 1000ULL;

    // Перезапуск тоже должен дать первую запись на ближайшем вызове handle().
    last_us = 0;
    first_record_pending = true;
    state = TELEMETRY_STATE_RUN;
    return true;
}

void telemetry_task_stop(void)
{
    state = TELEMETRY_STATE_IDLE;
    first_record_pending = false;
}

void telemetry_task_handle(void)
{
    switch (state)
    {
        case TELEMETRY_STATE_IDLE:
            return;

        case TELEMETRY_STATE_RUN:
            break;
    }

    // Первый отсчёт делаем сразу на ближайшей итерации суперцикла.
    // Следующие отсчёты разрешает systime_period_elapsed().
    const bool first_record = first_record_pending;
    if (first_record)
    {
        first_record_pending = false;
    }
    else if (!systime_period_elapsed(&last_us, period_us))
    {
        return;
    }

    telemetry_record_t record;
    record.time_us = systime_us();
    record.voltage_v = platform_voltage_read();
    record.temp_c = platform_temp_read();

    if (first_record)
    {
        // Начальная временная точка — время первого фактического отсчёта.
        last_us = record.time_us;
    }

    // Callback вызывается сразу: указатель на локальную запись нельзя сохранять
    // для использования после возврата из callback.
    if (on_record != NULL)
    {
        on_record(&record);
    }
}

telemetry_state_t telemetry_task_get_state(void)
{
    return state;
}

uint32_t telemetry_task_get_period_ms(void)
{
    return period_ms;
}
