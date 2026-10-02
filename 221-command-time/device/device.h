#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>

#define DEVICE_PROJECT "221-command-time"
#define DEVICE_REPO    "es-student"
#define DEVICE_BOARD   "pico"
#define DEVICE_NAME    "es-led-module"
#define FIRMWARE_VERSION "1.1.0"

struct info_t {
    uint32_t version;
    char name[13];
    uint8_t revision;
};

extern struct info_t device_card;

void device_info(void);
void dev_info(void);

#endif