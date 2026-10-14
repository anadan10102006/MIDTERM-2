# Programa de medicion para el taller (CL11)
add_executable(benchmark benchmark.cpp)

target_link_libraries(benchmark
    PRIVATE
        image-filters
        ${OpenCV_LIBRARIES}
        OpenMP::OpenMP_CXX
)

set_target_properties(benchmark PROPERTIES CXX_STANDARD 17)
target_compile_options(benchmark PRIVATE -O3)
