#include <Arduino.h>
#include "bleInterface.h"
#include "mqttInterface.h"
#include "nextionInterface.h"

extern TaskHandle_t xheartBeatHandle;  // Task handles
extern TaskHandle_t xhandleNextionHandle;
extern TaskHandle_t xcheckWiFiHandle;

extern bleInterface bleIF;
extern myNextionInterface myNex;

char uptimeBuffer[20];  // scratch space for storing formatted 'uptime' string

// Calculate uptime & populate uptime buffer for future use
void uptime() {
  // Constants for uptime calculations
  static const uint32_t millis_in_day = 1000 * 60 * 60 * 24;
  static const uint32_t millis_in_hour = 1000 * 60 * 60;
  static const uint32_t millis_in_minute = 1000 * 60;

  uint8_t days = millis() / (millis_in_day);
  uint8_t hours = (millis() - (days * millis_in_day)) / millis_in_hour;
  uint8_t minutes =
      (millis() - (days * millis_in_day) - (hours * millis_in_hour)) /
      millis_in_minute;
  snprintf(uptimeBuffer, sizeof(uptimeBuffer), "Uptime: %2dd%2dh%2dm", days,
           hours, minutes);
}

// // Heartbeat task - update uptime buffer
// //                - update heartbeat field on Nextion
void heartBeat(void* parameter) {
  uint8_t loopct = 0;
  
  for (;;) {  // ever
    loopct++;
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    uptime();
    sLog.send(uptimeBuffer);

    mqtt::NexClient.publish(myNex.uptimeTopic, uptimeBuffer);

    myNex.writeStr(NEXT_UPTIME_WIDGET, uptimeBuffer);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    myNex.writeNum("heartbeat", 1);

    char buffer[48];
    snprintf(buffer, sizeof(buffer), "N:%ib\\rH:%ib\\rW:%ib\\rH:%i,%i",
             uxTaskGetStackHighWaterMark(xhandleNextionHandle),
             uxTaskGetStackHighWaterMark(xheartBeatHandle),
             uxTaskGetStackHighWaterMark(xcheckWiFiHandle),
             esp_get_minimum_free_heap_size(),
             heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

    myNex.writeStr(NEXT_ESPOUT_WIDGET, buffer);

    snprintf(buffer, sizeof(buffer), "%i", esp_get_minimum_free_heap_size());
    mqtt::NexClient.publish(NEXT_ESP_FREEHEAP, buffer);

    snprintf(buffer, sizeof(buffer), "%i",
             uxTaskGetStackHighWaterMark(xhandleNextionHandle));
    mqtt::NexClient.publish(NEXT_ESP_NSTACK, buffer);

    snprintf(buffer, sizeof(buffer), "%i",
             uxTaskGetStackHighWaterMark(xheartBeatHandle));
    mqtt::NexClient.publish(NEXT_ESP_HSTACK, buffer);

    snprintf(buffer, sizeof(buffer), "%i",
             uxTaskGetStackHighWaterMark(xcheckWiFiHandle));
    mqtt::NexClient.publish(NEXT_ESP_WSTACK, buffer);

    bleIF.updateUptime(uptimeBuffer);

    if (loopct > 100) {
      loopct = 0;
    }
  }
  vTaskDelete(NULL);  // Should never reach this.
}  // heartBeat()