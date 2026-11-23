#include "block-task/block-task.h"

#include <stddef.h>
#include "platform/platform.h"

static block_task_callback_t block_callback;
static uint32_t block_count;

bool block_task_start(uint32_t count, block_task_callback_t on_block)
{
    if (block_callback != NULL || on_block == NULL)
    {
        return false;
    }
    block_callback = on_block;
    block_count = count;

    if (!platform_block_start(count))
    {
        block_callback = NULL;
        block_count = 0u;
        return false;
    }
    return true;
}

void block_task_handle(void)
{
    if (block_callback == NULL || !platform_block_done())
    {
        return;
    }

    block_task_callback_t cb = block_callback;
    const uint32_t count = block_count;
    const uint32_t time_us = platform_block_time_us();
    const uint16_t *samples = platform_block_samples();

    block_callback = NULL;
    block_count = 0u;
    cb(samples, count, time_us);
}
