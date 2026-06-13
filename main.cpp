#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "arista.h"
#include "circuncirculo.h"
#include "punto.h"
#include "triangulo.h"

using std::vector;

vector<Punto> leerPuntos(const std::string& nombreArchivo) {
    std::ifstream archivo(nombreArchivo);
    vector<Punto> puntos;
    double x, y;

    while (archivo >> x >> y) {
        puntos.push_back(Punto(x, y));
    }

    return puntos;
}

Triangulo crearSuperTriangulo(const vector<Punto>& puntos) {
    double minX = puntos.front().x;
    double minY = puntos.front().y;
    double maxX = puntos.front().x;
    double maxY = puntos.front().y;

    for (const auto& punto : puntos) {
        minX = std::min(minX, punto.x);
        minY = std::min(minY, punto.y);
        maxX = std::max(maxX, punto.x);
        maxY = std::max(maxY, punto.y);
    }

    double dx = maxX - minX;
    double dy = maxY - minY;
    double deltaMax = std::max(dx, dy) * 10.0;
    if (deltaMax < 1.0) {
        deltaMax = 1.0;
    }

    double midX = (minX + maxX) / 2.0;
    double midY = (minY + maxY) / 2.0;

    Punto p1(midX - 20.0 * deltaMax, midY - deltaMax);
    Punto p2(midX, midY + 20.0 * deltaMax);
    Punto p3(midX + 20.0 * deltaMax, midY - deltaMax);

    return Triangulo(p1, p2, p3);
}

vector<Arista> aristasDeTriangulo(const Triangulo& triangulo) {
    return {Arista(triangulo.a, triangulo.b), Arista(triangulo.b, triangulo.c), Arista(triangulo.c, triangulo.a)};
}

bool aristasIguales(const Arista& e1, const Arista& e2) {
    return (e1.p1 == e2.p1 && e1.p2 == e2.p2) || (e1.p1 == e2.p2 && e1.p2 == e2.p1);
}

vector<Arista> construirCavidad(const vector<Triangulo>& triangulosInvalidos) {
    vector<Arista> todasAristas;

    for (const auto& triangulo : triangulosInvalidos) {
        todasAristas.push_back(Arista(triangulo.a, triangulo.b));
        todasAristas.push_back(Arista(triangulo.b, triangulo.c));
        todasAristas.push_back(Arista(triangulo.c, triangulo.a));
    }

    vector<Arista> fronteras;
    for (const auto& arista : todasAristas) {
        int contador = 0;
        for (const auto& otraArista : todasAristas) {
            if (aristasIguales(arista, otraArista)) {
                ++contador;
            }
        }

        if (contador == 1) {
            fronteras.push_back(arista);
        }
    }

    return fronteras;
}

bool triangulosIguales(const Triangulo& primero, const Triangulo& segundo) {
    return primero.a == segundo.a && primero.b == segundo.b && primero.c == segundo.c;
}

bool contieneVerticeSuperTriangulo(const Triangulo& triangulo, const Triangulo& superTriangulo) {
    return triangulo.a == superTriangulo.a || triangulo.a == superTriangulo.b || triangulo.a == superTriangulo.c ||
           triangulo.b == superTriangulo.a || triangulo.b == superTriangulo.b || triangulo.b == superTriangulo.c ||
           triangulo.c == superTriangulo.a || triangulo.c == superTriangulo.b || triangulo.c == superTriangulo.c;
}

//actividad 6 retriangulacion
vector<Triangulo> bowyerWatson(const vector<Punto>& puntos) {
    if (puntos.empty()) {
        return {};
    }

    Triangulo superTriangulo = crearSuperTriangulo(puntos);
    vector<Triangulo> triangulacion;
    triangulacion.push_back(superTriangulo);

    for (const auto& punto : puntos) {
        punto.imprimir();

        vector<Triangulo> triangulosInvalidos;

        for (const auto& triangulo : triangulacion) {
            Circuncirculo circulo(triangulo);
            if (circulo.contiene(punto)) {
                triangulosInvalidos.push_back(triangulo);
            }
        }

        std::cout << "Triangulos invalidos encontrados: " << triangulosInvalidos.size() << std::endl;
        for (const auto& triangulo : triangulosInvalidos) {
            triangulo.imprimir();
        }

        vector<Arista> cavidad = construirCavidad(triangulosInvalidos);

        vector<Triangulo> triangulacionActualizada;
        for (const auto& triangulo : triangulacion) {
            bool esInvalido = false;
            for (const auto& invalido : triangulosInvalidos) {
                if (triangulosIguales(triangulo, invalido)) {
                    esInvalido = true;
                    break;
                }
            }

            if (!esInvalido) {
                triangulacionActualizada.push_back(triangulo);
            }
        }

        triangulacion = triangulacionActualizada;

        for (const auto& arista : cavidad) {
            triangulacion.push_back(Triangulo(arista.p1, arista.p2, punto));
        }
    }

    vector<Triangulo> triangulacionFinal;
    for (const auto& triangulo : triangulacion) {
        if (!contieneVerticeSuperTriangulo(triangulo, superTriangulo)) {
            triangulacionFinal.push_back(triangulo);
        }
    }

    return triangulacionFinal;
}

void imprimirTriangulacion(const vector<Triangulo>& triangulacion) {
    std::cout << "Triangulacion final (" << triangulacion.size() << " triangulos):" << std::endl;
    for (const auto& triangulo : triangulacion) {
        triangulo.imprimir();
    }
}

int main() {
    try {
        vector<Punto> puntos = leerPuntos("puntos.txt");

        if (puntos.empty()) {
            std::cerr << "No se pudieron leer puntos desde puntos.txt" << std::endl;
            return 1;
        }

        vector<Triangulo> triangulacion = bowyerWatson(puntos);
        imprimirTriangulacion(triangulacion);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
