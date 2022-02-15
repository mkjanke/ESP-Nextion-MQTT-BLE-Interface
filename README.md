## Nextion BLE interface

An attempt to create and interface that allows updating a Nextion display via either MQTT or BLE.

Note: the TinyMQTT library currently has a memory leak when MQTT client connections are closed. Use with caution!

The ESP32 code in this repository includes 

* a Bluetooth LE interface that permits updating Nextion objects by writing to a BLE characteristic. 
* an MQTT interface that allows an MQTT client to update Nextion objects by publishing to the MQTT broker on the ESP32

MQTT topics:

* display/event           // topic that recieves filtered Nextion events
* display/event/#        // One topic per Nextion event code in hex (70, 71, etc)
* display/command         // Topic used to send command to teh Nextion (page 0, p0.val=12345, etc.)

Bluetooth Charateristics

* SERVICE_UUID        "611f9238-3915-11ec-8d3d-0242ac130003"  // Bluetooth service
* MESSAGE_UUID        "6bcbec08-7fbb-11ec-a8a3-0242ac120002"  // Nextion Command (write to this characteristic to send command to Nextion)
* EVENT_UUID          "6bcbee60-7fbb-11ec-a8a3-0242ac120002"  // Nextion message
* UPTIME_UUID         "611f96f2-3915-11ec-8d3d-0242ac130003"  // ESP32 Uptime

A Nextion command written as ASCII text to the MESSAGE_UUID will be forwarded to the Nextion as written.

##Flow
###Nextion sends unsolicited message

```
     Nextion
        ↓            
+-----------------+
| Nextion Message | 
| 87 FF FF FF     |
+-----------------+
        ↓
        ↓  Serial
        ↓
+-------------------+ 
|  nextionInterface |
|      listen()     | 
+-------------------+ 
        ↓
        ↓
        ↓
+----------------------+         +----------------+
| task handleNextion() | ------> | bleInterface   |-----> BLE Event Characteristic
|                      |         | writeEvent()   |
+----------------------+         +----------------+
        ↓
        ↓
        ↓                +-------------------------+
        +--------------> | mqttNexClient.publish() |----> MQTT 'Event' Topic
                         |                         |      MQTT 'Event/FF' Topic
                         +-------------------------+

```

###BLE client updates BLE Characteristic

```
+-------------------------+        +------------------------+
| mqttNexClient.publish() | <----- | messageWaiting == true | <--- BLE Message
|   Topic 'Command'       |        | msgBuffer updated      |      Characteristic
+-------------------------+        +------------------------+
        ↓
        ↓ MQTT Message
        ↓
+----------------------------+
|  mqttNexClient.subscribe() |
|   Topic 'Command'          |
|                            |
+----------------------------+
        ↓
        ↓  Serial TX
        ↓
+-----------------+
| Nextion Message |
| V1.val=1234     |
| V1.txt=qwerty   |
| page 0          |
| get V1.val      |
+-----------------+
        ↓
        ↓  Serial RX
        ↓
+--------------------+  serial  +--------------+
| Nextion Message    | -------> |   listen()   | ------> [Treat as unsolicited message]
| 70 01 ... FF FF FF |          |              |
| 71 01 ... FF FF FF |          +--------------+
+--------------------+

```
