#include <Arduino.h>
#include <web.h>

// TODO async server
WebServer server(80);

#include <wifi.h>
#include <ota.h>
#include <utils.h>

// Насосы
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

// Экран
//Тип подключения дисплея: 1 - по шине I2C, 2 - десятиконтактное. Обязательно указывать ДО подключения библиотеки
//Если этого не сделать, при компиляции возникнет ошибка: "LCD type connect has not been declared"
#define _LCD_TYPE 1

#include <LCD_1602_RUS_ALL.h>

LCD_1602_RUS lcd(0x27, 16, 2); // Check I2C address of LCD, normally 0x27 or 0x3F

// Весы
#include "HX711.h"

#ifndef LOADCELL_DOUT_PIN
// GPIO19
#define LOADCELL_DOUT_PIN 19
#endif

#ifndef LOADCELL_SCK_PIN
// GPIO18
#define LOADCELL_SCK_PIN 18
#endif

HX711 scale;

// TODO config from ini/env
#ifndef CALIBRATION_FACTOR_A
#define CALIBRATION_FACTOR_A 0.1f
#endif
#ifndef CALIBRATION_FACTOR_B
#define CALIBRATION_FACTOR_B 2.0f
#endif

// Объем тары
#define CONCENTRATE_TARE_VOLUME 400
#define TARE_VOLUME 400

#define DEFAULT_PRELOAD 5000



enum Side {
    A,
    B
};


struct PumpInfo {
    const typeof(shield1.getMotor(0)) *pump;
    const char *name;
    const Side side;
    // предварительная подкачка раствора из трубок, в мс по умолчанию - 5000 мс
    uint16_t preload;
    float pumped;
    float plan;
};


// TODO pass from env
PumpInfo PUMPS[] = {
        {&shield1.getMotor(1), "Ca(NO3)2",     A},
        {&shield1.getMotor(2), "KNO3",         A},
        {&shield1.getMotor(3), "NH4NO3",       A},
        {&shield1.getMotor(5), "MgSO4",        B},
        {&shield2.getMotor(1), "KH2PO4",       B},
        {&shield2.getMotor(2), "K2SO4",        B},
        {&shield2.getMotor(3), "Micro 1000:1", B},
        {&shield2.getMotor(4), "B",            B},
};

const uint8_t TOTAL_PUMPS = sizeof PUMPS / sizeof *PUMPS;


// Функции помп

void pumpStart(const PumpInfo *pump) {
    pump->pump->forward();
}

void pumpStop(const PumpInfo *pump) {
    pump->pump->stop();
}

void pumpReverse(const PumpInfo *pump) {
    pump->pump->backward();
}


//Функция налива
float pumping(const PumpInfo *pump) {
    float planWeight = pump->plan;
    auto name = pump->name;
    // Либо нечего наливать либо наливаем почти всю бутылку
    if (planWeight <= 0 || planWeight >= CONCENTRATE_TARE_VOLUME) {
        // todo lcd.printf
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
        uint16_t preload = DEFAULT_PRELOAD;
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

#ifdef REPORT_URL
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
#endif

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
#ifdef REPORT_URL
    sendReport();
#endif
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

uint16_t findPumpPreload(const PumpInfo *pump) {
    pumpReverse(pump);
    delay(30000);
    pumpStop(pump);
    uint16_t time = 0;

    pumpStart(pump);
    auto weight = scale.get_units(10);

    while (abs(weight - scale.get_units(10)) < 0.1) {
        time += 100;
        delay(100);
        if (time > 30000) {
            break;
        }
    }
    pumpStop(pump);
    pumpReverse(pump);
    delay(30000);
    pumpStop(pump);
    return time;
}

void handlePumpPreloadCalibration() {
    String httpstr = "<meta>"
                     "<h1>Preload calibrate</h1>";


    if (server.method() == HTTP_POST) {
        auto pump_index = server.arg("pump").toInt();
        if (pump_index >= TOTAL_PUMPS) {

        } else {
            if (pump_index == -1) {
                for (int i = 0; i < TOTAL_PUMPS; ++i) {
                    auto pump = PUMPS[i];
                    pump.preload = findPumpPreload(&pump);
                }
            } else {
                auto pump = PUMPS[pump_index];
                pump.preload = findPumpPreload(&pump);
            }
        }
    }
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);
        httpstr += "P" + pump_index + "=" + formatFloat(pump.preload || DEFAULT_PRELOAD, 0) + "</br>";
    }

    httpstr += "<br><b> Calibration start </b>"
               "<form action='.' method='get'>";
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump_index = formatFloat(i + 1, 0);
        httpstr += "<button type='submit' name='pump' value='"
                   + pump_index + "'>Pump No. " + pump_index + "</button></br>";
    }
    httpstr += "</form>";

    server.send(200, "text/html", httpstr);

}

float findCalibrationFactor(float calibratedWeight) {
    float threshold = 0.01;
    int iterations = 100;
    float weight, factor, diff, step, sign;
    factor = 2000;
    diff = 1000;
    while (iterations > 0) {
        iterations--;

        weight = scale.get_value(diff < 1 ? 64 : 10);
        diff = calibratedWeight - weight;

        sign = +1;
        if (diff < 0) {
            sign = -1;
        }
        diff = abs(diff);
        if (diff <= threshold) {
            break;
        }
        step = 100;
        if (diff < 100) {
            step = 10;
        } else if (diff < 10) {
            step = 0.1;
        } else if (diff < 1) {
            step = 0.01;
        }
        step *= sign;
        factor += step;
        scale.set_scale(factor);
    }
    return factor;
}

void handleCalibrationScale() {
    String httpstr = "<meta>"
                     "<h1>Calibrate</h1>";

    httpstr += "A=" + formatFloat(CALIBRATION_FACTOR_A, 3) + "<br>";
    httpstr += "B=" + formatFloat(CALIBRATION_FACTOR_B, 3) + "<br>";

    httpstr += "";

    float calA = server.arg("a").toFloat();
    float calB = server.arg("b").toFloat();
    bool autoCalA = server.arg("auto").toInt() == 1;
    bool autoCalB = server.arg("auto").toInt() == 2;
    float calibratedWeight = server.arg("weight").toFloat();

    if (autoCalA) {
        calA = findCalibrationFactor(calibratedWeight);
    } else if (autoCalB) {
        calB = findCalibrationFactor(calibratedWeight);
    }

    scale.set_scale(calA);
    auto weightA = scale.get_value(64);

    scale.set_scale(calB);
    auto weightB = scale.get_value(64);

    httpstr += "Weight A=" + formatFloat(weightA, 2) + "<br>";
    httpstr += "Weight B=" + formatFloat(weightB, 2) + "<br>";

    httpstr += "<br><b> Calibration start </b>"
               "<form action='.' method='get'>";
    httpstr +=
            "<p> Calibrated weight: <input type='number' min='0' max='1000' step='0.01'  name='weight' value='"
            + formatFloat(calibratedWeight, 2)
            + "'/></p>"
              "<p> Scale A: <input type='number'  step='0.01'  name='a' value='"
            +
            formatFloat(calA, 2)
            + "'/></p>"
              "<p> Scale B: <input type='number'  step='0.01'  name='b' value='"
            +
            formatFloat(calB, 2)
            + "'/></p>"
              "";

    httpstr += "<p>"
               "<button type='submit'>Check</button>"
               "<button type='submit' name='auto' value='1'>Auto A</button>"
               "<button type='submit' name='auto' value='2'>Auto B</button>"
               "</p>";
    httpstr += "</form>";

    server.send(200, "text/html", httpstr);

}

void setup() {
    Serial.begin(SERIAL_SPEED);
    Serial.println("Booting");

    lcd.setCursor(10, 0);
    lcd.print("Booting...");

    wifi::setup();
    Ota::setup();

    server.on("/", handleRoot);
    server.on("/start", handleStart);
    server.on("/test", handleTest);
    server.on("/scale", handleCalibrationScale);
    server.on("/preload", handlePumpPreloadCalibration);
    server.begin();

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.power_up();

    lcd.setCursor(10, 0);
    lcd.print("Start");
    Serial.println("Start");
}

void loop() {
    server.handleClient();
    Ota::loop();

    lcd.setCursor(0, 1);
    float weight = scale.get_units(16);
    float filtered_weight = kalman_filter(weight);
    lcd.print(filtered_weight, 2);
    lcd.print("         ");
    lcd.setCursor(10, 0);
    lcd.print("Ready  ");

}


