#include <web.h>

WebServer server(80);

#include <WiFiClient.h>


#include <wifi.h>
#include <ota.h>
#include <utils.h>


#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>
Adafruit_AM2320 am2320 = Adafruit_AM2320();


const uint serial = SERIAL_SPEED;


void handleRoot() {
    String httpstr = "<meta http-equiv='refresh' content='10'>";
    httpstr += "Hello World!<br>";
    httpstr += "Temp: " + formatFloat(am2320.readTemperature(), 2) + "<br>";
    httpstr += "Hum: " + formatFloat(am2320.readHumidity(), 2);
    server.send(200, "text/html", httpstr);
}

void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    wifi::setup();
    Ota::setup();

    server.on("/", handleRoot);
    server.begin();

    am2320.begin();
}

void loop() {
    server.handleClient();
    Ota::loop();

    Serial.print("Temp: ");
    Serial.println(am2320.readTemperature());
    Serial.print("Hum: ");
    Serial.println(am2320.readHumidity());
}


