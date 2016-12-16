#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Should contain 'wifi_ssid' and 'wifi_password'
#include "wifi_config.h"

// Pins connected to incremental rotary encoder
#define OUT0 D0
#define OUT1 D1

ESP8266WebServer server(80);

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
  Serial.begin(115200);

  setup_wifi();

  pinMode(OUT0, OUTPUT);
  pinMode(OUT1, OUTPUT);

  server.on("/", handleSetTemperature);
  server.begin();
  Serial.println("HTTP server started");
}

void handleSetTemperature() {
  float temp = -1;
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    if(server.argName(i) == "temp") {
      temp = server.arg(i).toFloat();
    }
  }

  int z = 0;
  // Set temperature to OFF
  for(uint8_t i = 0; i < 60; ++i) {
    digitalWrite(OUT1, z);
    delay(30);
    digitalWrite(OUT0, z);
    delay(30);

    z = (z+1) % 2;
    yield();
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
    yield();
  }

  server.send(204, "text/html", "");
}

void loop() {
  if(WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }
  server.handleClient();
}
