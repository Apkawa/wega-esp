
#ifndef AFSHIELD_H
#define AFSHIELD_H

#include <Arduino.h>
// For analogWrite
#include <esp32-hal-ledc.h>
#include <AdapterPin/DefaultPinAdapter.h>


// Pins
#define MOTORLATCH 12
#define MOTORCLK 4
#define MOTORENABLE 7
#define MOTORDATA 8

// Speed control
#define MOTOR1_PWM 11
#define MOTOR2_PWM 3
#define MOTOR3_PWM 6
#define MOTOR4_PWM 5
#define SERVO1_PWM 10
#define SERVO2_PWM 9


typedef uint8_t pin;

template<class PinAdapter=DefaultPinAdapter>
class AFShield {
private:
    // Interface
    class AFMotor {
    public:
        AFMotor(AFShield<PinAdapter> &shield, uint8_t number) : number(number), shield(shield) {};

        void forward();

        void backward();

        void stop();

        void release();

    private:
        uint8_t number;
        AFShield &shield;
    };

    class AFServo {
        AFServo(AFShield *shield, uint8_t number) : shield(shield) {};
        // TODO
//        void write(int angle);
//
//        void writeMicroseconds(int uS);
//
//        int read();

    private:
        AFShield *shield;
//        Servo servo;
        // TODO Servo.h
    };

public:
    AFShield() :
            pinLatch(MOTORLATCH),
            pinClk(MOTORCLK),
            pinEnable(MOTORENABLE),
            pinData(MOTORDATA),
            pinMotor1Speed(MOTOR1_PWM),
            pinMotor2Speed(MOTOR2_PWM),
            pinMotor3Speed(MOTOR3_PWM),
            pinMotor4Speed(MOTOR4_PWM),
            pinServo1Speed(SERVO1_PWM),
            pinServo2Speed(SERVO2_PWM),
            pinAdapter(new PinAdapter()) {
    }

    AFShield(PinAdapter *pinAdapter) :
            pinLatch(MOTORLATCH),
            pinClk(MOTORCLK),
            pinEnable(MOTORENABLE),
            pinData(MOTORDATA),
            pinMotor1Speed(MOTOR1_PWM),
            pinMotor2Speed(MOTOR2_PWM),
            pinMotor3Speed(MOTOR3_PWM),
            pinMotor4Speed(MOTOR4_PWM),
            pinServo1Speed(SERVO1_PWM),
            pinServo2Speed(SERVO2_PWM),
            pinAdapter(pinAdapter) {
    }

    AFShield(
            pin pinLatch,
            pin pinClk,
            pin pinEnable,
            pin pinData,
            pin pinMotor1Speed,
            pin pinMotor2Speed,
            pin pinMotor3Speed,
            pin pinMotor4Speed,
            pin pinServo1Speed,
            pin pinServo2Speed
    ) :
            pinLatch(pinLatch),
            pinClk(pinClk),
            pinEnable(pinEnable),
            pinData(pinData),
            pinMotor1Speed(pinMotor1Speed),
            pinMotor2Speed(pinMotor2Speed),
            pinMotor3Speed(pinMotor3Speed),
            pinMotor4Speed(pinMotor4Speed),
            pinServo1Speed(pinServo1Speed),
            pinServo2Speed(pinServo2Speed),
            pinAdapter(new PinAdapter()) {
    };

    AFShield(
            PinAdapter *pinAdapter,
            pin pinLatch,
            pin pinClk,
            pin pinEnable,
            pin pinData,
            pin pinMotor1Speed,
            pin pinMotor2Speed,
            pin pinMotor3Speed,
            pin pinMotor4Speed,
            pin pinServo1Speed,
            pin pinServo2Speed
    ) :
            pinLatch(pinLatch),
            pinClk(pinClk),
            pinEnable(pinEnable),
            pinData(pinData),
            pinMotor1Speed(pinMotor1Speed),
            pinMotor2Speed(pinMotor2Speed),
            pinMotor3Speed(pinMotor3Speed),
            pinMotor4Speed(pinMotor4Speed),
            pinServo1Speed(pinServo1Speed),
            pinServo2Speed(pinServo2Speed),
            pinAdapter(pinAdapter) {
    };

    const AFMotor & getMotor(uint8_t nMotor);

//    AFServo getServo(uint8_t nServo);

private:
    void shiftWrite(uint8_t output, uint8_t high_low);

    void motorOutput(uint8_t output, uint8_t high_low, int speed);

    void motor(uint8_t nMotor, uint8_t command, int speed);

    // wrapper of pin
    void _pinMode(uint8_t pin, uint8_t mode);

    void _digitalWrite(uint8_t pin, uint8_t val);

    void _analogWrite(uint8_t pin, uint8_t val);

    void _shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);

    pin pinLatch;
    pin pinClk;
    pin pinEnable;
    pin pinData;
    // Disabled
    pin pinMotor1Speed = 0;
    pin pinMotor2Speed = 0;
    pin pinMotor3Speed = 0;
    pin pinMotor4Speed = 0;
    pin pinServo1Speed = 0;
    pin pinServo2Speed = 0;

    // Cache
    AFMotor *motors[4] = {nullptr};
    AFServo *servos[2] = {nullptr};
    PinAdapter *pinAdapter;
};

#include "AFShield.tpp"


#endif //AFSHIELD_H
