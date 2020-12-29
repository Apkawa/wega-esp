#include <Arduino.h>
#include <web.h>

// TODO async server
WebServer server(80);

#include <wifi.h>
#include <ota.h>
#include <utils.h>

#include <Wire.h>

#include <AFShield.h>
#include <AdapterPin/MCP23017PinAdapter.h>
#include "Adafruit_MCP23017.h"

Adafruit_MCP23017 mcp;
MCP23017PinAdapter pinAdapter(mcp);

AFShield<MCP23017PinAdapter> shield1(
        &pinAdapter,
        4,
        0,
        2,
        3,
        0,
        0,
        0,
        1,
        0,
        0
);

AFShield<MCP23017PinAdapter> shield2(
        &pinAdapter,
        4,
        0,
        2,
        3,
        0,
        0,
        0,
        1,
        0,
        0
);


//Тип подключения дисплея: 1 - по шине I2C, 2 - десятиконтактное. Обязательно указывать ДО подключения библиотеки
//Если этого не сделать, при компиляции возникнет ошибка: "LCD type connect has not been declared"
#define _LCD_TYPE 1

#include <LCD_1602_RUS_ALL.h>

LCD_1602_RUS lcd(0x27, 16, 2); // Check I2C address of LCD, normally 0x27 or 0x3F

#include "HX711.h"

// GPIO19
const int LOADCELL_DOUT_PIN = 19;
// GPIO18
const int LOADCELL_SCK_PIN = 18;
HX711 scale;

// TODO config from ini/env
#define CALIBRATION_FACTOR_A 0.1f
#define CALIBRATION_FACTOR_B 2.0f

// Объем тары
#define CONCENTRATE_TARE_VOLUME 400
#define TARE_VOLUME 400

#define REPORT_URL "http://192.168.237.107/remote/mixerdb.php?r=1"

const uint serial = SERIAL_SPEED;

enum Side {
    A,
    B
};


struct PumpInfo {
    const typeof(shield1.getMotor(0)) *pump;
    const char *name;
    const Side side;
    // предварительная подкачка раствора из трубок, в мс по умолчанию - 5000 мс
    const uint16_t preload;
    float pumped;
    float plan;
};


// TODO pass from env
PumpInfo PUMPS[] = {
        {&shield1.getMotor(1), "Ca(NO3)2", A},
        {&shield1.getMotor(2), "KNO3", A},
        {&shield1.getMotor(3), "NH4NO3", A},
        {&shield1.getMotor(5), "MgSO4", B},
        {&shield2.getMotor(1), "KH2PO4", B},
        {&shield2.getMotor(2), "K2SO4", B},
        {&shield2.getMotor(3), "Micro 1000:1", B},
        {&shield2.getMotor(4), "B", B},
};

const uint8_t TOTAL_PUMPS = sizeof PUMPS / sizeof *PUMPS;


// Функции помп

void pumpStart(const PumpInfo *pump)  {
    pump->pump->forward();
}

void pumpStop(PumpInfo *pump) {
    pump->pump->stop();
}

void pumpReverse(PumpInfo *pump) {
    pump->pump->backward();
}


//Функция налива
float pumping(PumpInfo *pump) {
    float planWeight = pump->plan;
    auto name = pump->name;
    // Либо нечего наливать либо наливаем почти всю бутылку
    if (planWeight <= 0 || planWeight >= CONCENTRATE_TARE_VOLUME) {
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.print(":");
        lcd.print(formatFloat(planWeight, 2));
        lcd.setCursor(10, 0);
        lcd.print("SKIP...   ");
        delay(1000);
        return 0.0f;
    } else {
        //Продувка
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.print(" Reverse...");
        pumpReverse(pump);
        delay(30000);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.setCursor(0, 1);
        lcd.print(" Preload=");
        uint16_t preload = 5000;
        if (pump->preload > 0) {
            preload = pump->preload;
        }
        lcd.print(formatFloat(preload, 0));
        lcd.print("ms");

        pumpStart(pump);
        delay(preload);
        pumpStop(pump);

        //
        scale.tare(255);
        //
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.print(":");
        lcd.print(formatFloat(planWeight, 2));

        lcd.setCursor(10, 0);
        lcd.print("RUNING");

        float value = scale.get_units(64);
        float pumpedValue = value;
        uint accuratePumpDelay = 80;
        while (value < planWeight - 0.01) {
            lcd.setCursor(0, 1);
            lcd.print(value, 2);
            lcd.print(" (");
            lcd.print(100 - (planWeight - value) / planWeight * 100, 0);
            lcd.print("%) ");
            lcd.print(accuratePumpDelay, 0);
            lcd.print("ms     ");

            pumpStart(pump);
            if (value < (planWeight - 1.5)) {
                if (planWeight - value > 20) { delay(10000); } else { delay(2000); }
                pumpedValue = value;
                accuratePumpDelay = 80;
            } else {
                lcd.setCursor(10, 0);
                lcd.print("PCIS ");

                if (value - pumpedValue < 0.01) {
                    if (accuratePumpDelay < 80) {
                        accuratePumpDelay = accuratePumpDelay + 2;
                    }
                }
                if (value - pumpedValue > 0.01) {
                    if (accuratePumpDelay > 2) {
                        accuratePumpDelay = accuratePumpDelay - 2;
                    }
                }
                if (value - pumpedValue > 0.1) { accuratePumpDelay = 0; }

                pumpedValue = value;
                delay(accuratePumpDelay);
            }
            pumpStop(pump);
            delay(1000);
            value = scale.get_units(254);
        }
        pumpStop(pump);

        lcd.setCursor(0, 1);
        lcd.print(value, 2);
        lcd.print(" (");
        lcd.print(100 - (planWeight - value) / planWeight * 100, 2);
        lcd.print("%)      ");
        // Продувка
        pumpReverse(pump);
        delay(10000);
        pumpStop(pump);
        return value;
    }
}

void sendReport() {
    WiFiClient client;
    HTTPClient http;
    // TODO config
    String url = REPORT_URL;

    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);
        url += "&p" + pump_index + "=" + formatFloat(pump.plan, 3);
        url += "&v" + pump_index + "=" + formatFloat(pump.pumped, 3);
    }
    http.begin(client, url);
    http.GET();
    http.end();
    Serial.println(url);
}

void handleRoot() {
    String httpstr = "<meta>"
                     "<h1>Mixer</h1>"
                     "Plan: <br>";
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);
        httpstr += "P" + pump_index + " = " + formatFloat(pump.plan, 3) + " [" + pump.name + "] <br/>";
    }
    httpstr += "<br><br><a href=""plan1"">PLAN1 START</a>"
               "<form action='start' method='get'>";
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);
        auto pump_name = 'p' + pump_index;
        httpstr +=
                "<p>P" + pump_index + "= <input type='number' min='0' max='500'  name='" + pump_name + "' value='" +
                server.arg(pump_name) +
                "'/> " + pump.name + "</p>";
    }
    httpstr += "<p><input type='submit' value='Start'/></p></form>";
    server.send(200, "text/html", httpstr);
}


void handleStart() {
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = &PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);
        String pump_name = String('p' + pump_index);
        pump->plan = server.arg(pump_name).toFloat();
    }

    String httpstr = "<h1>Mixer start</h1>"
                     "Plan: <br>";
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);
        httpstr += "P" + pump_index + "=" + formatFloat(pump.plan, 3) + "<br/>";
    }
    server.send(200, "text/html", httpstr);

    scale.tare(255);
    // TODO защита от перелива
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = &PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);

        if (pump->side == A)
            scale.set_scale(CALIBRATION_FACTOR_A);
        else
            scale.set_scale(CALIBRATION_FACTOR_B);

        pump->pumped = pumping(pump);
    }

    sendReport();
}

void handleTest() {
    float dl = 30000;
    server.send(200, "text/html", "testing pump...");

    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);
        lcd.home();
        lcd.print("Pump " + pump_index + " Start");
        pumpStart(&pump);
        delay(dl);
        lcd.home();
        lcd.print("Pump " + pump_index + " Reverse       ");
        pumpReverse(&pump);
        delay(dl);
        lcd.home();
        lcd.print("Pump " + pump_index + " Stop      ");
        delay(1000);
        pumpStop(&pump);
        lcd.home();
    }
}

// TODO калибровка прелоада
void handlePumpPreloadCalibration() {
    String httpstr = "<meta>"
                     "<h1>Calibrate</h1>";

}

// TODO встроенная калибровка весов миксера
void handleCalibrationScale() {
    String httpstr = "<meta>"
                     "<h1>Calibrate</h1>";

    float calA = server.arg('a').toFloat();
    float calB = server.arg('b').toFloat();

    httpstr += "A=" + formatFloat(CALIBRATION_FACTOR_A, 3) + "<br>";
    httpstr += "B=" + formatFloat(CALIBRATION_FACTOR_B, 3) + "<br>";

    httpstr += "";

    scale.set_scale(calA);
    auto weightA = scale.get_value(64);

    scale.set_scale(calB);
    auto weightB = scale.get_value(64);

    httpstr += "Weight A=" + formatFloat(weightA, 2) + "<br>";
    httpstr += "Weight B=" + formatFloat(weightB, 2) + "<br>";

    // TODO form

    server.send(200, "text/html", httpstr)

}

void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    lcd.setCursor(10, 0);
    lcd.print("Booting...");

    wifi::setup();
    Ota::setup();

    server.on("/", handleRoot);
    server.on("/start", handleStart);
    server.on("/test", handleTest);
    server.begin();

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.power_up();

    lcd.setCursor(10, 0);
    lcd.print("Start");
}

void loop() {
    server.handleClient();
    Ota::loop();
}


