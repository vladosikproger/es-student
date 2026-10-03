#include "clock.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"

// Вспомогательная функция (штамповочный пресс для строк таблицы)
// Изолирована модификатором static
static void row(const char *name, uint32_t set_khz, uint32_t measured_khz)
{
    printf("%-8s %9u %12u\n", name, (unsigned)set_khz, (unsigned)measured_khz);
}
void uptime(void)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}
void clk_info(void)
{
    // 1. Шапка таблицы
    // %-8s - строка, 8 символов, прижата влево (названия сигналов)
    // %9s  - строка, 9 символов, прижата вправо (настроенная частота)
    // %12s - строка, 12 символов, прижата вправо (измеренная частота)
    printf("%-8s %9s %12s\n", "clock", "configured", "measured");

    // 2. Прогоняем 5 основных сигналов через наш конвейер row()
    // Деление на 1000 переводит герцы в килогерцы прямо "на лету"
    row("clk_ref",  clock_get_hz(clk_ref)  / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_REF));
    row("clk_sys",  clock_get_hz(clk_sys)  / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS));
    row("clk_peri", clock_get_hz(clk_peri) / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI));
    row("clk_usb",  clock_get_hz(clk_usb)  / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB));
    row("clk_adc",  clock_get_hz(clk_adc)  / 1000, frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC));

    // 3. Аномалия (ROSC)
    // Обрати внимание на %9s вместо %9u, чтобы передать прочерк как строку
    uint32_t rosc_meas = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    printf("%-8s %9s %12u\n", "rosc", "-", (unsigned)rosc_meas);
}