#include "api/api-commands.h"

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

    const char *state_str =
        button_task_is_pressed() ? "pressed" : "released";

    uint32_t count = button_task_get_press_count();

    printf("button: %s, presses %lu\n",
           state_str,
           (unsigned long)count);
}

static void cmd_get_voltage(const command_t *cmd)
{
    (void)cmd;

    printf("voltage: %.3f V\n", platform_voltage_read());
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
    { "pi_start",        "start Pi calculation",            cmd_pi_start },
    { "pi",              "get Pi status or result",         cmd_pi },
    { "profiling",       "print profiling statistics",      cmd_profiling },
    { "profiling_reset", "reset profiling statistics",      cmd_profiling_reset }
};

const uint32_t api_command_count =
    sizeof(api_commands) / sizeof(api_commands[0]);