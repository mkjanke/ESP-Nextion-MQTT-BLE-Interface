#ifndef NEXTIONINTERFACE_H
#define NEXTIONINTERFACE_H

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
  
  myNextionInterface(HardwareSerial& serial) {
    /* Initialize semaphores

      Note from FreeRtos docs:
      The semaphore is created in the 'empty' state, meaning the semaphore
      must first be given using the xSemaphoreGive() API function before it can
      subsequently be taken (obtained) using the xSemaphoreTake() function.
    */
    _xSerialWriteSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(_xSerialWriteSemaphore);
    _xSerialReadSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(_xSerialReadSemaphore);

    _serial = &serial;
  }

  void begin(unsigned long baud) {
    _serial->begin(baud,SERIAL_8N1, RXDN, TXDN);
    delay(100);  // Pause for effect
    flushReads();
    //Reset Nextion
    delay(400);
    sLog.send("Resetting Display");
    writeCmd("rest");
  }

  // Read and throw away serial input until no bytes (or timeout)
  void flushReads(){
    if(_xSerialReadSemaphore != NULL ){
      if (xSemaphoreTake(_xSerialReadSemaphore, 400 / portTICK_PERIOD_MS)) {
        unsigned long _timer = millis();
        while ((_serial->available() > 0) && (millis() - _timer) < 400L ) {
          _serial->read();  // Start with clear serial port.
        }
        xSemaphoreGive(_xSerialReadSemaphore);
      }
    }
  }

  // Thread safe writes (I think...)
  void writeNum(const String& _componentName, uint32_t _val) {
    sLog.send((String)"writeNum: " + _componentName + ", " + _val);
    if(_xSerialWriteSemaphore != NULL ){
      if (xSemaphoreTake(_xSerialWriteSemaphore, 100 / portTICK_PERIOD_MS) ==
          pdTRUE) {
        _serial->print(_componentName);
        _serial->print("=");
        _serial->print(_val);
        _serial->print(_cmdTerminator);
        xSemaphoreGive(_xSerialWriteSemaphore);
      } else {
        sLog.send("writeNum Semaphore fail");
      }
    }
  } //writeNum()

  void writeStr(const String& command, const String& txt) {
    sLog.send("writeStr: " + command + ", " + txt);
    if(_xSerialWriteSemaphore != NULL ){
      if (xSemaphoreTake(_xSerialWriteSemaphore, 100 / portTICK_PERIOD_MS) ==
          pdTRUE) {
        _serial->print(command);
        _serial->print("=\"");
        _serial->print(txt);
        _serial->print("\"");
        _serial->print(_cmdTerminator);
        xSemaphoreGive(_xSerialWriteSemaphore);
      } else {
        sLog.send("writeNum Semaphore fail");
      }
    }
  } //writeStr()

  void writeCmd(const String& command) {
    sLog.send("writeCmd: " + command);
    if(_xSerialWriteSemaphore != NULL ){
      if(_xSerialWriteSemaphore != NULL ){
        if (xSemaphoreTake(_xSerialWriteSemaphore, 100 / portTICK_PERIOD_MS) ==
            pdTRUE) {
          _serial->print(command);
          _serial->print(_cmdTerminator);
          xSemaphoreGive(_xSerialWriteSemaphore);
        } else {
          sLog.send("writeCmd Semaphore fail");
        }
      }
    }
  } //writeCmd()

  // This gets called in a task or the loop().
  // Pass std::string and maximum ytes to be read
  // Returns actual bytes read and populated string
  // or 'false' if no bytes read
  int listen(std::string& _nexBytes, uint8_t _size) {
    if(_xSerialReadSemaphore != NULL ){
      if (xSemaphoreTake(_xSerialReadSemaphore, 100 / portTICK_PERIOD_MS)) {
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
          xSemaphoreGive(_xSerialReadSemaphore);
          return _nexBytes.length();
        }
        xSemaphoreGive(_xSerialReadSemaphore);
        return false;
      } else {
        sLog.send("listen() read Semaphore fail");
        return false;
      }
    }
    else
      return false;
  }  //listen()
};

#endif  // NEXTIONINTERFACE_H