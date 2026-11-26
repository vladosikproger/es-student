#include "slave/slave.h"
#include "platform/platform.h"

#include <stdbool.h>
#include <stdint.h>

// 500 ms per change: full blink cycle = 1 second.
enum { BLINK_PERIOD_MS = 500 };

static uint32_t last_change;
static bool led_on;

void slave_device_init(void)
{
    platform_slave_init();
    led_on = false;
    last_change = platform_time_ms();
}

void slave_device_handle(void)
{
    // No blocking delays: the loop stays responsive.
    if ((uint32_t)(platform_time_ms() - last_change) >= BLINK_PERIOD_MS)
    {
        last_change += BLINK_PERIOD_MS;
        led_on = !led_on;
        platform_led_set(led_on);
    }
}
