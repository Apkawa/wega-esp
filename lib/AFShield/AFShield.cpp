#include "AFShield.h"

// Motor number on shield
#define MOTOR1_A 2
#define MOTOR1_B 3

#define MOTOR2_A 1
#define MOTOR2_B 4

#define MOTOR3_A 5
#define MOTOR3_B 7

#define MOTOR4_A 0
#define MOTOR4_B 6

#define FORWARD 1
#define BACKWARD 2
#define BRAKE 3
#define RELEASE 4


void AFShield::shiftWrite(uint8_t output, uint8_t high_low) {
    static int latch_copy;
    static int shift_register_initialized = false;
// Do the initialization on the fly,
// at the first time it is used.
    if (!shift_register_initialized) {
// Set pins for shift register to output
        _pinMode(pinLatch, OUTPUT);
        _pinMode(pinEnable, OUTPUT);
        _pinMode(pinClk, OUTPUT);
        _pinMode(pinData, OUTPUT);
// Set pins for shift register to default value (low);
        _digitalWrite(pinLatch, LOW);
        _digitalWrite(pinClk, LOW);
        _digitalWrite(pinData, LOW);
// Enable the shift register, set Enable pin Low.
        _digitalWrite(pinEnable, LOW);
// start with all outputs (of the shift register) low
        latch_copy = 0;
        shift_register_initialized = true;
    }
// The defines HIGH and LOW are 1 and 0.
// So this is valid.
    bitWrite(latch_copy, output, high_low);
    _shiftOut(pinData, pinClk, MSBFIRST, latch_copy);
    delayMicroseconds(5); // For safety, not really needed.
    _digitalWrite(pinLatch, HIGH);
    delayMicroseconds(5); // For safety, not really needed.
    _digitalWrite(pinLatch, LOW);

}

void AFShield::motorOutput(uint8_t output, uint8_t high_low, int speed) {
    int motorPWM;

    switch (output) {
        case MOTOR1_A:
        case MOTOR1_B:
            motorPWM = pinMotor1Speed;
            break;
        case MOTOR2_A:
        case MOTOR2_B:
            motorPWM = pinMotor2Speed;
            break;
        case MOTOR3_A:
        case MOTOR3_B:
            motorPWM = pinMotor3Speed;
            break;
        case MOTOR4_A:
        case MOTOR4_B:
            motorPWM = pinMotor4Speed;
            break;
        default:
            // incorrect motor!
            return;
    }
    shiftWrite(output, high_low);
    // set PWM only if it is valid
    if (speed >= 0 && speed <= 255) {
        _analogWrite(motorPWM, speed);
    }

}

void AFShield::motor(uint8_t nMotor, uint8_t command, int speed) {
    if (nMotor < 1 || nMotor > 4) return;

    int motorA, motorB;
    switch (nMotor) {
        case 1:
            motorA = MOTOR1_A;
            motorB = MOTOR1_B;
            break;
        case 2:
            motorA = MOTOR2_A;
            motorB = MOTOR2_B;
            break;
        case 3:
            motorA = MOTOR3_A;
            motorB = MOTOR3_B;
            break;
        case 4:
            motorA = MOTOR4_A;
            motorB = MOTOR4_B;
            break;
        default:
            break;
    }
    switch (command) {
        case FORWARD:
            motorOutput(motorA, HIGH, speed);
            motorOutput(motorB, LOW, -1); // -1: no PWM set
            break;
        case BACKWARD:
            motorOutput(motorA, LOW, speed);
            motorOutput(motorB, HIGH, -1); // -1: no PWM set
            break;
        case BRAKE:
            motorOutput(motorA, LOW, 255); // 255: fully on.
            motorOutput(motorB, LOW, -1); // -1: no PWM set
            break;
        case RELEASE:
            motorOutput(motorA, LOW, 0); // 0: output floating.
            motorOutput(motorB, LOW, -1); // -1: no PWM set
            break;
        default:
            break;
    }
}

void AFShield::_pinMode(uint8_t pin, uint8_t mode) {
    Serial.print("PIN_MODE");
    Serial.print(pin);
    Serial.print(" ");
    Serial.println(mode);

    pinMode(pin, mode);
}

void AFShield::_digitalWrite(uint8_t pin, uint8_t val) {
    Serial.print("digitalWrite");
    Serial.print(pin);
    Serial.print(" ");
    Serial.println(val);

    digitalWrite(pin, val);
}

void AFShield::_analogWrite(uint8_t pin, uint8_t val) {
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

void AFShield::_shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) {
    Serial.print("shiftOut");
    Serial.print(dataPin);
    Serial.print(" ");
    Serial.print(clockPin);
    Serial.print(" ");
    Serial.println(val);
    shiftOut(dataPin, clockPin, bitOrder, val);
}

const AFShield::AFMotor &AFShield::getMotor(uint8_t nMotor) {
    auto m = motors[nMotor - 1];
    if (m == nullptr) {
        motors[nMotor - 1] = (m = new AFMotor(*this, nMotor));
    }
    static auto _m = *m;
    return _m;
}


void AFShield::AFMotor::forward() {
    shield.motor(number, FORWARD, 255);

}

void AFShield::AFMotor::backward() {
    shield.motor(number, BACKWARD, 255);

}

void AFShield::AFMotor::stop() {
    shield.motor(number, BRAKE, 255);
}

void AFShield::AFMotor::release() {
    shield.motor(number, RELEASE, 255);

}
