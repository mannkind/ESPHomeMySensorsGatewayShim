// CHANGE THESE TO SUIT YOUR GATEWAY
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN D0
#define MY_RF24_CS_PIN D4
#define MY_GATEWAY_SERIAL
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mysensors_rx"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mysensors_tx"

#include <MySensors.h>

// Function prototypes
void MySensorsCustomReceive(const MyMessage &message) __attribute__((weak));
bool protocolMQTTParse(MyMessage &message, const char *topic, const char *payload, unsigned int length);

const char *TAG = "mysensors";
class MySensorsGatewayShim : public Component, public CustomMQTTDevice
{
  public:
    void setup() override
    {
        this->subscribe(this->MySensorsBootloaderRegisterTopic(), &MySensorsGatewayShim::MySensorsBootloaderRegisterCallback);
        this->subscribe(this->MySensorsBootloaderIDResponseTopic(), &MySensorsGatewayShim::MySensorsBootloaderIDResponseCallback);
    }

    void MySensorsBootloaderRegisterCallback(const std::string &topic, const std::string &payload) 
    {
        ESP_LOGD(TAG, "Received Register - Topic: %s", topic.c_str());
        MyMessage msg;
        protocolMQTT2MyMessage(msg, const_cast<char*>(topic.c_str()), reinterpret_cast<uint8_t*>(const_cast<char*>(payload.c_str())),  payload.length());
        auto nodeId = msg.getDestination();
        ESP_LOGD(TAG, "Received Register - NodeId: %d", nodeId);

        auto found = false;
        for (auto i = 0; i < this->nodes.size(); i++) {
            const auto knownId = this->nodes[i];
            if (knownId == nodeId)
            {
                // Don't re-subscribe to the configuration/firmware topics
                return;
            }
        }

        ESP_LOGD(TAG, "New Register - Payload: %d", nodeId);
        this->nodes.push_back(nodeId);
        
        for (uint8_t typeId = 1; typeId < 4; typeId += 2)
        {
            this->subscribe(this->MySensorsConfigurationFirmwareResponseTopic(nodeId, typeId), &MySensorsGatewayShim::MySensorsBootloaderCFResponseCallback);
        }
    }

    /**
     * ID responses
     */
    void MySensorsBootloaderIDResponseCallback(const std::string &topic, const std::string &payload)
    {
        ESP_LOGD(TAG, "Received ID Response - Topic: %s, Payload: %s", topic.c_str(), payload.c_str());

        MyMessage msg;
        protocolMQTT2MyMessage(msg, const_cast<char*>(topic.c_str()), reinterpret_cast<uint8_t*>(const_cast<char*>(payload.c_str())),  payload.length());
        transportRouteMessage(msg);
    }

    /**
     * Configuration/firmware responses
     */
    void MySensorsBootloaderCFResponseCallback(const std::string &topic, const std::string &payload)
    {
        ESP_LOGD(TAG, "Received CF Response - Topic: %s, Payload: %s", topic.c_str(), payload.c_str());

        MyMessage msg;
        protocolMQTT2MyMessage(msg, const_cast<char*>(topic.c_str()), reinterpret_cast<uint8_t*>(const_cast<char*>(payload.c_str())),  payload.length());
        transportRouteMessage(msg);
    }

    /**
     * Generate a topic based on various parameters
     */
    const std::string MySensorsBootloaderTopic(uint8_t nodeId, uint8_t sensorId, uint8_t commandId, uint8_t typeId) const
    {
        char buf[sizeof(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX) + 20];
        sprintf(buf, "%s/%d/%d/%d/0/%d", MY_MQTT_SUBSCRIBE_TOPIC_PREFIX, nodeId, sensorId, commandId, typeId);
        return std::string(buf);
    }

    /**
     * Generate the topic for id responses
     */
    const std::string MySensorsBootloaderIDResponseTopic() const
    {
        return this->MySensorsBootloaderTopic(255, 255, 3, 4);
    }

    /**
     * Generate the topic for configuration/firmware responses
     */
    const std::string MySensorsConfigurationFirmwareResponseTopic(uint8_t nodeId, uint8_t typeId) const
    {
        return this->MySensorsBootloaderTopic(nodeId, 255, 4, typeId);
    }

    /**
     * Generate the topic for registering a node
     */
    const std::string MySensorsBootloaderRegisterTopic() const
    {
        char buf[sizeof(MY_MQTT_PUBLISH_TOPIC_PREFIX) + 20];
        sprintf(buf, "%s/+/255/4/0/0", MY_MQTT_PUBLISH_TOPIC_PREFIX);
        return std::string(buf);
    }

  protected:
    std::vector<uint8_t> nodes;
};

/* 
 * Allows the gateway to send MySensors compatible MQTT topics/payloads
 * And custom topics/payloads for esphomelib compatibility
 */
void receive(const MyMessage &_message)
{
    MyMessage message = _message;
    ESP_LOGD(TAG, "RF24Packet - Sender: %u, Sensor: %u, Command: %u, Type: %u; Payload: %s", message.sender, message.sensor, mGetCommand(message), message.type, message.getString(_convBuffer));

    // Default MySensors node handling
    char _buf[MAX_PAYLOAD * 2 + 1];
    const auto topic = protocolMyMessage2MQTT(MY_MQTT_PUBLISH_TOPIC_PREFIX, message);
    const auto msg = message.getString(_buf);
    const auto retain = mGetCommand(message) == C_SET || 
        (mGetCommand(message) == C_INTERNAL && message.type == I_BATTERY_LEVEL);

    mqtt::global_mqtt_client->publish(topic, msg, strlen(msg), 0, retain);
}