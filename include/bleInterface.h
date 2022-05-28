#ifndef BLEINTERFACE_H
#define BLEINTERFACE_H
#include <stdint.h>  // typedef uint8_t
#include <string>

class bleInterface {
 public:
  static bool deviceConnected;
  static bool messageWaiting;
  char msgBuffer[64];  // Return Nextion command from BLE interface

  void begin();
  void updateUptime(const char *);
  void updateStatus(const char *);

  void writeMessage(std::string &);
  void writeMessage(const char *);
  void writeMessage(const uint8_t *, uint8_t);
  void writeEvent(std::string &);
  void writeEvent(const char *);
  void writeEvent(const uint8_t *, uint8_t);

  void stopAdvertising();
  void startAdvertising();

  bleInterface(){
  };
};

#endif  // BLEINTERFACE_H
