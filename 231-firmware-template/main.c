#include <stdio.h>
#include "pico/stdlib.h"

#include "firmware.h"
#include "device/device.h"
#include "logging/log.h"
#include "profiling/profiling.h"

#include "stdio-text-protocol/stdio-text-protocol.h"
#include "api.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"

// Требуемый тестом 2.3.5 перечисляемый тип с номерами секундомеров
typedef enum {
    STOPWATCH_LOOP,
    STOPWATCH_API,
    STOPWATCH_LED,
    STOPWATCH_BUTTON,
    STOPWATCH_PI,
    STOPWATCH_COUNT
} stopwatch_id_t;

int main() {
    stdio_init_all();
    
    device_info_t info;
    device_get_info(&info);
    LOG_INF("%s %s\n", FIRMWARE_NAME, FIRMWARE_VERSION);

    stdio_text_protocol_init();
    led_task_init();
    button_task_init();
    pi_task_init();
    
    profiling_init();

    // Инициализация секундомеров, которую ожидает тест 2.3.5
    profiling_stopwatch_init(STOPWATCH_LOOP,   "loop");
    profiling_stopwatch_init(STOPWATCH_API,    "api");
    profiling_stopwatch_init(STOPWATCH_LED,    "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI,     "pi");

    while (true) {
        profiling_start(STOPWATCH_LOOP);

        profiling_start(STOPWATCH_API);
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL) { 
            api_handle(command);
        }
        profiling_stop(STOPWATCH_API);

        profiling_start(STOPWATCH_LED);
        led_task_handle();
        profiling_stop(STOPWATCH_LED);

        profiling_start(STOPWATCH_BUTTON);
        button_task_handle();
        profiling_stop(STOPWATCH_BUTTON);

        profiling_start(STOPWATCH_PI);
        pi_task_handle();
        profiling_stop(STOPWATCH_PI);

        profiling_stop(STOPWATCH_LOOP);
    }
    return 0;
}