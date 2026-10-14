#include "filter.hpp"

#include <omp.h>

#include <cassert>
#include <cstddef>
#include <limits>

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

void filter2d_8_ia(const ImageU8& src, ImageU8& dst, const Image32F& kernel)
{
	// Filtering cannot be done in-place
	assert(src.data != dst.data);

	// Images must be continuous so we can operate on memory directly
	assert(src.isContinuous() && dst.isContinuous());

	// Images sizes must be equal
	assert((src.cols == dst.cols) && (src.rows == dst.rows));
	const auto cols = src.cols;
	const auto rows = src.rows;

	// Number of channels must be equal
	assert(src.channels() == dst.channels());

	// Images types must be equal to CV_8UC1 - single channel U8
	assert(src.type() == CV_8UC1 && dst.type() == CV_8UC1);
	using Value = unsigned char;

	// Kernel must be squared
	assert(kernel.cols == kernel.rows);
	const auto kernel_size = kernel.cols;

	// Kernel size must be an odd number
	assert((kernel_size % 2) == 1);

	// Kernel must be continuous
	assert(kernel.isContinuous());

	// Kernel must be float-typed, and so is the accumulator
	assert(kernel.type() == CV_32F);
	using Accumulator = float;

	const auto half_kernel_size = (kernel_size / 2);

	const int width = (cols - 2 * half_kernel_size);
	const int height = (rows - 2 * half_kernel_size);

	const auto dst_offset = ((half_kernel_size * cols) + half_kernel_size);

	const unsigned char* const src_base = src.data;
	unsigned char* const dst_base = dst.data + dst_offset;
	const float* const kernel_base = reinterpret_cast<const float*>(kernel.data);

	// Each row is independent: pointers are computed from i instead of
	// being incremented, so iterations can be distributed among threads
	#pragma omp parallel for schedule(static)
	for(int i = 0; i < height; ++i)
	{
		const unsigned char* src_it_j = src_base + static_cast<std::size_t>(i) * cols;
		unsigned char* dst_it_j = dst_base + static_cast<std::size_t>(i) * cols;

		for(int j = 0; j < width; ++j)
		{
			auto acc = Accumulator{0};

			const unsigned char* src_it_m = src_it_j;
			const float* kernel_it = kernel_base;

			// Same accumulation order as the sequential version,
			// so the numerical result is identical
			for(int m = 0; m < kernel_size; ++m)
			{
				const unsigned char* src_it_n = src_it_m;

				for(int n = 0; n < kernel_size; ++n)
				{
					const auto src_value = *(src_it_n);
					const auto kernel_value = *(kernel_it);

					acc += (src_value * kernel_value);

					++src_it_n;
					++kernel_it;
				}

				src_it_m += cols;
			}

			*dst_it_j = clamp_to<Value>(acc);

			++src_it_j;
			++dst_it_j;
		}
	}
}
