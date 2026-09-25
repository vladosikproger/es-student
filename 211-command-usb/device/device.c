#include "device.h"
#include "pico/version.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "pico/unique_id.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"

// Переменная с новым, оптимизированным порядком полей
struct info_t device_card = {
    10000,          // version (4 байта)
    "es-led-module",// name (13 байт)
    1               // revision (1 байт)
};

// Твоя старая функция (ее потерял компилятор)
void device_info(void)
{
    // --- серийный номер платы ---
    char board_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    pico_get_unique_board_id_string(board_id, sizeof(board_id));

    // --- регистр CHIP_ID ---
    volatile uint32_t *chip_id = (uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    uint32_t id = *chip_id;

    uint32_t manufacturer = (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    uint32_t part         = (id & SYSINFO_CHIP_ID_PART_BITS)         >> SYSINFO_CHIP_ID_PART_LSB;
    uint32_t revision     = (id & SYSINFO_CHIP_ID_REVISION_BITS)     >> SYSINFO_CHIP_ID_REVISION_LSB;

    // --- пять строк паспорта ---
    printf("project: %s\n", DEVICE_PROJECT);
    printf("repo: %s\n", DEVICE_REPO);
    printf("board: %s\n", DEVICE_BOARD);
    printf("serial: %s\n", board_id);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           manufacturer, part, revision);
    printf("pico-sdk: %s\n", PICO_SDK_VERSION_STRING);
}

// Наша новая функция из текущего задания
void dev_info(void)
{
    printf("%-15s %-11s %4s %6s %s\n", "struct", "address", "size", "offset", "value");
    
    printf("%-15s 0x%08x %5u\n", 
           "device_card", 
           (uint32_t)&device_card, 
           sizeof(struct info_t));

    // Поле version (теперь идет первым)
    printf("- %-13s 0x%08x %5u %6u 0x%08x\n",
           "version",
           (uint32_t)&device_card.version,
           sizeof(device_card.version),
           offsetof(struct info_t, version),
           device_card.version);

    // Поле name (идет вторым)
    printf("- %-13s 0x%08x %5u %6u %s\n",
           "name",
           (uint32_t)device_card.name,
           sizeof(device_card.name),
           offsetof(struct info_t, name),
           device_card.name);

    // Поле revision (идет третьим)
    printf("- %-13s 0x%08x %5u %6u %u\n",
           "revision",
           (uint32_t)&device_card.revision,
           sizeof(device_card.revision),
           offsetof(struct info_t, revision),
           device_card.revision);

    unsigned fields = sizeof(device_card.version) + sizeof(device_card.name) + sizeof(device_card.revision);
    unsigned total = sizeof(struct info_t);
    unsigned padding = total - fields;

    printf("fields %u, sizeof %u, padding %u\n", fields, total, padding);
}