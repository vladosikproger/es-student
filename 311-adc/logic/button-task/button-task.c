#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"

#include <stddef.h>

static const uint64_t DEBOUNCE_TIME_US = 20000;

typedef enum
{
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING
} button_state_t;

static button_state_t button_state;
static uint64_t transition_start_time_us;
static uint32_t press_count;
static button_task_callback_t on_press_callback;

void button_task_init(button_task_callback_t on_press)
{
    on_press_callback = on_press;
    button_state = BUTTON_STATE_RELEASED;
    transition_start_time_us = 0;
    press_count = 0;
}

void button_task_handle(void)
{
    bool is_physical_pressed = platform_button_read();
    uint64_t now_us = systime_us();

    switch (button_state)
    {
        case BUTTON_STATE_RELEASED:
            if (is_physical_pressed)
            {
                transition_start_time_us = now_us;
                button_state = BUTTON_STATE_PRESSING;
            }
            break;

        case BUTTON_STATE_PRESSING:
            if (!is_physical_pressed)
            {
                button_state = BUTTON_STATE_RELEASED;
            }
            else if (now_us - transition_start_time_us >= DEBOUNCE_TIME_US)
            {
                button_state = BUTTON_STATE_PRESSED;
                press_count++;
                if (on_press_callback != NULL)
                {
                    on_press_callback();
                }
            }
            break;

        case BUTTON_STATE_PRESSED:
            if (!is_physical_pressed)
            {
                transition_start_time_us = now_us;
                button_state = BUTTON_STATE_RELEASING;
            }
            break;

        case BUTTON_STATE_RELEASING:
            if (is_physical_pressed)
            {
                button_state = BUTTON_STATE_PRESSED;
            }
            else if (now_us - transition_start_time_us >= DEBOUNCE_TIME_US)
            {
                button_state = BUTTON_STATE_RELEASED;
            }
            break;
    }
}

bool button_task_is_pressed(void)
{
    return button_state == BUTTON_STATE_PRESSED
        || button_state == BUTTON_STATE_RELEASING;
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}