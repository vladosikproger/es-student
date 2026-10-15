#include "stdio-text-protocol/stdio-text-protocol.h"
#include "pico/stdlib.h"
#include <stdio.h>

// 1. РЎРѕСЃС‚РѕСЏРЅРёРµ РјРѕРґСѓР»СЏ (РРЅРєР°РїСЃСѓР»СЏС†РёСЏ)
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
    // 2. Р§С‚РµРЅРёРµ СЃРёРјРІРѕР»Р° Р±РµР· РѕР¶РёРґР°РЅРёСЏ (РќРµР±Р»РѕРєРёСЂСѓСЋС‰РёР№ РІРІРѕРґ-РІС‹РІРѕРґ)
    int c = getchar_timeout_us(0);

    if (c == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    // 3. РћР±СЂР°Р±РѕС‚РєР° РєРѕРЅС†Р° СЃС‚СЂРѕРєРё (Enter)
    if (c == '\r' || c == '\n')
    {
        // РїСѓСЃС‚СѓСЋ СЃС‚СЂРѕРєСѓ РїСЂРѕРїСѓСЃРєР°РµРј
        if (line_length == 0) return NULL;

        // РїРµСЂРµРІРѕРґРёРј РєР°СЂРµС‚РєСѓ РІ С‚РµСЂРјРёРЅР°Р»Рµ
        printf("\r\n");
        
        // Р—Р°РјС‹РєР°РµРј СЃС‚СЂРѕРєСѓ РЅСѓР»СЊ-С‚РµСЂРјРёРЅР°С‚РѕСЂРѕРј РґР»СЏ СЃС‚Р°РЅРґР°СЂС‚РЅС‹С… С„СѓРЅРєС†РёР№ C
        line_buffer[line_length] = '\0';

        // РЎР±СЂР°СЃС‹РІР°РµРј СЃС‚Р°СЂСѓСЋ РєРѕРјР°РЅРґСѓ РїРµСЂРµРґ СЃР±РѕСЂРєРѕР№ РЅРѕРІРѕР№
        current_command.name = NULL;
        current_command.argc = 0;
        current_command.truncated = is_truncated;

        // Р”РµР»РµРЅРёРµ СЃС‚СЂРѕРєРё РЅР° СЃР»РѕРІР° РЅР° РјРµСЃС‚Рµ (In-place tokenization)
        char *ptr = line_buffer;
        bool in_word = false;

        while (*ptr != '\0')
        {
            if (*ptr == ' ')
            {
                *ptr = '\0'; // Р—Р°РјРµРЅСЏРµРј РїСЂРѕР±РµР» РєРѕРЅС†РѕРј РїРѕРґСЃС‚СЂРѕРєРё
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
                        current_command.truncated = true; // РђСЂРіСѓРјРµРЅС‚РѕРІ Р±РѕР»СЊС€Рµ 4
                    }
                    in_word = true;
                }
            }
            ptr++;
        }

        // РџРѕРґРіРѕС‚РѕРІРєР° Рє С‡С‚РµРЅРёСЋ СЃР»РµРґСѓСЋС‰РµР№ РєРѕРјР°РЅРґС‹
        line_length = 0;
        is_truncated = false;

        return &current_command;
    }

    // 4. РћР±СЂР°Р±РѕС‚РєР° Backspace (РЈРґР°Р»РµРЅРёРµ СЃРёРјРІРѕР»Р°)
    if (c == '\b' || c == 0x7F)
    {
        if (line_length > 0)
        {
            line_length--;
            is_truncated = false;
            printf("\b \b"); // РЎС‚РёСЂР°РµРј СЃРёРјРІРѕР» СЃ СЌРєСЂР°РЅР° С‚РµСЂРјРёРЅР°Р»Р°
        }
        return NULL;
    }

    // 5. РћР±СЂР°Р±РѕС‚РєР° РѕР±С‹С‡РЅРѕРіРѕ РїРµС‡Р°С‚РЅРѕРіРѕ СЃРёРјРІРѕР»Р°
    if (c >= 32 && c <= 126)
    {
        if (line_length < STDIO_TEXT_PROTOCOL_LINE_SIZE - 1)
        {
            line_buffer[line_length] = (char)c;
            line_length++;
            putchar(c); // Р­С…Рѕ: РІРѕР·РІСЂР°С‰Р°РµРј СЃРёРјРІРѕР» РІ С‚РµСЂРјРёРЅР°Р»
        }
        else
        {
            is_truncated = true; // Р‘СѓС„РµСЂ РїРµСЂРµРїРѕР»РЅРµРЅ
        }
    }

    return NULL;
}
