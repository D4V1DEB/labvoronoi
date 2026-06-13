#include <iostream>
#include "punto.h"
#include "triangulo.h"
#include "circuncirculo.h"

int main() {
    std::cout << "circuncentro y circuncirculo" << std::endl;

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

        Circuncirculo circ(tri);
        circ.imprimir();

        Punto puntoDentro(12.5, 0);
        Punto puntoFuera(40, 40);

        std::cout << "Contiene (12.5, 0): " << (circ.contiene(puntoDentro) ? "si" : "no") << std::endl;
        std::cout << "Contiene (40, 40): " << (circ.contiene(puntoFuera) ? "si" : "no") << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
