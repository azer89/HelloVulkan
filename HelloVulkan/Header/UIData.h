#ifndef USER_INTERFACE_DATA
#define USER_INTERFACE_DATA

#include "PushConstants.h"

#include <array>

namespace GizmoMode
{
	constexpr int None = 0;
	constexpr int Translate = 1;
	constexpr int Rotate = 2;
	constexpr int Scale = 3;
};

/*
Variables used by ImGui and used by pipelines and resources
*/
struct UIData
{
public:
	PushConstPBR pbrPC_ = 
	{
		.lightIntensity = 1.75f,
		.baseReflectivity = 0.01f,
		.maxReflectionLod = 1.f,
		.lightFalloff = 0.1f,
		.albedoMultipler = 0.1f
	};

	std::array<float, 3> shadowCasterPosition_ = {0.f, 0.f, 0.f};
	float shadowNearPlane_ = 0.1f;
	float shadowFarPlane_ = 50.f;
	float shadowOrthoSize_ = 15.0f;
	float shadowMinBias_ = 0.001f;
	float shadowMaxBias_ = 0.001f;

	// Mouse-related
	bool mouseFirstUse_ = true;
	bool mouseLeftPressed_ = false; // Left button pressed
	bool mouseLeftHold_ = false; // Left button hold

	// Mouse position at all times
	float mousePositionX_ = 0;
	float mousePositionY_ = 0;

	// Mouse position only when clicked
	float mousePressX_ = 0;
	float mousePressY_ = 0;

	bool imguiShow_ = true;

	int gizmoMode_ = 0;
	int gizmoModelIndex = -1;
	int gizmoInstanceIndex = -1;

	bool renderInfiniteGrid_ = true;
	bool renderDebug_ = false;
	bool renderAABB_ = false;
	bool renderLights_ = true;

	// SSAO
	float ssaoRadius_ = 0.25f;
	float ssaoBias_ = 0.05f;
	float ssaoPower_ = 1.0f;

public:
	bool GizmoCanSelect() const { return mouseLeftPressed_ && gizmoMode_ != 0; }
	bool GizmoActive() const { return gizmoModelIndex > 0 && gizmoInstanceIndex > 0; }
};

#endif