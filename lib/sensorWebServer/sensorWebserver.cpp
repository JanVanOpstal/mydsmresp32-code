#ifndef sensorWebServercpp
#define sensorWebServercpp

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <webPage.cpp>
#include <wifiConfig.h>
//#include <mqttClient.cpp>



class SensorWebserver{
  public:
  //WifiConfig* wificonfig;
  // MQTTClient* mqttclient;
  
  AsyncWebServer  webserverLocal;
  String newSSID = "";

  SensorWebserver()//WifiConfig *wific)//, MQTTClient *mqttc)
    :webserverLocal(80)
  {
    //wificonfig = wific;
    //mqttclient = mqttc;
  }

  ~SensorWebserver(){
  }

  void startSensorWebServer(){
    Serial.println("################ Webserver Start #######################");
    webserverLocal.on("/me", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.println("yeahy");
      request->send(200, "text/html", "Hier zie");
    });
    // webserverLocal.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    //   request->send(200, "text/html", this->mainMenuPage());
    // });
    // webserverLocal.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request){
    //   request->send(200, "text/html", this->wifiConfigPage());
    // });
    // // webserverLocal.on("/mqtt", [this](){
    // //   this->webserverLocal.send(200, "text/html", this->mqttConfigPage());
    // // });
    // webserverLocal.on("/savewifi", HTTP_POST, [this](AsyncWebServerRequest *request){

    //   int params = request->params(); // 0
    //   Serial.println(params);
    //   if (request->hasParam("body", true)) { // This is important, otherwise the sketch will crash if there is no body
    //     DynamicJsonDocument doc(1024);
    //     deserializeJson(doc, request->getParam("body",true)->value().c_str());
    //     String SSID = doc["SSID"];
    //     // Use SSID in wifi connection page.
    //     newSSID = SSID;
    //     String passwd = doc["passwd"];
    //     request->send(200, "text/json", "{}");
    //   } else {
    //     Serial.println("No body?!");
    //     request->send(200, "text/plain", "No body?!\n");
    //   }

      
    //   //this->wificonfig->setWifiSettings(SSID, passwd);
    // });
    // // webserverLocal.on("/savemqtt", HTTP_POST, [this](){
    // //   DynamicJsonDocument doc(1024);
    // //   deserializeJson(doc, this->webserverLocal.arg(0));
    // //   String host = doc["host"];
    // //   String port = doc["port"];
    // //   String clientID = doc["clientID"];
    // //   String topic = doc["topic"];
    // //   String user = doc["user"];
    // //   String passwd = doc["passwd"];

    // //   webserverLocal.send(200, "text/json", "{}");
    // //   this->mqttclient->setMQTTSettings(host,port.toInt(),clientID,topic,user,passwd);
    // // });
    // webserverLocal.on("/clearwifi", HTTP_POST, [this](AsyncWebServerRequest *request){
    //   request->send(200, "text/json", "{}");
    //   // wificonfig->setAPMode();
    // });
    // webserverLocal.on("/connecting", [this](AsyncWebServerRequest *request){
    //   request->send(200, "text/html", this->wifiRedirectPage());
    // });
    
    webserverLocal.begin();
    Serial.println("############### Webserver Started ######################");
  }

  void setLocalWifiSettings(String toSSID, String toPasswd){
    // wificonfig->setWifiSettings(toSSID, toPasswd);
  }
  
  // void handleClient(){
  //     webserverLocal.handleClient();
  // }

  private:
  void addSSIDList(WebPage &currentPage){
    // int n = wificonfig->scanChannels();
    
    // String returnValue[n];
    // if (n != 0){
    //   for (int i = 0; i < n; ++i) {
    //     // Print SSID and RSSI for each network found
    //     returnValue[i] = "<span class=\"ssid\" onclick=\"setSSID('" + wificonfig->channels[i].SSID + "')\">";
        
    //     if (wificonfig->channels[i].RSSI < -79){
    //         returnValue[i] += "<span class=\"fas fa-battery-empty\">";
    //     }
    //     else if (wificonfig->channels[i].RSSI < -69){
    //         returnValue[i] += "<span class=\"fas fa-battery-quarter\">";
    //     }
    //     else if (wificonfig->channels[i].RSSI < -59){
    //         returnValue[i] += "<span class=\"fas fa-battery-half\">";
    //     }
    //     else if (wificonfig->channels[i].RSSI < -49){
    //         returnValue[i] += "<span class=\"fas fa-battery-three-quarters\">";
    //     }
    //     else {
    //         returnValue[i] += "<span class=\"fas fa-battery-full\">";
    //     }


    //     returnValue[i] += "</span> &nbsp;<span class=\"";
    //     returnValue[i] += ((wificonfig->channels[i].encryptionType == WIFI_AUTH_OPEN)?"fas fa-unlock":"fas fa-lock");
    //     returnValue[i] += "\"></span> &nbsp;" + wificonfig->channels[i].SSID + "</span><br/>";

    //     Serial.print(i + 1);
    //     Serial.print(": ");
    //     Serial.print(wificonfig->channels[i].SSID);
    //     Serial.print(" (");
    //     Serial.print(wificonfig->channels[i].RSSI);
    //     Serial.print(")");
    //     Serial.println((wificonfig->channels[i].encryptionType == WIFI_AUTH_OPEN)?" ":"*");
    //   }
    // }
    // currentPage.addListOverview(returnValue, n);
  }

  String mainMenuPage(){
      WebPage mainPage;
      mainPage.addButton("Wifi", "wifi");
      mainPage.addSmallSpacer();
      mainPage.addButton("MQTT", "mqtt");
      return mainPage.BuildPage();
  }

  String wifiConfigPage(){
      WebPage wifiConfigPage;
      wifiConfigPage.title = "Wifi config";
      addSSIDList(wifiConfigPage);
      //wifiConfigPage.addTextbox("SSID","SSID", wificonfig->knownSSID, "SSID");
      
      wifiConfigPage.addPassword("Password", "Password", "", "Password");
      wifiConfigPage.addSmallSpacer();
      wifiConfigPage.addPostButton("Save", "save()", "safe");
      wifiConfigPage.addBigSpacer();
      wifiConfigPage.addPostButton("Clear", "clearW()", "danger");
      wifiConfigPage.addSmallSpacer();
      wifiConfigPage.addButton("Back", "");
      return wifiConfigPage.BuildPage();
  }

  // String mqttConfigPage(){
  //     WebPage mqttConfigPage;
  //     mqttConfigPage.title = "MQTT config";
  //     mqttConfigPage.addTextbox("Host", "Host", mqttclient->host, "Host");
  //     mqttConfigPage.addTextbox("Port", "Port", String(mqttclient->port), "Port");
  //     mqttConfigPage.addTextbox("Client", "Client", mqttclient->clientID, "Client");
  //     mqttConfigPage.addTextbox("User", "User", mqttclient->user, "User");
  //     mqttConfigPage.addPassword("Password", "Password", "", "Password");
  //     mqttConfigPage.addTextbox("Topic", "Topic", mqttclient->topic, "Topic");
  //     mqttConfigPage.addSmallSpacer();
  //     mqttConfigPage.addPostButton("Save", "saveMQTT()", "safe");
  //     mqttConfigPage.addSpacer();
  //     mqttConfigPage.addButton("Back", "");
  //     return mqttConfigPage.BuildPage();
  // }

  String wifiRedirectPage(){
    WebPage wifiRedirectPage;
    wifiRedirectPage.title = "Connection to SSID: " + newSSID;
    wifiRedirectPage.addButton("Home", "");
    return wifiRedirectPage.BuildPage();
  }

};

#endif
