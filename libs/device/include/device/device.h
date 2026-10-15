#pragma once
#include <stdint.h>

#define DEVICE_SERIAL_STR_LEN 17

typedef struct {
    const char *board_type;
    char serial[DEVICE_SERIAL_STR_LEN];
    uint16_t manufacturer;
    uint16_t part;
    uint8_t revision;
    const char *sdk_version;
} device_info_t;

void device_get_info(device_info_t *info);