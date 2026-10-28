#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LED_PIN 25
#define BUTTON_PIN 15

// Вход АЦП: вывод GP26, канал 0.
#define ADC_PIN 26
#define ADC_CHANNEL 0

// Опорное напряжение и число уровней 12-разрядного кода АЦП.
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Инициализируем АЦП и подключаем GP26 к аналоговому входу.
    adc_init();
    adc_gpio_init(ADC_PIN);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_button_read(void)
{
    return gpio_get(BUTTON_PIN) == 0;
}

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return (float)adc_read() * ADC_VREF_V / ADC_LEVELS;
}
