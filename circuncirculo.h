#ifndef CIRCUNCIRCULO_H
#define CIRCUNCIRCULO_H

#include "triangulo.h"

class Circuncirculo {
public:
    Punto centro;
    double radio;

    static constexpr double EPSILON = 1e-9;

    Circuncirculo(const Triangulo& triangulo) : centro(triangulo.circuncentro()), radio(centro.distancia(triangulo.a)) {}

    bool contiene(const Punto& punto) const {
        double dx = punto.x - centro.x;
        double dy = punto.y - centro.y;
        double distanciaAlCuadrado = dx * dx + dy * dy;
        return distanciaAlCuadrado <= radio * radio + EPSILON;
    }

    void imprimir() const {
        std::cout << "Circuncirculo:" << std::endl;
        std::cout << "Centro: ";
        centro.imprimir();
        std::cout << std::endl;
        std::cout << "Radio: " << radio << std::endl;
    }
};

#endif