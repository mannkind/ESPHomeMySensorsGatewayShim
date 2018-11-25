# ESPHomeLibMySensorsGatewayShim

A barebones shim to enable an ESPHomeLib device to function as a MySensors MQTT Gateway. Tested with ESPHomeLib 1.9.1, MySensors 2.3.0, and a Wemos D1 Mini w/an RF24 Radio.

## Setup/Use

Take a look at the example directory for a fully working example (very close to my setup).

In short:

* Add `#defines` from your MySensors gateway sketch to `main.cpp`.

  * `#define MY_RADIO_NRF24`
  * `#define MY_GATEWAY_SERIAL`
  * `#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mysensors_rx"`
  * `#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mysensors_tx"`

  On my setup, I had to change the pins for the RF24 radio.

  * `#define MY_RF24_CE_PIN D0`
  * `#define MY_RF24_CS_PIN D4`

* Add `#include "ESPHomeLibMySensorsGatewayShim.h"` under the `#defines`.

* Add the following lines before `App.setup()`, be sure to change the list of existing nodes to match your setup.

  ```c++
  const std::vector<uint8_t> existingNodes = {2, 15};
  MySensorsGatewayShim(existingNodes);

  App.setup();
  ```

* If you want to add a custom handler for received messages, use `void MySensorsCustomReceive(const MyMessage &message)`. See `example/src/ESPHomeLibMySensorsCustomReceive.h` for how I handle a few of my nodes.