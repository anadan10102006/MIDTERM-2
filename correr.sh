#!/usr/bin/env bash
# =============================================================================
# correr.sh
# Ejecuta el benchmark del taller y guarda los resultados.
#
# Uso (desde la carpeta donde esta este script):
#   bash correr.sh        -> corrida oficial, 30 repeticiones
#   bash correr.sh 2      -> prueba rapida, 2 repeticiones
# =============================================================================
set -e

AQUI="$(cd "$(dirname "$0")" && pwd)"
REPO="$AQUI/cpu-image-filters"
REPS="${1:-30}"

if [ ! -x "$REPO/build/bin/benchmark" ]; then
    echo "No existe el ejecutable. Corre primero: bash preparar.sh"
    exit 1
fi

# Busca una imagen por tamano en assets/ (el nombre debe contener 512, 1024...)
buscar() {
    ls "$REPO/assets/" | grep -E "$1" | grep -viE "salida" | head -1
}
I512=$(buscar "512");  I1024=$(buscar "1024")
I2048=$(buscar "2048"); I4096=$(buscar "4096")

if [ -z "$I512" ] || [ -z "$I1024" ] || [ -z "$I2048" ] || [ -z "$I4096" ]; then
    echo "No encontre las 4 imagenes en assets/ (sus nombres deben contener"
    echo "512, 1024, 2048 y 4096). Se usara sunflower.jpg redimensionada."
    echo "OJO: asi los resultados NO son comparables con los de Juliana."
    I512=sunflower.jpg; I1024=sunflower.jpg; I2048=sunflower.jpg; I4096=sunflower.jpg
fi

echo "Procesadores: $(nproc)   Repeticiones: $REPS"
echo "Imagenes: $I512  $I1024  $I2048  $I4096"
echo ""

cd "$REPO/build"
# systemd-inhibit evita que el computador se suspenda mientras corre
ARGS=("../assets/$I512" "../assets/$I1024" "../assets/$I2048" "../assets/$I4096" "$REPS")
if systemd-inhibit --what=sleep:idle true >/dev/null 2>&1; then
    systemd-inhibit --what=sleep:idle --why="benchmark taller" ./bin/benchmark "${ARGS[@]}"
else
    ./bin/benchmark "${ARGS[@]}"
fi

# Guardar resultados en una carpeta con fecha y hora
DESTINO="$AQUI/resultados/$(date +%Y-%m-%d_%H-%M)_${REPS}reps"
mkdir -p "$DESTINO"
cp resultados_resumen.csv resultados_crudos.csv "$DESTINO/"
lscpu > "$DESTINO/hardware.txt"

echo ""
echo "Resultados guardados en: $DESTINO"
echo "Lineas en crudos: $(wc -l < "$DESTINO/resultados_crudos.csv")  (con 30 reps deben ser 7681)"
echo "Configuraciones que NO pasaron la validacion: $(grep -c ',NO$' "$DESTINO/resultados_resumen.csv" || true)"
echo ""
echo "Para las graficas:  python3 graficas/generar_graficas.py \"$DESTINO/resultados_resumen.csv\""
