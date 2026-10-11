#include <stdio.h>
#include "pico/stdlib.h"
#include "led.h"
#include "profiling.h"

// Подключаем контракты наших новых модулей
#include "stdio-text-protocol.h"
#include "api.h"

// -------------------------------------------------------------
// ЛОГИКА МИГАНИЯ
// -------------------------------------------------------------
const uint BLINK_HALF_PERIOD_MS = 500;
uint64_t last_toggle_us = 0;

void blink(void)
{
    uint64_t now_us = time_us_64();
    if (now_us - last_toggle_us >= BLINK_HALF_PERIOD_MS * 1000)
    {
        last_toggle_us = now_us;
        led_toggle();
    }
}

// -------------------------------------------------------------
// ТОЧКА ВХОДА (СУПЕРЦИКЛ)
// -------------------------------------------------------------
int main(void)
{
    // Инициализация железа
    stdio_init_all();
    setbuf(stdout, NULL);
    led_init();
    profiling_init();

    // Инициализация состояния протокола перед стартом цикла
    stdio_text_protocol_init();

    while (1)
    {
        // 1. Сбор телеметрии
        profiling_iteration();
        
        // 2. Фоновая задача мигания
        blink();
        
        // 3. Конвейер команд: Протокол -> API
        const command_t *command = stdio_text_protocol_handle();
        
        if (command != NULL)
        {
            api_handle(command);
        }
    }
}