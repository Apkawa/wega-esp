//
// Created by apkawa on 12/6/20.
//

#ifndef WEGA_ESP32_WIFI_H
#define WEGA_ESP32_WIFI_H


namespace wifi {
    void setup();
    void setup(char const *ssid, char const *password);
    void loop();
}


#endif //WEGA_ESP32_WIFI_H
