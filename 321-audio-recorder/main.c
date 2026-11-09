#include <stdbool.h>
#include <stdio.h>

#include "firmware.h"
#include "logging/log.h"
#include "profiling/profiling.h"
#include "stdio-text-protocol/stdio-text-protocol.h"

#include "platform/platform.h"
#include "api/api.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "telemetry-task/telemetry-task.h"
#include "block-task/block-task.h"
#include "stream-task/stream-task.h"

// Требуемый тестом 2.3.5 перечисляемый тип с номерами секундомеров
typedef enum {
    STOPWATCH_LOOP,
    STOPWATCH_COMMAND,
    STOPWATCH_LED,
    STOPWATCH_BUTTON,
    STOPWATCH_PI,
    STOPWATCH_TELEMETRY,
    STOPWATCH_STREAM,
    STOPWATCH_COUNT
} stopwatch_id_t;

int main(void)
{
    platform_init();

    LOG_INF("%s %s\n", FIRMWARE_NAME, FIRMWARE_VERSION);

    stdio_text_protocol_init();
    led_task_init();
    // Первая кнопка: GP15. Короткое нажатие меняет состояние LED,
    // удержание дольше 1 секунды принудительно включает мигание.
    button_task_init(PLATFORM_BUTTON_1, led_task_next_state, led_task_blink);
    // Вторая кнопка: GP14. Короткое нажатие меняет период мигания.
    button_task_init(PLATFORM_BUTTON_2, led_task_next_period, NULL);
    pi_task_init();
    telemetry_task_init(api_print_telemetry);
    stream_task_init(api_send_frame);

    profiling_init();

    // Инициализация секундомеров, которую ожидает тест 2.3.5
    profiling_stopwatch_init(STOPWATCH_LOOP,   "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED,    "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI,     "pi");
    profiling_stopwatch_init(STOPWATCH_TELEMETRY, "telemetry");
    profiling_stopwatch_init(STOPWATCH_STREAM, "stream");

    while (true) {
        profiling_start(STOPWATCH_LOOP);

        profiling_start(STOPWATCH_COMMAND);
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL) {
            api_handle(command);
        }
        profiling_stop(STOPWATCH_COMMAND);

        profiling_start(STOPWATCH_LED);
        led_task_handle();
        profiling_stop(STOPWATCH_LED);

        profiling_start(STOPWATCH_BUTTON);
        button_task_handle();
        profiling_stop(STOPWATCH_BUTTON);

        profiling_start(STOPWATCH_PI);
        pi_task_handle();
        profiling_stop(STOPWATCH_PI);

        profiling_start(STOPWATCH_TELEMETRY);
        telemetry_task_handle();
        profiling_stop(STOPWATCH_TELEMETRY);

        block_task_handle();

        profiling_start(STOPWATCH_STREAM);
        stream_task_handle();
        profiling_stop(STOPWATCH_STREAM);

        // Кормим watchdog только после завершения ВСЕХ задач.
        // Если любая задача зависнет, выполнение сюда не дойдёт и плата сбросится.
        platform_watchdog_feed();

        profiling_stop(STOPWATCH_LOOP);
    }
    return 0;
}