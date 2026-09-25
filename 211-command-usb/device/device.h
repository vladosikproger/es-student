#pragma once

#include <stdio.h>
#include <stdint.h> // Обязательно добавляем для uint8_t и uint32_t

#define DEVICE_NAME "es-led-module"
#define FIRMWARE_VERSION "1.0.0"

#define DEVICE_PROJECT "211-command-usb"
#define DEVICE_REPO "https://github.com/vladosikproger/es-student"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

// --- НОВОЕ: объявляем структуру для паспорта ---
// НОВЫЙ ПОРЯДОК ПОЛЕЙ (от большего к меньшему)
struct info_t {
    uint32_t version;  // 4 байта
    char     name[13]; // 13 байт
    uint8_t  revision; // 1 байт
};

// --- НОВОЕ: говорим, что переменная device_card существует где-то в .c файле ---
extern struct info_t device_card;

void device_info(void); // Твоя старая функция (оставляем)
void dev_info(void);    // НОВОЕ: прототип новой функции для задания