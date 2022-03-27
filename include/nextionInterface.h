#ifndef NEXTIONINTERFACE_H
#define NEXTIONINTERFACE_H

#include "settings.h"
#include "log.h"

// Avoid smashing Nextion with overlapping reads and writes
SemaphoreHandle_t xSerialWriteSemaphore =  NULL;
SemaphoreHandle_t xSerialReadSemaphore = NULL;

extern SemaphoreHandle_t xLogSemaphore;

class myNextionInterface {
 private:
  HardwareSerial* _serial;
  const char _cmdTerminator[3] = {'\xFF', '\xFF', '\xFF'};

 public:
  bool respondToBLE = false;
  const std::string eventTopic = NEXT_EVENT;
  const std::string uptimeTopic = NEXT_UPTIME;
  const std::string cmdTopic = NEXT_COMMAND;
  
  myNextionInterface(HardwareSerial& serial) {
    /* Initialize semaphores

      Note from FreeRtos docs:
      The semaphore is created in the 'empty' state, meaning the semaphore
      must first be given using the xSemaphoreGive() API function before it can
      subsequently be taken (obtained) using the xSemaphoreTake() function.
    */
    xSerialWriteSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSerialWriteSemaphore);
    xSerialReadSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSerialReadSemaphore);

    _serial = &serial;
  }

  void begin(unsigned long baud) {
    _serial->begin(baud);
    delay(100);  // Pause for effect
    flushReads();
    //Reset Nextion
    delay(400);
    sLog.send("Resetting Display");
    writeCmd("rest");
  }

  // Read and throw away serial input until no bytes (or timeout)
  void flushReads(){
    if (xSemaphoreTake(xSerialReadSemaphore, 400 / portTICK_PERIOD_MS)) {
      unsigned long _timer = millis();
      while ((_serial->available() > 0) && (millis() - _timer) < 400L ) {
        _serial->read();  // Start with clear serial port.
      }
      xSemaphoreGive(xSerialReadSemaphore);
    }
  }

  // Thread safe writes (I think...)
  void writeNum(String _componentName, uint32_t _val) {
    sLog.send((String)"writeNum: " + _componentName + ", " + _val);

    if (xSemaphoreTake(xSerialWriteSemaphore, 100 / portTICK_PERIOD_MS) ==
        pdTRUE) {
      _serial->print(_componentName);
      _serial->print("=");
      _serial->print(_val);
      _serial->print(_cmdTerminator);
      xSemaphoreGive(xSerialWriteSemaphore);
    } else {
      sLog.send("writeNum Semaphore fail");
    }
  }

  void writeStr(String command, String txt) {
    sLog.send("writeStr: " + command + ", " + txt);

    if (xSemaphoreTake(xSerialWriteSemaphore, 100 / portTICK_PERIOD_MS) ==
        pdTRUE) {
      _serial->print(command);
      _serial->print("=\"");
      _serial->print(txt);
      _serial->print("\"");
      _serial->print(_cmdTerminator);
      xSemaphoreGive(xSerialWriteSemaphore);
    } else {
      sLog.send("writeNum Semaphore fail");
    }
  }

  void writeCmd(String command) {
    sLog.send("writeCmd: " + command);

    if (xSemaphoreTake(xSerialWriteSemaphore, 1000 / portTICK_PERIOD_MS) ==
        pdTRUE) {
      _serial->print(command);
      _serial->print(_cmdTerminator);
      xSemaphoreGive(xSerialWriteSemaphore);
    } else {
      sLog.send("writeCmd Semaphore fail");
    }
  }

  // This gets called in a task or the loop().
  // Pass std::string and maximum ytes to be read
  // Returns actual bytes read and populated string
  // or 'false' if no bytes read
  int listen(std::string& _nexBytes, uint8_t _size) {
    if (xSemaphoreTake(xSerialReadSemaphore, 100 / portTICK_PERIOD_MS)) {
      if (_serial->available() > 0) {
        int _byte_read = 0;
        uint8_t _terminatorCount = 0;       // Expect three '\xFF' bytes at end of each Nextion command 
        unsigned long _timer = millis();

        while ((_nexBytes.length() < _size) && (_terminatorCount < 3) && (millis() - _timer) < 400L) {
          _byte_read = _serial->read();
          if (_byte_read != -1) {           // HardwareSerial::read returns -1 if no bytes available
            _nexBytes.push_back(_byte_read);
          }
          if (_byte_read == '\xFF') _terminatorCount++;
        }
        xSemaphoreGive(xSerialReadSemaphore);
        return _nexBytes.length();
      }
      xSemaphoreGive(xSerialReadSemaphore);
      return false;
    } else {
      sLog.send("listen() read Semaphore fail");
      return false;
    }
  }

};

#endif  // NEXTIONINTERFACE_H