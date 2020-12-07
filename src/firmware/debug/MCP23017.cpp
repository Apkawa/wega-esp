#include <Arduino.h>
const uint serial = SERIAL_SPEED;

#include <Wire.h>
#include "Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;

#define MCP_PIN 0

void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    mcp.begin();      // use default address 0
    mcp.pinMode(MCP_PIN, OUTPUT);
}

void loop() {
    // Blink
    mcp.digitalWrite(MCP_PIN, LOW);
    delay(1000);
    mcp.digitalWrite(MCP_PIN, HIGH);
}


