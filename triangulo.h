#ifndef TRIANGULO_H
#define TRIANGULO_H

#include "punto.h"
#include <stdexcept>
#include <iostream>

class Triangulo {
public:
    Punto a, b, c;

    Triangulo() {}
    Triangulo(const Punto& a, const Punto& b, const Punto& c) : a(a), b(b), c(c) {}

    Punto circuncentro() const {
        double d = 2 * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));

        if (std::abs(d) < 1e-9) {
            throw std::runtime_error("Error: Los puntos del triangulo son colineales");
        }

        double ux = ((a.x * a.x + a.y * a.y) * (b.y - c.y) + (b.x * b.x + b.y * b.y) * (c.y - a.y) + (c.x * c.x + c.y * c.y) * (a.y - b.y)) / d;
        double uy = ((a.x * a.x + a.y * a.y) * (c.x - b.x) + (b.x * b.x + b.y * b.y) * (a.x - c.x) + (c.x * c.x + c.y * c.y) * (b.x - a.x)) / d;

        return Punto(ux, uy);
    }

    void imprimir() const {
        std::cout << "Triángulo: ";
        a.imprimir();
        std::cout << " - ";
        b.imprimir();
        std::cout << " - ";
        c.imprimir();
        std::cout << std::endl;
    }
};

#endif
