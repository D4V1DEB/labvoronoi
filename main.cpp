#include <iostream>
#include "punto.h"
#include "triangulo.h"

int main() {
    std::cout << "circuncentro" << std::endl;

    Punto p1(0, 0);
    Punto p2(25, 0);
    Punto p3(12.5, 20);

    Triangulo tri(p1, p2, p3);
    tri.imprimir();

    try {
        Punto centro = tri.circuncentro();
        std::cout << "circuncentro: " << std::endl;
        std::cout << " ";
        centro.imprimir();
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
