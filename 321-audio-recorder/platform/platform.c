#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/time.h"

#include <stddef.h>

#define LED_PIN 25
#define BUTTON_1_PIN 15
#define BUTTON_2_PIN 14
#define CHARGE_PIN 22

#define ADC_PIN 26
#define ADC_CHANNEL 0
#define ADC_TEMP_CHANNEL 4
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096
#define ADC_TEMP_V_AT_27_C 0.706f
#define ADC_TEMP_V_PER_C 0.001721f

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

    // Обе кнопки используют внутренние подтягивающие резисторы.
    // В отпущенном состоянии GPIO = 1; при нажатии кнопка соединяет GPIO с GND.
    for (uint32_t i = 0; i < (uint32_t)PLATFORM_BUTTON_COUNT; ++i)
    {
        gpio_init(button_pins[i]);
        gpio_set_dir(button_pins[i], GPIO_IN);
        gpio_pull_up(button_pins[i]);
    }

    // GP22 управляет RC-цепью. Безопасное начальное состояние — LOW.
    gpio_init(CHARGE_PIN);
    gpio_set_dir(CHARGE_PIN, GPIO_OUT);
    gpio_put(CHARGE_PIN, 0);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_set_temp_sensor_enabled(true);
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

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return (float)adc_read() * ADC_VREF_V / ADC_LEVELS;
}

float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    const float voltage = (float)adc_read() * ADC_VREF_V / ADC_LEVELS;
    return 27.0f - (voltage - ADC_TEMP_V_AT_27_C) / ADC_TEMP_V_PER_C;
}

void platform_charge_set(bool on)
{
    gpio_put(CHARGE_PIN, on);
}

// Объект должен жить всё время, пока SDK обслуживает повторяющийся будильник.
static repeating_timer_t blink_timer;

// Эти данные читаются/изменяются из разных контекстов:
// основной код управляет запуском/остановкой, callback вызывается из IRQ.
static bool blink_running;
static volatile bool blink_on;
static volatile uint32_t blink_toggles;

// Короткий обработчик прерывания. Не печатает, не ждёт и не выделяет память.
static bool on_blink(repeating_timer_t *timer)
{
    (void)timer;

    blink_on = !blink_on;
    gpio_put(LED_PIN, blink_on);
    blink_toggles++;

    // true просит SDK оставить повторяющийся будильник активным.
    return true;
}

bool platform_blink_start(uint32_t period_ms)
{
    if (period_ms < 10U || period_ms > 60000U)
    {
        return false;
    }

    // Если предыдущий запуск ещё активен, сначала корректно его снимаем.
    (void)platform_blink_stop();
    blink_toggles = 0;
    blink_on = false;

    // Отрицательный период означает отсчёт от начала прошлого callback:
    // интервалы между началами вызовов остаются привязанными к сетке времени.
    blink_running = add_repeating_timer_ms(-(int32_t)period_ms, on_blink, NULL, &blink_timer);

    return blink_running;
}

uint32_t platform_blink_stop(void)
{
    if (blink_running)
    {
        (void)cancel_repeating_timer(&blink_timer);
        blink_running = false;
    }

    blink_on = false;
    gpio_put(LED_PIN, 0);
    return blink_toggles;
}

