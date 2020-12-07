#include "lcd_i2c.h"

class lcd_i2c {
public:
    lcd_i2c() {
        _lcd = LCD_1602_RUS <LiquidCrystal_I2C>  lcd(0x27, 16, 2); // Check I2C address of LCD, normally 0x27 or 0x3F
    }
    void init() {
        _lcd.init()
    }
    i() {
        return const &_lcd;
    }
private:
    LCD_1602_RUS <LiquidCrystal_I2C> _lcd;
};
