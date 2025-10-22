#ifndef shellyudpservercpp
#define shellyudpservercpp

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>


#include "netpowerdata.h"

// UDP server config
static const int UDP_PORT = 1010;
static const char *DEVICE_ID = "esp32-shelly";
WiFiUDP udp;
SemaphoreHandle_t sendMutex;


// float calculate_derived_value(int32_t power) {
//     float decimal_point_enforcer = 0.001f;
//     // if (fabs(power) < 0.1f) return decimal_point_enforcer;
//     // float rounded = roundf(power);
//     // if (power == rounded || power == 0.0f) return power + decimal_point_enforcer;
//     return (float)power + decimal_point_enforcer;
// }

void create_em_response(JsonDocument &doc, int request_id, NetPowerData data, size_t n) {
    // int32_t a = data.netPowerL1;
    // String b = roundf(calculate_derived_value(data.netPowerL2) * 1000.0f) / 1000.0f;
    // String c = roundf(calculate_derived_value(data.netPowerL3) * 1000.0f) / 1000.0f;
    // String total = roundf(calculate_derived_value(data.netPower) * 1000.0f) / 1000.0f;
    // for (size_t i = 0; i < n; ++i) total += powers[i];
    // total = roundf(total * 1000.0f) / 1000.0f;
    // if (total == roundf(total) || total == 0.0f) total += 0.001f;
    doc["id"] = request_id;
    doc["src"] = DEVICE_ID;
    doc["dst"] = "unknown";
    JsonObject result = doc.createNestedObject("result");
    result["a_act_power"] = data.netPowerL1;
    result["b_act_power"] = data.netPowerL2;
    result["c_act_power"] = data.netPowerL3;
    result["total_act_power"] = data.netPower;
}

void create_em1_response(JsonDocument &doc, int request_id, NetPowerData data, size_t n) {
    //float total = calculate_derived_value(data.netPower);
    doc["id"] = request_id;
    doc["src"] = DEVICE_ID;
    doc["dst"] = "unknown";
    JsonObject result = doc.createNestedObject("result");
    result["act_power"] = data.netPower;
}

void udp_server_task(void *param) {
    Serial.println("Start UDP server");
    char incomingPacket[512];
    udp.begin(UDP_PORT);
    while (1) {
        int packetSize = udp.parsePacket();
        if (packetSize > 0) {
            int len = udp.read(incomingPacket, sizeof(incomingPacket) - 1);
            if (len > 0 && len < sizeof(incomingPacket)) {
                incomingPacket[len] = '\0';
                StaticJsonDocument<512> request;
                DeserializationError error = deserializeJson(request, incomingPacket);
                if (!error) {
                    int req_id = request["id"] | 0;
                    const char *method = request["method"] | "";
                    NetPowerData powers = {0};
                    //float powers[3] = {0};
                    // Read net power values (thread-safe)
                    if (xSemaphoreTake(netPowerMutex, portMAX_DELAY)) {
                        powers.netPowerL1 = g_netPowerData.netPowerL1;
                        powers.netPowerL2 = g_netPowerData.netPowerL2;
                        powers.netPowerL3 = g_netPowerData.netPowerL3;
                        powers.netPower = g_netPowerData.netPower;
                        xSemaphoreGive(netPowerMutex);
                    }
                    StaticJsonDocument<512> response;
                    if (strcmp(method, "EM.GetStatus") == 0) {
                        create_em_response(response, req_id, powers, 3);
                    } else if (strcmp(method, "EM1.GetStatus") == 0) {
                        create_em1_response(response, req_id, powers, 3);
                    } else {
                        continue;
                    }
                    Serial.printf("Sending UDP JSON: %s\n", response.as<String>().c_str());
                    char outBuf[512];
                    size_t outLen = serializeJson(response, outBuf, sizeof(outBuf));
                    xSemaphoreTake(sendMutex, portMAX_DELAY);
                    udp.beginPacket(udp.remoteIP(), udp.remotePort());
                    udp.write((const uint8_t*)outBuf, outLen);
                    udp.endPacket();
                    xSemaphoreGive(sendMutex);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void start_udp_server() {
    sendMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(udp_server_task, "UDPServer", 4096, NULL, 1, NULL, 1);
}

// In your setup() call start_udp_server() after WiFi is connected.


#endif