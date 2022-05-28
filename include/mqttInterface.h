#ifndef MQTT_INTERFACE_H
#define MQTT_INTERFACE_H

#include "TinyMqtt.h"

namespace mqtt {
  extern MqttClient NexClient;

  extern void begin();
  extern void loop();
}

#endif  //MQTT_INTERFACE_H