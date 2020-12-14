#include <Arduino.h>

// TODO зачем нужны?
float po;
float ap_raw;
float an_raw;

//Функция замера электропроводности в RAW
/*
 *
    d1 = 10;  // Цифровой порт соединенный с аналоговым портом и электродом измерения
    d2 = 11;  // Цифровой порт соединенный с электродом противофазы
    a1 = 0;   // Аналоговый порт измерения
    n = 50000;// Колличество измерений для усреднения
 */
float getConductivity(int d1, int d2, int a1, long iterations) {
    // Глобальные переменные

    pinMode(d1, OUTPUT);
    pinMode(d2, OUTPUT);
    long var = 1;
    unsigned long ap = 0;
    unsigned long an = 0;
    while (var < iterations) {

        digitalWrite(d2, HIGH);
        ap = 0 + analogRead(a1) + ap;
        digitalWrite(d2, LOW);

        digitalWrite(d1, HIGH);
        an = 1023 - analogRead(a1) + an;
        digitalWrite(d1, LOW);
        var++;
    }
    pinMode(d1, INPUT);
    pinMode(d2, INPUT);

    // TODO понять зачем это нужно
    // Усреднение АЦП при положительной фазе
    ap_raw = (float) ap / var;
    // Усреднение АЦП при отрицательной фазе
    an_raw = (float) an / var;
    // Расчет поляризации раствора
    po = ap_raw - an_raw;
    // Исключение влияние поляризаци
    // FIXME Бага? po не используется
    return (((float) ap + (float) an) / var / 2);

}

/* Функция калибровки
 *
 * x1  Показания АЦП при нижнем значении ЕС
 * ec1 Фактическое значение ЕС при нижнем пределе
 * x2  Показания АЦП при верхнем значении ЕС
 * ec2 Фактическое значение ЕС при верхнем пределе
 *
 */
float calibrationEC(float x1, float ec1, float x2, float ec2, float x) {
    float b = (-log(ec1 / ec2)) / log(x2 / x1);
    float a = ((pow(x1, -b)) * ec1);
    float ec0 = a * pow(x, b);
    return ec0;
}

/*
 * Функция термокомпенсации ЕС
 *
 * k = 0.02; // Коэффициент влияния температуры на проводимость раствора
 * ec0 - измеренный EC без компенсации
 * t - температура раствора
 */
float compensateECByTemp(float k, float ec0, float t) {
    return ec0 / (1 + k * (t - 25));
}


// Функция преобразования чисел с плавающей запятой в текст
String fFTS(float x, byte precision) {
    char tmp[50];
    dtostrf(x, 0, precision, tmp);
    return String(tmp);
}


void setup() {
    Serial.begin(SERIAL_SPEED);
}


void loop() {
    int d1, d2, a1;
    long n;
    float x, x1, x2, ec, ec0, ec1, ec2, t, tk, k, l, lev;
    String gp;


    String tstring = "tparse="; // Метка начала строки для парсинга
    String tstring_end = "="; // Метка окончания строки
    String pstring = ""; // Метка строки вывода колонку

    /////////////////////////////     Раствор 1   ////////////////////////////////////////////
    gp = "container 1"; // Номер группы измерения

    // Измеритель ЕС1
    //  Схема подключения
    //  d1----a1----->
    //  d2----------->
    d1 = 10;  // Цифровой порт соединенный с аналоговым портом и электродом измерения
    d2 = 11;  // Цифровой порт соединенный с электродом противофазы
    a1 = 0;   // Аналоговый порт измерения
    n = 50000;// Колличество измерений для усреднения

    // Калибровка ЕС
    ec1 = 0.01;    // Фактическое значение ЕС при нижнем пределе
    x1 = 16;        // Показания АЦП при нижнем значении ЕС
    ec2 = 3;     // Фактическое значение ЕС при верхнем пределе
    x2 = 160;        // Показания АЦП при верхнем значении ЕС
    // Температура раствора
    tk = 0.02; // Коэффициент влияния температуры на проводимость раствора

    // Температура раствора, берется из датчиков в растворе.
    t = 20;

    // Измерение RAW проводимости
    x = getConductivity(d1, d2, a1, n);

    pstring += gp + " RAW Solution polarization ADC = ";
    pstring += fFTS(po, 2);
    pstring += "\n";
    // Расчет ЕС
    ec0 = calibrationEC(x1, ec1, x2, ec2, x);
    // Термокомпенсация ЕС
    ec = compensateECByTemp(tk, ec0, t);

    // Заполнение строкового вывода для машинного парсинга
    tstring += fFTS(t, 2) + ";" + fFTS(x, 4) + ";" + fFTS(ec0, 4) + ";" + fFTS(ec, 4) + ";";

    // Человекочитаемый вывод
    pstring += gp + " Solution temperature = ";
    pstring += fFTS(t, 2);
    pstring += "\n";
    pstring += gp + " RAW EC ADC = ";
    pstring += fFTS(x, 4);
    pstring += "\n";
    pstring += gp + " RAW Level ADC = ";
    pstring += fFTS(l, 2);
    pstring += "\n";
    pstring += gp + " EC = ";
    pstring += fFTS(ec0, 4);
    pstring += "\n";
    pstring += gp + " ECt = ";
    pstring += fFTS(ec, 4);
    pstring += "\n";
    pstring += gp + " Level = ";
    pstring += fFTS(lev, 4);
    pstring += "\n";
    Serial.println(pstring);

}


