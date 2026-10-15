#include "button-task.h"
#include "pico/stdlib.h" // Оставляем, так как нам нужно физически читать ножки (GPIO)
#include "led-task.h"
#include "profiling/profiling.h"
#include "systime/systime.h" // Подключаем нашего независимого диспетчера времени

#define BUTTON_PIN 15
const uint64_t DEBOUNCE_TIME_US = 20000;

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING
} button_state_t;

static button_state_t button_state;
static uint64_t transition_start_time_us;
static uint32_t press_count;

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); 

    button_state = BUTTON_STATE_RELEASED;
    press_count = 0;
}

void button_task_handle(void)
{
    bool is_physical_pressed = (gpio_get(BUTTON_PIN) == 0);
    
    // ОБРАЩАЕМСЯ К ДИСПЕТЧЕРУ (вместо прямого обращения к железу)
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
                led_task_next_state(); 
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
    return (button_state == BUTTON_STATE_PRESSED || button_state == BUTTON_STATE_RELEASING);
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}