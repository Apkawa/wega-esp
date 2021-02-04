

#ifndef WEGA_ESP32_MCP23017PINADAPTER_H
#define WEGA_ESP32_MCP23017PINADAPTER_H

#include "AbstractPinAdapter.h"
#include <Arduino.h>

#include "Wire.h"
#include "Adafruit_MCP23017.h"

#define MCP23017_A0         0
#define MCP23017_A1         1
#define MCP23017_A2         2
#define MCP23017_A3         3
#define MCP23017_A4         4
#define MCP23017_A5         5
#define MCP23017_A6         6
#define MCP23017_A7         7
#define MCP23017_B0         8
#define MCP23017_B1         9
#define MCP23017_B2         10
#define MCP23017_B3         11
#define MCP23017_B4         12
#define MCP23017_B5         13
#define MCP23017_B6         14
#define MCP23017_B7         15


class MCP23017PinAdapter : AbstractPinAdapter {
public:
    MCP23017PinAdapter(Adafruit_MCP23017 &mcp) : mcp(mcp) {};

    void _pinMode(uint8_t pin, uint8_t mode) override {
        mcp.pinMode(pin, mode);
    };

    void _digitalWrite(uint8_t pin, uint8_t val) override {
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
