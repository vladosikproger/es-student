#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "platform/platform.h"

// A callback reports a recognised button event. The button task does not
// know what the event means for the device.
typedef void (*button_task_callback_t)(void);

// Initialise one button. If on_long_press is not NULL, on_press is treated as
// the short-press action and is called only after a stable release. The long
// callback is called once after the button has been held for one second.
// If on_long_press is NULL, on_press is called as soon as the press is debounced.
void button_task_init(
    platform_button_t button,
    button_task_callback_t on_press,
    button_task_callback_t on_long_press
);

// Processes every button. Call once during each superloop iteration.
void button_task_handle(void);

bool button_task_is_pressed(platform_button_t button);
uint32_t button_task_get_press_count(platform_button_t button);
