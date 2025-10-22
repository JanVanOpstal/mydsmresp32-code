#ifndef myWebServercpp
#define myWebServercpp

#include "myWebServer.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "webPage.cpp"
#include <wifiConfig.h>


MyWebServer::MyWebServer(WifiConfig *wific)
  :asyncWebServer(80){
    wificonfig = wific;
}

MyWebServer::~MyWebServer(){

}

void MyWebServer::startWebServer(){
    Serial.println("################ Webserver Start #######################");


    asyncWebServer.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/html", this->mainMenuPage());
    });
    asyncWebServer.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(200, "text/html", this->wifiConfigPage());
    });

    asyncWebServer.onRequestBody([this](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (request->url() == "/savewifi") {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, (const char*)data);
            String SSID = doc["SSID"];
            String passwd = doc["passwd"];
            
            this->wificonfig->setWifiSettings(SSID, passwd);

            request->send(200, "text/json", "{}");
        }
    });


    asyncWebServer.begin();
    Serial.println("############### Webserver Started ######################");
}


String MyWebServer::mainMenuPage(){
    WebPage mainPage;
    mainPage.addButton("Wifi", "wifi");
    mainPage.addSmallSpacer();
    mainPage.addButton("MQTT", "mqtt");
    return mainPage.BuildPage();
}

String MyWebServer::wifiConfigPage(){
    WebPage wifiConfigPage;
    wifiConfigPage.title = "Wifi config";
    
    // addSSIDList(wifiConfigPage); //Process is to long. Should be it's own process and visual when finished
    wifiConfigPage.addTextbox("SSID","SSID", wificonfig->knownSSID, "SSID");

    wifiConfigPage.addPassword("Password", "Password", "", "Password");
    wifiConfigPage.addSmallSpacer();
    wifiConfigPage.addPostButton("Save", "save()", "safe");
    wifiConfigPage.addBigSpacer();
    wifiConfigPage.addPostButton("Clear", "clearW()", "danger");
    wifiConfigPage.addSmallSpacer();
    wifiConfigPage.addButton("Back", "");
    return wifiConfigPage.BuildPage();
}

void MyWebServer::addSSIDList(WebPage &wifiConfigPage){
    
    int n = wificonfig->scanChannels();
    
    String returnValue[n];
    if (n != 0){
      for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        // returnValue[i] = "<span class=\"ssid\" onclick=\"setSSID('" + wificonfig->channels[i].SSID + "')\">";
        
        // if (wificonfig->channels[i].RSSI < -79){
        //     returnValue[i] += "<span class=\"fas fa-battery-empty\">";
        // }
        // else if (wificonfig->channels[i].RSSI < -69){
        //     returnValue[i] += "<span class=\"fas fa-battery-quarter\">";
        // }
        // else if (wificonfig->channels[i].RSSI < -59){
        //     returnValue[i] += "<span class=\"fas fa-battery-half\">";
        // }
        // else if (wificonfig->channels[i].RSSI < -49){
        //     returnValue[i] += "<span class=\"fas fa-battery-three-quarters\">";
        // }
        // else {
        //     returnValue[i] += "<span class=\"fas fa-battery-full\">";
        // }


        // returnValue[i] += "</span> &nbsp;<span class=\"";
        // returnValue[i] += ((wificonfig->channels[i].encryptionType == WIFI_AUTH_OPEN)?"fas fa-unlock":"fas fa-lock");
        // returnValue[i] += "\"></span> &nbsp;" + wificonfig->channels[i].SSID + "</span><br/>";

        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(wificonfig->channels[i].SSID);
        Serial.print(" (");
        Serial.print(wificonfig->channels[i].RSSI);
        Serial.print(")");
        Serial.println((wificonfig->channels[i].encryptionType == WIFI_AUTH_OPEN)?" ":"*");
      }
    }
    // wifiConfigPage.addListOverview(returnValue, n);
}

#endif