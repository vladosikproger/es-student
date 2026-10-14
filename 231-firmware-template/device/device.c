#include "device.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"

// Аппаратные словари для доступа к регистрам RP2040
#include "hardware/regs/sysinfo.h"
#include "hardware/structs/sysinfo.h"

void device_get_info(device_info_t *info)
{
    // Указатель на тип платы
    info->board_type = PICO_BOARD;

    // Чтение уникального серийного номера (64 бита) во внутренний массив структуры
    pico_get_unique_board_id_string(info->serial, sizeof(info->serial));

    // Прямое чтение железного регистра паспорта
    uint32_t chip_id = sysinfo_hw->chip_id;

    // Нарезка 32-битного числа на куски с помощью побитовых масок и сдвигов
    info->manufacturer = (chip_id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    info->part         = (chip_id & SYSINFO_CHIP_ID_PART_BITS)         >> SYSINFO_CHIP_ID_PART_LSB;
    info->revision     = (chip_id & SYSINFO_CHIP_ID_REVISION_BITS)     >> SYSINFO_CHIP_ID_REVISION_LSB;

    // Указатель на версию SDK
    info->sdk_version = PICO_SDK_VERSION_STRING;
}