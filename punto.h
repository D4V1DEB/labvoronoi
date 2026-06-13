#ifndef PUNTO_H
#define PUNTO_H

#include <cmath>
#include <iostream>

class Punto {
public:
    double x, y;

    Punto() : x(0), y(0) {}
    Punto(double x, double y) : x(x), y(y) {}

    double distancia(const Punto& otro) const {
        double dx = x - otro.x;
        double dy = y - otro.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    void imprimir() const {
        std::cout << "(" << x << " , " << y << ")";
    }

    bool operator==(const Punto& otro) const {
        return std::abs(x - otro.x) < 1e-9 && std::abs(y - otro.y) < 1e-9;
    }
};

#endif
