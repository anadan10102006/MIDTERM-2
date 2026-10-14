# Taller CL11 · Benchmarking de código C++ paralelo generado por IA (OpenMP)

Programación Paralela · Pontificia Universidad Javeriana Cali

Este paquete tiene **todo lo necesario** para repetir el experimento del taller en tu computador: el código de las cuatro variantes, el programa que hace las mediciones, los scripts que instalan y corren todo, el script de las gráficas y los resultados que obtuvo Juliana para que puedas comparar.

Si nunca has usado Linux, no pasa nada. Sigue la guía en orden y copia los comandos tal cual.

---

## Índice

1. [¿Qué hace este experimento?](#1-qué-hace-este-experimento)
2. [Versión corta (si ya sabes lo que haces)](#2-versión-corta-si-ya-sabes-lo-que-haces)
3. [Las cuatro variantes que se comparan](#3-las-cuatro-variantes-que-se-comparan)
4. [Qué se le cambió al repo original](#4-qué-se-le-cambió-al-repo-original)
5. [Qué trae este paquete](#5-qué-trae-este-paquete)
6. [Requisitos: Linux y 8 procesadores](#6-requisitos-linux-y-8-procesadores)
7. [Paso a paso con los scripts (recomendado)](#7-paso-a-paso-con-los-scripts-recomendado)
8. [Paso a paso manual (si el script falla)](#8-paso-a-paso-manual-si-el-script-falla)
9. [Probar las variantes por separado](#9-probar-las-variantes-por-separado)
10. [Cómo leer los resultados](#10-cómo-leer-los-resultados)
11. [Resultados de referencia (Juliana)](#11-resultados-de-referencia-juliana)
12. [Cómo agregar tu propia versión de IA](#12-cómo-agregar-tu-propia-versión-de-ia)
13. [Problemas frecuentes](#13-problemas-frecuentes)
14. [Transparencia sobre el uso de IA](#14-transparencia-sobre-el-uso-de-ia)
15. [Mini glosario de Linux](#15-mini-glosario-de-linux)

---

## 1. ¿Qué hace este experimento?

El objetivo del taller es medir **cuánto mejora el rendimiento al paralelizar** un filtro de imágenes y comparar **código paralelo escrito por IA contra código escrito a mano**.

El filtro es una **convolución 2D**: a cada píxel de la imagen se le aplica un promedio ponderado de sus vecinos usando una matriz pequeña llamada *kernel*. En este caso el kernel es un filtro de desenfoque (todos los pesos valen 1/k²).

Según el enunciado del taller, se prueban todas las combinaciones de:

| Parámetro | Valores |
|---|---|
| Tamaño de imagen | 512×512, 1024×1024, 2048×2048, 4096×4096 px |
| Tamaño del kernel | 3×3, 5×5, 7×7, 11×11 |
| Número de hilos | 1, 2, 4, 8, 16 |
| Repeticiones por configuración | 30 |

Por cada configuración se mide **solo el tiempo del filtrado** (no el de leer o guardar la imagen) y se calculan:

| Métrica | Fórmula | Qué significa |
|---|---|---|
| Tiempo | media de 30 corridas, en ms | Cuánto tarda |
| Desviación estándar | de las 30 corridas | Qué tan estables fueron las mediciones |
| Speedup | Sp = T₁ / Tp | Cuántas veces más rápido que la versión secuencial |
| Eficiencia | Ep = Sp / p | Qué tan bien se aprovecha cada hilo (ideal = 1) |

Además, **cada resultado se valida**: la imagen que produce cada versión paralela se compara píxel por píxel contra la de la versión secuencial. Si no son idénticas, se marca como error.

---

## 2. Versión corta (si ya sabes lo que haces)

Desde la carpeta del paquete, en una terminal de Linux con 8 CPUs:

```bash
nproc                    # debe decir 8
bash preparar.sh         # instala, descarga el repo, aplica cambios y compila
bash correr.sh 2         # prueba rápida (2 repeticiones)
bash correr.sh           # corrida oficial (30 repeticiones)
python3 graficas/generar_graficas.py resultados/<carpeta>/resultados_resumen.csv
```

Si algo de esto no te suena, sigue leyendo.

---

## 3. Las cuatro variantes que se comparan

Las cuatro hacen **exactamente la misma operación** y producen **exactamente la misma imagen**. Lo único que cambia es cómo está escrito el código.

| Variante | Nombre en los resultados | Archivo | Quién lo escribió |
|---|---|---|---|
| C++ Secuencial | `secuencial` | `lib/src/filter_seq.cpp` | Autor original del repo (2019), a mano |
| C++ Paralelo Manual | `manual_omp` | `lib/src/filter_omp.cpp` | Autor original del repo (2019), a mano |
| C++ Paralelo IA | `ia` | `lib/src/ia/filter_ia.cpp` | Generado por IA |
| C++ Paralelo IA + Opt. | `ia_opt` | `lib/src/ia/filter_ia_opt.cpp` | IA + optimización posterior |

### 3.1 Secuencial (la base de referencia)

Viene del repositorio **[akowalew/cpu-image-filters](https://github.com/akowalew/cpu-image-filters)**, un proyecto universitario de diciembre de 2019 (antes de que existieran asistentes de código con IA). Usa un solo hilo. Su tiempo es el **T₁** con el que se calcula el speedup de todas las demás.

La estructura del algoritmo son cuatro bucles anidados:

```
para cada fila i de la imagen
  para cada columna j
    acc = 0
    para cada fila m del kernel
      para cada columna n del kernel
        acc += pixel(i+m, j+n) * kernel(m, n)
    guardar acc (limitado a 0–255) en la imagen de salida
```

### 3.2 Paralelo Manual

Es del **mismo repositorio de 2019**. Es el mismo algoritmo que la secuencial, pero con una línea de OpenMP antes del primer bucle:

```cpp
#pragma omp parallel for
for(auto i = 0; i < height; ++i)
```

Eso reparte las filas de la imagen entre los hilos. Cada hilo procesa un grupo de filas por su cuenta.

> **Por qué este repo:** el taller pide un código paralelo "100% a mano", sacado de GitHub o GitLab, de antes de 2020 para garantizar que no tuvo ayuda de IA. Este repo cumple: primer commit el 8/12/2019, último el 17/12/2019, y usa OpenMP.

### 3.3 Paralelo IA

Se obtuvo pidiéndole a una IA que paralelizara el código secuencial. El prompt fue:

```
Paraleliza con OpenMP la siguiente función de convolución 2D en C++.
Mantén exactamente la misma firma y el mismo resultado numérico.
Renombra la función a filter2d_8_ia. Devuelve solo el código.

[contenido completo de lib/src/filter_seq.cpp]
```

**La primera respuesta no compiló.** Le faltaban la cabecera `<cstddef>` (para `size_t`) y la función auxiliar `clamp_to`. Se le devolvieron los errores de compilación a la misma IA, y la segunda respuesta sí compiló. Esa segunda respuesta es la que está en `filter_ia.cpp`, sin modificaciones.

Lo que hizo la IA, comparado con la versión manual:

- Agregó `schedule(static)` al `#pragma omp parallel for`.
- En vez de ir moviendo punteros de fila en fila, calcula el puntero de cada fila a partir de `i`. Así cada iteración es independiente.
- Mantuvo el mismo orden de suma, así que el resultado es idéntico.

En la práctica, rinde casi igual que la versión manual.

### 3.4 Paralelo IA + Opt.

Parte de la versión de IA y le aplica una optimización: **invertir el orden de los bucles** para que el compilador pueda usar instrucciones **SIMD** (vectoriales), que hacen varias multiplicaciones a la vez.

```
para cada fila i de la imagen                (repartido entre hilos)
  poner en 0 un arreglo acum[] con un acumulador por cada píxel de la fila
  para cada fila m del kernel
    para cada columna n del kernel
      peso = kernel(m, n)
      para cada columna j                    ← bucle largo, se vectoriza
        acum[j] += pixel(i+m, j+n) * peso
  guardar acum[j] (limitado a 0–255) en la imagen de salida
```

En la versión original, el bucle de más adentro es el del kernel (3 a 11 vueltas) y todas las sumas van a una sola variable, lo que impide vectorizar. Aquí el bucle de más adentro recorre toda la fila (cientos o miles de píxeles seguidos en memoria), cada uno con su propio acumulador, y eso sí se puede vectorizar.

Detalles importantes:

- Cada hilo tiene su **propio** arreglo `acum` (se declara dentro de `#pragma omp parallel`). Si fuera compartido, los hilos se pisarían los datos.
- Cada píxel sigue sumando en el mismo orden (primero `m`, luego `n`), así que **el resultado es idéntico bit a bit** a la versión secuencial.

> **Ojo:** esta optimización también se hizo con ayuda de IA (ver [sección 14](#14-transparencia-sobre-el-uso-de-ia)).

---

## 4. Qué se le cambió al repo original

El algoritmo de las versiones **secuencial** y **manual** (`filter_seq.cpp` y `filter_omp.cpp`) **no se tocó**. Lo que se agregó o corrigió fue:

| Archivo | Cambio | Por qué |
|---|---|---|
| `taller/benchmark.cpp` | **Nuevo.** Programa de medición | El repo no medía lo que pide el taller |
| `taller/CMakeLists.txt` | **Nuevo.** Cómo compilar el benchmark | Para que se compile junto con lo demás |
| `lib/src/ia/filter_ia.cpp` | **Nuevo.** Variante IA | — |
| `lib/src/ia/filter_ia_opt.cpp` | **Nuevo.** Variante IA + Opt. | — |
| `CMakeLists.txt` (raíz) | Se agregó `add_subdirectory(taller)` al final | Para que CMake compile la carpeta `taller/` |
| `lib/CMakeLists.txt` | Se agregaron `src/ia/filter_ia.cpp` y `src/ia/filter_ia_opt.cpp` a la librería | Para que las variantes de IA se compilen |
| `app/src/main.cpp` | `strcmp(type, "seq") == 1` → `== 0` | Error del original: siempre corría la versión OpenMP aunque pidieras `seq` |
| `app/src/main.cpp` | Se borró `setenv("OMP_NUM_THREADS", "4", 1);` | Forzaba 4 hilos e impedía elegir el número |
| `app/src/main.cpp` | `argc < 4` → `argc < 5` | Si faltaba el tamaño del kernel, el programa se caía en vez de mostrar cómo usarlo |
| `app/src/main.cpp` | Se agregó `#include <cstring>` | Necesario para `strcmp` |

Los cambios en `main.cpp` solo afectan al programa de ejemplo `filter-image`, **no** al benchmark ni a los algoritmos. El script `preparar.sh` hace todos estos cambios automáticamente.

### Cómo funciona el programa de medición (`taller/benchmark.cpp`)

1. Carga las 4 imágenes en escala de grises. Si alguna no mide exactamente 512, 1024, 2048 o 4096 px, la redimensiona y lo avisa.
2. Para cada imagen y cada kernel:
   - Corre la **secuencial** 30 veces → ese es T₁ y la imagen de referencia.
   - Para cada variante paralela y cada número de hilos (1, 2, 4, 8, 16):
     - Fija los hilos con `omp_set_num_threads(p)`.
     - Hace 1 corrida de **calentamiento** que no se cuenta.
     - Hace 30 corridas, midiendo **solo la llamada al filtro** con `omp_get_wtime()`.
     - Compara la última salida contra la secuencial, píxel por píxel.
3. Guarda todo en dos archivos CSV.

Usa `omp_set_dynamic(0)` para que OpenMP use exactamente los hilos pedidos.

---

## 5. Qué trae este paquete

```
taller-benchmarking-openmp/
├── README.md                    ← este archivo
├── preparar.sh                  ← instala todo y compila (se corre 1 vez)
├── correr.sh                    ← corre el benchmark y guarda los resultados
├── archivos/                    ← archivos que se agregan al repo original
│   ├── taller/
│   │   ├── benchmark.cpp
│   │   └── CMakeLists.txt
│   └── lib/src/ia/
│       ├── filter_ia.cpp
│       └── filter_ia_opt.cpp
├── imagenes/                    ← aquí van las 4 imágenes de entrada
├── graficas/
│   └── generar_graficas.py      ← genera las gráficas desde el CSV
└── resultados_juliana/          ← resultados de referencia (8 CPUs, 30 reps)
    ├── resultados_resumen.csv
    ├── resultados_crudos.csv
    ├── hardware.txt
    └── graficas/
```

Después de correr los scripts aparecen dos carpetas nuevas:

```
├── cpu-image-filters/           ← el repo descargado, con los cambios y compilado
└── resultados/                  ← tus resultados, una subcarpeta por corrida
```

---

## 6. Requisitos: Linux y 8 procesadores

### 6.1 Sistema operativo

Necesitas **Linux**, idealmente **Ubuntu 22.04 / 24.04** o derivados (Xubuntu, Kubuntu, Linux Mint). Juliana lo corrió en **Xubuntu dentro de una máquina virtual**.

Si estás en **Windows**, tienes dos opciones:

- **Máquina virtual** (VirtualBox, VMware) con Ubuntu o Xubuntu instalado.
- **WSL** (Ubuntu dentro de Windows). En PowerShell como administrador:

  ```powershell
  wsl --install -d Ubuntu-24.04
  ```

  Reinicias, abres "Ubuntu" desde el menú de inicio y ya tienes una terminal de Linux. WSL usa todos los núcleos de tu computador por defecto.

### 6.2 Abrir la terminal

En Ubuntu/Xubuntu: `Ctrl + Alt + T`, o busca "Terminal" en el menú.

### 6.3 Verificar que tienes 8 procesadores

```bash
nproc
```

**Debe decir 8 o más.** Si dice menos, el experimento no va a mostrar bien la escalabilidad: con 2 procesadores, por ejemplo, el speedup se estanca en 2 sin importar cuántos hilos pidas. Eso le pasó a Juliana en la primera corrida.

Para ver más detalle:

```bash
lscpu | grep -E "^CPU\(s\)|Model name|Thread|Core|Socket|Hypervisor"
```

Si aparece `Hypervisor vendor`, estás en una máquina virtual.

### 6.4 Si estás en una máquina virtual y `nproc` dice menos de 8

**En VirtualBox:**

1. Apaga Linux **por completo** (no suspender ni guardar estado).
2. En VirtualBox, selecciona la máquina → **Configuración** → **Sistema** → pestaña **Procesador**.
3. Sube **Procesador(es)** a **8**. Si tu computador tiene menos de 10 hilos en total, pon lo máximo que puedas dejando al menos 2 para Windows.
4. Aceptar, arrancar de nuevo y volver a correr `nproc`.

**En VMware:** apaga la máquina → *Settings* → *Processors* → sube el número de núcleos.

Para saber cuántos hilos tiene tu procesador real en Windows: `Ctrl + Shift + Esc` → *Rendimiento* → *CPU* → "Procesadores lógicos".

### 6.5 Nota sobre procesadores Intel de 12.ª generación en adelante

Muchos procesadores recientes (por ejemplo el **i5-12450H** de Juliana) tienen dos tipos de núcleos: **P** (rendimiento, rápidos) y **E** (eficiencia, más lentos). Con 8 hilos, algunos caen en núcleos E, y eso hace que el speedup crezca menos de lo ideal. No es un error; hay que mencionarlo en el análisis.

---

## 7. Paso a paso con los scripts (recomendado)

### Paso 1. Poner el paquete en Linux

Descomprime la carpeta `taller-benchmarking-openmp` donde quieras, por ejemplo en tu carpeta personal. Luego, en la terminal:

```bash
cd ~/taller-benchmarking-openmp
ls
```

Debes ver `README.md`, `preparar.sh`, `correr.sh`, etc. Si la descomprimiste en otro lado, cambia la ruta del `cd`.

> Si te llegó como `.zip` en Descargas: `cd ~/Descargas && unzip taller-benchmarking-openmp.zip && cd taller-benchmarking-openmp`
> (si tu carpeta se llama `Downloads`, usa ese nombre).

### Paso 2. Revisar las imágenes

```bash
ls imagenes/
```

Deben estar las 4 imágenes, y el nombre de cada una debe **contener su tamaño**:

```
kitkat512.jpg   meow1024.jpg   onedirection2048.jpg   4096.jpg
```

Si no están, pídeselas a Juliana para que los resultados sean comparables. Si no hay forma, el script usa una imagen de ejemplo del repo redimensionada, pero entonces tus resultados no se pueden comparar directamente con los de ella.

### Paso 3. Preparar todo (una sola vez)

```bash
bash preparar.sh
```

Este script:

1. Revisa que tengas 8 procesadores (si no, te pregunta si quieres seguir).
2. Instala lo necesario con `apt`: compilador `g++`, `cmake`, `git`, OpenCV, Google Benchmark, doctest, y pandas/matplotlib para las gráficas. **Te pide tu contraseña**: mientras la escribes no aparece nada en pantalla, ni asteriscos. Es normal. Escríbela y presiona Enter.
3. Descarga el repo original de GitHub en `cpu-image-filters/` (fijado al commit del 17/12/2019).
4. Copia los archivos del taller y las imágenes.
5. Aplica los cambios de la [sección 4](#4-qué-se-le-cambió-al-repo-original).
6. Compila.

Tarda varios minutos la primera vez, sobre todo descargando OpenCV. Sabes que salió bien si al final dice:

```
 LISTO. Ejecutables en: .../cpu-image-filters/build/bin/
benchmark
filter-image
image-filters-bench
```

### Paso 4. Prueba rápida (2 repeticiones)

```bash
bash correr.sh 2
```

Tarda uno o dos minutos. Mientras corre verás líneas como:

```
Imagen 4096x4096
  kernel 11x11  secuencial            839.921 ms
  kernel 11x11  manual_omp    1 h      969.805 ms  Sp=0.87  Ep=0.87  OK
  kernel 11x11  manual_omp    2 h      495.674 ms  Sp=1.69  Ep=0.85  OK
  ...
  kernel 11x11  ia            8 h      220.011 ms  Sp=3.82  Ep=0.48  OK
  ...
  kernel 11x11  ia_opt        8 h       62.947 ms  Sp=13.34 Ep=1.67  OK
```

Revisa que **todas** las líneas terminen en `OK` y que al final diga:

```
Configuraciones que NO pasaron la validacion: 0
```

Esta prueba solo sirve para saber que todo funciona. **No uses estos números en el informe.**

### Paso 5. Corrida oficial (30 repeticiones)

Antes de lanzarla:

- Conecta el portátil a la corriente.
- Cierra el navegador y todo lo que no necesites, **tanto en Linux como en Windows** si estás en máquina virtual. Cualquier programa abierto le quita CPU al benchmark y ensucia las mediciones.
- En Windows, si puedes, pon el plan de energía en "Máximo rendimiento".

Luego:

```bash
bash correr.sh
```

Con 8 procesadores tarda unos **10 a 20 minutos**. Con menos, más. **No uses el computador mientras corre.** El script evita que el computador se suspenda mientras está trabajando.

Al terminar te dice dónde guardó todo, por ejemplo:

```
Resultados guardados en: /home/tu_usuario/taller-benchmarking-openmp/resultados/2026-10-13_15-30_30reps
Lineas en crudos: 7681  (con 30 reps deben ser 7681)
Configuraciones que NO pasaron la validacion: 0
```

Revisa que diga **7681** y **0**.

### Paso 6. Generar las gráficas

El mismo script te muestra el comando exacto al final. Es así (cambia la carpeta por la tuya):

```bash
python3 graficas/generar_graficas.py resultados/2026-10-13_15-30_30reps/resultados_resumen.csv
```

> Truco: escribe `resultados/` y presiona `Tab` para que la terminal complete el nombre de la carpeta.

Se crea una carpeta `graficas/` dentro de esa carpeta de resultados con **17 imágenes PNG**:

| Archivo | Qué muestra |
|---|---|
| `tiempo_<tamaño>.png` | Tiempo vs. hilos, con barras de desviación estándar. Un panel por kernel |
| `speedup_<tamaño>.png` | Speedup vs. hilos, con la línea ideal (Sp = p) |
| `eficiencia_<tamaño>.png` | Eficiencia vs. hilos, con la línea ideal (Ep = 1) |
| `escalabilidad_relativa_<tamaño>.png` | Cuánto mejora cada variante respecto a **su propio** tiempo con 1 hilo |
| `speedup_8hilos_por_tamano.png` | Speedup con 8 hilos según el tamaño de imagen |

Para abrir la carpeta en el explorador de archivos:

```bash
xdg-open resultados/
```

---

## 8. Paso a paso manual (si el script falla)

Haz esto solo si `preparar.sh` dio error y no lo pudiste resolver con la [sección 13](#13-problemas-frecuentes). Todos los comandos se corren **desde la carpeta del paquete** salvo donde se indique.

**8.1 Instalar dependencias:**

```bash
sudo apt update
sudo apt install -y git g++ cmake libopencv-dev libbenchmark-dev doctest-dev python3-pandas python3-matplotlib
```

**8.2 Descargar el repo original:**

```bash
git clone https://github.com/akowalew/cpu-image-filters.git
cd cpu-image-filters
git checkout 6f81b9c15a6817f52aecabc8823668abe2579da8
cd ..
```

**8.3 Copiar los archivos del taller:**

```bash
mkdir -p cpu-image-filters/taller cpu-image-filters/lib/src/ia
cp archivos/taller/* cpu-image-filters/taller/
cp archivos/lib/src/ia/* cpu-image-filters/lib/src/ia/
cp imagenes/*.jpg cpu-image-filters/assets/
```

**8.4 Aplicar los cambios** (ahora desde dentro del repo):

```bash
cd cpu-image-filters

# Registrar la carpeta taller/
echo "add_subdirectory(taller)" >> CMakeLists.txt

# Agregar las variantes de IA a la librería
sed -i 's|\tsrc/filter_seq.cpp|\tsrc/filter_seq.cpp\n\tsrc/ia/filter_ia.cpp\n\tsrc/ia/filter_ia_opt.cpp|' lib/CMakeLists.txt

# Corregir el programa de ejemplo
sed -i 's/strcmp(type, "seq") == 1/strcmp(type, "seq") == 0/' app/src/main.cpp
sed -i '/setenv("OMP_NUM_THREADS", "4", 1);/d' app/src/main.cpp
sed -i 's/if(argc < 4)/if(argc < 5)/' app/src/main.cpp
sed -i 's/#include <cstdlib>/#include <cstdlib>\n#include <cstring>/' app/src/main.cpp
```

Verifica que quedaron bien:

```bash
tail -1 CMakeLists.txt                 # debe decir: add_subdirectory(taller)
grep "src/" lib/CMakeLists.txt         # deben aparecer src/ia/filter_ia.cpp y src/ia/filter_ia_opt.cpp
grep -n "strcmp\|argc <" app/src/main.cpp
```

**8.5 Compilar:**

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_BENCHMARKING=ON
make -j$(nproc)
```

Debe terminar con `Built target benchmark` sin líneas rojas de `error`.

**8.6 Correr** (desde `cpu-image-filters/build`):

```bash
# Prueba rápida
./bin/benchmark ../assets/kitkat512.jpg ../assets/meow1024.jpg ../assets/onedirection2048.jpg ../assets/4096.jpg 2

# Corrida oficial (30 repeticiones), sin que se suspenda el computador
systemd-inhibit --what=sleep:idle ./bin/benchmark ../assets/kitkat512.jpg ../assets/meow1024.jpg ../assets/onedirection2048.jpg ../assets/4096.jpg
```

Las imágenes deben ir **en orden de tamaño**: 512, 1024, 2048, 4096. Si el número final se omite, hace 30 repeticiones.

**8.7 Guardar los resultados** (desde `build`). El benchmark escribe `resultados_resumen.csv` y `resultados_crudos.csv` en la carpeta desde donde lo corres y **los sobrescribe en cada corrida**, así que guárdalos:

```bash
mkdir -p ../../resultados/mi_corrida
cp resultados_*.csv ../../resultados/mi_corrida/
lscpu > ../../resultados/mi_corrida/hardware.txt
```

**8.8 Gráficas** (desde la carpeta del paquete):

```bash
cd ../..
python3 graficas/generar_graficas.py resultados/mi_corrida/resultados_resumen.csv
```

---

## 9. Probar las variantes por separado

### 9.1 Con imágenes reales (solo secuencial y manual)

El programa `filter-image` del repo filtra una imagen y la muestra en pantalla. Solo trae la versión secuencial y la manual. Desde `cpu-image-filters/build`:

```bash
# Secuencial (1 hilo), kernel 5x5
./bin/filter-image seq ../assets/meow1024.jpg salida_seq.png 5

# Paralela manual con 4 hilos, kernel 5x5
OMP_NUM_THREADS=4 ./bin/filter-image omp ../assets/meow1024.jpg salida_omp.png 5
```

Formato: `./bin/filter-image <seq|omp> <entrada> <salida> <tamaño_kernel>`. El kernel debe ser impar (3, 5, 7, 11).

- Abre dos ventanas: la imagen original y la filtrada (en escala de grises). **Para cerrarlas, haz clic en una y presiona cualquier tecla.**
- Necesita entorno gráfico. En WSL sin interfaz gráfica o por SSH falla con un error de Qt.
- Usa salida `.png`, no `.jpg`: JPG comprime con pérdida y no sirve para comparar.

Para verificar que las dos dan exactamente lo mismo:

```bash
cmp salida_seq.png salida_omp.png && echo "iguales"
```

Para ver la imagen guardada: `xdg-open salida_seq.png`.

### 9.2 Todas las variantes (con el benchmark)

Las variantes `ia` e `ia_opt` solo se pueden correr desde el benchmark. Para probarlas rápido, usa `bash correr.sh 1` (1 repetición).

Si quieres medir solo algunas variantes, edita `cpu-image-filters/taller/benchmark.cpp`, busca `PARALLEL_VARIANTS` y comenta con `//` las que no quieras:

```cpp
static const std::vector<Variant> PARALLEL_VARIANTS = {
    {"manual_omp", filter2d_8_omp},
    {"ia",         filter2d_8_ia},
    // {"ia_opt",     filter2d_8_ia_opt},   ← comentada, no se mide
};
```

Luego recompila desde `build` con `make -j$(nproc)`.

---

## 10. Cómo leer los resultados

### 10.1 Una línea de la terminal

```
  kernel  5x5   manual_omp    4 h      82.346 ms  Sp=2.79  Ep=0.70  OK
```

Se lee: con kernel 5×5, la versión **manual** con **4 hilos** tardó en promedio **82.3 ms** (media de 30 corridas), fue **2.79 veces más rápida** que la secuencial, cada hilo se aprovechó al **70 %**, y la imagen resultante es **idéntica** a la secuencial (`OK`). Si dice `DIFIERE`, esa variante tiene un error.

### 10.2 `resultados_resumen.csv`

Una fila por configuración (256 filas + encabezado):

| Columna | Significado |
|---|---|
| `variante` | `secuencial`, `manual_omp`, `ia` o `ia_opt` |
| `tamano` | Lado de la imagen en px (512, 1024, 2048, 4096) |
| `kernel` | Lado del kernel (3, 5, 7, 11) |
| `hilos` | Número de hilos (la secuencial siempre 1) |
| `media_ms` | Tiempo promedio de las 30 corridas |
| `desv_ms` | Desviación estándar muestral de las 30 corridas |
| `speedup` | `media de la secuencial / media de esta fila` |
| `eficiencia` | `speedup / hilos` |
| `valido` | `si` si la imagen es idéntica a la secuencial, `NO` si no |

Para verlo en la terminal: `column -s, -t resultados_resumen.csv | less` (sales con `q`). También se abre con LibreOffice Calc, Excel o Google Sheets.

### 10.3 `resultados_crudos.csv`

Las **7680 mediciones individuales** (columnas: variante, tamaño, kernel, hilos, número de repetición, tiempo en ms). Sirve como respaldo y para gráficas de dispersión o diagramas de caja.

### 10.4 Comandos útiles para revisar

```bash
# ¿Alguna configuración falló la validación? (no debe imprimir nada)
grep ",NO$" resultados_resumen.csv

# Solo las filas de una variante
grep "^ia," resultados_resumen.csv | column -s, -t

# Comparar variantes lado a lado, ordenado por tamaño, kernel e hilos
grep -vE "^(variante|secuencial)," resultados_resumen.csv | sort -t, -k2,2n -k3,3n -k4,4n | column -s, -t
```

### 10.5 Por qué 30 repeticiones

Ninguna ejecución tarda exactamente lo mismo: el sistema operativo interrumpe, la caché a veces tiene los datos y a veces no, el procesador cambia de frecuencia. Con 30 repeticiones el promedio es representativo, se puede calcular la desviación estándar y se puede afirmar si una variante es más rápida que otra (si la diferencia es mayor que la desviación). Además, el taller lo exige.

---

## 11. Resultados de referencia (Juliana)

Están en `resultados_juliana/` (CSV, `hardware.txt` y las 17 gráficas).

**Máquina:** Intel Core i5-12450H (4 núcleos P + 4 núcleos E, 12 hilos), máquina virtual KVM con **8 CPUs** asignadas, Xubuntu, GCC 13.3, compilado en modo Release (`-O3`). Las 240 configuraciones paralelas pasaron la validación.

**Ejemplo: imagen 4096×4096, kernel 11×11**

| Variante | 1 hilo | 8 hilos | Speedup con 8 hilos (vs. secuencial) |
|---|---|---|---|
| Secuencial | 839.9 ms | — | 1.00 |
| Paralelo manual | 969.8 ms | 221.6 ms | 3.79 |
| Paralelo IA | 980.6 ms | 220.0 ms | 3.82 |
| Paralelo IA + Opt. | 207.1 ms | 62.9 ms | 13.34 |

**Lo más importante que muestran los datos:**

1. **La IA igualó al código manual, pero no lo superó.** `manual_omp` e `ia` dan prácticamente los mismos tiempos en todas las configuraciones. Tiene sentido: los dos paralelizan el mismo algoritmo de la misma forma (un `parallel for` sobre las filas).

2. **IA + Opt. es 4 a 5 veces más rápida que las otras dos**, incluso con 1 solo hilo. La ventaja viene de la vectorización, **no** de paralelizar mejor. Esto se ve en `escalabilidad_relativa_4096.png`: las tres variantes mejoran casi igual respecto a su propio tiempo con 1 hilo (unas 4 veces con 8 hilos).

3. **La eficiencia de `ia_opt` sale mayor que 1.** No es un speedup "superlineal": pasa porque T₁ es la secuencial sin vectorizar. Hay que explicarlo en el análisis para que no parezca un error.

4. **Con 1 hilo, `manual_omp` e `ia` son más lentas que la secuencial** en kernels de 5×5 y 7×7 (por ejemplo 322 ms contra 229 ms). Es el costo de OpenMP más el hecho de que el compilador optimiza peor el código dentro de una región paralela.

5. **No se llega al ideal de 8 con 8 hilos** (se queda en ~4). Las causas probables: los núcleos E son más lentos y, con `schedule(static)`, los hilos en núcleos P terminan antes y esperan; el ancho de banda de memoria es compartido; y la máquina virtual agrega overhead y ruido.

6. **Con 16 hilos** (más que las 8 CPUs) el rendimiento se estanca o empeora, como se esperaba.

7. **Con imágenes de 512 px y kernel 3×3** el trabajo dura menos de 1 ms, así que las mediciones son muy ruidosas y paralelizar casi no ayuda.

8. **Primera corrida con 2 CPUs:** Juliana corrió primero con la máquina virtual en 2 CPUs y el speedup se estancaba en ~2 para todas las variantes. Por eso se subió a 8.

Tus números no van a ser idénticos (depende del procesador, de si es máquina virtual, de qué más esté corriendo), pero las **tendencias** deberían ser las mismas.

---

## 12. Cómo agregar tu propia versión de IA

Si quieres generar **tu propia** variante con otra IA (o con otro prompt) y compararla, se hace así. Todo desde `cpu-image-filters/`.

**1. Copia el código secuencial:**

```bash
cat lib/src/filter_seq.cpp
```

**2. Pídele a la IA**, en un chat nuevo, algo como el prompt de la [sección 3.3](#33-paralelo-ia), pero cambiando el nombre de la función, por ejemplo a `filter2d_8_ia2`. Guarda capturas del prompt, la respuesta, el modelo y la fecha.

**3. Guarda la respuesta tal cual:**

```bash
nano lib/src/ia/filter_ia2.cpp
```

Pega con `Ctrl+Shift+V`, guarda con `Ctrl+O` y Enter, sal con `Ctrl+X`.

**4. Agrégala a la librería:**

```bash
sed -i 's|\tsrc/ia/filter_ia_opt.cpp|\tsrc/ia/filter_ia_opt.cpp\n\tsrc/ia/filter_ia2.cpp|' lib/CMakeLists.txt
grep "src/ia" lib/CMakeLists.txt
```

**5. Regístrala en el benchmark:**

```bash
nano taller/benchmark.cpp
```

Busca `PARALLEL_VARIANTS` (con `Ctrl+W`) y agrega dos líneas:

```cpp
void filter2d_8_ia(const ImageU8& src, ImageU8& dst, const Image32F& kernel);
void filter2d_8_ia_opt(const ImageU8& src, ImageU8& dst, const Image32F& kernel);
void filter2d_8_ia2(const ImageU8& src, ImageU8& dst, const Image32F& kernel);   // ← nueva

static const std::vector<Variant> PARALLEL_VARIANTS = {
    {"manual_omp", filter2d_8_omp},
    {"ia",         filter2d_8_ia},
    {"ia_opt",     filter2d_8_ia_opt},
    {"ia2",        filter2d_8_ia2},                                             // ← nueva
};
```

**6. Recompila y corre:**

```bash
cd build
cmake ..
make -j$(nproc)
cd ../..
bash correr.sh 2
```

Si no compila o sale `DIFIERE`, **anótalo**: es un resultado válido sobre la calidad del código de la IA. Para corregirlo, devuélvele los errores a la misma IA y documenta cuántos intentos necesitó.

> El script de gráficas solo dibuja `manual_omp`, `ia` e `ia_opt`. Para incluir otra variante, agrégala a la lista `V` al principio de `graficas/generar_graficas.py`.

---

## 13. Problemas frecuentes

| Síntoma | Causa | Solución |
|---|---|---|
| `nproc` dice 2 (o menos de 8) | La máquina virtual tiene pocos CPUs | [Sección 6.4](#64-si-estás-en-una-máquina-virtual-y-nproc-dice-menos-de-8) |
| `Could not find a package configuration file provided by "OpenCV"` | OpenCV no está instalado | `sudo apt install -y libopencv-dev`, luego borra `build/` y vuelve a compilar |
| Lo mismo con `benchmark` o `doctest` | Falta esa librería | `sudo apt install -y libbenchmark-dev doctest-dev` |
| `Unable to locate package libopencv-dev` | Falta el repositorio *universe* | `sudo add-apt-repository universe && sudo apt update`, y repite la instalación |
| `Could not get lock /var/lib/dpkg/lock` | Otra instalación está corriendo | Espera unos minutos o reinicia |
| `No such file or directory` al correr `./bin/...` | Estás en la carpeta equivocada | `pwd` para ver dónde estás; los ejecutables se corren desde `cpu-image-filters/build` |
| `cat: lib/src/...: No such file or directory` | Estás en `build` u otra carpeta | `cd` a la raíz `cpu-image-filters` |
| `basic_string: construction from null is not valid` | A `filter-image` le faltó el tamaño del kernel | Agrega el número al final: `... salida.png 5` |
| `qt.qpa.xcb: could not connect to display` | `filter-image` necesita pantalla | Úsalo desde un escritorio Linux, no por SSH ni WSL sin interfaz gráfica |
| `'size_t' does not name a type` o `'clamp_to' was not declared` | El código de IA está incompleto | Ver [sección 3.3](#33-paralelo-ia); el `filter_ia.cpp` de este paquete ya está corregido |
| `Configuring incomplete, errors occurred!` | Algo falló en CMake | Lee las líneas `CMake Error` de más arriba; casi siempre es una librería faltante |
| CMake sigue fallando después de instalar lo que faltaba | Caché del intento anterior | `cd cpu-image-filters && rm -rf build` y vuelve a `bash preparar.sh` |
| Salen filas `DIFIERE` / `NO` | Esa variante no produce la misma imagen | Si es una variante nueva, revisa el código; las del paquete deben dar todas `OK` |
| No aparecen filas de `ia` en los resultados | La variante no está registrada o el `make` falló | Revisa la [sección 12](#12-cómo-agregar-tu-propia-versión-de-ia), pasos 4 y 5, y que el `make` termine sin errores |
| `crudos` no tiene 7681 líneas | Corrida incompleta o con otro número de repeticiones | Vuelve a correr `bash correr.sh` sin número |
| Error de NumPy al generar gráficas (`compiled using NumPy 1.x`) | Mezcla de paquetes de `pip` y de `apt` | `sudo apt install --reinstall python3-numpy python3-pandas python3-matplotlib` |
| VS Code se congela | — | `pkill code` y edita con `nano` |
| La contraseña "no se escribe" | Es normal en Linux | Escríbela a ciegas y presiona Enter |

---

## 14. Transparencia sobre el uso de IA

Para el informe es importante dejar claro qué hizo cada quién:

| Parte | Origen |
|---|---|
| Secuencial y Paralelo manual | Repositorio de akowalew y marcin-jamroz, diciembre de 2019, escrito a mano |
| Paralelo IA | Generado por IA a partir del código secuencial; segunda respuesta, tras devolverle errores de compilación |
| Optimización de IA + Opt. | La idea de invertir los bucles y el código final se obtuvieron en una consulta adicional a un asistente de IA |
| Programa de medición (`benchmark.cpp`), scripts y gráficas | Escritos con ayuda de un asistente de IA |
| Correcciones a `main.cpp` | Corrección de errores del programa de ejemplo del repo original |

En la metodología del artículo conviene decir explícitamente que la variante "IA + Opt." se optimizó con asistencia de IA y no de forma completamente manual, o preguntarle al profesor si prefiere que ese ajuste lo haga cada estudiante.

---

## 15. Mini glosario de Linux

| Comando | Qué hace |
|---|---|
| `pwd` | Muestra en qué carpeta estás |
| `ls` | Lista lo que hay en la carpeta |
| `cd carpeta` | Entra a una carpeta |
| `cd ..` | Sube una carpeta |
| `cd ~` | Va a tu carpeta personal |
| `cat archivo` | Muestra el contenido de un archivo |
| `nano archivo` | Abre un editor de texto en la terminal (`Ctrl+O` guarda, `Ctrl+X` sale, `Ctrl+W` busca) |
| `cp origen destino` | Copia archivos |
| `rm archivo` | Borra un archivo (no hay papelera) |
| `mkdir -p carpeta` | Crea una carpeta |
| `xdg-open archivo` | Abre un archivo o carpeta con el programa por defecto |
| `nproc` | Número de procesadores |
| `Tab` | Autocompleta nombres de archivos y carpetas |
| `Ctrl+C` | Detiene el programa que está corriendo |
| `Ctrl+Shift+C` / `Ctrl+Shift+V` | Copiar / pegar en la terminal |

---

**Repositorio original:** https://github.com/akowalew/cpu-image-filters (licencia MIT), autores akowalew y marcin-jamroz.
