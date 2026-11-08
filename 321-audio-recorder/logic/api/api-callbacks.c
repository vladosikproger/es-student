#include "api/api-commands.h"
#include "api/api.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "button-task/button-task.h"
#include "firmware.h"
#include "led-task/led-task.h"
#include "pi-task/pi-task.h"
#include "telemetry-task/telemetry-task.h"
#include "block-task/block-task.h"
#include "platform/platform.h"

#include "device/device.h"
#include "profiling/profiling.h"
#include "systime/systime.h"
static bool parse_u32(const char *text, uint32_t *out_value)
{
    if (text == NULL || text[0] == '\0' || out_value == NULL)
    {
        return false;
    }

    // По ТЗ допускается только десятичная запись из цифр 0--9.
    // strtoul() сам по себе принимает ведущий знак '+', поэтому до вызова
    // strtoul() отдельно проверяем каждый символ.
    for (const char *p = text; *p != '\0'; ++p)
    {
        if (*p < '0' || *p > '9')
        {
            return false;
        }
    }

    char *endptr = NULL;
    errno = 0;
    unsigned long value = strtoul(text, &endptr, 10);

    if (errno == ERANGE || endptr == text || *endptr != '\0'
        || value > UINT32_MAX)
    {
        return false;
    }

    *out_value = (uint32_t)value;
    return true;
}

static void cmd_info(const command_t *cmd)
{
    (void)cmd;

    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", FIRMWARE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", FIRMWARE_PROJECT);
    printf("repo: %s\n", FIRMWARE_REPO);
    printf("board: %s\n", info.board_type);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)info.manufacturer,
           (unsigned)info.part,
           (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.sdk_version);
}

static void cmd_uptime(const command_t *cmd)
{
    (void)cmd;

    printf("uptime: %llu ms\n",
           (unsigned long long)systime_ms());
}

static void cmd_profiling(const command_t *cmd)
{
    (void)cmd;

#if !PROFILING_ENABLED
    printf("profiling is disabled in this build\n");
    return;
#endif

    profiling_result_t res;
    uint32_t id = 0;

    while (profiling_get(id, &res))
    {
        printf("%-10s count %-10lu mean %.2f us  max %lu us\n",
               res.name,
               (unsigned long)res.count,
               res.mean_us,
               (unsigned long)res.max_us);
        id++;
    }
}

static void cmd_profiling_reset(const command_t *cmd)
{
    (void)cmd;

    profiling_reset();
    printf("profiling reset\n");
}

static void cmd_pi_start(const command_t *cmd)
{
    uint32_t terms = 1000000;

    if (cmd->argc > 1)
    {
        printf("error: usage pi_start [terms]\n");
        return;
    }

    if (cmd->argc == 1)
    {
        if (!parse_u32(cmd->argv[0], &terms))
        {
            printf("error: usage pi_start [terms]\n");
            return;
        }

        if (terms == 0)
        {
            printf("error: terms must be greater than 0\n");
            return;
        }
    }

    if (pi_task_start(terms))
    {
        printf("pi: started, %lu terms\n", (unsigned long)terms);
    }
    else
    {
        printf("error: pi is busy\n");
    }
}

static void cmd_pi(const command_t *cmd)
{
    (void)cmd;

    double result = 0.0;
    uint32_t time_ms = 0;
    pi_state_t state = pi_task_get_result(&result, &time_ms);

    if (state == PI_STATE_NOT_STARTED)
    {
        printf("pi: not started\n");
    }
    else if (state == PI_STATE_RUNNING)
    {
        printf("pi: running\n");
    }
    else if (state == PI_STATE_READY)
    {
        printf("pi: %.9f in %lu ms\n",
               result,
               (unsigned long)time_ms);
    }
}

static void print_led_state(void)
{
    const char *state_str = "unknown";

    switch (led_task_get_state())
    {
        case LED_STATE_OFF:
            state_str = "off";
            break;

        case LED_STATE_ON:
            state_str = "on";
            break;

        case LED_STATE_BLINK:
            state_str = "blink";
            break;
    }

    printf("led: %s, period %lu ms\n",
           state_str,
           (unsigned long)led_task_get_period_ms());
}

static void cmd_led_enable(const command_t *cmd)
{
    (void)cmd;

    led_task_set_state(LED_STATE_ON);
    print_led_state();
}

static void cmd_led_disable(const command_t *cmd)
{
    (void)cmd;

    led_task_set_state(LED_STATE_OFF);
    print_led_state();
}

static void cmd_led_blink(const command_t *cmd)
{
    (void)cmd;

    led_task_set_state(LED_STATE_BLINK);
    print_led_state();
}

static void cmd_led_period(const command_t *cmd)
{
    uint32_t period;

    if (cmd->argc != 1 || !parse_u32(cmd->argv[0], &period))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    if (!led_task_set_period_ms(period))
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }

    print_led_state();
}

static void cmd_button(const command_t *cmd)
{
    (void)cmd;

    const char *state_1 = button_task_is_pressed(PLATFORM_BUTTON_1)
        ? "pressed" : "released";
    const char *state_2 = button_task_is_pressed(PLATFORM_BUTTON_2)
        ? "pressed" : "released";

    // Первая строка сохраняет прежний формат ответа для GP15.
    printf("button: %s, presses %lu\n",
           state_1,
           (unsigned long)button_task_get_press_count(PLATFORM_BUTTON_1));
    // Вторая строка показывает состояние и счётчик кнопки GP14.
    printf("button2: %s, presses %lu\n",
           state_2,
           (unsigned long)button_task_get_press_count(PLATFORM_BUTTON_2));
}

static void cmd_charge_on(const command_t *cmd)
{
    (void)cmd;

    platform_charge_set(true);
    printf("charge: on\n");
}

static void cmd_charge_off(const command_t *cmd)
{
    (void)cmd;

    platform_charge_set(false);
    printf("charge: off\n");
}

static void cmd_get_voltage(const command_t *cmd)
{
    (void)cmd;

    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }

    printf("voltage: %.3f V\n", platform_voltage_read());
}

static void cmd_get_temp(const command_t *cmd)
{
    (void)cmd;

    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }

    printf("temp: %.1f C\n", platform_temp_read());
}


#define TELEMETRY_DEFAULT_PERIOD_MS 100U

void api_print_telemetry(const telemetry_record_t *record)
{
    if (record == NULL)
    {
        return;
    }

    printf("tm: %llu %.3f %.1f\n",
           (unsigned long long)record->time_us,
           (double)record->voltage_v,
           (double)record->temp_c);
}

static void cmd_tm_start(const command_t *cmd)
{
    uint32_t period_ms = TELEMETRY_DEFAULT_PERIOD_MS;

    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }

    if (cmd->argc > 1)
    {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }

    if (cmd->argc == 1 && !parse_u32(cmd->argv[0], &period_ms))
    {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }

    if (!telemetry_task_start(period_ms))
    {
        printf("error: period_ms must be at least 10\n");
        return;
    }

    printf("telemetry: started, period %lu ms\n",
           (unsigned long)period_ms);
}

static void cmd_tm_stop(const command_t *cmd)
{
    (void)cmd;

    telemetry_task_stop();
    printf("telemetry: stopped\n");
}



static void cmd_cap_start(const command_t *cmd)
{
    uint32_t count;
    uint32_t period_us;

    if (cmd->argc != 2
        || !parse_u32(cmd->argv[0], &count)
        || !parse_u32(cmd->argv[1], &period_us))
    {
        printf("error: usage cap_start <N> <period_us>\n");
        return;
    }

    if (count == 0U || count > PLATFORM_CAPTURE_MAX || period_us < 10U)
    {
        printf("error: N must be 1..2000, period_us at least 10\n");
        return;
    }

    // Телеметрия тоже использует тот же АЦП, поэтому два режима не совмещаем.
    if (telemetry_task_get_state() != TELEMETRY_STATE_IDLE
        || !platform_capture_start(count, period_us))
    {
        printf("error: adc busy\n");
        return;
    }

    printf("capture: started, %lu samples, period %lu us\n",
           (unsigned long)count,
           (unsigned long)period_us);
}

static void cmd_cap_dump(const command_t *cmd)
{
    (void)cmd;

    if (!platform_capture_done())
    {
        printf("error: no capture\n");
        return;
    }

    platform_capture_t capture;
    platform_capture_get(&capture);

    printf("capture: %lu samples, period %lu us, interval %lu..%lu us, handler %.1f %%\n",
           (unsigned long)capture.count,
           (unsigned long)capture.period_us,
           (unsigned long)capture.min_interval_us,
           (unsigned long)capture.max_interval_us,
           (double)capture.handler_percent);

    for (uint32_t i = 0U; i < capture.count; ++i)
    {
        if ((i % 16U) == 0U)
        {
            printf("cap:");
        }

        printf(" %u", (unsigned)capture.samples[i]);

        if (((i + 1U) % 16U) == 0U || (i + 1U) == capture.count)
        {
            printf("\n");
        }
    }
}


static void print_dma_cap(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    if (samples == NULL || count == 0U)
    {
        return;
    }

    uint16_t min_value = UINT16_MAX;
    uint16_t max_value = 0U;
    uint64_t sum = 0U;

    for (uint32_t i = 0U; i < count; ++i)
    {
        const uint16_t value = samples[i];
        if (value < min_value)
        {
            min_value = value;
        }
        if (value > max_value)
        {
            max_value = value;
        }
        sum += value;
    }

    // Сначала расширяем count до uint64_t, чтобы умножение не переполнилось.
    const uint64_t expected_us =
        (uint64_t)count * 1000000ULL / PLATFORM_SAMPLE_RATE_HZ;
    const double mean_value = (double)sum / (double)count;

    printf("dma_cap: %lu samples in %lu us, expected %llu us, min %u max %u mean %.1f\n",
           (unsigned long)count,
           (unsigned long)time_us,
           (unsigned long long)expected_us,
           (unsigned)min_value,
           (unsigned)max_value,
           mean_value);
}

static void cmd_dma_cap(const command_t *cmd)
{
    uint32_t count;

    if (cmd->argc != 1 || !parse_u32(cmd->argv[0], &count))
    {
        printf("error: usage dma_cap <N>\n");
        return;
    }

    if (count == 0U || count > PLATFORM_BLOCK_MAX)
    {
        printf("error: N must be 1..8192\n");
        return;
    }

    // Телеметрия и захват блока используют один и тот же аппаратный АЦП.
    if (telemetry_task_get_state() != TELEMETRY_STATE_IDLE
        || !block_task_start(count, print_dma_cap))
    {
        printf("error: adc busy\n");
        return;
    }
}

static void cmd_timer_blink_start(const command_t *cmd)
{
    uint32_t period_ms;

    if (cmd->argc != 1 || !parse_u32(cmd->argv[0], &period_ms))
    {
        printf("error: usage timer_blink_start <period_ms>\n");
        return;
    }

    if (!platform_blink_start(period_ms))
    {
        printf("error: period_ms must be 10..60000\n");
        return;
    }

    // LED теперь принадлежит таймерному callback.
    // Состояние OFF заставляет led_task_handle() перестать трогать GPIO25.
    led_task_set_state(LED_STATE_OFF);

    printf("timer_blink: started, period %lu ms\n", (unsigned long)period_ms);
}

static void cmd_timer_blink_stop(const command_t *cmd)
{
    (void)cmd;

    const uint32_t toggles = platform_blink_stop();
    printf("timer_blink: stopped, %lu toggles\n", (unsigned long)toggles);
}

const api_command_t api_commands[] =
{
    { "info",            "device passport",                 cmd_info },
    { "uptime",          "system uptime in ms",             cmd_uptime },
    { "led_enable",      "turn LED on",                     cmd_led_enable },
    { "led_disable",     "turn LED off",                    cmd_led_disable },
    { "led_blink",       "blink LED",                       cmd_led_blink },
    { "led_period",      "set LED blink period in ms",      cmd_led_period },
    { "button",          "button state and press count",    cmd_button },
    { "get_voltage",     "voltage on GP26, ADC input 0",     cmd_get_voltage },
    { "get_temp",        "chip temperature, degrees Celsius", cmd_get_temp },
    { "charge_on",       "set GP22 high; charge RC capacitor", cmd_charge_on },
    { "charge_off",      "set GP22 low; discharge RC capacitor", cmd_charge_off },
    { "tm_start",        "start telemetry stream [period_ms]", cmd_tm_start },
    { "tm_stop",         "stop telemetry stream",             cmd_tm_stop },
    { "pi_start",        "start Pi calculation",            cmd_pi_start },
    { "pi",              "get Pi status or result",         cmd_pi },
    { "profiling",       "print profiling statistics",      cmd_profiling },
    { "profiling_reset", "reset profiling statistics",      cmd_profiling_reset },
    { "timer_blink_start", "blink LED by timer: timer_blink_start <period_ms>", cmd_timer_blink_start },
    { "timer_blink_stop",  "stop timer blink, number of toggles", cmd_timer_blink_stop },
    { "cap_start",         "capture GP26 by timer: cap_start <N> <period_us>", cmd_cap_start },
    { "cap_dump",          "print last timer capture", cmd_cap_dump },
    { "dma_cap",           "capture a block using ADC and DMA: dma_cap <N>", cmd_dma_cap }
};

const uint32_t api_command_count =
    sizeof(api_commands) / sizeof(api_commands[0]);