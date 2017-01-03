#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>

// Should contain 'wifi_ssid' and 'wifi_password'
#include "wifi_config.h"

// Pins connected to incremental rotary encoder
#define OUT0 D5
#define OUT1 D1

#define OLD_TEMP_EEPROM_ADDRESS 0


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(9600);

  // Connect D0 to RST to wake up
  pinMode(D0, WAKEUP_PULLUP);

  setup_wifi();

  pinMode(OUT0, OUTPUT);
  pinMode(OUT1, OUTPUT);

  updateTemperatureIfChanged();

  // Go into deep sleep for 60 seconds
  ESP.deepSleep(60 * 1000000);
}

void setTemperature(float temp) {
  int z = 0;
  // Set temperature to OFF
  for(uint8_t i = 0; i < 60; ++i) {
    digitalWrite(OUT1, z);
    delay(30);
    digitalWrite(OUT0, z);
    delay(30);

    z = (z+1) % 2;
  }

  /* Substract 4.5 from the given temperature to compute the number of steps
   * required to set the desired temperature, as the temperature directly jumps
   * from OFF tp 5C. */
  for(uint8_t i = 0; i < (temp-4.5)*2; ++i) {
    digitalWrite(OUT0, z);
    delay(30);
    digitalWrite(OUT1, z);
    delay(30);

    z = (z+1) % 2;
  }
}

void updateTemperatureIfChanged() {
  float old_temp;
  EEPROM.get(OLD_TEMP_EEPROM_ADDRESS, old_temp);

  HTTPClient http;
  http.begin("192.168.178.2", 8123, "/api/states/input_slider.hallway_thermostat");
  int httpCode = http.GET();

  if(httpCode > 0) {
      // file found at server
      if(httpCode == HTTP_CODE_OK) {
          String payload = http.getString();

          /*
           * {
           *   "attributes": {
           *     "friendly_name": "Thermostat temperature",
           *       "max": 22.0,
           *       "min": 0.0,
           *       "step": 0.5
           *   },
           *   "entity_id": "input_slider.hallway_thermostat",
           *   "last_changed": "2016-12-28T18:12:40.244266+00:00",
           *   "last_updated": "2016-12-28T18:12:40.244266+00:00",
           *   "state": "0.0"
           * }
           *
           *
           * This yields 160, which is not sufficient:
           * const int BUFFER_SIZE = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5);
           *
           * I found 500 to work by trial and error.
           */
          StaticJsonBuffer<500> jsonBuffer;

          JsonObject& root = jsonBuffer.parseObject(payload);
          if (!root.success()) {
            Serial.println("parseObject() failed");
            return;
          }

          float temp = root["state"];
          if(temp != old_temp) {
            EEPROM.put(OLD_TEMP_EEPROM_ADDRESS, temp);
            setTemperature(temp);
          }
      }
  }

  http.end();
}

void loop() {}
