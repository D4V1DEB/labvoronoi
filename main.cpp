#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "arista.h"
#include "circuncirculo.h"
#include "punto.h"
#include "triangulo.h"

using std::vector;
namespace fs = std::filesystem;

struct ResultadoEjecucion {
    int n;
    size_t regiones;
    size_t verticesVoronoi;
    size_t aristasVoronoi;
    size_t triangulos;
    size_t k;
    long long tiempoMs;
    std::string archivoEntrada;
    std::string archivoDelaunay;
    std::string archivoVoronoi;
};

struct Caja {
    double minX;
    double maxX;
    double minY;
    double maxY;
};

struct DimensionesImagen {
    int ancho;
    int alto;
};

bool parecenPorcentajes(const vector<Punto>& puntos) {
    if (puntos.empty()) {
        return false;
    }

    for (const auto& punto : puntos) {
        if (punto.x < 0.0 || punto.x > 100.0 || punto.y < 0.0 || punto.y > 100.0) {
            return false;
        }
    }

    return true;
}

Punto escalarPunto(const Punto& punto, const DimensionesImagen& imagen, bool esPorcentaje) {
    if (!esPorcentaje) {
        return punto;
    }

    return Punto((punto.x / 100.0) * (imagen.ancho - 1), (punto.y / 100.0) * (imagen.alto - 1));
}

vector<Punto> escalarPuntos(const vector<Punto>& puntos, const DimensionesImagen& imagen, bool esPorcentaje) {
    vector<Punto> escalados;
    escalados.reserve(puntos.size());
    for (const auto& punto : puntos) {
        escalados.push_back(escalarPunto(punto, imagen, esPorcentaje));
    }
    return escalados;
}

DimensionesImagen leerDimensionesJPEG(const fs::path& archivoImagen) {
    std::ifstream archivo(archivoImagen, std::ios::binary);
    if (!archivo) {
        throw std::runtime_error("No se pudo abrir la imagen: " + archivoImagen.string());
    }

    unsigned char byte1 = 0;
    unsigned char byte2 = 0;
    archivo.read(reinterpret_cast<char*>(&byte1), 1);
    archivo.read(reinterpret_cast<char*>(&byte2), 1);
    if (byte1 != 0xFF || byte2 != 0xD8) {
        throw std::runtime_error("La imagen no parece ser JPEG: " + archivoImagen.string());
    }

    while (archivo) {
        unsigned char marcador = 0;
        archivo.read(reinterpret_cast<char*>(&marcador), 1);
        if (!archivo || marcador != 0xFF) {
            continue;
        }

        do {
            archivo.read(reinterpret_cast<char*>(&marcador), 1);
        } while (archivo && marcador == 0xFF);

        if (!archivo) {
            break;
        }

        if (marcador == 0xD9 || marcador == 0xDA) {
            break;
        }

        unsigned char alto1 = 0;
        unsigned char alto2 = 0;
        unsigned char ancho1 = 0;
        unsigned char ancho2 = 0;
        unsigned char longitud1 = 0;
        unsigned char longitud2 = 0;

        archivo.read(reinterpret_cast<char*>(&longitud1), 1);
        archivo.read(reinterpret_cast<char*>(&longitud2), 1);
        int longitud = (static_cast<int>(longitud1) << 8) | static_cast<int>(longitud2);

        if (longitud < 7) {
            throw std::runtime_error("Marcador JPEG invalido en " + archivoImagen.string());
        }

        if (marcador >= 0xC0 && marcador <= 0xC3) {
            unsigned char precision = 0;
            archivo.read(reinterpret_cast<char*>(&precision), 1);
            archivo.read(reinterpret_cast<char*>(&alto1), 1);
            archivo.read(reinterpret_cast<char*>(&alto2), 1);
            archivo.read(reinterpret_cast<char*>(&ancho1), 1);
            archivo.read(reinterpret_cast<char*>(&ancho2), 1);

            int alto = (static_cast<int>(alto1) << 8) | static_cast<int>(alto2);
            int ancho = (static_cast<int>(ancho1) << 8) | static_cast<int>(ancho2);
            return {ancho, alto};
        }

        archivo.seekg(longitud - 2 - 2, std::ios::cur);
    }

    throw std::runtime_error("No se pudieron leer las dimensiones JPEG de " + archivoImagen.string());
}

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

bool comparten2Vertices(const Triangulo& t1, const Triangulo& t2) {
    vector<Punto> vertices1 = {t1.a, t1.b, t1.c};
    vector<Punto> vertices2 = {t2.a, t2.b, t2.c};
    int comunes = 0;

    for (const auto& vertice1 : vertices1) {
        for (const auto& vertice2 : vertices2) {
            if (vertice1 == vertice2) {
                ++comunes;
            }
        }
    }

    return comunes == 2;
}

bool puntoMenorLexicografico(const Punto& a, const Punto& b) {
    if (std::abs(a.x - b.x) > 1e-9) {
        return a.x < b.x;
    }
    return a.y < b.y;
}

double productoCruz(const Punto& o, const Punto& a, const Punto& b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

bool trianguloDegenerado(const Triangulo& triangulo) {
    return std::abs(productoCruz(triangulo.a, triangulo.b, triangulo.c)) < 1e-9;
}

size_t calcularCascoConvexo(const vector<Punto>& puntos) {
    if (puntos.size() <= 2) {
        return puntos.size();
    }

    vector<Punto> ordenados = puntos;
    std::sort(ordenados.begin(), ordenados.end(), puntoMenorLexicografico);
    ordenados.erase(std::unique(ordenados.begin(), ordenados.end()), ordenados.end());

    if (ordenados.size() <= 2) {
        return ordenados.size();
    }

    vector<Punto> cascoInferior;
    for (const auto& p : ordenados) {
        while (cascoInferior.size() >= 2 &&
               productoCruz(cascoInferior[cascoInferior.size() - 2], cascoInferior.back(), p) <= 1e-9) {
            cascoInferior.pop_back();
        }
        cascoInferior.push_back(p);
    }

    vector<Punto> cascoSuperior;
    for (int i = static_cast<int>(ordenados.size()) - 1; i >= 0; --i) {
        const Punto& p = ordenados[i];
        while (cascoSuperior.size() >= 2 &&
               productoCruz(cascoSuperior[cascoSuperior.size() - 2], cascoSuperior.back(), p) <= 1e-9) {
            cascoSuperior.pop_back();
        }
        cascoSuperior.push_back(p);
    }

    if (!cascoInferior.empty()) {
        cascoInferior.pop_back();
    }
    if (!cascoSuperior.empty()) {
        cascoSuperior.pop_back();
    }

    return cascoInferior.size() + cascoSuperior.size();
}

vector<Punto> obtenerVerticesVoronoiUnicos(const vector<Triangulo>& triangulacion) {
    vector<Punto> verticesVoronoi;

    for (const auto& triangulo : triangulacion) {
        if (trianguloDegenerado(triangulo)) {
            continue;
        }
        Punto circuncentro = triangulo.circuncentro();
        if (std::find(verticesVoronoi.begin(), verticesVoronoi.end(), circuncentro) == verticesVoronoi.end()) {
            verticesVoronoi.push_back(circuncentro);
        }
    }

    return verticesVoronoi;
}

vector<Arista> construirVoronoi(const vector<Triangulo>& triangulacion) {
    vector<Arista> aristasVoronoi;

    for (size_t i = 0; i < triangulacion.size(); ++i) {
        for (size_t j = i + 1; j < triangulacion.size(); ++j) {
            const Triangulo& t1 = triangulacion[i];
            const Triangulo& t2 = triangulacion[j];

            if (trianguloDegenerado(t1) || trianguloDegenerado(t2)) {
                continue;
            }

            if (comparten2Vertices(t1, t2)) {
                Punto c1 = t1.circuncentro();
                Punto c2 = t2.circuncentro();
                aristasVoronoi.push_back(Arista(c1, c2));
            }
        }
    }

    return aristasVoronoi;
}

Caja construirCajaDominio(const DimensionesImagen& imagen) {
    return {0.0, static_cast<double>(imagen.ancho - 1), 0.0, static_cast<double>(imagen.alto - 1)};
}

bool recortarSegmento(const Arista& entrada, const Caja& caja, Arista& salida) {
    double x1 = entrada.p1.x;
    double y1 = entrada.p1.y;
    double x2 = entrada.p2.x;
    double y2 = entrada.p2.y;

    double dx = x2 - x1;
    double dy = y2 - y1;

    double t0 = 0.0;
    double t1 = 1.0;

    auto ajustar = [&](double p, double q) -> bool {
        if (std::abs(p) < 1e-12) {
            return q >= 0.0;
        }

        double r = q / p;
        if (p < 0.0) {
            if (r > t1) {
                return false;
            }
            if (r > t0) {
                t0 = r;
            }
        } else {
            if (r < t0) {
                return false;
            }
            if (r < t1) {
                t1 = r;
            }
        }
        return true;
    };

    if (!ajustar(-dx, x1 - caja.minX)) {
        return false;
    }
    if (!ajustar(dx, caja.maxX - x1)) {
        return false;
    }
    if (!ajustar(-dy, y1 - caja.minY)) {
        return false;
    }
    if (!ajustar(dy, caja.maxY - y1)) {
        return false;
    }

    auto limitar = [](double valor, double minimo, double maximo) {
        return std::max(minimo, std::min(valor, maximo));
    };

    Punto q1(
        limitar(x1 + t0 * dx, caja.minX, caja.maxX),
        limitar(y1 + t0 * dy, caja.minY, caja.maxY));
    Punto q2(
        limitar(x1 + t1 * dx, caja.minX, caja.maxX),
        limitar(y1 + t1 * dy, caja.minY, caja.maxY));
    if (q1 == q2) {
        return false;
    }

    salida = Arista(q1, q2);
    return true;
}

vector<Arista> recortarAristasAlDominio(const vector<Arista>& aristas, const Caja& caja) {
    vector<Arista> recortadas;
    for (const auto& arista : aristas) {
        Arista recortada;
        if (recortarSegmento(arista, caja, recortada)) {
            if (std::find(recortadas.begin(), recortadas.end(), recortada) == recortadas.end()) {
                recortadas.push_back(recortada);
            }
        }
    }
    return recortadas;
}

vector<Triangulo> bowyerWatson(const vector<Punto>& puntos) {
    if (puntos.empty()) {
        return {};
    }

    Triangulo superTriangulo = crearSuperTriangulo(puntos);
    vector<Triangulo> triangulacion;
    triangulacion.push_back(superTriangulo);

    for (const auto& punto : puntos) {
        vector<Triangulo> triangulosInvalidos;

        for (const auto& triangulo : triangulacion) {
            if (trianguloDegenerado(triangulo)) {
                continue;
            }
            Circuncirculo circulo(triangulo);
            if (circulo.contiene(punto)) {
                triangulosInvalidos.push_back(triangulo);
            }
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
            Triangulo nuevoTriangulo(arista.p1, arista.p2, punto);
            if (!trianguloDegenerado(nuevoTriangulo)) {
                triangulacion.push_back(nuevoTriangulo);
            }
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

vector<Arista> extraerAristasDelaunay(const vector<Triangulo>& triangulacion) {
    vector<Arista> aristas;

    for (const auto& triangulo : triangulacion) {
        for (const auto& arista : aristasDeTriangulo(triangulo)) {
            if (std::find(aristas.begin(), aristas.end(), arista) == aristas.end()) {
                aristas.push_back(arista);
            }
        }
    }

    return aristas;
}

void guardarAristas(const vector<Arista>& aristas, const std::string& archivoSalida) {
    std::ofstream salida(archivoSalida);
    for (const auto& arista : aristas) {
        salida << arista.p1.x << " " << arista.p1.y << " "
               << arista.p2.x << " " << arista.p2.y << "\n";
    }
}

int extraerN(const std::string& nombreArchivo) {
    std::string numero;
    for (char c : nombreArchivo) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            numero.push_back(c);
        } else if (!numero.empty()) {
            break;
        }
    }

    if (numero.empty()) {
        return -1;
    }

    return std::stoi(numero);
}

vector<fs::path> obtenerEntradasPuntos(const fs::path& carpetaPuntos) {
    vector<fs::path> entradas;

    if (!fs::exists(carpetaPuntos) || !fs::is_directory(carpetaPuntos)) {
        return entradas;
    }

    for (const auto& entrada : fs::directory_iterator(carpetaPuntos)) {
        if (entrada.is_regular_file() && entrada.path().extension() == ".txt") {
            entradas.push_back(entrada.path());
        }
    }

    std::sort(entradas.begin(), entradas.end(), [](const fs::path& a, const fs::path& b) {
        int na = extraerN(a.filename().string());
        int nb = extraerN(b.filename().string());
        if (na == nb) {
            return a.filename().string() < b.filename().string();
        }
        return na < nb;
    });

    return entradas;
}

ResultadoEjecucion ejecutarCaso(const fs::path& archivoEntrada, const fs::path& carpetaSalida, const DimensionesImagen& imagen) {
    vector<Punto> puntosCrudos = leerPuntos(archivoEntrada.string());
    bool usaPorcentajes = parecenPorcentajes(puntosCrudos);
    vector<Punto> puntos = escalarPuntos(puntosCrudos, imagen, usaPorcentajes);
    if (puntos.empty()) {
        throw std::runtime_error("No se pudieron leer puntos desde " + archivoEntrada.string());
    }

    auto tInicio = std::chrono::steady_clock::now();
    vector<Triangulo> triangulacion = bowyerWatson(puntos);
    vector<Punto> verticesVoronoi = obtenerVerticesVoronoiUnicos(triangulacion);
    Caja cajaDominio = construirCajaDominio(imagen);
    vector<Arista> aristasVoronoiBrutas = construirVoronoi(triangulacion);
    vector<Arista> aristasVoronoi = recortarAristasAlDominio(aristasVoronoiBrutas, cajaDominio);
    vector<Arista> aristasDelaunay = extraerAristasDelaunay(triangulacion);
    auto tFin = std::chrono::steady_clock::now();

    int n = extraerN(archivoEntrada.filename().string());
    std::string sufijo = n > 0 ? std::to_string(n) : archivoEntrada.stem().string();

    fs::path archivoDelaunay = carpetaSalida / ("delaunay_n" + sufijo + ".txt");
    fs::path archivoVoronoi = carpetaSalida / ("voronoi_n" + sufijo + ".txt");

    guardarAristas(aristasDelaunay, archivoDelaunay.string());
    guardarAristas(aristasVoronoi, archivoVoronoi.string());

    ResultadoEjecucion resultado;
    resultado.n = n;
    resultado.regiones = puntos.size();
    resultado.verticesVoronoi = verticesVoronoi.size();
    resultado.aristasVoronoi = aristasVoronoi.size();
    resultado.triangulos = triangulacion.size();
    resultado.k = calcularCascoConvexo(puntos);
    resultado.tiempoMs = std::chrono::duration_cast<std::chrono::milliseconds>(tFin - tInicio).count();
    resultado.archivoEntrada = archivoEntrada.string();
    resultado.archivoDelaunay = archivoDelaunay.string();
    resultado.archivoVoronoi = archivoVoronoi.string();
    return resultado;
}

void guardarResumen(const vector<ResultadoEjecucion>& resultados, const fs::path& archivoResumen) {
    std::ofstream salida(archivoResumen.string());
    salida << "n,regiones,vertices_voronoi,aristas_voronoi,triangulos,k,teorico,delta,tiempo_ms\n";

    for (const auto& r : resultados) {
        long long teorico = static_cast<long long>(2 * r.regiones) - 2 - static_cast<long long>(r.k);
        long long delta = static_cast<long long>(r.triangulos) - teorico;
        salida << r.n << ","
               << r.regiones << ","
               << r.verticesVoronoi << ","
               << r.aristasVoronoi << ","
               << r.triangulos << ","
               << r.k << ","
               << teorico << ","
               << delta << ","
               << r.tiempoMs << "\n";
    }
}

void imprimirResumenConsola(const vector<ResultadoEjecucion>& resultados) {
    std::cout << "Resultados por caso:" << std::endl;
    for (const auto& r : resultados) {
        long long teorico = static_cast<long long>(2 * r.regiones) - 2 - static_cast<long long>(r.k);
        long long delta = static_cast<long long>(r.triangulos) - teorico;
        std::cout << "n=" << r.n
                  << " regiones=" << r.regiones
                  << " verticesVoronoi=" << r.verticesVoronoi
                  << " aristasVoronoi=" << r.aristasVoronoi
                  << " triangulos=" << r.triangulos
                  << " k=" << r.k
                  << " teorico=" << teorico
                  << " delta=" << delta
                  << " tiempoMs=" << r.tiempoMs
                  << std::endl;
    }
}

int main(int argc, char* argv[]) {
    try {
        fs::path carpetaPuntos = "puntos";
        fs::path carpetaSalida = "salidas";
        fs::path archivoImagen = "imagen.jpg";

        if (argc >= 2) {
            carpetaPuntos = argv[1];
        }
        if (argc >= 3) {
            carpetaSalida = argv[2];
        }
        if (argc >= 4) {
            archivoImagen = argv[3];
        }

        fs::create_directories(carpetaSalida);
        DimensionesImagen dimensionesImagen = leerDimensionesJPEG(archivoImagen);
        vector<fs::path> archivos = obtenerEntradasPuntos(carpetaPuntos);

        if (archivos.empty()) {
            std::cerr << "No se encontraron archivos .txt en " << carpetaPuntos.string() << std::endl;
            return 1;
        }

        vector<ResultadoEjecucion> resultados;
        for (const auto& archivo : archivos) {
            resultados.push_back(ejecutarCaso(archivo, carpetaSalida, dimensionesImagen));
        }

        guardarResumen(resultados, carpetaSalida / "resumen.csv");
        imprimirResumenConsola(resultados);
        std::cout << "Resumen guardado en " << (carpetaSalida / "resumen.csv").string() << std::endl;
        std::cout << "Aristas exportadas en " << carpetaSalida.string() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
