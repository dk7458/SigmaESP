#include "AdaCommunicator.h"
#include <Arduino.h>
#include <Wire.h>

AdaCommunicator::AdaCommunicator() : tcpServer(8086) {}

void AdaCommunicator::begin() {
    tcpServer.begin();
    Serial.println("TCP server listening on port 8086");
}

void AdaCommunicator::handleClient() {
    WiFiClient client = tcpServer.available();
    if (!client) return;

    Serial.println("New connection");
    digitalWrite(LED_BUILTIN, HIGH);

    // track some state used by handlers
    bool flashingDetected = false;
    int lastRegSize = 0;
    unsigned long lastHeapLog = millis();

    while (client.connected()) {
        // Log heap occasionally
        if (millis() - lastHeapLog > 5000) {
            Serial.printf("Heap: %u bytes\n", ESP.getFreeHeap());
            lastHeapLog = millis();
        }

        if (client.available() > 0) {
            int next = client.peek();
            if (next < 0) { yield(); continue; }

            if (next == CMD_WRITE) {
                handleWrite(client, flashingDetected, lastRegSize);
            } else if (next == CMD_READ) {
                handleRead(client);
            } else {
                // Unknown or stray byte — consume & skip
                uint8_t b = client.read();
                Serial.printf("Skipping stray byte 0x%02X\n", b);
            }
        } else {
            // no data — yield and let WiFi run
            yield();
        }
    }

    client.stop();
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Client disconnected");
}

size_t AdaCommunicator::readWithTimeout(WiFiClient &client, uint8_t *buf, size_t n, uint32_t timeoutMs) {
    size_t got = 0;
    uint32_t start = millis();
    while (got < n && client.connected()) {
        if (client.available()) {
            int r = client.readBytes((char*)(buf + got), n - got);
            if (r > 0) got += r;
        } else {
            yield();
            if (millis() - start > timeoutMs) break;
        }
    }
    return got;
}

void AdaCommunicator::handleWrite(WiFiClient &client, bool &flashingDetected, int &lastRegSize) {
    // consume command byte (we peeked before)
    client.read(); // drop CMD_WRITE

    // read remaining 9 header bytes
    uint8_t hdrBuf[9];
    size_t h = readWithTimeout(client, hdrBuf, 9, 2000);
    if (h < 9) {
        Serial.printf("Write header read failed (%u)\n", (unsigned)h);
        return;
    }

    adauWriteHeader hdr;
    hdr.safeload = hdrBuf[0];
    hdr.placement = hdrBuf[1];
    hdr.totalLen = (hdrBuf[2] << 8) | hdrBuf[3];
    hdr.chipAddr = hdrBuf[4];
    hdr.dataLen = (hdrBuf[5] << 8) | hdrBuf[6];
    hdr.address = (hdrBuf[7] << 8) | hdrBuf[8];

    Serial.printf("WRITE hdr: safeload=%u chip=0x%02X addr=0x%04X len=%u\n",
                  hdr.safeload, hdr.chipAddr, hdr.address, hdr.dataLen);

    digitalWrite(LED_BUILTIN, LOW);

    // determine register size
    uint8_t registerSize = hdr.dataLen;
    uint16_t regAddress = hdr.address;
    if ((hdr.address == dspRegister::CoreRegister) && (hdr.dataLen == 2))
        registerSize = CORE_REGISTER_R0_REGSIZE;
    else if ((hdr.address == dspRegister::CoreRegister) && (hdr.dataLen == 24))
        registerSize = HARDWARE_CONF_REGSIZE;
    else if (hdr.address == 0x0400) {
        registerSize = PROGRAM_REGSIZE;
        flashingDetected = true;
    }
    else if (hdr.address == 0)
        registerSize = PARAMETER_REGSIZE;
    else if (flashingDetected)
        registerSize = lastRegSize;

    lastRegSize = registerSize;

    // Normalize safeload: treat non-1 as normal write
    bool isSafeload = (hdr.safeload == 1);
    if (hdr.safeload != 0 && hdr.safeload != 1) {
        Serial.printf("Warning: unexpected safeload=%u -> treating as normal write\n", hdr.safeload);
        isSafeload = false;
    }

    const size_t CHUNK_SIZE = 1024;
    uint8_t chunk[CHUNK_SIZE];

    if (isSafeload) {
        int words = hdr.dataLen / 4;
        DSPWriter dspWriter;
        for (int w = 0; w < words && client.connected(); ++w) {
            size_t got = readWithTimeout(client, chunk, 4, 2000);
            if (got < 4) {
                Serial.println("safeload: partial data or timeout");
                break;
            }
            uint8_t dataArray[5];
            dataArray[0] = 0;
            dataArray[1] = chunk[0];
            dataArray[2] = chunk[1];
            dataArray[3] = chunk[2];
            dataArray[4] = chunk[3];
            dspWriter.safeload_writeRegister(regAddress + w, dataArray, (w == words - 1));
            yield();
        }
    } else {
        // Stream normal block in chunks
        uint32_t remaining = hdr.dataLen;
        uint32_t offsetAddress = regAddress;
        while (remaining && client.connected()) {
            size_t toRead = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
            size_t got = readWithTimeout(client, chunk, toRead, 5000);
            if (got == 0) {
                Serial.println("payload read failed or timeout");
                break;
            }

            // Pass chunk to DSPWriter
            DSPWriter::writeRegisterBlock(offsetAddress, got, chunk, registerSize);

            // Advance address: assume address increments by number of registers written
            // where each register occupies registerSize bytes.
            uint16_t regsWritten = (registerSize > 0) ? (got / registerSize) : 0;
            offsetAddress += regsWritten;
            remaining -= got;
            yield();
        }
    }

    // acknowledge
    client.write("OK", 2);
    digitalWrite(LED_BUILTIN, HIGH);
}

void AdaCommunicator::handleRead(WiFiClient &client) {
    // consume command byte
    client.read();

    // read remaining 7 header bytes
    uint8_t hdrBuf[7];
    size_t h = readWithTimeout(client, hdrBuf, 7, 2000);
    if (h < 7) {
        Serial.printf("Read header failed (%u)\n", (unsigned)h);
        return;
    }

    adauReadHeader rh;
    rh.totalLen = (hdrBuf[0] << 8) | hdrBuf[1];
    rh.chipAddr = hdrBuf[2];
    rh.dataLen = (hdrBuf[3] << 8) | hdrBuf[4];
    rh.address = (hdrBuf[5] << 8) | hdrBuf[6];

    Serial.printf("READ hdr: chip=0x%02X addr=0x%04X len=%u\n", rh.chipAddr, rh.address, rh.dataLen);

    // Read from ADAU and send back
    uint8_t *buf = (uint8_t*) malloc(rh.dataLen ? rh.dataLen : 1);
    if (!buf) {
        Serial.println("ERR: malloc failed for read response");
        client.write("ERR", 3);
        return;
    }

    Wire.beginTransmission(rh.chipAddr);
    Wire.write((rh.address >> 8) & 0xFF);
    Wire.write(rh.address & 0xFF);
    Wire.endTransmission(false);
    Wire.requestFrom((int)rh.chipAddr, (int)rh.dataLen);
    for (int i = 0; i < rh.dataLen && Wire.available(); ++i) buf[i] = Wire.read();

    client.write(buf, rh.dataLen);
    free(buf);
}