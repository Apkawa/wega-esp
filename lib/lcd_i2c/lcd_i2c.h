#ifndef WEGA_ESP32_LCD_I2C_H
#define WEGA_ESP32_LCD_I2C_H


//Тип подключения дисплея: 1 - по шине I2C, 2 - десятиконтактное. Обязательно указывать ДО подключения библиотеки
//Если этого не сделать, при компиляции возникнет ошибка: "LCD type connect has not been declared"

#define _LCD_TYPE 1
#include <Wire.h>
#include <LCD_1602_RUS_ALL.h>

class lcd_i2c {
public:
    lcd_i2c();
    void init();
    const LCD_1602_RUS <LiquidCrystal_I2C>* i();
private:
    LCD_1602_RUS <LiquidCrystal_I2C> _lcd;
};


#endif //WEGA_ESP32_LCD_I2C_H
