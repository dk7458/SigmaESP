#include "WebProgrammer.h"
#include "HexProgrammer.h"
#include <Arduino.h>

volatile bool programmingMode = false;
uint8_t hexUploadBuffer[HEX_UPLOAD_MAX_SIZE];
size_t hexUploadLen = 0;

WebProgrammer::WebProgrammer() : webServer(80) {}

void WebProgrammer::setup() {
    webServer.on("/", [this]() { handleRoot(); });
    webServer.on("/enter-prog", [this]() { handleEnterProg(); });
    webServer.on("/upload", HTTP_GET, [this]() { handleUploadPage(); });
    webServer.on("/upload", HTTP_POST, [this]() { webServer.send(200); }, [this]() { handleFileUpload(); });
    webServer.begin();
    Serial.println("Web server started on port 80");
}

void WebProgrammer::handleClient() {
    webServer.handleClient();
}

void WebProgrammer::handleEnterProg() {
    programmingMode = true;
    webServer.send(200, "text/plain", "Programming mode activated. Go to /upload to program EEPROM.");
}

void WebProgrammer::handleUploadPage() {
    String html = "<h2>ADAU1701 EEPROM Programmer</h2>";
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='hexfile' accept='.hex'/><br><br>";
    html += "<input type='submit' value='Upload & Program'/></form>";
    webServer.send(200, "text/html", html);
}

void WebProgrammer::handleFileUpload() {
    HTTPUpload& upload = webServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        hexUploadLen = 0;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (hexUploadLen + upload.currentSize < HEX_UPLOAD_MAX_SIZE) {
            memcpy(hexUploadBuffer + hexUploadLen, upload.buf, upload.currentSize);
            hexUploadLen += upload.currentSize;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        // File received, trigger programming
        webServer.send(200, "text/html", "<h3>Upload complete. Programming EEPROM...</h3>");
        // Put ADAU1701 in reset
        digitalWrite(ADAU_RESET_PIN, LOW);
        delay(10);
        bool ok = HexProgrammer::programFromHex(hexUploadBuffer, hexUploadLen);
        bool verify = false;
        if (ok) verify = HexProgrammer::verifyFromHex(hexUploadBuffer, hexUploadLen);
        // Release ADAU1701 from reset
        digitalWrite(ADAU_RESET_PIN, HIGH);
        if (ok && verify) {
            webServer.send(200, "text/html", "<h3>Programming and verification successful! ADAU1701 rebooted.<br><a href='/'>Return to main page</a></h3>");
        } else {
            webServer.send(200, "text/html", "<h3>Programming or verification failed.<br><a href='/'>Return to main page</a></h3>");
        }
        programmingMode = false;
    }
}

void WebProgrammer::handleRoot() {
    webServer.send(200, "text/html", "<h2>ADAU1701 Wireless Programmer</h2><a href='/enter-prog'>Enter Programming Mode</a>");
}