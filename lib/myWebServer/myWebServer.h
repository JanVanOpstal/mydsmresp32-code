#ifndef myWebServerh
#define myWebServerh

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <wifiConfig.h>
#include "webPage.cpp"

class MyWebServer{
    private:
        AsyncWebServer asyncWebServer;
        WifiConfig* wificonfig;

        String mainMenuPage();
        String wifiConfigPage();
        void addSSIDList(WebPage &wifiConfigPage);

    public:
        MyWebServer(WifiConfig *wifiConfig);
        ~MyWebServer();
        
        void startWebServer();
};

#endif