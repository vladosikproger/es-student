#pragma once

#include <stdbool.h>
#include <stdint.h>

void platform_master_init(void);
bool platform_i2c_write(uint8_t address, uint8_t reg, uint8_t value);
bool platform_i2c_read(uint8_t address, uint8_t reg, uint8_t *value);

// Two independently selectable SPI devices (CS0=GP16, CS1=GP5).
enum { PLATFORM_SPI_CS_COUNT = 2 };
void platform_spi_write(uint8_t cs, uint8_t command, uint8_t value);
uint8_t platform_spi_read(uint8_t cs, uint8_t command, uint8_t pause_us);
