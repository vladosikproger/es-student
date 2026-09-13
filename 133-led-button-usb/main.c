// 1. Подключаем заголовочные файлы
#include "pico/stdlib.h"      // Включает в себя и stdio.h, и hardware/gpio.h

// 2. Объявляем константы
const uint LED_PIN = 25;
const uint BUTTON_PIN = 15;
const uint DEBOUNCE_MS = 20;

// 3. Функция чтения кнопки с защитой от дребезга
bool get_button_debounce(uint pin)
{
    bool state1 = gpio_get(pin);       // Первое чтение состояния
    sleep_ms(DEBOUNCE_MS);             // Ждём 20 мс, пока механический дребезг утихнет
    bool state2 = gpio_get(pin);       // Второе чтение состояния
    
    // Если оба чтения совпали, значит сигнал стабилен. Возвращаем его.
    return (state1 == state2) ? state1 : state2;
}

// 4. Функция управления светодиодом и вывода сообщения (как в задании)
void set_led(bool on)
{
    gpio_put(LED_PIN, on);
    printf("led %s\n", on ? "on" : "off");
}

int main()
{
    // 5. Инициализация
    stdio_init_all(); // Включаем USB-связь для printf
    
    // Настраиваем светодиод
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Настраиваем кнопку
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Включаем внутреннюю подтяжку к 3.3В
    
    // 6. Переменные состояния
    bool led_state = false;    // Текущее состояние светодиода
    bool previous_state = true; // Предыдущее состояние кнопки (true = не нажата, т.к. есть pull-up)
    
    // 7. Бесконечный цикл
    while (1)
    {
        // Читаем стабильное состояние кнопки
        bool current_state = get_button_debounce(BUTTON_PIN);
        
        // Проверяем фронт нажатия: было не нажато (true), стало нажато (false)
        if (previous_state == true && current_state == false)
        {
            // Инвертируем состояние светодиода
            led_state = !led_state;
            
            // ВЫЗЫВАЕМ нашу новую функцию (вместо прямого gpio_put)
            set_led(led_state);
        }
        
        // Запоминаем текущее состояние как предыдущее для следующей итерации
        previous_state = current_state;
        
        // Небольшая задержка, чтобы не нагружать процессор на 100%
        sleep_ms(10); 
    }
}