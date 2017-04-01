#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <ArduinoJson.h>

// Should contain 'wifi_ssid' and 'wifi_password'
#include "wifi_config.h"

// Pins connected to incremental rotary encoder inputs of thermostat
#define OUT0 D5
#define OUT1 D1

// Pins connected to incremental rotary encoder
#define IN0 D2
#define IN1 D3

// File for next update
#define UPDATE_URI "http://192.168.178.2/thermostat/kids.v0001.bin"

// IPAddress ip(192, 168, 178, 210);  // Hallway
IPAddress ip(192, 168, 178, 211);     // Kids room
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 178, 1);

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.config(ip, dns, gateway, subnet);
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

void in0_change() {
  int in0 = digitalRead(IN0);
  digitalWrite(OUT0, in0);
}

void in1_change() {
  int in1 = digitalRead(IN1);
  digitalWrite(OUT1, in1);
}

void setup() {
  Serial.begin(115200);

  pinMode(OUT0, OUTPUT);
  pinMode(OUT1, OUTPUT);

  pinMode(IN0, INPUT_PULLUP);
  pinMode(IN1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IN0), in0_change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(IN1), in1_change, CHANGE);
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
  static float old_temp = 0.0f;

  HTTPClient http;
  http.begin("192.168.178.2", 8123, "/api/states/input_slider.lenyas_room_thermostat");
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
            old_temp = temp;
            setTemperature(temp);
          }
      }
  }

  http.end();
}

void loop() {
  setup_wifi();

  // Try to update
  ESPhttpUpdate.update(UPDATE_URI);

  updateTemperatureIfChanged();

  // Sleep for 60 seconds
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin(60 * 1000000L);
  delay(60 * 1000);

  // Wakeup
  WiFi.mode(WIFI_STA);
}
