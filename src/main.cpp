/*
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * Example that shows how to parse a P1 message and automatically print
 * the result.
*/

#include <Arduino.h> // including freeRTOS
#include <HardwareSerial.h>
#include <wifiConfig.h>
#include <sensorWebServer.cpp>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <shellyudpserver.cpp>
#include "esp_task_wdt.h"


#include "netpowerdata.h"

NetPowerData g_netPowerData = {0};
SemaphoreHandle_t netPowerMutex = NULL;

void updateNetPower(int32_t np, int32_t l1, int32_t l2, int32_t l3) {
    if (xSemaphoreTake(netPowerMutex, portMAX_DELAY)) {
        g_netPowerData.netPower = np;
        g_netPowerData.netPowerL1 = l1;
        g_netPowerData.netPowerL2 = l2;
        g_netPowerData.netPowerL3 = l3;
        xSemaphoreGive(netPowerMutex);
    }
}

#include <ESPAsyncWebServer.h>
#include <myWebServer.h>

//#Watchdog timer timeout
#define WDT_TIMEOUT_SECONDS 10

//#include <WiFi.h>
#include "dsmr.h"

// Wifi
#define WIFI_TIMEOUT_MS 20000;
#define MAX_BYTES_PER_READ 2048;
const int localPort = 1010; // listen for UDP packets on this port

// Telegram type
using MyData = ParsedData<
  /* FixedValue */ power_delivered,
  /* FixedValue */ power_returned,
  /* FixedValue */ power_delivered_l1,
  /* FixedValue */ power_delivered_l2,
  /* FixedValue */ power_delivered_l3,
  /* FixedValue */ power_returned_l1,
  /* FixedValue */ power_returned_l2,
  /* FixedValue */ power_returned_l3
>;

HardwareSerial SerialPort(2);
WifiConfig wifiConfig;
//WiFiUDP udp;
MyWebServer myWebServer(&wifiConfig);
static const uint8_t telegrams_queue_len = 5;

static QueueHandle_t telegrams_queue;
//SensorWebserver sensorWebServer;//&wifiConfig);
//AsyncWebServer server(80);

void postTelegram(void *parameters){
  String item;
  // 1. Add this task to the Task Watchdog Timer (TWDT)
  // The handle to the current task is available via xTaskGetCurrentTaskHandle()
  esp_task_wdt_add(NULL); // Passing NULL adds the current task

  // Loop forever
  while(1){
    // See if there's a message in the queue (do not block)
    if (xQueueReceive(telegrams_queue, (void *)&item, 0) == pdTRUE && WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;

        // Replace [IP or URL] withe the address of your DSMR API endpoint (if needed add port after the domain or IP)
        http.begin("http://[IP or URL]/api/v1/datalogger/dsmrreading");
        // Replace [yourtoken] with the token from dsmr api
        http.addHeader("authorization", "Token [yourtoken]");
        //http.addHeader("Content-Type", "application/json");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        //const size_t capacity = JSON_OBJECT_SIZE(1) + 16;
        //StaticJsonDocument<1000> doc;
        //doc["telegram"] = item;
        //String jsonData = "";
        //serializeJson(doc, jsonData);
        //Serial.println(jsonData);
        
        int httpResponseCode = http.POST("telegram=/" + item.substring(1));
        Serial.print("HTTPResponse: ");
        Serial.println(httpResponseCode);

        esp_task_wdt_reset();
    }

    // Wait before trying again
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void keepWiFiAlive(void * parameters){
  for(;;){
    if(WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_MODE_AP){
      //Serial.println("WiFi available");
      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }
    
    // Serial.println("WiFi Connecting");
    // WiFi.mode(WIFI_STA);
    // WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

    // unsigned long startAttemptTime = millis();

    // // Keep loop!ing while we're not connected and haven't reached the timeout
    // while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS){}

    wifiConfig.startWifi();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    myWebServer.startWebServer();
    //sensorWebServer.startSensorWebServer();
    
    // When we couldn't make a WiFi connection
    if (WiFi.status() != WL_CONNECTED){
      if (WiFi.getMode() == WIFI_MODE_AP){
        Serial.println("[WIFI] Not yet configured");
      }
      else {
        Serial.println("[WIFI] Failed");
      }
      vTaskDelay(20000 / portTICK_PERIOD_MS);
      continue;
    }

    Serial.print("[WIFI] Connected: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.dnsIP());
    
  // UDP server is now handled in shellyudpserver.cpp
  }
}

void read_P1(void * parameters){

  //while (SerialPort.available()){
  for (;;){
    vTaskDelay(1 / portTICK_PERIOD_MS);
    // if (SerialPort.available()){
    //   Serial.println("available");
    // }
    // else{
    //   Serial.println("unavailable");
    // }
    while(SerialPort.available()){
      //Serial.println("Read Telegram");
      static String telegram{""};
      const char incomingChar = SerialPort.read();
      
      telegram.concat(incomingChar);
      //Serial.println(telegram);
      if ('!' == incomingChar) {      /* checksum reached, wait for and read 6 more bytes then the telegram is received completely - see DSMR 5.0.2 ¶ 6.2 */
        while (SerialPort.available() < 6)
          vTaskDelay(1 / portTICK_PERIOD_MS);

        while (SerialPort.available())
          telegram.concat((char)SerialPort.read());

        //Serial.println(telegram);
        if (xQueueSend(telegrams_queue, (void *)&telegram, 10) != pdTRUE){
          Serial.println("Queue full");
        }

        // Get Power from telegram
        MyData data;
        ParseResult<void> result = P1Parser::parse(&data, telegram.c_str(), telegram.length());
        if (result.err) {
          Serial.print("Error parsing telegram: ");
          Serial.println(result.err);
        }
        else if (!data.all_present()) {
          Serial.println("Not all data present");
        }
        else 
        {
          int32_t netPower = (int32_t)data.power_delivered.int_val()-(int32_t)data.power_returned.int_val();
          int32_t netPowerL1 = (int32_t)data.power_delivered_l1.int_val()-(int32_t)data.power_returned_l1.int_val();
          int32_t netPowerL2 = (int32_t)data.power_delivered_l2.int_val()-(int32_t)data.power_returned_l2.int_val();
          int32_t netPowerL3 = (int32_t)data.power_delivered_l3.int_val()-(int32_t)data.power_returned_l3.int_val();
          updateNetPower(netPower, netPowerL1, netPowerL2, netPowerL3);

          Serial.print("Power L1: ");
          Serial.print(netPowerL1);
          Serial.print(" W, L2: ");
          Serial.print(netPowerL2);
          Serial.print(" W, L3: ");
          Serial.print(netPowerL3);
          Serial.print(" W, Total: ");
          //float totalPower = data.power_delivered_l1.int_val() + data.power_delivered_l2.int_val() + data.power_delivered_l3.int_val();
          Serial.print(netPower);
          Serial.println(" W");
        }

        telegram = "";
        Serial.println("Read Telegram!");
     }
    }
  }
  //delay(1);
}

// // Data to parse
// const char msg[] =
//   "/KFM5KAIFA-METER\r\n"
//   "\r\n"
//   "1-0:1.8.1(000671.578*kWh)\r\n"
//   "1-0:1.7.0(00.318*kW)\r\n"
//   "!1E1D\r\n";

// /**
//  * Define the data we're interested in, as well as the datastructure to
//  * hold the parsed data.
//  * Each template argument below results in a field of the same name.
//  */
// using MyData = ParsedData<
//   /* String */ identification,
//   /* FixedValue */ power_delivered,
//   energy_delivered_tariff1
// >;


void setup() {
  Serial.begin(115200); //.begin(115200, SERIAL_8N1, 18,19);
  SerialPort.begin(115200, SERIAL_8N1, 18, 17);
  // Initialize WDT with a 5 second timeout and enable panic mode (reboot on timeout)
  esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
  telegrams_queue = xQueueCreate(telegrams_queue_len, sizeof(String));

    // Create mutex for net power data
    netPowerMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    keepWiFiAlive,
    "Keep WiFi Alive",
    5000,
    NULL,
    1,
    NULL,
    CONFIG_ARDUINO_RUNNING_CORE
  ); // Run task on same core as Arduino (needed for WiFi)

  xTaskCreate(
    read_P1,
    "Read P1 port telegrams",
    12000,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    postTelegram,
    "Post telegrams",
    6000,
    NULL,
    1,
    NULL
  );

  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(pdMS_TO_TICKS(10)); // FreeRTOS: yield for 10ms
  }
  
  start_udp_server();
  // xTaskCreate(
  //   marstekInject,
  //   "Marstek P1 Simulation",
  //   8192,
  //   NULL,
  //   1,
  //   NULL
  // );
}

void loop () {
  for (;;){
  //   vTaskDelay(1 / portTICK_PERIOD_MS);
  //   if (SerialPort.available()){
  //     Serial.println("Ltest");
  //   }
  //   else{
      // Serial.println("Ltest2");
      // String telegram = "Blablabla data";
      // if (xQueueSend(telegrams_queue, (void *)&telegram, 10) != pdTRUE){
      //     Serial.println("Queue full");
      //   }
  //   }
    delay(10000);
  }
}