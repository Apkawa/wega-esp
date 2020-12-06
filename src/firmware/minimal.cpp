#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebServer.h>

WebServer server(80);

#include <WiFiClient.h>
#include <HTTPClient.h>

const char *hostname = HOSTNAME;
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;
const uint serial = SERIAL_SPEED;


void handleRoot() {
    String httpstr = "<meta http-equiv='refresh' content='10'>";
    httpstr += "Hello World! from ota<br>";
    server.send(200, "text/html", httpstr);
}

void setup() {
    Serial.begin(serial);
    Serial.println("Booting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.print("Wifi: ");
        Serial.println(ssid);
        Serial.print(password);
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);

        ESP.restart();
    }

    ArduinoOTA.setHostname(hostname);
    ArduinoOTA
            .onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    type = "filesystem";

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println("Start updating " + type);
            })
            .onEnd([]() {
                Serial.println("\nEnd");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");
            });

    ArduinoOTA.begin();

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    MDNS.begin(hostname);
    MDNS.addService("http", "tcp", 80);
    server.on("/", handleRoot);
    server.begin();


}

void loop() {
    server.handleClient();
    ArduinoOTA.handle();
}

