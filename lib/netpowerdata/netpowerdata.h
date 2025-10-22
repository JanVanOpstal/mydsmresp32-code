#ifndef NETPOWERDATA_H
#define NETPOWERDATA_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Net power data structure and mutex
typedef struct {
    int32_t netPower;
    int32_t netPowerL1;
    int32_t netPowerL2;
    int32_t netPowerL3;
} NetPowerData;

extern NetPowerData g_netPowerData;
extern SemaphoreHandle_t netPowerMutex;

void updateNetPower(int32_t np, int32_t l1, int32_t l2, int32_t l3);

#endif // NETPOWERDATA_H
