#include <wifi.h>
#include <ota.h>
#include <utils.h>

const uint serial = SERIAL_SPEED;

// https://wiki.iarduino.ru/page/hx_711_with_tenzo/
// http://arduinolab.pw/index.php/2017/07/03/vesy-na-arduino-i-kalibrovka-tenzodatchika-s-hx711/
#include "HX711.h"

// GPIO19
const int LOADCELL_DOUT_PIN = 19;
// GPIO18
const int LOADCELL_SCK_PIN = 18;

HX711 scale;

float calibration_factor = -3.7;


// Todo make helpers
void calibrationScale() {
    scale.set_scale(calibration_factor); //Adjust to this calibration factor

    Serial.print("Reading: ");
    float units = scale.get_units();
    if (units < 0)
    {
        units = 0.00;
    }
    float grams = units * 0.035274f;
    Serial.print(grams);
    Serial.print(" grams");
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();

    if(Serial.available())
    {
        char temp = Serial.read();
        if(temp == '+' || temp == 'w')
            calibration_factor += 1;
        else if(temp == '-' || temp == 'a')
            calibration_factor -= 1;
        else if (temp == 'e')
            calibration_factor += 0.1;
        else if (temp == 'q')
            calibration_factor -= 0.1;
        else if (temp == 'W')
            calibration_factor += 100;
        else if (temp == 'A')
            calibration_factor -= 100;
        else if (temp == 'E')
            calibration_factor += 10;
        else if (temp == 'Q')
            calibration_factor -= 10;
    }
}

void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    wifi::setup();
    Ota::setup();

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(calibration_factor); //A side
    scale.power_up();

    delay (3000);
    scale.tare(255);

}

void loop() {
    Ota::loop();

    calibrationScale();
}




