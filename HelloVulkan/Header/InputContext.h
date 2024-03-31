#ifndef INPUT_CONTEXT
#define INPUT_CONTEXT

#include "UBOs.h"
#include "PushConstants.h"

#include <array>

namespace EditMode
{
	constexpr int None = 0;
	constexpr int Translate = 1;
	constexpr int Rotate = 2;
	constexpr int Scale = 3;
};

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
	float shadowOrthoSize_ = 20.0f;
	float shadowMinBias_ = 0.001f;
	float shadowMaxBias_ = 0.001f;

	// Mouse-related
	bool firstMouse_ = true;
	bool leftMousePressed_ = false;
	bool leftMouseHold_ = false;
	bool showImgui_ = true;

	// Mouse position at all times
	float mousePositionX = 0;
	float mousePositionY = 0;

	// Mouse position only when clicked
	float mousePressX = 0;
	float mousePressY = 0;

	// Gizmo
	int editMode_ = 0;
	int selectedModelIndex = -1;
	int selectedInstanceIndex = -1;

	bool renderInfiniteGrid_ = true;
	bool renderDebug_ = false;
	bool renderAABB_ = false;
	bool renderLights_ = true;
};

#endif