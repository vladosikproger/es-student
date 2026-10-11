#include "stdio-text-protocol.h"
#include "pico/stdlib.h"
#include <stdio.h>

// 1. Состояние модуля (Инкапсуляция)
static char line_buffer[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint32_t line_length = 0;
static bool is_truncated = false;
static command_t current_command;

void stdio_text_protocol_init(void)
{
    line_length = 0;
    is_truncated = false;
}

const command_t *stdio_text_protocol_handle(void)
{
    // 2. Чтение символа без ожидания (Неблокирующий ввод-вывод)
    int c = getchar_timeout_us(0);

    if (c == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    // 3. Обработка конца строки (Enter)
    if (c == '\r' || c == '\n')
    {
        // пустую строку пропускаем
        if (line_length == 0) return NULL;

        // переводим каретку в терминале
        printf("\r\n");
        
        // Замыкаем строку нуль-терминатором для стандартных функций C
        line_buffer[line_length] = '\0';

        // Сбрасываем старую команду перед сборкой новой
        current_command.name = NULL;
        current_command.argc = 0;
        current_command.truncated = is_truncated;

        // Деление строки на слова на месте (In-place tokenization)
        char *ptr = line_buffer;
        bool in_word = false;

        while (*ptr != '\0')
        {
            if (*ptr == ' ')
            {
                *ptr = '\0'; // Заменяем пробел концом подстроки
                in_word = false;
            }
            else
            {
                if (!in_word)
                {
                    if (current_command.name == NULL)
                    {
                        current_command.name = ptr;
                    }
                    else if (current_command.argc < STDIO_TEXT_PROTOCOL_MAX_ARGS)
                    {
                        current_command.argv[current_command.argc] = ptr;
                        current_command.argc++;
                    }
                    else
                    {
                        current_command.truncated = true; // Аргументов больше 4
                    }
                    in_word = true;
                }
            }
            ptr++;
        }

        // Подготовка к чтению следующей команды
        line_length = 0;
        is_truncated = false;

        return &current_command;
    }

    // 4. Обработка Backspace (Удаление символа)
    if (c == '\b' || c == 0x7F)
    {
        if (line_length > 0)
        {
            line_length--;
            is_truncated = false;
            printf("\b \b"); // Стираем символ с экрана терминала
        }
        return NULL;
    }

    // 5. Обработка обычного печатного символа
    if (c >= 32 && c <= 126)
    {
        if (line_length < STDIO_TEXT_PROTOCOL_LINE_SIZE - 1)
        {
            line_buffer[line_length] = (char)c;
            line_length++;
            putchar(c); // Эхо: возвращаем символ в терминал
        }
        else
        {
            is_truncated = true; // Буфер переполнен
        }
    }

    return NULL;
}