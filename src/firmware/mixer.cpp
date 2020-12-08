#include <Arduino.h>
#include <web.h>

WebServer server(80);

#include <wifi.h>
#include <ota.h>
#include <utils.h>

#include <Wire.h>
#include "Adafruit_MCP23017.h"
Adafruit_MCP23017 mcp;

//Тип подключения дисплея: 1 - по шине I2C, 2 - десятиконтактное. Обязательно указывать ДО подключения библиотеки
//Если этого не сделать, при компиляции возникнет ошибка: "LCD type connect has not been declared"
#define _LCD_TYPE 1
#include <LCD_1602_RUS_ALL.h>

LCD_1602_RUS<LiquidCrystal_I2C> lcd(0x27, 16, 2); // Check I2C address of LCD, normally 0x27 or 0x3F

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
    const uint8_t pinDirect;
    const uint8_t pinReverse;
    const char *name;
    const Side side;
    float pumped;
    float plan;
};

// TODO pass from env
PumpInfo PUMPS[] = {
        {11, 3,  "Ca(NO3)2",     A},
        {2,  10, "KNO3",         A},
        {1,  9,  "NH4NO3",       A},
        {0,  8,  "MgSO4",        B},
        {7,  15, "KH2PO4",       B},
        {6,  14, "K2SO4",        B},
        {5,  13, "Micro 1000:1", B},
        {4,  12, "B",            B},
};

const uint8_t TOTAL_PUMPS = sizeof PUMPS / sizeof *PUMPS;

// Функции помп

void pumpStart(uint8_t pinDirect, uint8_t pinReverse) {
    mcp.begin();
    mcp.pinMode(pinDirect, OUTPUT);
    mcp.pinMode(pinReverse, OUTPUT);
    mcp.digitalWrite(pinDirect, HIGH);
    mcp.digitalWrite(pinReverse, LOW);
}

void pumpStop(uint8_t pinDirect, uint8_t pinReverse) {
    mcp.begin();
    mcp.pinMode(pinDirect, OUTPUT);
    mcp.pinMode(pinReverse, OUTPUT);
    mcp.digitalWrite(pinDirect, LOW);
    mcp.digitalWrite(pinReverse, LOW);
}

void pumpReverse(uint8_t pinDirect, uint8_t pinReverse) {
    mcp.begin();
    mcp.pinMode(pinDirect, OUTPUT);
    mcp.pinMode(pinReverse, OUTPUT);
    mcp.digitalWrite(pinDirect, LOW);
    mcp.digitalWrite(pinReverse, HIGH);
}

//Функция налива
float pumping(PumpInfo *pump, uint preload) {
    uint8_t pinDirect = pump->pinDirect;
    uint8_t pinReverse = pump->pinReverse;
    float planWeight = pump->plan;
    auto name = pump->name;
    //
    if (planWeight <= 0 || planWeight >= CONCENTRATE_TARE_VOLUME) {
        // Либо нечего наливать либо наливаем почти всю бутылку
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.print(":");
        lcd.print(formatFloat(planWeight, 2));
        lcd.setCursor(10, 0);
        lcd.print("SKIP..   ");
        delay(1000);
        return 0.0f;
    } else {
        //Продувка
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.print(" Reverse...");
        pumpReverse(pinDirect, pinReverse);
        delay(30000);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(name);
        lcd.setCursor(0, 1);
        lcd.print(" Preload=");
        lcd.print(formatFloat(preload, 0));
        lcd.print("ms");
        scale.tare(255);

        pumpStart(pinDirect, pinReverse);
        delay(preload);
        pumpStop(pinDirect, pinReverse);

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

            pumpStart(pinDirect, pinReverse);
            if (value < (planWeight - 1.5)) {
                if (planWeight - value > 20) { delay(10000); } else { delay(2000); }
                pumpedValue = value;
                accuratePumpDelay = 80;
            } else {
                lcd.setCursor(10, 0);
                lcd.print("PCIS ");

                if (value - pumpedValue < 0.01) { if (accuratePumpDelay < 80) { accuratePumpDelay = accuratePumpDelay + 2; }}
                if (value - pumpedValue > 0.01) { if (accuratePumpDelay > 2) { accuratePumpDelay = accuratePumpDelay - 2; }}
                if (value - pumpedValue > 0.1) { accuratePumpDelay = 0; }

                pumpedValue = value;
                delay(accuratePumpDelay);

            }
            //mcp.digitalWrite(npump, LOW);
            pumpStop(pinDirect, pinReverse);
            delay(100);

            value = scale.get_units(254);
        }
        pumpStop(pinDirect, pinReverse);

        lcd.setCursor(0, 1);
        lcd.print(value, 2);
        lcd.print(" (");
        lcd.print(100 - (planWeight - value) / planWeight * 100, 2);
        lcd.print("%)      ");
        pumpReverse(pinDirect, pinReverse);
        delay(10000);
        pumpStop(pinDirect, pinReverse);
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

    // TODO защита от перелива
    for (int i = 0; i < TOTAL_PUMPS; ++i) {
        auto pump = &PUMPS[i];
        auto pump_index = formatFloat(i + 1, 0);

        if (pump->side == A)
            scale.set_scale(CALIBRATION_FACTOR_A);
        else
            scale.set_scale(CALIBRATION_FACTOR_B);

        pump->pumped = pumping(pump, 5000);
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
        pumpStart(pump.pinDirect, pump.pinReverse);
        delay(dl);
        lcd.home();
        lcd.print("Pump " + pump_index + " Reverse       ");
        pumpReverse(pump.pinDirect, pump.pinReverse);
        delay(dl);
        lcd.home();
        lcd.print("Pump " + pump_index + " Stop      ");
        delay(1000);
        pumpStop(pump.pinDirect, pump.pinReverse);
        lcd.home();
    }
}


void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    wifi::setup();
    Ota::setup();

    server.on("/", handleRoot);
    server.on("/start", handleStart);
    server.on("/test", handleTest);
    server.begin();
}

void loop() {
    server.handleClient();
    Ota::loop();
}


