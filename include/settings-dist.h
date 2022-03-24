#ifndef SETTINGS_H
#define SETTINGS_H

#define WIFI_SSID "----"            //Change me
#define WIFI_PASSWORD "------"      //Change me
#define DEVICE_NAME "ESP-Display"   //Change me

#define HEARTBEAT 1000L             // Sensor and WiFi loop delay (ms)

//MQTT broker
#define MQTT_PORT 1883

// MQTT topics
#define NEXT_COMMAND "display/command"         // Commands forwarded to Nextion
#define NEXT_UPTIME  "display/uptime"          // ESP32 Uptime - sent to Nextion
#define NEXT_EVENT "display/event"             // Events as read from Nextion
#define NEXT_ESP_FREEHEAP "display/ESP/fHeap"  // ESP Min Free Heap Output Topic
#define NEXT_ESP_NSTACK "display/ESP/nStack"   // Nextion Task Stack High Water Output Topic
#define NEXT_ESP_HSTACK "display/ESP/hStack"   // Heartbeat Task Stack High Water Output Topic
#define NEXT_ESP_WSTACK "display/ESP/wStack"   // Web Server Task Stack High Water Output Topic
#define NEXT_HEARTBEAT_VAR "heartbeat"         // Nextion Heartbeat global variable

// Nextion Serial pins
#define RXD2 16
#define TXD2 17

// Nextion Widgets
#define NEXT_UPTIME_WIDGET "page0.upTxt.txt" // Name of Uptime widget on Nextion display
#define NEXT_ESPOUT_WIDGET "page1.ESP.txt"   // Widget to display ESP debug output

// BlueTooth UUID's
#define SERVICE_UUID        "611f9238-3915-11ec-8d3d-0242ac130003"  // Bluetooth service
#define MESSAGE_UUID        "6bcbec08-7fbb-11ec-a8a3-0242ac120002"  // Nextion Command (write to this characteristic to send command to Nextion)
#define EVENT_UUID          "6bcbee60-7fbb-11ec-a8a3-0242ac120002"  // Nextion message
#define UPTIME_UUID         "611f96f2-3915-11ec-8d3d-0242ac130003"  // ESP32 Uptime

#endif //SETTINGS_H