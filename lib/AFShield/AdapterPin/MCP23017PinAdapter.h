

#ifndef WEGA_ESP32_MCP23017PINADAPTER_H
#define WEGA_ESP32_MCP23017PINADAPTER_H

#include "AbstractPinAdapter.h"
#include <Arduino.h>

#include "Wire.h"
#include "Adafruit_MCP23017.h"


class MCP23017PinAdapter : AbstractPinAdapter {
public:
    MCP23017PinAdapter(Adafruit_MCP23017 &mcp) : mcp(mcp) {};

    void _pinMode(uint8_t pin, uint8_t mode) override {
        Serial.println("MCP_pinMode");
        mcp.pinMode(pin, mode);
    };

    void _digitalWrite(uint8_t pin, uint8_t val) override {
        Serial.println("MCP_digitalWrite");
        mcp.digitalWrite(pin, val);
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

private:
    Adafruit_MCP23017 &mcp;



};


#endif //WEGA_ESP32_MCP23017PINADAPTER_H
