#ifndef CUBE_PUSH_CONSTANT
#define CUBE_PUSH_CONSTANT

#include <cstdint>

enum class Distribution : unsigned int
{
	Lambertian = 0, // Diffuse
	GGX = 1, // Specular
};

struct PushConstant
{
	float roughness = 0.f;
	float lodBias = 0.f;
	uint32_t sampleCount = 1u;
	uint32_t mipLevel = 1u;
	uint32_t width = 1024u;
	Distribution distribution = Distribution::Lambertian;
};

#endif
