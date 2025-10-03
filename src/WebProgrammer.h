#ifndef WEB_PROGRAMMER_H
#define WEB_PROGRAMMER_H

#include <WebServer.h>

#define HEX_UPLOAD_MAX_SIZE (32*1024)
#define ADAU_RESET_PIN 12

extern volatile bool programmingMode;
extern uint8_t hexUploadBuffer[HEX_UPLOAD_MAX_SIZE];
extern size_t hexUploadLen;

class WebProgrammer {
public:
    WebProgrammer();
    void setup();
    void handleClient();
private:
    WebServer webServer;
    void handleEnterProg();
    void handleUploadPage();
    void handleFileUpload();
    void handleRoot();
};

#endif