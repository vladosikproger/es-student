#include "api.h"
#include "pico/stdlib.h"
#include "device.h"
#include "profiling.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // Для функции strtoul
#include "led-task.h"
#include "button-task.h"

// 1. Описание типа обработчика и структуры таблицы
typedef struct {
    const char *name;
    const char *help;
    void (*callback)(const command_t *);
} api_command_t;

// 2. Безопасный парсер чисел (Альтернатива тяжелому sscanf)
static bool parse_u32(const char *text, uint32_t *out_value)
{
    char *endptr;
    uint32_t value = strtoul(text, &endptr, 10);

    if (endptr == text || *endptr != '\0')
    {
        return false;
    }

    *out_value = value;
    return true;
}

// Глобальная переменная для математики
volatile double pi_result;

// -------------------------------------------------------------
// БИЗНЕС-ЛОГИКА (Математика)
// -------------------------------------------------------------
static double calc_pi(uint32_t terms)
{
    double sum = 0.0;
    double sign = 1.0;
    for (uint32_t k = 0; k < terms; k++)
    {
        sum += sign / (2.0 * k + 1.0);
        sign = -sign;
    }
    return sum * 4.0;
}

// -------------------------------------------------------------
// ОБРАБОТЧИКИ
// -------------------------------------------------------------
static void cmd_info(const command_t *cmd)
{
    printf("%s %s\n", PROJECT_NAME, FIRMWARE_VERSION);
    device_info();
}

static void cmd_uptime(const command_t *cmd)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

static void cmd_main_time_exec(const command_t *cmd)
{
    printf("iteration avg %.2f us, max %u us\n", profiling_avg_us(), (unsigned)profiling_max_us());
}

static void cmd_main_time_reset(const command_t *cmd)
{
    profiling_reset_max();
    printf("max reset\n");
}

static void cmd_calc_pi(const command_t *cmd)
{
    uint32_t terms = 1000000;

    if (cmd->argc > 1)
    {
        printf("error: usage calc_pi [terms]\n");
        return;
    }

    if (cmd->argc == 1)
    {
        if (!parse_u32(cmd->argv[0], &terms))
        {
            printf("error: usage calc_pi [terms]\n");
            return;
        }
        
        if (terms == 0)
        {
            printf("error: terms must be strictly positive\n");
            return;
        }
    }

    uint64_t start_us = time_us_64();
    pi_result = calc_pi(terms);
    uint64_t spent_us = time_us_64() - start_us;

    printf("pi: %.9f\n", pi_result);
    printf("time: %llu ms\n", spent_us / 1000);
}

// --- ОБЩАЯ ФУНКЦИЯ ОТВЕТА СВЕТОДИОДА ---
static void print_led_state(void)
{
    const char *state_str = "unknown";
    switch (led_task_get_state())
    {
        case LED_STATE_OFF:   state_str = "off"; break;
        case LED_STATE_ON:    state_str = "on"; break;
        case LED_STATE_BLINK: state_str = "blink"; break;
    }
    
    printf("led: %s, period %lu ms\n", state_str, led_task_get_period_ms());
}

// --- ОБРАБОТЧИКИ КОМАНД СВЕТОДИОДА ---
static void cmd_led_enable(const command_t *cmd)
{
    led_task_set_state(LED_STATE_ON);
    print_led_state();
}

static void cmd_led_disable(const command_t *cmd)
{
    led_task_set_state(LED_STATE_OFF);
    print_led_state();
}

static void cmd_led_blink(const command_t *cmd)
{
    led_task_set_state(LED_STATE_BLINK);
    print_led_state();
}

static void cmd_led_period(const command_t *cmd)
{
    uint32_t period;

    if (cmd->argc != 1)
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    if (!parse_u32(cmd->argv[0], &period))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    if (!led_task_set_period_ms(period))
    {
        printf("error: period must be strictly positive\n");
        return;
    }

    print_led_state();
}

// --- НОВЫЙ ОБРАБОТЧИК КНОПКИ ---
static void cmd_button(const command_t *cmd)
{
    const char *state_str = button_task_is_pressed() ? "pressed" : "released";
    uint32_t count = button_task_get_press_count();
    
    printf("button: %s, presses %lu\n", state_str, (unsigned long)count);
}

// -------------------------------------------------------------
// ТАБЛИЦА И ДИСПЕТЧЕР
// -------------------------------------------------------------
static const api_command_t commands[] = {
    { "info",            "Print device info",                         cmd_info },
    { "uptime",          "Print system uptime in ms",                 cmd_uptime },
    { "calc_pi",         "Calculate Pi. Usage: calc_pi [terms]",      cmd_calc_pi },
    { "main_time_exec",  "Print main loop iteration time",            cmd_main_time_exec },
    { "main_time_reset", "Reset max iteration time",                  cmd_main_time_reset },
    { "led_enable",      "Turn LED on",                               cmd_led_enable },
    { "led_disable",     "Turn LED off",                              cmd_led_disable },
    { "led_blink",       "Blink LED",                                 cmd_led_blink },
    { "led_period",      "Set LED blink period in ms",                cmd_led_period },
    { "button",          "Print button state and press count",        cmd_button }
};
static const uint32_t command_count = sizeof(commands) / sizeof(commands[0]);

void api_handle(const command_t *command)
{
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }

    if (strcmp(command->name, "help") == 0)
    {
        printf("help\n"); 
        for (uint32_t i = 0; i < command_count; i++)
        {
            printf("%-16s %s\n", commands[i].name, commands[i].help);
        }
        return;
    }

    for (uint32_t i = 0; i < command_count; i++)
    {
        if (strcmp(command->name, commands[i].name) == 0)
        {
            commands[i].callback(command);
            return;
        }
    }

    printf("error: unknown command '%s', try help\n", command->name);
}