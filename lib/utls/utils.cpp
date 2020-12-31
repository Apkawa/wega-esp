#include "utils.h"


String formatFloat(float x, byte precision) {
    char tmp[50];
    dtostrf(x, 0, precision, tmp);
    return String(tmp);
}

// Todo use class
//Коэффициенты фильтрации Кальмана
float Kl1 = 0.1, Pr1 = 0.0001, Pc1 = 0.0, G1 = 1.0, P1 = 0.0, Xp1 = 0.0, Zp1 = 0.0, Xe1 = 0.0;

float kalman_filter(float val) {
    Pc1 = P1 + Pr1;
    G1 = Pc1 / (Pc1 + Kl1);
    P1 = (1 - G1) * Pc1;
    Xp1 = Xe1;
    Zp1 = Xp1;
    Xe1 = G1 * (val - Zp1) + Xp1; // "фильтрованное" значение

    if (abs(val - Xe1) / Xe1 > 0.05) {
        Xe1 = val;
    }
    return Xe1;
}
