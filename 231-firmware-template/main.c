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

    uint32_t STOPWATCH_LOOP   = profiling_allocate("loop");
    uint32_t STOPWATCH_API    = profiling_allocate("api");
    uint32_t STOPWATCH_LED    = profiling_allocate("led");
    uint32_t STOPWATCH_BUTTON = profiling_allocate("button");
    uint32_t STOPWATCH_PI     = profiling_allocate("pi");

    // РўСЂСЋРє РґР»СЏ РїРѕРґР°РІР»РµРЅРёСЏ РїСЂРµРґСѓРїСЂРµР¶РґРµРЅРёР№ unused-variable.
    // Р­С‚Рѕ РёРЅСЃС‚СЂСѓРєС†РёСЏ "РІС…РѕР»РѕСЃС‚СѓСЋ". РћРЅР° СѓСЃРїРѕРєР°РёРІР°РµС‚ СЃС‚СЂРѕРіРёР№ РєРѕРјРїРёР»СЏС‚РѕСЂ, 
    // РЅРѕ С„РёР·РёС‡РµСЃРєРё РЅРµ СЃРѕР·РґР°РµС‚ РЅРёРєР°РєРёС… РєРѕРјР°РЅРґ РІ РїР°РјСЏС‚Рё РјРёРєСЂРѕРєРѕРЅС‚СЂРѕР»Р»РµСЂР°.
    (void)STOPWATCH_LOOP;
    (void)STOPWATCH_API;
    (void)STOPWATCH_LED;
    (void)STOPWATCH_BUTTON;
    (void)STOPWATCH_PI;

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
