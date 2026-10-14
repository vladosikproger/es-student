#pragma once
#include <stdint.h>

// Константы прошивки
#define DEVICE_NAME "my-pico-device"
#define PROJECT_NAME DEVICE_NAME
#define FIRMWARE_VERSION "0.1.0"

// Длина строки серийного номера (16 символов + 1 нулевой байт)
#define DEVICE_SERIAL_STR_LEN 17

// Структура паспорта прибора
typedef struct {
    const char *board_type;
    char serial[DEVICE_SERIAL_STR_LEN];
    uint16_t manufacturer;
    uint16_t part;
    uint8_t revision;
    const char *sdk_version;
} device_info_t;

// Прототип функции
void device_get_info(device_info_t *info);