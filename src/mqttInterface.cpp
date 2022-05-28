#include "mqttInterface.h"
#include "settings.h"
#include "log.h"
#include "nextionInterface.h"

extern myNextionInterface myNex;
extern Logger sLog;

namespace mqtt {

  MqttBroker Broker(MQTT_PORT);
  MqttClient NexClient(&Broker);

  // MQTT client callback
  void onPublishEvent(const MqttClient*, 
                      const Topic& topic,
                      const char* payload, size_t) {
    sLog.send(((std::string)"--> mqttNexClient received: " + topic.c_str() + " : " + payload).c_str());

    if (topic.matches(myNex.cmdTopic)) {
      myNex.writeCmd(payload);
    }
  }

  // Initialize MQTT broker and client
  void begin(){
    vTaskDelay(100 / portTICK_PERIOD_MS);
    sLog.send("Starting MQTT Broker", true);
    Broker.begin();

    NexClient.setCallback(mqtt::onPublishEvent);
    NexClient.subscribe(myNex.eventTopic);
    NexClient.subscribe(myNex.cmdTopic);
    NexClient.subscribe(myNex.uptimeTopic);
  }

  // Call loop() function for both client and broker
  void loop(){
    Broker.loop();
    NexClient.loop();
  }
  
}
