#include "log.h"
#include "bleInterface.h"
#include <WiFi.h>

Logger sLog;
extern bleInterface bleIF;
 
#ifdef TLOG_TELNET
#include <TelnetSerialStream.h>
TelnetSerialStream telnetSerialStream = TelnetSerialStream();
#endif

#ifdef TLOG_SYSLOG
#include <SyslogStream.h>
#define SYSLOG_HOST "192.168.1.7"
SyslogStream syslogStream = SyslogStream();
#endif

SemaphoreHandle_t xLogSemaphore = NULL;  // Avoid smashing Log with overlapping writes

void Logger::init(){
  // Stop the default TLog instancea from logging to serial
  Log.disableSerial(true);
  Debug.disableSerial(true);

  xLogSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(xLogSemaphore);
  disableSerial(true);
  
  #ifdef TLOG_TELNET
  addPrintStream(std::make_shared<TelnetSerialStream>(telnetSerialStream));
  #endif
  
  #ifdef TLOG_SYSLOG
  syslogStream.setDestination(SYSLOG_HOST);
  syslogStream.setIdentifier(DEVICE_NAME);
  syslogStream.setRaw(true);

  const std::shared_ptr<LOGBase> syslogStreamPtr = std::make_shared<SyslogStream>(syslogStream);
  addPrintStream(syslogStreamPtr);
  syslogStream.begin();
  #endif

  begin();
}

void Logger::send(String &message, bool ble) {
  if (WiFi.isConnected()){
    if (xSemaphoreTake(xLogSemaphore, 100 / portTICK_PERIOD_MS) == pdTRUE) {
      println((String)DEVICE_NAME + ": " + message);
      xSemaphoreGive(xLogSemaphore);
    }
  }
  if (ble)
    bleIF.updateStatus(message.c_str());
}
void Logger::send(const char *message, bool ble) {
  if (WiFi.isConnected()){
    if (xSemaphoreTake(xLogSemaphore, 100 / portTICK_PERIOD_MS) == pdTRUE) {
      println((String)DEVICE_NAME + ": " + message);
      xSemaphoreGive(xLogSemaphore);
    }
  }
  if (ble)
    bleIF.updateStatus(message);
}
