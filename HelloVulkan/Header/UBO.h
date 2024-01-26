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
	glm::vec4 cameraPosition;
};

struct ModelUBO
{
	glm::mat4 model;
};

#endif
