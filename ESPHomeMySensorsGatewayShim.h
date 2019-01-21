#include <MySensors.h>

// Function prototypes
void MySensorsCustomReceive(const MyMessage &message) __attribute__((weak));
bool protocolMQTTParse(MyMessage &message, const char *topic, const char *payload, unsigned int length);

const char *TAG = "mysensors";
class MySensorsGatewayShim
{
  public:
    void setup()
    {
        this->MySensorsBootloaderRegister();
        this->MySensorsBootloaderIDResponse(); 
    }

    /**
     * Subscribe to node registration topic
     */
    void MySensorsBootloaderRegister()
    {
        const auto topic = this->MySensorsBootloaderRegisterTopic();
        const auto callback = [this](const std::string &topic, const std::string &payload) {
            ESP_LOGD(TAG, "Received Register - Payload: %s", payload.c_str());
            const auto nodeId = atoi(payload.c_str());

            auto found = false;
            for (auto i = 0; i < this->nodes.size(); i++) {
                const auto knownId = this->nodes[i];
                if (knownId == nodeId)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                ESP_LOGD(TAG, "New Register - Payload: %d", nodeId);
                this->nodes.push_back(nodeId);
                this->MySensorsBootloaderCFResponse(nodeId);
            }
        };

        mqtt::global_mqtt_client->subscribe(topic, callback);
    }

    /**
     * Subscribe to id responses
     */
    void MySensorsBootloaderIDResponse()
    {
        const auto topic = this->MySensorsBootloaderIDResponseTopic();
        const auto callback = [this](const std::string &topic, const std::string &payload) {
            //const auto topic = this->MySensorsBootloaderIDResponseTopic();
            const auto size = payload.length();

            ESP_LOGD(TAG, "Received ID Response - Topic: %s, Payload: %s", topic.c_str(), payload.c_str());

            this->msg.clear();
            protocolMQTTParse(msg, topic.c_str(), payload.c_str(), size);
            transportRouteMessage(msg);
        };

        mqtt::global_mqtt_client->subscribe(topic, callback);
    }

    /**
     * Subscribe to configuration/firmware responses
     */
    void MySensorsBootloaderCFResponse(const uint8_t nodeId)
    {
        for (uint8_t typeId = 1; typeId < 4; typeId += 2)
        {
            const auto topic = this->MySensorsConfigurationFirmwareResponseTopic(nodeId, typeId);
            const auto callback = [this, nodeId, typeId](const std::string &topic, const std::string &payload) {
                //const auto topic = this->MySensorsConfigurationFirmwareResponseTopic(nodeId, typeId);
                const auto size = payload.size();

                ESP_LOGD(TAG, "Received CF Response - NodeID: %u; Topic: %s, Payload: %s", nodeId, topic.c_str(), payload.c_str());

                this->msg.clear();
                protocolMQTTParse(msg, topic.c_str(), payload.c_str(), size);
                transportRouteMessage(msg);
            };

            mqtt::global_mqtt_client->subscribe(topic, callback);
        }
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
        char buf[sizeof(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX) + 10];
        sprintf(buf, "%s/%s", MY_MQTT_SUBSCRIBE_TOPIC_PREFIX, "register");
        return std::string(buf);
    }

  protected:
    MyMessage msg;
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
    const auto topic = protocolFormatMQTTTopic(MY_MQTT_PUBLISH_TOPIC_PREFIX, message);
    const auto msg = message.getString(_buf);
    const auto retain = mGetCommand(message) == C_SET;

    mqtt::global_mqtt_client->publish(topic, msg, strlen(msg), 0, retain);

    // Custom MySensors node handling
    if (MySensorsCustomReceive)
    {
        MySensorsCustomReceive(message);
    }
}

/* 
 * protocolMQTTParse taken from MySensors MyProtocolMySensors.cpp w/minor modifications.
 * Requires #define MY_GATEWAY_MQTT_CLIENT which brings with it a lot of baggage.
 * 
 * Original copyright notice from MyProtocolMySensors.cpp
 * 
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2018 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
bool protocolMQTTParse(MyMessage &message, const char *topic, const char *payload, unsigned int length)
{
    char *str, *p;
    uint8_t i = 0;
    uint8_t command = 0;
    char *tt = const_cast<char *>(topic) + strlen(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX) + 1;

    for (str = strtok_r(tt, "/", &p); str && i <= 5; str = strtok_r(NULL, "/", &p))
    {
        switch (i)
        {
        case 0:
        {
            // Node id
            message.destination = atoi(str);
            break;
        }
        case 1:
        {
            // Sensor id
            message.sensor = atoi(str);
            break;
        }
        case 2:
        {
            // Command type
            command = atoi(str);
            mSetCommand(message, command);
            break;
        }
        case 3:
        {
            // Ack flag
            mSetRequestAck(message, atoi(str) ? 1 : 0);
            break;
        }
        case 4:
        {
            // Sub type
            message.type = atoi(str);
            break;
        }
        }
        i++;
    }

    if (i != 5)
    {
        return false;
    }

    message.sender = GATEWAY_ADDRESS;
    message.last = GATEWAY_ADDRESS;
    mSetAck(message, false);

    // Add payload
    if (command == C_STREAM)
    {
        uint8_t bvalue[MAX_PAYLOAD];
        uint8_t blen = 0;
        while (*payload)
        {
            uint8_t val;
            val = protocolH2i(*payload++) << 4;
            val += protocolH2i(*payload++);
            bvalue[blen] = val;
            blen++;
        }
        message.set(bvalue, blen);
    }
    else
    {
        char *ca;
        ca = (char *)payload;
        ca += length;
        *ca = '\0';
        message.set((const char *)payload);
    }

    return true;
}