# Compilar y ejecutar el programa en C++
g++ -std=c++17 main.cpp -o labvoronoi
.\labvoronoi

# Generar las gráficas
$env:MPLBACKEND="Agg"
python grafica.py -n 50
python grafica.py -n 100
python grafica.py -n 150
python grafica.py -n 200
python grafica.py -n 250
