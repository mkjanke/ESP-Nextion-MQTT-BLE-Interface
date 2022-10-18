#include <NimBLEDevice.h>

#include "bleInterface.h"
#include "settings.h"
#include "log.h"

bool bleInterface::deviceConnected = false;
bool bleInterface::messageWaiting = false;

// Bluetooth related defines and classes
BLEServer* pServer;
BLEService* pService;
BLEAdvertising* pAdvertising;

BLECharacteristic* message_BLEC;
BLECharacteristic* event_BLEC;
BLECharacteristic* uptimeBLEC;
BLECharacteristic* statusBLEC;

class MyServerCallbacks : public BLEServerCallbacks {
  char pBuffer[64];

  void onConnect(BLEServer* pServer, ble_gap_conn_desc* desc) { 
    bleInterface::deviceConnected = true; 
    snprintf(pBuffer, sizeof(pBuffer), "BLE Conn %s L: %d, St: %d, Itvl: %d", 
              NimBLEAddress(desc->peer_ota_addr).toString().c_str(), 
              desc->conn_latency, desc->supervision_timeout, desc->conn_itvl);
    sLog.send(pBuffer);
    // pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);

    pAdvertising->start();
    pAdvertising->setScanResponse(true);
  };

  void onDisconnect(BLEServer* pServer, ble_gap_conn_desc* desc) {
    // Not sure if this is necessary or not
    NimBLEDevice::whiteListAdd(NimBLEAddress(desc->peer_ota_addr));

    bleInterface::deviceConnected = false;
    snprintf(pBuffer, sizeof(pBuffer), "BLE Disconnect %s", 
              NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    sLog.send(pBuffer);
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
    for (size_t _i = 0; _i < _size; _i++) {
      _buffer[_i] = 0;
    }
    value.copy(_buffer, _size);
    bleInterface::messageWaiting = true;
    // Value has been read and copied, clear value from charateristic.
    pCharacteristic->setValue('\00');
  }
};

// Uptime
void bleInterface::updateUptime(const char* buff) {
  uptimeBLEC->setValue((std::string)buff);
  uptimeBLEC->notify();
}

// Device Status
void bleInterface::updateStatus(const char* buff) {
  statusBLEC->setValue((std::string)buff);
  statusBLEC->notify();
}

// Write message from Nextion
//    back to BLE characteristic
void bleInterface::writeMessage(std::string & buff) {
  message_BLEC->setValue(buff);
  message_BLEC->notify();
}
void bleInterface::writeMessage(const char* buff) {
  message_BLEC->setValue((std::string)buff);
  message_BLEC->notify();
}
void bleInterface::writeMessage(const uint8_t* buff, uint8_t len) {
  message_BLEC->setValue(buff, len);
  message_BLEC->notify();
}

// Write event from Nextion
//    back to BLE characteristic
void bleInterface::writeEvent(std::string & buff) {
  event_BLEC->setValue(buff);
  event_BLEC->notify();
}
void bleInterface::writeEvent(const char* buff) {
  event_BLEC->setValue((std::string)buff);
  event_BLEC->notify();
}
void bleInterface::writeEvent(const uint8_t* buff, uint8_t len) {
  event_BLEC->setValue(buff, len);
  event_BLEC->notify();
}

void bleInterface::stopAdvertising() {
  pAdvertising->stop();
  pAdvertising->setScanResponse(false);
}
void bleInterface::startAdvertising() {
  pAdvertising->start();
  pAdvertising->setScanResponse(true);
}

void bleInterface::begin() {

  sLog.send("Starting BlueTooth");

  BLEDevice::init(DEVICE_NAME);         // Create BLE device
  pServer = BLEDevice::createServer();  // Create BLE server
  pServer->setCallbacks(
      new MyServerCallbacks());  // Set the callback function of the server

  pServer->advertiseOnDisconnect(true);

  pService = pServer->createService(SERVICE_UUID);  // Create BLE service

  // Characteristic for sending commands to Nextion
  message_BLEC = pService->createCharacteristic(
      MESSAGE_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  message_BLEC->setCallbacks(new stringCallback(msgBuffer, sizeof(msgBuffer)));
  NimBLEDescriptor* messageBLEDesc;
  messageBLEDesc =
      message_BLEC->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25);
  messageBLEDesc->setValue("Nextion Message");

  // Characteristic to hold events from Nextion device.
  event_BLEC = pService->createCharacteristic(
      EVENT_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  NimBLEDescriptor* eventBLEDesc;
  eventBLEDesc =
      event_BLEC->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25);
  eventBLEDesc->setValue("Nextion Event");

  // Characteristic to hold ESP32 'uptime' DD.HH.MM
  uptimeBLEC = pService->createCharacteristic(
      UPTIME_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  NimBLEDescriptor* uptimeBLEDesc;
  uptimeBLEDesc =
      uptimeBLEC->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25);
  uptimeBLEDesc->setValue("Device Uptime");

  // Characteristic to hold misc ESP32 Status messages
  statusBLEC = pService->createCharacteristic(
      STATUS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  NimBLEDescriptor* statusBLEDesc;
  statusBLEDesc =
      statusBLEC->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25);
  statusBLEDesc->setValue("Device Status");

  pService->start();
  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); 

  startAdvertising();

}
