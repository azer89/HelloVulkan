#ifndef UNIFORM_BUFFER_OBJECTS
#define UNIFORM_BUFFER_OBJECTS

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

// Shadow mapping
struct ShadowMapUBO
{
	alignas(16)
	glm::mat4 lightSpaceMatrix;
};

// Per model transformation matrix
struct ModelUBO
{
	alignas(16)
	glm::mat4 model;
};

// For clustered forward
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
