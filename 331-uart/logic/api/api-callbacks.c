#include "api/api-commands.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "button-task/button-task.h"
#include "block-task/block-task.h"
#include "edge-task/edge-task.h"
#include "uart-task/uart-task.h"
#include "firmware.h"
#include "led-task/led-task.h"
#include "pi-task/pi-task.h"

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

static void print_edge(platform_line_mode_t mode, bool ok,
                       uint32_t rise_us, uint32_t fall_us)
{
    const char *name = mode == PLATFORM_LINE_PUSH_PULL ? "pp" : "od";
    if (!ok)
    {
        printf("edge %s: timeout\n", name);
        return;
    }
    printf("edge %s: rise %.1f ms, fall %.1f ms\n",
           name, rise_us / 1000.0, fall_us / 1000.0);
}

static void cmd_edge(const command_t *cmd)
{
    platform_line_mode_t mode;
    if (cmd->argc == 1 && strcmp(cmd->argv[0], "pp") == 0)
    {
        mode = PLATFORM_LINE_PUSH_PULL;
    }
    else if (cmd->argc == 1 && strcmp(cmd->argv[0], "od") == 0)
    {
        mode = PLATFORM_LINE_OPEN_DRAIN;
    }
    else
    {
        printf("error: usage edge pp|od\n");
        return;
    }

    if (!edge_task_start(mode, print_edge))
    {
        printf("error: edge is busy\n");
    }
}

// У RX есть место для 32 байтов; читаем их после окончания передачи.
#define ECHO_TEXT_MAX 32u

static void print_bytes(const uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        const unsigned char byte = data[i];
        putchar(isprint(byte) ? byte : '.');
    }
}

static void print_errors(uint32_t errors)
{
    printf("err:");
    if (errors == 0)
    {
        printf(" none");
        return;
    }
    if (errors & PLATFORM_UART_FE) printf(" FE");
    if (errors & PLATFORM_UART_PE) printf(" PE");
    if (errors & PLATFORM_UART_BE) printf(" BE");
    if (errors & PLATFORM_UART_OE) printf(" OE");
}

// Пока USB-команда ждёт ответа (для link — и обратного), не меняем связь.
static bool uart_command_pending;

static void note_start(bool accepted)
{
    if (accepted) uart_command_pending = true;
    else puts("error: uart is busy");
}

static bool may_reconfigure_uart(void)
{
    if (!uart_command_pending) return true;
    puts("error: uart is busy");
    return false;
}

static void print_received(const uint8_t *data, size_t size, uint32_t errors)
{
    print_bytes(data, size);
    putchar(' ');
    print_errors(errors);
    putchar('\n');
}

static void print_echo(const uint8_t *data, size_t size, uint32_t errors)
{
    uart_command_pending = false;
    printf("rx: ");
    print_received(data, size, errors);
}

static void cmd_loop(const command_t *cmd)
{
    bool internal;
    if (cmd->argc == 1 && strcmp(cmd->argv[0], "internal") == 0)
        internal = true;
    else if (cmd->argc == 1 && strcmp(cmd->argv[0], "pins") == 0)
        internal = false;
    else
    {
        printf("error: usage loop internal|pins\n");
        return;
    }

    if (!may_reconfigure_uart()) return;
    platform_uart_loop_set(internal);
    printf("loop: %s\n", cmd->argv[0]);
}

// Одна проверка для echo и link. Не передаём завершающий '\0'.
static bool read_word(const command_t *cmd, const char *usage, size_t *size)
{
    if (cmd->argc != 1)
    {
        puts(usage);
        return false;
    }
    *size = strlen(cmd->argv[0]);
    if (*size > ECHO_TEXT_MAX)
    {
        puts("error: text longer than 32");
        return false;
    }
    return true;
}

static void cmd_echo(const command_t *cmd)
{
    size_t size;
    if (!read_word(cmd, "error: usage echo <text>", &size)) return;
    note_start(uart_task_start(PLATFORM_UART0, PLATFORM_UART0,
                              (const uint8_t *)cmd->argv[0], size, print_echo));
}

static const char text_line[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
_Static_assert(sizeof(text_line) - 1 == ECHO_TEXT_MAX, "text must fit one RX FIFO");
#define REFERENCE_BAUD 115200u

static void print_link_back(const uint8_t *data, size_t size, uint32_t errors)
{
    uart_command_pending = false;
    printf("1->0: ");
    print_received(data, size, errors);
}

static void print_link_forth(const uint8_t *data, size_t size, uint32_t errors)
{
    printf("0->1: ");
    print_received(data, size, errors);
    // uart-task уже свободна и сразу копирует именно ПРИНЯТЫЕ байты.
    uart_command_pending = false;
    note_start(uart_task_start(PLATFORM_UART1, PLATFORM_UART0, data, size, print_link_back));
}

static void cmd_link(const command_t *cmd)
{
    size_t size;
    if (!read_word(cmd, "error: usage link <text>", &size)) return;
    note_start(uart_task_start(PLATFORM_UART0, PLATFORM_UART1,
                              (const uint8_t *)cmd->argv[0], size, print_link_forth));
}

static void print_text(const uint8_t *data, size_t size, uint32_t errors)
{
    uart_command_pending = false;
    printf("text: %u chars ", (unsigned)size);
    print_received(data, size, errors);
}

static void cmd_text(const command_t *cmd)
{
    if (cmd->argc != 0)
    {
        puts("error: usage text");
        return;
    }
    note_start(uart_task_start(PLATFORM_UART1, PLATFORM_UART0,
                              (const uint8_t *)text_line, sizeof(text_line) - 1, print_text));
}

static void cmd_baud_shift(const command_t *cmd)
{
    const char *arg = (cmd->argc == 1) ? cmd->argv[0] : "";
    char *end = NULL;
    errno = 0;
    const long percent = strtol(arg, &end, 10);
    if (end == arg || *end != '\0' || isspace((unsigned char)arg[0]))
    {
        puts("error: usage baud_shift <percent>");
        return;
    }
    if (errno == ERANGE || percent < -50 || percent > 50)
    {
        puts("error: percent must be -50..50");
        return;
    }
    if (!may_reconfigure_uart()) return;
    const uint32_t requested = REFERENCE_BAUD * (uint32_t)(100 + percent) / 100u;
    const uint32_t actual = platform_uart_baud_set(PLATFORM_UART1, requested);
    printf("baud_shift: uart1 %lu baud\n", (unsigned long)actual);
}

static bool parse_parity(const char *word, platform_parity_t *out)
{
    if (strcmp(word, "none") == 0)      *out = PLATFORM_PARITY_NONE;
    else if (strcmp(word, "even") == 0) *out = PLATFORM_PARITY_EVEN;
    else if (strcmp(word, "odd") == 0)  *out = PLATFORM_PARITY_ODD;
    else return false;
    return true;
}

static void cmd_parity(const command_t *cmd)
{
    platform_parity_t p0, p1;
    if (cmd->argc != 2 || !parse_parity(cmd->argv[0], &p0) || !parse_parity(cmd->argv[1], &p1))
    {
        puts("error: usage parity none|even|odd none|even|odd");
        return;
    }
    if (!may_reconfigure_uart()) return;
    platform_uart_parity_set(PLATFORM_UART0, p0);
    platform_uart_parity_set(PLATFORM_UART1, p1);
    printf("parity: uart0 %s, uart1 %s\n", cmd->argv[0], cmd->argv[1]);
}

// Приём блока длиной вдвое больше аппаратной RX FIFO.
#define BLOCK_SIZE 64u
static uint8_t block_data[BLOCK_SIZE];
static bool block_irq_enabled;

static void print_block(const uint8_t *data, size_t size, uint32_t errors)
{
    // Восстанавливаем обычный опрос, чтобы не менять поведение п3.3.2–3.3.3.
    platform_uart_rx_irq_set(false);
    uart_command_pending = false;

    const bool match = size > 0 && size <= BLOCK_SIZE &&
                       memcmp(data, block_data, size) == 0;
    printf("block %s: %u/64 %s, ",
           block_irq_enabled ? "irq" : "poll",
           (unsigned)size,
           match ? "match" : "mismatch");
    print_errors(errors);
    putchar('\n');
}

static void cmd_block(const command_t *cmd)
{
    if (cmd->argc != 1 ||
        (strcmp(cmd->argv[0], "irq") != 0 &&
         strcmp(cmd->argv[0], "poll") != 0))
    {
        puts("error: usage block irq|poll");
        return;
    }

    if (!may_reconfigure_uart()) return;

    const bool use_irq = strcmp(cmd->argv[0], "irq") == 0;
    for (size_t i = 0; i < BLOCK_SIZE; ++i)
    {
        block_data[i] = (uint8_t)i;
    }

    // После uart_task_start() приёмник будет очищен, до начала передачи.
    platform_uart_rx_irq_set(false);
    if (!uart_task_start(PLATFORM_UART1, PLATFORM_UART0,
                         block_data, BLOCK_SIZE, print_block))
    {
        puts("error: uart is busy");
        return;
    }

    block_irq_enabled = use_irq;
    uart_command_pending = true;
    platform_uart_rx_irq_set(use_irq);
}

// Oscilloscope: capture GP0 from GP26 before transmitting UART0->UART1.
#define SCOPE_SAMPLES PLATFORM_BLOCK_MAX
#define SCOPE_BAUD_MIN 9600u
#define SCOPE_TEXT_MAX 4u

static uint32_t scope_baud;
static uint8_t scope_rx_data[SCOPE_TEXT_MAX];
static size_t scope_rx_size;
static uint32_t scope_rx_errors;
static bool scope_rx_done;
static bool scope_capture_valid;

static void print_scope_rx(const uint8_t *data, size_t size, uint32_t errors)
{
    // Called after uart-task's quiet period, before the 8.2 ms ADC block ends.
    scope_rx_size = size < SCOPE_TEXT_MAX ? size : SCOPE_TEXT_MAX;
    memcpy(scope_rx_data, data, scope_rx_size);
    scope_rx_errors = errors;
    scope_rx_done = true;
}

static void print_scope(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    (void)time_us;
    if (!scope_capture_valid)
    {
        return;  // UART task did not start; failure was already reported.
    }

    // Restore normal speeds before emitting a large USB text response.
    platform_uart_baud_set(PLATFORM_UART0, REFERENCE_BAUD);
    platform_uart_baud_set(PLATFORM_UART1, REFERENCE_BAUD);

    printf("scope: %lu samples, %lu Hz, %lu baud, uart1 ",
           (unsigned long)count,
           (unsigned long)PLATFORM_SAMPLE_RATE_HZ,
           (unsigned long)scope_baud);
    if (scope_rx_done)
    {
        print_bytes(scope_rx_data, scope_rx_size);
    }
    else
    {
        printf("<missing>");
    }
    putchar(' ');
    print_errors(scope_rx_errors);
    putchar('\n');

    for (uint32_t i = 0; i < count; ++i)
    {
        printf("%u", (unsigned)samples[i]);
        putchar((i % 16u == 15u || i + 1u == count) ? '\n' : ' ');
    }
    puts("scope: end");
    scope_capture_valid = false;
    uart_command_pending = false;
}

static void cmd_scope(const command_t *cmd)
{
    uint32_t baud;
    if (cmd->argc != 2 || !parse_u32(cmd->argv[0], &baud) ||
        baud < SCOPE_BAUD_MIN || baud > REFERENCE_BAUD ||
        cmd->argv[1][0] == '\0' || strlen(cmd->argv[1]) > SCOPE_TEXT_MAX)
    {
        puts("error: usage scope <baud> <text>");
        return;
    }

    if (!may_reconfigure_uart()) return;

    scope_baud = baud;
    scope_rx_size = 0u;
    scope_rx_errors = 0u;
    scope_rx_done = false;
    scope_capture_valid = true;

    platform_uart_rx_irq_set(false);
    platform_uart_baud_set(PLATFORM_UART0, baud);
    platform_uart_baud_set(PLATFORM_UART1, baud);

    // Starting ADC first captures idle HIGH before the UART start bit.
    if (!block_task_start(SCOPE_SAMPLES, print_scope))
    {
        scope_capture_valid = false;
        platform_uart_baud_set(PLATFORM_UART0, REFERENCE_BAUD);
        platform_uart_baud_set(PLATFORM_UART1, REFERENCE_BAUD);
        puts("error: adc busy");
        return;
    }

    if (!uart_task_start(PLATFORM_UART0, PLATFORM_UART1,
                         (const uint8_t *)cmd->argv[1],
                         strlen(cmd->argv[1]), print_scope_rx))
    {
        scope_capture_valid = false;
        platform_uart_baud_set(PLATFORM_UART0, REFERENCE_BAUD);
        platform_uart_baud_set(PLATFORM_UART1, REFERENCE_BAUD);
        puts("error: uart is busy");
        return;
    }
    uart_command_pending = true;
}

const api_command_t api_commands[] =
{
    { "info",            "device passport",                 cmd_info },
    { "uptime",          "system uptime in ms",             cmd_uptime },
    { "pi_start",        "start Pi calculation",            cmd_pi_start },
    { "pi",              "get Pi status or result",         cmd_pi },
    { "profiling",       "print profiling statistics",      cmd_profiling },
    { "profiling_reset", "reset profiling statistics",      cmd_profiling_reset },
    { "led_enable",      "turn LED on",                     cmd_led_enable },
    { "led_disable",     "turn LED off",                    cmd_led_disable },
    { "led_blink",       "blink LED",                       cmd_led_blink },
    { "led_period",      "set LED blink period in ms",      cmd_led_period },
    { "button",          "button state and press count",    cmd_button },
    { "edge", "rise and fall of the RC line: edge pp|od", cmd_edge },
    { "loop", "UART0 loopback: loop internal|pins", cmd_loop },
    { "echo", "text through UART0 and back: echo <text>", cmd_echo },
    { "link", "text from UART0 to UART1 and back: link <text>", cmd_link },
    { "baud_shift", "shift UART1 baud rate: baud_shift <percent>", cmd_baud_shift },
    { "text", "32 characters from UART1 to UART0", cmd_text },
    { "parity", "parity of UART0 and UART1: parity <rx> <tx>", cmd_parity },
    { "block", "64 bytes from UART1 to UART0: block irq|poll", cmd_block },
    { "scope", "UART0 line on ADC: scope <baud> <text>", cmd_scope }
};

const uint32_t api_command_count =
    sizeof(api_commands) / sizeof(api_commands[0]);