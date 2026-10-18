#include "profiling/profiling.h"

#if PROFILING_ENABLED

#include "systime/systime.h"
#include <string.h>

#define PROFILING_MAX_STOPWATCHES 10u

typedef struct {
    const char *name;
    uint64_t start_time_us;
    uint64_t sum_us;
    uint32_t count;
    uint32_t max_us;
    bool allocated;
    bool running;
} stopwatch_t;

static stopwatch_t stopwatches[PROFILING_MAX_STOPWATCHES];

void profiling_init(void)
{
    memset(stopwatches, 0, sizeof(stopwatches));
}

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id >= PROFILING_MAX_STOPWATCHES) {
        return;
    }

    stopwatch_t *sw = &stopwatches[id];
    sw->name = (name != NULL) ? name : "unnamed";
    sw->start_time_us = 0;
    sw->sum_us = 0;
    sw->count = 0;
    sw->max_us = 0;
    sw->allocated = true;
    sw->running = false;
}

uint32_t profiling_allocate(const char *name)
{
    for (uint32_t id = 0; id < PROFILING_MAX_STOPWATCHES; ++id) {
        if (!stopwatches[id].allocated) {
            profiling_stopwatch_init(id, name);
            return id;
        }
    }

    /* IDs 0..PROFILING_MAX_STOPWATCHES-1 are valid; this value is not. */
    return PROFILING_MAX_STOPWATCHES;
}

void profiling_start(uint32_t id)
{
    if (id >= PROFILING_MAX_STOPWATCHES || !stopwatches[id].allocated) {
        return;
    }

    stopwatches[id].start_time_us = systime_us();
    stopwatches[id].running = true;
}

void profiling_stop(uint32_t id)
{
    if (id >= PROFILING_MAX_STOPWATCHES
        || !stopwatches[id].allocated
        || !stopwatches[id].running) {
        return;
    }

    const uint64_t end_time_us = systime_us();
    const uint64_t elapsed_us = end_time_us - stopwatches[id].start_time_us;
    const uint32_t duration_us = elapsed_us > UINT32_MAX
        ? UINT32_MAX
        : (uint32_t)elapsed_us;

    stopwatches[id].sum_us += duration_us;
    ++stopwatches[id].count;
    if (duration_us > stopwatches[id].max_us) {
        stopwatches[id].max_us = duration_us;
    }
    stopwatches[id].running = false;
}

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_MAX_STOPWATCHES
        || result == NULL
        || !stopwatches[id].allocated) {
        return false;
    }

    result->name = stopwatches[id].name;
    result->count = stopwatches[id].count;
    result->mean_us = stopwatches[id].count == 0u
        ? 0.0f
        : (float)stopwatches[id].sum_us / (float)stopwatches[id].count;
    result->max_us = stopwatches[id].max_us;
    return true;
}

void profiling_reset(void)
{
    for (uint32_t id = 0; id < PROFILING_MAX_STOPWATCHES; ++id) {
        if (stopwatches[id].allocated) {
            stopwatches[id].start_time_us = 0;
            stopwatches[id].sum_us = 0;
            stopwatches[id].count = 0;
            stopwatches[id].max_us = 0;
            stopwatches[id].running = false;
        }
    }
}

#endif /* PROFILING_ENABLED */
