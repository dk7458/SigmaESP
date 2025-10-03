#ifndef ADA_COMMUNICATOR_H
#define ADA_COMMUNICATOR_H

#include <WiFi.h>
#include "DSPWriter.h"

#define CMD_WRITE 0x09
#define CMD_READ  0x0a

// Headers
struct adauWriteHeader {
    uint8_t command;
    uint8_t safeload;
    uint8_t placement;
    uint16_t totalLen;
    uint8_t chipAddr;
    uint16_t dataLen;
    uint16_t address;
};

struct adauReadHeader {
    uint8_t command;
    uint16_t totalLen;
    uint8_t chipAddr;
    uint16_t dataLen;
    uint16_t address;
};

class AdaCommunicator {
public:
    AdaCommunicator();
    void begin();
    void handleClient();
private:
    WiFiServer tcpServer;
    void handleWrite(WiFiClient &client, bool &flashingDetected, int &lastRegSize);
    void handleRead(WiFiClient &client);
    size_t readWithTimeout(WiFiClient &client, uint8_t *buf, size_t n, uint32_t timeoutMs = 2000);
};

#endif