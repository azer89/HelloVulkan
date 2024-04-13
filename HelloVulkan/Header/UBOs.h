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
	alignas(4)
	float cameraNear;
	alignas(4)
	float cameraFar;
};

// Needed for generating rays
struct RaytracingUBO
{
	alignas(16)
	glm::mat4 projectionInverse;
	alignas(16)
	glm::mat4 viewInverse;
	alignas(16)
	glm::vec4 cameraPosition;
	alignas(16)
	glm::vec4 shadowCasterPosition;
	alignas(4)
	uint32_t frame;
	alignas(4)
	uint32_t sampleCountPerFrame;
	alignas(4)
	uint32_t currentSampleCount;
	alignas(4)
	uint32_t rayBounceCount;
	alignas(4)
	float skyIntensity;
	alignas(4)
	float lightIntensity;
	alignas(4)
	float specularFuzziness;
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

struct SSAOUBO
{
	alignas(16)
	glm::mat4 projection;
	alignas(4)
	float radius;
	alignas(4)
	float bias;
	alignas(4)
	float screenWidth;
	alignas(4)
	float screenHeight;
	alignas(4)
	float noiseSize;
};

#endif
