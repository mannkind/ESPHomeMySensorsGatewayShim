// Auto generated code by esphomeyaml
#include "esphomelib/application.h"
#include "core/MyMessage.h"

using namespace esphomelib;

void MySensorsCustomReceive(const MyMessage &message)
{
    if (mGetCommand(message) != C_SET || message.isAck())
    {
        return;
    }

    const auto table_plants = 2,
               doorbell = 15;

    // Table Plants; Moisture Sensor
    if (message.sender == table_plants && message.type == V_LEVEL)
    {
        char buf[5]; // 0-1024 + \0
        const auto msg = message.getString(buf);
        mqtt::global_mqtt_client->publish("table_plants", msg, strlen(msg), 0, true);
    }
    else if (message.sender == doorbell && message.type == V_TRIPPED)
    {
        const auto msg = message.getInt() == HIGH ? "ON" : "OFF";
        mqtt::global_mqtt_client->publish("doorbell", msg, strlen(msg), 0, true);
    }
}