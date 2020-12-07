#include <web.h>

WebServer server(80);

#include <WiFiClient.h>

#include <wifi.h>
#include <ota.h>
#include <utils.h>

// Test DS18B20 temperature sensor
#include <OneWire.h>
#include <DallasTemperature.h>


// GPIO4
#define ONE_WIRE_BUS 4

// создаем экземпляр класса oneWire; с его помощью
// можно коммуницировать с любыми девайсами, работающими
// через интерфейс 1-Wire, а не только с температурными датчиками
// от компании Maxim/Dallas:
OneWire oneWire(ONE_WIRE_BUS);

// передаем объект oneWire объекту DS18B20:
DallasTemperature DS18B20(&oneWire);

float getTemperature() {
    float tempC;
    do {
        DS18B20.requestTemperatures();
        tempC = DS18B20.getTempCByIndex(0);
        delay(100);
    } while (tempC == 85.0 || tempC == (-127.0));
    return tempC;
}

const uint serial = SERIAL_SPEED;


void handleRoot() {
    String httpstr = "<meta http-equiv='refresh' content='10'>";
    httpstr += "Hello World!<br>";
    httpstr += "DS18B20: " + formatFloat(getTemperature(), 2) + "C <br>";
    server.send(200, "text/html", httpstr);
}

void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    wifi::setup();
    Ota::setup();

    server.on("/", handleRoot);
    server.begin();

    // по умолчанию разрешение датчика – 9-битное;
    // если у вас какие-то проблемы, его имеет смысл
    // поднять до 12 бит; если увеличить задержку,
    // это даст датчику больше времени на обработку
    // температурных данных
    DS18B20.begin();
}

void loop() {
    server.handleClient();
    Ota::loop();
}


