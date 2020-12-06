//
// Created by apkawa on 12/6/20.
//

#include "wifi.h"

#ifdef ESP32
#include <ESPmDNS.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#endif

#include <WiFiUdp.h>


const char *hostname = HOSTNAME;
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

namespace wifi {
    void setup() {
        setup(ssid, password);
    }

    void setup(char const *ssid, char const *password) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        while (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.print("Wifi: ");
            Serial.println(ssid);
            Serial.println(password);
            Serial.println("Connection Failed! Rebooting...");
            delay(5000);

            ESP.restart();
        }

        MDNS.begin(hostname);
        MDNS.addService("http", "tcp", 80);

        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("mDns: ");
        Serial.print(hostname);
        Serial.println(".local");
    }
}