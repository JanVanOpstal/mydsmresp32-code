#ifndef wifiConfigcpp
#define wifiConfigcpp

#include "wifiConfig.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "webPage.cpp"


WifiConfig::WifiConfig()
{
}
WifiConfig::~WifiConfig(){
    wifiPreferences.end();
    //delete [] channels;
}

void WifiConfig::startWifi(){
    wifiPreferences.begin("wifi-config");
    knownSSID = wifiPreferences.getString("SSID","");
    knownPasswd = wifiPreferences.getString("WifiPasswd","");
    Serial.println(knownSSID + " " + knownPasswd);
    if (knownSSID == "" || knownPasswd == ""){
        setAPMode();
    }
    else {
        setSTAMode();
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void WifiConfig::setSTAMode(){
    Serial.println("################## Wifi Access point ###################");
    if (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_CONNECTED){
      WiFi.disconnect();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    currentMode = WIFI_MODE_STA;
    WiFi.mode(currentMode);
    WiFi.begin(knownSSID.c_str(), knownPasswd.c_str());

    Serial.println("################### Wifi Connecting ####################");
    int n = 0;
    while (WiFi.status() != WL_CONNECTED && n < 600) {
      n += 1;
      vTaskDelay(100 / portTICK_PERIOD_MS);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED){
      Serial.println("Connected to " + knownSSID);
    }
    else{
      Serial.println("Switch to Access point mode.");
      setAPMode();
    }
}

void WifiConfig::setAPMode(){
    IPAddress accesspointIP = IPAddress(192, 168, 4, 1);
    Serial.println("################## Wifi Access point ###################");
    if (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_CONNECTED){
      WiFi.disconnect();
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    Serial.println("SMPS_SETUP");
    Serial.println(accesspointIP);
    
    WiFi.softAPConfig(accesspointIP, accesspointIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("SMPS_SETUP");
    currentMode = WIFI_MODE_AP;
    WiFi.mode(currentMode);
    
    Serial.println("Access point online.");
}

int WifiConfig::scanChannels(){
  int n = WiFi.scanNetworks();
  WiFiChannel *localChannels = new WiFiChannel[n];
  vTaskDelay(100 / portTICK_PERIOD_MS);
  Serial.println("");

  for (int i = 0; i < n; ++i){
    localChannels[i].SSID = WiFi.SSID(i);
    localChannels[i].RSSI = WiFi.RSSI(i);
    localChannels[i].encryptionType = WiFi.encryptionType(i);
  }
  channels = localChannels;

  if (currentMode == WIFI_MODE_AP){
    setAPMode();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  return n;
}

void WifiConfig::setWifiSettings(String toSSID, String toPasswd){ 
  Serial.println("####################### Set SSID #######################");   
  wifiPreferences.putString("SSID", toSSID);
  wifiPreferences.putString("WifiPasswd", toPasswd);
  knownPasswd = toPasswd;
  knownSSID = toSSID;
  vTaskDelay(100 / portTICK_PERIOD_MS);
  setSTAMode();
}

#endif