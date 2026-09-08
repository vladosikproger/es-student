// добавляем заголовочный файл функций ввода-вывода
#include "pico/stdio.h"

// добавляем заголовочный файл функций работы с GPIO
#include "hardware/gpio.h"
#include "pico/stdlib.h"
// объявляем константу вывода светодиода
const uint LED_PIN = 25;

// объявляем константу вывода кнопки
const uint BUTTON_PIN = 15;

// константа задержки для защиты от дребезга
const uint DEBOUNCE_MS = 20;

// функция чтения кнопки с защитой от дребезга
bool get_button_debounce(uint pin)
{
    bool state = gpio_get(pin);
    sleep_ms(DEBOUNCE_MS);
    return state && gpio_get(pin);
}

int main()
{
    // инициализируем стандартный ввод-вывод
    stdio_init_all();
    
    // инициализируем пин светодиода
    gpio_init(LED_PIN);
    
    // настраиваем пин светодиода на выход
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // инициализируем пин кнопки
    gpio_init(BUTTON_PIN);
    
    // настраиваем пин кнопки на вход
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    
    // включаем подтягивающий резистор
    gpio_pull_up(BUTTON_PIN);
    
    // переменная: светится ли светодиод
    bool led = false;
    
    // переменная: что было на прошлом шаге цикла
    bool previous = false;
    
    // бесконечный цикл
    while (1)
    {
        // читаем текущее состояние кнопки (с защитой от дребезга!)
        bool current = get_button_debounce(BUTTON_PIN);
        
        // проверяем: если было true и стало false (кнопку нажали)
        if (previous == true && current == false)
        {
            // переключаем светодиод
            led = !led;
            
            // устанавливаем новое состояние светодиода
            gpio_put(LED_PIN, led);
        }
        
        // запоминаем текущее состояние для следующего цикла
        previous = current;
    }
}