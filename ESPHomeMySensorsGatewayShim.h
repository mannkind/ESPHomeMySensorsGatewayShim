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
        this->subscribe(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "/#", &MySensorsGatewayShim::Publish2MySensorsNetwork);
    }

    /**
     * Publish incoming messages on MY_MQTT_SUBSCRIBE_TOPIC_PREFIX to the MySensors network
     */
    void Publish2MySensorsNetwork(const std::string &topic, const std::string &payload)
    {
        ESP_LOGD(TAG, "Incoming to send to MySensors Network - Topic: %s, Payload: %s", topic.c_str(), payload.c_str());

        MyMessage msg;
        protocolMQTT2MyMessage(msg, const_cast<char*>(topic.c_str()), reinterpret_cast<uint8_t*>(const_cast<char*>(payload.c_str())),  payload.length());
        transportRouteMessage(msg);
    }
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

    ESP_LOGD(TAG, "Incoming to send to MQTT Network - Topic: %s, Payload: %s", topic, message.getString(_convBuffer));
    mqtt::global_mqtt_client->publish(topic, msg, strlen(msg), 0, retain);
}