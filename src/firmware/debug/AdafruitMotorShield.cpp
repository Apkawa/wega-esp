#include <Arduino.h>
#include <web.h>

WebServer server(80);


#include <wifi.h>
#include <ota.h>
#include <utils.h>

#include <AFShield.h>

const uint serial = SERIAL_SPEED;

// https://www.dessy.ru/include/images/ware/pdf/s/shield_l293d.pdf
// Поддержка шилда Adafruit Motor Shield (4DC или 2stepper/servos)
// http://robotosha.ru/arduino/motor-shield.html
// https://cdn-learn.adafruit.com/assets/assets/000/009/536/original/adafruit_products_mshieldv2schem.png?1396892649

AFShield<> shield1(
        18,
        4,
        17,
        5,
        0,
        0,
        0,
        16,
        0,
        0
);
auto motor1 = shield1.getMotor(4);

void handleRoot() {
    String httpstr = "<meta http-equiv='refresh' content='10'>";
    httpstr += "Hello World!<br>";
    server.send(200, "text/html", httpstr);
}

void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    wifi::setup();
    Ota::setup();

    server.on("/", handleRoot);
    server.begin();

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


