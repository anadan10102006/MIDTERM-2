# Taller CL11 · Benchmarking de código C++ paralelo generado por IA (OpenMP)

Programación Paralela · Pontificia Universidad Javeriana Cali

Integrantes: Maria Lucía Castillo García, Juliana González Sánchez, Ana Daniela Paredes Tovar

---

## Índice

1. [¿Qué hace este experimento?](#1-qué-hace-este-experimento)
2. [Las cuatro variantes que se comparan](#2-las-cuatro-variantes-que-se-comparan)
3. [Qué se le cambió al repo original](#3-qué-se-le-cambió-al-repo-original)
4. [Qué trae este paquete](#4-qué-trae-este-paquete)
5. [Requisitos: Linux y 8 procesadores](#5-requisitos-linux-y-8-procesadores)
6. [Paso a paso con los scripts (recomendado)](#6-paso-a-paso-con-los-scripts-recomendado)
7. [Paso a paso manual (si el script falla)](#7-paso-a-paso-manual-si-el-script-falla)
8. [Probar las variantes por separado](#8-probar-las-variantes-por-separado)
9. [Cómo leer los resultados](#9-cómo-leer-los-resultados)
10. [Resultados de referencia](#10-resultados-de-referencia)
11. [Cómo agregar una versión de IA propia](#11-cómo-agregar-una-versión-de-ia-propia)
12. [Problemas frecuentes](#12-problemas-frecuentes)
13. [Transparencia sobre el uso de IA](#13-transparencia-sobre-el-uso-de-ia)
14. [Mini glosario de Linux](#14-mini-glosario-de-linux)

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

## 2. Las cuatro variantes que se comparan

Las cuatro hacen **exactamente la misma operación** y producen **exactamente la misma imagen**. Lo único que cambia es cómo está escrito el código.

| Variante | Nombre en los resultados | Archivo | Autoría |
|---|---|---|---|
| C++ Secuencial | `secuencial` | `lib/src/filter_seq.cpp` | Autor original del repo (2019), a mano |
| C++ Paralelo Manual | `manual_omp` | `lib/src/filter_omp.cpp` | Autor original del repo (2019), a mano |
| C++ Paralelo IA | `ia` | `lib/src/ia/filter_ia.cpp` | Generado por IA |
| C++ Paralelo IA + Opt. | `ia_opt` | `lib/src/ia/filter_ia_opt.cpp` | IA + optimización posterior |

### 2.1 Secuencial (la base de referencia)

Proviene del repositorio **[akowalew/cpu-image-filters](https://github.com/akowalew/cpu-image-filters)**, un proyecto universitario de diciembre de 2019 (anterior a la existencia de asistentes de código con IA). Usa un solo hilo. Su tiempo es el **T₁** con el que se calcula el speedup de todas las demás variantes.

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

### 2.2 Paralelo Manual

Proviene del **mismo repositorio de 2019**. Es el mismo algoritmo que la secuencial, con una línea de OpenMP antes del primer bucle:

```cpp
#pragma omp parallel for
for(auto i = 0; i < height; ++i)
```

Esta directiva reparte las filas de la imagen entre los hilos. Cada hilo procesa un grupo de filas de forma independiente.

> **Justificación de la elección:** el taller exige un código paralelo escrito completamente a mano, obtenido de GitHub o GitLab y anterior a 2020, para garantizar que no tuvo asistencia de IA. Este repositorio cumple los requisitos: primer commit el 8/12/2019, último commit el 17/12/2019, y usa OpenMP.

### 2.3 Paralelo IA

Se obtuvo solicitando a una IA que paralelizara el código secuencial. El prompt fue:

```
Paraleliza con OpenMP la siguiente función de convolución 2D en C++.
Mantén exactamente la misma firma y el mismo resultado numérico.
Renombra la función a filter2d_8_ia. Devuelve solo el código.

[contenido completo de lib/src/filter_seq.cpp]
```

**La primera respuesta no compiló.** Le faltaban la cabecera `<cstddef>` (para `size_t`) y la función auxiliar `clamp_to`. Se devolvieron los errores de compilación a la misma IA y la segunda respuesta sí compiló. Esa segunda respuesta es la que se encuentra en `filter_ia.cpp`, sin modificaciones.

Cambios introducidos por la IA respecto a la versión manual:

- Agregó `schedule(static)` al `#pragma omp parallel for`.
- En lugar de desplazar punteros de fila en fila, calcula el puntero de cada fila a partir de `i`, de modo que cada iteración es independiente.
- Mantuvo el mismo orden de suma, por lo que el resultado es idéntico.

En la práctica, su rendimiento es casi igual al de la versión manual.

### 2.4 Paralelo IA + Opt.

Parte de la versión de IA y le aplica una optimización: **invertir el orden de los bucles** para que el compilador pueda usar instrucciones **SIMD** (vectoriales), que realizan varias multiplicaciones a la vez.

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

En la versión original, el bucle más interno es el del kernel (de 3 a 11 iteraciones) y todas las sumas se acumulan en una sola variable, lo que impide la vectorización. En esta variante, el bucle más interno recorre toda la fila (cientos o miles de píxeles contiguos en memoria), cada uno con su propio acumulador, lo que sí permite vectorizar.

Detalles importantes:

- Cada hilo tiene su **propio** arreglo `acum` (se declara dentro de `#pragma omp parallel`). Si fuera compartido, los hilos sobrescribirían los datos de los demás.
- Cada píxel suma en el mismo orden (primero `m`, luego `n`), por lo que **el resultado es idéntico bit a bit** al de la versión secuencial.

> **Nota:** esta optimización también se realizó con ayuda de IA (ver [sección 13](#13-transparencia-sobre-el-uso-de-ia)).

---

## 3. Qué se le cambió al repo original

El algoritmo de las versiones **secuencial** y **manual** (`filter_seq.cpp` y `filter_omp.cpp`) **no se modificó**. Los archivos agregados o corregidos son:

| Archivo | Cambio | Motivo |
|---|---|---|
| `taller/benchmark.cpp` | **Nuevo.** Programa de medición | El repo no medía lo que exige el taller |
| `taller/CMakeLists.txt` | **Nuevo.** Instrucciones de compilación del benchmark | Para compilarlo junto con el resto del proyecto |
| `lib/src/ia/filter_ia.cpp` | **Nuevo.** Variante IA | — |
| `lib/src/ia/filter_ia_opt.cpp` | **Nuevo.** Variante IA + Opt. | — |
| `CMakeLists.txt` (raíz) | Se agregó `add_subdirectory(taller)` al final | Para que CMake compile la carpeta `taller/` |
| `lib/CMakeLists.txt` | Se agregaron `src/ia/filter_ia.cpp` y `src/ia/filter_ia_opt.cpp` a la librería | Para que las variantes de IA se compilen |
| `app/src/main.cpp` | `strcmp(type, "seq") == 1` → `== 0` | Error del original: siempre ejecutaba la versión OpenMP aunque se indicara `seq` |
| `app/src/main.cpp` | Se eliminó `setenv("OMP_NUM_THREADS", "4", 1);` | Forzaba 4 hilos e impedía elegir el número |
| `app/src/main.cpp` | `argc < 4` → `argc < 5` | Si faltaba el tamaño del kernel, el programa fallaba en lugar de mostrar el modo de uso |
| `app/src/main.cpp` | Se agregó `#include <cstring>` | Necesario para `strcmp` |

Los cambios en `main.cpp` solo afectan al programa de ejemplo `filter-image`, **no** al benchmark ni a los algoritmos. El script `preparar.sh` aplica todos estos cambios automáticamente.

### Funcionamiento del programa de medición (`taller/benchmark.cpp`)

1. Carga las 4 imágenes en escala de grises. Si alguna no mide exactamente 512, 1024, 2048 o 4096 px, la redimensiona y lo informa.
2. Para cada imagen y cada kernel:
   - Ejecuta la **secuencial** 30 veces; ese es T₁ y la imagen de referencia.
   - Para cada variante paralela y cada número de hilos (1, 2, 4, 8, 16):
     - Fija los hilos con `omp_set_num_threads(p)`.
     - Realiza 1 corrida de **calentamiento** que no se contabiliza.
     - Realiza 30 corridas, midiendo **solo la llamada al filtro** con `omp_get_wtime()`.
     - Compara la última salida contra la secuencial, píxel por píxel.
3. Guarda todo en dos archivos CSV.

Usa `omp_set_dynamic(0)` para que OpenMP utilice exactamente los hilos solicitados.

---

## 4. Qué trae este paquete

```
taller-benchmarking-openmp/
├── README.md                    ← este archivo
├── preparar.sh                  ← instala todo y compila (se ejecuta una vez)
├── correr.sh                    ← ejecuta el benchmark y guarda los resultados
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
└── resultados_referencia/       ← resultados de referencia (8 CPUs, 30 reps)
    ├── resultados_resumen.csv
    ├── resultados_crudos.csv
    ├── hardware.txt
    └── graficas/
```

Después de ejecutar los scripts aparecen dos carpetas nuevas:

```
├── cpu-image-filters/           ← el repo descargado, con los cambios y compilado
└── resultados/                  ← los resultados, una subcarpeta por corrida
```

---

## 5. Requisitos: Linux y 8 procesadores

### 5.1 Sistema operativo

Se requiere **Linux**, idealmente **Ubuntu 22.04 / 24.04** o derivados (Xubuntu, Kubuntu, Linux Mint). Los resultados de referencia se obtuvieron en **Xubuntu dentro de una máquina virtual**.

En **Windows** hay dos opciones:

- **Máquina virtual** (VirtualBox, VMware) con Ubuntu o Xubuntu instalado.
- **WSL** (Ubuntu dentro de Windows). En PowerShell como administrador:

  ```powershell
  wsl --install -d Ubuntu-24.04
  ```

  Tras reiniciar, se abre "Ubuntu" desde el menú de inicio y se obtiene una terminal de Linux. WSL usa por defecto todos los núcleos del computador.

### 5.2 Abrir la terminal

En Ubuntu/Xubuntu: `Ctrl + Alt + T`, o buscar "Terminal" en el menú.

### 5.3 Verificar que se dispone de 8 procesadores

```bash
nproc
```

**Debe indicar 8 o más.** Si indica menos, el experimento no refleja correctamente la escalabilidad: con 2 procesadores, por ejemplo, el speedup se estanca en 2 sin importar cuántos hilos se soliciten. Este comportamiento se observó en una corrida preliminar.

Para ver más detalle:

```bash
lscpu | grep -E "^CPU\(s\)|Model name|Thread|Core|Socket|Hypervisor"
```

Si aparece `Hypervisor vendor`, el sistema se ejecuta en una máquina virtual.

### 5.4 Si se usa una máquina virtual y `nproc` indica menos de 8

**En VirtualBox:**

1. Apagar Linux **por completo** (no suspender ni guardar estado).
2. En VirtualBox, seleccionar la máquina → **Configuración** → **Sistema** → pestaña **Procesador**.
3. Subir **Procesador(es)** a **8**. Si el computador tiene menos de 10 hilos en total, asignar el máximo posible dejando al menos 2 para el sistema anfitrión.
4. Aceptar, arrancar de nuevo y volver a ejecutar `nproc`.

**En VMware:** apagar la máquina → *Settings* → *Processors* → aumentar el número de núcleos.

Para conocer los hilos del procesador real en Windows: `Ctrl + Shift + Esc` → *Rendimiento* → *CPU* → "Procesadores lógicos".

### 5.5 Nota sobre procesadores Intel de 12.ª generación en adelante

Muchos procesadores recientes (por ejemplo el **i5-12450H** utilizado en los resultados de referencia) tienen dos tipos de núcleos: **P** (rendimiento, rápidos) y **E** (eficiencia, más lentos). Con 8 hilos, algunos se asignan a núcleos E, lo que hace que el speedup crezca menos de lo ideal. No es un error y debe mencionarse en el análisis.

---

## 6. Paso a paso con los scripts (recomendado)

### Paso 1. Ubicar el paquete en Linux

Descomprimir la carpeta `taller-benchmarking-openmp` en la ubicación deseada, por ejemplo la carpeta personal. Luego, en la terminal:

```bash
cd ~/taller-benchmarking-openmp
ls
```

Deben aparecer `README.md`, `preparar.sh`, `correr.sh`, etc. Si la carpeta se descomprimió en otra ubicación, se debe ajustar la ruta del `cd`.

> Si el paquete se encuentra como `.zip` en Descargas: `cd ~/Descargas && unzip taller-benchmarking-openmp.zip && cd taller-benchmarking-openmp`
> (si la carpeta se llama `Downloads`, usar ese nombre).

### Paso 2. Revisar las imágenes

```bash
ls imagenes/
```

Deben estar las 4 imágenes, y el nombre de cada una debe **contener su tamaño**:

```
kitkat512.jpg   meow1024.jpg   onedirection2048.jpg   4096.jpg
```

Si no están disponibles, el script usa una imagen de ejemplo del repo redimensionada; en ese caso, los resultados no son directamente comparables con los de referencia.

### Paso 3. Preparar el entorno (una sola vez)

```bash
bash preparar.sh
```

Este script:

1. Verifica que el sistema tenga 8 procesadores (si no, pregunta si se desea continuar).
2. Instala las dependencias con `apt`: compilador `g++`, `cmake`, `git`, OpenCV, Google Benchmark, doctest, y pandas/matplotlib para las gráficas. **Solicita la contraseña de usuario**: mientras se escribe no aparece nada en pantalla, ni asteriscos. Es el comportamiento normal; se escribe la contraseña y se presiona Enter.
3. Descarga el repo original de GitHub en `cpu-image-filters/` (fijado al commit del 17/12/2019).
4. Copia los archivos del taller y las imágenes.
5. Aplica los cambios de la [sección 3](#3-qué-se-le-cambió-al-repo-original).
6. Compila.

Tarda varios minutos la primera vez, principalmente por la descarga de OpenCV. La ejecución fue exitosa si al final aparece:

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

Tarda entre uno y dos minutos. Durante la ejecución se muestran líneas como:

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

Se debe verificar que **todas** las líneas terminen en `OK` y que al final se indique:

```
Configuraciones que NO pasaron la validacion: 0
```

Esta prueba solo confirma el correcto funcionamiento. **Sus valores no deben usarse en el informe.**

### Paso 5. Corrida oficial (30 repeticiones)

Antes de lanzarla:

- Conectar el portátil a la corriente.
- Cerrar el navegador y cualquier programa innecesario, **tanto en Linux como en Windows** si se usa máquina virtual. Cualquier programa abierto consume CPU y afecta las mediciones.
- En Windows, de ser posible, configurar el plan de energía en "Máximo rendimiento".

Luego:

```bash
bash correr.sh
```

Con 8 procesadores tarda entre **10 y 20 minutos**; con menos, más. **No se debe usar el computador durante la ejecución.** El script evita que el equipo se suspenda mientras trabaja.

Al terminar se indica dónde se guardaron los resultados, por ejemplo:

```
Resultados guardados en: /home/tu_usuario/taller-benchmarking-openmp/resultados/2026-10-13_15-30_30reps
Lineas en crudos: 7681  (con 30 reps deben ser 7681)
Configuraciones que NO pasaron la validacion: 0
```

Se debe verificar que los valores sean **7681** y **0**.

### Paso 6. Generar las gráficas

El mismo script muestra al final el comando exacto, con la forma (ajustando la carpeta):

```bash
python3 graficas/generar_graficas.py resultados/2026-10-13_15-30_30reps/resultados_resumen.csv
```

Se crea una carpeta `graficas/` dentro de la carpeta de resultados con **17 imágenes PNG**:

| Archivo | Qué muestra |
|---|---|
| `tiempo_<tamaño>.png` | Tiempo vs. hilos, con barras de desviación estándar. Un panel por kernel |
| `speedup_<tamaño>.png` | Speedup vs. hilos, con la línea ideal (Sp = p) |
| `eficiencia_<tamaño>.png` | Eficiencia vs. hilos, con la línea ideal (Ep = 1) |
| `escalabilidad_relativa_<tamaño>.png` | Mejora de cada variante respecto a **su propio** tiempo con 1 hilo |
| `speedup_8hilos_por_tamano.png` | Speedup con 8 hilos según el tamaño de imagen |

Para abrir la carpeta en el explorador de archivos:

```bash
xdg-open resultados/
```

---

## 7. Paso a paso manual (si el script falla)

Este procedimiento solo debe seguirse si `preparar.sh` produjo un error que no se pudo resolver con la [sección 12](#12-problemas-frecuentes). Todos los comandos se ejecutan **desde la carpeta del paquete**, salvo donde se indique lo contrario.

**7.1 Instalar dependencias:**

```bash
sudo apt update
sudo apt install -y git g++ cmake libopencv-dev libbenchmark-dev doctest-dev python3-pandas python3-matplotlib
```

**7.2 Descargar el repo original:**

```bash
git clone https://github.com/akowalew/cpu-image-filters.git
cd cpu-image-filters
git checkout 6f81b9c15a6817f52aecabc8823668abe2579da8
cd ..
```

**7.3 Copiar los archivos del taller:**

```bash
mkdir -p cpu-image-filters/taller cpu-image-filters/lib/src/ia
cp archivos/taller/* cpu-image-filters/taller/
cp archivos/lib/src/ia/* cpu-image-filters/lib/src/ia/
cp imagenes/*.jpg cpu-image-filters/assets/
```

**7.4 Aplicar los cambios** (ahora desde dentro del repo):

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

Verificar que los cambios se aplicaron correctamente:

```bash
tail -1 CMakeLists.txt                 # debe decir: add_subdirectory(taller)
grep "src/" lib/CMakeLists.txt         # deben aparecer src/ia/filter_ia.cpp y src/ia/filter_ia_opt.cpp
grep -n "strcmp\|argc <" app/src/main.cpp
```

**7.5 Compilar:**

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DBUILD_BENCHMARKING=ON
make -j$(nproc)
```

Debe terminar con `Built target benchmark` y sin líneas de `error`.

**7.6 Ejecutar** (desde `cpu-image-filters/build`):

```bash
# Prueba rápida
./bin/benchmark ../assets/kitkat512.jpg ../assets/meow1024.jpg ../assets/onedirection2048.jpg ../assets/4096.jpg 2

# Corrida oficial (30 repeticiones), sin que se suspenda el computador
systemd-inhibit --what=sleep:idle ./bin/benchmark ../assets/kitkat512.jpg ../assets/meow1024.jpg ../assets/onedirection2048.jpg ../assets/4096.jpg
```

Las imágenes deben indicarse **en orden de tamaño**: 512, 1024, 2048, 4096. Si se omite el número final, se hacen 30 repeticiones.

**7.7 Guardar los resultados** (desde `build`). El benchmark escribe `resultados_resumen.csv` y `resultados_crudos.csv` en la carpeta desde donde se ejecuta y **los sobrescribe en cada corrida**, por lo que deben guardarse:

```bash
mkdir -p ../../resultados/mi_corrida
cp resultados_*.csv ../../resultados/mi_corrida/
lscpu > ../../resultados/mi_corrida/hardware.txt
```

**7.8 Gráficas** (desde la carpeta del paquete):

```bash
cd ../..
python3 graficas/generar_graficas.py resultados/mi_corrida/resultados_resumen.csv
```

---

## 8. Probar las variantes por separado

### 8.1 Con imágenes reales (solo secuencial y manual)

El programa `filter-image` del repo filtra una imagen y la muestra en pantalla. Solo incluye la versión secuencial y la manual. Desde `cpu-image-filters/build`:

```bash
# Secuencial (1 hilo), kernel 5x5
./bin/filter-image seq ../assets/meow1024.jpg salida_seq.png 5

# Paralela manual con 4 hilos, kernel 5x5
OMP_NUM_THREADS=4 ./bin/filter-image omp ../assets/meow1024.jpg salida_omp.png 5
```

Formato: `./bin/filter-image <seq|omp> <entrada> <salida> <tamaño_kernel>`. El kernel debe ser impar (3, 5, 7, 11).

- Abre dos ventanas: la imagen original y la filtrada (en escala de grises). **Para cerrarlas, se hace clic en una de ellas y se presiona cualquier tecla.**
- Requiere entorno gráfico. En WSL sin interfaz gráfica o por SSH falla con un error de Qt.
- Se debe usar salida `.png` y no `.jpg`: JPG comprime con pérdida y no sirve para comparar.

Para verificar que ambas versiones producen exactamente el mismo resultado:

```bash
cmp salida_seq.png salida_omp.png && echo "iguales"
```

Para ver la imagen guardada: `xdg-open salida_seq.png`.

### 8.2 Todas las variantes (con el benchmark)

Las variantes `ia` e `ia_opt` solo pueden ejecutarse desde el benchmark. Para probarlas rápidamente, se usa `bash correr.sh 1` (1 repetición).

Para medir solo algunas variantes, se edita `cpu-image-filters/taller/benchmark.cpp`, se busca `PARALLEL_VARIANTS` y se comentan con `//` las que no se deseen:

```cpp
static const std::vector<Variant> PARALLEL_VARIANTS = {
    {"manual_omp", filter2d_8_omp},
    {"ia",         filter2d_8_ia},
    // {"ia_opt",     filter2d_8_ia_opt},   ← comentada, no se mide
};
```

Luego se recompila desde `build` con `make -j$(nproc)`.

---

## 9. Cómo leer los resultados

### 9.1 Una línea de la terminal

```
  kernel  5x5   manual_omp    4 h      82.346 ms  Sp=2.79  Ep=0.70  OK
```

Se interpreta así: con kernel 5×5, la versión **manual** con **4 hilos** tardó en promedio **82.3 ms** (media de 30 corridas), fue **2.79 veces más rápida** que la secuencial, cada hilo se aprovechó al **70 %**, y la imagen resultante es **idéntica** a la secuencial (`OK`). Si aparece `DIFIERE`, esa variante tiene un error.

### 9.2 `resultados_resumen.csv`

Una fila por configuración (256 filas más el encabezado):

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

Para verlo en la terminal: `column -s, -t resultados_resumen.csv | less` (se sale con `q`). También puede abrirse con LibreOffice Calc, Excel o Google Sheets.

### 9.3 `resultados_crudos.csv`

Contiene las **7680 mediciones individuales** (columnas: variante, tamaño, kernel, hilos, número de repetición, tiempo en ms). Sirve como respaldo y para gráficas de dispersión o diagramas de caja.

### 9.4 Comandos útiles para revisar

```bash
# ¿Alguna configuración falló la validación? (no debe imprimir nada)
grep ",NO$" resultados_resumen.csv

# Solo las filas de una variante
grep "^ia," resultados_resumen.csv | column -s, -t

# Comparar variantes lado a lado, ordenado por tamaño, kernel e hilos
grep -vE "^(variante|secuencial)," resultados_resumen.csv | sort -t, -k2,2n -k3,3n -k4,4n | column -s, -t
```

### 9.5 Por qué 30 repeticiones

Ninguna ejecución tarda exactamente lo mismo: el sistema operativo introduce interrupciones, la caché a veces contiene los datos y a veces no, y el procesador cambia de frecuencia. Con 30 repeticiones el promedio es representativo, se puede calcular la desviación estándar y se puede afirmar si una variante es más rápida que otra (cuando la diferencia es mayor que la desviación). Además, el taller lo exige.

---

## 10. Resultados de referencia

Se encuentran en `resultados_referencia/` (CSV, `hardware.txt` y las 17 gráficas).

**Máquina:** Intel Core i5-12450H (4 núcleos P + 4 núcleos E, 12 hilos), máquina virtual KVM con **8 CPUs** asignadas, Xubuntu, GCC 13.3, compilado en modo Release (`-O3`). Las 240 configuraciones paralelas pasaron la validación.

**Ejemplo: imagen 4096×4096, kernel 11×11**

| Variante | 1 hilo | 8 hilos | Speedup con 8 hilos (vs. secuencial) |
|---|---|---|---|
| Secuencial | 839.9 ms | — | 1.00 |
| Paralelo manual | 969.8 ms | 221.6 ms | 3.79 |
| Paralelo IA | 980.6 ms | 220.0 ms | 3.82 |
| Paralelo IA + Opt. | 207.1 ms | 62.9 ms | 13.34 |

**Principales observaciones:**

1. **La IA igualó al código manual, pero no lo superó.** `manual_omp` e `ia` presentan prácticamente los mismos tiempos en todas las configuraciones. Es coherente: ambas paralelizan el mismo algoritmo de la misma forma (un `parallel for` sobre las filas).

2. **IA + Opt. es entre 4 y 5 veces más rápida que las otras dos**, incluso con 1 solo hilo. La ventaja proviene de la vectorización, **no** de una mejor paralelización. Esto se observa en `escalabilidad_relativa_4096.png`: las tres variantes mejoran casi igual respecto a su propio tiempo con 1 hilo (alrededor de 4 veces con 8 hilos).

3. **La eficiencia de `ia_opt` es mayor que 1.** No corresponde a un speedup superlineal: ocurre porque T₁ es la secuencial sin vectorizar. Debe explicarse en el análisis para evitar que parezca un error.

4. **Con 1 hilo, `manual_omp` e `ia` son más lentas que la secuencial** en kernels de 5×5 y 7×7 (por ejemplo 322 ms contra 229 ms). Se debe al costo de OpenMP y a que el compilador optimiza peor el código dentro de una región paralela.

5. **No se alcanza el ideal de 8 con 8 hilos** (el speedup se queda en ~4). Las causas probables son: los núcleos E son más lentos y, con `schedule(static)`, los hilos en núcleos P terminan antes y esperan; el ancho de banda de memoria es compartido; y la máquina virtual agrega overhead y ruido.

6. **Con 16 hilos** (más que las 8 CPUs) el rendimiento se estanca o empeora, como se esperaba.

7. **Con imágenes de 512 px y kernel 3×3** el trabajo dura menos de 1 ms, por lo que las mediciones son muy ruidosas y paralelizar casi no aporta.

8. **Corrida preliminar con 2 CPUs:** con la máquina virtual configurada con 2 CPUs, el speedup se estancó en ~2 para todas las variantes. Por esta razón se aumentó a 8.

Los valores obtenidos en otro equipo no serán idénticos (dependen del procesador, de si es máquina virtual y de la carga del sistema), pero las **tendencias** deberían ser las mismas.

---

## 11. Cómo agregar una versión de IA propia

Para generar una variante propia con otra IA (o con otro prompt) y compararla, se procede así. Todo desde `cpu-image-filters/`.

**1. Copiar el código secuencial:**

```bash
cat lib/src/filter_seq.cpp
```

**2. Solicitar a la IA**, en un chat nuevo, un prompt como el de la [sección 2.3](#23-paralelo-ia), cambiando el nombre de la función, por ejemplo a `filter2d_8_ia2`. Se recomienda guardar capturas del prompt, la respuesta, el modelo y la fecha.

**3. Guardar la respuesta sin modificaciones:**

```bash
nano lib/src/ia/filter_ia2.cpp
```

Se pega con `Ctrl+Shift+V`, se guarda con `Ctrl+O` y Enter, y se sale con `Ctrl+X`.

**4. Agregarla a la librería:**

```bash
sed -i 's|\tsrc/ia/filter_ia_opt.cpp|\tsrc/ia/filter_ia_opt.cpp\n\tsrc/ia/filter_ia2.cpp|' lib/CMakeLists.txt
grep "src/ia" lib/CMakeLists.txt
```

**5. Registrarla en el benchmark:**

```bash
nano taller/benchmark.cpp
```

Se busca `PARALLEL_VARIANTS` (con `Ctrl+W`) y se agregan dos líneas:

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

**6. Recompilar y ejecutar:**

```bash
cd build
cmake ..
make -j$(nproc)
cd ../..
bash correr.sh 2
```

Si no compila o aparece `DIFIERE`, debe registrarse: es un resultado válido sobre la calidad del código generado por la IA. Para corregirlo, se devuelven los errores a la misma IA y se documenta cuántos intentos fueron necesarios.

> El script de gráficas solo dibuja `manual_omp`, `ia` e `ia_opt`. Para incluir otra variante, se agrega a la lista `V` al inicio de `graficas/generar_graficas.py`.

---

## 12. Problemas frecuentes

| Síntoma | Causa | Solución |
|---|---|---|
| `nproc` indica 2 (o menos de 8) | La máquina virtual tiene pocos CPUs | [Sección 5.4](#54-si-se-usa-una-máquina-virtual-y-nproc-indica-menos-de-8) |
| `Could not find a package configuration file provided by "OpenCV"` | OpenCV no está instalado | `sudo apt install -y libopencv-dev`, luego borrar `build/` y volver a compilar |
| Lo mismo con `benchmark` o `doctest` | Falta esa librería | `sudo apt install -y libbenchmark-dev doctest-dev` |
| `Unable to locate package libopencv-dev` | Falta el repositorio *universe* | `sudo add-apt-repository universe && sudo apt update`, y repetir la instalación |
| `Could not get lock /var/lib/dpkg/lock` | Otra instalación está en curso | Esperar unos minutos o reiniciar |
| `No such file or directory` al ejecutar `./bin/...` | Carpeta de trabajo incorrecta | Usar `pwd` para verificar; los ejecutables se corren desde `cpu-image-filters/build` |
| `cat: lib/src/...: No such file or directory` | Se está en `build` u otra carpeta | Usar `cd` para ir a la raíz `cpu-image-filters` |
| `basic_string: construction from null is not valid` | A `filter-image` le faltó el tamaño del kernel | Agregar el número al final: `... salida.png 5` |
| `qt.qpa.xcb: could not connect to display` | `filter-image` requiere pantalla | Usarlo desde un escritorio Linux, no por SSH ni WSL sin interfaz gráfica |
| `'size_t' does not name a type` o `'clamp_to' was not declared` | El código de IA está incompleto | Ver [sección 2.3](#23-paralelo-ia); el `filter_ia.cpp` de este paquete ya está corregido |
| `Configuring incomplete, errors occurred!` | Falló algo en CMake | Revisar las líneas `CMake Error` anteriores; casi siempre es una librería faltante |
| CMake sigue fallando después de instalar lo faltante | Caché del intento anterior | `cd cpu-image-filters && rm -rf build` y volver a ejecutar `bash preparar.sh` |
| Aparecen filas `DIFIERE` / `NO` | Esa variante no produce la misma imagen | Si es una variante nueva, revisar el código; las del paquete deben dar todas `OK` |
| No aparecen filas de `ia` en los resultados | La variante no está registrada o `make` falló | Revisar la [sección 11](#11-cómo-agregar-una-versión-de-ia-propia), pasos 4 y 5, y que `make` termine sin errores |
| `crudos` no tiene 7681 líneas | Corrida incompleta o con otro número de repeticiones | Volver a ejecutar `bash correr.sh` sin número |
| Error de NumPy al generar gráficas (`compiled using NumPy 1.x`) | Mezcla de paquetes de `pip` y de `apt` | `sudo apt install --reinstall python3-numpy python3-pandas python3-matplotlib` |

---

## 13. Transparencia sobre el uso de IA

| Parte | Origen |
|---|---|
| Secuencial y Paralelo manual | Repositorio de akowalew y marcin-jamroz, diciembre de 2019, escrito a mano |
| Paralelo IA | Generado por IA a partir del código secuencial; segunda respuesta, tras devolverle los errores de compilación |
| Optimización de IA + Opt. | La idea de invertir los bucles y el código final se obtuvieron en una consulta adicional a un asistente de IA |
| Programa de medición (`benchmark.cpp`), scripts y gráficas | Escritos con ayuda de un asistente de IA |
| Correcciones a `main.cpp` | Corrección de errores del programa de ejemplo del repo original |

En la metodología del artículo se indica de forma explícita que la variante "IA + Opt." se optimizó con asistencia de IA y no de forma completamente manual.

---

## 14. Mini glosario de Linux

| Comando | Qué hace |
|---|---|
| `pwd` | Muestra en qué carpeta se está |
| `ls` | Lista el contenido de la carpeta |
| `cd carpeta` | Entra a una carpeta |
| `cd ..` | Sube una carpeta |
| `cd ~` | Va a la carpeta personal |
| `cat archivo` | Muestra el contenido de un archivo |
| `nano archivo` | Abre un editor de texto en la terminal (`Ctrl+O` guarda, `Ctrl+X` sale, `Ctrl+W` busca) |
| `cp origen destino` | Copia archivos |
| `rm archivo` | Borra un archivo (no hay papelera) |
| `mkdir -p carpeta` | Crea una carpeta |
| `xdg-open archivo` | Abre un archivo o carpeta con el programa por defecto |
| `nproc` | Número de procesadores |
| `Tab` | Autocompleta nombres de archivos y carpetas |
| `Ctrl+C` | Detiene el programa en ejecución |
| `Ctrl+Shift+C` / `Ctrl+Shift+V` | Copiar / pegar en la terminal |

---

**Repositorio original:** https://github.com/akowalew/cpu-image-filters (licencia MIT), autores akowalew y marcin-jamroz.
