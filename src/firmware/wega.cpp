#include <Arduino.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebServer.h>

// #include "Wire.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>

#include <WiFiClient.h>
#include <HTTPClient.h>

#include "options.h"
#include "utils.h"


Adafruit_AM2320 am2320 = Adafruit_AM2320();


WebServer server(80);

//const char *mDNSName = NAME;
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

String formatFloat(float x, byte precision) {
    char tmp[50];
    dtostrf(x, 0, precision, tmp);
    return String(tmp);
}

void handleRoot() {
    String httpstr = "<meta http-equiv='refresh' content='10'>";
    httpstr += "Hello World!<br>";
    httpstr += "Temp: " + formatFloat(am2320.readTemperature(), 2) + "<br>";
    httpstr += "Hum: " + formatFloat(am2320.readHumidity(), 2);
    server.send(200, "text/html", httpstr);
}



void setup() {
    Serial.begin(115200);
    Serial.println("Booting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        //Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    MDNS.begin("esp32w3");
    MDNS.addService("http", "tcp", 80);
    server.on("/", handleRoot);
    server.begin();

    am2320.begin();

}

void loop() {
    server.handleClient();
    ArduinoOTA.handle();

    Serial.print("Temp: ");
    Serial.println(am2320.readTemperature());
    Serial.print("Hum: ");
    Serial.println(am2320.readHumidity());

    // todo task
    delay(1000);
}

