#ifndef wifiConfigh
#define wifiConfigh

#include <Arduino.h>
#include <sensorWebServer.cpp>
#include <Preferences.h>
#include <WiFi.h>

struct WiFiChannel{
    String SSID;
    int32_t RSSI;
    wifi_auth_mode_t encryptionType;

    WiFiChannel(){
        SSID = "";
        RSSI = 0;
    }
};

class WifiConfig{
    private:
        Preferences wifiPreferences;
        String knownPasswd = "";
        wifi_mode_t currentMode = WIFI_MODE_AP;

        void setSTAMode();
        void setAPMode();
    public:
        WiFiChannel* channels = new WiFiChannel[1];
        String knownSSID = "";
        bool connected = false;

        WifiConfig();
        ~WifiConfig();
        
        void startWifi();
        int scanChannels();
        void setWifiSettings(String toSSID, String toPasswd);
};

#endif