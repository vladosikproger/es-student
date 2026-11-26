#pragma once

// Software-visible register map for the next practice.
enum
{
    SLAVE_I2C_ADDRESS = 0x42,
    SLAVE_REG_CMD = 0x00,   // write only: 'e' (enable) / 'd' (disable)
    SLAVE_REG_STATE = 0x01  // read / write: LED state, 0 or 1
};

void slave_device_init(void);
void slave_device_handle(void);
