#ifndef WEGA_ESP32_UTILS_H
#define WEGA_ESP32_UTILS_H
#include <Arduino.h>

#define Serial_print(msg) Serial.print(msg)
#define Serial_println(msg) Serial.println(msg)

String formatFloat(float x, byte precision);


float kalman_filter(float val);


#endif //WEGA_ESP32_UTILS_H
