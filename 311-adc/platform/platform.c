#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LED_PIN 25
#define BUTTON_PIN 15

// Вход АЦП: вывод GP26, канал 0.
#define ADC_PIN 26
#define ADC_CHANNEL 0

// Встроенный датчик температуры подключён к каналу 4 АЦП.
#define ADC_TEMP_CHANNEL 4

// Опорное напряжение и число уровней 12-разрядного кода АЦП.
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

// Параметры встроенного датчика температуры из документации RP2040.
#define ADC_TEMP_V_AT_27_C 0.706f
#define ADC_TEMP_V_PER_C 0.001721f

void platform_init(void)
{
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Инициализируем АЦП.
    adc_init();
    adc_gpio_init(ADC_PIN);

    // У встроенного датчика температуры отдельное управление питанием.
    adc_set_temp_sensor_enabled(true);
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
    // Перед каждым измерением выбираем внешний вход GP26 (канал 0).
    adc_select_input(ADC_CHANNEL);
    return (float)adc_read() * ADC_VREF_V / ADC_LEVELS;
}

float platform_temp_read(void)
{
    // Внутренний датчик температуры расположен на канале 4.
    adc_select_input(ADC_TEMP_CHANNEL);

    // Сначала получаем код АЦП, затем переводим его в вольты.
    const float voltage = (float)adc_read() * ADC_VREF_V / ADC_LEVELS;

    // U = 0,706 В при 27 °C; напряжение уменьшается на 1,721 мВ/°C.
    return 27.0f - (voltage - ADC_TEMP_V_AT_27_C) / ADC_TEMP_V_PER_C;
}
