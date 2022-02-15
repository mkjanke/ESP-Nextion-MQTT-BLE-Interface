## Nextion BLE interface

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
