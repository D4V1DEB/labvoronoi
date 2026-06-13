import argparse
from pathlib import Path

import matplotlib.image as mpimg
import matplotlib.pyplot as plt


def leer_puntos(path):
    puntos = []
    with open(path, "r", encoding="utf-8") as f:
        for linea in f:
            linea = linea.strip()
            if not linea:
                continue
            x, y = map(float, linea.split())
            puntos.append((x, y))
    return puntos


def leer_aristas(path):
    aristas = []
    with open(path, "r", encoding="utf-8") as f:
        for linea in f:
            linea = linea.strip()
            if not linea:
                continue
            x1, y1, x2, y2 = map(float, linea.split())
            aristas.append((x1, y1, x2, y2))
    return aristas


def detectar_modo_coordenadas(puntos):
    if not puntos:
        return "pixel"

    # The source point list defines the coordinate system.
    # Voronoi circumcenters can naturally fall outside [0, 100] even when input is percent.
    if all(0.0 <= x <= 100.0 and 0.0 <= y <= 100.0 for x, y in puntos):
        return "percent"
    return "pixel"


def escalar_punto(x, y, ancho, alto, modo):
    if modo == "percent":
        return (x / 100.0) * (ancho - 1), (y / 100.0) * (alto - 1)
    return x, y


def escalar_puntos(puntos, ancho, alto, modo):
    return [escalar_punto(x, y, ancho, alto, modo) for x, y in puntos]


def dibujar_aristas(ax, aristas, color, linewidth):
    for x1, y1, x2, y2 in aristas:
        ax.plot([x1, x2], [y1, y2], color=color, linewidth=linewidth)


def main():
    parser = argparse.ArgumentParser(description="Superponer Delaunay y Voronoi sobre una imagen")
    parser.add_argument("-n", type=int, required=True, help="tamano del conjunto (50,100,150,200,250)")
    parser.add_argument("--imagen", default="imagen.jpg", help="ruta de la imagen base")
    parser.add_argument("--puntos-dir", default="puntos", help="carpeta de archivos de puntos")
    parser.add_argument("--salidas-dir", default="salidas", help="carpeta de salidas generadas por C++")
    parser.add_argument("--out", default=None, help="ruta png de salida")
    parser.add_argument(
        "--coords",
        choices=["auto", "pixel", "percent"],
        default="auto",
        help="tipo de coordenadas en archivos de puntos/aristas",
    )
    args = parser.parse_args()

    puntos_path = Path(args.puntos_dir) / f"{args.n}-puntos.txt"
    delaunay_path = Path(args.salidas_dir) / f"delaunay_n{args.n}.txt"
    voronoi_path = Path(args.salidas_dir) / f"voronoi_n{args.n}.txt"

    if args.out is None:
        out_path = Path(args.salidas_dir) / f"resultado_n{args.n}.png"
    else:
        out_path = Path(args.out)

    img = mpimg.imread(args.imagen)
    alto, ancho = img.shape[0], img.shape[1]
    fig, ax = plt.subplots(figsize=(10, 7))
    ax.imshow(img)
    ax.set_xlim(0, ancho - 1)
    ax.set_ylim(alto - 1, 0)
    ax.set_autoscale_on(False)

    puntos = leer_puntos(puntos_path)
    aristas_delaunay = leer_aristas(delaunay_path)
    aristas_voronoi = leer_aristas(voronoi_path)

    if args.coords == "auto":
        modo = detectar_modo_coordenadas(puntos)
    else:
        modo = args.coords

    puntos = escalar_puntos(puntos, ancho, alto, modo)

    dibujar_aristas(ax, aristas_delaunay, "blue", 0.5)
    dibujar_aristas(ax, aristas_voronoi, "red", 0.7)

    if puntos:
        xs, ys = zip(*puntos)
        ax.scatter(xs, ys, c="yellow", s=10)

    ax.set_title(f"Delaunay + Voronoi (n={args.n}, coords=px)")
    plt.savefig(out_path, dpi=200)
    plt.close(fig)


if __name__ == "__main__":
    main()