#include "platform/platform.h"

#include <stdint.h>
#include "pico/stdlib.h"

#define LED_PIN 25
#define BUTTON_1_PIN 15
#define BUTTON_2_PIN 14

static const uint32_t button_pins[PLATFORM_BUTTON_COUNT] =
{
    BUTTON_1_PIN,
    BUTTON_2_PIN
};

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    for (uint32_t i = 0; i < (uint32_t)PLATFORM_BUTTON_COUNT; ++i)
    {
        gpio_init(button_pins[i]);
        gpio_set_dir(button_pins[i], GPIO_IN);
        gpio_pull_up(button_pins[i]);
    }
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_button_read(platform_button_t button)
{
    const uint32_t index = (uint32_t)button;
    if (index >= (uint32_t)PLATFORM_BUTTON_COUNT)
    {
        return false;
    }

    return gpio_get(button_pins[index]) == 0;
}
