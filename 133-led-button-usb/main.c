#include <stdio.h>
#include "pico/stdlib.h"

// Константы: номера пинов
const uint LED_PIN = 25;           // Встроенный светодиод
const uint BUTTON_PIN = 15;        // Кнопка на GPIO 15
const uint DEBOUNCE_MS = 20;       // Время защиты от дребезга

// Функция защиты от дребезга кнопки
bool get_button_debounce(uint pin)
{
    // Читаем состояние кнопки первый раз
    bool state1 = gpio_get(pin);
    
    // Ждём 20мс (время дребезга контактов)
    sleep_ms(DEBOUNCE_MS);
    
    // Читаем состояние второй раз
    bool state2 = gpio_get(pin);
    
    // Если оба раза одинаково — возвращаем это значение
    // Если разное — возвращаем последнее (дребезг ещё идёт)
    return (state1 == state2) ? state1 : state2;
}

// Функция включения/выключения светодиода
void set_led(bool on)
{
    // Установить напряжение на пине LED_PIN
    // on=true → 3.3В (светодиод горит)
    // on=false → 0В (светодиод не горит)
    gpio_put(LED_PIN, on);
    
    // Отправить строку в USB-порт
    // Если on=true → "led on"
    // Если on=false → "led off"
    printf("led %s\n", on ? "on" : "off");
    
    // Принудительно сбросить буфер вывода
    // Без этого printf может не отправить данные сразу
    fflush(stdout);
}

int main()
{
    // Инициализировать USB для вывода через printf
    stdio_init_all();
    
    // Отключить буферизацию stdout
    // Каждый символ отправляется сразу в USB
    setbuf(stdout, NULL);
    
    // Инициализировать пин светодиода
    gpio_init(LED_PIN);
    // Установить направление: OUT (выход)
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // Инициализировать пин кнопки
    gpio_init(BUTTON_PIN);
    // Установить направление: IN (вход)
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    // Включить pull-up резистор
    // Когда кнопка не нажата — пин подтянут к 3.3В (логическая 1)
    // Когда кнопка нажата — пин замкнут на землю (логический 0)
    gpio_pull_up(BUTTON_PIN);
    
    // Переменные для хранения состояния
    bool led_state = false;           // Светодиод выключен
    bool previous_state = true;       // Кнопка не нажата (pull-up = true)
    
    // БЕСКОНЕЧНЫЙ ЦИКЛ
    while (1)
    {
        // Читаем состояние кнопки с защитой от дребезга
        bool current_state = get_button_debounce(BUTTON_PIN);
        
        // Проверяем: было true (не нажата), стало false (нажата)
        // Это называется "падающий фронт" (falling edge)
        if (previous_state == true && current_state == false)
        {
            // Переключаем светодиод в противоположное состояние
            // Если был false → станет true
            // Если был true → станет false
            led_state = !led_state;
            
            // Вызываем функцию установки светодиода
            set_led(led_state);
        }
        
        // Запоминаем текущее состояние как "предыдущее"
        // для следующей итерации цикла
        previous_state = current_state;
        
        // Ждём 10мс перед следующей проверкой
        // Этого достаточно для реакции на нажатие
        sleep_ms(10);
    }
}
