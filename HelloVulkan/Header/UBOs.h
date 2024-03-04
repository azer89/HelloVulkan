#ifndef UNIFORM_BUFFER_OBJECTS
#define UNIFORM_BUFFER_OBJECTS

#include "Configs.h"
#include "glm/glm.hpp"

// General-purpose
struct CameraUBO
{
	alignas(16)
	glm::mat4 projection;
	alignas(16)
	glm::mat4 view;
	alignas(16)
	glm::vec4 position;
};

// Needed for generating rays
struct RaytracingCameraUBO
{
	alignas(16)
	glm::mat4 projectionInverse;
	alignas(16)
	glm::mat4 viewInverse;
};

struct ShadowMapUBO
{
	alignas(16)
	glm::mat4 lightSpaceMatrices[ShadowConfig::CascadeCount];
	alignas(16)
	glm::vec4 splitValues;
	alignas(16)
	glm::vec4 lightPosition;
	alignas(4)
	float shadowMinBias;
	alignas(4)
	float shadowMaxBias;
	alignas(4)
	float cameraNearPlane;
	alignas(4)
	float cameraFarPlane;
	alignas(4)
	float poissonSize;
};

// Per model transformation matrix
struct ModelUBO
{
	alignas(16)
	glm::mat4 model;
};

struct ClusterForwardUBO
{
	alignas(16)
	glm::mat4 cameraInverseProjection;
	alignas(16)
	glm::mat4 cameraView;
	alignas(8)
	glm::vec2 screenSize;
	alignas(4)
	float sliceScaling;
	alignas(4)
	float sliceBias;
	alignas(4)
	float cameraNear;
	alignas(4)
	float cameraFar;
	alignas(4)
	uint32_t sliceCountX;
	alignas(4)
	uint32_t sliceCountY;
	alignas(4)
	uint32_t sliceCountZ;
};

#endif
