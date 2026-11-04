#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"

#include <stddef.h>

#define DEBOUNCE_TIME_US 20000u
#define LONG_PRESS_TIME_US 1000000u

typedef enum
{
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING
} button_state_t;

static button_state_t button_states[PLATFORM_BUTTON_COUNT];
static uint64_t transition_start_time_us[PLATFORM_BUTTON_COUNT];
static uint64_t press_start_time_us[PLATFORM_BUTTON_COUNT];
static uint32_t press_counts[PLATFORM_BUTTON_COUNT];
static button_task_callback_t on_press_callbacks[PLATFORM_BUTTON_COUNT];
static button_task_callback_t on_long_press_callbacks[PLATFORM_BUTTON_COUNT];
static bool long_press_reported[PLATFORM_BUTTON_COUNT];

void button_task_init(
    platform_button_t button,
    button_task_callback_t on_press,
    button_task_callback_t on_long_press
)
{
    const uint32_t index = (uint32_t)button;
    if (index >= (uint32_t)PLATFORM_BUTTON_COUNT)
    {
        return;
    }

    button_states[index] = BUTTON_STATE_RELEASED;
    transition_start_time_us[index] = 0;
    press_start_time_us[index] = 0;
    press_counts[index] = 0;
    on_press_callbacks[index] = on_press;
    on_long_press_callbacks[index] = on_long_press;
    long_press_reported[index] = false;
}

void button_task_handle(void)
{
    const uint64_t now_us = systime_us();

    for (uint32_t i = 0; i < (uint32_t)PLATFORM_BUTTON_COUNT; ++i)
    {
        const platform_button_t button = (platform_button_t)i;
        const bool is_physical_pressed = platform_button_read(button);

        switch (button_states[i])
        {
            case BUTTON_STATE_RELEASED:
                if (is_physical_pressed)
                {
                    transition_start_time_us[i] = now_us;
                    long_press_reported[i] = false;
                    button_states[i] = BUTTON_STATE_PRESSING;
                }
                break;

            case BUTTON_STATE_PRESSING:
                if (!is_physical_pressed)
                {
                    // Сигнал не продержался достаточно долго: считаем его дребезгом.
                    button_states[i] = BUTTON_STATE_RELEASED;
                }
                else if (now_us - transition_start_time_us[i] >= DEBOUNCE_TIME_US)
                {
                    button_states[i] = BUTTON_STATE_PRESSED;
                    press_start_time_us[i] = now_us;
                    ++press_counts[i];

                    // Для кнопки без отдельного long-press действия
                    // короткое действие выполняется сразу после debounce.
                    if (on_long_press_callbacks[i] == NULL
                        && on_press_callbacks[i] != NULL)
                    {
                        on_press_callbacks[i]();
                    }
                }
                break;

            case BUTTON_STATE_PRESSED:
                if (is_physical_pressed
                    && on_long_press_callbacks[i] != NULL
                    && !long_press_reported[i]
                    && now_us - press_start_time_us[i] >= LONG_PRESS_TIME_US)
                {
                    long_press_reported[i] = true;
                    on_long_press_callbacks[i]();
                }

                if (!is_physical_pressed)
                {
                    transition_start_time_us[i] = now_us;
                    button_states[i] = BUTTON_STATE_RELEASING;
                }
                break;

            case BUTTON_STATE_RELEASING:
                if (is_physical_pressed)
                {
                    // Отпускание оказалось дребезгом; нажатие продолжается.
                    button_states[i] = BUTTON_STATE_PRESSED;
                }
                else if (now_us - transition_start_time_us[i] >= DEBOUNCE_TIME_US)
                {
                    button_states[i] = BUTTON_STATE_RELEASED;

                    // Если long-press не сработал, это было короткое нажатие.
                    if (on_long_press_callbacks[i] != NULL
                        && !long_press_reported[i]
                        && on_press_callbacks[i] != NULL)
                    {
                        on_press_callbacks[i]();
                    }
                }
                break;
        }
    }
}

bool button_task_is_pressed(platform_button_t button)
{
    const uint32_t index = (uint32_t)button;
    if (index >= (uint32_t)PLATFORM_BUTTON_COUNT)
    {
        return false;
    }

    return button_states[index] == BUTTON_STATE_PRESSED
        || button_states[index] == BUTTON_STATE_RELEASING;
}

uint32_t button_task_get_press_count(platform_button_t button)
{
    const uint32_t index = (uint32_t)button;
    if (index >= (uint32_t)PLATFORM_BUTTON_COUNT)
    {
        return 0;
    }

    return press_counts[index];
}
