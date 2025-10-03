#include "HexProgrammer.h"
#include <string.h>

namespace HexProgrammer {
    bool parseLine(const char* line, HexRecord& rec) {
        if (line[0] != ':') return false;
        uint8_t l = strtoul(line+1, 0, 16);
        uint16_t addr = strtoul(line+3, 0, 16);
        uint8_t type = strtoul(line+7, 0, 16);
        if (type != 0x00) return false; // Only data records
        rec.address = addr;
        rec.len = l;
        for (uint8_t i = 0; i < l; ++i)
            rec.data[i] = strtoul(line+9+2*i, 0, 16);
        return true;
    }

    bool programFromHex(const uint8_t* hex, size_t hexLen) {
        char line[64];
        size_t pos = 0;
        while (pos < hexLen) {
            // Extract line
            size_t llen = 0;
            while (pos+llen < hexLen && hex[pos+llen] != '\n' && llen < sizeof(line)-1) llen++;
            memcpy(line, hex+pos, llen); line[llen] = 0;
            pos += llen+1;
            HexRecord rec;
            if (!parseLine(line, rec)) continue;
            // Write in EEPROM page chunks
            uint16_t pageAddr = rec.address & ~(EEPROM_PAGE_SIZE-1);
            uint8_t pageBuf[EEPROM_PAGE_SIZE];
            memset(pageBuf, 0xFF, EEPROM_PAGE_SIZE);
            // Read current page
            EEPROMHandler::read(pageAddr, pageBuf, EEPROM_PAGE_SIZE);
            memcpy(pageBuf + (rec.address & (EEPROM_PAGE_SIZE-1)), rec.data, rec.len);
            // Write back page
            if (!EEPROMHandler::writePage(pageAddr, pageBuf, EEPROM_PAGE_SIZE)) return false;
        }
        return true;
    }

    bool verifyFromHex(const uint8_t* hex, size_t hexLen) {
        char line[64];
        size_t pos = 0;
        while (pos < hexLen) {
            size_t llen = 0;
            while (pos+llen < hexLen && hex[pos+llen] != '\n' && llen < sizeof(line)-1) llen++;
            memcpy(line, hex+pos, llen); line[llen] = 0;
            pos += llen+1;
            HexRecord rec;
            if (!parseLine(line, rec)) continue;
            uint8_t buf[EEPROM_PAGE_SIZE];
            EEPROMHandler::read(rec.address, buf, rec.len);
            if (memcmp(buf, rec.data, rec.len) != 0) return false;
        }
        return true;
    }
}