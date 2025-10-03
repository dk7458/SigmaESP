#include "EEPROMHandler.h"
#include <Arduino.h>

namespace EEPROMHandler {
    bool writePage(uint16_t addr, const uint8_t* data, size_t len) {
        if (len > EEPROM_PAGE_SIZE) return false;
        digitalWrite(WP_PIN, LOW); // Disable write protect
        Wire.beginTransmission(EEPROM_I2C_ADDRESS);
        Wire.write((addr >> 8) & 0xFF);
        Wire.write(addr & 0xFF);
        for (size_t i = 0; i < len; ++i) Wire.write(data[i]);
        if (Wire.endTransmission() != 0) {
            digitalWrite(WP_PIN, HIGH); // Re-enable
            return false;
        }
        delay(6); // EEPROM write cycle time (5ms typical)
        digitalWrite(WP_PIN, HIGH); // Re-enable write protect
        return true;
    }

    bool read(uint16_t addr, uint8_t* data, size_t len) {
        Wire.beginTransmission(EEPROM_I2C_ADDRESS);
        Wire.write((addr >> 8) & 0xFF);
        Wire.write(addr & 0xFF);
        if (Wire.endTransmission(false) != 0) return false;
        Wire.requestFrom(EEPROM_I2C_ADDRESS, (int)len);
        for (size_t i = 0; i < len; ++i) {
            if (Wire.available()) data[i] = Wire.read();
            else return false;
        }
        return true;
    }
}