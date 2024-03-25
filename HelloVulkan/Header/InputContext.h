#ifndef INPUT_CONTEXT
#define INPUT_CONTEXT

#include "UBOs.h"
#include "PushConstants.h"

#include <array>

struct InputContext
{
public:
	PushConstPBR pbrPC_ = 
	{
		.lightIntensity = 1.5f,
		.baseReflectivity = 0.01f,
		.maxReflectionLod = 1.f,
		.lightFalloff = 0.1f,
		.albedoMultipler = 0.1f
	};

	std::array<float, 3> shadowCasterPosition_ = {0.f, 0.f, 0.f};
	float shadowNearPlane_ = 0.1f;
	float shadowFarPlane_ = 50.f;
	float shadowOrthoSize_ = 12.5f;
	float shadowMinBias_ = 0.001f;
	float shadowMaxBias_ = 0.001f;

	bool renderInfiniteGrid_ = true;
	bool renderDebug_ = false;
	bool renderAABB_ = false;
	bool renderLights_ = true;
};

#endif