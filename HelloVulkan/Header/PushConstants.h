#ifndef PUSH_CONSTANTS
#define PUSH_CONSTANTS

#include <cstdint>

// For IBL Lookup Table
struct PushConstantsBRDFLUT
{
	uint32_t width;
	uint32_t height;
	uint32_t sampleCount;
};

// For generating specular and diffuse maps
struct PushConstantCubeFilter
{
	float roughness = 0.f;
	uint32_t outputDiffuseSampleCount = 1u;
};

// Additional customization for PBR
struct PushConstantPBR
{
	float lightIntensity = 1.f;
	float baseReflectivity = 0.04f;
	float maxReflectionLod = 4.f;
	float lightFalloff = 1.0f; // Small --> slower falloff, Big --> faster falloff
	float albedoMultipler = 0.0f; // Show albedo color if the scene is too dark, default value should be zero
};

#endif