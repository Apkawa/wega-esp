#include <Arduino.h>

#include <wifi.h>
#include <ota.h>
#include <utils.h>

const uint serial = SERIAL_SPEED;

// TODO move to lib
//
/*
 * Функция прямого опроса ультразвукового датчика возврат в микросекундах эха
 * triggerPin - Digital I/O Pin
 * echoPin - Digital I/O Pin
 */
float ultraSonic(int triggerPin, int echoPin, float temperature, float humidity, long iterations) {
    long count = 0;
    ulong total_time = 0;
    while (count < iterations) {
        count++;
        pinMode(triggerPin, OUTPUT);
        pinMode(echoPin, INPUT);

        digitalWrite(triggerPin, 1);
        delayMicroseconds(10);
        digitalWrite(triggerPin, 0);


        long n = 0;
        long limit = 100000;

        ulong start_time, end_time;
        while (n < limit) {
            n++;
            if (digitalRead(echoPin) == 1) {
                start_time = micros();
                long z = 0;
                while (digitalRead(echoPin) == 1 and z < 20000) {
                    z++;
                    end_time = micros();
                }
                n = limit;
                delay(200);
            }

        }

        total_time = total_time + (end_time - start_time);

    }
    double vSound = 20.046796 * sqrt(273.15 + temperature);
//    double vSound = 331.3+(0.606*temperature)+(0.0124*humidity);

    double avg_time = float(total_time) / float(count);
    Serial.print("Debug: ");
    Serial.print(formatFloat(float(avg_time), 5));
    Serial.print(" ");
    Serial.println(formatFloat(float(vSound), 4));
    return float((vSound / 10000) * ( avg_time / 2));
}


void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    wifi::setup();
    Ota::setup();

}

void loop() {
    Ota::loop();

    Serial.print("ultraSonic: ");
    Serial.println(formatFloat(ultraSonic(16, 17, 25, 43, 5), 5));
}


