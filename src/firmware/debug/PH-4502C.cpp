#include <Arduino.h>


#include <wifi.h>
#include <ota.h>

// Пример чтения с pH сенсора


float AnalogReadMid(int port, long count) {
    server.handleClient();
    ArduinoOTA.handle();

    long n = 0;
    double sensorValue = 0;
    while (n < count) {
        n++;
        server.handleClient();
        ArduinoOTA.handle();
        sensorValue = (analogRead(port)) + sensorValue;
    }
    return sensorValue / n;

}

void setup() {
    Serial.begin(SERIAL_SPEED);
    Serial.println("Booting");

    wifi::setup();
    Ota::setup();

}

void loop() {
    Ota::loop();

    // TODO оформить пример
    // read the input on analog pin 0:
    long n = 0;
    float a, b;

    // Калибровка pH
    float x1 = 3730;// Показания сенсора при pH 4.01
    float y1 = 4.01;
    float x2 = 2977;// Показания сенсора при pH 6.86
    float y2 = 6.86;
    a = (-x2 * y1 + y2 * x1) / (-x2 + x1);
    b = (-y2 + y1) / (-x2 + x1);

    //pH shield
    pHraw = AnalogReadMid(33, 5);
    pH = a + b * pHraw;
    Serial.print("pHraw: ");
    Serial.print(pHraw);
    Serial.print(" pH: ");
    Serial.println(pH);

}


