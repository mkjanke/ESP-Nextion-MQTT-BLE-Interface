#include "log.h"
#include "bleInterface.h"
#include <WiFi.h>

extern bleInterface bleIF;

WiFiUDP udpClient;
Logger sLog(udpClient, SYSLOG_PROTO_BSD);

SemaphoreHandle_t _xLogSemaphore = NULL;  // Avoid smashing Log with overlapping writes

void Logger::init(){
  server(SYSLOG_HOST, SYSLOG_PORT);
  deviceHostname(DEVICE_NAME);
  defaultPriority(LOG_INFO);
  // Serial.begin(115200);
  setSerialPrint(false);

  _xLogSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(_xLogSemaphore);
 }

void Logger::send(const String &message, bool ble) {
  if (WiFi.isConnected()){
    if (_xLogSemaphore != NULL) {
      if (xSemaphoreTake(_xLogSemaphore, 100 / portTICK_PERIOD_MS) == pdTRUE) {
        log(LOG_INFO, message);
        xSemaphoreGive(_xLogSemaphore);
      }
    }
  }
  if (ble)
    bleIF.updateStatus(message.c_str());
}
void Logger::send(const char *message, bool ble) {
  if (WiFi.isConnected()){
    if (_xLogSemaphore != NULL) {
      if (xSemaphoreTake(_xLogSemaphore, 100 / portTICK_PERIOD_MS) == pdTRUE) {
        log(LOG_INFO, message);
        xSemaphoreGive(_xLogSemaphore);
      }
    }
  }
  if (ble)
    bleIF.updateStatus(message);
}
