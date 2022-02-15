#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "TinyMqtt.h"
#include "bleInterface.h"
#include "esp_system.h"
#include "nextionInterface.h"
#include "settings.h"

void uptime();

void heartBeat(void*);
void checkWiFi(void*);
void handleNextion(void*);
void outputWebPage(WiFiClient);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

char uptimeBuffer[20];  // scratch space for storing formatted 'uptime' string

bleInterface bleIF;
bool bleInterface::deviceConnected = false;
bool bleInterface::messageWaiting = false;

WiFiServer server(80);

myNextionInterface myNex(Serial2);

TaskHandle_t xheartBeatHandle = NULL;  // Task handles
TaskHandle_t xhandleNextionHandle = NULL;
TaskHandle_t xcheckWiFiHandle = NULL;

MqttBroker mqttBroker(MQTT_PORT);
MqttClient mqttNexClient(&mqttBroker);

// MQTT client callback
void onPublishEvent(const MqttClient* /* srce */, const Topic& topic,
                const char* payload, size_t /* length */) {
  Serial << "--> mqttNexClient received: " << topic.c_str() << ": "
         << payload << endl;
  if (topic.matches(myNex.cmdTopic)){
    Serial << "Matched " << myNex.cmdTopic << ": " << payload << endl;
    myNex.writeCmd(payload);
  }
}

void setup() {
  Serial.begin(115200);
  Serial << DEVICE_NAME << " Is Now Woke!" << endl;

  // Initialize Nextion interface
  vTaskDelay(100 / portTICK_PERIOD_MS);
  Serial << "Starting myNex" << endl;
  myNex.begin(115200);

  vTaskDelay(100 / portTICK_PERIOD_MS);
  Serial << "Starting BlueTooth" << endl;
  bleIF.begin();
  bleIF.updateUptime(uptimeBuffer);

  // Initialize WiFi
  vTaskDelay(500 / portTICK_PERIOD_MS);
  Serial << "Starting WiFi" << endl;
  WiFi.begin(ssid, password);
  WiFi.setHostname(DEVICE_NAME);

  // Initialize MQTT broker
  vTaskDelay(500 / portTICK_PERIOD_MS);
  mqttBroker.begin();
  Serial << "Broker ready : " << WiFi.localIP() << " on port " << MQTT_PORT << endl;

  mqttNexClient.setCallback(onPublishEvent);
  mqttNexClient.subscribe(myNex.eventTopic);
  mqttNexClient.subscribe(myNex.cmdTopic);
  mqttNexClient.subscribe(myNex.uptimeTopic);

  // Initialize OTA Update libraries
  ArduinoOTA.setHostname(DEVICE_NAME);
  ArduinoOTA.onStart([]() { bleIF.stopAdvertising(); });
  ArduinoOTA.begin();

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  // Start web server
  server.begin();
  Serial << "Web Server started." << endl;

  // Start background tasks - Wifi, Nextion & heartbeat
  xTaskCreate(checkWiFi, "Check WiFi", 2000, NULL, 1, &xcheckWiFiHandle);
  xTaskCreate(handleNextion, "Nextion Handler", 3000, NULL, 6,
              &xhandleNextionHandle);
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
    //    digitalWrite(LED_OUT,HIGH);
    Serial << "Connect from " << client.remoteIP() << endl;
    outputWebPage(client);  // If a new client connects,
    client.flush();
    client.stop();
    //    digitalWrite(LED_OUT,LOW);
  }

  delay(100);
  if (loopct > 100) {
    loopct = 0;
    // mqttBroker.dump();

    Serial << "Task HW: N: "
           << uxTaskGetStackHighWaterMark(xhandleNextionHandle)
           << "b H: " << uxTaskGetStackHighWaterMark(xheartBeatHandle)
           << "b W: " << uxTaskGetStackHighWaterMark(xcheckWiFiHandle) 
           << "b" << endl;
    Serial << "Min Free Heap: " << esp_get_minimum_free_heap_size() << endl;
  }
}  // loop()

// Read incoming events (messages) from Nextion
// Forward filtered events to BLEinterface
//
// Check for command from BLE interface
// write command to Nextion Display

void handleNextion(void* parameter) {
  // Events we care about
  const std::string filter = "\x65\x66\x67\x68\x70\x71\x86\x87";

  std::string _bytes;       //Raw bytes returned from Nextion, including FF terminaters
   _bytes.reserve(48);
  std::string _hexString;   //_bytes converted to space delimited ASCII chars in std::string
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

        Serial << "handleNextion returned: " << _hexString.c_str() << endl;

        // If we see interesting event from Nextion, forward to BLE interface and to MQTT.
        mqttNexClient.publish(myNex.eventTopic, _hexString);

        std::size_t _found = _bytes.find_first_of(filter);
        if (_found != std::string::npos) {
          bleIF.writeEvent(_hexString);
          std::string t = myNex.eventTopic + "/" + _hexString.substr(0,2);
          mqttNexClient.publish(t, _hexString);
        }

      } else {
        Serial << "Short read" << endl;
        myNex.flushReads();
      }
    }
    // Check for incoming message from BLE interface
    // If we see message from BLE, forward to Nextion via MQTT
    if (bleInterface::messageWaiting) {
//      Serial.println(bleIF.msgBuffer);
      Serial << "bleIF says: " << bleIF.msgBuffer << endl;
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
//
void heartBeat(void* parameter) {
  uint8_t loopct = 0;

  for (;;) {  // ever
    loopct++;
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    uptime();
    mqttNexClient.publish(myNex.uptimeTopic, uptimeBuffer);
    myNex.writeStr("page0.upTxt.txt", uptimeBuffer);
    myNex.writeNum("heartbeat", 1);

    bleIF.updateUptime(uptimeBuffer);

    if (loopct > 100) {
      loopct = 0;
    }
  }
  vTaskDelete(NULL);  // Should never reach this.
} //heartBeat()

// Check Wifi task
// check Wifi connection, attempt reconnect
//
void checkWiFi(void* parameter) {
  for (;;) {  // infinite loop
    vTaskDelay(HEARTBEAT * 30 / portTICK_PERIOD_MS);
    if (WiFi.status() == WL_CONNECTED) {
      Serial << "My IP: " << WiFi.localIP() << endl;
    } else {
      vTaskDelay(HEARTBEAT * 60 / portTICK_PERIOD_MS);
      Serial << "No WiFi Found...reconnecting" << endl;
      WiFi.reconnect();  // Try to reconnect to the server
    }
  }
  vTaskDelete(NULL);  // Should never reach this.
} //checkWiFI()

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