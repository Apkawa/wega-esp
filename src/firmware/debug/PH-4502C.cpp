// Пример чтения с pH сенсора
#include <Arduino.h>
#include <wifi.h>
#include <ota.h>

#define DIRECT 1

#if DIRECT = 0
// ADS1115 for pH
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads;
//ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
#endif


float AnalogReadMid(int port, long count) {
    server.handleClient();
    ArduinoOTA.handle();

    long n = 0;
    double sensorValue = 0;
    while (n < count) {
        n++;
        server.handleClient();
        ArduinoOTA.handle();
        sensorValue = (analogRead(port)) + sensorValue;
    }
    return sensorValue / n;

}

// Функция усреднения значений измерения напряжения на ADS1117 между портами 0 и 1
float adsdiff01(long count) {
    ArduinoOTA.handle();

    long n=0;
    double sensorValue=0;
    while ( n< count){
        n++;
        server.handleClient();
        ArduinoOTA.handle();
        sensorValue = (ads.readADC_Differential_0_1())+sensorValue;
    }
    return sensorValue/n;

}




void loop() {
    Ota::loop();

    // TODO оформить пример
    // read the input on analog pin 0:
    float pHraw;


#if DIRECT
    // читаем данные напрямую с pH shield
    pHraw = AnalogReadMid(33, 5);
#else
    // В случае использования развязки - читаем через интерфейс i2c
    pHraw = adsdiff01(5000);
#endif

    // Калибровка pH
    float x1 = 3730;// Показания сенсора при pH 4.01
    float y1 = 4.01;
    float x2 = 2977;// Показания сенсора при pH 6.86
    float y2 = 6.86;

    float a = (-x2 * y1 + y2 * x1) / (-x2 + x1);
    float b = (-y2 + y1) / (-x2 + x1);

    pH = a + b * pHraw;

    Serial.print("pHraw: ");
    Serial.print(pHraw);
    Serial.print(" pH: ");
    Serial.println(pH);

}


