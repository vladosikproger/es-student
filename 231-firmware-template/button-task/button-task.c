#include "button-task.h"
#include "pico/stdlib.h"
#include "led-task.h" // Для вызова led_task_next_state()

#define BUTTON_PIN 15 // ЗАМЕНИ НА СВОЙ ПИН КНОПКИ
const uint64_t DEBOUNCE_TIME_US = 20000; // Время дребезга: 20 мс = 20 000 мкс
typedef enum {
    BUTTON_STATE_RELEASED,   // Стабильно отпущена
    BUTTON_STATE_PRESSING,   // Переходный процесс: возможно нажатие, ждем 20 мс
    BUTTON_STATE_PRESSED,    // Стабильно нажата
    BUTTON_STATE_RELEASING   // Переходный процесс: возможно отпускание, ждем 20 мс
} button_state_t;
// Скрытая память задачи
static button_state_t button_state;
static uint64_t transition_start_time_us;
static uint32_t press_count;

void button_task_init(void)
{
    // Настройка вывода на вход
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    
    // Подтяжка к питанию (Pull-up). Отпущенная кнопка даст '1', нажатая '0'
    gpio_pull_up(BUTTON_PIN); 

    button_state = BUTTON_STATE_RELEASED;
    press_count = 0;
}

void button_task_handle(void)
{
    // 1. Читаем физический уровень (0 - нажата, 1 - отпущена)
    // Инвертируем логику для удобства: is_physical_pressed == true, когда на пине 0 Вольт.
    bool is_physical_pressed = (gpio_get(BUTTON_PIN) == 0);
    
    // 2. Читаем аппаратные часы
    uint64_t now_us = time_us_64();

    // 3. Машина состояний
    switch (button_state)
    {
        case BUTTON_STATE_RELEASED:
            if (is_physical_pressed)
            {
                // Как только заметили падение напряжения, запускаем секундомер
                transition_start_time_us = now_us;
                button_state = BUTTON_STATE_PRESSING;
            }
            break;

        case BUTTON_STATE_PRESSING:
            if (!is_physical_pressed)
            {
                // Кнопка снова отжата до истечения 20 мс. Это был дребезг или помеха.
                button_state = BUTTON_STATE_RELEASED;
            }
            else if (now_us - transition_start_time_us >= DEBOUNCE_TIME_US)
            {
                // Кнопка удерживается 20 мс непрерывно. Фиксируем нажатие!
                button_state = BUTTON_STATE_PRESSED;
                press_count++;
                
                // Вызываем функцию светодиода по ТЗ
                led_task_next_state(); 
            }
            break;

        case BUTTON_STATE_PRESSED:
            if (!is_physical_pressed)
            {
                // Контакт пропал. Запускаем секундомер на отпускание
                transition_start_time_us = now_us;
                button_state = BUTTON_STATE_RELEASING;
            }
            break;

        case BUTTON_STATE_RELEASING:
            if (is_physical_pressed)
            {
                // Контакт вернулся до истечения 20 мс. Это дребезг при отпускании.
                button_state = BUTTON_STATE_PRESSED;
            }
            else if (now_us - transition_start_time_us >= DEBOUNCE_TIME_US)
            {
                // Контакта нет 20 мс непрерывно. Кнопка точно отпущена.
                button_state = BUTTON_STATE_RELEASED;
            }
            break;
    }
}

// Реализация геттеров
bool button_task_is_pressed(void)
{
    // По ТЗ: "Кнопка нажата и в состоянии «нажата», и в «отпускание?»"
    return (button_state == BUTTON_STATE_PRESSED || button_state == BUTTON_STATE_RELEASING);
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}