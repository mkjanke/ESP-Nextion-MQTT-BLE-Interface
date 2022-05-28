#ifndef NEXTIONINTERFACE_H
#define NEXTIONINTERFACE_H

#include <Arduino.h>

#include "settings.h"
#include "log.h"

class myNextionInterface {
 private:
  HardwareSerial* _serial;
  const char _cmdTerminator[3] = {'\xFF', '\xFF', '\xFF'};

  // Avoid smashing Nextion with overlapping reads and writes
  SemaphoreHandle_t _xSerialWriteSemaphore =  NULL;
  SemaphoreHandle_t _xSerialReadSemaphore = NULL;

 public:
  bool respondToBLE = false;
  const std::string eventTopic = NEXT_EVENT;
  const std::string uptimeTopic = NEXT_UPTIME;
  const std::string cmdTopic = NEXT_COMMAND;

  myNextionInterface(HardwareSerial&);

  void begin(unsigned long);
  void flushReads();

  void writeNum(const String&, uint32_t);
  void writeStr(const String&, const String&); 
  void writeCmd(const String&);

  int listen(std::string&, uint8_t);

  void wifiIcon(bool);
};



#endif  // NEXTIONINTERFACE_H