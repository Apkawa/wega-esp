//
// Created by apkawa on 12/6/20.
//

#ifndef WEGA_ESP32_WIFI_H
#define WEGA_ESP32_WIFI_H

#include "web.h"

namespace wifi {
    void setup();
    void setup(char const *ssid, char const *password);
    void loop();
    void setup_server(WebServer *server);
}


#endif //WEGA_ESP32_WIFI_H
