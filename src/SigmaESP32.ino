#include <Wire.h>#include <Wire.h>#include <Wire.h>#include <Wire.h>#include <Wire.h>

#include <WiFi.h>

#include "DSPWriter.h"#include <WiFi.h>

#include "EEPROMHandler.h"

#include "HexProgrammer.h"#include "DSPWriter.h"#include <WiFi.h>

#include "WebProgrammer.h"

#include "AdaCommunicator.h"#include "EEPROMHandler.h"



// GPIO for ADAU1701 reset (choose a free pin, e.g., GPIO12)#include "HexProgrammer.h"#include "DSPWriter.h"#include <WiFi.h>#include "DSPWriter.h"

#define ADAU_RESET_PIN 12

#define LED_BUILTIN 15  // For LOLIN S2 Mini#include "WebProgrammer.h"
// Clean ESP32 main that uses the refactored modules
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "DSPWriter.h"
#include "EEPROMHandler.h"
#include "HexProgrammer.h"
#include "WebProgrammer.h"
#include "AdaCommunicator.h"

// Pin definitions for LOLIN S2 Mini
#define ADAU_RESET_PIN 12
#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif
#define WP_PIN 5

// WiFi AP credentials
const char* ssid     = "ADAU_AP";
const char* password = "12345678";  // must be >=8 chars

// Module instances
WebProgrammer webProgrammer;
AdaCommunicator adaCommunicator;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== ESP32 ADAU interface starting ===");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(ADAU_RESET_PIN, OUTPUT);
  digitalWrite(ADAU_RESET_PIN, HIGH); // release from reset

  pinMode(WP_PIN, OUTPUT);
  digitalWrite(WP_PIN, HIGH); // enable write-protect by default

  // Start WiFi AP
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("AP start failed!");
  } else {
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  // SDA = GPIO7, SCL = GPIO9 on ESP32-S2 (LOLIN S2 Mini)
  Wire.begin(7, 9);

  adaCommunicator.begin();
  webProgrammer.setup();

  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
}

void loop() {
  webProgrammer.handleClient();

  // If web programmer is active (upload in progress) only service web requests
  extern volatile bool programmingMode; // declared in WebProgrammer.h
  if (programmingMode) {
    yield();
    return;
  }

  adaCommunicator.handleClient();
}
  // SDA = GPIO7, SCL = GPIO9 for ESP32-S2
