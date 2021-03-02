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
        11, // B3
        8, // B0
        2, // A2
        10, // B2
        3, // A3
        0, // A0
        9, // B1
        1, // A1
        0,
        0
);

AFShield<MCP23017PinAdapter> shield2(
        &pinAdapter,
        15, // B7
        12, // B4
        6, // A6
        14, // B6
        7, // A7
        4, // A4
        13, // B5
        5, // A5
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
#define LOADCELL_DOUT_PIN 25 // D5 or GPIO 14
#endif

#ifndef LOADCELL_SCK_PIN
#define LOADCELL_SCK_PIN 26 // D6 or GPIO 12
#endif

HX711 scale;

// TODO config from ini/env
#ifndef CALIBRATION_FACTOR_A
#define CALIBRATION_FACTOR_A 1844.7990
#endif
#ifndef CALIBRATION_FACTOR_B
#define CALIBRATION_FACTOR_B 1843.6873
#endif

// Объем тары
#define CONCENTRATE_TARE_VOLUME 400
#define TARE_VOLUME 400

// Скорость налива
#define PUMP_SPEED 0.5
#define ACCURATE_PUMP_DELAY 0
#define DEFAULT_PRELOAD 3000
#define REVERSE_DELAY (DEFAULT_PRELOAD * 2)

enum Status {
    READY,
    PUMP,
};

enum Side {
    A,
    B
};

struct PumpInfo {
    decltype(shield1.getMotor(0)) pump;
    const char *name;
    const Side side;
    // предварительная подкачка раствора из трубок, в мс по умолчанию - 5000 мс
    uint16_t preload;
    float pumped;
    float plan;
};

Status status = READY;
uint8_t currentPumpPumped = -1;
float filteredWeight;

// TODO pass from env
PumpInfo PUMPS[] = {
        {shield1.getMotor(1), "Ca(NO3)2",     A}, // 1
        {shield1.getMotor(2), "KNO3",         A}, // 2
        {shield1.getMotor(3), "NH4NO3",       A}, // 3
        {shield1.getMotor(4), "MgSO4",        B}, // 4
        {shield2.getMotor(1), "KH2PO4",       B}, // 5
        {shield2.getMotor(2), "K2SO4",        B}, // 6
        {shield2.getMotor(3), "Micro 1000:1", B}, // 7
        {shield2.getMotor(4), "Fe",           B}, // 8
};

const uint8_t TOTAL_PUMPS = sizeof PUMPS / sizeof *PUMPS;



// Функции помп

void pumpStart(const PumpInfo *pump) {
    pump->pump.forward();
}

void pumpStop(const PumpInfo *pump) {
    pump->pump.stop();
}

void pumpReverse(const PumpInfo *pump) {
    pump->pump.backward();
}


//Функция налива
float pumping(PumpInfo *pump) {
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
        delay(REVERSE_DELAY);

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
        //
        scale.tare(255);
        //

        pumpStart(pump);
        delay(preload);
        pumpStop(pump);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.print(":");
        lcd.print(formatFloat(planWeight, 2));

        lcd.setCursor(10, 0);
        lcd.print("RUN");

        float value = scale.get_units(64);
        float pumpedValue = value;
        uint accuratePumpDelay = ACCURATE_PUMP_DELAY;
        while (value < planWeight - 0.01) {
            pump->pumped = pumpedValue;
            server.handleClient();

            lcd.setCursor(0, 1);
            lcd.print(value, 2);
            lcd.print(" (");
            lcd.print(100 - (planWeight - value) / planWeight * 100, 0);
            lcd.print("%) ");
            lcd.print(formatFloat(accuratePumpDelay, 0));
            lcd.print("ms     ");

            pumpStart(pump);
            if (value < (planWeight - 1.5)) {
                if (planWeight - value > 20) {
                    delay(10000 * PUMP_SPEED);
                } else {
                    delay(2000 * PUMP_SPEED);
                }
                pumpedValue = value;
                accuratePumpDelay = ACCURATE_PUMP_DELAY;
            } else {
                lcd.setCursor(10, 0);
                lcd.print("PCIS ");

                if (value - pumpedValue < 0.01) {
                    if (accuratePumpDelay < ACCURATE_PUMP_DELAY) {
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
                if (accuratePumpDelay > 0) {
                    delay(accuratePumpDelay);
                }
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
                     "<br>";
    if (status == PUMP && currentPumpPumped >= 0) {
        auto pump = PUMPS[currentPumpPumped];
        httpstr += "<meta http-equiv='refresh' content='10'>";
        httpstr += "<p>Pumped: P"
                + formatFloat(currentPumpPumped + 1, 0)
                + " (" + pump.name + ") = "
                + formatFloat(pump.pumped, 2) + "g </p>";
    } else {
        httpstr += "<p>Ready</p>";
    }

    httpstr += "<form action='start' method='get'>";
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);
        auto pump_name = 'p' + pump_index;
        auto pump_percent = (pump.pumped - pump.plan) / pump.plan * 100;
        auto pump_value = server.hasArg(pump_name) ? server.arg(pump_name).toFloat() : pump.plan;
        httpstr +=
                "<p>P" + pump_index + "= <input type='number' min='0' max='500' step='0.01' name='" + pump_name + "' value='" +
                formatFloat(pump_value, 2) +
                "'/> " + pump.name + " "
                 + (pump.plan > 0 ? formatFloat(pump.pumped, 2) + "g (" + pump_percent + "%)" : "" )
                 + "</p>";
    }
    if (status == READY) {

        httpstr += "<p><input type='submit' value='Start'/>"
                 "<input type='button' class='button' onclick=\"window.location.href = 'scale';\" value='Scale'/>"
                 "<input type='button' class='button' onclick=\"window.location.href = 'calibrate';\" value='Calibrate'/>"
                 "</p></form>" ;
    }

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

    if (status == PUMP)  {
        return;
    }
    status = PUMP;
    scale.tare(255);
    // TODO защита от перелива
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = &PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);

        if (pump->side == A)
            scale.set_scale(CALIBRATION_FACTOR_A);
        else
            scale.set_scale(CALIBRATION_FACTOR_B);
        currentPumpPumped = i;
        pump->pumped = pumping(pump);
    }
#ifdef REPORT_URL
    sendReport();
#endif
    lcd.clear();
    status = READY;
}

void handleTest() {
    float dl = 5000;
    server.send(200, "text/html", "testing pump...");

    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);
        lcd.setCursor(0, 1);
        lcd.print("Pump" + pump_index + " " + pump.name);
        lcd.home();
        lcd.print("Start     ");
        pumpStart(&pump);
        delay(dl);
        lcd.home();
        lcd.print("Reverse       ");
        pumpReverse(&pump);
        delay(dl);
        lcd.home();
        lcd.print("Stop      ");
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
        httpstr += "P" + pump_index + "=" + formatFloat(pump.preload > 0 ? pump.preload : DEFAULT_PRELOAD, 0) + "</br>";
    }

    httpstr += "<br><b> Calibration start </b>"
               "<form action='/preload' method='post'>";
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump_index = formatFloat(i + 1, 0);
        httpstr += "<button type='submit' name='pump' value='"
                   + pump_index + "'>Pump No. " + pump_index + "</button></br>";
    }
    httpstr += "</form>";

    server.send(200, "text/html", httpstr);

}

void handleTare() {
    scale.tare(255);
    String message = "<script language='JavaScript' type='text/javascript'>setTimeout('window.history.go(-1)',0);</script>";
    message += "<input type='button' class='button' onclick='history.back();' value='back'/>";

    server.send(200, "text/html", message);

}

void handleDisplayScale (){
    float raw = scale.read_average(255);
    String message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";
    message += "<meta http-equiv='refresh' content='5'>";
    message += "<h3>Current weight = " + formatFloat(filteredWeight,2) + "</h3>";
    message += "RAW = " + formatFloat(raw,0);
    message += "<p><input type='button' class='button' onclick=\"window.location.href = 'tare';\" value='Set to ZERO'/>  ";
    message += "<input type='button' class='button' onclick=\"window.location.href = '/';\" value='Home'/>";
    message += "</p>";

    server.send(200, "text/html", message);

}

void handleCalibrationScale() {
    float raw = scale.read_average(255);
    String message = "<head><link rel='stylesheet' type='text/css' href='style.css'></head>";
    message += "Calibrate (calculate scale_calibration value)";
    message += "<h1>Current RAW = " + formatFloat(raw, 0) + "</h1>";
    scale.set_scale(CALIBRATION_FACTOR_A);
    message += "<br><h2>Current Value for point A = " + formatFloat(scale.get_units(128), 2) + "g</h2>";
    scale.set_scale(CALIBRATION_FACTOR_A);
    message += "<br><h2>Current Value for point B = " + formatFloat(scale.get_units(128), 2) + "g</h2>";
    message += "<br>Current scale_calibration_A = " + formatFloat(CALIBRATION_FACTOR_A, 4);
    message += "<br>Current scale_calibration_B = " + formatFloat(CALIBRATION_FACTOR_B, 4);
    message += "<form action='' method='get'>";
    message += "<p>RAW on Zero <input type='text' name='x1' value='" + server.arg("x1") + "'/></p>";
    message += "<p>RAW value with load <input type='text' name='x2' value='" + server.arg("x2") + "'/></p>";
    message += "<p>Value with load (gramm) <input type='text' name='s2' value='" + server.arg("s2") + "'/></p>";
    message += "<p><input type='submit' class='button' value='Submit'/>  ";
    message += "<input type='button' class='button' onclick=\"window.location.href = 'tare';\" value='Set to ZERO'/>  ";
    message += "<button class='button'  onClick='window.location.reload();'>Refresh</button>  ";
    message += "<input class='button' type='button' onclick=\"window.location.href = '/';\" value='Home'/>";
    message += "</p>";

    float x1 = server.arg("x1").toFloat();
    float x2 = server.arg("x2").toFloat();
    float s2 = server.arg("s2").toFloat();
    float k, y, s;

    if (s2 != 0) {
        k = -(x1 - x2) / s2;
        message += "<br> scale_calibration = <b>" + formatFloat(k, 4) + "</b> copy and paste to your sketch";
    }

    if (x1 > 0 and x2 > 0) {
        y = -(s2 * x1) / (x1 - x2);
        message += "<br>Calculate preloaded weight = " + formatFloat(y, 2) + "g";
    }

    if (s2 != 0) {
        s = raw * (1 / k) - y;
        message += "<br>Calculate weight = " + formatFloat(s, 2) + "g";
    }

    server.send(200, "text/html", message);
}


void setup() {
    Serial.begin(SERIAL_SPEED);
    Serial.println("Booting");
    lcd.init();
    lcd.backlight();
    lcd.setCursor(10, 0);
    lcd.print("Booting...");

    wifi::setup();
    Ota::setup();

    mcp.begin();

    server.on("/", handleRoot);
    server.on("/start", handleStart);
    server.on("/test", handleTest);
    server.on("/calibrate", handleCalibrationScale);
    server.on("/scale", handleDisplayScale);
    server.on("/tare", handleTare);
    server.on("/preload", handlePumpPreloadCalibration);
    server.begin();

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(CALIBRATION_FACTOR_A); //A side
    scale.power_up();


    lcd.setCursor(10, 0);
    lcd.print("Start");
    Serial.println("Start");

    server.handleClient();
    Ota::loop();

    delay(5000);
    if (scale.wait_ready_timeout()) {
        scale.tare(10);
    }
    Serial.println("Ready");
}

void loop() {
    server.handleClient();
    Ota::loop();

    lcd.setCursor(0, 1);
    if (scale.wait_ready_timeout()) {
        float weight = scale.get_units(16);
        filteredWeight = kalman_filter(weight);
        lcd.print(filteredWeight, 2);
        Serial.println(filteredWeight);
    } else {
        lcd.print("SCALE ERR");
    }
    lcd.print("         ");
    lcd.setCursor(10, 0);
    lcd.print("Ready  ");
    lcd.setCursor(0, 0);
}


