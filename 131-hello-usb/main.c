#include <stdio.h>
#include "pico/stdlib.h"

const uint LED_PIN = 25;
const uint BUTTON_PIN = 15;

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
    bool button_stable = true;
    
    printf("Ready\n");
    fflush(stdout);
    
    while (1)
    {
        bool reading = !gpio_get(BUTTON_PIN);
        
        if (reading && button_stable)
        {
            led_state = !led_state;
            gpio_put(LED_PIN, led_state);
            printf("led %s\n", led_state ? "on" : "off");
            fflush(stdout);
            
            button_stable = false;
            sleep_ms(300);
        }
        else if (!reading)
        {
            button_stable = true;
        }
        
        sleep_ms(20);
    }
}
