#include <Arduino.h>
#include <web.h>

WebServer server(80);


#include <wifi.h>
#include <ota.h>


void handleRoot() {
    String httpstr = "<meta http-equiv='refresh' content='10'>";
    httpstr += "Hello World!<br>";
    server.send(200, "text/html", httpstr);
}

void setup() {
    Serial.begin(SERIAL_SPEED);
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


