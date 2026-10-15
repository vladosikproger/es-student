#include "systime/systime.h"
#include "pico/stdlib.h"

uint64_t systime_us(void) {
    return time_us_64();
}

uint64_t systime_ms(void) {
    return time_us_64() / 1000;
}

bool systime_period_elapsed(uint64_t *last_us, uint64_t period_us) {
    uint64_t now = systime_us();
    
    // Если это самый первый запуск (метка времени равна нулю)
    if (*last_us == 0) {
        *last_us = now;
        return false;
    }

    // Проверяем, прошло ли нужное время с прошлой контрольной точки
    if (now - *last_us >= period_us) {
        // СДВИГ МЕТКИ НА ПЕРИОД (исправляет накопление ошибки!)
        // Мы прибавляем период к старому значению, а не заменяем его на 'now'
        *last_us += period_us;
        return true;
    }
    
    return false;
}