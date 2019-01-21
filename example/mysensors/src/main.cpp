// Auto generated code by esphomeyaml
// ========== AUTO GENERATED INCLUDE BLOCK BEGIN ===========
#include "esphomelib/application.h"
using namespace esphomelib;
// ========== AUTO GENERATED INCLUDE BLOCK END ===========

#define MY_RADIO_NRF24
#define MY_RF24_CE_PIN D0
#define MY_RF24_CS_PIN D4
#define MY_GATEWAY_SERIAL
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mysensors_rx"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mysensors_tx"

#include "../../../ESPHomeMySensorsGatewayShim.h"
#include "ESPHomeMySensorsCustomReceive.h"
MySensorsGatewayShim MSGS;



void setup()
{
  // ===== DO NOT EDIT ANYTHING BELOW THIS LINE =====
  // ========== AUTO GENERATED CODE BEGIN ===========
  App.set_name("mysensors");
  App.set_compilation_datetime(__DATE__ ", " __TIME__);
  LogComponent *logcomponent = App.init_log(115200);
  logcomponent->set_global_log_level(ESPHOMELIB_LOG_LEVEL_DEBUG);
  WiFiComponent *wificomponent = App.init_wifi();
  WiFiAP wifiap = WiFiAP();
  wifiap.set_ssid("My Wifi SSID");
  wifiap.set_password("My Wifi Password");
  wificomponent->add_sta(wifiap);
  OTAComponent *otacomponent = App.init_ota();
  otacomponent->start_safe_mode();
  mqtt::MQTTClientComponent *mqtt_mqttclientcomponent = App.init_mqtt("test.mosquitto.org", 1883, "", "");
  // =========== AUTO GENERATED CODE END ============
  // ========= YOU CAN EDIT AFTER THIS LINE =========
  MSGS.setup();
  App.setup();
}

void loop()
{
  App.loop();
}
