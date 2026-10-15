#include "profiling/profiling.h"
#include "pico/stdlib.h"
#include <string.h>

#if PROFILING_ENABLED

#define PROFILING_MAX_STOPWATCHES 10

typedef struct {
    const char *name;
    uint32_t start_time_us;
    uint64_t sum_us;
    uint32_t count;
    uint32_t max_us;
    bool allocated;
} stopwatch_t;

static stopwatch_t stopwatches[PROFILING_MAX_STOPWATCHES];

void profiling_init(void) {
    memset(stopwatches, 0, sizeof(stopwatches));
}

uint32_t profiling_allocate(const char *name) {
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++) {
        if (!stopwatches[i].allocated) {
            stopwatches[i].name = name;
            stopwatches[i].allocated = true;
            return i;
        }
    }
    return 0;
}

void profiling_start(uint32_t id) {
    if (id < PROFILING_MAX_STOPWATCHES && stopwatches[id].allocated) {
        stopwatches[id].start_time_us = time_us_32();
    }
}

void profiling_stop(uint32_t id) {
    if (id < PROFILING_MAX_STOPWATCHES && stopwatches[id].allocated) {
        uint32_t end_time = time_us_32();
        uint32_t duration = end_time - stopwatches[id].start_time_us;
        
        stopwatches[id].sum_us += duration;
        stopwatches[id].count++;
        if (duration > stopwatches[id].max_us) {
            stopwatches[id].max_us = duration;
        }
    }
}

bool profiling_get(uint32_t id, profiling_result_t *result) {
    if (id < PROFILING_MAX_STOPWATCHES && stopwatches[id].allocated && stopwatches[id].count > 0) {
        result->name = stopwatches[id].name;
        result->count = stopwatches[id].count;
        result->mean_us = (float)stopwatches[id].sum_us / stopwatches[id].count;
        result->max_us = stopwatches[id].max_us;
        return true;
    }
    return false;
}

void profiling_reset(void) {
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++) {
        if (stopwatches[i].allocated) {
            stopwatches[i].sum_us = 0;
            stopwatches[i].count = 0;
            stopwatches[i].max_us = 0;
        }
    }
}

#endif