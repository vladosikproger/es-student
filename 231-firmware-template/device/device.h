#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>

// Макросы вшиваются компилятором прямо в секцию .rodata (только для чтения), не тратя SRAM
#define PROJECT_NAME "231-firmware-template"
#define FIRMWARE_NAME "best" // Твое название
#define FIRMWARE_VERSION "1.0.0" 

// Оставляем только нужные константы для вывода
#define DEVICE_BOARD "pico"

// Единственная функция, которая нам нужна для работы с железом
void device_info(void);

#endif