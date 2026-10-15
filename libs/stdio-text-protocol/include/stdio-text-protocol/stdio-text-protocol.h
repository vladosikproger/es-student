#pragma once

#include <stdbool.h>
#include <stdint.h>

#define STDIO_TEXT_PROTOCOL_LINE_SIZE 64
#define STDIO_TEXT_PROTOCOL_MAX_ARGS 4

// Команда, принятая из одной строки (Абстрактное синтаксическое дерево)
typedef struct {
    const char *name;
    uint32_t argc;
    const char *argv[STDIO_TEXT_PROTOCOL_MAX_ARGS];

    // Флаг переполнения (буфер забит или аргументов > 4)
    bool truncated;
} command_t;

void stdio_text_protocol_init(void);

// Функция-обработчик (Конечный автомат)
const command_t *stdio_text_protocol_handle(void);