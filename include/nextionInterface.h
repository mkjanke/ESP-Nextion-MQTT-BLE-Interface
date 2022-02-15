#ifndef NEXTIONINTERFACE_H
#define NEXTIONINTERFACE_H

SemaphoreHandle_t xSerialWriteSemaphore =
    NULL;  // Avoid smashing Nextion with overlapping writes
SemaphoreHandle_t xSerialReadSemaphore =
    NULL;  // Avoid smashing app with overlapping reads

class myNextionInterface {
 private:
  HardwareSerial* _serial;
  const char _cmdTerminator[3] = {'\xFF', '\xFF', '\xFF'};

 public:
  bool respondToBLE = false;
  const std::string eventTopic = "display/event";
  const std::string uptimeTopic = "display/uptime";
  const std::string cmdTopic = "display/command";
  
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
    if (xSemaphoreTake(xSerialWriteSemaphore, 100 / portTICK_PERIOD_MS) ==
        pdTRUE) {
      _serial->print(_componentName);
      _serial->print("=");
      _serial->print(_val);
      _serial->print(_cmdTerminator);
      xSemaphoreGive(xSerialWriteSemaphore);
    } else {
      Serial << "writeNum Semaphore fail" << endl;
    }
  }

  void writeStr(String command, String txt) {
    if (xSemaphoreTake(xSerialWriteSemaphore, 100 / portTICK_PERIOD_MS) ==
        pdTRUE) {
      _serial->print(command);
      _serial->print("=\"");
      _serial->print(txt);
      _serial->print("\"");
      _serial->print(_cmdTerminator);
      xSemaphoreGive(xSerialWriteSemaphore);
    } else {
      Serial << "writeStr Semaphore fail" << endl;
    }
  }

  void writeCmd(String command) {
    if (xSemaphoreTake(xSerialWriteSemaphore, 1000 / portTICK_PERIOD_MS) ==
        pdTRUE) {
      _serial->print(command);
      _serial->print(_cmdTerminator);
      xSemaphoreGive(xSerialWriteSemaphore);
    } else {
      Serial << "writeStr Semaphore fail" << endl;
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
      Serial << "listen() read Semaphore fail" << endl;
      return false;
    }
  }

};

#endif  // NEXTIONINTERFACE_H