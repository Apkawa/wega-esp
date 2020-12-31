#ifndef WEGA_ESP32_UTILS_H
#define WEGA_ESP32_UTILS_H
#include <Arduino.h>

// undefine stdlib's abs if encountered
#ifdef abs
#undef abs
#endif

#define abs(x) ((x)>0?(x):-(x))

#define Serial_print(msg) Serial.print(msg)
#define Serial_println(msg) Serial.println(msg)

String formatFloat(float x, byte precision);


float kalman_filter(float val);


#endif //WEGA_ESP32_UTILS_H
