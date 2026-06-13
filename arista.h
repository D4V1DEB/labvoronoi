#ifndef ARISTA_H
#define ARISTA_H

#include "punto.h"
#include <cmath>

class Arista {
public:
    Punto p1, p2;

    Arista() {}
    Arista(const Punto& a, const Punto& b) : p1(a), p2(b) {
        if (esMenor(p2, p1)) {
            Punto temporal = p1;
            p1 = p2;
            p2 = temporal;
        }
    }

    bool operator==(const Arista& otra) const {
        return p1 == otra.p1 && p2 == otra.p2;
    }

private:
    static bool esMenor(const Punto& a, const Punto& b) {
        if (std::abs(a.x - b.x) > 1e-9) {
            return a.x < b.x;
        }
        return a.y < b.y - 1e-9;
    }
};

#endif