#ifndef SETTINGS_H
#define SETTINGS_H

#define WIFI_SSID "----"            //Change me
#define WIFI_PASSWORD "------"      //Change me
#define DEVICE_NAME "ESP-Display"   //Change me

#define HEARTBEAT 1000L             // Sensor and WiFi loop delay (ms)
#define SYSLOG_HOST "192.168.1.7"
#define SYSLOG_PORT 514

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

// Nextion Serial configuration
#define NEXTION_SERIAL Serial1
#define NEXTION_BAUD 115200
#define RXDN 19
#define TXDN 21

// Nextion Widgets
#define NEXT_UPTIME_WIDGET "page0.upTxt.txt" // Name of Uptime widget on Nextion display
#define NEXT_ESPOUT_WIDGET "page1.ESP.txt"   // Widget to display ESP debug output
#define NEXT_WIFI_CONN_WIDGET "WiFiConnected" //WiFi Icon Bright
#define NEXT_WIFI_DISC_WIDGET "WiFiDisc" //WiFi Icon Dim

// BlueTooth UUID's
#define SERVICE_UUID        "611f9238-3915-11ec-8d3d-0242ac130003"  // 9238
#define MESSAGE_UUID        "6bcbec08-7fbb-11ec-a8a3-0242ac120002"  // 0002 String - Nextion Command
#define EVENT_UUID          "6bcbee60-7fbb-11ec-a8a3-0242ac120002"
#define UPTIME_UUID         "611f96f2-3915-11ec-8d3d-0242ac130003"  // 96f2 String
#define STATUS_UUID         "6bcbf0ea-7fbb-11ec-a8a3-0242ac120002"  // String

#endif //SETTINGS_H