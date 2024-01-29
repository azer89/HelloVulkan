#ifndef UNIFORM_BUFFER_OBJECTS
#define UNIFORM_BUFFER_OBJECTS

#include "glm/glm.hpp"

// TODO Separate this into multiple UBOs
struct CameraUBO
{
	alignas(16)
	glm::mat4 cameraProjection;
	alignas(16)
	glm::mat4 cameraView;
	alignas(16)
	glm::vec4 cameraPosition;
	alignas(4)
	float cameraNear;
	alignas(4)
	float cameraFar;
};

struct ModelUBO
{
	alignas(16)
	glm::mat4 model;
};

#endif
