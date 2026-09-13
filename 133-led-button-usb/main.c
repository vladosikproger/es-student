#include <stdio.h>
#include "pico/stdlib.h"

const uint LED_PIN = 25;
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

// Функция включения/выключения светодиода
void set_led(bool on)
{
    gpio_put(LED_PIN, on);
    printf("led %s\n", on ? "on" : "off");
    fflush(stdout);
}

// Функция разбора команды
bool handle_command(int command, bool led)
{
    if (command == 'e')
    {
        led = true;
        set_led(led);
    }
    else if (command == 'd')
    {
        led = false;
        set_led(led);
    }
    else
    {
        printf("unknown command: %c\n", command);
        fflush(stdout);
    }
    
    return led;
}

int main()
{
    stdio_init_all();
    setbuf(stdout, NULL);
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    
    bool led_state = false;
    bool previous_state = true;
    
    while (1)
    {
        // 1. Опрос кнопки
        bool current_state = get_button_debounce(BUTTON_PIN);
        
        if (previous_state == true && current_state == false)
        {
            led_state = !led_state;
            set_led(led_state);
        }
        
        previous_state = current_state;
        
        // 2. Приём команды из USB
        int command = getchar_timeout_us(0);
        
        if (command == PICO_ERROR_TIMEOUT)
        {
            continue;  // Символа нет — идём на новый круг
        }
        
        led_state = handle_command(command, led_state);
        
        sleep_ms(10);
    }
}
