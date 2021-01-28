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
        11, // B3
        8, // B0
        2, // A2
        10, // B2
        3, // A3
        0, // A0
        9, // B1
        1, // A1
        0,
        0
);
AFShield<MCP23017PinAdapter> shield2(
        &pinAdapter,
        15, // B7
        12, // B4
        6, // A6
        14, // B6
        7, // A7
        4, // A4
        13, // B5
        5, // A5
        0,
        0
);

auto motor1 = shield1.getMotor(1);
auto motor4 = shield2.getMotor(4);

const uint serial = SERIAL_SPEED;



void setup() {
    Serial.begin(serial);
    Serial.println("Booting");


    mcp.begin();
    Serial.println("Mcp");
    motor1.stop();
}

void loop() {
    server.handleClient();
    Ota::loop();
    if (Serial.available()) {
        char temp = Serial.read();
        switch (temp) {
            case '+':
                motor4.forward();
                Serial.println("Forward");
                break;
            case '-':
                motor4.backward();
                Serial.println("Backward");
                break;
            case 's':
                motor4.stop();
                Serial.println("Stop");
                break;

        }
    }
}


