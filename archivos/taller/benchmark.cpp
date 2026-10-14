// benchmark.cpp
//
// Programa de medicion para el taller CL11 de Programacion Paralela.
// No modifica los filtros del repo: solo los llama y mide su tiempo.
//
// Recorre 4 tamanos de imagen x 4 kernels x 5 cantidades de hilos,
// repite cada configuracion N veces (30 por defecto), mide solo el
// tiempo del filtrado y valida cada salida contra la version secuencial.
//
// Uso:
//   ./bin/benchmark <img512> <img1024> <img2048> <img4096> [repeticiones]

#include <omp.h>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "filter_seq.hpp"
#include "filter_omp.hpp"
#include "kernel.hpp"

// Firma comun que deben tener todas las variantes
using FilterFn = void (*)(const ImageU8&, ImageU8&, const Image32F&);

struct Variant
{
    std::string name;
    FilterFn fn;
};

// ---------------------------------------------------------------------------
// Variantes paralelas a comparar.
// Cuando tengas las versiones de IA, declaralas arriba y agregalas aqui:
//   void filter2d_8_ia(const ImageU8&, ImageU8&, const Image32F&);
//   {"ia", filter2d_8_ia},
// ---------------------------------------------------------------------------
void filter2d_8_ia(const ImageU8& src, ImageU8& dst, const Image32F& kernel);
void filter2d_8_ia_opt(const ImageU8& src, ImageU8& dst, const Image32F& kernel);

static const std::vector<Variant> PARALLEL_VARIANTS = {
    {"manual_omp", filter2d_8_omp},
    {"ia",         filter2d_8_ia},
    {"ia_opt",     filter2d_8_ia_opt},
};

static const int SIZES[] = {512, 1024, 2048, 4096};
static const int KERNELS[] = {3, 5, 7, 11};
static const int THREADS[] = {1, 2, 4, 8, 16};

struct Stats
{
    double mean;
    double stdev;
};

static Stats compute_stats(const std::vector<double>& t)
{
    double sum = 0.0;
    for (double x : t) sum += x;
    const double mean = sum / t.size();

    double acc = 0.0;
    for (double x : t) acc += (x - mean) * (x - mean);
    const double stdev = (t.size() > 1) ? std::sqrt(acc / (t.size() - 1)) : 0.0;

    return {mean, stdev};
}

// Carga la imagen en escala de grises y la deja exactamente de size x size
static ImageU8 load_image(const std::string& path, int size)
{
    cv::Mat img = cv::imread(path, cv::IMREAD_GRAYSCALE);
    if (img.empty())
    {
        std::fprintf(stderr, "No se pudo leer la imagen: %s\n", path.c_str());
        std::exit(1);
    }

    if (img.cols != size || img.rows != size)
    {
        std::printf("  Aviso: %s mide %dx%d, se redimensiona a %dx%d\n",
                    path.c_str(), img.cols, img.rows, size, size);
        cv::Mat resized;
        cv::resize(img, resized, cv::Size(size, size), 0, 0, cv::INTER_AREA);
        img = resized;
    }

    return ImageU8(img.clone());
}

// Ejecuta un filtro 'reps' veces y devuelve los tiempos en ms.
// Solo se cronometra la llamada al filtro.
static std::vector<double> run(FilterFn fn, const ImageU8& src, ImageU8& dst,
                               const Image32F& kernel, int reps)
{
    // Corrida de calentamiento (no se cuenta)
    dst.setTo(0);
    fn(src, dst, kernel);

    std::vector<double> times;
    times.reserve(reps);

    for (int r = 0; r < reps; ++r)
    {
        dst.setTo(0);
        const double t0 = omp_get_wtime();
        fn(src, dst, kernel);
        const double t1 = omp_get_wtime();
        times.push_back((t1 - t0) * 1000.0);
    }

    return times;
}

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        std::printf("Uso: %s <img512> <img1024> <img2048> <img4096> [repeticiones]\n", argv[0]);
        return 1;
    }

    const int reps = (argc >= 6) ? std::atoi(argv[5]) : 30;

    // Que OpenMP use exactamente los hilos que le pedimos
    omp_set_dynamic(0);

    std::printf("Nucleos logicos disponibles: %d\n", omp_get_num_procs());
    std::printf("Repeticiones por configuracion: %d\n\n", reps);

    std::ofstream raw("resultados_crudos.csv");
    std::ofstream summary("resultados_resumen.csv");
    raw << "variante,tamano,kernel,hilos,repeticion,tiempo_ms\n";
    summary << "variante,tamano,kernel,hilos,media_ms,desv_ms,speedup,eficiencia,valido\n";

    for (int s = 0; s < 4; ++s)
    {
        const int size = SIZES[s];
        std::printf("Imagen %dx%d\n", size, size);
        const ImageU8 src = load_image(argv[1 + s], size);

        for (int k : KERNELS)
        {
            Image32F kernel(k, k);
            low_pass_kernel(kernel);

            // --- Secuencial: base de referencia (T1) ---
            ImageU8 ref(size, size);
            const auto seq_times = run(filter2d_8_seq, src, ref, kernel, reps);
            const Stats seq = compute_stats(seq_times);

            for (int r = 0; r < reps; ++r)
                raw << "secuencial," << size << ',' << k << ",1," << r + 1 << ','
                    << seq_times[r] << '\n';

            summary << "secuencial," << size << ',' << k << ",1," << seq.mean << ','
                    << seq.stdev << ",1,1,si\n";

            std::printf("  kernel %2dx%-2d  secuencial        %10.3f ms\n", k, k, seq.mean);

            // --- Variantes paralelas ---
            for (const auto& v : PARALLEL_VARIANTS)
            {
                for (int p : THREADS)
                {
                    omp_set_num_threads(p);

                    ImageU8 dst(size, size);
                    const auto times = run(v.fn, src, dst, kernel, reps);
                    const Stats st = compute_stats(times);

                    // Validacion: la salida debe ser identica a la secuencial
                    const bool ok = (cv::countNonZero(dst != ref) == 0);

                    const double speedup = seq.mean / st.mean;
                    const double eficiencia = speedup / p;

                    for (int r = 0; r < reps; ++r)
                        raw << v.name << ',' << size << ',' << k << ',' << p << ','
                            << r + 1 << ',' << times[r] << '\n';

                    summary << v.name << ',' << size << ',' << k << ',' << p << ','
                            << st.mean << ',' << st.stdev << ',' << speedup << ','
                            << eficiencia << ',' << (ok ? "si" : "NO") << '\n';

                    std::printf("  kernel %2dx%-2d  %-12s %2d h %10.3f ms  Sp=%.2f  Ep=%.2f  %s\n",
                                k, k, v.name.c_str(), p, st.mean, speedup, eficiencia,
                                ok ? "OK" : "DIFIERE");
                }
            }
        }
        std::printf("\n");
    }

    std::printf("Listo. Archivos: resultados_crudos.csv y resultados_resumen.csv\n");
    return 0;
}
