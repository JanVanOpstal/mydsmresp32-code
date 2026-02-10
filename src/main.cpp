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
//#include "esp_task_wdt.h"


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
//#define WDT_TIMEOUT_SECONDS 10

// Wifi
#define WIFI_TIMEOUT_MS 20000;
#define MAX_BYTES_PER_READ 2048;
const int localPort = 1010; // listen for UDP packets on this port

/// @brief  DSMR type
typedef struct {
    int32_t power_delivered;
    int32_t power_delivered_l1;
    int32_t power_delivered_l2;
    int32_t power_delivered_l3;
    int32_t power_returned;
    int32_t power_returned_l1;
    int32_t power_returned_l2;
    int32_t power_returned_l3;
    uint32_t timestamp_ms;
} p1_parsed_t;

HardwareSerial SerialPort(2);
WifiConfig wifiConfig;
//WiFiUDP udp;
MyWebServer myWebServer(&wifiConfig);
static const uint8_t telegrams_queue_len = 5;

// Maximum telegram size (chars) - matches MAX_BYTES_PER_READ
#define TELEGRAM_MAX_LEN 2048

typedef struct {
  char buf[TELEGRAM_MAX_LEN + 1];
} Telegram;

static QueueHandle_t telegrams_queue;
//SensorWebserver sensorWebServer;//&wifiConfig);
//AsyncWebServer server(80);

void postTelegram(void *parameters){
  static Telegram item;
  static char payload[TELEGRAM_MAX_LEN + 16];
  static WiFiClient client;
  static HTTPClient http;
  // 1. Add this task to the Task Watchdog Timer (TWDT)
  // The handle to the current task is available via xTaskGetCurrentTaskHandle()
  // esp_task_wdt_add(NULL); // Passing NULL adds the current task

  // Loop forever
  while(1){
    // Only attempt to receive when WiFi is connected to avoid dropping messages
    if (WiFi.status() == WL_CONNECTED) {
      if (xQueueReceive(telegrams_queue, &item, 0) == pdTRUE) {
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
        
        // Build payload without using dynamic String allocations
        char payload[TELEGRAM_MAX_LEN + 16];
        // Safely skip leading '/' if present to match original behavior
        const char *body = item.buf[0] == '/' ? &item.buf[1] : item.buf;
        snprintf(payload, sizeof(payload), "telegram=/%s", body);
        int httpResponseCode = http.POST(payload);
        Serial.print("HTTPResponse: ");
        Serial.println(httpResponseCode);
        http.end();
        // we stored the telegram by value in the queue, nothing to free

        // esp_task_wdt_reset();
    }

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

// Fallback parser (same naive scanning used previously) - extracted here for reuse
static bool fallback_parse(const char *telegram, p1_parsed_t *out) {
    if (!telegram || !out) return false;
    memset(out, 0, sizeof(*out));
    const char *p;
    double v;
    p = strstr(telegram, "1-0:1.7.0(");
    if (p && sscanf(p, "1-0:1.7.0(%lf", &v) == 1) out->power_delivered = (int32_t)(v * 1000.0);
    p = strstr(telegram, "1-0:21.7.0(");
    if (p && sscanf(p, "1-0:21.7.0(%lf", &v) == 1) out->power_delivered_l1 = (int32_t)(v * 1000.0);
    p = strstr(telegram, "1-0:41.7.0(");
    if (p && sscanf(p, "1-0:41.7.0(%lf", &v) == 1) out->power_delivered_l2 = (int32_t)(v * 1000.0);
    p = strstr(telegram, "1-0:61.7.0(");
    if (p && sscanf(p, "1-0:61.7.0(%lf", &v) == 1) out->power_delivered_l3 = (int32_t)(v * 1000.0);
    p = strstr(telegram, "1-0:2.7.0(");
    if (p && sscanf(p, "1-0:2.7.0(%lf", &v) == 1) out->power_returned = (int32_t)(v * 1000.0);
    p = strstr(telegram, "1-0:22.7.0(");
    if (p && sscanf(p, "1-0:22.7.0(%lf", &v) == 1) out->power_returned_l1 = (int32_t)(v * 1000.0);
    p = strstr(telegram, "1-0:42.7.0(");
    if (p && sscanf(p, "1-0:42.7.0(%lf", &v) == 1) out->power_returned_l2 = (int32_t)(v * 1000.0);
    p = strstr(telegram, "1-0:62.7.0(");
    if (p && sscanf(p, "1-0:62.7.0(%lf", &v) == 1) out->power_returned_l3 = (int32_t)(v * 1000.0);
    if (out->power_delivered != 0 
      || out->power_returned != 0 
      || out->power_delivered_l1 != 0
      || out->power_returned_l1 != 0
      || out->power_delivered_l2 != 0
      || out->power_returned_l2 != 0
      || out->power_delivered_l3 != 0
      || out->power_returned_l3 != 0) {
        out->timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        return true;
    }
    return false;
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
      // Use a static char buffer to avoid dynamic allocations
      static char telegram[TELEGRAM_MAX_LEN + 1];
      static size_t tel_len = 0;
      const char incomingChar = SerialPort.read();

      if (tel_len < TELEGRAM_MAX_LEN) {
        telegram[tel_len++] = incomingChar;
      }

      if ('!' == incomingChar) {      /* checksum reached, wait for and read 6 more bytes then the telegram is received completely - see DSMR 5.0.2 ¶ 6.2 */
        while (SerialPort.available() < 6)
          vTaskDelay(1 / portTICK_PERIOD_MS);

        while (SerialPort.available()) {
          char c = (char)SerialPort.read();
          if (tel_len < TELEGRAM_MAX_LEN) {
            telegram[tel_len++] = c;
          }
        }

        telegram[tel_len] = '\0';

        Telegram msg;
        // copy and ensure NUL termination
        strncpy(msg.buf, telegram, TELEGRAM_MAX_LEN);
        msg.buf[TELEGRAM_MAX_LEN] = '\0';

        if (xQueueSend(telegrams_queue, &msg, pdMS_TO_TICKS(10)) != pdTRUE){
          Serial.println("Queue full");
        }

        // Get Power from telegram
        p1_parsed_t data;
        bool result = fallback_parse(telegram, &data);

        if (!result) {
          Serial.print("Error parsing telegram");
        }
        else 
        {
          int32_t netPower = data.power_delivered-data.power_returned;
          int32_t netPowerL1 = data.power_delivered_l1-data.power_returned_l1;
          int32_t netPowerL2 = data.power_delivered_l2-data.power_returned_l2;
          int32_t netPowerL3 = data.power_delivered_l3-data.power_returned_l3;
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
        
        tel_len = 0;
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
  //esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
  // Use queue of Telegram structs to avoid dynamic allocations.
  // FreeRTOS queues copy raw bytes; we'll queue the Telegram buffer by value.
  telegrams_queue = xQueueCreate(telegrams_queue_len, sizeof(Telegram));

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
    8000,
    NULL,
    1,
    NULL
  );

  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(pdMS_TO_TICKS(10)); // FreeRTOS: yield for 10ms
    // esp_task_wdt_reset();
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
