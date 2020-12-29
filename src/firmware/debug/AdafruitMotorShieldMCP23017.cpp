#include <Arduino.h>
#include <web.h>

WebServer server(80);


#include <wifi.h>
#include <ota.h>
#include <utils.h>

// https://www.dessy.ru/include/images/ware/pdf/s/shield_l293d.pdf
// Поддержка шилда Adafruit Motor Shield (4DC или 2stepper/servos)
// http://robotosha.ru/arduino/motor-shield.html
// https://cdn-learn.adafruit.com/assets/assets/000/009/536/original/adafruit_products_mshieldv2schem.png?1396892649

#include <AFShield.h>
#include <AdapterPin/MCP23017PinAdapter.h>
#include "Adafruit_MCP23017.h"

Adafruit_MCP23017 mcp;
MCP23017PinAdapter pinAdapter(mcp);

AFShield<MCP23017PinAdapter> shield1(
        &pinAdapter,
        4,
        0,
        2,
        3,
        0,
        0,
        0,
        1,
        0,
        0
);

auto motor1 = shield1.getMotor(4);

const uint serial = SERIAL_SPEED;



void setup() {
    Serial.begin(serial);
    Serial.println("Booting");


    mcp.begin();
    Serial.println("Mcp");
    motor1.release();
}

void loop() {
    server.handleClient();
    Ota::loop();
    if (Serial.available()) {
        char temp = Serial.read();
        switch (temp) {
            case '+':
                motor1.forward();
                Serial.println("Forward");
                break;
            case '-':
                motor1.backward();
                Serial.println("Backward");
                break;
            case 's':
                motor1.stop();
                Serial.println("Stop");
                break;

        }
    }
}


