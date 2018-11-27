// Auto generated code by esphomeyaml
#include "esphomelib/application.h"

using namespace esphomelib;

#include <MySensors.h>

// Function prototypes
void MySensorsGatewayShim(const std::vector<uint8_t> &existingNodes);
void MySensorsBootloaderIDResponse();
void MysensorsBootloaderCFResponse(const std::vector<uint8_t> &existingNodes);
void MySensorsCustomReceive(const MyMessage &message) __attribute__((weak));
bool protocolMQTTParse(MyMessage &message, char *topic, uint8_t *payload, unsigned int length);

const char *TAG = "component.mysensors_gateway";

std::string uint8_to_string(uint8_t num)
{
  char buf[4]; // 0-255 + \0
  sprintf(buf, "%d", num);
  return std::string(buf);
}

std::string MySensorsBootloaderIDResponseTopic()
{
  return std::string(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "/255/255/3/0/4");
}

std::string mysensorsConfigurationFirmwareResponseTopic(uint8_t nodeId, uint8_t type)
{
  return std::string(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "/") + uint8_to_string(nodeId) + "/255/4/0/" + uint8_to_string(type);
}

void MySensorsGatewayShim(const std::vector<uint8_t> &existingNodes)
{
  MySensorsBootloaderIDResponse();
  MysensorsBootloaderCFResponse(existingNodes);
}

void MySensorsBootloaderIDResponse()
{
  static MyMessage msg;
  const auto callback = [](const std::string &_payload) {
    const auto _topic = MySensorsBootloaderIDResponseTopic();
    msg.clear();
    ESP_LOGD(TAG, "Received ID Response - Topic: %s, Payload: %s", _topic.c_str(), _payload.c_str());

    const auto topic = const_cast<char *>(_topic.c_str());
    const auto payload = reinterpret_cast<uint8_t *>(const_cast<char *>(_payload.c_str()));
    const auto size = _payload.length();

    const auto parsed = protocolMQTTParse(msg, topic, payload, size);
    const auto result = transportRouteMessage(msg);
  };

  const auto topic = MySensorsBootloaderIDResponseTopic();
  mqtt::global_mqtt_client->subscribe(topic, callback);
}

void MysensorsBootloaderCFResponse(const std::vector<uint8_t> &existingNodes)
{
  static MyMessage msg;
  for (uint8_t i = 0; i < existingNodes.size(); i++)
  {
    const auto nodeId = existingNodes[i];
    for (uint8_t type = 1; type < 4; type += 2)
    {
      const auto callback = [nodeId, type](const std::string &_payload) {
        const auto _topic = mysensorsConfigurationFirmwareResponseTopic(nodeId, type);
        msg.clear();
        ESP_LOGD(TAG, "Received CF Response - NodeID: %u; Topic: %s, Payload: %s", nodeId, _topic.c_str(), _payload.c_str());

        const auto topic = const_cast<char *>(_topic.c_str());
        const auto payload = reinterpret_cast<uint8_t *>(const_cast<char *>(_payload.c_str()));
        const auto size = _payload.length();

        const auto parsed = protocolMQTTParse(msg, topic, payload, size);
        const auto result = transportRouteMessage(msg);
      };

      const auto topic = mysensorsConfigurationFirmwareResponseTopic(nodeId, type);
      mqtt::global_mqtt_client->subscribe(topic, callback);
    }
  }
}

/* 
 * Allows the gateway to send MySensors compatible MQTT topics/payloads
 * And custom topics/payloads for esphomelib compatibility
 */
void receive(const MyMessage &_message)
{
  MyMessage message = _message;
  ESP_LOGD(TAG, "RF24Packet - Sender: %u, Sensor: %u, Command: %u, Type: %u; Payload: %s", message.sender, message.sensor, mGetCommand(message), message.type, message.getString(_convBuffer));

  // Default MySensors node handling
  const auto topic = protocolFormatMQTTTopic(MY_MQTT_PUBLISH_TOPIC_PREFIX, message);
  const auto msg = message.getString(_convBuffer);
  const auto retain = mGetCommand(message) == C_SET;

  mqtt::global_mqtt_client->publish(topic, msg, strlen(msg), 0, retain);

  // Custom MySensors node handling
  if (MySensorsCustomReceive)
  {
    MySensorsCustomReceive(message);
  }
}

/* 
 * protocolMQTTParse taken from MySensors MyProtocolMySensors.cpp
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
bool protocolMQTTParse(MyMessage &message, char *topic, uint8_t *payload, unsigned int length)
{
  char *str, *p;
  uint8_t i = 0;
  uint8_t command = 0;
  if (topic != strstr(topic, MY_MQTT_SUBSCRIBE_TOPIC_PREFIX))
  {
    // Prefix doesn't match incoming topic
    return false;
  }
  for (str = strtok_r(topic + strlen(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX) + 1, "/", &p); str && i <= 5;
       str = strtok_r(NULL, "/", &p))
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