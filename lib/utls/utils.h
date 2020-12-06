#ifndef WEGA_ESP32_UTILS_H
#define WEGA_ESP32_UTILS_H
#include <Arduino.h>

String formatFloat(float x, byte precision);


float kalman_filter(float val);


#endif //WEGA_ESP32_UTILS_H
