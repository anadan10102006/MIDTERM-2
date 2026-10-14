#!/usr/bin/env bash
# =============================================================================
# preparar.sh
# Descarga el repo original (akowalew/cpu-image-filters, 2019), le agrega los
# archivos del taller (benchmark + variantes de IA), aplica las correcciones
# necesarias y compila todo.
#
# Uso (desde la carpeta donde esta este script):
#   bash preparar.sh
# =============================================================================
set -e

AQUI="$(cd "$(dirname "$0")" && pwd)"
REPO="$AQUI/cpu-image-filters"
COMMIT="6f81b9c15a6817f52aecabc8823668abe2579da8"   # ultimo commit, 17/12/2019

echo "=============================================="
echo " 1/6  Revisando procesadores"
echo "=============================================="
NUCLEOS=$(nproc)
echo "Procesadores disponibles: $NUCLEOS"
if [ "$NUCLEOS" -lt 8 ]; then
    echo ""
    echo "  AVISO: tienes menos de 8 procesadores."
    echo "  El taller mide hasta 16 hilos y los resultados no seran comparables."
    echo "  Si estas en una maquina virtual, sube los CPUs a 8 (ver README)."
    echo ""
    read -p "  ¿Quieres continuar de todas formas? (s/n) " RESP
    [ "$RESP" = "s" ] || exit 1
fi

echo "=============================================="
echo " 2/6  Instalando dependencias (pide tu contraseña)"
echo "=============================================="
sudo apt update
sudo apt install -y git g++ cmake libopencv-dev libbenchmark-dev doctest-dev \
                    python3-pandas python3-matplotlib

echo "=============================================="
echo " 3/6  Descargando el repo original"
echo "=============================================="
if [ -d "$REPO" ]; then
    echo "Ya existe $REPO, no se vuelve a descargar."
else
    git clone https://github.com/akowalew/cpu-image-filters.git "$REPO"
    git -C "$REPO" checkout -q "$COMMIT"
fi

echo "=============================================="
echo " 4/6  Copiando archivos del taller"
echo "=============================================="
mkdir -p "$REPO/taller" "$REPO/lib/src/ia"
cp "$AQUI/archivos/taller/benchmark.cpp"      "$REPO/taller/"
cp "$AQUI/archivos/taller/CMakeLists.txt"     "$REPO/taller/"
cp "$AQUI/archivos/lib/src/ia/filter_ia.cpp"     "$REPO/lib/src/ia/"
cp "$AQUI/archivos/lib/src/ia/filter_ia_opt.cpp" "$REPO/lib/src/ia/"

# Imagenes de prueba
IMGS=$(find "$AQUI/imagenes" -maxdepth 1 -type f \( -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.png" -o -iname "*.bmp" -o -iname "*.pgm" -o -iname "*.ppm" \))
if [ -n "$IMGS" ]; then
    echo "$IMGS" | while read -r f; do cp "$f" "$REPO/assets/"; done
    echo "Imagenes copiadas a assets/:"
    ls "$REPO/assets/"
else
    echo "AVISO: la carpeta imagenes/ no tiene imagenes (ver README)."
fi

echo "=============================================="
echo " 5/6  Aplicando cambios al repo original"
echo "=============================================="
cd "$REPO"

# a) Registrar la carpeta taller/ en el CMake principal
grep -q "add_subdirectory(taller)" CMakeLists.txt || echo "add_subdirectory(taller)" >> CMakeLists.txt

# b) Agregar las variantes de IA a la libreria
grep -q "src/ia/filter_ia.cpp" lib/CMakeLists.txt || \
    sed -i 's|\tsrc/filter_seq.cpp|\tsrc/filter_seq.cpp\n\tsrc/ia/filter_ia.cpp\n\tsrc/ia/filter_ia_opt.cpp|' lib/CMakeLists.txt

# c) Corregir errores del programa de ejemplo filter-image (no del algoritmo)
sed -i 's/strcmp(type, "seq") == 1/strcmp(type, "seq") == 0/' app/src/main.cpp
sed -i '/setenv("OMP_NUM_THREADS", "4", 1);/d' app/src/main.cpp
sed -i 's/if(argc < 4)/if(argc < 5)/' app/src/main.cpp
grep -q "#include <cstring>" app/src/main.cpp || sed -i 's/#include <cstdlib>/#include <cstdlib>\n#include <cstring>/' app/src/main.cpp

echo "Cambios aplicados."

echo "=============================================="
echo " 6/6  Compilando"
echo "=============================================="
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_BENCHMARKING=ON
make -j"$(nproc)"

echo ""
echo "=============================================="
echo " LISTO. Ejecutables en: $REPO/build/bin/"
ls "$REPO/build/bin/"
echo " Siguiente paso: bash correr.sh 2   (prueba rapida)"
echo "=============================================="
