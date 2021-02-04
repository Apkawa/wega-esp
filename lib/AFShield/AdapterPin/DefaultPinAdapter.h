

#ifndef WEGA_ESP32_DEFAULTPINADAPTER_H
#define WEGA_ESP32_DEFAULTPINADAPTER_H

#include "AbstractPinAdapter.h"
#include <Arduino.h>


class DefaultPinAdapter : AbstractPinAdapter {
public:
    void _pinMode(uint8_t pin, uint8_t mode) override {
        pinMode(pin, mode);
    };

    void _digitalWrite(uint8_t pin, uint8_t val) override {
        digitalWrite(pin, val);
    }

    void _shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) override {
        uint8_t i;

        for(i = 0; i < 8; i++) {
            if(bitOrder == LSBFIRST)
                _digitalWrite(dataPin, !!(val & (1 << i)));
            else
                _digitalWrite(dataPin, !!(val & (1 << (7 - i))));

            _digitalWrite(clockPin, HIGH);
            _digitalWrite(clockPin, LOW);
        }
    };

    void _analogWrite(uint8_t pin, uint8_t val) override {
        _pinMode(pin, OUTPUT);
        switch (val) {
            case 0:
                _digitalWrite(pin, LOW);
                break;
            case 255:
                _digitalWrite(pin, HIGH);
                break;
            default:
                //todo
                ;
        }
    }



};


#endif //WEGA_ESP32_DEFAULTPINADAPTER_H
