#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "TinyMqtt.h"
#include "bleInterface.h"
#include "esp_system.h"
#include "nextionInterface.h"
#include "settings.h"

#include "log.h"

void uptime();

void heartBeat(void*);
void checkWiFi(void*);
void wifiIcon(bool);
void handleWiFiEvent(WiFiEvent_t, WiFiEventInfo_t);

void handleNextion(void*);
void outputWebPage(WiFiClient);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

char uptimeBuffer[20];  // scratch space for storing formatted 'uptime' string

bleInterface bleIF;
bool bleInterface::deviceConnected = false;
bool bleInterface::messageWaiting = false;

WiFiServer server(80);

myNextionInterface myNex(Serial);

TaskHandle_t xheartBeatHandle = NULL;  // Task handles
TaskHandle_t xhandleNextionHandle = NULL;
TaskHandle_t xcheckWiFiHandle = NULL;

MqttBroker mqttBroker(MQTT_PORT);
MqttClient mqttNexClient(&mqttBroker);

// MQTT client callback
void onPublishEvent(const MqttClient*, 
                    const Topic& topic,
                    const char* payload, size_t) {
  sLog.send((String)"--> mqttNexClient received: " + topic.c_str() + " : " + payload);

  if (topic.matches(myNex.cmdTopic)) {
    myNex.writeCmd(payload);
  }
}

// Enable/disable Wifi Icon on Nextion Display
void wifiIcon(bool on) {
  const String wifiConnected = NEXT_WIFI_CONN_WIDGET;
  const String wifiDisconnected = NEXT_WIFI_DISC_WIDGET;
  if (on){
    myNex.writeCmd("vis " + wifiConnected + ",1");
    myNex.writeCmd("vis " + wifiDisconnected + ",0");
  }
  else {
    myNex.writeCmd("vis " + wifiDisconnected + ",1");
    myNex.writeCmd("vis " + wifiConnected + ",0");
  }
}

// WiFi Event handler - catch WiFi events, forward to BLE interface
void handleWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info){
  String status;
  status.reserve(40);

  switch (event){
    case SYSTEM_EVENT_STA_GOT_IP:
      status=(String)"IP:" + WiFi.localIP().toString() + " DNS:" + WiFi.dnsIP().toString();
      bleIF.updateStatus(status.c_str());
      wifiIcon(true);
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      bleIF.updateStatus("WiFi not connected");
      wifiIcon(false);
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
  WiFi.onEvent(handleWiFiEvent);

  // Initialize OTA Update libraries
  ArduinoOTA.setHostname(DEVICE_NAME);
  ArduinoOTA.onStart([]() { bleIF.stopAdvertising(); });
  ArduinoOTA.begin();

  vTaskDelay(2500 / portTICK_PERIOD_MS);
  sLog.init();
  sLog.send((String)DEVICE_NAME + " is Woke");

  sLog.send("Starting BlueTooth");
  bleIF.begin();
  bleIF.updateUptime(uptimeBuffer);

  // Initialize MQTT broker
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sLog.send("Starting MQTT Broker", true);
  mqttBroker.begin();

  // Initialize Nextion interface
  vTaskDelay(100 / portTICK_PERIOD_MS);
  sLog.send("Starting myNex", true);
  myNex.begin(115200);

  mqttNexClient.setCallback(onPublishEvent);
  mqttNexClient.subscribe(myNex.eventTopic);
  mqttNexClient.subscribe(myNex.cmdTopic);
  mqttNexClient.subscribe(myNex.uptimeTopic);

  vTaskDelay(100 / portTICK_PERIOD_MS);
  // Start web server
  server.begin();
  sLog.send("Web Server started.", true);

  // Start background tasks - Wifi, Nextion & heartbeat
  xTaskCreate(checkWiFi, "Check WiFi", 3000, NULL, 1, &xcheckWiFiHandle);
  xTaskCreate(handleNextion, "Nextion Handler", 3000, NULL, 6, &xhandleNextionHandle);
  xTaskCreate(heartBeat, "Heartbeat", 3000, NULL, 1, &xheartBeatHandle);
}

uint8_t loopct = 0;

void loop() {
  ArduinoOTA.handle();

  mqttBroker.loop();
  mqttNexClient.loop();

  loopct++;

  WiFiClient client = server.available();  // Listen for incoming clients
  if (client) {
    sLog.send((String)"Connect from " + client.remoteIP(), true);
    outputWebPage(client);  // If a new client connects,
    client.flush();
    client.stop();
  }

  sLog.loop();

  delay(100);
  if (loopct > 100) {
    loopct = 0;
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
            mqttNexClient.publish(myNex.eventTopic, _hexString);
            bleIF.writeEvent(_hexString);

            if (_bytes[0] == '\x70') {
              bleIF.writeMessage(_hexString);
            }
            if (_bytes[0] == '\x71') {
              bleIF.writeMessage(_hexString);
            }
            // Forward filtered events to MQTT as subtopics
            std::string t = myNex.eventTopic + "/" + _hexString.substr(0, 2);
            mqttNexClient.publish(t, _hexString);
          }
        }
      } else {
        sLog.send("handleNextion: Short read");
        myNex.flushReads();
      }
    }
    // Check for incoming message from BLE interface
    // If we see message from BLE, forward to Nextion via MQTT
    if (bleInterface::messageWaiting) {
      bleInterface::messageWaiting = false;
      mqttNexClient.publish(myNex.cmdTopic, bleIF.msgBuffer);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);  // Should never reach this.
}  // handleNextion()

// Calculate uptime & populate uptime buffer for future use
void uptime() {
  // Constants for uptime calculations
  static const uint32_t millis_in_day = 1000 * 60 * 60 * 24;
  static const uint32_t millis_in_hour = 1000 * 60 * 60;
  static const uint32_t millis_in_minute = 1000 * 60;

  uint8_t days = millis() / (millis_in_day);
  uint8_t hours = (millis() - (days * millis_in_day)) / millis_in_hour;
  uint8_t minutes =
      (millis() - (days * millis_in_day) - (hours * millis_in_hour)) /
      millis_in_minute;
  snprintf(uptimeBuffer, sizeof(uptimeBuffer), "Uptime: %2dd%2dh%2dm", days,
           hours, minutes);
}

// Heartbeat task - update uptime buffer
//                - update heartbeat field on Nextion
void heartBeat(void* parameter) {
  uint8_t loopct = 0;

  for (;;) {  // ever
    loopct++;
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    uptime();
    sLog.send(uptimeBuffer);

    mqttNexClient.publish(myNex.uptimeTopic, uptimeBuffer);

    myNex.writeStr(NEXT_UPTIME_WIDGET, uptimeBuffer);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    myNex.writeNum("heartbeat", 1);

    char buffer[37];
    snprintf(buffer, sizeof(buffer), "N:%ib\\rH:%ib\\rW:%ib\\rH:%i",
             uxTaskGetStackHighWaterMark(xhandleNextionHandle),
             uxTaskGetStackHighWaterMark(xheartBeatHandle),
             uxTaskGetStackHighWaterMark(xcheckWiFiHandle),
             esp_get_minimum_free_heap_size());

    myNex.writeStr(NEXT_ESPOUT_WIDGET, buffer);

    snprintf(buffer, sizeof(buffer), "%i", esp_get_minimum_free_heap_size());
    mqttNexClient.publish(NEXT_ESP_FREEHEAP, buffer);

    snprintf(buffer, sizeof(buffer), "%i",
             uxTaskGetStackHighWaterMark(xhandleNextionHandle));
    mqttNexClient.publish(NEXT_ESP_NSTACK, buffer);

    snprintf(buffer, sizeof(buffer), "%i",
             uxTaskGetStackHighWaterMark(xheartBeatHandle));
    mqttNexClient.publish(NEXT_ESP_HSTACK, buffer);

    snprintf(buffer, sizeof(buffer), "%i",
             uxTaskGetStackHighWaterMark(xcheckWiFiHandle));
    mqttNexClient.publish(NEXT_ESP_WSTACK, buffer);

    bleIF.updateUptime(uptimeBuffer);

    if (loopct > 100) {
      loopct = 0;
    }
  }
  vTaskDelete(NULL);  // Should never reach this.
}  // heartBeat()

// Check Wifi task
// check Wifi connection, attempt reconnect
void checkWiFi(void* parameter) {
  String status;
  status.reserve(40);

  for (;;) {  // infinite loop
    vTaskDelay(HEARTBEAT * 30 / portTICK_PERIOD_MS);
    if (WiFi.status() == WL_CONNECTED) {
      status=(String)"IP:" + WiFi.localIP().toString() + " DNS:" + WiFi.dnsIP().toString();
      sLog.send(status, true);
      wifiIcon(true); // Show Wifi Icon
    } else {
      bleIF.updateStatus("WiFi not connected");
      wifiIcon(false); // Dim Wifi Icon
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
