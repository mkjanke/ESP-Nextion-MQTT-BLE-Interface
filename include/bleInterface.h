#ifndef BLEINTERFACE_H
#define BLEINTERFACE_H
#include <stdint.h>  // typedef uint8_t

class bleInterface {
 public:
  static bool deviceConnected;
  static bool messageWaiting;
  char msgBuffer[32];  // Return Nextion command from BLE interface

  void begin();
  void updateUptime(char *);

  void writeMessage(std::string);
  void writeMessage(char *);
  void writeMessage(uint8_t *, uint8_t);
  void writeEvent(std::string);
  void writeEvent(char *);
  void writeEvent(uint8_t *, uint8_t);

  void stopAdvertising();

  bleInterface(){};
};

#endif  // BLEINTERFACE_H
