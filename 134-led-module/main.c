#include <stdio.h>
#include "pico/stdlib.h"
#include "led.h"

const uint BUTTON_PIN = 15;
const uint DEBOUNCE_MS = 20;

// Функция защиты от дребезга кнопки
bool get_button_debounce(uint pin)
{
    bool state1 = gpio_get(pin);
    sleep_ms(DEBOUNCE_MS);
    bool state2 = gpio_get(pin);
    return (state1 == state2) ? state1 : state2;
}

// Функция разбора команды
void handle_command(int command)
{
    if (command == 'e')
    {
        led_set(true);
        printf("led %s\n", led_is_on() ? "on" : "off");
        fflush(stdout);
    }
    else if (command == 'd')
    {
        led_set(false);
        printf("led %s\n", led_is_on() ? "on" : "off");
        fflush(stdout);
    }
    else
    {
        printf("unknown command: %c\n", command);
        fflush(stdout);
    }
}

int main()
{
    stdio_init_all();
    setbuf(stdout, NULL);

    led_init();

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    bool previous_state = true;

    while (1)
    {
        // 1. Опрос кнопки
        bool current_state = get_button_debounce(BUTTON_PIN);

        if (previous_state == true && current_state == false)
        {
            led_toggle();
            printf("led %s\n", led_is_on() ? "on" : "off");
            fflush(stdout);
        }

        previous_state = current_state;

        // 2. Приём команды из USB
        int command = getchar_timeout_us(0);

        if (command == PICO_ERROR_TIMEOUT)
        {
            continue;
        }

        handle_command(command);

        sleep_ms(10);
    }
}