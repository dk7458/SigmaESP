#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
    // STA credentials (device will try to connect as station first)
    static const char* STA_SSID = "<nevada>";
    static const char* STA_PASSWORD = "<Joska1948>";

    // AP fallback credentials (used if STA connection fails)
    static const char* AP_SSID = "ADAU_AP";
    static const char* AP_PASSWORD = "12345678"; // keep >=8 chars

    // How long to attempt STA connection (ms)
    // Increased default to 30s to allow for slow AP responses
    static const unsigned long STA_TIMEOUT_MS = 30000UL;
}

#endif // CONFIG_H
