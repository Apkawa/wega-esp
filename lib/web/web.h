//
// Created by apkawa on 12/6/20.
//

#ifndef WEGA_ESP32_WEB_H
#define WEGA_ESP32_WEB_H

#ifdef ESP32
#include <WebServer.h>
#include <HTTPClient.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#define WebServer ESP8266WebServer
#endif




#endif //WEGA_ESP32_WEB_H
