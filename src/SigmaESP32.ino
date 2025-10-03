// Main for LOLIN S2 Mini (ESP32-S2)
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "Config.h"
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

// Module instances
WebProgrammer webProgrammer;
AdaCommunicator adaCommunicator;

// STA reconnect policy when AP fallback is active
static const unsigned long STA_RETRY_INTERVAL_MS = 60000UL; // retry STA every 60s
static unsigned long lastStaRetry = 0;

static bool startAP(const char* apSsid, const char* apPass) {
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(apSsid, apPass);
  if (ok) {
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  } else Serial.println("AP start failed!");
  return ok;
}

static bool trySTAConnect(const char* staSsid, const char* staPass, unsigned long timeoutMs) {
  // sanitize inputs: allow users to paste <ssid> or <pass> and strip <> if present
  String ss = (staSsid) ? String(staSsid) : String("");
  String sp = (staPass) ? String(staPass) : String("");
  if (ss.length() > 1 && ss.startsWith("<") && ss.endsWith(">")) ss = ss.substring(1, ss.length()-1);
  if (sp.length() > 1 && sp.startsWith("<") && sp.endsWith(">")) sp = sp.substring(1, sp.length()-1);

  if (ss.length() == 0) {
    Serial.println("No STA SSID configured");
    return false;
  }

  // clean start
  WiFi.disconnect(true); // clear previous config
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ss.c_str(), sp.c_str());
  Serial.printf("Trying STA connect to '%s'...\n", ss.c_str());

  unsigned long start = millis();
  unsigned long lastLog = 0;
  while (millis() - start < timeoutMs) {
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) {
      Serial.print("STA IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    // log status every 1s
    if (millis() - lastLog > 1000) {
      lastLog = millis();
      Serial.printf("WiFi status: %d\n", (int)st);
    }
    delay(200);
    yield();
  }
  Serial.println("STA connect timed out");
  WiFi.disconnect(true);
  return false;
}

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

  // Try STA first, fall back to AP
  bool staOk = false;
  if (Config::STA_SSID && Config::STA_SSID[0]) {
    // Ensure radio is in STA mode before scanning
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true); // clear previous connection but keep radio up for scan
    delay(100);

    // scan to see if the target SSID is visible (helps debugging)
    Serial.println("Scanning for networks...");
    int n = WiFi.scanNetworks();
    Serial.printf("Scan found %d networks\n", n);
    bool found = false;
    for (int i = 0; i < n; ++i) {
      String s = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      int ch = WiFi.channel(i);
      bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
      Serial.printf("%d: %s (RSSI=%d ch=%d %s)\n", i+1, s.c_str(), rssi, ch, open?"OPEN":"SECURED");
      if (s.equals(String(Config::STA_SSID))) {
        Serial.printf("Found target SSID '%s' (RSSI=%d)\n", s.c_str(), rssi);
        found = true;
        // don't break; list other networks for debugging
      }
    }
    if (!found) Serial.printf("Target SSID '%s' not found in scan\n", Config::STA_SSID);
    staOk = trySTAConnect(Config::STA_SSID, Config::STA_PASSWORD, Config::STA_TIMEOUT_MS);
  }
  if (!staOk) {
    startAP(Config::AP_SSID, Config::AP_PASSWORD);
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

  // If not connected as STA, and AP fallback is active, periodically retry STA
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastStaRetry >= STA_RETRY_INTERVAL_MS) {
      lastStaRetry = now;
      Serial.println("Attempting periodic STA reconnect...");
      bool ok = trySTAConnect(Config::STA_SSID, Config::STA_PASSWORD, Config::STA_TIMEOUT_MS);
      if (ok) {
        // We successfully connected as STA; if AP was running, stop it
        if (WiFi.getMode() & WIFI_MODE_AP) {
          Serial.println("Disabling AP mode after STA success");
          WiFi.softAPdisconnect(true);
        }
      } else {
        Serial.println("Periodic STA reconnect failed; will retry later");
      }
    }
  }
}
