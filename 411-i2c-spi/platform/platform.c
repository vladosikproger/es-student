#include "platform/platform.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

// GP8 = I2C0 SDA, GP9 = I2C0 SCL.
enum {
    BUS_BAUD = 100000,
    I2C_TIMEOUT_US = 10000,
    SPI_CS_HOLD_US = 2,
};

// CS 0 selects GP16, CS 1 selects GP5 (free until later practice).
static const uint spi_cs_pins[PLATFORM_SPI_CS_COUNT] = {16, 5};

void platform_master_init(void)
{
    stdio_init_all();

    i2c_init(i2c0, BUS_BAUD);
    // The GYBMEP breakout already has 10 kOhm pull-ups to 3V3.
    // Do not enable additional internal weak pull-ups.
    gpio_disable_pulls(8);
    gpio_set_function(8, GPIO_FUNC_I2C);
    gpio_disable_pulls(9);
    gpio_set_function(9, GPIO_FUNC_I2C);

    // SPI0 works alongside I2C0, but the four-pin sensor stays on I2C.
    // Preload high levels before enabling outputs (CS active-low).
    for (uint cs = 0; cs < PLATFORM_SPI_CS_COUNT; cs++)
    {
        const uint pin = spi_cs_pins[cs];
        gpio_init(pin);
        gpio_put(pin, 1);
        gpio_set_dir(pin, GPIO_OUT);
    }

    spi_init(spi0, BUS_BAUD);
    spi_set_format(spi0, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(2, GPIO_FUNC_SPI); // SPI0 SCK
    gpio_set_function(3, GPIO_FUNC_SPI); // SPI0 MOSI
    gpio_pull_down(4);                  // SPI0 MISO reads 0 if unconnected
    gpio_set_function(4, GPIO_FUNC_SPI);
}

bool platform_i2c_write(uint8_t address, uint8_t reg, uint8_t value)
{
    const uint8_t bytes[] = {reg, value};
    return i2c_write_timeout_us(i2c0, address, bytes, sizeof(bytes), false,
                                I2C_TIMEOUT_US) == 2;
}

bool platform_i2c_read(uint8_t address, uint8_t reg, uint8_t *value)
{
    if (value == NULL)
    {
        return false;
    }

    // Keep the bus (nostop=true): the following read starts with repeated START.
    if (i2c_write_timeout_us(i2c0, address, &reg, 1, true,
                             I2C_TIMEOUT_US) != 1)
    {
        return false;
    }

    // Last byte of the read ends with STOP (nostop=false).
    return i2c_read_timeout_us(i2c0, address, value, 1, false,
                               I2C_TIMEOUT_US) == 1;
}

void platform_spi_write(uint8_t cs, uint8_t command, uint8_t value)
{
    const uint8_t bytes[] = {command, value};
    gpio_put(spi_cs_pins[cs], 0);
    spi_write_blocking(spi0, bytes, sizeof(bytes));
    busy_wait_us_32(SPI_CS_HOLD_US);
    gpio_put(spi_cs_pins[cs], 1);
}

uint8_t platform_spi_read(uint8_t cs, uint8_t command, uint8_t pause_us)
{
    uint8_t value = 0;
    gpio_put(spi_cs_pins[cs], 0);
    spi_write_blocking(spi0, &command, 1);
    if (pause_us != 0)
    {
        busy_wait_us_32(pause_us);
    }
    spi_read_blocking(spi0, 0x00, &value, 1);
    busy_wait_us_32(SPI_CS_HOLD_US);
    gpio_put(spi_cs_pins[cs], 1);
    return value;
}
