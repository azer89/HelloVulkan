#ifndef UNIFORM_BUFFER_OBJECTS
#define UNIFORM_BUFFER_OBJECTS

#include "glm/glm.hpp"

struct PerFrameUBO
{
	alignas(16)
	glm::mat4 cameraProjection;
	alignas(16)
	glm::mat4 cameraView;
	alignas(16)
	glm::vec4 cameraPosition; // TODO change to vec3
};

struct ModelUBO
{
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
	unsigned int sliceCountX;
	alignas(4)
	unsigned int sliceCountY;
	alignas(4)
	unsigned int sliceCountZ;
};

#endif
