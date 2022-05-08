#include "log.h"
#include "bleInterface.h"
#include <WiFi.h>

extern bleInterface bleIF;

WiFiUDP udpClient;
Logger sLog(udpClient, SYSLOG_PROTO_BSD);
 
void Logger::init(){
  server(SYSLOG_HOST, SYSLOG_PORT);
  deviceHostname(DEVICE_NAME);
  defaultPriority(LOG_INFO);
 }

void Logger::send(String &message, bool ble) {
  if (WiFi.isConnected()){
    log(LOG_INFO, message.c_str());
  }
  if (ble)
    bleIF.updateStatus(message.c_str());
}
void Logger::send(const char *message, bool ble) {
  if (WiFi.isConnected()){
      log(LOG_INFO, message);
  }
  if (ble)
    bleIF.updateStatus(message);
}
