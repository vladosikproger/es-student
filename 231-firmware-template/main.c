#include <stdio.h>
#include "pico/stdlib.h"

// Подключаем модули для стартового сообщения
#include "device.h" 
#include "log.h" 

#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"

// Создаем перечисляемый тип для наших 5 секундомеров
typedef enum {
    STOPWATCH_LOOP,
    STOPWATCH_COMMAND,
    STOPWATCH_LED,
    STOPWATCH_BUTTON,
    STOPWATCH_PI
} stopwatch_id_t;

int main(void)
{
    stdio_init_all();
    setbuf(stdout, NULL);
    
    // 1. Выводим строку журнала при запуске (то, что мы убрали из log.c)
    // Макрос LOG_INF сам подставит метку [INF]
    LOG_INF("%s %s\n", DEVICE_NAME, FIRMWARE_VERSION);
    
    // 2. Инициализируем 5 секундомеров (даем им имена)
    profiling_stopwatch_init(STOPWATCH_LOOP,    "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED,     "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON,  "button");
    profiling_stopwatch_init(STOPWATCH_PI,      "pi");

    stdio_text_protocol_init();
    led_task_init(); 
    button_task_init();
    pi_task_init();

    // 3. Бесконечный суперцикл
    while (1)
    {
        // Начинаем замер времени всей итерации суперцикла
        profiling_start(STOPWATCH_LOOP);
        
        // --- ЗАДАЧА КОМАНД ---
        profiling_start(STOPWATCH_COMMAND);
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
        profiling_stop(STOPWATCH_COMMAND);

        // --- ЗАДАЧА СВЕТОДИОДА ---
        profiling_start(STOPWATCH_LED);
        led_task_handle();
        profiling_stop(STOPWATCH_LED);
        
        // --- ЗАДАЧА КНОПКИ ---
        profiling_start(STOPWATCH_BUTTON);
        button_task_handle();
        profiling_stop(STOPWATCH_BUTTON);

        // --- ЗАДАЧА РАСЧЕТА ПИ ---
        profiling_start(STOPWATCH_PI);
        pi_task_handle();
        profiling_stop(STOPWATCH_PI);
        
        // Останавливаем замер времени всей итерации суперцикла
        profiling_stop(STOPWATCH_LOOP);
    }
}