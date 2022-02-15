#ifndef SETTINGS_H
#define SETTINGS_H

#define WIFI_SSID "----"            //Change me
#define WIFI_PASSWORD "------"      //Change me

#define HEARTBEAT 1000L             // Sensor and WiFi loop delay (ms)
#define DEVICE_NAME "ESP-Display"   //Cnange me

//MQTT broker
#define MQTT_PORT 1883

// Nextion Serial pins
#define RXD2 16
#define TXD2 17

// BlueTooth UUID's
#define SERVICE_UUID        "611f9238-3915-11ec-8d3d-0242ac130003"  // Bluetooth service
#define MESSAGE_UUID        "6bcbec08-7fbb-11ec-a8a3-0242ac120002"  // Nextion Command (write to this characteristic to send command to Nextion)
#define EVENT_UUID          "6bcbee60-7fbb-11ec-a8a3-0242ac120002"  // Nextion message
#define UPTIME_UUID         "611f96f2-3915-11ec-8d3d-0242ac130003"  // ESP32 Uptime

#endif //SETTINS_H