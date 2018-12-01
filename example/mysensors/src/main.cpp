// Auto generated code by esphomeyaml
#include "esphomelib/application.h"

using namespace esphomelib;

#define MY_RADIO_NRF24
#define MY_RF24_CE_PIN D0
#define MY_RF24_CS_PIN D4
#define MY_GATEWAY_SERIAL
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mysensors_rx"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mysensors_tx"

#include "ESPHomeLibMySensorsGatewayShim.h"
#include "ESPHomeLibMySensorsCustomReceive.h"
MySensorsGatewayShim MSGS;

void setup() {
  // ===== DO NOT EDIT ANYTHING BELOW THIS LINE =====
  // ========== AUTO GENERATED CODE BEGIN ===========
  App.set_name("mysensors");
  App.set_compilation_datetime(__DATE__ ", " __TIME__);
  ::LogComponent *_logcomponent = App.init_log(115200);
  _logcomponent->set_global_log_level(ESPHOMELIB_LOG_LEVEL_DEBUG);
  ::WiFiComponent *_wificomponent = App.init_wifi();
  _wificomponent->set_sta(::WiFiAp{
      .ssid = "My Wifi SSID",
      .password = "My Wifi Password",
      .channel = -1,
  });
  ::OTAComponent *_otacomponent = App.init_ota();
  _otacomponent->start_safe_mode();
  App.init_mqtt("mosquitto.org", 1883, "", "");
  // =========== AUTO GENERATED CODE END ============
  // ========= YOU CAN EDIT AFTER THIS LINE =========
  MSGS.setup();
  App.setup();
}

void loop() {
  App.loop();
  delay(16);
}
