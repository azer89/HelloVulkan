#ifndef UTILITY_HEADER
#define UTILITY_HEADER

#include <random>
#include <span>

namespace Utility
{
	inline float RandomNumber()
	{
		static std::uniform_real_distribution<float> distribution(0.0, 1.0);
		static std::random_device rd;
		static std::mt19937 generator(rd());
		return distribution(generator);
	}

	inline float RandomNumber(float min, float max)
	{
		return min + (max - min) * RandomNumber();
	}

	inline uint32_t AlignedSize(uint32_t value, uint32_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	inline float Lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}

	inline uint32_t MipMapCount(uint32_t w, uint32_t h)
	{
		uint32_t levels = 1;
		while ((w | h) >> levels)
		{
			levels += 1;
		}
		return levels;
	}

	inline uint32_t MipMapCount(uint32_t size)
	{
		uint32_t levels = 1;
		while (size >> levels)
		{
			levels += 1;
		}
		return levels;
	}

	template<class T, std::size_t N>
	auto SubSpan(std::span<T, N> s, std::size_t offset, std::size_t width)
	{
		return s.subspan(offset, offset + width <= s.size() ? width : 0u);
	}
}

#endif
