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

struct ShadowMapUBO
{
	alignas(16)
	glm::mat4 lightSpaceMatrix;
	alignas(16)
	glm::vec4 lightPosition;
	alignas(4)
	float shadowMinBias;
	alignas(4)
	float shadowMaxBias;
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

// For compute-based frustum culling
struct FrustumUBO
{
	alignas(16)
	glm::vec4 planes[6];
	alignas(16)
	glm::vec4 corners[8];
};

#endif
