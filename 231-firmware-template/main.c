#include <stdio.h>
#include "pico/stdlib.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"

// 1. Подключаем контракты наших задач
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h" // ДОБАВЛЕНО: подключаем новый калькулятор

int main(void)
{
    stdio_init_all();
    setbuf(stdout, NULL);
    profiling_init();
    
    stdio_text_protocol_init();
    
    // 2. Подготовка к работе (делается один раз при включении в розетку)
    led_task_init(); 
    button_task_init();
    pi_task_init(); // ДОБАВЛЕНО: говорим калькулятору подготовить переменные

    // 3. Бесконечный суперцикл (процессор крутится здесь миллионы раз в секунду)
    while (1)
    {
        profiling_iteration();
        
        // По очереди даем микрофон каждой задаче
        led_task_handle();
        button_task_handle();
        pi_task_handle(); // ДОБАВЛЕНО: калькулятор считает свои 100 шагов и выходит
        
        // Опрос терминала (ввод команд)
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
    }
}