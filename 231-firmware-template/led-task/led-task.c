#include "led-task.h"
#include "pico/stdlib.h"

// 1. Скрытая память задачи (State)
static led_state_t current_state;
static uint32_t blink_period_ms;
static uint64_t last_toggle_us;

// 2. Механизм точного времени (компенсация дрейфа)
static bool period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = time_us_64();
    if (now_us - *last_us >= period_us)
    {
        // Защита от долгого зависания суперцикла
        if (now_us - *last_us > 2 * period_us)
        {
            *last_us = now_us;
        }
        else
        {
            *last_us += period_us; // Сохранение идеальной математической сетки
        }
        return true;
    }
    return false;
}

// 3. Инициализация (настройка железа)
void led_task_init(void)
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    blink_period_ms = 1000;
    led_task_set_state(LED_STATE_BLINK);
}

// 4. Переход машины состояний (Действие при входе)
void led_task_set_state(led_state_t state)
{
    current_state = state;
    
    switch (current_state)
    {
        case LED_STATE_OFF:
            gpio_put(PICO_DEFAULT_LED_PIN, false); // Гасим пин
            break;
        case LED_STATE_ON:
            gpio_put(PICO_DEFAULT_LED_PIN, true);  // Зажигаем пин
            break;
        case LED_STATE_BLINK:
            last_toggle_us = time_us_64();         // Сброс таймера
            gpio_put(PICO_DEFAULT_LED_PIN, true);  // Мгновенная реакция
            break;
    }
}

// 5. Обработчик в суперцикле (Tick)
void led_task_handle(void)
{
    switch (current_state)
    {
        case LED_STATE_OFF:
        case LED_STATE_ON:
            // Аппаратура статична, ядро процессора свободно
            break;

        case LED_STATE_BLINK:
            // Полпериода — это смена состояния (toggle)
            if (period_elapsed(&last_toggle_us, (blink_period_ms * 1000ULL) / 2))
            {
                gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
            }
            break;
    }
}

// 6. Геттеры и сеттеры для API
bool led_task_set_period_ms(uint32_t period_ms)
{
    if (period_ms == 0) return false;
    blink_period_ms = period_ms;
    return true;
}

uint32_t led_task_get_period_ms(void)
{
    return blink_period_ms;
}

led_state_t led_task_get_state(void)
{
    return current_state;
}