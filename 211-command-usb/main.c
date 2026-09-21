#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "led.h"
#include "log.h"
#include "device.h"

const uint BUTTON_PIN = 15;
const uint DEBOUNCE_MS = 20;

#define LINE_SIZE 32

char line[LINE_SIZE];
uint line_length = 0;

bool get_button_debounce(uint pin)
{
    bool state1 = gpio_get(pin);
    sleep_ms(DEBOUNCE_MS);
    bool state2 = gpio_get(pin);
    return (state1 == state2) ? state1 : state2;
}

void handle_command(const char *command)
{
    if (strcmp(command, "enable") == 0)
    {
        led_set(true);
        LOG_INF("led %s\n", led_is_on() ? "on" : "off");
    }
    else if (strcmp(command, "disable") == 0)
    {
        led_set(false);
        LOG_INF("led %s\n", led_is_on() ? "on" : "off");
    }
    else if (strcmp(command, "version") == 0)
    {
        log_version();
    }
    else if (strcmp(command, "info") == 0)
    {
        device_info();
    }
    else
    {
        LOG_ERR("unknown command: %s\n", command);
    }
}

void read_line(void)
{
    int symbol = getchar_timeout_us(0);

    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return;
    }

    if (symbol == '\r' || symbol == '\n')
    {
        putchar('\n');
        line[line_length] = '\0';

        if (line_length > 0)
        {
            LOG_DBG("got %s\n", line);
            handle_command(line);
        }

        line_length = 0;
        return;
    }

    if (line_length + 1 < LINE_SIZE)
    {
        line[line_length] = (char)symbol;
        line_length = line_length + 1;
        putchar(symbol);
    }
}

int main()
{
    stdio_init_all();
    setbuf(stdout, NULL);

    led_init();
    log_version();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    bool previous_state = true;

    while (1)
    {
        bool current_state = get_button_debounce(BUTTON_PIN);

        if (previous_state == true && current_state == false)
        {
            led_toggle();
            LOG_INF("led %s\n", led_is_on() ? "on" : "off");
        }

        previous_state = current_state;

        read_line();

        sleep_ms(10);
    }
}