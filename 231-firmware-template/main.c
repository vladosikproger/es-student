#include <stdio.h>
#include "pico/stdlib.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"
// 1. Подключаем контракт нашей новой задачи
#include "led-task.h"

int main(void)
{
    stdio_init_all();
    setbuf(stdout, NULL);
    profiling_init();
    
    stdio_text_protocol_init();
    
    // 2. Инициализируем автомат светодиода (настраиваем ножки)
    led_task_init(); 

    while (1)
    {
        profiling_iteration();
        
        // 3. Выделяем процессорное время нашей задаче каждый такт суперцикла
        led_task_handle();
        
        // 4. Опрос терминала (ввод команд)
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
    }
}