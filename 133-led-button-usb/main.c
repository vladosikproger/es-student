#include <stdio.h>
#include "pico/stdlib.h"

const uint LED_PIN = 25;
const uint BUTTON_PIN = 15;
const uint DEBOUNCE_MS = 20;

bool get_button_debounce(uint pin)
{
    bool state1 = gpio_get(pin);
    sleep_ms(DEBOUNCE_MS);
    bool state2 = gpio_get(pin);
    return (state1 == state2) ? state1 : state2;
}

int main()
{
    stdio_init_all();
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
    
    bool led_state = false;
    bool previous_state = true;
    
    while (1)
    {
        bool current_state = get_button_debounce(BUTTON_PIN);
        
        if (previous_state == true && current_state == false)
        {
            led_state = !led_state;
            gpio_put(LED_PIN, led_state);
            printf("led %s\n", led_state ? "on" : "off");
            fflush(stdout);
        }
        
        previous_state = current_state;
        sleep_ms(10);
    }
}
