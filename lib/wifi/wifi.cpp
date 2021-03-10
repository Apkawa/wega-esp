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

#define FALLBACK_SSID HOSTNAME
#define FALLBACK_IP IPAddress(10, 0, 0, 1)
#define FALLBACK_GW IPAddress(10, 0, 0, 1)

const char *hostname = HOSTNAME;
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

uint8_t fallbackMode = 0;

namespace wifi {
    void setup() {
        setup(ssid, password);
    }

    void setup(char const *ssid, char const *password) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);

        delay(1000);
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.print("Run fallback mode");
            delay(1000);
            WiFi.disconnect();
            WiFi.mode(WIFI_AP);
            WiFi.softAP(FALLBACK_SSID);
            WiFi.softAPConfig(
                    FALLBACK_IP, FALLBACK_GW,
                    IPAddress(255, 255, 255, 0));
            delay(1000);
            fallbackMode = 1;
        }
        MDNS.begin(hostname);
        MDNS.addService("http", "tcp", 80);

        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("mDns: ");
        Serial.print(hostname);
        Serial.println(".local");
    }

    void setup_server(WebServer *server) {

        server->on("/wifi", [server]() {
            if (server->method() == HTTP_POST) {
                auto new_ssid = server->arg("ssid");
                auto new_password = server->arg("password");
                server->send(200, "text/html", "Wifi changed " + new_ssid + ":" + new_password);
                WiFi.disconnect();
                MDNS.end();
                delay(1000);
                setup(new_ssid.c_str(), new_password.c_str());
                return;
            }
            String body = "<h2>Wifi config<h2>"
                          "<form action='/wifi' method='post'>"
                          "<input name='ssid' placeholder='ssid' autocorrect='off' autocapitalize='none' />"
                          "<input name='password' placeholder='password'  autocorrect='off' autocapitalize='none' />"
                          "<button type='submit'>Save</button>"
                          "</form>";

            server->send(200, "text/html", body);
        });
    }
}