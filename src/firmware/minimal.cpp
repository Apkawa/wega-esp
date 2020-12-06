
#include <web.h>

WebServer server(80);

#include <WiFiClient.h>

#include <wifi.h>
#include <ota.h>
#include <utils.h>

const uint serial = SERIAL_SPEED;


void handleRoot() {
    String httpstr = "<meta http-equiv='refresh' content='10'>";
    httpstr += "Hello World! from ota5<br>";
    server.send(200, "text/html", httpstr);
}

void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    wifi::setup();
    Ota::setup();

    server.on("/", handleRoot);
    server.begin();
}

void loop() {
    server.handleClient();
    Ota::loop();
}


