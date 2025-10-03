#ifndef HEX_PROGRAMMER_H
#define HEX_PROGRAMMER_H

#include <stdint.h>
#include "EEPROMHandler.h"

#define EEPROM_PAGE_SIZE 64

struct HexRecord {
    uint16_t address;
    uint8_t data[EEPROM_PAGE_SIZE];
    uint8_t len;
};

namespace HexProgrammer {
    bool parseLine(const char* line, HexRecord& rec);
    bool programFromHex(const uint8_t* hex, size_t hexLen);
    bool verifyFromHex(const uint8_t* hex, size_t hexLen);
}

#endif