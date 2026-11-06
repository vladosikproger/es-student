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

// Таймерный индикатор записи.
// false — период вне диапазона 10..60000 мс или будильник не удалось создать.
bool platform_blink_start(uint32_t period_ms);

// Останавливает мигание, гасит светодиод и возвращает число переключений.
uint32_t platform_blink_stop(void);

// Максимальное количество отсчётов в одном захвате по будильнику.
#define PLATFORM_CAPTURE_MAX 2000

// Занят ли АЦП захватом по будильнику.
bool platform_adc_busy(void);

// Захват count отсчётов GP26 по будильнику с периодом period_us.
// false — АЦП занят или параметры не позволяют начать захват.
bool platform_capture_start(uint32_t count, uint32_t period_us);

// true, если захват запускали и все отсчёты уже набраны.
bool platform_capture_done(void);

typedef struct
{
    const uint16_t *samples;
    uint32_t count;
    uint32_t period_us;
    uint32_t min_interval_us;
    uint32_t max_interval_us;
    float handler_percent;
} platform_capture_t;

// Заполняет структуру результатами последнего завершённого захвата.
void platform_capture_get(platform_capture_t *capture);
