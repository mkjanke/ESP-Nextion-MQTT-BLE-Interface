#include <NimBLEDevice.h>

#include "bleInterface.h"
#include "settings.h"

// Bluetooth related defines and classes
BLEServer* pServer;
BLEService* pService;
BLEAdvertising* pAdvertising;

BLECharacteristic* message_BLEC;
BLECharacteristic* event_BLEC;
BLECharacteristic* uptimeBLEC;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { bleInterface::deviceConnected = true; };

  void onDisconnect(BLEServer* pServer) {
    bleInterface::deviceConnected = false;
  }
};

// General purpose callback for handling String characteristic writes
class stringCallback : public BLECharacteristicCallbacks {
 private:
  char* _buffer;
  size_t _size;

 public:
  stringCallback(char* val, size_t sz) {
    _buffer = val;
    _size = sz;
  }

  // Characteristic has been written by BLE client.
  // copy & clear characteristic
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    // clear buffer, then copy bytes
    for (size_t _i = 0; _i < _size; _i++)
    {
      _buffer[_i] = 0;
    }
    value.copy(_buffer, _size);
    bleInterface::messageWaiting = true;
    // Value has been read and copied, clear value from charateristic.
    pCharacteristic->setValue('\00');
  }
};

void bleInterface::updateUptime(char* buff) {
  uptimeBLEC->setValue((std::string)buff);
}

// Write message from Nextion
//    back to BLE characteristic
void bleInterface::writeMessage(char* buff) {
  message_BLEC->setValue((std::string)buff);
}
void bleInterface::writeMessage(uint8_t* buff, uint8_t len) {
  message_BLEC->setValue(buff, len);
}

// Write event from Nextion
//    back to BLE characteristic
void bleInterface::writeEvent(std::string buff) {
  event_BLEC->setValue(buff);
}
void bleInterface::writeEvent(char* buff) {
  event_BLEC->setValue((std::string)buff);
}
void bleInterface::writeEvent(uint8_t* buff, uint8_t len) {
  event_BLEC->setValue(buff, len);
}

void bleInterface::stopAdvertising() {
  pAdvertising->stop();
  pAdvertising->setScanResponse(false);
}

void bleInterface::begin() {
  BLEDevice::init(DEVICE_NAME);         // Create BLE device
  pServer = BLEDevice::createServer();  // Create BLE server
  pServer->setCallbacks(
      new MyServerCallbacks());  // Set the callback function of the server
  pService = pServer->createService(SERVICE_UUID);  // Create BLE service

  message_BLEC = pService->createCharacteristic(
      MESSAGE_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

  event_BLEC = pService->createCharacteristic(
      EVENT_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  uptimeBLEC =
      pService->createCharacteristic(UPTIME_UUID, NIMBLE_PROPERTY::READ);

//  message_BLEC->setCallbacks(new stringCallback(msgBuffer, sizeof(msgBuffer)));
  message_BLEC->setCallbacks(new stringCallback(msgBuffer, sizeof(msgBuffer)));
  
  NimBLEDescriptor* messageBLEDesc;
  NimBLEDescriptor* eventBLEDesc;
  NimBLEDescriptor* uptimeBLEDesc;

  messageBLEDesc =
      message_BLEC->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25);
  messageBLEDesc->setValue("Nextion Message");

  eventBLEDesc =
      event_BLEC->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25);
  eventBLEDesc->setValue("Nextion Event");

  uptimeBLEDesc =
      uptimeBLEC->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25);
  uptimeBLEDesc->setValue("Device Uptime");

  pService->start();
  pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  pAdvertising->setScanResponse(true);
}
