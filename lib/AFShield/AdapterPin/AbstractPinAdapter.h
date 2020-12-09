#ifndef WEGA_ESP32_ABSTRACTPINADAPTER_H
#define WEGA_ESP32_ABSTRACTPINADAPTER_H
#include <Arduino.h>


class AbstractPinAdapter {
public:
    virtual void _pinMode(uint8_t pin, uint8_t mode) = 0;

    virtual void _digitalWrite(uint8_t pin, uint8_t val) = 0;

    virtual void _analogWrite(uint8_t pin, uint8_t val) = 0;

    virtual void _shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) = 0;

};


#endif //WEGA_ESP32_ABSTRACTPINADAPTER_H
