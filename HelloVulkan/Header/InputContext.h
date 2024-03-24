#ifndef INPUT_CONTEXT
#define INPUT_CONTEXT

#include "UBOs.h"
#include "PushConstants.h"

#include <array>

struct InputContext
{
public:
	// TODO Initialize these structs
	CameraUBO cameraUBO_ = {};
	PushConstPBR pbrPC_ = {};

	std::array<float, 3> shadowCasterPosition_ = {0.f, 0.f, 0.f};
	bool renderInfiniteGrid_ = true;
	bool renderAABB_ = false;
	bool renderLights_ = true;
};

#endif