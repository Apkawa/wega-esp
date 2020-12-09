#include <Arduino.h>
#include <web.h>

WebServer server(80);


#include <wifi.h>
#include <ota.h>

const uint serial = SERIAL_SPEED;

// I2C esp32
// GPIO21 - SDA
// GPIO22 - SCL

//Тип подключения дисплея: 1 - по шине I2C, 2 - десятиконтактное. Обязательно указывать ДО подключения библиотеки
//Если этого не сделать, при компиляции возникнет ошибка: "LCD type connect has not been declared"
#define _LCD_TYPE 1
#include <Wire.h>
#include <LCD_1602_RUS_ALL.h>

LCD_1602_RUS <LiquidCrystal_I2C>  lcd(0x27, 16, 2); // Check I2C address of LCD, normally 0x27 or 0x3F


void handleRoot() {
    String httpstr = "<meta http-equiv='refresh' content='10'>";
    httpstr += "Hello World!<br>";
    server.send(200, "text/html", httpstr);
}

void setup() {
    Serial.begin(serial);
    Serial.println("Booting");

    wifi::setup();
    Ota::setup();

    server.on("/", handleRoot);
    server.begin();

    lcd.init();                      // initialize the lcd
//    // Print a message to the LCD.
    lcd.backlight();
    lcd.setCursor(2, 0);
    lcd.print(L"Теперь можно");
    lcd.setCursor(0, 1);
    lcd.print(L"    cпать   ");
}

void loop() {
    server.handleClient();
    Ota::loop();
}


