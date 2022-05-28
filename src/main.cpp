#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "bleInterface.h"
#include "mqttInterface.h"
#include "esp_system.h"
#include "nextionInterface.h"
#include "settings.h"

#include "log.h"

extern void heartBeat(void*);
void checkWiFi(void*);
void handleWiFiEvent(WiFiEvent_t, WiFiEventInfo_t);

void handleNextion(void*);
void outputWebPage(WiFiClient);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

extern char uptimeBuffer[];  // scratch space for storing formatted 'uptime' string

bleInterface bleIF;

WiFiServer server(80);

// May need to use Serial2, depending on ESP32 module and pinouts
myNextionInterface myNex(Serial1);

TaskHandle_t xheartBeatHandle = NULL;  // Task handles
TaskHandle_t xhandleNextionHandle = NULL;
TaskHandle_t xcheckWiFiHandle = NULL;

// WiFi Event handler - catch WiFi events, forward to BLE interface
void handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info){
  String status;
  status.reserve(40);

  switch (event){
    // case SYSTEM_EVENT_STA_GOT_IP:
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      status=(String)"IP:" + WiFi.localIP().toString() + " DNS:" + WiFi.dnsIP().toString();
      bleIF.updateStatus(status.c_str());
      myNex.wifiIcon(true);
      break;
    // case SYSTEM_EVENT_STA_DISCONNECTED:
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      bleIF.updateStatus("WiFi not connected");
      myNex.wifiIcon(false);
      break;
    default:
      break;
  }
}

void setup() {
  // Initialize WiFi
  vTaskDelay(500 / portTICK_PERIOD_MS);
  WiFi.begin(ssid, password);
  WiFi.setHostname(DEVICE_NAME);

  // Initialize OTA Update libraries
  ArduinoOTA.setHostname(DEVICE_NAME);
  ArduinoOTA.onStart([]() { bleIF.stopAdvertising(); });
  ArduinoOTA.begin();

  vTaskDelay(2500 / portTICK_PERIOD_MS);
  sLog.init();

  bleIF.begin();        // Start Bluetooth Interface
  mqtt::begin();        // Initialize MQTT broker
  myNex.begin(115200);  // Initialize Nextion interface

  vTaskDelay(100 / portTICK_PERIOD_MS);
  // Start web server
  server.begin();
  sLog.send("Web Server started.", true);

  // Start background tasks - Wifi, Nextion & heartbeat
  xTaskCreate(checkWiFi, "Check WiFi", 3000, NULL, 1, &xcheckWiFiHandle);
  xTaskCreate(handleNextion, "Nextion Handler", 3000, NULL, 6, &xhandleNextionHandle);
  xTaskCreate(heartBeat, "Heartbeat", 3000, NULL, 1, &xheartBeatHandle);
  
  sLog.send((String)DEVICE_NAME + " is Woke");

  // Timing sensitive - if too early in setup() bootup crashes with spinlock error.
  WiFi.onEvent(handleWiFiEvent);

}

void loop() {
  ArduinoOTA.handle();

  mqtt::loop();

  // Check for incoming message from BLE interface
  // If we see message from BLE, forward to Nextion via MQTT
  if (bleInterface::messageWaiting) {
    bleInterface::messageWaiting = false;
    mqtt::NexClient.publish(myNex.cmdTopic, bleIF.msgBuffer);
  }
  vTaskDelay(100 / portTICK_PERIOD_MS);

  WiFiClient client = server.available();  // Listen for incoming clients
  if (client) {
    sLog.send((String)"Connect from " + client.remoteIP(), true);
    outputWebPage(client);  // If a new client connects,
    client.flush();
    client.stop();
  }
}  // loop()

// Read incoming events (messages) from Nextion
// Forward filtered events to BLEinterface
//
// Check for command from BLE interface
// write command to Nextion Display
void handleNextion(void* parameter) {
  // Events we care about
  const char filter[] = {'\x65', '\x66', '\x67', '\x68',
                         '\x70', '\x71', '\x86', '\x87',
                         '\xAA'};

  std::string
      _bytes;  // Raw bytes returned from Nextion, including FF terminaters
  _bytes.reserve(48);
  std::string _hexString;  //_bytes converted to space delimited ASCII chars in
                           //std::string
                           // I.E. 1A B4 E4 FF FF FF
  char _x[] = {};

  vTaskDelay(100 / portTICK_PERIOD_MS);

  for (;;) {  // ever
    // Check for incoming event from Nextion
    if (_bytes.length() > 0) _bytes.clear();
    if (_hexString.length() > 0) _hexString.clear();

    int _len = myNex.listen(_bytes, 48);
    if (_len) {
      if (_len > 3) {
        _hexString.reserve(_len * 3);
        for (const auto& item : _bytes) {
          sprintf(_x, "%02X ", item);
          _hexString += _x;
        }
        sLog.send((String)"handleNextion returned: " + _hexString.c_str());

        // If we see interesting event from Nextion, forward to BLE interface
        // and to MQTT.
        for (size_t i = 0; i < sizeof filter; i++) {
          if (_bytes[0] == filter[i]) {
            mqtt::NexClient.publish(myNex.eventTopic, _hexString);
            bleIF.writeEvent(_hexString);

            if (_bytes[0] == '\x70') {
              bleIF.writeMessage(_hexString);
            }
            if (_bytes[0] == '\x71') {
              bleIF.writeMessage(_hexString);
            }
            // Forward filtered events to MQTT as subtopics
            std::string t = myNex.eventTopic + "/" + _hexString.substr(0, 2);
            mqtt::NexClient.publish(t, _hexString);
          }
        }
      } else {
        sLog.send("handleNextion: Short read");
        myNex.flushReads();
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);  // Should never reach this.
}  // handleNextion()


// Check Wifi task
// check Wifi connection, attempt reconnect
void checkWiFi(void* parameter) {
  String status;
  status.reserve(40);

  for (;;) {  // infinite loop
    vTaskDelay(HEARTBEAT * 30 / portTICK_PERIOD_MS);
    if (WiFi.status() == WL_CONNECTED) {
      status=(String)"IP:" + WiFi.localIP().toString() + " DNS:" + WiFi.dnsIP().toString();
      sLog.send(status.c_str(), true);
      myNex.wifiIcon(true); // Show Wifi Icon
    } else {
      bleIF.updateStatus("WiFi not connected");
      myNex.wifiIcon(false); // Dim Wifi Icon
      vTaskDelay(HEARTBEAT * 30 / portTICK_PERIOD_MS);
      WiFi.reconnect();
    }
  }
  vTaskDelete(NULL);  // Should never reach this.
}  // checkWiFI()

void outputWebPage(WiFiClient client) {
  if (client) {
    client << "HTTP/1.1 200 OK" << endl;
    client << "Content-Type: text/html" << endl;
    client << endl;  //  Important.
    client << "<!DOCTYPE HTML> <html> <head><meta charset=utf-8></head>"
           << "<body><font face='Arial'> <h1>" << DEVICE_NAME << "</h1>"
           << "<br>" << uptimeBuffer << endl;
    client << "<br>Task HW: N: "
           << uxTaskGetStackHighWaterMark(xhandleNextionHandle)
           << "b H: " << uxTaskGetStackHighWaterMark(xheartBeatHandle)
           << "b W: " << uxTaskGetStackHighWaterMark(xcheckWiFiHandle)
           << "b <br>" << endl;
    client << "Min Free Heap: " << esp_get_minimum_free_heap_size() << endl;
    client << "<br></font></center></body></html>" << endl;
  }
}  // outputWebPage()
