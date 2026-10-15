#include "led-task.h"
#include "pico/stdlib.h"
#include "systime/systime.h"

// Аппаратный пин светодиода на Raspberry Pi Pico
#define LED_PIN 25

static led_state_t led_state = LED_STATE_OFF;
static uint32_t led_period_ms = 1000;
static uint64_t last_toggle_us = 0;

void led_task_init(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
}

void led_task_handle(void) {
    if (led_state == LED_STATE_OFF) {
        gpio_put(LED_PIN, 0);
    } 
    else if (led_state == LED_STATE_ON) {
        gpio_put(LED_PIN, 1);
    } 
    else if (led_state == LED_STATE_BLINK) {
        // Вызываем функцию из нашей новой библиотеки
        if (systime_period_elapsed(&last_toggle_us, led_period_ms * 1000)) {
            gpio_put(LED_PIN, !gpio_get(LED_PIN));
        }
    }
}

led_state_t led_task_get_state(void) {
    return led_state;
}

void led_task_set_state(led_state_t state) {
    led_state = state;
}

uint32_t led_task_get_period_ms(void) {
    return led_period_ms;
}

bool led_task_set_period_ms(uint32_t period_ms) {
    if (period_ms == 0) {
        return false;
    }
    led_period_ms = period_ms;
    return true;
}
void led_task_next_state(void) {
    if (led_state == LED_STATE_OFF) {
        led_state = LED_STATE_ON;
    } 
    else if (led_state == LED_STATE_ON) {
        led_state = LED_STATE_BLINK;
    } 
    else {
        led_state = LED_STATE_OFF;
    }
}