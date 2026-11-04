#pragma once

#include <stdbool.h>
#include <stdint.h>

// Две физические кнопки устройства.
// BUTTON_1 подключена к GP15, BUTTON_2 — к GP14.
typedef enum
{
    PLATFORM_BUTTON_1 = 0,
    PLATFORM_BUTTON_2,
    PLATFORM_BUTTON_COUNT
} platform_button_t;

void platform_init(void);
void platform_led_set(bool on);

// Возвращает true, когда указанная кнопка нажата.
// Кнопки включены активным низким уровнем: GPIO замыкается на GND.
bool platform_button_read(platform_button_t button);

float platform_voltage_read(void);
float platform_temp_read(void);

// true — GP22 ≈ 3,3 В (заряд), false — GP22 ≈ 0 В (разряд).
void platform_charge_set(bool on);
