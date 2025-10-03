#ifndef EEPROM_HANDLER_H
#define EEPROM_HANDLER_H

#include <Wire.h>

#define EEPROM_I2C_ADDRESS 0x50 // 7-bit address for 24LC256/512 (0xA0 >> 1)
#define EEPROM_PAGE_SIZE 64
#define WP_PIN 5

namespace EEPROMHandler {
    bool writePage(uint16_t addr, const uint8_t* data, size_t len);
    bool read(uint16_t addr, uint8_t* data, size_t len);
}

#endif