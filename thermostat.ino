#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "wifi_config.h"

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
  
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  
  server.on("/", handleSetTemperature);
  server.begin();
  Serial.println("HTTP server started");
}

void handleSetTemperature() {
  uint8 temp = -1;
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    if(server.argName(i) == "temp") {
      temp = server.arg(i).toInt();
    }
  }

  int z = 0;
  // Set temperature to OFF
  for(uint8_t i = 0; i < 60; ++i) {
    digitalWrite(D1, z);
    delay(30);
    digitalWrite(D0, z);
    delay(30);

    z = (z+1) % 2;
    yield();
  }

  for(uint8_t i = 0; i < temp*2; ++i) {
    digitalWrite(D0, z);
    delay(30);
    digitalWrite(D1, z);
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
