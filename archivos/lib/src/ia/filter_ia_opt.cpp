#include "filter.hpp"

#include <omp.h>

#include <cassert>
#include <cstddef>
#include <limits>
#include <vector>

namespace {

template<typename Dst, typename Src>
Dst clamp_to(Src src)
{
	constexpr auto MaxDst = std::numeric_limits<Dst>::max();
	constexpr auto MinDst = std::numeric_limits<Dst>::min();

	constexpr auto MaxSrc = static_cast<Src>(MaxDst);
	constexpr auto MinSrc = static_cast<Src>(MinDst);

	if(src > MaxSrc)
		return MaxDst;
	else if(src < MinSrc)
		return MinDst;
	else
		return static_cast<Dst>(src);
}

} // namespace

void filter2d_8_ia_opt(const ImageU8& src, ImageU8& dst, const Image32F& kernel)
{
	assert(src.data != dst.data);
	assert(src.isContinuous() && dst.isContinuous());
	assert((src.cols == dst.cols) && (src.rows == dst.rows));
	assert(src.type() == CV_8UC1 && dst.type() == CV_8UC1);
	assert(kernel.cols == kernel.rows);
	assert((kernel.cols % 2) == 1);
	assert(kernel.isContinuous() && kernel.type() == CV_32F);

	const int cols = src.cols;
	const int rows = src.rows;
	const int kernel_size = kernel.cols;
	const int half_kernel_size = kernel_size / 2;

	const int width = cols - 2 * half_kernel_size;
	const int height = rows - 2 * half_kernel_size;

	const unsigned char* const src_base = src.data;
	unsigned char* const dst_base = dst.data + half_kernel_size * cols + half_kernel_size;
	const float* const kernel_base = reinterpret_cast<const float*>(kernel.data);

	#pragma omp parallel
	{
		// Acumuladores privados de cada hilo: uno por pixel de la fila
		std::vector<float> acc(width);

		#pragma omp for schedule(static)
		for(int i = 0; i < height; ++i)
		{
			float* const a = acc.data();

			for(int j = 0; j < width; ++j)
				a[j] = 0.0f;

			// Bucles invertidos: el mas interno recorre toda la fila,
			// lo que permite al compilador vectorizarlo (SIMD).
			// Cada pixel suma en el mismo orden (m, n) que la version
			// secuencial, asi que el resultado es identico.
			for(int m = 0; m < kernel_size; ++m)
			{
				const unsigned char* const src_row =
					src_base + static_cast<std::size_t>(i + m) * cols;

				for(int n = 0; n < kernel_size; ++n)
				{
					const float k = kernel_base[m * kernel_size + n];
					const unsigned char* const s = src_row + n;

					for(int j = 0; j < width; ++j)
						a[j] += s[j] * k;
				}
			}

			unsigned char* const d = dst_base + static_cast<std::size_t>(i) * cols;

			for(int j = 0; j < width; ++j)
				d[j] = clamp_to<unsigned char>(a[j]);
		}
	}
}
